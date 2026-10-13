// Xbox defaults for a fresh install, and where the virtual PS3 drives live.
#include "stdafx.h"
#include "xbx_internal.h"

#include "Emu/system_config.h"
#include "Emu/vfs_config.h"
#include "Emu/Io/pad_config.h"
#include "Utilities/File.h"

LOG_CHANNEL(xbx_log, "XBX");

namespace xbx
{
	void apply_first_run_settings()
	{
		const std::string cfg_path = fs::get_config_dir(true) + "config.yml";

		if (!fs::is_file(cfg_path))
		{
			// g_cfg still holds RPCS3's defaults here; Emu.Init() loads this file right after.
			g_cfg.video.renderer.set(video_renderer::vulkan);
			g_cfg.video.vk.exclusive_fullscreen_mode.set(vk_exclusive_fs_mode::unspecified);
			g_cfg.audio.renderer.set(audio_renderer::xaudio);
			g_cfg.misc.use_native_interface.set(true); // All dialogs and the launcher are RSX overlays

			if (fs::write_file(cfg_path, fs::rewrite, g_cfg.to_string()))
			{
				xbx_log.success("Wrote Xbox defaults to %s", cfg_path);
			}
			else
			{
				xbx_log.error("Could not write %s (%s)", cfg_path, fs::g_tls_error);
			}
		}

		// Up to four Xbox pads, mapped to PS3 players 1-4 through XInput
		// load() returns false when the global input profile does not exist yet (first run)
		if (!g_cfg_input.load())
		{
			for (u32 i = 0; i < 4; i++)
			{
				cfg_player* player = g_cfg_input.player[i];
				player->handler.set(pad_handler::xinput);
				player->device.from_string(fmt::format("XInput Pad #%d", i + 1));
			}

			g_cfg_input.save("", "");
			xbx_log.success("Mapped Xbox controllers 1-4 to PS3 players 1-4");
		}
	}

	void configure_vfs()
	{
		g_cfg_vfs.load();

		// Big installs belong on the USB drive when the user created the folders for it
		if (const std::string& usb = get_paths().usb_root; !usb.empty())
		{
			if (fs::is_dir(usb + "dev_hdd0/"))
			{
				g_cfg_vfs.dev_hdd0.from_string(usb + "dev_hdd0/");
			}

			if (fs::is_dir(usb + "games/"))
			{
				g_cfg_vfs.games_dir.from_string(usb + "games/");
			}
		}

		fs::create_path(g_cfg_vfs.get(g_cfg_vfs.games_dir));
		g_cfg_vfs.save();

		xbx_log.notice("dev_hdd0: %s", g_cfg_vfs.get(g_cfg_vfs.dev_hdd0));
		xbx_log.notice("games: %s", g_cfg_vfs.get(g_cfg_vfs.games_dir));
	}
}
