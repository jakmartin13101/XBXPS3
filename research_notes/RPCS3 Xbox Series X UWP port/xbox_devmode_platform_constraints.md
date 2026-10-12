# Xbox Series X|S Developer Mode (and Retail) UWP Platform Constraints for an RPCS3 Port (state as of Oct 2026)

> Method note for the report writer: direct fetches of learn.microsoft.com, walbourn.github.io, neowin.net, ku.nz and emulationrevival.github.io failed (proxy policy/DNS) during this session. Microsoft Learn and Chuck Walbourn content below was obtained through search-engine extracts of those pages (URLs cited as the source), not by reading the full pages. GitHub-hosted material (emulator ports, devmodewiki, RPCS3 issues) was read directly. The Microsoft UWP-on-Xbox "System resources" page now appears under a `/previous-versions/` URL, so Microsoft has archived it. Treat its numbers as the last official statement, not as a page Microsoft still maintains.

## 1. Memory: App vs Game, Series X vs Series S, Dev Mode vs Retail; how to set "Game"; more memory?

### Takeaway
The official figure, unchanged since the Xbox One era, is a 1 GB foreground limit for UWP "Apps" and 5 GB for UWP "Games". The limit is shared by CPU and GPU, because memory is unified. A 2026 community port (eden-xbox, a Switch emulator) measured exactly 5120 MiB in Game mode on a retail Series X in Dev Mode. RPCS3 maintainers cite this RAM ceiling as a blocker. One user-written Microsoft Q&A post claims about 8 GB on Series X, but nothing official backs it. The only documented way to exceed the cap is running under the Visual Studio debugger, and no source confirms this still works on Series X|S.

