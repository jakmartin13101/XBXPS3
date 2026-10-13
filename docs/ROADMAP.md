# Roadmap

The research behind this plan is in [`research_notes/`](../research_notes/RPCS3%20Xbox%20Series%20X%20UWP%20port/). Each phase ends in a check that can only be done on a real console.

## Phase 0: First build and hardware probe (next)

1. Build all three stages on Windows and work through [link triage](BUILDING.md#link-triage) until `check-imports.ps1` passes.
2. Deploy, set Game mode, launch, and read `LocalState/xbxps3_probe.txt`.

The probe answers the four questions that decide the architecture:

| Probe line | Good result | If it fails |
|---|---|---|
| 64 GiB placeholder reserve, aliased views | OK, mirror coherent | Port `util/vm_native.cpp` to `VirtualAlloc2FromApp` / `MapViewOfFile3FromApp`. If the address space itself is capped, shrink RPCS3's reservations (sudo mirror, 32 GiB hook region). |
| RWX `VirtualAlloc`, call returns 42 | OK | Make RPCS3's JIT W^X: dual-mapped RW/RX views for the LLVM memory managers (`Utilities/JITLLVM.cpp`) and the asmjit runtime (`Utilities/JITASM.cpp`), like Xenia's UWP code cache. |
| Highest shader model ≥ 6.6 | Dozen gets descriptor indexing | RPCS3's Vulkan backend requires `runtimeDescriptorArray`. Either patch Dozen/RPCS3 to work without it, or start Phase 3 (native D3D12) sooner. Chuck Walbourn's 2022 write-up lists shader models up to 6.4 for UWP on Xbox, while the Mesa-UWP fork targets 6.6+ features, so **this is the biggest open risk.** |
| App memory limit ≈ 5 GiB | Game mode | Ask the user to set Game mode. The app already warns about this. |

## Phase 1: Launcher on screen

- Big Picture Mode renders through Dozen on the TV.
- Controllers navigate it.
- Firmware install from the Install folder works.
- Disc folders and ISOs show up in the grid.
- Fonts come from the bundled DejaVu-as-Arial until firmware fonts exist.

## Phase 2: First games

- PPU/SPU LLVM recompilers work.
- Shader cache and LLVM cache write to LocalState or USB.
- Audio plays through XAudio2.
- Suspend and resume keep emulation intact.
- Test 2D and simple 3D titles first.
- Expect many games to be limited by the ~5 GB budget, which is shared by CPU and GPU. RPCS3 desktop recommends 8 GB+.
- Tune the defaults in `core/src/xbx_settings.cpp`:
  - resolution scale 100%;
  - the shader interpreter / async compile mode;
  - "VRAM allocation limit" sized to the budget;
  - SPU block size.

## Phase 3: Performance and correctness

- Measure Dozen overhead. The research estimated up to ~2× slower than native D3D12 in one comparison.
- Profile the CPU side. Series X Zen 2 at 3.6–3.8 GHz with 4 exclusive and 2 shared cores for UWP games is below RPCS3's recommended spec. Pin RPCS3's PPU/SPU threads with `GetSystemCpuSetInformation`.
- If Dozen is the bottleneck or blocked (Phase 0 SM check), write a native D3D12 RSX backend. Reuse RPCS3's GLSL decompilers and compile GLSL → SPIR-V (glslang) → DXIL with Mesa's `spirv_to_dxil` and `dxil.dll` signing, as eden-xbox does. Use the Vulkan backend as the structural template. This is months of work.

## Phase 4: Console polish

- Xbox settings page and on-demand install menu (see [CONSOLE_UI.md](CONSOLE_UI.md#planned-ui-work)).
- Keyboard and mouse handlers via CoreWindow events.
- On-screen keyboard text entry for RPCN sign-in (CoreInputView).
- DualSense/DualShock 4 through `Windows.Gaming.Input.RawGameController`, with gyro.
- HDR output and 120 Hz modes via `HdmiDisplayInformation.RequestSetCurrentDisplayModeAsync`.

## Known constraints (from the research)

- RPCS3 maintainers have said they will not do a UWP port (issues #9315 and #15787). This is an independent downstream project, so do not file Xbox bugs upstream.
- Distribution is sideload-only in Dev Mode. The Store does not accept UWP games, and Retail-mode emulators have drawn account suspensions since about 2023.
- No public RPCS3-on-Xbox build was known to work as of October 2026. Prior art covers PS2, GameCube/Wii, Xbox 360 and Switch emulators, which XBXPS3 borrows from.
