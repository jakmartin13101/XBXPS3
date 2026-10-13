// g_emu_callbacks for Xbox. Modeled on headless_application.cpp and main_application.cpp,
// with every Qt dependency replaced. All dialogs use RPCS3's native RSX overlays, which work
// with a gamepad and need no windowing toolkit.
#include "stdafx.h"
#include "xbx_internal.h"
#include "xbx_gs_frame.h"

#include "Emu/emu_callbacks.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/IdManager.h"
#include "Emu/localized_string_id.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/Null/NullAudioBackend.h"
#include "Emu/Audio/Null/null_enumerator.h"
#include "Emu/Audio/XAudio2/XAudio2Backend.h"
#include "Emu/Audio/XAudio2/xaudio2_enumerator.h"
#include "Emu/Cell/Modules/cellAudio.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Emu/Cell/lv2/sys_rsxaudio.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/null_music_handler.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/VK/VKGSRender.h"
#include "Emu/RSX/Overlays/overlay_perf_metrics.h"
#include "Emu/RSX/Overlays/overlay_debug_overlay.h"
#include "Emu/RSX/Overlays/BigPicture/overlay_big_picture.h"
#include "Input/pad_thread.h"
#include "Utilities/File.h"
#include "util/video_source.h"

#include <chrono>
#include <filesystem>

LOG_CHANNEL(sys_log, "SYS");
LOG_CHANNEL(xbx_log, "XBX");

[[noreturn]] void report_fatal_error(std::string_view text, bool is_html = false, bool include_help_text = true);

namespace xbx
{
	namespace
	{
		void on_settings_changed()
		{
			rpcs3::utils::configure_logs(Emu.IsStopped());

			if (!Emu.IsStopped())
			{
				g_cfg.audio.provider.set(Emu.IsVsh() ? audio_provider::rsxaudio : audio_provider::cell_audio);
			}

			audio::configure_audio();
			audio::configure_rsxaudio();
			rsx::overlays::reset_performance_overlay();
			rsx::overlays::reset_debug_overlay();
		}