### Cited Findings
- Official UWP-on-Xbox resource table: foreground memory is **1 GB for apps and 5 GB for games**. Background apps get **128 MB**, a limit that applies only to concurrent apps such as background music players. **Games are suspended and terminated in the background.** Exceeding the limits causes allocation failures. — [Microsoft Learn: System resources for UWP apps and games on Xbox](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation); the same content now sits at an archived URL: [previous-versions copy](https://learn.microsoft.com/en-us/previous-versions/windows/uwp/xbox-apps/system-resource-allocation)
- Same page: "when running your app or game from the Visual Studio debugger, these memory constraints do not apply." — [Microsoft Learn (search extract)](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation)
- The search-engine rendering of the page attributes the 1 GB / 5 GB figures to Series X|S. One page does not list different numbers for Series S and Series X. — [Microsoft Learn (search extract)](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation)
- **Community measurement (2026, retail Series X in Dev Mode):** "In Game mode the whole app gets **5120 MiB** (~5 GiB), *shared with the GPU* on the Series." The app must be set to Game mode, or the limit is "much lower". The texture-cache budget is min(DXGI budget, app limit − 1.5 GiB). Tears of the Kingdom peaks at about 6.4 GiB uncapped and hits a "memory wall" on console. — [eden-xbox README (JulianDr14/eden-xbox)](https://github.com/JulianDr14/eden-xbox) (mirror: [temamumtaza/eden-xbox](https://github.com/temamumtaza/eden-xbox))
- A Microsoft Q&A post by a user (2025) claims Dev Mode RAM is "around 5 GB on Series S and about 8 GB on Series X". The Microsoft support reply only says the limits "are set by Microsoft to reserve memory for the system", and no Microsoft source confirms 8 GB. — [Microsoft Q&A 5522312](https://learn.microsoft.com/en-us/answers/questions/5522312/request-to-increase-ram-allocation-in-xbox-series)
- A second 2025 Q&A request asks to raise Dev Mode RAM "from 5GB to 8–10GB" for PS3 and Switch emulators. No change is recorded. — [Microsoft Q&A 5519160](https://learn.microsoft.com/en-us/answers/questions/5519160/title-increase-ram-allocation-in-xbox-developer-mo)
- RPCS3 team responses on Xbox. FlexBy420 (contributor), 10 Jul 2024: "theres not enough ram in dev mode". AniLeo (member): "not of interest and still blocked by other dev mode restrictions". A non-member commenter (May 2025) put the limit at "about 4Gb". The issue was locked on 22 May 2025. — [RPCS3 issue #15787](https://github.com/RPCS3/rpcs3/issues/15787)
- **How "Game" vs "App" is set.** In Dev Home: highlight the app in Games & apps, press View, choose View details, and set "App type" to Game. Since Nov 2017, Device Portal can also default newly deployed dev apps to Game. Published packages are marked Game or App in Store metadata. The old `expandedResources` restricted capability in the appxmanifest "still exists but is deprecated", and Store submissions using it fail. — [Walbourn, "DirectX and UWP on Xbox One" (post source)](https://raw.githubusercontent.com/walbourn/walbourn.github.io/main/_posts/2017-12-15-directx-and-uwp-on-xbox-one.md)
- The XBSX2 guide says to set App type to Game. "In App mode, the console limits CPU, GPU, and RAM access, which causes heavy stuttering." — [devmodewiki xbsx2-guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/emulation/xbsx2-guide.mdx)
- eden-xbox deploy steps end with: "Set Eden to Game mode in the app list, then launch it." — [temamumtaza/eden-xbox](https://github.com/temamumtaza/eden-xbox)
- **Hardware context (not the UWP budget).** Series X has 16 GB GDDR6: 10 GB "GPU optimal" at 560 GB/s and 6 GB "standard" at 336 GB/s, of which 2.5 GB is reserved for the OS. That leaves about 13.5 GB for GDK titles; the 13.5 is arithmetic from the cited figures, not a quoted number. — [Guru3D spec report](https://www.guru3d.com/news-story/xbox-series-x-specs-released-by-microsoft.html)
- The June 2022 GDK gave Series S *GDK* titles "hundreds of megabytes" more RAM. This change is GDK-only and does not affect Dev Mode UWP. — [XDA](https://www.xda-developers.com/microsoft-unlocks-more-memory-for-xbox-series-s-developers/); [Windows Central](https://windowscentral.com/gaming/xbox/microsoft-game-development-kit-update-can-improve-performance-on-xbox-series-s)
- **Retail.** "UWP based games are no longer accepted in the Xbox Store", so publishing a game requires ID@Xbox. Apps can still be published through Partner Center with extra Xbox review. — [Xbox UWP FAQ (MicrosoftDocs source)](https://raw.githubusercontent.com/MicrosoftDocs/windows-dev-docs/docs/uwp/xbox-apps/frequently-asked-questions.yml)
- Dev Mode setup also asks you to allocate at least 5 GB of *storage* in "Manage Dev Storage". This is disk space, not RAM. — [devmodewiki setup guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/xbox-setup/xbox-developer-mode-setup.mdx)

### Inferences
- For planning, assume about 5 GiB total, CPU plus GPU together, on both Series X and Series S in Dev Mode Game mode. Do not count on more. Unified memory means the 5 GiB also covers D3D12 heaps, swapchains, shader caches and PSO caches.
- A Store-distributed retail-mode UWP can only be an "App", because UWP games are no longer accepted. Under the official table that means 1 GB and a 45% GPU share, which is unusable for RPCS3. Retail sideloading of emulators is also policed (see section 8).
- The limit is described as "memory usage", meaning commit or working set, not address space. Large reserved but uncommitted ranges should not count against it. No source says so explicitly (see section 2).

### Gaps
- No current (2024–2026) Microsoft page gives separate Series S vs Series X Dev Mode game budgets. The 8 GB Series X claim is unverified and conflicts with the 5120 MiB measured on Series X.
- No source confirms whether the "no limit under the Visual Studio debugger" exemption still holds on Series X|S in 2026, or how much memory becomes usable under it.
- No source found on whether Series S budgets differ from Series X in Dev Mode.

## 2. Virtual address space: full 128 TB? Large (16–32 GB+) reservations?

### Takeaway
No official statement on per-process VA limits for UWP on Xbox was found. Shipping Xbox UWP emulators use placeholder reservations (`VirtualAlloc2FromApp` + `MapViewOfFile3FromApp`, Dolphin) and multi-GB guest reservations committed on demand (eden-xbox). That shows multi-GB reservations work. The eden-xbox author says mirroring the whole Switch address space (fastmem) is "impossible inside the Xbox sandbox". Very large or multiple aliased views, as RPCS3 uses, are therefore unproven and must be tested on hardware.

### Cited Findings
- The Dolphin UWP fork's `MemArenaWin.cpp` resolves `VirtualAlloc2FromApp` and `MapViewOfFile3FromApp` from `api-ms-win-core-memory-l1-1-6` at runtime. It reserves the arena with `MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS` and maps views into the placeholders with `MapViewOfFile3`. It falls back with the log "VirtualAlloc2 and/or MapViewFromFile3 unavailable". — [XboxEmulationHub/dolphin MemArenaWin.cpp](https://raw.githubusercontent.com/XboxEmulationHub/dolphin/master/Source/Core/Common/MemArenaWin.cpp)
- eden-xbox: "No fastmem. Fastmem would need to mirror the Switch address space in the host's, which is impossible inside the Xbox sandbox". Every guest access goes through a page table, and "Guest memory is reserved up front and committed on demand." — [eden-xbox README](https://github.com/JulianDr14/eden-xbox)
- Microsoft API docs (search extract) on `MapViewOfFile3FromApp`: the base address is rounded down to 64 KB and the offset must be 64 KB aligned. Only data or pagefile-backed sections are supported, not image or physical memory. — [MapViewOfFile3FromApp docs](https://learn.microsoft.com/en-in/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile3fromapp)
- `VirtualAlloc2FromApp` and `MapViewOfFile3FromApp` are exported in the `api-ms-win-core-memory-l1-1-6` WindowsApp API set. — [mingw-w64 api-ms-win-core-memory-l1-1-6_windowsapp.def](https://mingw.googlesource.com/mingw-w64/+/bbf8d4999eadff507d183bff6382fa6e8638dd09/mingw-w64-crt/lib-common/api-ms-win-core-memory-l1-1-6_windowsapp.def)
- On general Windows, a `MEM_RESERVE` can fail even with free VA when the kernel cannot allocate VAD descriptors under low memory. — [Microsoft archive blog (slavao)](https://learn.microsoft.com/en-us/archive/blogs/slavao/be-aware-virtualalloc-with-mem_reserve-can-fail-even-if-you-have-plenty-of-vas-inside-of-the-process)
- XBSX2 (the PCSX2 UWP fork) still has upstream `common/Windows/WinHostSys.cpp` code on master. That code uses `VirtualAlloc2` with `MEM_RESERVE | MEM_RESERVE_PLACEHOLDER` and maps write-capable executable pages to `PAGE_EXECUTE_READWRITE`, with no UWP `#ifdef`s in that file. — [XBSX2 WinHostSys.cpp](https://raw.githubusercontent.com/XboxEmulationHub/XBSX2/master/common/Windows/WinHostSys.cpp)
- Xenia Canary UWP, an Xbox 360 emulator fork for Xbox One/Series, exists and builds as an x64 UWP. — [SirMangler/xenia](https://github.com/SirMangler/xenia); [devmodewiki Xenia build guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/development/compiling-xenia-guide.mdx)

### Inferences
- Dolphin's placeholder plus view-mapping arena shows that the RPCS3-style pattern works at multi-GB scale under UWP on Xbox: reserve a placeholder, then map a shared section at one or more addresses. Dolphin reserves several GB for fastmem.
- Eden's "impossible" claim most likely concerns mirroring the 39-bit Switch address space, a reservation of hundreds of GB. It does not show that 4–32 GB reservations fail. An RPCS3 port should prototype its full `vm::` layout early (several 4 GB-aligned regions plus aliased views) to find the actual ceiling.
- XBSX2's master file may be shadowed by UWP-specific code elsewhere in the tree. It is not proof that `VirtualAlloc2` (non-FromApp) or RWX pages work on Xbox.

### Gaps
- No official or community number for maximum VA reservation per UWP process on Xbox, or for whether the full 128 TB user space is exposed.
- Not verified whether Xenia UWP keeps upstream Xenia's large multi-view guest mappings intact on Xbox.

## 3. CPU: cores/threads for UWP games, SMT, AVX/AVX2, affinity

### Takeaway
Officially, UWP games get "4 exclusive and 2 shared CPU cores" and apps get "a share of 2–4 cores". That is a fraction of the Series X's 8 Zen 2 cores (16 threads with SMT at 3.66 GHz, or 8 threads at 3.8 GHz). AVX2 is usable by UWP code on Series X|S: XBSX2 ships a Series-only AVX2 build. Whether SMT siblings are exposed to UWP, and how many logical processors `GetSystemInfo` reports, is undocumented.

### Cited Findings
- Official table: "Games get 4 exclusive and 2 shared CPU cores". "Apps get a share of 2–4 CPU cores depending on the number of apps and games running". — [Microsoft Learn: System resources (search extract)](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation)
- The Xbox One Fall Creators Update (Oct 2017) "Expanded Resources" change described games getting 6 exclusive cores, 5 GB RAM and full GPU. This is an Xbox One-era article, and its wording ("6 exclusive") differs from the current table ("4 exclusive + 2 shared"). — [Neowin (2017)](https://www.neowin.net/news/xbox-one-to-get-expanded-resources-feature-with-fall-creators-update/); [Walbourn post: expanded resources arrived Oct 2017](https://raw.githubusercontent.com/walbourn/walbourn.github.io/main/_posts/2017-12-15-directx-and-uwp-on-xbox-one.md)
- Series X CPU: 8 Zen 2 cores at 3.8 GHz with SMT off, or 3.66 GHz with SMT on (16 threads), with fixed clocks. These are GDK/hardware figures, not UWP-specific ones. Launch coverage also reported that GDK titles typically run about 7 cores, one being reserved for the OS (search extract; not verified against a primary source). — [Guru3D](https://www.guru3d.com/news-story/xbox-series-x-specs-released-by-microsoft.html)
- XBSX2 builds: "AVX2 version only runs on Series S|X, while SSE2 can run on the One or Series". The guide says "AVX2: For Series X|S" and "SSE4: For Xbox One family". — [ConsoleMods homebrew list](https://consolemods.org/wiki/Xbox_One:Homebrew_Apps_List); [devmodewiki xbsx2-guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/emulation/xbsx2-guide.mdx)
- Walbourn's DirectXMath note says Xbox One supports ABM/AES/BMI1/MOVBE and Series X|S adds BMI2 (search extract). — [Walbourn, "DirectXMath AVX and AVX2 – A Coda"](https://walbourn.github.io/directxmath-avx-and-avx2-coda/)
- `SetThreadAffinityMask`'s documented requirements list UWP apps as supported (search extract). — [SetThreadAffinityMask docs](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadaffinitymask)
- eden-xbox caps its JIT code cache at 128 MiB per emulated core. It spins idle emulated cores briefly before blocking to cut handoff latency, which suggests per-core scheduling latency matters. — [eden-xbox README](https://github.com/JulianDr14/eden-xbox)

### Inferences
- RPCS3's PPU/SPU thread model, which wants about 8 or more host threads at high clocks, will run on about 4 to 6 cores. The 2 "shared" cores are contended with the OS, which makes SPU-heavy titles the most constrained.
- AVX2 code generated by LLVM for Zen 2 should run. AVX-512 is absent on Zen 2. If LLVM's host-CPU detection on Xbox returns something odd, RPCS3 can pin its LLVM target explicitly to `znver2`.

### Gaps
- No source states how many logical processors (with or without SMT) a UWP game sees on Series X|S, whether SMT mode is fixed, or the clock.
- No source on whether CPU sets or affinity calls behave on Xbox (vs documented availability on Windows).
- Not verified whether CPUID or XGETBV report AVX/AVX2 normally to UWP processes, though the AVX2 XBSX2 build implies they work.

## 4. GPU: D3D versions, feature levels, shader models, DXR/mesh, Vulkan/GL, GPU share

### Takeaway
UWP on Series X|S gets D3D11 and D3D12 at hardware feature level 11_0, in both App and Game mode. D3D12 supports shader models 5.1 through 6.4. The Agility SDK is not available on the console, and nothing documents DirectX 12 Ultimate features (DXR, mesh shaders, SM 6.5+) as available to UWP. DXIL works: eden-xbox runs DXIL signed by `dxil.dll`, and one port switched from FXC to DXC because FXC failed on Xbox. There is no native Vulkan or OpenGL. Games get the full GPU and apps get 45%. Community Mesa-UWP builds now provide OpenGL over D3D12. RPCS3 needs Vulkan or desktop GL 4.3, and its maintainers have repeatedly declined a UWP port.

### Cited Findings
- Official: "DirectX support (Xbox Series X|S) — Apps: DirectX 11 and DirectX 12 with Hardware Feature Level 11.0; Games: DirectX 11 and DirectX 12 with Hardware Feature Level 11.0". Apps and games must target x64. GPU: apps get a 45% share, and games get "full access to available GPU cycles". — [Microsoft Learn: System resources (search extract)](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation)
- Walbourn, Microsoft's DirectX/Xbox ATG engineer, writing in 2022: on Series X|S, FL 11.0 for D3D11 and D3D12 in App and Game mode. "For DirectX 12, Shader Models 5.1 to 6.4 is supported." WARP can be used on D3D12 in Dev Mode even in App mode, but not in retail. UWP-on-Xbox development requires the Windows 11 22H2 SDK (22621) or later. All from search extracts. — [Walbourn, "DirectX and UWP on Xbox Series X|S"](https://walbourn.github.io/directx-and-uwp-on-xbox-series-x-s/)
- Xbox One era, for contrast: D3D11 FL 10.0 (no SM5/compute/tessellation/BC6H/BC7), and D3D12 only in Game mode at FL 11.0. Creating a DX12 device outside Game mode, or omitting FL 10.0 on DX11, silently returns WARP. — [Walbourn Xbox One post source](https://raw.githubusercontent.com/walbourn/walbourn.github.io/main/_posts/2017-12-15-directx-and-uwp-on-xbox-one.md)
- eden-xbox renderer: "Feature level 11.0, shader model 6.4, no Agility SDK on the console". Guest SPIR-V is converted with Mesa `spirv_to_dxil`, signed with `dxil.dll`, and cached on disk. If either DLL is missing, the renderer falls back to a CPU-presented framebuffer. — [eden-xbox README](https://github.com/JulianDr14/eden-xbox)
- WiiCompiled-Xbox-UWP moved from FXC to runtime-loaded DXC because "Legacy FXC failed to compile some GX pipelines on Xbox with E_OUTOFMEMORY" (search extract). — [itzjuan612/WiiCompiled-Xbox-UWP](https://github.com/itzjuan612/WiiCompiled-Xbox-UWP)
- A developer reported a UWP D3D12 compute workload running far slower than expected on Xbox (Series S), despite the documented full GPU access for games. — [Microsoft Q&A 897042](https://learn.microsoft.com/en-us/answers/questions/897042/uwp-d3d12-compute-shader-super-slow-on-xbox)
- RPCS3 (Jan 2021): "even if RPCS3 was ported to a Xbox Series X UWP app it would be useless since it only supports Direct3D, not … Vulkan or OpenGL (4.3)". Also: "We will not port RPCS3 to UWP." — [RPCS3 on X (1349115368539885569)](https://x.com/rpcs3/status/1349115368539885569); [RPCS3 on X (1261773023440441344)](https://x.com/rpcs3/status/1261773023440441344)
- RPCS3 #9315: AniLeo: "RPCS3 only has OpenGL 4.3 and Vulkan renders. Angle only translates from OpenGL ES 2.0 and 3.0". In Nov 2024: "no one in the team owns an Xbox Series X … So this is a won't do regardless." A relayed Microsoft email (2022) said Mesa GL/Vulkan drivers would not "currently work with UWP". — [RPCS3 issue #9315](https://github.com/RPCS3/rpcs3/issues/9315)
- Mesa on UWP/Xbox: in March 2024 Aerisarn ported Mesa (the D3D12 Gallium driver, GL over D3D12) to UWP, running glxgears and dhewm3 (Doom 3) on Xbox Series. — [Phoronix](https://www.phoronix.com/news/UWP-Mesa-Xbox-Port). By Feb 2022 the Mesa D3D12 driver exposed OpenGL 4.2. — [Phoronix](https://www.phoronix.com/news/OpenGL-4.2-Mesa-D3D12). Dozen (Vulkan on D3D12) reached 99.75% Vulkan 1.0 CTS pass by Jan 2023 and was working toward 1.1. — [Phoronix](https://www.phoronix.com/news/Micorosft-Dzn-99p-Vulkan); [Phoronix](https://www.phoronix.com/news/Microsoft-Dzn-Vulkan-1.1-Nears)
- XboxEmulationHub maintains `mesa-uwp`, with branch `uwp-26.2.2` visible in 2026, built with a UWP-capable Meson fork using `--uwp` and `-D_XBOX_UWP`. The org also hosts `dawn-uwp` (WebGPU/Dawn) and `SDL3-uwp`. — [XboxEmulationHub/mesa-uwp](https://github.com/XboxEmulationHub/mesa-uwp); [devmodewiki Mesa build guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/development/compiling-mesa-guide.mdx); [XboxEmulationHub org](https://github.com/XboxEmulationHub)
- Swapchain and HDR (Walbourn, Xbox One era): UWP always reports a 1920x1080 window, and Xbox One X can create a 3840x2160 swapchain. For Series S he advises 2560x1440. "UWP apps have limited HDR access", and games wanting HDR should contact ID@Xbox. — [Walbourn Xbox One post source](https://raw.githubusercontent.com/walbourn/walbourn.github.io/main/_posts/2017-12-15-directx-and-uwp-on-xbox-one.md)

### Inferences
- FL 11_0 with SM 6.4 and no Agility SDK means no guaranteed access to Enhanced Barriers, mesh shaders, DXR, SM 6.5+ or Sampler Feedback. Features available at FL 11_0 through `CheckFeatureSupport`, such as resource binding tier, ROVs and typed UAV loads, must be probed at runtime. Dozen's requirements (FL 11_0+ with specific options) may or may not be met.
- An RPCS3 port needs one of three things: (a) a native D3D12 backend, a large new renderer, as eden-xbox built for Eden; (b) Vulkan-over-D3D12 (Dozen) on Xbox, unproven and probably limited to about Vulkan 1.0/1.1; or (c) GL-over-D3D12 via mesa-uwp, which has historically lacked GL 4.3 and remains unverified on Xbox. SPIR-V → DXIL via `spirv_to_dxil` plus `dxil.dll` is already proven on Xbox by eden-xbox.

### Gaps
- No source states what `CheckFeatureSupport` actually reports under UWP on Series X: max FL, wave ops, 16-bit ops, ROVs, VRS, DXR or mesh-shader tiers.
- No source on whether Dozen (Vulkan) runs on Xbox, or which GL version mesa-uwp exposes on Xbox in 2026.
- No source on 4K/120 Hz or HDR swapchain support specifically for UWP on Series X in 2026.

## 5. JIT / dynamic code: codeGeneration, *FromApp APIs, W^X

### Takeaway
UWP JIT goes through the `codeGeneration` capability and the `*FromApp` memory APIs. Microsoft's docs say `VirtualProtectFromApp` lets pages become executable but never write and execute at once, so W^X is enforced. eden-xbox (dynarmic) states "W^X enforced inside the UWP AppContainer" on Xbox, and PPSSPP's UWP build treats the platform as W^X-exclusive. Dolphin's UWP fork runs its x64 JIT on Xbox. RPCS3's LLVM and ASMJIT code emission must therefore use a write-then-flip, or dual-mapped, executable memory scheme.

### Cited Findings
- `VirtualProtectFromApp` is for Store apps with JIT and requires the `codeGeneration` capability. It "allows you to mark pages as executable, but does not allow you to set both write and execute permissions". The English page lists `PAGE_EXECUTE_READWRITE` and `PAGE_EXECUTE_WRITECOPY` as errors; some localized versions list `PAGE_EXECUTE` and `PAGE_EXECUTE_READ` as allowed only with `codeGeneration`. The two versions are worded differently. — [VirtualProtectFromApp (MSDN)](https://msdn.microsoft.com/en-us/library/mt169846(v=vs.85)); [localized Learn copy](https://learn.microsoft.com/sv-se/windows/win32/api/memoryapi/nf-memoryapi-virtualprotectfromapp)
- eden-xbox: "Dynarmic JIT translates ARM64/ARM32 guest code to x64, with W^X enforced inside the UWP AppContainer". Code cache is 128 MiB per core on Xbox, and a JIT pre-warm budget of 64, then 32, then 16 MiB per core is used. — [eden-xbox README](https://github.com/JulianDr14/eden-xbox)
- PPSSPP `Common/MemoryUtil.cpp` uses `VirtualAllocFromApp` and `VirtualProtectFromApp` under `PPSSPP_PLATFORM(UWP)`. `PlatformIsWXExclusive()` returns true for UWP, and a write-plus-execute request asserts "W^X is in effect, can't both write and exec". — [PPSSPP MemoryUtil.cpp](https://raw.githubusercontent.com/hrydgard/ppsspp/master/Common/MemoryUtil.cpp)
- Dolphin UWP v1.1.9 lists JIT64 among CPU backends and says "JITARM64 Will never work for UWP", so x64 JIT is the working path on Xbox. — [XboxEmulationHub/dolphin Discussion #1](https://github.com/XboxEmulationHub/dolphin/discussions/1)
- A mingw-w64 CRT commit notes that a `VirtualProtect` shim mapped to `VirtualProtectFromApp` works on UWP without `codeGeneration` as long as no pseudo-relocs touch code sections. — [mingw-w64 commit d6c933ed](https://third-party-mirror.googlesource.com/mingw-w64/+/d6c933edb04bdf0c0baf06703305bc597a25eed5)
- Conflicting signal: XBSX2's master `WinHostSys.cpp` still requests `PAGE_EXECUTE_READWRITE` for writable executable pages (see section 2). Whether that path is used on Xbox was not verified. — [XBSX2 WinHostSys.cpp](https://raw.githubusercontent.com/XboxEmulationHub/XBSX2/master/common/Windows/WinHostSys.cpp)

### Inferences
- RPCS3's LLVM JIT (MCJIT/ORC memory manager) and ASMJIT (SPU/PPU trampolines) assume RWX or freely toggled pages. A port needs a custom memory manager: allocate RW, emit, call `VirtualProtectFromApp`(RX), then `FlushInstructionCache`. Alternatively, dual-map one section RW and RX with `MapViewOfFile3FromApp`; that does not violate per-page W^X, but it is untested on Xbox. Patchable code such as SPU block linking or self-modifying trampolines needs RX↔RW flips, a performance cost.
- eden's 128 MiB per core code-cache cap reflects memory-budget pressure, not just API limits. RPCS3's PPU LLVM module cache (precompiled objects) will compete for the 5 GiB.

### Gaps
- No Microsoft Xbox-specific statement on `codeGeneration` behavior was found. Evidence comes from emulator ports.
- Not verified whether dual RW/RX views of one section are permitted on Xbox, or whether `CreateFileMappingFromApp` with `PAGE_EXECUTE_*` is allowed.

## 6. File system access: LocalState, USB, broadFileSystemAccess, pickers, SMB, file-size limits

### Takeaway
In practice Dev Mode UWP emulators read from two places. The first is their own `LocalState`, filled through Device Portal's File Explorer. The second is an NTFS external drive (e.g. `E:\`) set as "Media", with an "ALL APPLICATION PACKAGES" ACL that lets the AppContainer open files by Win32 path. File pickers and declared file-type associations (`removableStorage`) also work, and SMB shares without domain auth are usable. A 2020-era press claim says UWP on Xbox cannot access files over 2 GB; it is unverified for 2026 and matters for 4–25 GB PS3 ISOs. No source documents `broadFileSystemAccess` on Xbox.

### Cited Findings
- USB preparation: format the drive NTFS, then add the "ALL APPLICATION PACKAGES" principal with Full Control (replace child permissions). Connect it to the Xbox and set it as **Media**, not "Games and Apps". The XboxMediaUSB tool sometimes applies wrong permissions. — [devmodewiki setup guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/xbox-setup/xbox-developer-mode-setup.mdx)
- XBSX2: the BIOS on USB lives at `E:\XBSX2\bios`, or internally at `LocalAppData\XBSX2\LocalState\bios`. Internal storage "works … but is limited" and USB is recommended; incorrect NTFS permissions block USB access. — [devmodewiki xbsx2-guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/emulation/xbsx2-guide.mdx)
- eden-xbox file access. Games go in `LocalState/games/` via Device Portal File Explorer, or in external folders and USB added by folder browser or typed absolute path. "The Xbox browser only shows Eden's internal storage, removable volumes Windows exposes, and folders with saved permissions." The package's file-type associations limit removable and UNC access to `.nsp/.xci/.nro/.keys/.nca`. SMB shares that need domain auth fail without `enterpriseAuthentication`. `RemovableDevices`/UNC behavior on Xbox is "still untested". — [temamumtaza/eden-xbox README](https://github.com/temamumtaza/eden-xbox)
- How-To Geek and TechRepublic (2020): "UWP apps can only access files that are 2GB or smaller" on Xbox One / Series X|S in Dev Mode, "unique to developer mode". — [How-To Geek](https://www.howtogeek.com/703443/how-to-put-your-xbox-series-x-or-s-into-developer-mode/); [TechRepublic](https://www.techrepublic.com/article/xbox-series-s-and-x-developer-mode-3-things-you-can-do-with-it-and-3-you-cant/)
- An older user report lists USB as `E:` and says you "can't write" to it and that files on USB "can't be >2GB". This is a single anecdotal report from the Xbox One era. — [dev.to "TIL UWP on Xbox One"](https://dev.to/goldenxp/til-uwp-on-xbox-one-4276)
- Official known issue: `StorageFile.CopyAsync` of an encrypted file to an unencrypted destination throws access-denied. — [Known issues with UWP on Xbox (search extract)](https://learn.microsoft.com/en-in/windows/uwp/xbox-apps/known-issues)
- Microsoft Q&A threads on PC show `broadFileSystemAccess` paired with `removableStorage`, and a case where `broadFileSystemAccess` + `runFullTrust` stopped the app appearing in File System privacy settings. None of these are about Xbox. — [Q&A 952796](https://learn.microsoft.com/en-us/answers/questions/952796/access-denied-when-using-broadfilesystemaccess-and); [Q&A 9842](https://learn.microsoft.com/en-us/answers/questions/9842/question-about-uwp-and-broadfilesystemaccess)

### Inferences
- The ACL trick (granting the AppContainer SID group access) is what lets ports use plain Win32 `CreateFile2`/`CreateFileFromApp` paths on `E:\`. That suits RPCS3's VFS mapping (`/dev_hdd0`, `/dev_bdvd`, extracted game folders), probably better than `StorageFile`.
- Dolphin (Wii ISOs about 4.4 GB) and XBSX2 (DVD9 ISOs about 8.5 GB) are used widely from USB. That suggests the 2 GB limit is outdated or applies only to specific paths such as the 2020 media copier or StorageFile copies, but no source tested it directly. RPCS3 can largely sidestep the question with extracted (folder-format) games.

### Gaps
- No primary source verifies the current >2 GB single-file limit status, exFAT or FAT32 support for Dev Mode app access, or `broadFileSystemAccess` behavior on Xbox.
- No benchmarks found comparing `StorageFile` and Win32 file I/O throughput on Xbox.

## 7. Restricted Win32 APIs; GDK; "Win32 on Xbox"

### Takeaway
Dev Mode runs UWP (AppContainer, x64) packages only. Console GDK titles need the GDKX extensions and an Xbox developer program (ID@Xbox), so the GDK is not a homebrew route on retail hardware. The public GitHub GDK is PC-only. Inside UWP, desktop APIs that RPCS3 uses (LoadLibrary, CreateProcess, the registry, console, raw VirtualAlloc/VirtualProtect, timeBeginPeriod) must be replaced with app-partition equivalents. Ports commonly resolve newer kernelbase API-set exports at runtime, as Dolphin does.

### Cited Findings
- The public GDK on GitHub is "a subset of what's available to developers in the Xbox developer program". Console development requires the "Microsoft Game Development Kit with Xbox Extensions (GDKX)", and titles "must be retargeted and rebuilt" for Xbox once it is installed. Console dev-kit pages need authorization. — [microsoft/GDK README](https://github.com/microsoft/GDK); [GDK June 2022 release notes](https://github.com/microsoft/GDK/releases/tag/June_2022)
- GDK titles are packaged as MSIXVC with `MicrosoftGame.config`. — [microsoft/GDK](https://github.com/microsoft/GDK)
- Xbox FAQ: games built in Dev Mode can be tested, but publishing requires ID@Xbox; "x86 app development and store submissions are no longer supported". — [Xbox UWP FAQ source](https://raw.githubusercontent.com/MicrosoftDocs/windows-dev-docs/docs/uwp/xbox-apps/frequently-asked-questions.yml)
- XBSX2 requires Dev Mode: "Retail consoles aren't supported." — [devmodewiki xbsx2-guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/emulation/xbsx2-guide.mdx)
- Dolphin's UWP memory code loads `VirtualAlloc2FromApp`/`MapViewOfFile3FromApp` dynamically from the `api-ms-win-core-memory-l1-1-6` API set rather than linking kernel32 desktop APIs. — [Dolphin MemArenaWin.cpp](https://raw.githubusercontent.com/XboxEmulationHub/dolphin/master/Source/Core/Common/MemArenaWin.cpp)
- The Xenia UWP build guide removes `file_picker.win.cc` from the UWP project because it "is known to cause build issues". This is an example of desktop UI code that must be stripped. — [devmodewiki Xenia guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/development/compiling-xenia-guide.mdx)
- `SetThreadAffinityMask` documentation lists UWP apps as supported (search extract). — [SetThreadAffinityMask](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadaffinitymask)
- Community infrastructure for UWP ports exists: `SDL3-uwp`, `dawn-uwp`, `mesa-uwp` and `uwp-dep`. — [XboxEmulationHub org](https://github.com/XboxEmulationHub)

### Inferences
- RPCS3's Qt frontend cannot be shipped as-is. Qt has a deprecated WinRT port, so community ports replace the UI, as XBSX2 did with `pcsx2-uwp` alongside `pcsx2-qt` and eden-xbox with an "Xbox-focused frontend". SDL3-uwp is the likely input and window layer.
- APIs to audit in RPCS3: `LoadLibrary` (→ `LoadPackagedLibrary`), `VirtualAlloc`/`VirtualProtect`/`MapViewOfFile*` (→ `*FromApp`), `CreateFileMapping` (→ `CreateFileMappingFromApp`), `timeBeginPeriod` (no UWP equivalent; use waitable timers), `CreateProcess`, the console, the registry, and `GetModuleHandle` of system DLLs. The UWP status of each was not individually verified in this session.

### Gaps
- No Microsoft statement found that explicitly says "GDK titles cannot run in retail Dev Mode". It follows from GDKX gating and dev-kit requirements, but no source says it outright.
- Per-API UWP availability for `timeBeginPeriod`, `SetThreadSelectedCpuSets`, `LoadPackagedLibrary` and `GetModuleHandle` was not confirmed. The Microsoft "Win32 and COM APIs for UWP apps" list could not be fetched.

## 8. Packaging, deployment, Dev Mode activation, and 2024–2026 changes

### Takeaway
Activation: install "Xbox Dev Mode" from the Store, enter its code at aka.ms/activatexbox under a Partner Center/Store developer account, and the console reboots into Dev Home. Individual developer registration has been free since mid-2025 with government-ID verification; the old fee was $19. Deploy signed x64 `.appx`/`.msix`/bundles with VCLibs dependencies through Device Portal (`https://<ip>:11443`) or Visual Studio Remote Machine, set the app to Game, and copy data into LocalState with Device Portal's File Explorer. Microsoft said in a statement (date not confirmed) that it has "no plans to remove or disable Developer Mode". Retail-mode emulators have been blocked and punished with suspensions since 2023.

### Cited Findings
- Activation steps: leave the Xbox Update Preview first (avoids error E208). Install the "Xbox Dev Mode" Store app, enter the displayed code at aka.ms/activatexbox, and the console restarts. Then allocate at least 5 GB in Manage Dev Storage and enable Device Portal (with optional auth) under Settings > Remote Access Settings. Switching Dev ↔ Retail "deletes your Quick Resume states" and local captures. — [devmodewiki setup guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/xbox-setup/xbox-developer-mode-setup.mdx)
- Account cost. Individual accounts are free but need a government photo ID, a selfie and SMS verification. The community guide says company accounts cost $99 one-time. — [devmodewiki setup guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/xbox-setup/xbox-developer-mode-setup.mdx). Microsoft: under the new onboarding there are "no registration fees", starting from storedeveloper.microsoft.com; the legacy Partner Center flow and non-flighted markets may still charge. — [Microsoft Learn: Free developer registration for individual developers](https://learn.microsoft.com/en-us/windows/apps/publish/whats-new-individual-developer). Announced May 2025, effective about June 2025. — [playday.one (19 May 2025)](https://playday.one/2025/05/19/microsoft-makes-individual-developer-accounts-free-next-month/). The two sources disagree on whether company accounts still pay.
- Earlier guides (2020–2024) cite a one-time $19 individual fee. — [pchardwarepro guide](https://www.pchardwarepro.com/en/Activate-developer-mode-on-Xbox-Series-X-step-by-step./)
- Neowin reported that Partner Center accounts used for Dev Mode were deactivated in a cleanup. Microsoft said they would be re-enabled and that "We have no plans to remove or disable Developer Mode on Xbox consoles." The date was not retrieved. — [Neowin](https://www.neowin.net/news/microsoft-is-revoking-access-to-dev-mode-on-xbox-series-s-and-x-consoles/)
- Device Portal deployment. Open `https://<XBOX-IP>:11443`, then Apps → Install Certificate (the `.cer`) and Apps → Deploy, choosing the `.appx` and adding `Microsoft.VCLibs.x64.14.00.appx` as a dependency. Update in place with the same certificate; do not uninstall first. Never expose the Device Portal to the internet. — [eden-xbox README](https://github.com/temamumtaza/eden-xbox). Device Portal uses a self-signed TLS certificate on port 11443. — [openappx (PyPI)](https://pypi.org/project/openappx/)
- Microsoft's Xbox devdocs describe the sideloading order: sideload the framework packages matching the VS version used to build, then register the app. — [devdocs.xbox.com: packaging framework packages](https://devdocs.xbox.com/build/core-features/common/packaging/packaging-framework-packages)
- Visual Studio route: VS2022 with the UWP workload, building x64 Release. Use Publish → Create App Packages → Sideloading with a certificate, then deploy via Device Portal or VS "Remote Machine" on the same LAN. — [devmodewiki Xenia guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/development/compiling-xenia-guide.mdx). VS remote deployment requires Dev Mode, a signed-in user (error 0x87e10008 otherwise) and "Universal (Unencrypted Protocol)" authentication; Retail Mode cannot be targeted. — [Xbox UWP FAQ source](https://raw.githubusercontent.com/MicrosoftDocs/windows-dev-docs/docs/uwp/xbox-apps/frequently-asked-questions.yml)
- Known issue: VS deployment fails when Parental Controls are on. Deploy with them off, launch once on console and enter the PIN. — [Known issues with UWP on Xbox (search extract)](https://learn.microsoft.com/en-in/windows/uwp/xbox-apps/known-issues)
- Retail-mode enforcement. Microsoft closed the retail-mode emulator loophole and VGC reported 15-day suspensions for players running emulators in retail mode, around mid-2023 (date inferred from search metadata). — [VGC](https://www.videogameschronicle.com/news/xbox-has-reportedly-started-suspending-players-who-run-emulators-in-retail-mode/). LevelUp later reported a paid third-party group re-enabling retail-mode emulation; Microsoft was not involved. — [LevelUp](https://www.levelup.com/en/news/746367/Emulation-has-returned-to-Xbox-Series-XS-in-retail-mode-but-some-players-are-disappointed/)
- The Dev Mode emulator ecosystem was still active in 2026: XBSX2 2.0.9.1 (1 Sep 2026), Xenia Canary 1.1.7.1 (4 Apr 2026), and ScummVM 2026.3.1.62 listed by Emulation Revival (search extract). — [Emulation Revival](https://emulationrevival.github.io/xbox-dev-mode/emulators.html)

### Inferences
- A finished RPCS3 port would be distributed as a sideloaded, self-signed MSIX plus VCLibs, the same as XBSX2, Dolphin and Xenia UWP. Users would copy firmware (PS3UPDAT.PUP) and games to an ACL'd NTFS USB drive or LocalState.
- There is no viable retail path. UWP games are not accepted in the Store, Store apps are limited to the App budget (1 GB, 45% GPU), and retail sideloading is actively punished.

### Gaps
- No exact 2026 Microsoft documentation on Dev Mode program terms was retrieved (learn.microsoft.com blocked). The Neowin deactivation episode's date was not confirmed.
- No info found on any 2025–2026 Xbox OS changes to the UWP resource table.

## 9. Other limits: PLM/suspension, title-safe, swapchain resolution/refresh

### Takeaway
UWP games are suspended, and may be terminated, when backgrounded. RPCS3 must handle `Suspending`/`Resuming` and the B/Back request. If the Back request is unhandled, the app is suspended back to Dev Home. Official guidance assumes a 1920x1080 logical window with larger swapchains per device; Series S should target about 1440p. HDR is limited for UWP. No 2026 source documents 4K/120 Hz for UWP on Series X.

### Cited Findings
- "Games are suspended and terminated in the background"; the background memory cap is 128 MB for apps. — [Microsoft Learn: System resources (search extract)](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation)
- Handle `BackRequested` and set `Handled = true`. Otherwise the B button returns to Dev Home and suspends the app. Read B via Windows.Gaming.Input or `xinput_uap.lib`. — [Walbourn Xbox One post source](https://raw.githubusercontent.com/walbourn/walbourn.github.io/main/_posts/2017-12-15-directx-and-uwp-on-xbox-one.md)
- Window and swapchain: UWP reports 1920x1080. Use 3840x2160 only on capable hardware, 2560x1440 on Series S, and note UWP HDR is limited (Xbox One-era guidance, Series S note included). Detecting the device model needs `GetGamingDeviceModelInformation` (SDK 16299+). — [Walbourn Xbox One post source](https://raw.githubusercontent.com/walbourn/walbourn.github.io/main/_posts/2017-12-15-directx-and-uwp-on-xbox-one.md)
- eden-xbox is still validating "error recovery (returning to the library after a GPU failure)" on Series, which suggests device-removed handling is fragile. — [eden-xbox README](https://github.com/JulianDr14/eden-xbox)
- XBSX2 recommends Optimal Frame Pacing plus Vsync for smooth output. — [devmodewiki xbsx2-guide](https://raw.githubusercontent.com/XboxEmulationHub/devmodewiki/master/docs/emulation/xbsx2-guide.mdx)

### Inferences
- RPCS3 must checkpoint or pause emulation threads on `Suspending`. PLM may terminate a suspended game, which would lose emulator state unless RPCS3 savestates on suspend. LLVM caches and other disk caches should be flushed on suspend.
- Title-safe area: TV overscan guidance for Xbox UWP (draw UI inside the inner area) applies to any custom frontend. Specific percentages were not retrieved.

### Gaps
- No source on the Series X UWP maximum swapchain size, 120 Hz or VRR availability, or HDR10 output from UWP in 2026.
- Title-safe percentages and the PLM timeout (how long `Suspending` may run on Xbox) were not retrieved.
