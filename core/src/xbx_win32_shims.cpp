// Stand-ins for the few desktop-only Win32 imports left in the RPCS3 core.
//
// The UWP app links WindowsApp.lib + mincore.lib only, never user32/gdi32/iphlpapi, because
// importing a DLL that the Xbox OS lacks makes the whole process fail to load. RPCS3 code
// compiled against the desktop headers calls these functions through __imp_ pointers
// (dllimport), so defining the pointers here satisfies the linker without touching that code.
//
// Call sites (RPCS3 2ba2f75):
//   MessageBoxA/W   Utilities/File.cpp, util/vm_native.cpp, Emu/Cell/lv2/sys_time.cpp
//   MonitorFromWindow, GetClientRect, GetDC, ReleaseDC and the GDI calls
//                   Emu/RSX/VK/vkutils/swapchain*.{cpp,hpp} (CPU-copy fallback swapchain, unused with Dozen)
//   GetAdaptersInfo Emu/NP/np_handler.cpp (MAC address for RPCN, optional)
//
// When the link reports another unresolved __imp_ symbol from a desktop DLL, add it here or
// guard the call site with RPCS3_UWP in patches/rpcs3. See docs/BUILDING.md, "Link triage".
//
// This file deliberately does not include <windows.h>: those headers declare the real
// functions as dllimport, which would clash with the definitions below.
#include "xbx_internal.h"

#include <cstdint>
#include <string>

namespace
{
	using HWND = void*;
	using HDC = void*;
	using HGDIOBJ = void*;
	using UINT = unsigned int;
	using ULONG = unsigned long;
	using DWORD = unsigned long;

	constexpr int IDOK = 1;
	constexpr DWORD ERROR_NOT_SUPPORTED = 50;

	void report(const char* what, const char* text)
	{
		if (const auto& host = xbx::host(); host.debug_print)
		{
			std::string line = "[win32 shim] ";
			line += what;
			if (text)
			{
				line += ": ";
				line += text;
			}
			line += '\n';
			host.debug_print(host.ctx, line.c_str());
		}
	}

	int __stdcall shim_MessageBoxA(HWND, const char* text, const char* caption, UINT)
	{
		report(caption ? caption : "MessageBoxA", text);
		return IDOK;
	}

	int __stdcall shim_MessageBoxW(HWND, const wchar_t* text, const wchar_t*, UINT)
	{
		std::string narrow;
		for (const wchar_t* p = text; p && *p; p++)
		{
			narrow += *p < 0x80 ? static_cast<char>(*p) : '?';
		}
		report("MessageBoxW", narrow.c_str());
		return IDOK;
	}

	void* __stdcall shim_MonitorFromWindow(HWND, DWORD) { return nullptr; }
	int __stdcall shim_GetClientRect(HWND, void*) { return 0; }
	HDC __stdcall shim_GetDC(HWND) { return nullptr; }
	int __stdcall shim_ReleaseDC(HWND, HDC) { return 0; }
	HDC __stdcall shim_CreateCompatibleDC(HDC) { return nullptr; }
	void* __stdcall shim_CreateDIBSection(HDC, const void*, UINT, void** bits, void*, DWORD) { if (bits) *bits = nullptr; return nullptr; }
	HGDIOBJ __stdcall shim_SelectObject(HDC, HGDIOBJ) { return nullptr; }
	int __stdcall shim_DeleteObject(HGDIOBJ) { return 0; }
	int __stdcall shim_DeleteDC(HDC) { return 0; }
	int __stdcall shim_BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return 0; }
	int __stdcall shim_GdiFlush() { return 1; }
	ULONG __stdcall shim_GetAdaptersInfo(void*, ULONG*) { return ERROR_NOT_SUPPORTED; }
}

#define XBX_IMPORT_SHIM(name) extern "C" const void* __imp_##name = reinterpret_cast<const void*>(&shim_##name)

XBX_IMPORT_SHIM(MessageBoxA);
XBX_IMPORT_SHIM(MessageBoxW);
XBX_IMPORT_SHIM(MonitorFromWindow);
XBX_IMPORT_SHIM(GetClientRect);
XBX_IMPORT_SHIM(GetDC);
XBX_IMPORT_SHIM(ReleaseDC);
XBX_IMPORT_SHIM(CreateCompatibleDC);
XBX_IMPORT_SHIM(CreateDIBSection);
XBX_IMPORT_SHIM(SelectObject);
XBX_IMPORT_SHIM(DeleteObject);
XBX_IMPORT_SHIM(DeleteDC);
XBX_IMPORT_SHIM(BitBlt);
XBX_IMPORT_SHIM(GdiFlush);
XBX_IMPORT_SHIM(GetAdaptersInfo);
