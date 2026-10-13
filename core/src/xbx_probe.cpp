// Hardware probe. Answers, on the console itself, the questions the research could not:
//   - Can a Dev Mode app reserve RPCS3's ~60 GiB of address space and map aliased views
//     into placeholders at fixed addresses (vm_native.cpp)?
//   - Are RWX pages allowed (RPCS3's JIT default), or only W^X?
//   - Which D3D12 shader model and options does Dozen get? (Dozen needs SM 6.6 for
//     descriptor indexing, which RPCS3 requires.)
//   - How much memory and which CPU features does the app actually get?
// The report is written to LocalState/xbxps3_probe.txt on every launch.
#include "stdafx.h"
#include "xbx_internal.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <intrin.h>
#include <wrl/client.h>

#include <cstdio>
#include <string>

#pragma comment(lib, "mincore")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

using Microsoft::WRL::ComPtr;

namespace
{
	struct report
	{
		std::string text;

		template <typename... Args>
		void line(const char* fmt, Args... args)
		{
			char buf[512];
			std::snprintf(buf, sizeof(buf), fmt, args...);
			text += buf;
			text += "\r\n";
		}
	};

	// mov eax, 42; ret
	constexpr unsigned char ret42[] = { 0xB8, 0x2A, 0x00, 0x00, 0x00, 0xC3 };

	int call_code(void* code)
	{
		__try
		{
			return reinterpret_cast<int (*)()>(code)();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return -static_cast<int>(GetExceptionCode() & 0x7fffffff);
		}
	}

