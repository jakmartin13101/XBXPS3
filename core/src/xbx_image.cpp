// Image loading for the game grid (ICON0/PIC1) and cellSearch, replacing QImageReader.
#include "stdafx.h"
#include "xbx_internal.h"

#include "Utilities/File.h"
#include "Utilities/StrUtil.h"

#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstring>

LOG_CHANNEL(xbx_log, "XBX");

namespace xbx
{
	namespace
	{
		std::vector<u8> read_all(const std::string& path)
		{
			fs::file f(path);
			return f ? f.to_vector<u8>() : std::vector<u8>{};
		}

		std::string sub_type_from_extension(const std::string& path)
		{
			const std::string ext = fmt::to_lower(path.substr(path.find_last_of('.') + 1));

			if (ext == "jpg" || ext == "jpeg")
				return "jpeg";

			return ext;
		}
	}

	bool get_image_info(const std::string& filename, std::string& sub_type, int& width, int& height, int& orientation)
	{
		sub_type.clear();
		width = 0;
		height = 0;
		orientation = 0; // CELL_SEARCH_ORIENTATION_UNKNOWN

		const std::vector<u8> data = read_all(filename);
		int channels = 0;

		if (data.empty() || !stbi_info_from_memory(data.data(), ::narrow<int>(data.size()), &width, &height, &channels))
		{
			xbx_log.error("get_image_info: cannot read '%s'", filename);
			return false;
		}

		sub_type = sub_type_from_extension(filename);
		orientation = 1; // CELL_SEARCH_ORIENTATION_TOP_LEFT (EXIF rotation is not applied)
		return true;
	}

	bool get_scaled_image(const std::string& path, int target_width, int target_height, int& width, int& height, unsigned char* dst, bool force_fit)
	{
		width = 0;
		height = 0;

		if (target_width <= 0 || target_height <= 0 || !dst)
		{
			return false;
		}

		const std::vector<u8> data = read_all(path);
		int src_w = 0, src_h = 0, channels = 0;
		stbi_uc* src = data.empty() ? nullptr : stbi_load_from_memory(data.data(), ::narrow<int>(data.size()), &src_w, &src_h, &channels, 4);

		if (!src || src_w <= 0 || src_h <= 0)
		{
			xbx_log.error("get_scaled_image: cannot decode '%s'", path);
			stbi_image_free(src);
			return false;
		}

		// Same fitting rules as the Qt frontend: keep the aspect ratio, never grow unless forced
		int out_w = src_w;
		int out_h = src_h;

		if (force_fit || src_w > target_width || src_h > target_height)
		{
			const f32 target_ratio = target_width / static_cast<f32>(target_height);
			const f32 image_ratio = src_w / static_cast<f32>(src_h);
			const f32 convert_ratio = image_ratio / target_ratio;

			if (convert_ratio > 1.0f)
			{
				out_w = target_width;
				out_h = static_cast<int>(target_height / convert_ratio);
			}
			else if (convert_ratio < 1.0f)
			{
				out_w = static_cast<int>(target_width * convert_ratio);
				out_h = target_height;
			}
			else
			{
				out_w = target_width;
				out_h = target_height;
			}
		}

		out_w = std::clamp(out_w, 1, target_width);
		out_h = std::clamp(out_h, 1, target_height);

		// Bilinear resample into a tightly packed RGBA8 buffer (stride = out_w * 4)
		const f32 sx = src_w / static_cast<f32>(out_w);
		const f32 sy = src_h / static_cast<f32>(out_h);

		for (int y = 0; y < out_h; y++)
		{
			const f32 fy = std::max(0.0f, (y + 0.5f) * sy - 0.5f);
			const int y0 = std::min(static_cast<int>(fy), src_h - 1);
			const int y1 = std::min(y0 + 1, src_h - 1);
			const f32 wy = fy - y0;

			for (int x = 0; x < out_w; x++)
			{
				const f32 fx = std::max(0.0f, (x + 0.5f) * sx - 0.5f);
				const int x0 = std::min(static_cast<int>(fx), src_w - 1);
				const int x1 = std::min(x0 + 1, src_w - 1);
				const f32 wx = fx - x0;

				const stbi_uc* p00 = src + (y0 * src_w + x0) * 4;
				const stbi_uc* p01 = src + (y0 * src_w + x1) * 4;
				const stbi_uc* p10 = src + (y1 * src_w + x0) * 4;
				const stbi_uc* p11 = src + (y1 * src_w + x1) * 4;
				u8* out = dst + (static_cast<usz>(y) * out_w + x) * 4;

				for (int c = 0; c < 4; c++)
				{
					const f32 top = p00[c] + (p01[c] - p00[c]) * wx;
					const f32 bottom = p10[c] + (p11[c] - p10[c]) * wx;
					out[c] = static_cast<u8>(std::lround(top + (bottom - top) * wy));
				}
			}
		}

		stbi_image_free(src);

		width = out_w;
		height = out_h;
		return true;
	}
}
