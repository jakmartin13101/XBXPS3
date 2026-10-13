#pragma once

#include "xbx_core.h"

#include <functional>
#include <string>

namespace xbx
{
	// Host services handed over by the shell in xbx_core_init.
	const xbx_host_api& host();

	struct paths
	{
		std::string install_dir;     // Read-only package folder
		std::string local_state_dir; // Writable LocalState folder (RPCS3 config dir)
		std::string usb_root;        // "E:/XBXPS3/" or empty
	};

	const paths& get_paths();

	// The emulator's "main thread" (the Qt GUI thread on desktop). Everything RPCS3
	// posts through g_emu_callbacks.call_from_main_thread runs here, in order.
	void main_thread_start();
	void main_thread_post(std::function<void()> func);
	void main_thread_stop();

	// Fills g_emu_callbacks (xbx_callbacks.cpp).
	void install_emu_callbacks();

	// Imports PS3UPDAT.PUP, *.pkg, *.rap and *.edat dropped into the Install folders (xbx_install.cpp).
	void import_pending_files();
	bool install_firmware(const std::string& pup_path);

	// Applies Xbox defaults to config.yml on first run (xbx_settings.cpp).
	void apply_first_run_settings();
	void configure_vfs();

	// Localized strings for the native overlays (xbx_strings.cpp).
	std::string get_localized_string(int id, const char* args);
	std::u32string get_localized_u32string(int id, const char* args);

	// Image helpers for the Big Picture game grid and cellSearch (xbx_image.cpp).
	bool get_image_info(const std::string& filename, std::string& sub_type, int& width, int& height, int& orientation);
	bool get_scaled_image(const std::string& path, int target_width, int target_height, int& width, int& height, unsigned char* dst, bool force_fit);

	// Hands the Dozen driver to volk (xbx_vulkan.cpp). Returns false if Vulkan is unavailable.
	bool vulkan_init();
	bool vulkan_available();
}
