# Prior art: emulators ported to Xbox One / Series X|S Dev Mode (UWP), and RPCS3-on-Xbox efforts (status as of 2026-10-12)

Method note: most claims below were checked against the source code itself. I shallow-cloned the repos on 2026-10-12 and grepped them: XboxEmulationHub/XBSX2, XboxEmulationHub/dolphin, SirMangler/xenia (canary_experimental), hrydgard/ppsspp, XboxEmulationHub/RetroArch, XboxEmulationHub/mesa-uwp, danprice142/Azahar-UWP, XboxEmulationHub/xbox-swap, worleydl/UnleashedRecomp-uwp, the full commit history of stenzek/duckstation, and the Dev Mode Wiki and Emulation Revival site sources. Where a claim comes only from news or forums, it is marked as such. Many news domains (VGC, Windows Central, GBAtemp, Phoronix, wccftech, emulationrevival.github.io) could not be fetched directly from this environment. For those, search-result summaries were used and are flagged.

---

## 1. RPCS3 on Xbox: has anyone ported it, what did the RPCS3 team say, and what were the results?

### Takeaway
As of 2026-10-12 **no working RPCS3 build for Xbox (UWP/Dev Mode) exists in any verifiable public source**. The RPCS3 team has said repeatedly, from 2020 to 2024, that they will not port to UWP, and has closed and locked the requests. The only public RPCS3-UWP code is a single-commit fork published in Aug 2026 (Edem180599/rpcs3-uwp). It is a mostly AI-generated scaffold whose own audit documents record thousands of compile errors, and it shows no evidence of a successful build or of running on hardware. The only PS3 emulator ever distributed for Xbox Dev Mode is a ChonkyStation 3 "proof of concept". It loads one decrypted ELF at a time, uses RPCS3 on a PC only to decrypt EBOOTs, and has since been delisted from the main community store.

### Cited Findings
**RPCS3 team statements (chronological)**
- 2020-05-16: the official @rpcs3 account replied "We will not port RPCS3 to UWP". The date is decoded from the tweet's snowflake ID 1261773023440441344. — [RPCS3 on X](https://x.com/rpcs3/status/1261773023440441344?lang=en)
- 2020-11-22: issue #9315, "[Feature Request] xbox series s/x port in dev mode". AniLeo (RPCS3 member): "We've already mentioned RPCS3 won't be ported to UWP, at least not by us". — [RPCS3 issue #9315](https://github.com/RPCS3/rpcs3/issues/9315)
- 2021-05-15, in #9315: AniLeo said "RPCS3 only has OpenGL 4.3 and Vulkan renders", and that ANGLE "only translates from OpenGL ES 2.0 and 3.0, not desktop OpenGL 4.3". — [RPCS3 issue #9315](https://github.com/RPCS3/rpcs3/issues/9315)
- 2022-06-03 and 2022-06-07, in #9315: AniLeo said the Series X cannot run the Mesa drivers. A user (danprice142) then posted a forwarded email from a Mesa/Microsoft developer. It said neither Mesa option "would currently work with UWP" and listed the blockers: missing winsys extensions, no ICD loader in UWP, banned window-hook APIs, and no D3D12 interop. It added: "We (Microsoft) don't really have plans to add this kind of stuff." — [RPCS3 issue #9315](https://github.com/RPCS3/rpcs3/issues/9315)
- 2024-07-10: issue #15787, "[Feature request] Support for Xbox through new Mesa UWP port", pointing to aerisarn/mesa-uwp. FlexBy420 (contributor) said the Mesa port "doesn't change the fact that there isn't enough RAM in dev mode". AniLeo closed it the same day as "not planned", a duplicate of #9315 that is "still blocked by other dev mode restrictions". — [RPCS3 issue #15787](https://github.com/RPCS3/rpcs3/issues/15787)
- 2024-11-23, in #9315: AniLeo said the team has no Series X owner and nobody wants to port to UWP, "all the technical limitations from dev mode aside", so "this is a won't do regardless". — [RPCS3 issue #9315](https://github.com/RPCS3/rpcs3/issues/9315)
- 2024-12-30 to 2025-05-22, in #15787: user edvinkuric asked how to build RPCS3 as a UWP app: whether to remove Qt, how to add Mesa, and which devkit limits apply. No result was ever reported. A user claimed 16 GB of RAM is enough. Another user (Gael2433, not a maintainer) answered that Dev Mode limits memory "to about 4GB". The RPCS3 org account then locked the thread on 2025-05-22. — [RPCS3 issue #15787](https://github.com/RPCS3/rpcs3/issues/15787)

