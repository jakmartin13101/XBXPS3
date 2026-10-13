# Building XBXPS3

## Prerequisites (Windows 10/11 x64 PC)

- **Visual Studio 2022** (17.10+) with these workloads:
  - *Desktop development with C++*
  - *Universal Windows Platform development*, including *C++ (v143) Universal Windows Platform tools*
- **Windows SDK 10.0.22621** or newer. It provides C++/WinRT headers, `xinputuap.lib` and `dxil.dll`.
- **CMake 3.28+**, **Ninja**, **Git**, **PowerShell 7**
- **Vulkan SDK 1.4.341** (RPCS3's CMake looks for its headers; `vulkan-1.lib` is never linked into the app)
- **Python 3** with `pip install meson mako`, plus **winflexbison** on PATH (for Mesa)
- About 60 GB of free disk. LLVM and RPCS3 builds are large.

## Steps

```powershell
./scripts/bootstrap.ps1     # external/: RPCS3 2ba2f75 + patches/rpcs3, volk, Mesa-UWP, fonts
./scripts/build.ps1         # all stages; or -Stage core|mesa|app
```

| Stage | What | Output |
|---|---|---|
| `core` | RPCS3 as the top-level CMake project with `-DRPCS3_UWP=ON -DXBXPS3_FRONTEND_DIR=core`. Ninja, Release, `/MD`, `/arch:AVX2`, LLVM built from its submodule. | `build/core/xbxps3_core/xbxps3_core.lib` plus `xbxps3_core_system_libs.txt` |
| `mesa` | Dozen from `external/mesa-uwp` with `-Duwp=true` and `/D_XBOX_UWP` | `build/mesa/vulkan_dzn.dll` |
| `app` | `app/` with the Visual Studio generator, `CMAKE_SYSTEM_NAME=WindowsStore`. Packages and signs with a self-signed `CN=XBXPS3 Dev` certificate. | `build/app/AppPackages/XBXPS3_*/XBXPS3_*.msix` |

### Why two stages

The RPCS3 core and its ~28 third-party libraries are built as ordinary **desktop** static libraries. They keep using `VirtualAlloc2`, `MapViewOfFile3`, vectored exception handlers and RWX JIT pages, and are merged into a single `.lib`. Only the small shell in `app/` is a UWP project.

This mirrors the Xbox ports that actually ship:
- XBSX2 builds PCSX2's core as a desktop library and links it into its UWP app.
- worleydl's porting notes say "turn the main executable into a static lib linked by a new UWP project".

Rewriting RPCS3's memory and JIT layers against the `*FromApp` APIs is the fallback if the probe shows Dev Mode refusing desktop APIs (see [ROADMAP.md](ROADMAP.md)).

## Link triage

The first MSVC build will surface problems no Linux syntax check can catch. Typical ones:

| Error | Fix |
|---|---|
| `LNK2019 unresolved external __imp_Foo` where Foo is user32/gdi32/winmm/setupapi | Add a stand-in to `core/src/xbx_win32_shims.cpp`, or guard the call with `RPCS3_UWP` in a new patch |
| Unresolved symbol defined by the desktop frontend (`rpcs3.cpp`, `main_application.cpp`, `rpcs3qt/*`) | Define it in `core/src` (as done for `report_fatal_error` and `g_input_config_override`) or add the upstream file to `core/CMakeLists.txt` |
| `LNK2038 RuntimeLibrary mismatch` | A prebuilt dependency uses `/MT`, most likely RPCS3's prebuilt ffmpeg on Windows. Rebuild it with `/MD`, or with `--toolchain=msvc` and `-DWINAPI_FAMILY=WINAPI_FAMILY_APP` |
| `check-imports.ps1` fails after linking | Some object still imports a desktop DLL. `dumpbin /DEPENDENTS build\app\Release\XBXPS3.exe` names it; `link /VERBOSE` shows which object pulled it |

### Making a new RPCS3 patch

```powershell
cd external/rpcs3            # branch xbxps3, patches already applied as commits
# edit, then:
git commit -am "area: what and why"
git format-patch --no-signature 2ba2f75 -o ../../patches/rpcs3/
```

Keep patches small and guarded with `#ifdef RPCS3_UWP` so upstream behaviour does not change. To move to a newer RPCS3, update `$Rpcs3Commit` in `scripts/bootstrap.ps1`, then rebase the `xbxps3` branch and re-export the series.

## Iterating without a full rebuild

- Change only `core/src`: run `-Stage core`. Ninja rebuilds a few objects and re-merges the lib. Then run `-Stage app`.
- Change only `app/src`: run `-Stage app`.
- From Visual Studio: open `build/app/XBXPS3.sln`. Set the target to *Remote Machine* (the Xbox IP and PIN from Dev Home) and press F5. You get breakpoints in both the shell and the core.
