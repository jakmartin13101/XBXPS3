// Drop-folder installer. A console has no file picker worth using, so files are imported
// from two folders at startup:
//   LocalState/Install/   (copy with Device Portal > File explorer)
//   E:/XBXPS3/Install/    (NTFS USB drive, see docs/DEPLOYING.md)
// PS3UPDAT.PUP installs the firmware, *.pkg are installed as games/DLC/updates, and
// *.rap / *.edat licenses go to dev_hdd0/home/00000001/exdata. Processed files are moved
// into Install/Done so they are not imported twice.
#include "stdafx.h"
#include "xbx_internal.h"

#include "Crypto/key_vault.h"
#include "Crypto/unself.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Emu/system_progress.hpp"
#include "Emu/system_utils.hpp"
#include "Emu/vfs_config.h"
#include "Loader/PUP.h"
#include "Loader/TAR.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "util/sysinfo.hpp"

#include <algorithm>

LOG_CHANNEL(xbx_log, "XBX");

namespace xbx
{
	bool install_firmware(const std::string& path)
	{
		fs::file pup_f(path);
		if (!pup_f)
		{
			xbx_log.error("Firmware: cannot open %s (%s)", path, fs::g_tls_error);
			return false;
		}

		pup_object pup(std::move(pup_f));

		if (pup.operator pup_error() != pup_error::ok)
		{
			xbx_log.error("Firmware: %s is not a valid PUP file (%s)", path, pup.get_formatted_error());
			return false;
		}

		fs::file update_files_f = pup.get_file(0x300);
		const usz update_files_size = update_files_f ? update_files_f.size() : 0;

		if (!update_files_size)
		{
			xbx_log.error("Firmware: installation package database not found");
			return false;
		}

		fs::device_stat dev_stat{};
		if (!fs::statfs(g_cfg_vfs.get_dev_flash(), dev_stat) || dev_stat.avail_free < update_files_size)
		{
			xbx_log.error("Firmware: not enough free space in %s", g_cfg_vfs.get_dev_flash());
			return false;
		}

		tar_object update_files(update_files_f);

		// Only the "dev_flash_*" entries are installed. Each is a SCE-encrypted TAR of dev_flash files.
		auto update_filenames = update_files.get_filenames();
		update_filenames.erase(std::remove_if(update_filenames.begin(), update_filenames.end(),
			[](const std::string& s) { return s.find("dev_flash_") == umax; }), update_filenames.end());

		if (update_filenames.empty())
		{
			xbx_log.error("Firmware: no dev_flash packages found");
			return false;
		}

		std::string version_string;
		if (fs::file version = pup.get_file(0x100))
		{
			version_string = version.to_string();
		}

		if (const usz pos = version_string.find('\n'); pos != umax)
		{
			version_string.erase(pos);
		}

		scoped_progress_dialog progress(fmt::format("Installing PS3 firmware %s...", version_string));
		g_progr_ftotal += ::size32(update_filenames);

		// Destination of tar_object::extract()
		vfs::mount("/dev_flash", g_cfg_vfs.get_dev_flash());

		bool ok = true;

		for (const std::string& update_filename : update_filenames)
		{
			auto update_file_stream = update_files.get_file(update_filename);

			if (update_file_stream->m_file_handler)
			{
				update_file_stream->m_file_handler->handle_file_op(*update_file_stream, 0, update_file_stream->get_size(umax), nullptr);
			}

			fs::file update_file = fs::make_stream(std::move(update_file_stream->data));

			SCEDecrypter self_dec(update_file);
			self_dec.LoadHeaders();
			self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
			self_dec.DecryptData();

			auto dev_flash_tar_f = self_dec.MakeFile();
			if (dev_flash_tar_f.size() < 3)
			{
				xbx_log.error("Firmware: %s could not be decrypted", update_filename);
				ok = false;
				break;
			}

			tar_object dev_flash_tar(dev_flash_tar_f[2]);
			if (!dev_flash_tar.extract())
			{
				xbx_log.error("Firmware: %s could not be extracted", update_filename);
				ok = false;
				break;
			}

			g_progr_fdone++;
		}

		update_files_f.close();

		if (ok)
		{
			xbx_log.success("Firmware %s installed", utils::get_firmware_version());
		}

		return ok;
	}

	namespace
	{
		enum class import_kind
		{
			none,
			firmware,
			package,
			license,
		};

		import_kind classify(const std::string& name)
		{
			const std::string lower = fmt::to_lower(name);

			if (lower.ends_with(".pup"))
				return import_kind::firmware;
			if (lower.ends_with(".pkg"))
				return import_kind::package;
			if (lower.ends_with(".rap") || lower.ends_with(".edat"))
				return import_kind::license;

			return import_kind::none;
		}

		void mark_done(const std::string& dir, const std::string& name)
		{
			const std::string done_dir = dir + "Done/";
			fs::create_path(done_dir);

			if (!fs::rename(dir + name, done_dir + name, true))
			{
				xbx_log.warning("Could not move %s to %s (%s)", name, done_dir, fs::g_tls_error);
			}
		}
	}

	void import_pending_files()
	{
		std::vector<std::string> dirs{ get_paths().local_state_dir + "Install/" };

		if (!get_paths().usb_root.empty())
		{
			dirs.push_back(get_paths().usb_root + "Install/");
		}

		usz installed_packages = 0;

		// Firmware first: packages and games need it
		for (const import_kind pass : { import_kind::firmware, import_kind::license, import_kind::package })
		{
			for (const std::string& dir : dirs)
			{
				std::vector<std::string> names;

				for (const fs::dir_entry& entry : fs::dir(dir))
				{
					if (!entry.is_directory && classify(entry.name) == pass)
					{
						names.push_back(entry.name);
					}
				}

				std::sort(names.begin(), names.end());

				for (const std::string& name : names)
				{
					const std::string path = dir + name;
					bool ok = false;

					switch (pass)
					{
					case import_kind::firmware:
						ok = install_firmware(path);
						break;
					case import_kind::license:
					{
						const std::string exdata = rpcs3::utils::get_hdd0_dir() + "home/" + Emu.GetUsr() + "/exdata/";
						ok = fs::create_path(exdata) && fs::copy_file(path, exdata + name, true);
						if (ok)
							xbx_log.notice("License %s -> %s", name, exdata);
						else
							xbx_log.error("License %s -> %s failed (%s)", name, exdata, fs::g_tls_error);
						break;
					}
					case import_kind::package:
					{
						scoped_progress_dialog progress(fmt::format("Installing %s...", name));
						ok = rpcs3::utils::install_pkg(path, false);
						installed_packages += ok;
						break;
					}
					case import_kind::none:
						break;
					}

					if (ok)
					{
						mark_done(dir, name);
					}
					else
					{
						xbx_log.error("Import failed: %s", path);
					}
				}
			}
		}

		if (installed_packages)
		{
			// Restart Big Picture Mode so the new games show up in the grid
			// (xbx_callbacks.cpp brings it back once emulation has stopped).
			xbx_log.notice("Installed %u package(s), reloading the game list", installed_packages);
			main_thread_post([]() { Emu.GracefulShutdown(false); });
		}
	}
}
