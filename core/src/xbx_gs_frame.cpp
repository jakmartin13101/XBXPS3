#include "stdafx.h"
#include "xbx_gs_frame.h"
#include "xbx_internal.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Utilities/File.h"

#include <png.h>

#include <chrono>
#include <thread>

LOG_CHANNEL(screenshot_log, "SCREENSHOT");

namespace
{
	void output_mode(int& width, int& height, double& refresh)
	{
		width = 1920;
		height = 1080;
		refresh = 60.0;

		if (const auto& host = xbx::host(); host.get_output_mode)
		{
			host.get_output_mode(host.ctx, &width, &height, &refresh);
		}
	}
}

int xbx_gs_frame::client_width()
{
	int w, h;
	double r;
	output_mode(w, h, r);
	return w;
}

int xbx_gs_frame::client_height()
{
	int w, h;
	double r;
	output_mode(w, h, r);
	return h;
}

f64 xbx_gs_frame::client_display_rate()
{
	int w, h;
	double r;
	output_mode(w, h, r);
	return r;
}

display_handle_t xbx_gs_frame::handle() const
{
	// Non-null token, see the class comment
	return reinterpret_cast<HWND>(static_cast<uintptr_t>(0x5842));
}

void xbx_gs_frame::take_screenshot(std::vector<u8>&& sshot_data, u32 sshot_width, u32 sshot_height, bool is_bgra)
{
	std::thread([data = std::move(sshot_data), sshot_width, sshot_height, is_bgra]() mutable
	{
		const std::string title_id = Emu.GetTitleID().empty() ? std::string("BigPicture") : Emu.GetTitleID();
		const std::string dir = rpcs3::utils::get_screenshots_dir(title_id);

		if (!fs::create_path(dir))
		{
			screenshot_log.error("Failed to create %s", dir);
			return;
		}

		const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm tm{};
		localtime_s(&tm, &t);
		const std::string path = fmt::format("%s%s_%04d%02d%02d_%02d%02d%02d.png", dir, title_id,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		if (is_bgra)
		{
			for (usz i = 0; i + 3 < data.size(); i += 4)
			{
				std::swap(data[i], data[i + 2]);
			}
		}

		png_image image{};
		image.version = PNG_IMAGE_VERSION;
		image.width = sshot_width;
		image.height = sshot_height;
		image.format = PNG_FORMAT_RGBA;

		png_alloc_size_t size = 0;
		if (!png_image_write_to_memory(&image, nullptr, &size, 0, data.data(), 0, nullptr))
		{
			screenshot_log.error("PNG encode failed: %s", image.message);
			return;
		}

		std::vector<u8> encoded(size);
		if (!png_image_write_to_memory(&image, encoded.data(), &size, 0, data.data(), 0, nullptr))
		{
			screenshot_log.error("PNG encode failed: %s", image.message);
			return;
		}

		encoded.resize(size);

		if (fs::write_file(path, fs::rewrite, encoded))
		{
			screenshot_log.success("Saved %s", path);
		}
		else
		{
			screenshot_log.error("Failed to write %s (%s)", path, fs::g_tls_error);
		}
	}).detach();
}
