# XBXPS3

A port of [RPCS3](https://github.com/RPCS3/rpcs3), the PlayStation 3 emulator, to **Xbox Series X|S in Developer Mode**. It runs as a UWP game with a controller-first, TV-friendly interface.

> **Status: first build, never compiled for or run on an Xbox yet.**
> - The RPCS3 patch series applies cleanly to the pinned commit.
> - The core sources and the patched RPCS3 files pass a syntax check against the real RPCS3 headers (Windows SDK headers via MinGW).
> - Nothing has been built with MSVC, linked, packaged or launched on a console. Expect a round of compile and link fixes on the first Windows build (see [docs/BUILDING.md](docs/BUILDING.md#link-triage)).
> - The first launch on hardware writes `xbxps3_probe.txt`. It answers the open questions that decide whether this approach works (see [docs/ROADMAP.md](docs/ROADMAP.md)).

## How it works

```
 Xbox (Dev Mode, app type "Game")
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │ XBXPS3.exe  (UWP, CoreWindow)                                                │
 │                                                                              │
 │  app/  C++/WinRT shell                     core/  xbxps3_core.lib (desktop C++)│
 │  ─ window + composition visual   ◄──C ABI──► ─ RPCS3 rpcs3_emu (patched)      │
 │  ─ suspend/resume, Back button      xbx_core.h ─ emu callbacks, GS frame       │
 │  ─ View+Menu = PS button                     ─ firmware/PKG/RAP importer     │
 │  ─ TV mode, Game-mode check                  ─ Xbox defaults, probe          │
 │  ─ loads vulkan_dzn.dll ─┐                   ─ XInput (xinputuap), XAudio2   │
 │                          │                                                   │
 │  Mesa Dozen (Vulkan → D3D12, UWP fork) ◄── volk ◄── RPCS3 Vulkan renderer    │
 │                          │                                                   │
 │                    Direct3D 12 ─► GPU                                         │
 └─────────────────────────────────────────────────────────────────────────────┘
```

- **Console UI = RPCS3's own Big Picture Mode.** RPCS3 now ships a full-screen, gamepad-driven game launcher, home menu, dialogs and on-screen keyboard, all drawn by the emulator's renderer (RSX overlays) and all Qt-free. XBXPS3 boots straight into it, so the whole app is navigable with an Xbox controller from the couch. See [docs/CONSOLE_UI.md](docs/CONSOLE_UI.md).
- **Graphics:** RPCS3 renders with Vulkan, and Xbox has only Direct3D. [Mesa-UWP](https://github.com/rodrigoandrigo/Mesa-UWP)'s Dozen driver translates Vulkan to D3D12 inside the app. RPCS3 calls Vulkan through [volk](https://github.com/zeux/volk), since there is no system Vulkan loader.
- **CPU/JIT:** the RPCS3 core stays desktop C++ with its LLVM and ASMJIT recompilers. It is linked into the UWP app the way the shipped [XBSX2](https://github.com/XboxEmulationHub/XBSX2) (PCSX2) port does it, with the `codeGeneration` and `runFullTrust` capabilities.
- **Only 6 small patches touch RPCS3** ([patches/rpcs3](patches/rpcs3)). Everything else lives in this repo, so rebasing onto newer RPCS3 stays cheap.

## Quick start

On a Windows 11 PC with Visual Studio 2022 (C++ desktop and UWP workloads), CMake, Ninja, Python 3 (`pip install meson mako`), Git and PowerShell 7:

```powershell
./scripts/bootstrap.ps1          # RPCS3 @ 2ba2f75 + patches, volk, Mesa-UWP, fonts
./scripts/build.ps1              # core (LLVM build takes ~1 h), Dozen, signed MSIX
./scripts/deploy.ps1 -Console <xbox-ip>
```

Then in Dev Home on the Xbox, set **XBXPS3 → View details → App type → Game**. Copy your PS3 firmware (`PS3UPDAT.PUP`) into the Install folder and launch the app ([docs/DEPLOYING.md](docs/DEPLOYING.md)).

You need your own PS3 firmware and your own legally dumped games. This project contains no Sony code or keys beyond what RPCS3 itself includes.

## Controls

| Xbox | PS3 | |
|---|---|---|
| A / B / X / Y | Cross / Circle / Square / Triangle | B is captured, so it never backs out of the app |
| LB / RB, LT / RT | L1 / R1, L2 / R2 | |
| L-stick / R-stick click | L3 / R3 | |
| View / Menu | Select / Start | |
| **View + Menu together** | **PS button** | Opens the RPCS3 home menu (save states, settings, quit game) |
| Xbox button | (console) | Leaves the app; emulation is frozen while suspended |

Controllers 1–4 map to PS3 players 1–4 through XInput.

## Repository layout

| Path | What |
|---|---|
| `patches/rpcs3/` | The 6-patch series applied to RPCS3 (CMake mode `RPCS3_UWP`, paths, volk/Dozen, XAudio2, XInput, hook) |
| `core/` | Qt-free RPCS3 frontend compiled as desktop C++ and merged into `xbxps3_core.lib` |
| `app/` | UWP shell, manifest, assets; CMake with the Visual Studio WindowsStore generator |
| `scripts/` | bootstrap, build (core / Mesa / app), import audit, Device Portal deploy |
| `docs/` | building, deploying, console UI, roadmap |
| `research_notes/` | The research behind every design decision (platform limits, prior art, RPCS3 internals, graphics) |

## Credits

RPCS3 team (GPL-2.0). Mesa and Dozen, including the UWP work by rodrigoandrigo and aerisarn. volk by Arseny Kapoulkine. The XBSX2, Dolphin, PPSSPP and RetroArch Xbox ports, whose public code showed what works in Dev Mode.