		// The launcher is the home screen of the app, so it must always come back. The core already
		// re-enters Big Picture Mode when a game launched from it stops. This catches the other
		// cases: a failed boot, or "Exit Big Picture Mode", which on Xbox reloads the game list.
		// Leaving the app is done with the Xbox button, like any other console app.
		void return_to_launcher_after_stop()
		{
			std::thread([]()
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(750));

				main_thread_post([]()
				{
					if (!shutting_down() && Emu.IsStopped(true) && !g_big_picture_mode_active)
					{
						xbx_log.notice("Emulation stopped, returning to Big Picture Mode");
						Emu.BootBigPictureMode();
					}
				});
			}).detach();
		}

		std::filesystem::path to_path(std::string_view utf8)
		{
			return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
		}

		std::string to_utf8(const std::filesystem::path& path)
		{
			const std::u8string s = path.generic_u8string();
			return std::string(s.begin(), s.end());
		}

		std::string timestamp_for_filename()
		{
			const auto now = std::chrono::system_clock::now();
			const auto t = std::chrono::system_clock::to_time_t(now);
			std::tm tm{};
			localtime_s(&tm, &t);
			return fmt::format("%04d-%02d-%02d_%02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		}
	}

	void install_emu_callbacks()
	{
		emu_callbacks& cb = g_emu_callbacks;

		cb.call_from_main_thread = [](std::function<void()> func, atomic_t<u32>* wake_up)
		{
			main_thread_post([func = std::move(func), wake_up]()
			{
				func();

				if (wake_up)
				{
					*wake_up = true;
					wake_up->notify_one();
				}
			});
		};

		cb.try_to_quit = [](bool /*force_quit*/, std::function<void()> on_exit) -> bool
		{
			// There is no confirmation dialog on Xbox: the Big Picture menu already asked.
			if (on_exit)
			{
				on_exit();
			}

			if (host().request_exit)
			{
				host().request_exit(host().ctx);
			}

			return true;
		};

		cb.update_emu_settings = []()
		{
			Emu.CallFromMainThread([]() { on_settings_changed(); });
		};

		cb.save_emu_settings = []()
		{
			Emu.BlockingCallFromMainThread([]()
			{
				Emulator::SaveSettings(g_cfg.to_string(), Emu.GetTitleID());
			});
		};

		// Video
		cb.get_gs_frame = []() -> std::unique_ptr<GSFrameBase>
		{
			return std::make_unique<xbx_gs_frame>();
		};

		cb.close_gs_frame = []() {};

		cb.init_gs_render = [](utils::serial* ar)
		{
			video_renderer type = g_cfg.video.renderer;

			if (type == video_renderer::opengl)
			{
				// There is no OpenGL on Xbox. Dozen gives us Vulkan.
				xbx_log.warning("OpenGL is not available on Xbox, using Vulkan");
				type = video_renderer::vulkan;
			}

			if (type == video_renderer::vulkan && !vulkan_available())
			{
				xbx_log.error("Vulkan driver missing, falling back to the Null renderer");
				type = video_renderer::null;
			}

			switch (type)
			{
			case video_renderer::vulkan:
				g_fxo->init<rsx::thread, named_thread<VKGSRender>>(ar);
				break;
			case video_renderer::null:
			default:
				g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
				break;
			}
		};

		// Input: Xbox pads through XInput (xinputuap). Keyboard and mouse are not emulated yet.
		cb.init_kb_handler = []()
		{
			ensure(g_fxo->init<KeyboardHandlerBase, NullKeyboardHandler>(Emu.DeserialManager()));
		};

		cb.init_mouse_handler = []()
		{
			ensure(g_fxo->init<MouseHandlerBase, NullMouseHandler>(Emu.DeserialManager()));
		};

		cb.init_pad_handler = [](std::string_view title_id)
		{
			ensure(g_fxo->init<named_thread<pad_thread>>(nullptr, nullptr, title_id));
		};

		// Audio: XAudio2 is the only output API on Xbox (patch 0004 removes MMDevice use)
		cb.get_audio = []() -> std::shared_ptr<AudioBackend>
		{
			std::shared_ptr<AudioBackend> result;

			if (g_cfg.audio.renderer.get() != audio_renderer::null)
			{
				result = std::make_shared<XAudio2Backend>();
			}

			if (!result || !result->Initialized())
			{
				sys_log.error("Audio renderer could not be initialized, using the Null renderer");
				result = std::make_shared<NullAudioBackend>();
			}

			return result;
		};

		cb.get_audio_enumerator = [](u64 renderer) -> std::shared_ptr<audio_device_enumerator>
		{
			if (static_cast<audio_renderer>(renderer) == audio_renderer::null)
			{
				return std::make_shared<null_enumerator>();
			}

			return std::make_shared<xaudio2_enumerator>();
		};

		cb.get_camera_handler = []() -> std::shared_ptr<camera_handler_base>
		{
			return std::make_shared<null_camera_handler>();
		};

		cb.get_music_handler = []() -> std::shared_ptr<music_handler_base>
		{
			return std::make_shared<null_music_handler>();
		};

		// Dialogs: returning null makes RPCS3 use its native overlays (use_native_interface is forced on)
		cb.get_msg_dialog                 = []() -> std::shared_ptr<MsgDialogBase> { return nullptr; };
		cb.get_osk_dialog                 = []() -> std::shared_ptr<OskDialogBase> { return nullptr; };
		cb.get_save_dialog                = []() -> std::unique_ptr<SaveDialogBase> { return nullptr; };
		cb.get_sendmessage_dialog         = []() -> std::shared_ptr<SendMessageDialogBase> { return nullptr; };
		cb.get_recvmessage_dialog         = []() -> std::shared_ptr<RecvMessageDialogBase> { return nullptr; };
		cb.get_trophy_notification_dialog = []() -> std::unique_ptr<TrophyNotificationBase> { return nullptr; };

		// Strings for the overlays (generated from rpcs3qt/localized_emu.h at build time)
		cb.get_localized_string = [](localized_string_id id, const char* args) -> std::string
		{
			return get_localized_string(static_cast<int>(id), args);
		};

		cb.get_localized_u32string = [](localized_string_id id, const char* args) -> std::u32string
		{
			return get_localized_u32string(static_cast<int>(id), args);
		};

		cb.get_localized_setting = [](const cfg::_base* node, u32 index) -> std::string
		{
			if (!node)
			{
				return {};
			}

			const std::vector<std::string> list = node->to_list();
			return index < list.size() ? list[index] : std::string{};
		};

		// Lifecycle
		cb.on_run = [](bool /*start_playtime*/)
		{
			if (host().set_display_sleep)
			{
				host().set_display_sleep(host().ctx, 0);
			}
		};

		cb.on_pause = []() {};
		cb.on_resume = []() {};
		cb.on_ready = []() {};

		cb.on_stop = []()
		{
			if (host().set_display_sleep)
			{
				host().set_display_sleep(host().ctx, 1);
			}

			return_to_launcher_after_stop();
		};

		cb.on_missing_fw = []()
		{
			xbx_log.error("PS3 firmware is missing. Copy PS3UPDAT.PUP into the Install folder and restart XBXPS3.");
		};

		cb.on_emulation_stop_no_response = [](std::shared_ptr<atomic_t<bool>> closed_successfully, int /*seconds_waiting_already*/)
		{
			if (!closed_successfully || !*closed_successfully)
			{
				report_fatal_error("Stopping the emulator took too long. A thread has probably deadlocked.", false, true);
			}
		};

		cb.on_save_state_progress = [](std::shared_ptr<atomic_t<bool>>, stx::shared_ptr<utils::serial>, stx::atomic_ptr<std::string>*, std::shared_ptr<void>) {};
		cb.enable_disc_eject = [](bool) {};
		cb.enable_disc_insert = [](bool) {};
		cb.handle_taskbar_progress = [](s32, s32) {};
		cb.add_breakpoint = [](u32) {};
		cb.check_microphone_permissions = []() {};
		cb.make_video_source = []() -> std::unique_ptr<video_source> { return nullptr; };
		cb.enable_gamemode = [](bool) {};
		cb.play_sound = [](const std::string&, std::optional<f32>) {};

		cb.display_sleep_control_supported = []() { return true; };
		cb.enable_display_sleep = [](bool enabled)
		{
			if (host().set_display_sleep)
			{
				host().set_display_sleep(host().ctx, enabled ? 1 : 0);
			}
		};

		// Files and images
		cb.get_image_info = [](const std::string& filename, std::string& sub_type, s32& width, s32& height, s32& orientation) -> bool
		{
			int w = 0, h = 0, o = 0;
			const bool ok = xbx::get_image_info(filename, sub_type, w, h, o);
			width = w;
			height = h;
			orientation = o;
			return ok;
		};

		cb.get_scaled_image = [](const std::string& path, s32 target_width, s32 target_height, s32& width, s32& height, u8* dst, bool force_fit) -> bool
		{
			int w = 0, h = 0;
			const bool ok = xbx::get_scaled_image(path, target_width, target_height, w, h, dst, force_fit);
			width = w;
			height = h;
			return ok;
		};

		cb.resolve_path = [](std::string_view sv) -> std::string
		{
			std::error_code ec;
			const std::filesystem::path path = to_path(sv);

			if (!std::filesystem::exists(path, ec))
			{
				return {};
			}

			return to_utf8(std::filesystem::canonical(path, ec));
		};

		cb.resolve_path_may_not_exist = [](std::string_view sv) -> std::string
		{
			std::error_code ec;
			return to_utf8(std::filesystem::absolute(to_path(sv), ec).lexically_normal());
		};

		cb.get_font_dirs = []() -> std::vector<std::string>
		{
			// Fonts shipped in the package first, then the console's own fonts if the sandbox allows reading them.
			// The PS3 firmware fonts in dev_flash are searched by RPCS3 itself.
			return { get_paths().install_dir + "Fonts/", "C:/Windows/Fonts/" };
		};

		cb.get_photo_path = [](std::string_view title) -> std::string
		{
			const std::string dir = get_paths().local_state_dir + "photos/" + std::string(title) + "/";
			fs::create_path(dir);
			return dir + timestamp_for_filename() + ".png";
		};

		cb.on_install_pkgs = [](const std::vector<std::string>& pkgs, bool from_optical_drive) -> bool
		{
			for (const std::string& pkg : pkgs)
			{
				if (!rpcs3::utils::install_pkg(pkg, from_optical_drive))
				{
					sys_log.error("Failed to install %s", pkg);
					return false;
				}
			}

			return true;
		};

		// The per-game config database needs the Qt network stack; not available yet.
		cb.get_database_config = [](const std::string&) -> std::string { return {}; };
	}
}
