// Overlay strings without Qt. The table is generated at build time from
// rpcs3qt/localized_emu.h plus core/tools/overrides.txt (see CMakeLists.txt).
#include "stdafx.h"
#include "xbx_internal.h"

#include "Emu/localized_string_id.h"

#include <string_view>
#include <unordered_map>

namespace xbx
{
	namespace
	{
		struct entry
		{
			localized_string_id id;
			const char8_t* text;
		};

		constexpr entry s_table[] =
		{
#include "xbx_localized_table.inc"
		};

		const std::unordered_map<int, std::string_view>& table()
		{
			static const std::unordered_map<int, std::string_view> map = []
			{
				std::unordered_map<int, std::string_view> result;
				for (const entry& e : s_table)
				{
					result.emplace(static_cast<int>(e.id), std::string_view(reinterpret_cast<const char*>(e.text)));
				}
				return result;
			}();

			return map;
		}

		// QString::arg semantics: replace every occurrence of the lowest-numbered %N marker
		std::string apply_arg(std::string_view text, const char* args)
		{
			std::string result{text};

			if (!args)
			{
				return result;
			}

			for (char n = '0'; n <= '9'; n++)
			{
				const char marker[3] = { '%', n, '\0' };

				if (result.find(marker) == std::string::npos)
				{
					continue;
				}

				for (usz pos = 0; (pos = result.find(marker, pos)) != std::string::npos;)
				{
					result.replace(pos, 2, args);
					pos += std::char_traits<char>::length(args);
				}

				break;
			}

			return result;
		}

		std::u32string utf8_to_utf32(std::string_view s)
		{
			std::u32string out;
			out.reserve(s.size());

			for (usz i = 0; i < s.size();)
			{
				const u8 c = static_cast<u8>(s[i]);
				char32_t cp = U'�';
				usz len = 1;

				if (c < 0x80)
				{
					cp = c;
				}
				else if ((c >> 5) == 0x6 && i + 1 < s.size())
				{
					cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F);
					len = 2;
				}
				else if ((c >> 4) == 0xE && i + 2 < s.size())
				{
					cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
					len = 3;
				}
				else if ((c >> 3) == 0x1E && i + 3 < s.size())
				{
					cp = ((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
					len = 4;
				}

				out += cp;
				i += len;
			}

			return out;
		}
	}

	std::string get_localized_string(int id, const char* args)
	{
		const auto& map = table();

		if (const auto it = map.find(id); it != map.end())
		{
			return apply_arg(it->second, args);
		}

		return "Unknown";
	}

	std::u32string get_localized_u32string(int id, const char* args)
	{
		return utf8_to_utf32(get_localized_string(id, args));
	}
}
