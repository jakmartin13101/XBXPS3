// Entry points of the XBXPS3 core: startup, lifetime events and the emulator main thread.
#include "stdafx.h"
#include "xbx_internal.h"

#include "rpcs3_version.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Input/pad_thread.h"
#include "Utilities/File.h"
#include "Utilities/Thread.h"
#include "util/logs.hpp"
#include "util/sysinfo.hpp"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

LOG_CHANNEL(sys_log, "SYS");
LOG_CHANNEL(xbx_log, "XBX");

// Directories consumed by the patched fs::get_config_dir() / get_executable_dir() (patch 0002).
extern std::string g_uwp_executable_dir;
extern std::string g_uwp_config_dir;

// Defined by the desktop frontend (rpcs3.cpp), referenced by pad_config.cpp and pad_thread.cpp.
std::string g_input_config_override;

namespace xbx
{
	namespace
	{
		xbx_host_api s_host{};
		paths s_paths;

		std::unique_ptr<logs::listener> s_log_file;

		// Forwards warnings and worse to the debugger so a Visual Studio remote session sees them live.
		struct debugger_listener final : logs::listener
		{
			void log(u64 /*stamp*/, const logs::message& msg, std::string_view prefix, std::string_view text) override
			{
				if (!s_host.debug_print)
				{
					return;
				}

				if (msg == logs::level::fatal || msg == logs::level::error || msg == logs::level::warning || msg == logs::level::always)
				{
					std::string line;
					line.reserve(prefix.size() + text.size() + 2);
					line += prefix;
					line += text;
					line += '\n';
					s_host.debug_print(s_host.ctx, line.c_str());
				}
			}
		};

		debugger_listener s_debugger_listener;

		// Emulator main thread
		std::mutex s_queue_mutex;
		std::condition_variable s_queue_cv;
		std::deque<std::function<void()>> s_queue;
		bool s_queue_stop = false;
		std::thread s_main_thread;

		std::string with_slash(std::string path)
		{
			std::replace(path.begin(), path.end(), '\\', '/');

			if (!path.empty() && path.back() != '/')
			{
				path += '/';
			}

			return path;
		}

		void init_logging()
		{
			const std::string log_dir = fs::get_log_dir();
			fs::create_path(log_dir);

			fs::device_stat stats{};
			u64 max_size = 256 * 1024 * 1024;

			if (fs::statfs(log_dir, stats) && stats.avail_free / 4 < max_size)
			{
				max_size = stats.avail_free / 4;
			}

			s_log_file = logs::make_file_listener(log_dir + "RPCS3.log", max_size);
			logs::listener::add(&s_debugger_listener);

			logs::stored_message ver{sys_log.always()};
			ver.text = fmt::format("RPCS3 v%s (XBXPS3 Xbox frontend)", rpcs3::get_verbose_version());

			logs::stored_message arch{sys_log.always()};
			arch.text = fmt::format("Architecture: %s", utils::get_architecture());

			logs::stored_message sys{sys_log.always()};
			sys.text = utils::get_system_info();

			logs::stored_message os{sys_log.always()};
			os.text = utils::get_OS_version_string();

			logs::stored_message time{sys_log.always()};
			time.text = fmt::format("Current Time: %s", std::chrono::system_clock::now());

			logs::set_init({std::move(ver), std::move(arch), std::move(sys), std::move(os), std::move(time)});
		}
	}

	const xbx_host_api& host()
	{
		return s_host;
	}

	const paths& get_paths()
	{
		return s_paths;
	}

	void main_thread_start()
	{
		s_main_thread = std::thread([]()
		{
			thread_ctrl::set_name("Main Thread");

			while (true)
			{
				std::function<void()> func;
				{
					std::unique_lock lock(s_queue_mutex);
					s_queue_cv.wait(lock, [] { return s_queue_stop || !s_queue.empty(); });

					if (s_queue.empty())
					{
						break;
					}

					func = std::move(s_queue.front());
					s_queue.pop_front();
				}

				func();
			}
		});
	}

	void main_thread_post(std::function<void()> func)
	{
		{
			std::lock_guard lock(s_queue_mutex);
			s_queue.push_back(std::move(func));
		}

		s_queue_cv.notify_one();
	}

	void main_thread_stop()
	{
		{
			std::lock_guard lock(s_queue_mutex);
			s_queue_stop = true;
		}

		s_queue_cv.notify_one();

		if (s_main_thread.joinable())
		{
			s_main_thread.join();
		}
	}
}