	void probe_memory_layout(report& r)
	{
		r.line("[Address space: placeholders + aliased views, as in RPCS3 util/vm_native.cpp]");

		// RPCS3 reserves ~56 GiB for guest memory plus JIT regions
		constexpr u64 reserve_size = 64ull << 30;
		void* base = VirtualAlloc2(nullptr, nullptr, reserve_size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
		r.line("VirtualAlloc2 reserve 64 GiB placeholder: %s (%p, err %lu)", base ? "OK" : "FAILED", base, base ? 0ul : GetLastError());

		if (!base)
		{
			return;
		}

		constexpr u64 view = 0x10000;
		HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(view), nullptr);
		r.line("CreateFileMappingW 64 KiB: %s", section ? "OK" : "FAILED");

		// Split two 64 KiB placeholders off the reservation and map the same section twice
		u8* const a = static_cast<u8*>(base);
		u8* const b = a + (32ull << 30);
		const bool split1 = VirtualFree(a, view, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
		const bool split2 = VirtualFree(b, view, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
		r.line("Split placeholders: %s", split1 && split2 ? "OK" : "FAILED");

		void* va = section ? MapViewOfFile3(section, nullptr, a, 0, view, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0) : nullptr;
		void* vb = section ? MapViewOfFile3(section, nullptr, b, 0, view, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0) : nullptr;
		r.line("MapViewOfFile3 at fixed addresses: %s / %s", va ? "OK" : "FAILED", vb ? "OK" : "FAILED");

		if (va && vb)
		{
			static_cast<volatile u32*>(va)[0] = 0x12345678;
			r.line("Mirror coherency (write view A, read view B): %s", static_cast<volatile u32*>(vb)[0] == 0x12345678 ? "OK" : "FAILED");
		}

		if (va) UnmapViewOfFile2(GetCurrentProcess(), va, 0);
		if (vb) UnmapViewOfFile2(GetCurrentProcess(), vb, 0);
		if (section) CloseHandle(section);
		VirtualFree(base, 0, MEM_RELEASE);
	}

	void probe_jit(report& r)
	{
		r.line("");
		r.line("[JIT memory (RPCS3 uses RWX pages on Windows)]");

		if (void* p = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
		{
			std::memcpy(p, ret42, sizeof(ret42));
			FlushInstructionCache(GetCurrentProcess(), p, sizeof(ret42));
			r.line("RWX VirtualAlloc: OK, call returned %d (42 = works)", call_code(p));
			VirtualFree(p, 0, MEM_RELEASE);
		}
		else
		{
			r.line("RWX VirtualAlloc: FAILED (err %lu). RPCS3's JIT allocators need W^X patches.", GetLastError());
		}

		if (void* p = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
		{
			std::memcpy(p, ret42, sizeof(ret42));
			DWORD old{};
			const bool prot = VirtualProtect(p, 0x1000, PAGE_EXECUTE_READ, &old);
			FlushInstructionCache(GetCurrentProcess(), p, sizeof(ret42));
			r.line("W^X (RW -> RX VirtualProtect): %s, call returned %d", prot ? "OK" : "FAILED", prot ? call_code(p) : 0);
			VirtualFree(p, 0, MEM_RELEASE);
		}

		if (void* p = VirtualAllocFromApp(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
		{
			std::memcpy(p, ret42, sizeof(ret42));
			ULONG old{};
			const bool prot = VirtualProtectFromApp(p, 0x1000, PAGE_EXECUTE_READ, &old);
			FlushInstructionCache(GetCurrentProcess(), p, sizeof(ret42));
			r.line("W^X with *FromApp APIs (codeGeneration capability): %s, call returned %d", prot ? "OK" : "FAILED", prot ? call_code(p) : 0);
			VirtualFree(p, 0, MEM_RELEASE);
		}
	}

	void probe_cpu_memory(report& r)
	{
		r.line("");
		r.line("[CPU and memory]");

		SYSTEM_INFO si{};
		GetNativeSystemInfo(&si);
		r.line("Logical processors: %lu", si.dwNumberOfProcessors);

		int regs[4]{};
		__cpuidex(regs, 7, 0);
		const bool avx2 = (regs[1] & (1 << 5)) != 0;
		__cpuid(regs, 1);
		const bool osxsave = (regs[2] & (1 << 27)) != 0;
		const bool avx_os = osxsave && (_xgetbv(0) & 6) == 6;
		r.line("AVX2: %s (OS AVX state %s)", avx2 ? "yes" : "no", avx_os ? "enabled" : "disabled");

		MEMORYSTATUSEX ms{ sizeof(ms) };
		GlobalMemoryStatusEx(&ms);
		r.line("Physical memory visible: %llu MiB total, %llu MiB available", ms.ullTotalPhys >> 20, ms.ullAvailPhys >> 20);
		r.line("Virtual address space: %llu GiB total, %llu GiB available", ms.ullTotalVirtual >> 30, ms.ullAvailVirtual >> 30);
	}

	void probe_d3d12(report& r)
	{
		r.line("");
		r.line("[Direct3D 12 (what Dozen builds on)]");

		ComPtr<ID3D12Device> device;
		HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
		r.line("D3D12CreateDevice(FL 11_0): 0x%08lx", static_cast<unsigned long>(hr));

		if (FAILED(hr))
		{
			return;
		}

		const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
		D3D12_FEATURE_DATA_FEATURE_LEVELS fl{ static_cast<UINT>(std::size(levels)), levels };
		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &fl, sizeof(fl))))
		{
			r.line("Max feature level: 0x%x", fl.MaxSupportedFeatureLevel);
		}

		D3D12_FEATURE_DATA_SHADER_MODEL sm{ D3D_SHADER_MODEL_6_7 };
		while (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm))) && sm.HighestShaderModel > D3D_SHADER_MODEL_5_1)
		{
			sm.HighestShaderModel = static_cast<D3D_SHADER_MODEL>(sm.HighestShaderModel - 1);
		}
		r.line("Highest shader model: %d.%d %s", sm.HighestShaderModel >> 4, sm.HighestShaderModel & 0xf,
			sm.HighestShaderModel >= D3D_SHADER_MODEL_6_6 ? "(Dozen descriptor indexing OK)" : "(BELOW 6.6: Dozen cannot expose descriptor indexing, RPCS3 Vulkan will refuse the device)");

		D3D12_FEATURE_DATA_D3D12_OPTIONS o{};
		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &o, sizeof(o))))
		{
			r.line("Resource binding tier: %d, ROVs: %d, typed UAV load extra formats: %d, conservative raster tier: %d",
				o.ResourceBindingTier, o.ROVsSupported, o.TypedUAVLoadAdditionalFormats, o.ConservativeRasterizationTier);
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS1 o1{};
		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &o1, sizeof(o1))))
		{
			r.line("Wave ops: %d, lanes %u-%u", o1.WaveOps, o1.WaveLaneCountMin, o1.WaveLaneCountMax);
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS4 o4{};
		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &o4, sizeof(o4))))
		{
			r.line("Native 16-bit shader ops: %d", o4.Native16BitShaderOpsSupported);
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS12 o12{};
		r.line("Enhanced barriers: %s", SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &o12, sizeof(o12))) && o12.EnhancedBarriersSupported ? "yes" : "no");

		ComPtr<IDXGIFactory4> factory;
		if (SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
		{
			ComPtr<IDXGIAdapter1> adapter;
			if (SUCCEEDED(factory->EnumAdapters1(0, &adapter)))
			{
				DXGI_ADAPTER_DESC1 desc{};
				adapter->GetDesc1(&desc);
				r.line("Adapter 0: vendor 0x%04x device 0x%04x, dedicated VRAM %llu MiB, shared %llu MiB",
					desc.VendorId, desc.DeviceId, static_cast<u64>(desc.DedicatedVideoMemory) >> 20, static_cast<u64>(desc.SharedSystemMemory) >> 20);
			}
		}
	}
}

extern "C" void xbx_core_run_probe(const char* report_path_utf8, const char* header_utf8)
{
	report r;

	if (header_utf8)
	{
		r.text += header_utf8;
	}

	probe_cpu_memory(r);
	r.line("");
	probe_memory_layout(r);
	probe_jit(r);
	probe_d3d12(r);

	if (const auto& host = xbx::host(); host.debug_print)
	{
		host.debug_print(host.ctx, r.text.c_str());
	}

	const int wlen = MultiByteToWideChar(CP_UTF8, 0, report_path_utf8, -1, nullptr, 0);
	std::wstring wpath(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, report_path_utf8, -1, wpath.data(), wlen);

	if (HANDLE f = CreateFile2(wpath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr); f != INVALID_HANDLE_VALUE)
	{
		DWORD written = 0;
		WriteFile(f, r.text.data(), static_cast<DWORD>(r.text.size()), &written, nullptr);
		CloseHandle(f);
	}
}