**Code attempts**
- **Edem180599/rpcs3-uwp** (GPL-2.0; created 2026-08-17, pushed 2026-08-17, 0 stars, one commit, no releases). — [GitHub repo](https://github.com/Edem180599/rpcs3-uwp)
  - The root contains `XBXS3_UWP/`, `CMakeLists_uwp.txt`, `build_uwp.cmd`, `mesa-uwp-build/`, `llvm-prebuilt/`, `create_d3d12_stubs.py`, `fix_stub_wolfssl_curl.ps1`, `rpcs3_uwp_complete.rar` and several French "EXPERT_REVIEW"/"AUDIT" documents. — [GitHub repo](https://github.com/Edem180599/rpcs3-uwp)
  - `CMakeLists_uwp.txt` targets `CMAKE_SYSTEM_NAME=WindowsStore` with these options: `USE_D3D12=ON`, `USE_VULKAN=OFF`, `USE_MESA_UWP=ON` ("link contre mesa-uwp-build (vulkan_dzn.dll + spirv_to_dxil.lib)"), `USE_XBOX_SWAP=ON` ("allocateur file-backed >5 GB RAM") and `USE_SDL=OFF`. The entry point is a C++/WinRT `IFrameworkView` in `XBXS3_UWP/App.cpp`. — [CMakeLists_uwp.txt](https://raw.githubusercontent.com/Edem180599/rpcs3-uwp/main/CMakeLists_uwp.txt); [App.cpp](https://raw.githubusercontent.com/Edem180599/rpcs3-uwp/main/XBXS3_UWP/App.cpp)
  - `AUDIT_COMPLET.md` (dated 2026-08-15) records **6,272 MSVC errors across 11 projects**. The largest groups are asmjit (3,177, from mixing 1.21 sources with 1.11 headers) and the `d3d12_backend` (2,378: incomplete D3D12MA stub, `D3D12_OPTIONS14/15/16` missing from the SDK, wrong field names). libusb, miniupnpc, hidapi, rtmidi, cubeb, wolfssl and OpenAL are listed as UWP-incompatible and to be disabled. After patches it claims "0 erreur attendue (extrapolation)", i.e. zero errors are expected by extrapolation, not observed. — [AUDIT_COMPLET.md](https://raw.githubusercontent.com/Edem180599/rpcs3-uwp/main/AUDIT_COMPLET.md)
  - `EXPERT_FINAL_REVIEW.md` says "85 stubs créés" (85 stub headers created) for missing D3D12 headers and declares the package "BUILDABLE" subject to prerequisites. `GUIDE_FINALISATION.md` is written "pour agent (humain ou IA)" (for an agent, human or AI) and cites a 44,458-line implementation plan. The audit paths point to `/home/z/my-project/...`. — [EXPERT_FINAL_REVIEW.md](https://raw.githubusercontent.com/Edem180599/rpcs3-uwp/main/EXPERT_FINAL_REVIEW.md); [GUIDE_FINALISATION.md](https://raw.githubusercontent.com/Edem180599/rpcs3-uwp/main/GUIDE_FINALISATION.md)
  - The `PerformanceSummary` file in the repo root is empty. — [raw file](https://raw.githubusercontent.com/Edem180599/rpcs3-uwp/main/PerformanceSummary)
- GitHub repository searches for "rpcs3 uwp", "rpcs3 xbox", "rpcs3 winrt", "xbps3" and PS3/Xbox/UWP turned up no other RPCS3 Xbox port. A code search for `rpcs3 filename:Package.appxmanifest` matches only `XBXS3_UWP/Package.appxmanifest` in Edem180599/rpcs3-uwp. — [GitHub code search result](https://github.com/Edem180599/rpcs3-uwp)

**The only PS3 emulator released for Xbox Dev Mode: ChonkyStation 3 UWP (proof of concept)**
- The Dev Mode Wiki guide (tested 2025-03-23) describes a "highly experimental and incomplete" proof of concept "Ported By: @MoonPower". It loads only "the *first* `.elf`" in `LocalState\dev_hdd0\game`, has no game list, and crashes are "common". Changing the app type to Game "improves stability", and storage is internal LocalState only. — [Dev Mode Wiki, ChonkyStation 3 guide](https://wiki.sternserv.xyz/docs/experimental/chonkystation3-guide/) (source: [devmodewiki .mdx](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/experimental/chonkystation3-guide.mdx))
- The package is an MSIX bundle that needs `Microsoft.NET.Native.Framework.2.2`, `Microsoft.NET.Native.Runtime.2.2` and VCLibs. The app tile is named "ChunkyStation C#". — [devmodewiki .mdx](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/experimental/chonkystation3-guide.mdx)
- RPCS3's only role is on a PC: "Utilities > Decrypt PS3 Binaries" turns EBOOT.BIN into an ELF that the Xbox build can load. — [devmodewiki .mdx](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/experimental/chonkystation3-guide.mdx)
- The Emulation Revival store data marks the ChonkyStation 3 entry `"missingFromYaml": true` (checked 2026-10-08), so it is no longer in the experimental-apps list. A compatibility doc card still exists. — [emulationrevival discord-forum-posts.json](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/json/discord-forum-posts.json); [compatibility.html](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/compatibility.html)
- Upstream ChonkyStation3 (liuk7071) is described as able to boot "a few simple commercial games" and is listed as discontinued, paused in favour of ChonkyStation4. — [AlternativeTo](https://alternativeto.net/software/chonkystation3/about); [Le Bottin des Jeux Linux](https://lebottinlinux.vps.a-lec.org/Nouveau_files/ChonkyStation3-16480.html)

**Rumors and videos**
- July 2024 coverage of a Modern Vintage Gamer video said a Mesa (OpenGL/Vulkan) UWP port "in theory" could make PS3 and Switch emulation possible on Xbox. The coverage showed Citra (3DS) running, not RPCS3. — [wccftech](https://wccftech.com/xbox-series-x-run-playstation-3-games/amp/); [GGRecon](https://www.ggrecon.com/articles/new-update-could-mean-your-xbox-plays-ps3-games-before-your-ps5-can/) (search-summary only; page not fetchable here)
- No reputable outlet, RPCS3 statement or repository confirms any video of RPCS3 running on an Xbox. — (negative result across the searches above)

**Related non-Xbox ports of RPCS3 (2025–2026) that show the codebase being re-hosted**
- RPCS3-Android (2025): "discontinued", Alpha-7 is the last release, and it was merged into RPCSX. — [RPCS3-Android README](https://github.com/RPCS3-Android/rpcs3-android)
- aps3e: an experimental Android PS3 emulator "ported and optimized based on RPCS3". — [aenu1/aps3e](https://github.com/aenu1/aps3e)
- ARMSX3 (Aug 2026): an Android port with a separate Kotlin UI app and a ~1.3 GB core `.so`. — [ARMSX2/ARMSX3](https://github.com/ARMSX2/ARMSX3)
- RPCS3 iOS release repo (XITRIX, Aug 2026). — [XITRIX/RPCS3-iOS-Releases](https://github.com/XITRIX/RPCS3-iOS-Releases)
- rpcs3-PS5 (lavavex, created 2026-10-07): RPCS3 runs as a native title on a jailbroken PS5. It renders through Vulkan on a RADV port, outputs audio through SceAudioOut, reads input through ScePad, and its "frontend is RPCS3's own Big Picture Mode". Need for Speed Carbon is reported "fully playable". — [lavavex/rpcs3-PS5 README](https://github.com/lavavex/rpcs3-PS5)
- Upstream RPCS3 gained a controller-driven **Big Picture Mode rendered with the RSX overlay system** on 2026-08-25 (commit "Add Big Picture Mode using RSX overlay UI" by BeezBumba), with many Megamouse fixes in Aug–Sep 2026. It is started via `Emulator::BootBigPictureMode()` and a `--big-picture` CLI argument. — [RPCS3 BigPicture commits](https://github.com/RPCS3/rpcs3/commits/master/rpcs3/Emu/RSX/Overlays/BigPicture); [overlay_big_picture.h](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/Overlays/BigPicture/overlay_big_picture.h); [rpcs3.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/rpcs3.cpp)

### Inferences
- An RPCS3 Xbox port would be first-of-its-kind and unsupported upstream. Do not plan on upstreaming. The RPCS3 team has locked both issues, and AniLeo's 2024 statement says "won't do" even setting technical limits aside.
- Edem180599/rpcs3-uwp shows that agent-generated scaffolding hits the same blockers as everyone else: asmjit and LLVM in an AppContainer, a removed D3D12 backend, and desktop-only third-party libraries. It cannot be cited as evidence of feasibility. The one useful thing in it is its dependency triage list (libusb, miniupnpc, hidapi, rtmidi, cubeb, OpenAL and wolfssl are flagged for disabling).
- RPCS3's new Big Picture Mode (an RSX-overlay UI) is the natural Xbox frontend. It removes the need for Qt, which mirrors how XBSX2 adopted PCSX2's ImGui FullscreenUI (see section 3). The PS5 port already uses it as its only frontend.
- The PS5 port proves that RPCS3 can be re-hosted on a console with a non-Windows Vulkan driver. An Xbox port instead needs Vulkan-over-D3D12 (Dozen) or a native D3D12 renderer.

### Gaps
- I could not search the RPCS3 Discord or forum, YouTube, or r/XboxRetailHomebrew directly; the proxy blocks those sites or they need logins. No statement beyond GitHub and X was found.
- The source code of ChonkyStation 3 UWP is not linked anywhere I could find. Whether it is a C# reimplementation or a wrapper around the C++ core is unverified; the .NET Native dependencies and the "C#" app name suggest C#.
- The Edem180599 repo could not be cloned through the API (access-restricted in this session). Only raw files were read, so its actual buildability is unverified.

---

## 2. Dolphin UWP (SirMangler → SternXD → XboxEmulationHub): JIT, memory, D3D12/D3D11, controllers, file access, UI

### Takeaway
Dolphin UWP keeps Dolphin's core almost unchanged. The core is compiled as a normal desktop static lib with `WINRT_XBOX=1`. Desktop JIT memory calls (`VirtualAlloc` RWX) and the vectored exception handler (VEH) for fastmem work unchanged. The memory arena uses `VirtualAlloc2FromApp`/`MapViewOfFile3FromApp` placeholders. D3D12 is made the default backend, with logic ops disabled and a custom swap chain for the `CoreWindow`. Vulkan and OpenGL are compiled out. Qt is replaced by a custom ImGui frontend, and controllers use Windows.Gaming.Input.

### Cited Findings
- History: SirMangler released a Dolphin UWP beta on 2022-12-06. It supports up to 1440p on Series S, and Metroid Prime runs at roughly 20–25 fps on Xbox One. — [Dexerto](https://www.dexerto.com/tech/gamecube-emulator-dolphin-has-been-ported-to-xbox-2041763); [Windows Central](https://windowscentral.com/gaming/xbox/you-can-now-run-gamecube-and-wii-games-on-an-xbox-series-xors-with-dolphin) (search summary)
- The project is now maintained by Stern (SternXD). Store sources: SternXD/dolphin and XboxEmulationHub/dolphin. The latest release in store data is 1.1.9.0 (DolphinWinRT_1.1.9.0_x64.msix). The default branch's last commit is 2025-11-05 ("ImGuiFrontend: Seperate settings from main file"), and the org page shows the repo updated 2026-09-29. — [Emulation Revival emulators.yml](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/emulators.yml); [release_data.json](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/release_data.json); [XboxEmulationHub/dolphin](https://github.com/XboxEmulationHub/dolphin)
- The UWP shell is `Source/DolphinWinRT/` (main.cpp, Host.h, UWPUtils.h, Package.appxmanifest). The build guide's startup project is `DolphinWinRT`, built in Release x64 and deployed to the Xbox as a "Remote Machine". — [XboxEmulationHub/dolphin Source/DolphinWinRT](https://github.com/XboxEmulationHub/dolphin/tree/master/Source/DolphinWinRT); [Dev Mode Wiki compile guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/development/compiling-dolphin-guide.mdx)
- Manifest capabilities: `runFullTrust`, `broadFileSystemAccess`, `removableStorage`, `expandedResources`, `codeGeneration`, `internetClientServer` and `privateNetworkClientServer`. — [DolphinWinRT Package.appxmanifest](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/DolphinWinRT/Package.appxmanifest)
- Build: `Base.Dolphin.props` defines `WINRT_XBOX=1`. `HAS_OPENGL`, `HAS_VULKAN` and `USE_DISCORD_PRESENCE` are only defined when `WINRT_XBOX==0`, so OpenGL and Vulkan are compiled out on Xbox. — [Base.Dolphin.props](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/VSProps/Base.Dolphin.props)
- JIT: `Common::AllocateExecutableMemory` still calls `VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE)` with no UWP special case. Write-protect toggles use `VirtualProtect` with `PAGE_EXECUTE_READ`/`PAGE_EXECUTE_READWRITE`. — [MemoryUtil.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/Common/MemoryUtil.cpp)
- Memory arena: Dolphin resolves `VirtualAlloc2FromApp` and `MapViewOfFile3FromApp` dynamically from `api-ms-win-core-memory-l1-1-6.dll` (after `IsApiSetImplemented`). It reserves the guest region with `MEM_RESERVE | MEM_RESERVE_PLACEHOLDER` and maps views into placeholders. If the API set is missing it falls back to the old reserve-then-free approach. — [MemArenaWin.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/Common/MemArenaWin.cpp)
- Fastmem and fault handling: `AddVectoredExceptionHandler(TRUE, Handler)` is used unchanged. — [MemTools.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/Core/MemTools.cpp)
- Graphics:
  - Under `WINRT_XBOX` the D3D12 backend is placed first in the backend list (`backends.emplace(backends.begin(), DX12::VideoBackend)`). — [VideoBackendBase.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/VideoCommon/VideoBackendBase.cpp)
  - D3D12 sets `bSupportsLogicOp = false` on Xbox. — [D3D12 VideoBackend.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/VideoBackends/D3D12/VideoBackend.cpp)
  - The shared D3D swap chain uses `CreateSwapChainForCoreWindow` instead of `CreateSwapChainForHwnd`, and skips `MakeWindowAssociation` and the HWND client-rect logic. — [D3DCommon/SwapChain.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/VideoBackends/D3DCommon/SwapChain.cpp)
- Shader compilation on Xbox: `XboxUberShaderWorkarounds.h` caps `MAX_COMPILER_THREADS = 2` and `MAX_SHADER_VARIANTS = 64`, prefers ubershader modes, and uses "simplified blending" and "conservative compilation" under `WINRT_XBOX`. — [XboxUberShaderWorkarounds.h](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/VideoCommon/XboxUberShaderWorkarounds.h)
- UI: Qt is not used. A custom ImGui frontend in `Source/Core/UICommon/ImGuiMenu/` provides the main menu, settings, mapping window, netplay and an on-screen keyboard (`WinRTKeyboard.cpp`). `main.cpp` creates `ImGuiFrontend::ImGuiFrontend()`, swallows the B/Back button via `SystemNavigationManager::BackRequested`, and queries `HdmiDisplayInformation`. — [ImGuiMenu](https://github.com/XboxEmulationHub/dolphin/tree/master/Source/Core/UICommon/ImGuiMenu); [DolphinWinRT/main.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/DolphinWinRT/main.cpp)
- Input: uses the WGInput (Windows.Gaming.Input) ControllerInterface, with UWP ifdefs in `WGInput.cpp`. A known gotcha: "Xbox language must be set to English US for controller input". Per-game control configuration on the console was historically limited to premade configs, and users were told to configure on PC. — [WGInput.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/InputCommon/ControllerInterface/WGInput/WGInput.cpp); [emulators.yml](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/emulators.yml); [Dev Mode Wiki Dolphin guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/dolphin-guide.mdx)
- File access: `FileUtil.cpp` has a `WINRT_XBOX` path, including a custom `CreateFullPath`. Users keep games on an NTFS USB drive (`E:\`) with "ALL APPLICATION PACKAGES" permissions; user data lives in `LocalAppData\Dolphin\LocalState`. — [FileUtil.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/Common/FileUtil.cpp); [Dolphin guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/dolphin-guide.mdx)

### Inferences
- Dolphin ships a JIT that allocates RWX with plain `VirtualAlloc`, and a fastmem VEH, with no UWP fallback. Since it works on Series hardware, those desktop calls must be allowed for a Dev Mode app with `codeGeneration` and `runFullTrust`. This contradicts the documented `*FromApp` contract (section 8), so RPCS3 (LLVM and asmjit JIT, VEH-based access-violation handling) can probably keep its desktop code paths. Verify early on hardware.
- `bSupportsLogicOp=false` matters for RPCS3. RSX exposes logic ops, so a D3D12 renderer path on Xbox would have to emulate them or ignore them.
- The shader-compile thread cap hints that shader and PSO compilation contends with the emulator for CPU on Xbox. RPCS3's async shader compilation and pipeline cache would face the same pressure.

### Gaps
- I did not find a written upstream rationale for disabling logic ops on Xbox (driver limitation or bug), nor a benchmark comparing Dolphin UWP against a PC.
- SirMangler's original UWP repo is not among his current public GitHub repos; history before the SternXD/XboxEmulationHub forks was not reviewed.

---

## 3. XBSX2 (PCSX2 → UWP): Big Picture/ImGui UI, recompilers, D3D11/D3D12, fastmem mapping, file access, settings

### Takeaway
XBSX2 is the closest analogue to an RPCS3 port: a large x86-64 emulator that uses a recompiler plus shared-memory mirrors, now with a Big Picture-style UI. It keeps PCSX2's desktop memory code. `VirtualAlloc2` placeholders with `MapViewOfFile3` views, and a code region mapped RWX inside the same placeholder area, work unchanged with `mincore`. The UI is PCSX2's ImGui FullscreenUI, with Qt dropped. Only D3D11/D3D12 are built. SDL3 (a UWP fork) is used with the WGI/GameInput backends, and the app is a small C++/WinRT `IFrameworkView` shell (~1.2k lines). Contrary to the brief, the author credits are SirMangler and TheRhysWyrill, later maintained by SternXD, not worleydl.

### Cited Findings
- Credits:
  - The README credits "SirMangler / TheRhysWyrill – For rewriting PCSX2 for UWP". It recommends Series S/X, with Xbox One "supported with limited compatibility". — [XBSX2 README](https://github.com/XboxEmulationHub/XBSX2)
  - The UWP history begins with "Initial commit. Introduce WinRT/Xbox support." by sirmangler on 2023-05-13. Since 2025 nearly all UWP commits are by SternXD; the latest is "UWP/BuildVersion: Bump version to 2.0.9.1" on 2026-09-01. — [XBSX2 commit b115e7eb](https://github.com/XboxEmulationHub/XBSX2/commit/b115e7eb); [commit 88dbbc50](https://github.com/XboxEmulationHub/XBSX2/commit/88dbbc50)
  - Release v2.0.9.1 ships two packages, `XBSX2_2.0.9.1_AVX2.msixbundle` for Series and `..._SSE4.msixbundle` for Xbox One. — [release_data.json](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/release_data.json); [XBSX2 guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/xbsx2-guide.mdx)
- Prehistory: stenzek (DuckStation) built a standalone PCSX2 Xbox app, but "had not … released a complete version", and the community fork followed. — [Kotaku AU](https://www.kotaku.com.au/?p=1289681); [TheGamer](https://www.thegamer.com/new-ps2-emulator-lets-you-play-ratchet-clank-and-god-of-war-on-xbox-series-s/) (search summaries)
- Project layout:
  - `pcsx2-uwp/` holds `Package.appxmanifest`, `pcsx2-uwp.vcxproj` and `src/{main.cpp, UWPKeyboard.cpp, UWPSound.cpp, UWPUtils.h}`. — [pcsx2-uwp](https://github.com/XboxEmulationHub/XBSX2/tree/master/pcsx2-uwp)
  - The UWP project sets `AppContainerApplication=true`, `ApplicationType=Windows Store`, target SDK 10.0.26100 (minimum 10.0.22000), `WINRT_XBOX=1`. It links `xaudio2.lib; xinputuap.lib; windowsapp.lib` plus the PCSX2 dependency props. — [pcsx2-uwp.vcxproj](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/pcsx2-uwp.vcxproj)
  - The core `pcsx2.vcxproj` defines `WINRT_XBOX=1` and drops `ENABLE_OPENGL`, `ENABLE_VULKAN`, `SPU2X_CUBEB`, `SDL_BUILD` and `ENABLE_DISCORD_PRESENCE` for Xbox. — [pcsx2.vcxproj](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/pcsx2.vcxproj)
- Manifest capabilities: `internetClient`, `internetClientServer`, `codeGeneration`, `removableStorage`, and the restricted capabilities `runFullTrust`, `broadFileSystemAccess` and `expandedResources`. It also registers an `xbsx2:` protocol for launching from frontends. — [XBSX2 Package.appxmanifest](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/Package.appxmanifest)
- Memory and fastmem (no Xbox-specific changes):
  - `SharedMemoryMappingArea::Create` calls `VirtualAlloc2(..., MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS)`. `Map` splits placeholders with `VirtualFreeEx(..., MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)` and maps views with `MapViewOfFile3(..., MEM_REPLACE_PLACEHOLDER)`. Shared memory comes from `CreateFileMappingW(INVALID_HANDLE_VALUE, …)`. — [WinHostSys.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/common/Windows/WinHostSys.cpp)
  - `GS.cpp` adds `#pragma comment(lib, "mincore")` under `WINRT_XBOX` for these APIs. — [GS.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/GS/GS.cpp)
  - `Memory.cpp` maps both the EE/IOP/VU data memory and the recompiler code region into one `SharedMemoryMappingArea`. The code region uses `PageAccess_Any()` (RWX). VM init fails ("Failed to map main memory" / "Failed to allocate code memory") if either step fails; there is no fallback. — [Memory.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/Memory.cpp)
  - `HostSys::MemProtect` maps to `VirtualProtect` with `PAGE_EXECUTE_READWRITE` when write+exec is requested. Page faults use `AddVectoredExceptionHandler(1, ExceptionHandler)`. — [WinHostSys.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/common/Windows/WinHostSys.cpp)
- Graphics (D3D11 and D3D12 only):
  - `GS.cpp` compiles out the `OGL` and `VK` renderer types under `WINRT_XBOX`. — [GS.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/GS/GS.cpp)
  - The swap chain is fixed at 3 buffers on Xbox. The HWND/exclusive-fullscreen code and PIX debug markers are compiled out. — [GSDevice12.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/GS/Renderers/DX12/GSDevice12.cpp); [GSDevice11.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/GS/Renderers/DX11/GSDevice11.cpp)
  - `primitive_id` support becomes a user option (`DisablePrimitiveID`) on Xbox. — [commit a348bf8a](https://github.com/XboxEmulationHub/XBSX2/commit/a348bf8a); [GSDevice12.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/GS/Renderers/DX12/GSDevice12.cpp)
  - Commit 81cd055d (2026-07-12) forces `m_enhanced_barriers = false` with the log line "D3D12: Enhanced Barriers: Not Supported" on Xbox. It also replaces feedback barriers with explicit RENDER_TARGET→PIXEL_SHADER_RESOURCE→RENDER_TARGET transitions ("Flush RT caches on Xbox feedback barriers"). — [XBSX2 commit 81cd055d](https://github.com/XboxEmulationHub/XBSX2/commit/81cd055d)
- Display:
  - `GetMaxOutputResolution` uses `GetGamingDeviceModelInformation` to cap Xbox One/One S at 1080p and Series S at 1440p. The comment reads: "The Series S can output to 4K but if you try to make a 4K Swap Chain the application will crash". — [pcsx2-uwp/src/main.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/src/main.cpp)
  - The refresh rate is read from `HdmiDisplayInformation::GetCurrentDisplayMode().RefreshRate()`. — [pcsx2-uwp/src/main.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/src/main.cpp)
  - `WindowInfo::Type::WinRT` carries the `CoreWindow`. — [pcsx2-uwp/src/main.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/src/main.cpp)
- UI: there is no Qt. The app initializes `ImGuiManager::InitializeFullscreenUI()` (PCSX2's Big Picture mode) and adds `WINRT_XBOX` changes in `FullscreenUI.cpp` (15 sites), `FullscreenUI_Settings.cpp` (24 sites) and `ImGuiFullscreen.cpp`, including an "unofficial fork" warning. Text input goes through `UWPKeyboard` (CoreWindow character events into ImGui). — [pcsx2-uwp/src/main.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/src/main.cpp); [FullscreenUI.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/ImGui/FullscreenUI.cpp); [commit 07ebff99](https://github.com/XboxEmulationHub/XBSX2/commit/07ebff99)
- Input:
  - The DInput source and keyboard/mouse bindings are removed under `WINRT_XBOX`; the SDL and XInput sources remain. — [InputManager.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/Input/InputManager.cpp)
  - SDL is set with `SDL_SetHint("SDL_JOYSTICK_WGI","1")` and `"SDL_JOYSTICK_GAMEINPUT","1"` ("Xbox/UWP expose pads through WGI/GameInput") and initialized without the haptic subsystem. — [SDLInputSource.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/Input/SDLInputSource.cpp)
  - The hidden ordinal-100 `XInputGetState` (Guide button) is skipped. — [XInputSource.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/Input/XInputSource.cpp)
  - SDL comes from the org's SDL3-uwp fork (branch `uwp-3.4.16`, with a `src/video/winrt` driver; last commit 2026-09-03, "uwp: Minimal picker impl"). — [XboxEmulationHub/SDL3-uwp](https://github.com/XboxEmulationHub/SDL3-uwp)
- Audio: a custom `UWPSound.cpp` uses XAudio2 (`IXAudio2SourceVoice`, mastering voice) in place of cubeb. — [UWPSound.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/src/UWPSound.cpp)
- File access:
  - `FileSystem.cpp` includes `winrt/Windows.Storage.h` and replaces `GetLogicalDriveStrings` with a curated root list ("Filter Root directory list for UWP/Xbox"). — [FileSystem.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/common/FileSystem.cpp); [commit 8e4ac946](https://github.com/XboxEmulationHub/XBSX2/commit/8e4ac946)
  - Users put the BIOS and games on NTFS USB (`E:\XBSX2\...`) with "ALL APPLICATION PACKAGES" permissions, or in `LocalAppData\XBSX2\LocalState`. — [XBSX2 guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/xbsx2-guide.mdx)
- Settings and packaging:
  - PCSX2's `patches.zip` must be dropped into `pcsx2-uwp/Resources` before building. — [Compiling XBSX2 guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/development/compiling-xbsx2-guide.mdx)
  - Dependencies are built by `.github/workflows/scripts/uwp/build-dependencies.bat` (the xbsx2-windows-dependencies repo). — [Compiling XBSX2 guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/development/compiling-xbsx2-guide.mdx)
  - Users must set the app type to Game; "In 'App' mode, the console restricts CPU, GPU, and RAM access, which causes heavy stuttering". — [XBSX2 guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/xbsx2-guide.mdx)

### Inferences
- XBSX2 shows that the PCSX2/RPCS3-style "reserve a huge placeholder, map shared-memory views and a RWX code area into it" pattern works on Series consoles in Dev Mode with desktop APIs from `mincore`. VM init would otherwise fail outright. RPCS3's `utils::memory_reserve`/`utils::shm` mirrors and its RWX JIT regions are therefore likely portable without rewriting to `*FromApp` APIs, as long as the core is compiled as a desktop static lib and linked into a UWP shell.
- The D3D12 gotchas carry over to any D3D12 path for RSX: no Enhanced Barriers, SV_PrimitiveID problems, explicit feedback transitions, and the Series S 4K swap-chain crash.
- Reusing PCSX2's FullscreenUI pattern maps onto RPCS3's RSX-overlay Big Picture Mode (section 1). The XBSX2 changes show which settings to hide or patch for Xbox: exclusive fullscreen, renderer choices, keyboard and mouse.

### Gaps
- No primary benchmark of XBSX2 against a comparable PC was found. Exact first-public-release date is unclear: one Spanish-language article claims July 2022, while the code history starts 2023-05-13. The 2022 date may refer to stenzek's private build or a RetroArch core.

---

## 4. Xenia (Xbox 360) on Xbox: UWP ports, D3D12, 4 GB guest mapping, JIT

### Takeaway
Xenia Canary UWP is by SirMangler, released 2023-02-04, and is now maintained by DanP (danprice142/xenia-canary-uwp, "Blades style UI"). It uses only Xenia's native D3D12 backend. Upstream Xenia's Windows memory layer already implements `VirtualAllocFromApp`, `VirtualAlloc2FromApp`/`MapViewOfFile3FromApp` placeholders and W^X-aware code caches. The guest's virtual and physical views, laid out at a fixed 4 GB stride, therefore map on Xbox with little change. The UWP shell is about 675 lines, using XInput and ImGui.

### Cited Findings
- Repo and dates:
  - SirMangler/xenia, branch `canary_experimental`: "Xbox 360 Emulator Research Project for UWP", an unofficial Canary fork whose users are told not to ask for help on the official Xenia Discord. Last commit 2024-12-05. — [SirMangler/xenia](https://github.com/SirMangler/xenia)
  - The Emulation Revival listing names "Xenia Canary – An Xbox 360 emulator now with Blades style UI", porter Sir Mangler, maintainer DanP, source danprice142/xenia-canary-uwp. Store data: version 1.1.7.1, release 2026-04-04, first release 2023-02-04. — [emulators.yml](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/emulators.yml); [release_data.json](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/release_data.json); [danprice142/xenia-canary-uwp](https://github.com/danprice142/xenia-canary-uwp)
  - Windows Central reported a new canary UWP build on 2023-02-07 that fixed early problems, with limited compatibility at the time. — [Windows Central](https://windowscentral.com/gaming/xbox/xenia-xbox-series-xors-grabs-a-potentially-more-powerful-xbox-360-emulator) (search summary)
- Build: clone `-b canary_experimental --recursive`, set `xenia-canary-uwp` as the startup project in Release x64, remove `file_picker.win.cc`, and retarget zstd if needed. — [Dev Mode Wiki: compiling Xenia](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/development/compiling-xenia-guide.mdx)
- Manifest capabilities: `internetClient`, `runFullTrust`, `broadFileSystemAccess`, `removableStorage` and `codeGeneration`. `expandedResources` is not declared in SirMangler's tree. — [xenia-canary-uwp/Package.appxmanifest](https://github.com/SirMangler/xenia/blob/canary_experimental/xenia-canary-uwp/Package.appxmanifest)
- Memory (upstream Xenia code):
  - `memory_win.cc` uses desktop `VirtualAlloc`/`MapViewOfFileEx` when `WINAPI_PARTITION_DESKTOP | GAMES` is defined. Otherwise it uses `VirtualAllocFromApp`, `VirtualProtectFromApp` and `CreateFileMappingFromApp`.
  - Its `MapFileView` builds fixed-address views by reserving a placeholder with `VirtualAlloc2FromApp(..., MEM_RESERVE | MEM_RESERVE_PLACEHOLDER)` and then calling `MapViewOfFile3FromApp(..., MEM_REPLACE_PLACEHOLDER)`; a comment notes these "were added in 10.0.17134.0".
  - `IsWritableExecutableMemorySupported()` returns false in FromApp mode, so the code cache uses a separate writable view.
  - Source: [memory_win.cc](https://github.com/SirMangler/xenia/blob/canary_experimental/src/xenia/base/memory_win.cc); also present upstream: [xenia-project/xenia memory_win.cc](https://github.com/xenia-project/xenia/blob/master/src/xenia/base/memory_win.cc)
  - Guest layout: `virtual_membase_ = mapping_base_` and `physical_membase_ = mapping_base_ + 0x100000000`, with the heaps (v00000000, v40000000, v80000000, v90000000, physical, vA0000000, vC0000000, vE0000000) mapped as aliased views of one file mapping. — [memory.cc](https://github.com/SirMangler/xenia/blob/canary_experimental/src/xenia/memory.cc)
  - Exceptions: `AddVectoredExceptionHandler` for MMIO and write-watch, and `RtlInstallFunctionTableCallback` for the x64 JIT code cache's unwind info. — [exception_handler_win.cc](https://github.com/SirMangler/xenia/blob/canary_experimental/src/xenia/base/exception_handler_win.cc); [x64_code_cache_win.cc](https://github.com/SirMangler/xenia/blob/canary_experimental/src/xenia/cpu/backend/x64/x64_code_cache_win.cc)
- Graphics: `src/xenia/gpu/` has only a `d3d12` backend (plus null); there is no Vulkan in this tree. The presenter has `WINAPI_FAMILY_PARTITION` checks for the non-desktop surface path. — [xenia gpu dir](https://github.com/SirMangler/xenia/tree/canary_experimental/src/xenia/gpu); [d3d12_presenter.cc](https://github.com/SirMangler/xenia/blob/canary_experimental/src/xenia/ui/d3d12/d3d12_presenter.cc)
- UWP shell: `xenia-canary-uwp/` (App.cpp, XeniaUWP.cpp, window_uwp.cc, surface_uwp.cpp, WinRTKeyboard.cpp; ~675 lines). It feeds `X_INPUT_GAMEPAD_*` state into ImGui gamepad keys and reads game paths from the `gamepaths` cvar. — [xenia-canary-uwp](https://github.com/SirMangler/xenia/tree/canary_experimental/xenia-canary-uwp)
- Performance (forum anecdote): on a Series X, Goldeneye XBLA ran at a full 60 fps with occasional shader-cache stutter, and later Canary builds were faster. — [NeoGAF thread](https://www.neogaf.com/threads/xbox-360-emulator-xenia-has-been-ported-to-uwp-series-s-x-version-available.1651529/page-2) (search summary)
- Related static-recompilation port: worleydl's UnleashedRecomp-uwp (branch `uwp-port`, last commit 2025-03-23) moves the 4 GB PPC guest base from `0x100000000` (left commented out) to `0x80000000`. It uses `VirtualAlloc(MEM_COMMIT | MEM_RESERVE)` and falls back to an arbitrary address. — [UnleashedRecomp-uwp memory.cpp](https://github.com/worleydl/UnleashedRecomp-uwp/blob/uwp-port/UnleashedRecomp/kernel/memory.cpp)

### Inferences
- Xenia is the strongest evidence that a 4 GB+ guest space with multiple aliased views (RPCS3 also keeps `g_base_addr`, `g_sudo_addr` and an exec area as shm mirrors) can be mapped on Xbox Dev Mode. Xenia's `MapFileView` FromApp implementation can be reused nearly verbatim if RPCS3's `utils::memory_map_fd`/`shm::map` need an AppContainer-safe variant.
- UnleashedRecomp's base-address change suggests some fixed high addresses (e.g. `0x100000000`) may be unavailable inside an Xbox UWP process. RPCS3 should keep its "try a hint, else any address" logic.

### Gaps
- I could not confirm which Windows API partition the Xenia UWP core libraries are actually compiled with (premake defines nothing explicit), so whether the FromApp or desktop branch runs on Xbox is unverified.
- No systematic Xenia UWP vs PC performance data was found.

---

## 5. Other relevant ports: PPSSPP, RetroArch, DuckStation, Azahar, Flycast, Mesa/Dozen, and Switch/Wii U/PS4/Vita attempts

### Takeaway
PPSSPP (official) is the conservative reference. It is strictly D3D11, uses a W^X JIT through `*FromApp` APIs, and gives up on view aliasing; its author documented in Sep 2026 that placeholder reservations failed in his AppContainer. DuckStation had an official UWP port from Jul 2021 to Oct 2022 and solved fault handling without a vectored exception handler (VEH). RetroArch UWP and Azahar UWP use Mesa's GLon12 (and ANGLE). The community Mesa-UWP fork now builds **Dozen (Vulkan on D3D12) with a CoreWindow WSI**, and RetroArch ships `vulkan_dzn.dll`, but I found **no emulator that renders through Dozen on Xbox**. No Cemu, yuzu, Ryujinx, Vita3K or shadPS4 Xbox build exists.

### Cited Findings
**PPSSPP (official UWP; v1.20.4 released 2026-05-16)**
- The repo has a full `UWP/` project (App.cpp, PPSSPP_UWPMain.cpp, XAudioSoundStream, UWPHelpers). It detects Xbox via `DeviceFamily() == L"Windows.Xbox"`. — [ppsspp/UWP](https://github.com/hrydgard/ppsspp/tree/master/UWP); [release_data.json](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/release_data.json)
- Manifest capabilities: `codeGeneration`, `removableStorage`, `runFullTrust`, `broadFileSystemAccess`, `expandedResources` and networking. — [UWP/Package.appxmanifest](https://github.com/hrydgard/ppsspp/blob/master/UWP/Package.appxmanifest)
- The GPU backend is forced to D3D11 on UWP ("Enforce D3D11."). — [Core/Config.cpp](https://github.com/hrydgard/ppsspp/blob/master/Core/Config.cpp)
- JIT: `PlatformIsWXExclusive()` returns true for UWP, and code memory uses `VirtualAllocFromApp` and `VirtualProtectFromApp`. — [Common/MemoryUtil.cpp](https://github.com/hrydgard/ppsspp/blob/master/Common/MemoryUtil.cpp)
- Memory: the UWP `MemArena::CreateView` commits with `VirtualAllocFromApp` and ignores offsets, so views do not alias; only the VRAM mirrors are lost. A comment added 2026-09-02 reads: "Don't try to fix this with placeholders: CreateFileMappingFromApp + VirtualAlloc2FromApp (MEM_RESERVE_PLACEHOLDER) + MapViewOfFile3FromApp compiles … but the very first reservation fails at runtime with ERROR_INVALID_ADDRESS (487) - … the app container just won't hand out placeholders. Tried and reverted." — [Common/MemArenaWin32.cpp](https://github.com/hrydgard/ppsspp/blob/master/Common/MemArenaWin32.cpp); [commit history](https://github.com/hrydgard/ppsspp/commits/master/Common/MemArenaWin32.cpp)
  - This conflicts with XBSX2, Dolphin and Xenia, which use placeholder reservations on Xbox (sections 2–4). The PPSSPP comment does not say whether it was tested on an Xbox or on desktop Windows in AppContainer.

**DuckStation (official UWP port, Jul 2021 – Oct 2022)**
- "Add UWP frontend" landed 2021-07-04 and was merged in PR #2359 on 2021-07-11. "UWP: Declare expandedResources in manifest" followed on 2021-07-16. — [commit 9affb03](https://github.com/stenzek/duckstation/commit/9affb03); [commit 3265acb](https://github.com/stenzek/duckstation/commit/3265acb)
- Removal: "CI: … disable UWP (for now)" on 2022-07-21, "CI: Remove UWP completely" on 2022-10-06, and "Misc: Remove last remnants of UWP" on 2022-11-03. — [commit 0590a60](https://github.com/stenzek/duckstation/commit/0590a60); [commit 5dcf242](https://github.com/stenzek/duckstation/commit/5dcf242); [commit 97ff2bf](https://github.com/stenzek/duckstation/commit/97ff2bf)
- JIT buffer on UWP: `VirtualAlloc2FromApp(..., MEM_COMMIT, PAGE_READWRITE)`, then `VirtualProtectFromApp(..., PAGE_EXECUTE_READWRITE)`. The memory arena uses `CreateFileMappingFromApp`/`MapViewOfFileFromApp`, with the comment "UWP does not support fixed mappings" (fixed views return nullptr, i.e. no fastmem). File I/O moved to `CreateFile2FromAppW`, `FindFirstFileExFromAppW`, `CreateDirectoryFromAppW` and similar. — [commit 03ab189](https://github.com/stenzek/duckstation/commit/03ab189)
- Fault handling without a VEH: "Common/PageFaultHandler: Support for unwind exceptions … Needed for UWP." It registers an `UNWIND_INFO` exception handler over the JIT region via `RtlInstallFunctionTableCallback`, and the code cache reserves handler space inside the JIT buffer. — [commit 0f911e9](https://github.com/stenzek/duckstation/commit/0f911e9); [commit ca747d8](https://github.com/stenzek/duckstation/commit/ca747d8)
- The README described the DuckStation fullscreen (ImGui) UI as available for UWP/Xbox One. — [README commit 309169d](https://repo.retrodeck.net/Xargon/Duckstation/commit/309169d14c47af689ef93170c85698f3e10c01ad)

**RetroArch UWP (XboxEmulationHub fork; last commit 2026-08-31; maintainer TRW / TheRhysWyrill)**
- UWP sources: `frontend/drivers/platform_uwp.c`, `gfx/drivers_context/uwp_egl_ctx.c`, `input/drivers/uwp_input.c`, `libretro-common/vfs/vfs_implementation_uwp.cpp`. — [XboxEmulationHub/RetroArch](https://github.com/XboxEmulationHub/RetroArch)
- `pkg/msvc-uwp` ships prebuilt ANGLE (from SternXD/angle) and Mesa (from SternXD/mesa-uwp, `uwp-25.3.3`). The Mesa set includes `libgallium_wgl.dll`, `opengl32.dll`, `libEGL.dll`, `libGLESv2.dll`, `dxil.dll`, `spirv_to_dxil.dll` and **`vulkan_dzn.dll`**. — [MESA README](https://github.com/XboxEmulationHub/RetroArch/blob/master/pkg/msvc-uwp/RetroArch-msvcUWP/MESA/README.md); [ANGLE README](https://github.com/XboxEmulationHub/RetroArch/blob/master/pkg/msvc-uwp/RetroArch-msvcUWP/ANGLE/README.md)
- The vcxproj defines `HAVE_D3D11`, `HAVE_D3D12`, `HAVE_OPENGL(ES)`, `HAVE_ANGLE`, `HAVE_EGL` and `HAVE_DYNAMIC`, but **not `HAVE_VULKAN`**. Its manifest declares `runFullTrust`, `broadFileSystemAccess` and `expandedResources`, but not `codeGeneration`. — [RetroArch-msvcUWP.vcxproj](https://github.com/XboxEmulationHub/RetroArch/blob/master/pkg/msvc-uwp/RetroArch-msvcUWP/RetroArch-msvcUWP.vcxproj); [Package.appxmanifest](https://github.com/XboxEmulationHub/RetroArch/blob/master/pkg/msvc-uwp/RetroArch-msvcUWP/Package.appxmanifest)
- A "Retroarch 3DS" build with Citra, Azahar and Panda3DS cores is credited to Aerisarn (porter) and DanP. — [emulators.yml](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/emulators.yml)

**Mesa-UWP and Dozen**
- History: Aerisarn's Mesa UWP port ran Gallium (GLon12) on Xbox by March 2024. Separately, upstream Mesa merged initial Xbox **GDK** support (GLon12) for official devkits. — [Phoronix: UWP'ed Mesa on Xbox](https://www.phoronix.com/news/UWP-Mesa-Xbox-Port); [Phoronix: Mesa adds Xbox GDK](https://www.phoronix.com/news/Mesa-Adds-Xbox-GDK) (search summaries)
- Current fork: XboxEmulationHub/mesa-uwp, branch `uwp-26.2.2`, last commit 2026-09-03 ("UWP: Add support for UWP and Xbox compatibility").
  - Defaults: `gallium-drivers = ['d3d12']` and **`vulkan-drivers = ['microsoft-experimental']`** (Dozen). — [meson.options](https://github.com/XboxEmulationHub/mesa-uwp/blob/uwp-26.2.2/meson.options)
  - UWP-specific files include `d3d12_dxgi_xbox_screen.cpp`, `d3d12_wgl_framebuffer_uwp.cpp`/`_xbox.cpp` and `winsys/uwp/gdi_uwp.cpp`. — [XboxEmulationHub/mesa-uwp](https://github.com/XboxEmulationHub/mesa-uwp/tree/uwp-26.2.2)
  - Vulkan WSI: `wsi_common_win32.cpp` has `_XBOX_UWP` paths calling `CreateSwapChainForCoreWindow(queue, uwp_get_window_reference(), …)`. — [wsi_common_win32.cpp](https://github.com/XboxEmulationHub/mesa-uwp/blob/uwp-26.2.2/src/vulkan/wsi/wsi_common_win32.cpp)
  - Build: `meson setup … --uwp -Dc_args=-D_XBOX_UWP …` using a custom `meson-uwp` executable. — [Dev Mode Wiki: compiling Mesa](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/development/compiling-mesa-guide.mdx)
- Dozen quality (desktop): 99.75%+ Vulkan 1.0 conformance, later exposing 1.1 and 1.2. — [Phoronix Dzn 99.75%](https://Phoronix.com/news/Micorosft-Dzn-99p-Vulkan) (search summary)
- Azahar UWP (danprice142/Azahar-UWP, branch `UWP`, "Add UWP support" on 2026-10-05; release 0.2126.1.1) renders through **Mesa GLon12**. It bundles only `libgallium_wgl.dll`/`opengl32.dll` from SternXD/mesa-uwp 26.1.3, plus three Azahar patches. One patch replaces Mesa's `std::_Exit(hr)` on a failed `CreatePipelineState` with skipping the draw ("Skip a draw whose PSO the device rejected instead of terminating"). — [BUILD_INFO.txt](https://github.com/danprice142/Azahar-UWP/blob/UWP/externals/uwp/mesa/BUILD_INFO.txt); [azahar-pso-failure.patch](https://github.com/danprice142/Azahar-UWP/blob/UWP/externals/uwp/mesa/azahar-pso-failure.patch); [release_data.json](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/release_data.json)
- Other GL-on-Mesa ports (Aerisarn): Sega Model 3 (Supermodel-uwp-gl), GZDoom and Raze. — [emulators.yml](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/emulators.yml)
- worleydl's porting notes recommend aerisarn's mesa-uwp and SDL-uwp-gl for GL projects. They add that DLLs load only from the package or LocalState, not from `E:\`. — [worleydl UWP porting notes](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/helpful-links/worleydls-uwp-porting-notes.mdx)

**Flycast**
- Upstream Flycast publishes an official `.appx` (v2.7, 2026-08-19; UWP builds since 2021-05-28). — [release_data.json](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/release_data.json); [flyinghead/flycast](https://github.com/flyinghead/flycast)

**Switch, Wii U, PS4 and Vita**
- I found no UWP builds of Cemu, yuzu, Ryujinx, Vita3K or shadPS4. Ryujinx topped an Xbox Emulation Hub Discord poll, but there is "no official build", and legal and technical hurdles were cited. — [Windows Report](https://windowsreport.com/xbox-eyes-nintendo-switch-emulation-ryujinx-reportedly-tops-discord-community-poll/); [WindowsForum](https://windowsforum.com/threads/ryujinx-on-xbox-emulation-via-uwp-dev-mode-faces-legal-hurdles.400099/) (search summaries)
- No Switch, Wii U, PS3, PS4 or Vita emulator appears in the 2026 store emulator or experimental lists. — [emulators.yml](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/emulators.yml); [experimental-apps.yml](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/_data/experimental-apps.yml)

### Inferences
- A Dozen route for RPCS3's Vulkan renderer exists on paper: the mesa-uwp builds `vulkan_dzn` with a CoreWindow WSI. But no Xbox emulator has shipped a Vulkan renderer through it, so RPCS3 would be the first heavy Vulkan workload on Dozen-on-Xbox. Desktop conformance is not proof of Xbox driver robustness; the Azahar PSO-failure patch shows Xbox D3D12 rejecting PSOs that desktop accepts.
- DuckStation's no-VEH technique (`RtlInstallFunctionTableCallback` plus an unwind handler on JIT regions) is a fallback in case RPCS3's `AddVectoredExceptionHandler`-based access-violation handling were blocked. Current ports (XBSX2, Dolphin, Xenia, xbox-swap) all show VEH working on Dev Mode, so it is probably unnecessary.
- PPSSPP's failed placeholder attempt versus XBSX2/Xenia/Dolphin success suggests that capabilities or build partition matter. Possibly the core must run with `runFullTrust` and desktop APIs, or the base-address hint may have been the issue. RPCS3 needs a placeholder smoke test as the first on-hardware milestone.

### Gaps
- Whether any RetroArch core actually uses the bundled `vulkan_dzn.dll` on Xbox is unknown; the frontend has no Vulkan driver compiled in.
- I found no published Dozen-on-Xbox performance data.
- I did not determine whether DuckStation's 2021 RWX `VirtualProtectFromApp` call succeeded on Xbox, or why UWP support was dropped (the commits give no rationale).

---

## 6. Cross-project comparison matrix (repo, last activity, graphics, JIT, UI, input, gotchas)

### Takeaway
Every successful heavy emulator port on Xbox shares one recipe: a C++/WinRT `CoreApplication`/`IFrameworkView` shell, D3D12 (or D3D11) through `CreateSwapChainForCoreWindow`, a JIT using desktop memory APIs plus the `codeGeneration` capability, a controller-first ImGui-style UI instead of Qt, XInput/WGI/GameInput (often via SDL), files in LocalState or on an NTFS USB drive at `E:\` with ALL APPLICATION PACKAGES permissions, the restricted capabilities `runFullTrust`, `broadFileSystemAccess` and `expandedResources`, and the user switching the app type to "Game".

### Cited Findings
| Project | Repo / last activity | Graphics on Xbox | JIT / memory | UI | Input | Notable gotchas | Sources |
|---|---|---|---|---|---|---|---|
| XBSX2 (PS2) | XboxEmulationHub/XBSX2, v2.0.9.1 on 2026-09-01 | D3D12/D3D11 only; 3-buffer CoreWindow swap chain | Desktop `VirtualAlloc2`/`MapViewOfFile3` placeholders; RWX code region; VEH | PCSX2 ImGui FullscreenUI (no Qt) | SDL3-uwp (WGI/GameInput hints), XInput (`xinputuap`), UWP keyboard | No Enhanced Barriers; feedback-barrier workaround; PrimitiveID toggle; Series S 4K swap chain crashes; AVX2 build for Series, SSE4 for One | [repo](https://github.com/XboxEmulationHub/XBSX2), [81cd055d](https://github.com/XboxEmulationHub/XBSX2/commit/81cd055d), [main.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/src/main.cpp) |
| Dolphin (GC/Wii) | XboxEmulationHub/dolphin & SternXD/dolphin; release 1.1.9.0; master 2025-11-05 | D3D12 default (D3D11 also); GL/VK compiled out | `VirtualAlloc` RWX; `VirtualAlloc2FromApp`/`MapViewOfFile3FromApp` arena; VEH fastmem | Custom ImGui frontend | WGInput; US-English locale required | Logic ops off; shader compile threads capped at 2 | [repo](https://github.com/XboxEmulationHub/dolphin), [MemArenaWin.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/Common/MemArenaWin.cpp) |
| Xenia Canary UWP (X360) | SirMangler/xenia canary_experimental (2024-12-05); danprice142/xenia-canary-uwp (store build 2026-04-04) | Native D3D12 only | FromApp placeholders; 4 GB virtual + physical views; W^X-aware code cache; VEH + function-table callback | ImGui ("Blades style UI" in DanP fork) | XInput | Limited compatibility; shader-cache stutter | [SirMangler/xenia](https://github.com/SirMangler/xenia), [memory_win.cc](https://github.com/SirMangler/xenia/blob/canary_experimental/src/xenia/base/memory_win.cc) |
| PPSSPP (PSP) | hrydgard/ppsspp (official; v1.20.4 on 2026-05-16) | D3D11 enforced | W^X JIT via FromApp; no aliasing; placeholders "fail with 487" | Native PPSSPP UI | UWP input pane, XInput | VRAM mirrors dropped | [MemArenaWin32.cpp](https://github.com/hrydgard/ppsspp/blob/master/Common/MemArenaWin32.cpp) |
| DuckStation (PS1) | stenzek/duckstation; UWP 2021-07 to 2022-10, removed | D3D11/D3D12 (era) | `VirtualAlloc2FromApp` + `VirtualProtectFromApp` RWX; no fixed views; unwind-handler faults | ImGui FullscreenUI | XInput/UWP | UWP later dropped | [03ab189](https://github.com/stenzek/duckstation/commit/03ab189), [0f911e9](https://github.com/stenzek/duckstation/commit/0f911e9) |
| RetroArch UWP | XboxEmulationHub/RetroArch (2026-08-31) | D3D11/D3D12; GL via Mesa GLon12 or ANGLE; ships `vulkan_dzn.dll` | Per-core | RetroArch menus | `uwp_input.c` | No `codeGeneration` in manifest | [pkg/msvc-uwp](https://github.com/XboxEmulationHub/RetroArch/tree/master/pkg/msvc-uwp) |
| Azahar UWP (3DS) | danprice142/Azahar-UWP (2026-10-05) | OpenGL over D3D12 via Mesa 26.1.3 | (Citra JIT) | Azahar UWP shell | SDL2 (patched) | Mesa patched not to exit on PSO failure | [BUILD_INFO.txt](https://github.com/danprice142/Azahar-UWP/blob/UWP/externals/uwp/mesa/BUILD_INFO.txt) |
| ChonkyStation 3 UWP (PS3) | @MoonPower; no public source; tested 2025-03-23; delisted | unknown | unknown | None (loads first ELF) | Xbox pad, limited | Crashes; one ELF at a time | [guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/experimental/chonkystation3-guide.mdx) |
| rpcs3-uwp (PS3) | Edem180599, one commit on 2026-08-17 | Planned D3D12 stubs + Mesa Dozen | Planned `xbox-swap` | unknown | unknown | 6,272 recorded build errors | [repo](https://github.com/Edem180599/rpcs3-uwp) |

- Shared infrastructure available for reuse:
  - [XboxEmulationHub/SDL3-uwp](https://github.com/XboxEmulationHub/SDL3-uwp) (SDL3 with a WinRT video driver)
  - [mesa-uwp](https://github.com/XboxEmulationHub/mesa-uwp) (GLon12 + Dozen)
  - [angle](https://github.com/XboxEmulationHub/angle)
  - [dawn-uwp](https://github.com/XboxEmulationHub/dawn-uwp)
  - [libuwp](https://github.com/worleydl/libuwp) (helpers such as file pickers and cached refresh rate; last commit 2025-12-13)
  - [xbox-swap](https://github.com/XboxEmulationHub/xbox-swap) (allocations above 5,120 MB)
  - [uwp-dep](https://github.com/worleydl/uwp-dep)
  - The org list: [XboxEmulationHub repositories](https://github.com/orgs/XboxEmulationHub/repositories)

### Inferences
- For RPCS3 the matrix points to this recipe:
  1. Build RPCS3's core (Emu, Utilities, LLVM, asmjit) as a desktop-partition static lib.
  2. Add a thin C++/WinRT shell modeled on `pcsx2-uwp/src/main.cpp` or `DolphinWinRT/main.cpp`.
  3. Use RPCS3's RSX-overlay Big Picture Mode as the UI.
  4. Use SDL3-uwp or XInput for pads.
  5. Use XAudio2 for audio, mirroring XBSX2's `UWPSound.cpp` (RPCS3 already has an XAudio2 backend on Windows).
  6. Choose a renderer: Vulkan via mesa-uwp Dozen, or a new native D3D12 backend.

### Gaps
- Exact input APIs for Xenia beyond XInput and ImGui, and Azahar's full input stack, were not exhaustively reviewed.

---

## 7. Performance of demanding emulators on Series X|S Dev Mode, and PC comparisons

### Takeaway
Reported results come mostly from news and YouTube, not benchmarks. PS2 (XBSX2) and GameCube/Wii (Dolphin) run well on Series X|S at upscaled resolutions. Xbox 360 (Xenia) runs a limited set of titles. No PS3 emulator has meaningful Xbox performance data, and I found no rigorous PC-equivalence benchmark.

### Cited Findings
- XBSX2: Modern Vintage Gamer reported 4K or higher internal resolution at 60 fps on Series X, with Series S recommended at about 1440p. The 1440p advice matches XBSX2's code, which caps the Series S swap chain at 1440p. — [eXputer](https://exputer.com/news/industry/xbsx-ps2-emulator-games-4k-xbox/) (search summary); [main.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2-uwp/src/main.cpp)
- GamePro.de claims PS2 classics run better on current Xbox consoles than on PS4/PS5. — [GamePro.de](https://www.gamepro.de/artikel/ps2-emulation-xbox-series-x-s,3386007.html) (headline only)
- The Dev Mode Wiki says Xbox One "struggles with most 3D PS2 games", while Series X/S supports "High-res rendering, HD textures, 60fps patches". — [XBSX2 guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/xbsx2-guide.mdx)
- Dolphin: up to 1440p on Series S, with very good GameCube performance at 1440p with HD textures (The Verge, relayed). Metroid Prime runs at about 20–25 fps on Xbox One. — [Dexerto](https://www.dexerto.com/tech/gamecube-emulator-dolphin-has-been-ported-to-xbox-2041763) (search summary)
- Xenia: Goldeneye XBLA at 60 fps on Series X (forum), with limited compatibility at launch. — [NeoGAF](https://www.neogaf.com/threads/xbox-360-emulator-xenia-has-been-ported-to-uwp-series-s-x-version-available.1651529/page-2); [Windows Central](https://windowscentral.com/gaming/xbox/xenia-xbox-series-xors-grabs-a-potentially-more-powerful-xbox-360-emulator) (search summaries)
- App type matters: guides consistently say "App" mode restricts CPU, GPU and RAM, and that "Game" type is required for performance. — [XBSX2 guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/xbsx2-guide.mdx); [RetroArch guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/retroarch-guide.mdx)
- Memory ceiling as seen by porters: xbox-swap exists "for allocations over 5120 MB on Xbox One and Series X|S". It implements user-space demand paging, with a VEH that maps 4 KB views of a LocalState swap file into a placeholder range, up to 512 MB resident, using LRU eviction. — [xbox-swap README](https://github.com/XboxEmulationHub/xbox-swap); [fmalloc.cpp](https://github.com/XboxEmulationHub/xbox-swap/blob/master/src/fmalloc.cpp)
- RPCS3 contributors cite RAM as a blocker: FlexBy420 said there is "not enough RAM in dev mode", and a non-member claimed about 4 GB. — [RPCS3 issue #15787](https://github.com/RPCS3/rpcs3/issues/15787)

### Inferences
- A PS3 port will be bounded by CPU (PPU and SPU LLVM recompilation and execution on the console's Zen 2 cores) and by the roughly 5 GB per-title memory budget implied by xbox-swap's design. That budget would hold PS3's 256 MB XDR plus 256 MB VRAM emulation, but leaves little room for RPCS3's PPU/SPU LLVM caches and the shader and texture caches typical on PC.
- Shader and PSO compilation stutter recurs in Dolphin (thread caps) and Xenia (cache stutter). It will likely be worse for RPCS3 and should be planned around with precompiled shader caches.

### Gaps
- I found no apples-to-apples Series X vs PC benchmarks for any emulator. The official per-process memory limits for Dev Mode Game-type apps were not verified from Microsoft documentation in this pass; learn.microsoft.com was not reachable from this environment.

---

## 8. JIT, memory, file and packaging rules across ports, mapped to the problems an RPCS3 port faces

### Takeaway
Microsoft's documentation says the `*FromApp` APIs reject `PAGE_EXECUTE_READWRITE`. Yet every heavy Xbox emulator port calls the desktop `VirtualAlloc`, `VirtualProtect` and `MapViewOfFile3` APIs with RWX and placeholders, from cores compiled as desktop libraries and packaged with `codeGeneration` plus `runFullTrust`. These are the practical Dev Mode rules to replicate, but they conflict with the documentation and with PPSSPP's failure report, so they must be validated on hardware first.

### Cited Findings
- Microsoft docs:
  - For `VirtualProtectFromApp`, "The following constants generate an error: PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY". `PAGE_EXECUTE` and `PAGE_EXECUTE_READ` are "allowed only for apps that have the codeGeneration capability". — [MicrosoftDocs: VirtualProtectFromApp](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-virtualprotectfromapp.md)
  - `VirtualAllocFromApp` and `VirtualAlloc2FromApp` reject all `PAGE_EXECUTE*` constants at allocation time. — [MicrosoftDocs: VirtualAllocFromApp](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-virtualallocfromapp.md); [VirtualAlloc2FromApp](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-virtualalloc2fromapp.md)
- In practice:
  - XBSX2 maps RWX code memory with desktop `VirtualAlloc2`/`MapViewOfFile3`. — [Memory.cpp](https://github.com/XboxEmulationHub/XBSX2/blob/master/pcsx2/Memory.cpp)
  - Dolphin allocates RWX with `VirtualAlloc`. — [MemoryUtil.cpp](https://github.com/XboxEmulationHub/dolphin/blob/master/Source/Core/Common/MemoryUtil.cpp)
  - DuckStation (2021) called `VirtualProtectFromApp(PAGE_EXECUTE_READWRITE)`. — [03ab189](https://github.com/stenzek/duckstation/commit/03ab189)
  - Xenia avoids W+X in FromApp mode. — [memory_win.cc](https://github.com/SirMangler/xenia/blob/canary_experimental/src/xenia/base/memory_win.cc)
  - PPSSPP uses strict W^X on UWP. — [MemoryUtil.cpp](https://github.com/hrydgard/ppsspp/blob/master/Common/MemoryUtil.cpp)
- Header partitioning: xbox-swap hand-declares the `AddVectoredExceptionHandler`/`RemoveVectoredExceptionHandler` prototypes under `_XBOX_UWP`, because the app-partition headers hide them, and then uses them at runtime. — [fmalloc.cpp](https://github.com/XboxEmulationHub/xbox-swap/blob/master/src/fmalloc.cpp)
- Build and packaging rules from worleydl's notes:
  - Use `/MD`; components must depend on `VCRUNTIME140_APP.dll` (check with `dumpbin /DEPENDENTS`).
  - Configure CMake dependencies with `-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0`.
  - Turn the main executable into a static lib linked by a new UWP project; add `onecore.lib`/`windowsapp.lib` for unresolved symbols.
  - Use `CreateSwapChainForCoreWindow`; get LocalState via `ApplicationData::Current().LocalFolder().Path()`; USB is `E:\`.
  - DLLs load only from the package or LocalState.
  - Source: [worleydl UWP porting notes](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/helpful-links/worleydls-uwp-porting-notes.mdx)
- File access in practice: NTFS USB with "ALL APPLICATION PACKAGES → Full Control", otherwise loads fail (RetroArch FAQ: "almost always a USB permissions issue"). Internal LocalState is reachable through Device Portal → File Explorer. — [RetroArch guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/retroarch-guide.mdx); [PPSSPP guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/emulation/ppsspp-guide.mdx)
- Packaging and deployment: MSIX/appxbundle sideloaded via Device Portal "Add", with dependency `.appx` files (VCLibs and similar), or deployed from Visual Studio as "Remote Machine" using the Dev Home PIN. The Dev Mode setup guide says to allocate at least 5 GB to Dev storage. — [Compiling XBSX2](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/development/compiling-xbsx2-guide.mdx); [Dev Mode setup](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/xbox-setup/xbox-developer-mode-setup.mdx)

### Inferences
- RPCS3 mapping:
  - LLVM's JIT memory manager and asmjit `JitRuntime` allocate RWX through `VirtualAlloc`/`VirtualProtect` on Windows. Per the XBSX2 and Dolphin precedent, keep them on the desktop APIs. As a fallback, implement a dual-mapping W^X allocator like Xenia's code cache.
  - RPCS3's `utils::shm` and `memory_reserve` should map onto the placeholder pattern XBSX2 uses.
  - VEH stays as it is.
  - File I/O: RPCS3's `fs::` uses `CreateFileW` and related calls. The desktop-partition core may keep them for LocalState and `E:\` paths. If the AppContainer blocks them, swap in the `*FromAppW` variants as DuckStation did.
- Third-party libraries to drop for Xbox, per the Edem180599 audit and the PCSX2/Dolphin precedent: Qt, libusb, hidapi, miniupnpc, rtmidi, cubeb/OpenAL (replace with XAudio2), discord-rpc, and OpenGL.

### Gaps
- I found no first-party Microsoft statement on whether Dev Mode apps with `runFullTrust` are exempt from the FromApp W^X rule; the evidence is behavioral, from shipped ports. PPSSPP's placeholder failure versus the others' success is unresolved.

---

## 9. Community and legal context: Microsoft's stance on emulators in Retail vs Dev Mode, and risk to dev accounts

### Takeaway
Emulators are banned from Retail mode. Store policy 10.13.10 led Microsoft to remove emulators and then block sideloaded "retail mode" emulators, with reported suspensions in 2023–2024. Dev Mode remains the sanctioned path for running your own UWP code; Microsoft publicly said in 2022 it has no plans to remove it. A January 2022 wave of dev-account deactivations hit emulation users, was called accidental, and was reversed.

### Cited Findings
- Microsoft Store policy 10.13.10: "Products that emulate a game system or game platform are not allowed on any device family." Emulator apps such as RetroArch and Dolphin were pulled from the Store, and later emulator launches in retail mode were blocked with an error. — [Neowin](https://www.neowin.net/news/microsoft-just-shut-down-a-lot-of-game-emulators-on-xbox-consoles-in-retail-mode/); [wccftech](https://wccftech.com/microsoft-cracks-down-on-xbox-series-emulation-in-retail-mode-causing-uproar-from-preservationists/amp/) (search summaries)
- Users of the "UWeaPons" retail workaround reportedly received 15-day suspensions, and some reported permanent bans. — [Tom's Hardware](https://www.tomshardware.com/news/microsoft-banning-people-using-emulators-xbox); [ResetEra](https://www.resetera.com/threads/retail-mode-emulation-on-xbox-is-now-dead-xbox-now-banning-emulator-uploaders-suspending-users-for-15-days.747001/); [GBAtemp](https://gbatemp.net/threads/microsoft-is-now-suspending-users-who-use-retail-mode-emulators-on-their-xbox-consoles.637104/) (search summaries; forum-level evidence)
- January 2022: Microsoft deactivated Dev Mode accounts used mainly for emulation. Jason Ronald called it accidental, said Microsoft has "no plans to remove or disable Developer Mode", and the accounts were reinstated. — [Pure Xbox](https://www.purexbox.com/news/2022/01/xbox-dev-mode-accounts-were-disabled-by-mistake-suggests-microsoft); [XDA](https://www.xda-developers.com/xbox-disables-dev-mode-accounts-emulation/); [TechRadar](https://www.techradar.com/news/xbox-dev-mode-isnt-going-anywhere-despite-emulation-concerns) (search summaries)
- Account cost in 2026: the community setup guide says individual developer accounts are now free (identity verification required) and company accounts pay a one-time fee. Older coverage cites a ~$19–20 fee. — [Dev Mode setup guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/xbox-setup/xbox-developer-mode-setup.mdx); [mydealz](https://www.mydealz.de/deals/xbox-als-emulator-verwenden-fur-atari-gameboy-nes-snes-gamecube-wii-wiiu-psp-ps2-ps3-xbox-360-uber-den-dev-mode-2620548) (search summary)
- Community norms: projects stress BIOS and firmware from owned consoles, and the Dev Mode Wiki notes legal implications for decryption keys. The rpcs3-PS5 port forbids redistributing builds over licence conflicts (RPCS3 GPL-2.0-only vs GPL-3.0 dependencies). — [XBSX2 README](https://github.com/XboxEmulationHub/XBSX2); [ChonkyStation guide](https://github.com/XboxEmulationHub/devmodewiki/blob/master/docs/experimental/chonkystation3-guide.mdx); [rpcs3-PS5 README](https://github.com/lavavex/rpcs3-PS5)

### Inferences
- An RPCS3 Xbox port must be distributed as a Dev Mode sideload, never through the Store. Dev-account risk appears low but not zero, given the 2022 incident. Licensing matters for redistribution: RPCS3 is GPL-2.0-only, so GPL-3.0-only components such as some mesa-uwp or SDL forks need a licence check before shipping binaries. Mesa is mostly MIT; this needs verification.

### Gaps
- I found no 2025–2026 Microsoft statement specifically on Dev Mode emulation, and no reports of Dev Mode bans after 2022. The primary Microsoft Q&A page on retail emulation was not reachable from this environment.