// Called by RPCS3 (Utilities/Thread.cpp, Emu/System.cpp, ...) on unrecoverable errors.
[[noreturn]] void report_fatal_error(std::string_view text, bool /*is_html*/, bool include_help_text)
{
	std::string message{text};

	if (include_help_text)
	{
		message += "\n\nThe log is in LocalState/log/RPCS3.log (Device Portal > File explorer > LocalAppData > XBXPS3).";
	}

	sys_log.fatal("%s", message);
	logs::listener::sync_all();

	if (xbx::host().fatal_error)
	{
		xbx::host().fatal_error(xbx::host().ctx, message.c_str());
	}

	std::abort();
}

extern "C" int xbx_core_init(const xbx_host_api* host, const xbx_core_paths* paths)
{
	if (!host || !paths || !paths->install_dir || !paths->local_state_dir)
	{
		return -1;
	}

	xbx::s_host = *host;
	xbx::s_paths.install_dir = xbx::with_slash(paths->install_dir);
	xbx::s_paths.local_state_dir = xbx::with_slash(paths->local_state_dir);
	xbx::s_paths.usb_root = paths->usb_root ? xbx::with_slash(paths->usb_root) : std::string{};

	// Must happen before anything asks RPCS3 for a path (patch 0002)
	g_uwp_executable_dir = xbx::s_paths.install_dir;
	g_uwp_config_dir = xbx::s_paths.local_state_dir;

	fs::create_path(fs::get_config_dir(true));
	fs::create_path(xbx::s_paths.local_state_dir + "Install/");

	xbx::init_logging();

	xbx_log.notice("Install dir: %s", xbx::s_paths.install_dir);
	xbx_log.notice("LocalState: %s", xbx::s_paths.local_state_dir);
	xbx_log.notice("USB root: %s", xbx::s_paths.usb_root.empty() ? "(none)" : xbx::s_paths.usb_root);

	if (!xbx::vulkan_init())
	{
		xbx_log.error("Vulkan (Dozen) is unavailable. Games will run with the Null renderer and show nothing.");
	}

	xbx::apply_first_run_settings();
	xbx::configure_vfs();
	xbx::install_emu_callbacks();
	xbx::main_thread_start();

	// Same sequence as main_application::InitializeEmulator, on the emulator main thread
	atomic_t<u32> done = 0;
	xbx::main_thread_post([&]()
	{
		Emu.SetHasGui(true);
		Emu.SetHeadless(false);
		Emu.SetUsr("00000001");
		Emu.Init();

		const std::string firmware_version = utils::get_firmware_version();
		sys_log.always()("%s", firmware_version.empty() ? "Missing Firmware" : ("Firmware version: " + firmware_version));

		rpcs3::utils::configure_logs(Emu.IsStopped());

		done = 1;
		done.notify_one();
	});

	while (!done)
	{
		done.wait(0);
	}
	return 0;
}

extern "C" void xbx_core_start()
{
	xbx::main_thread_post([]()
	{
		if (!Emu.BootBigPictureMode())
		{
			xbx_log.error("Failed to boot Big Picture Mode");
			return;
		}

		// Imports run on their own thread and report through the native progress dialog,
		// which Big Picture Mode has just started.
		std::thread([]()
		{
			thread_ctrl::set_name("XBX Importer");
			xbx::import_pending_files();
		}).detach();
	});
}

extern "C" void xbx_core_suspend()
{
	// The console suspends the whole process shortly after this returns.
	// Freeze emulation so no thread is mid-frame when the GPU and audio go away.
	atomic_t<u32> done = 0;
	xbx::main_thread_post([&]()
	{
		if (Emu.IsRunning())
		{
			Emu.Pause(true, false);
		}

		logs::listener::sync_all();
		done = 1;
		done.notify_one();
	});

	while (!done)
	{
		done.wait(0);
	}
}

extern "C" void xbx_core_resume()
{
	xbx::main_thread_post([]()
	{
		if (Emu.IsPaused())
		{
			Emu.Resume();
		}
	});
}

extern "C" void xbx_core_open_home_menu()
{
	pad::g_home_menu_requested = true;
}

extern "C" void xbx_core_shutdown()
{
	atomic_t<u32> done = 0;
	xbx::main_thread_post([&]()
	{
		if (!Emu.IsStopped())
		{
			Emu.Kill(false);
		}

		done = 1;
		done.notify_one();
	});

	while (!done)
	{
		done.wait(0);
	}
	xbx::main_thread_stop();
	logs::listener::sync_all();
}
