# Graphics strategy for RPCS3 on Xbox Series X|S Developer Mode (UWP, D3D11/D3D12 only): Dozen vs. D3D12 backend vs. GLOn12

Research date: 2026-10-12. Mesa state is from a GitHub mirror of upstream Mesa (`chaotic-cx/mesa-mirror`, version `26.3.0-devel`, HEAD dated 2026-09-17). gitlab.freedesktop.org could not be reached from this environment, so the mirror stands in for upstream. RPCS3 state is from `RPCS3/rpcs3` master as of 2026-10-11, inspected with git.

Evidence labels:
- **[VERIFIED-SRC]**: I read it in source code or official docs.
- **[COMMUNITY]**: a third-party port's own notes or README, usually measured on real consoles but not independently reproduced.
- **[SNIPPET]**: only a search-engine summary was reachable, because the host was blocked. Treat these with more caution.

---

## Q1. Option A: Mesa "Dozen" (dzn, Vulkan on D3D12). Status, feature gaps against RPCS3's needs, and whether it runs in UWP on Xbox

### Takeaway
Upstream Dozen as of 2026 has these limits:
- It advertises Vulkan 1.2 but is not conformant, and it does not fully implement 1.1, 1.2 or 1.3.
- It is in maintenance mode: about 3 commits a month, mostly tree-wide refactors.
- It needs a DXIL validator, and its descriptor indexing needs SM 6.6.
- It lacks several extensions RPCS3 uses. One RPCS3 currently treats as a hard requirement, `VK_EXT_shader_uniform_buffer_unsized_array`, was added in April 2026.

Upstream RPCS3 explicitly warns that Dozen is an "unsupported" driver. Upstream Dozen has no UWP/Xbox support: its WSI is HWND-only and it has no Xbox ifdefs.

Two out-of-tree UWP Mesa forks do exist:
- `aerisarn/mesa-uwp` (2024): a CoreWindow swapchain, a GDI stub "into vulkan", and an `_XBOX_UWP` define.
- `rodrigoandrigo/Mesa-UWP` (Mesa 26.2.2, commits 2026-10-08/09). This fork patches Dozen specifically for RPCS3. It adds custom border color, conditional rendering, provoking vertex, multi-draw, device fault, external memory host, stencil export and UBO unsized arrays, and a commit comment names RPCS3.

No public result (screenshots, FPS) of RPCS3 on Dozen on Xbox was found.

### Cited Findings
**Version, conformance and maintenance**
- `DZN_API_VERSION` is `VK_MAKE_VERSION(1, 2, VK_HEADER_VERSION)`, so Dozen reports Vulkan 1.2. `conformanceVersion` is `0.0.0.0` (not conformant). `driverID = VK_DRIVER_ID_MESA_DOZEN`. — [dzn_device.c (Mesa mirror)](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_device.c) [VERIFIED-SRC]
- Mesa `features.txt` lists dzn under "Vulkan 1.0 -- all DONE" only. dzn is absent from the "all DONE" lists for Vulkan 1.1, 1.2, 1.3 and 1.4. — [features.txt (Mesa mirror)](https://github.com/chaotic-cx/mesa-mirror/blob/main/docs/features.txt) [VERIFIED-SRC]
  - Missing 1.1 items: external_fence, sampler_ycbcr_conversion, variable_pointers.
  - Missing 1.2 items: shader_atomic_int64, subgroup_extended_types, vulkan_memory_model, host_query_reset, sampler_filter_minmax, shader_viewport_index_layer.
  - Missing 1.3 items include maintenance4, extended_dynamic_state 1/2, inline_uniform_block, pipeline_creation_cache_control, demote_to_helper_invocation, 4444_formats and texel_buffer_alignment.
- In 2022, Phoronix reported Dozen passing over 99.75% of the Vulkan 1.0 CTS. — [Phoronix](https://Phoronix.com/news/Micorosft-Dzn-99p-Vulkan) [SNIPPET]
- Upstream dzn commits from Sep 2025 to Aug 2026: about 35 commits, roughly 3 a month.
  - Most are tree-wide refactors (SHA1 to blake3, drirc, BITSET, getenv).
  - The only feature additions were `VK_EXT_memory_budget` (Feb 2026), `VK_EXT_debug_marker` (Jun 2026), a loop-continue lowering (Mar 2026) and a driconf deviceName override.
  - Jesse Natalie (`jenatali`, Microsoft) appears for warning and include cleanups. — [mesa-mirror commits for src/microsoft/vulkan](https://github.com/chaotic-cx/mesa-mirror/commits/main/src/microsoft/vulkan) [VERIFIED-SRC]

**Upstream dzn: what it does expose**
- Extensions:
  - `KHR_swapchain`, `KHR_dynamic_rendering`, `KHR_synchronization2`, `KHR_sampler_mirror_clamp_to_edge`, `KHR_timeline_semaphore`, `KHR_draw_indirect_count`, `EXT_vertex_attribute_divisor`, `EXT_subgroup_size_control`.
  - `KHR_shader_float16_int8` and `KHR_16bit_storage`, both gated on `Native16BitShaderOpsSupported`.
  - `EXT_descriptor_indexing` and buffer device address, both gated on `shader_model >= 6_6`.
  - `EXT_external_memory_host`, gated on `ID3D12Device13`.
  - `KHR_8bit_storage`, only behind a drirc debug option. — [dzn_device.c](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_device.c) [VERIFIED-SRC]
- Core features:
  - `geometryShader = true`, `tessellationShader = false`, `dualSrcBlend = false`, `logicOp = false`, `largePoints = false`, `fullDrawIndexUint32 = false`.
  - `wideLines` only via a drirc "claim_wide_lines" hack.
  - `depthBounds` follows `DepthBoundsTestSupported`, and `shaderFloat64` follows `DoublePrecisionFloatShaderOps`.
  - `runtimeDescriptorArray` and `descriptorIndexing` are available only when SM >= 6.6. — [dzn_device.c](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_device.c) [VERIFIED-SRC]
- Depth formats: D24S8 maps to `DXGI_FORMAT_D24_UNORM_S8_UINT` / `R24G8_TYPELESS`, natively in D3D12. — [dzn_image.c](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_image.c) [VERIFIED-SRC]
- Barriers: Dozen supports both legacy and enhanced barriers. Enhanced barriers are used when `options12.EnhancedBarriersSupported`, so they are not mandatory. — [dzn_device.c](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_device.c), [dzn_cmd_buffer.c](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_cmd_buffer.c) [VERIFIED-SRC]
- DXIL validator: instance creation calls `dxil_create_validator()` and fails with `VK_ERROR_INITIALIZATION_FAILED` if the validator cannot load. The only exception is `DZN_DEBUG=experimental`, which instead creates the device with D3D12 experimental shader models so it can run unsigned DXIL. On desktop the validator is `DXIL.dll`. — [dzn_device.c](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_device.c), [dxil_validator.cpp](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/compiler/dxil_validator.cpp) [VERIFIED-SRC]

**Upstream dzn: what it is missing that RPCS3 asks for**
- Absent from dzn's lists in `features.txt`, so missing upstream:
  - `VK_EXT_custom_border_color`, `VK_EXT_conditional_rendering`, `VK_EXT_depth_range_unrestricted`, `VK_EXT_attachment_feedback_loop_layout`, `VK_EXT_shader_stencil_export`, `VK_EXT_provoking_vertex`, `VK_EXT_multi_draw`, `VK_EXT_device_fault`, `VK_KHR_fragment_shader_barycentric`, `VK_EXT_border_color_swizzle`.
  - `VK_EXT_transform_feedback`, `VK_EXT_extended_dynamic_state`, `VK_KHR_push_descriptor`, `VK_EXT_shader_uniform_buffer_unsized_array`. — [features.txt](https://github.com/chaotic-cx/mesa-mirror/blob/main/docs/features.txt) [VERIFIED-SRC]

**What RPCS3's Vulkan backend requires**
- Always requested:
  - `VK_KHR_swapchain`.
  - The Vulkan 1.2 features `runtimeDescriptorArray = VK_TRUE` and `uniformBufferStandardLayout = VK_TRUE`, set with no support check.
  - A `VkPhysicalDeviceShaderUniformBufferUnsizedArrayFeaturesEXT` struct with `shaderUniformBufferUnsizedArray = VK_TRUE`. The code comment reads: "FIXME: Fall back to something. Idk how that would even work though, this really is a hard requirement". — [RPCS3 device.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/vkutils/device.cpp) [VERIFIED-SRC]
- The UBO-unsized-array requirement arrived in kd-11's commit "vk: Use UBOs for critical per-fragment data" (5839ad9c7, 2026-04-27). — [RPCS3 commit 5839ad9c7](https://github.com/RPCS3/rpcs3/commit/5839ad9c7) [VERIFIED-SRC via git log -S]
- Optional (used only if present): custom_border_color, border_color_swizzle, multi_draw, conditional_rendering, depth_range_unrestricted, external_memory_host, shader_stencil_export, attachment_feedback_loop_layout, fragment_shader_barycentric, synchronization2, device_fault, provoking_vertex, shaderFloat16, and update-after-bind descriptor bits.
- Disabled with an error log if missing: `depthBounds`, `logicOp`, `fullDrawIndexUint32`. — [RPCS3 device.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/vkutils/device.cpp) [VERIFIED-SRC]
- Shaders: RPCS3 compiles its GLSL to SPIR-V at runtime with glslang (`spirv::compile_glsl_to_spv`). Dozen would then run that SPIR-V through its own SPIR-V→NIR→DXIL compiler. — [VKProgramPipeline.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/VKProgramPipeline.cpp) [VERIFIED-SRC]

**RPCS3's own stance on Dozen**
- RPCS3 recognizes Dozen by its device name "Microsoft Direct3D12" or `VK_DRIVER_ID_MESA_DOZEN`, and shows a Windows MessageBox: "You're attempting to run rpcs3 on Microsoft's Dozen driver that emulates vulkan on top of Direct3D12. This driver is unsupported. You should use your vendor's vulkan driver whenever possible." — [VKHelpers.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/VKHelpers.cpp), [device.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/vkutils/device.cpp) [VERIFIED-SRC]
- The only other Dozen handling is a compute workgroup-size choice of 32. — [VKCompute.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/VKCompute.cpp) [VERIFIED-SRC]

**WSI and presentation in UWP**
- Upstream Mesa's Windows WSI (`wsi_common_win32.cpp`) takes an `HWND` from `VkWin32SurfaceCreateInfoKHR`, calls `GetClientRect(hwnd)`, and builds a DXGI flip swapchain. There is no CoreWindow or SwapChainPanel path. `VK_KHR_win32_surface` is DONE only for dzn and lvp. — [wsi_common_win32.cpp](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/vulkan/wsi/wsi_common_win32.cpp), [features.txt](https://github.com/chaotic-cx/mesa-mirror/blob/main/docs/features.txt) [VERIFIED-SRC]
- RPCS3's Windows swapchain uses `vkCreateWin32SurfaceKHR` (HWND). It also has a "native" `swapchain_WIN32` fallback that presents with GDI `BitBlt`. Neither works in a CoreWindow app without changes. — [swapchain_win32.hpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/vkutils/swapchain_win32.hpp) [VERIFIED-SRC]

**Xbox-specific code in upstream Mesa**
- Upstream Mesa's Xbox work covers GLon12 under the GDK only. Under `_GAMING_XBOX_SCARLETT` it loads `d3d12_xs.dll` and `dxcompiler_xs.dll`. — [d3d12_screen.cpp](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/gallium/drivers/d3d12/d3d12_screen.cpp), [dxil_validator.cpp](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/compiler/dxil_validator.cpp) [VERIFIED-SRC]
- `src/microsoft/vulkan` (Dozen) has no `_GAMING_XBOX` ifdefs. [VERIFIED-SRC: grep of mirror]
- Phoronix (Jan 2023) credited Ethan Lee with the GDK support and said it needs a GDKX agreement with Microsoft. — [Phoronix: Mesa Adds Xbox GDK](https://www.phoronix.com/news/Mesa-Adds-Xbox-GDK) [SNIPPET]

**UWP Mesa forks**
- `aerisarn/mesa-uwp` (Edoardo Canepa, Jan–Mar 2024). Fork commits:
  - "Define _XBOX_UWP to differentiate platform", "CreateSwapChainForCoreWindow", "STUB GDI into vulkan", "USE MY MESON FORK with --uwp", "[HACK] enable SAMPLER_VIEW_TARGET on XBOX UWP", "xbox-swap integration" (2024-03-30).
  - The README builds with a UWP-capable Meson fork (`--uwp`, `/D _XBOX_UWP`). — [aerisarn/mesa-uwp](https://github.com/aerisarn/mesa-uwp) [VERIFIED-SRC: git log]
- `rodrigoandrigo/Mesa-UWP` ("With a focus on improving the dozen"): Mesa 26.2.2, a single import commit on 2026-10-08, then Dozen patches on 2026-10-08/09. Its changes:
  - **Platform glue:** `_XBOX_UWP` paths in the WSI (uses `CreateSwapChainForComposition` and `uwp_get_width/height`), DXGI adapter enumeration instead of DXCore on UWP, and the D3D12 debug layer and GPU-based validation stripped "because not present in the Xbox retail runtime".
  - **New Dozen extensions:** `EXT_custom_border_color` (needs ID3D12Device11, AdvancedTextureOps, SM 6.7 and validator 1.7 or newer), `EXT_conditional_rendering`, `EXT_provoking_vertex`, `EXT_multi_draw`, `EXT_device_fault` (reports only the device-removed HRESULT), `EXT_shader_stencil_export`, `EXT_external_memory_host` (via `D3D12_FEATURE_EXISTING_HEAPS`), and `EXT_shader_uniform_buffer_unsized_array`.
  - **Descriptor fix:** commit ffc9f60's comment says "Applications such as RPCS3 request update-after-bind directly without enabling the aggregate or the promoted EXT extension", and adds a live-descriptor-heap path for them.
  - **DXIL/PSO fix:** commit 77ed30a, "Fix DXIL wave contracts and PSO validation". — [rodrigoandrigo/Mesa-UWP](https://github.com/rodrigoandrigo/Mesa-UWP) [VERIFIED-SRC: git show]
- The same author keeps an `rpcs3` fork (updated 2026-10-09), a "Teste-HW" Xbox UWP hardware-probe app, SDL3_UWP and Cemu-UWP-Host. No RPCS3-on-Xbox results are published, and the fork's visible master commits are upstream-only. — [rodrigoandrigo repositories](https://github.com/rodrigoandrigo?tab=repositories), [rodrigoandrigo/rpcs3 commits](https://github.com/rodrigoandrigo/rpcs3/commits/master) [VERIFIED-SRC]
- A mesa-dev reply on "Xbox Series S/X UWP" said neither GLon12 nor Dozen would work in UWP. It cited missing window-system hooks and no ICD loader in UWP, and said nobody was working on it. The message is older. — [mesa-dev archive](https://www.mail-archive.com/mesa-dev@lists.freedesktop.org/msg224326.html) [SNIPPET; host blocked]

**Performance and community signals**
- Dozen performance (anecdotal): a Windows-on-ARM blog's quick test of Doom measured about 40–75 FPS on Dozen against about 80–200 FPS on native Vulkan. — [devinthreethousand substack](https://devinthreethousand.substack.com/p/driver-tweak-for-windows-arm-could) [SNIPPET, unscientific]
- No formal Dozen-vs-native benchmark was found.
- RetroArch has an open bounty request to add Mesa D3D12 and Dozen to its UWP build for OpenGL and Vulkan on Xbox. — [libretro/RetroArch#13462](https://github.com/libretro/RetroArch/issues/13462) [SNIPPET]

### Inferences
- **Upstream RPCS3 on upstream Dozen would very likely fail `vkCreateDevice`.**
  - It chains `shaderUniformBufferUnsizedArray = VK_TRUE` and sets `runtimeDescriptorArray = VK_TRUE` with no fallback.
  - Upstream Dozen does not support the first feature, and supports the second only at SM ≥ 6.6.
  - Mesa's common runtime validates requested feature structs against supported features. That this causes `VK_ERROR_FEATURE_NOT_PRESENT` is my reading of how Mesa behaves, not something I tested.
  - The rodrigoandrigo fork adds exactly these, which supports the inference.
- **Dozen as of 2026 is a one-person-fork problem.** Upstream adds features slowly. Getting RPCS3 to work means maintaining a Mesa fork with UWP WSI, UWP build glue and RPCS3-specific extensions, and rebasing it as RPCS3 adds new Vulkan requirements (as it did in April 2026).
- **Missing features become correctness issues, not blockers.** Without logicOp, RPCS3 logs an error and disables it; PS3 games that use logic ops would render incorrectly. Without wide lines, large points or depth_range_unrestricted, RPCS3 uses its fallbacks.
- **Each shader is translated twice:** RSX → GLSL → SPIR-V (glslang) → NIR → DXIL (Dozen), then the Xbox driver compiles the DXIL. On an 8-core Zen 2 with a 5 GiB memory budget, expect higher compile stutter and CPU overhead than desktop Vulkan.
- **Time to first pixel is likely the shortest of all options** if the rodrigoandrigo fork is usable: RPCS3's Vulkan backend needs few changes beyond WSI and surface creation and the loader replacement (link the dzn ICD directly or use `vk_icdGetInstanceProcAddr`).

### Gaps
- No published proof (screenshots or FPS) of RPCS3 or any Vulkan-only emulator running through Dozen on Xbox UWP. The rodrigoandrigo work is in progress and undocumented.
- It is unknown whether the Xbox UWP D3D12 runtime reports SM 6.6 or higher, which Dozen's descriptor indexing needs (see Q7 for the conflicting evidence).
- No quantitative Dozen overhead figures on AMD RDNA2.
- Whether a Vulkan loader (`vulkan-1.dll`) can work in UWP was not checked. The fork presumably links the ICD directly, but I could not confirm this.

---

## Q2. Option B: Resurrect RPCS3's removed D3D12 backend

### Takeaway
kd-11 removed the D3D12 backend in commit `00bc3fe65` ("Drop d3d12 backend", 2019-10-22). It was 24 files and about 7.3k lines in `rpcs3/Emu/RSX/D3D12`, and it emitted HLSL compiled with FXC as `vs_5_0`/`ps_5_0` at FL 11_0. It had already been cut off from the shared RSX code in May 2019 and was barely maintained after 2016. Since removal, RSX code has had about 3,060 commits, so the old code no longer fits the current surface store, texture cache, decompiler or interpreter. Resurrection would be a rewrite that uses the old code only as reference.

### Cited Findings
- Commit `00bc3fe65` by kd-11, 2019-10-22, message "Drop d3d12 backend". It touched 72 files and deleted about 92k lines, including bundled dependencies; the `rpcs3/Emu/RSX/D3D12` directory alone was 24 files and 7,273 lines. It was a direct commit, not a PR. — [RPCS3 commit 00bc3fe65](https://github.com/RPCS3/rpcs3/commit/00bc3fe658685b107ffa8ebef61b288ddf1c67f2) [VERIFIED-SRC]
- Follow-up commits:
  - Megamouse "Remove further occurrences of DirectX 12" ([2cc71afdc](https://github.com/RPCS3/rpcs3/commit/2cc71afdc), 2019-10-22).
  - Megamouse "Remove VS D3D12 Dependencies" ([c88654bc8](https://github.com/RPCS3/rpcs3/commit/c88654bc8), 2019-10-23).
  - kd-11 "rsx: Clean up some unused legacy methods unnecessary after d3d removal" ([5f39a594a](https://github.com/RPCS3/rpcs3/commit/5f39a594a), 2019-11-09). [VERIFIED-SRC]
- Prior decoupling, both 2019-05-21:
  - kd-11 "d3d12: Remove all shared code with other backends" ([db5d56a22](https://github.com/RPCS3/rpcs3/commit/db5d56a22)).
  - kd-11 "rsx: Refactoring and cleanup after d3d12 separation - Remove deprecated functionality - Refactor to share code between common routines" ([655eff29e](https://github.com/RPCS3/rpcs3/commit/655eff29e)).
  - In Aug 2018, kd-11 committed "d3d12: restore working graphics" ([85d38b575](https://github.com/RPCS3/rpcs3/commit/85d38b575)), which implies the backend had been broken. [VERIFIED-SRC]
- Commit activity in the D3D12 directory by year: 508 (2015), 126 (2016), 56 (2017), 38 (2018), 21 (2019). The VK directory had 232 (2017), 222 (2018) and 219 (2019). — [RPCS3 git history](https://github.com/RPCS3/rpcs3/commits/master/rpcs3/Emu/RSX) [VERIFIED-SRC: git log counts]
- Contents of the removed backend:
  - Its own HLSL decompilers: `D3D12FragmentProgramDecompiler` and `D3D12VertexProgramDecompiler`.
  - Shaders compiled at runtime via `D3DCompile` with `"vs_5_0"`/`"ps_5_0"`; the device was created at `D3D_FEATURE_LEVEL_11_0`.
  - Its own `data_cache m_texture_cache` and `rsx::render_targets m_rtts`. — [D3D12PipelineState.cpp @14c63ec01](https://github.com/RPCS3/rpcs3/blob/14c63ec01/rpcs3/Emu/RSX/D3D12/D3D12PipelineState.cpp), [D3D12GSRender.cpp @14c63ec01](https://github.com/RPCS3/rpcs3/blob/14c63ec01/rpcs3/Emu/RSX/D3D12/D3D12GSRender.cpp) [VERIFIED-SRC]
- The header's own TODO list of unimplemented items: vertex textures, MSAA ("needs to be implemented from scratch"), a depth buffer read that needed odd RGBA8 conversion, range-keyed vertex and texture caching that "may be wrong", and incomplete Cell/RSX sync that "leads to deadlock". — [D3D12GSRender.h @14c63ec01](https://github.com/RPCS3/rpcs3/blob/14c63ec01/rpcs3/Emu/RSX/D3D12/D3D12GSRender.h) [VERIFIED-SRC]
- RSX churn since removal: about 3,060 commits to `rpcs3/Emu/RSX` (by year: 2020 475, 2021 425, 2022 438, 2023 360, 2024 250, 2025 440, 2026 to Oct 576).
  - By area: 604 Common (surface store, texture cache), 367 Program (decompilers, interpreter), 1,240 VK, 675 GL.
  - Recent 2026 work includes rsx/fp ROP output remap, a programmable-blending fix, IFE/LOOP decode fixes in the shader interpreter, and vp constant-access changes. — [RPCS3 RSX history](https://github.com/RPCS3/rpcs3/commits/master/rpcs3/Emu/RSX/Program) [VERIFIED-SRC: git log]
- I found no official blog post giving a reason for the removal. The commit message has no body.
- In February 2023 the RPCS3 account said on X: "Until Xbox Series supports Vulkan, there's no point, as that's the only relevant platform for an UWP build". — [RPCS3 on X](https://x.com/rpcs3/status/1622695113343307793) [SNIPPET]

### Inferences
- **The old backend is not a viable base.** It predates today's shared `surface_store` and texture cache (`texture_cache.h` and helpers), the shader interpreter, the MSAA and upscaler paths, the overlays system and the FIFO optimizations. Its HLSL decompiler would need to absorb 367-plus decompiler fixes made since.
- **Its only value is reference**, for RSX→D3D12 state mapping (blend, depth, primitive and format tables).
- **Upstream will not accept it:** there is no maintainer interest, and RPCS3 considers Vulkan the only relevant UWP target.

### Gaps
- No primary statement of why the backend was dropped, beyond the commit history showing it was unmaintained, decoupled and broken.
- No accounting of which games the D3D12 backend could still render in 2019.

---

## Q3. Option C: A new native D3D12 backend for RPCS3

### Takeaway
A native backend would be a third backend alongside VK (about 33k lines) and GL (about 18k lines). It would reuse `rsx::thread`/`GSRender`, the common surface store and texture cache templates, `ProgramStateCache` and the GLSL-emitting decompilers.

The cheapest shader route is to keep RPCS3's GLSL, compile it with glslang to SPIR-V (already done for VK), then convert SPIR-V to DXIL with either:
- Mesa's standalone `spirv_to_dxil`, which needs dxil.dll or unsigned DXIL; or
- SPIRV-Cross → HLSL → DXC (`dxcompiler.dll` is shippable in a UWP package and loads via `LoadPackagedLibrary`, per NXbox).

The best analog, NXbox's native D3D12 renderer for Eden, took months and is still partial: about 24 FPS on one screen, with no geometry shaders, tessellation, transform feedback or MSAA. A native backend gives the best long-term performance and control on Xbox, but has the longest time to first pixel and carries a permanent fork-maintenance burden.

### Cited Findings
**RPCS3 code to reuse or replicate**
- Current RSX layout:
  - Shared: `Common/` (surface_store, texture_cache*), `Program/` (FragmentProgramDecompiler, VertexProgramDecompiler, GLSLCommon, SPIRVCommon, ShaderInterpreter, GLSLInterpreter, GLSLSnippets, MSAA, Upscalers, ProgramStateCache), `Overlays/`, `Core/`.
  - Backends: `VK/` and `GL/`.
  - Line counts: Common about 17.2k, Program about 15.9k, VK about 33.1k, GL about 18.2k, Overlays about 20.2k. — [RPCS3 RSX tree](https://github.com/RPCS3/rpcs3/tree/master/rpcs3/Emu/RSX) [VERIFIED-SRC: sparse checkout and wc]
- Compute helpers are written as GLSL snippets (`CopyD24x8ToBuffer.glsl`, `GPUDeswizzle.glsl`, `RSXMemoryTiling.glsl`, `ShuffleBytes.glsl`, `E4M12Conversion.glsl`, and others). They would need porting or transpiling too. — [GLSLSnippets](https://github.com/RPCS3/rpcs3/tree/master/rpcs3/Emu/RSX/Program/GLSLSnippets) [VERIFIED-SRC]
- RPCS3 has Vulkan swapchain backends for win32, unix, macOS (Metal layer) and Android, so the WSI is already abstracted per platform. — [vkutils](https://github.com/RPCS3/rpcs3/tree/master/rpcs3/Emu/RSX/VK/vkutils) [VERIFIED-SRC]

**Shader path options**
- Mesa ships `src/microsoft/spirv_to_dxil` (`spirv_to_dxil.c/.h/.def`, `spirv2dxil.c`) as a standalone library. — [spirv_to_dxil (Mesa mirror)](https://github.com/chaotic-cx/mesa-mirror/tree/main/src/microsoft/spirv_to_dxil) [VERIFIED-SRC]
- A search summary said the `temamumtaza/eden-xbox` UWP port with a native D3D12 renderer uses Mesa's `spirv_to_dxil` for shader translation. The README on its `xbox` branch did not confirm the renderer details. — [temamumtaza/eden-xbox](https://github.com/temamumtaza/eden-xbox) [SNIPPET]

**Analog 1: NXbox (Eden Switch emulator UWP port)** [COMMUNITY] — [NXbox README](https://github.com/NspxMiguel/NXbox), [NXbox d3d12-backend.md](https://github.com/NspxMiguel/NXbox/blob/main/docs/d3d12-backend.md)
- Shader path: Maxwell IR → textual HLSL → packaged DXC → DXIL. Spirv_to_dxil and Mesa were deliberately excluded from the native path.
- `dxcompiler.dll` is packaged, loaded with `LoadPackagedLibrary`, and driven through `DxcCreateInstance`/`IDxcCompiler3`. Validated DXIL is produced before PSO creation.
- Swapchain: `IDXGIFactory2::CreateSwapChainForCoreWindow` with the CoreWindow `IUnknown`, flip model.
- All root signatures are version 1.0. The view heap holds 262,144 descriptors and the sampler heap 2,048.
- `ID3D12PipelineLibrary` is off by default because "even an empty library removes the Xbox device".
- Results so far:
  - Draws SM3DW's game-select screen at about 24 FPS. The app is suspended after about 90 s.
  - Geometry and tessellation shaders, transform feedback and MSAA are not done.
  - The work is staged P1 (clear/present), P2 (buffers/textures), P3 (pipelines).

**Analog 2: Cemu UWP Host** [COMMUNITY] — [Cemu-UWP-Host README](https://github.com/rodrigoandrigo/Cemu-UWP-Host)
- Its stable renderer is a custom D3D11 backend at exactly FL 11.0, running on Xbox's D3D11On12. Shaders go through "GLSL/SPIR-V/HLSL compatibility paths".
- It also has an "experimental internal Vulkan-to-Direct3D 12 backend" that redirects Cemu's Vulkan dispatch into an in-process D3D12 translation layer. "Mesa/Dozen is design reference only and is not a runtime or build dependency."
- It needs a 128 MB stack reserve "so the Xbox driver compiler doesn't hit the stack guard during complex pipeline creation".

### Inferences
- **Shader route trade-offs:**
  - **(i) glslang → SPIR-V → SPIRV-Cross HLSL → DXC.** Reuses all upstream decompiler fixes. DXC is known to load in a UWP package on Xbox. The cost is two compiler stacks and their memory on a 5 GiB budget.
  - **(ii) glslang → SPIR-V → `spirv_to_dxil`.** Avoids DXC but pulls in Mesa's NIR compiler and still needs signing or unsigned acceptance (see Q7).
  - **(iii) A native HLSL emitter alongside the GLSL emitter.** Best output, but it forks the decompiler and is the worst for keeping up with upstream.
  - Route (i) or (ii) keeps the decompiler shared with upstream. That matters because Program/ alone took 367 commits since 2019 and more than 25 interpreter and decompiler fixes in Jul–Oct 2026.
- **Rough scope:** expect a VK-sized backend (about 25–35k lines). Plausible items:
  - Descriptor management mapping RPCS3's VK descriptor layout to root signatures.
  - Resource state tracking.
  - D3D12 equivalents of the conditional rendering, ZCULL and occlusion queries, the D24S8/D32S8 conversion compute passes, and the MSAA resolve and "unresolve" passes.
  - An overlay renderer.
  - A CoreWindow swapchain.
- **Time to first pixel is longest** (months, by analogy to NXbox), and it is unlikely ever to be accepted upstream. However, it removes the Mesa-fork dependency and the double translation, and it can use Xbox-specific D3D12 behavior directly.

### Gaps
- No concrete estimate from RPCS3 developers of the effort for a new backend.
- Not verified whether SPIRV-Cross HLSL output handles RPCS3's GLSL features: subgroup operations, float16, UBO unsized arrays, texel buffers, and fragment barycentric fallbacks.
- `spirv_to_dxil` running inside a UWP package on Xbox is unverified beyond a search snippet.

---

## Q4. Option D: OpenGL through Mesa's d3d12 Gallium driver (GLOn12), or ANGLE

### Takeaway
GLOn12 exposes desktop OpenGL 4.6 on D3D12 upstream, and a patched UWP build reports "OpenGL 4.6 on D3D12 (SraKmd_arden)" on a retail Series X in Developer Mode. NXbox reached 30–60 FPS in some Switch titles with it, which shows the path is real. But it is fragile:
- Driver bugs: a query bug blacked out every frame, and GPU ASTC decoding produced garbage.
- Async shaders hang.
- A second GL context cannot be made current on Xbox's Mesa WGL.

That last point breaks RPCS3 GL's multi-context asynchronous shader compiler. GLOn12 also lacks `ARB_bindless_texture`, which disables RPCS3 GL's shader interpreter. ANGLE is GLES-only, and RPCS3 needs desktop GL with DSA and SSBOs, so ANGLE is not viable.

### Cited Findings
- Mesa `features.txt` lists d3d12 under GL 4.3, 4.4, 4.5 and 4.6 "all DONE". `GL_ARB_bindless_texture` is DONE only for nvc0, radeonsi and zink, not d3d12. — [features.txt](https://github.com/chaotic-cx/mesa-mirror/blob/main/docs/features.txt) [VERIFIED-SRC]
- RPCS3 GL backend requirements and behavior:
  - Required: `GL_ARB_texture_buffer_object`, plus `ARB_direct_state_access` or `EXT_direct_state_access`. SSBOs are needed for several paths.
  - Without bindless textures: "Bindless texture extension required for shader interpreter is not supported on your GPU. Will use async recompiler as a fallback."
  - In any shader mode other than `recompiler`, it creates extra shared GL contexts for the pipe compiler threads (`make_context`, `set_current`, `initialize_pipe_compiler(..., shader_compiler_threads_count)`). — [GLGSRender.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/GL/GLGSRender.cpp) [VERIFIED-SRC]
- NXbox console results [COMMUNITY] — [NXbox port notes](https://github.com/NspxMiguel/NXbox/blob/main/docs/nxbox-port.md), [NXbox README](https://github.com/NspxMiguel/NXbox):
  - Uses `aerisarn/mesa-uwp` release `alpha-2-resfix` (rev 15acdd7e) plus its own patch script. Mesa reported OpenGL 4.6 on "D3D12 (SraKmd_arden)", with a measured 5 GiB application memory limit.
  - "A second GL context cannot be made current on the Xbox's Mesa WGL: surfaceless `wglMakeCurrent` fails."
  - `use_asynchronous_shaders=true` hangs Persona 5 Royal on Mesa d3d12.
  - Fixed black screen: "Mesa's D3D12 driver began a pipeline-statistics query twice, so every command list failed to close and no GPU work ever ran."
  - The GPU ASTC compute decoder "leaves garbage in the lower mip levels on Mesa d3d12".
  - The Mesa UWP shim calls `CoreWindow::GetForCurrentThread()`, which broke context activation on a worker thread.
  - Performance on the GL path: 60 FPS in the SM3DW hub, 30 FPS in P5R, about 12 FPS in BotW (CPU-bound). The clear-screen driver test did 3,590 presents in 60 s.
- Phoronix (March 2024) reported aerisarn's UWP Mesa running on Xbox, with a Doom 3 fork (dhewm3) as the showcase. — [Phoronix: UWP'ed Mesa Running On Microsoft Xbox](https://www.phoronix.com/news/UWP-Mesa-Xbox-Port) [SNIPPET]
- A ResetEra thread was titled "Xbox Dev mode gets Mesa (OpenGL/Vulkan) support. Citra emu already in alpha". — [ResetEra](https://www.resetera.com/threads/xbox-dev-mode-gets-mesa-opengl-vulkan-support-citra-emu-already-in-alpha.921633/) [SNIPPET; title only, Vulkan claim unverified]
- Mesa's own d3d12 driver documentation still describes the goal as "full desktop OpenGL 3.3 support on devices that only support D3D12", although features.txt lists GL 4.6 done. — [d3d12.rst](https://github.com/chaotic-cx/mesa-mirror/blob/main/docs/drivers/d3d12.rst) [VERIFIED-SRC]

### Inferences
- **RPCS3's GL backend on GLOn12 would be restricted** to `shadermode = recompiler`: synchronous compilation on one context, with heavy stutter. The interpreter needs bindless textures and async needs shared contexts, so neither is available. RPCS3 GL is also less maintained and slower than VK upstream.
- **The translation stack is longer** than with Dozen: GL state machine → Gallium → D3D12. Shaders go GLSL → NIR → DXIL, with no glslang step.
- **GLOn12 is the most proven Mesa path on Xbox UWP**, with real console results in NXbox. Even so, NXbox is now moving to a native D3D12 renderer because of these limits.

### Gaps
- No measurement of RPCS3's GL backend on GLOn12 (desktop or Xbox).
- Unknown whether the shared-context limit can be fixed in the UWP WGL shim.

---

## Q5. Option E: Other translation layers

### Takeaway
Besides Dozen, the only Vulkan→D3D12 layer found is Cemu-UWP-Host's private, experimental, in-process one, built for Cemu's Vulkan usage only. vkd3d and vkd3d-proton go the other way (D3D12→Vulkan) and do not help. MoltenVK targets Metal. RPCS3 does already run on a translation layer on macOS (MoltenVK), so its Vulkan backend tolerates a reduced feature set.

### Cited Findings
- Cemu-UWP-Host's experimental backend "keeps Cemu's Vulkan renderer front-end and redirects its Vulkan dispatch into an in-process D3D12 translation layer. Mesa/Dozen is design reference only". — [Cemu-UWP-Host](https://github.com/rodrigoandrigo/Cemu-UWP-Host) [COMMUNITY]
- RPCS3's Vulkan device code has an Apple/MoltenVK vendor path and requests `VK_KHR_portability_subset` on Apple. So the Vulkan backend already copes with a layered, non-conformant implementation. — [RPCS3 device.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/vkutils/device.cpp) [VERIFIED-SRC]
- A search summary said a PS5 emulator port to Xbox Dev Mode translates Vulkan to DirectX with an unnamed layer. — search summary (no primary source) [SNIPPET, unverified]

### Inferences
- **A bespoke Vulkan→D3D12 shim (the Cemu-UWP-Host approach)** would let you implement only the Vulkan subset RPCS3 uses. In effect it is "Option C under a Vulkan facade". It avoids patching RSX but is still a large project, and it has to track RPCS3's Vulkan usage.
- **SPIRV-Cross is a component** (SPIR-V→HLSL), not a layer. It belongs inside Option C or a custom shim.

### Gaps
- No public, general-purpose Vulkan→D3D12 layer other than Dozen was found.

---

## Q6. What other Xbox Dev Mode emulators did, especially Vulkan-first ones

### Takeaway
No Vulkan-first emulator has a published, working Xbox port through Dozen. Mature Xbox dev-mode emulators either had native D3D11 or D3D12 backends (Dolphin, Xenia Canary, XBSX2/PCSX2, PPSSPP, RetroArch cores) or wrote new D3D renderers. For Vulkan/GL-first emulators:
- **Eden/NXbox:** went to GLOn12 first, now building native D3D12.
- **Cemu-UWP-Host:** wrote a custom D3D11 renderer plus an experimental private Vulkan→D3D12 layer.

RPCS3 maintainers closed the Mesa-UWP request as "not of interest", citing dev-mode restrictions, especially RAM.

### Cited Findings
- **RPCS3's position:**
  - Issue #15787, "Support for Xbox through new Mesa UWP port" (2024-07-10), was closed as not planned by AniLeo: "Duplicate of #9315, not of interest and still blocked by other dev mode restrictions".
  - Contributor FlexBy420: "there's not enough ram in dev mode".
  - The issue was locked on 2025-05-22. — [RPCS3#15787](https://github.com/RPCS3/rpcs3/issues/15787) [VERIFIED-SRC]
- **NXbox (Eden):** GL via a patched aerisarn Mesa as the default. A native D3D12 renderer (HLSL+DXC) is opt-in. "No game is fully playable yet". — [NXbox](https://github.com/NspxMiguel/NXbox) [COMMUNITY]
- **Cemu-UWP-Host:** D3D11 is the stable renderer, alongside the experimental Vulkan→D3D12 layer. It does not match upstream Vulkan compatibility. — [Cemu-UWP-Host](https://github.com/rodrigoandrigo/Cemu-UWP-Host) [COMMUNITY]
- **Catalog:** the Emulation Revival Xbox Dev Mode catalog lists Azahar, Dolphin, Flycast, PPSSPP, RetroArch, Retroarch 3DS, XBSX2, Xenia Canary and "Chonkystation 3". — [Emulation Revival app data (repo)](https://github.com/emulationrevival/emulationrevival.github.io/blob/main/json/app-links.json) [VERIFIED-SRC: repo JSON]
- **Other PS3 emulation on Xbox:** ChonkyStation 3 has a UWP "Proof of Concept" guide described as highly experimental. — [ChonkyStation 3 UWP guide](https://wiki.xbdev.store/en/Chonkystation3Guide) [SNIPPET]

### Inferences
- **The pattern among serious ports:** a Mesa GL path as a stopgap, then native D3D renderers. No one has demonstrated Dozen as a production path on Xbox. RPCS3 would be the first heavy Vulkan-only emulator to try.

### Gaps
- Graphics backends for the Azahar and ChonkyStation 3 Xbox builds were not confirmed (Mesa GL vs. native).
- No shadPS4, Vita3K, Ryujinx or yuzu Xbox ports via Dozen were found.

---

## Q7. Xbox GPU and D3D12-in-UWP specifics relevant to RSX emulation

### Takeaway
The hardware is RDNA2 (DirectX 12 Ultimate class). But Microsoft's own guidance for UWP on Series X|S is D3D12 **Feature Level 11.0** in both App and Game mode, with **Shader Model 5.1–6.4**; DXR, HDR and other DX12 Ultimate features need ID@Xbox. Console field reports show three things:
- The device is reported as "SraKmd_arden".
- A packaged `DXIL.dll` (validator 1.8 from the Windows SDK) and `dxcompiler.dll` load.
- Unsigned DXIL is accepted.

There is also a hint of SM 6.7/6.8 compilation via Mesa. Whether SM 6.6 (needed for Dozen bindless) is officially available is unresolved. The app memory budget is about 5 GiB.

### Cited Findings
**Official guidance (Chuck Walbourn, Microsoft, 2022-05-20)** — [Walbourn: DirectX and UWP on Xbox Series X|S](https://walbourn.github.io/directx-and-uwp-on-xbox-series-x-s/) (read via its [blog source repo](https://github.com/walbourn/walbourn.github.io/blob/main/_posts/2022-05-20-directx-and-uwp-on-xbox-series-x-s.md)) [VERIFIED-SRC]
- Series X|S UWP: "DirectX 11 Feature Level 11.0 / DirectX 12 Feature Level 11.0" in both App Mode and Game Mode.
- "For DirectX 12, Shader Models 5.1 to 6.4 is supported".
- HDR, DXR and other DX12 Ultimate features: "contact the ID@Xbox program".
- Since the August 2023 recovery, UWP development requires Windows SDK 22621 or later.
- The older Xbox One post notes that in Developer Mode a D3D12 device created outside Game mode becomes WARP. — [Walbourn: DirectX and UWP on Xbox One](https://walbourn.github.io/directx-and-uwp-on-xbox-one/) [VERIFIED-SRC]

**Hardware**
- Microsoft's DirectX 12 Ultimate announcement positions Series X as supporting DXR, VRS, Mesh Shaders and Sampler Feedback. That describes hardware, not UWP exposure. — [DirectX Dev Blog: Announcing DirectX 12 Ultimate](https://devblogs.microsoft.com/directx/announcing-directx-12-ultimate/) [SNIPPET]

**Field reports from Series X in Developer Mode** [COMMUNITY] — [NXbox port notes](https://github.com/NspxMiguel/NXbox/blob/main/docs/nxbox-port.md)
- Mesa d3d12 reports the adapter as "D3D12 (SraKmd_arden)".
- "DXIL.dll loads (validator 1.8, Windows SDK build). Every shader fails validation with `0x80AA0013` … at shader model 6.8 and at 6.7, so no shader is signed; the Xbox runtime still accepts them."
- "The DXC release validator cannot load in the package (error 126, it imports the desktop CRT)."
- "Adding the packaged DXIL validator resolved the initial pipeline-creation crash."
- An `NXBOX_D3D12_MAX_SM` knob exists.

**Other Xbox UWP runtime behavior**
- The D3D12 debug layer, GPU-based validation and DXGI debug "are not present in the Xbox retail runtime". — [rodrigoandrigo/Mesa-UWP dzn_device.c](https://github.com/rodrigoandrigo/Mesa-UWP/blob/main/src/microsoft/vulkan/dzn_device.c) [COMMUNITY/SRC]
- NXbox's native renderer: packaged `dxcompiler.dll` works via `LoadPackagedLibrary`. Pipeline libraries remove the device. Root signature 1.0 is used. Device-removed reason `0x887a0020` was seen. — [NXbox d3d12-backend.md](https://github.com/NspxMiguel/NXbox/blob/main/docs/d3d12-backend.md) [COMMUNITY]
- On Xbox, D3D11 runs as D3D11On12. A process-memory limit "close to 5120 MB" was observed on Series S. The Xbox driver compiler needed a 128 MB stack reserve. — [Cemu-UWP-Host](https://github.com/rodrigoandrigo/Cemu-UWP-Host) [COMMUNITY]
- NXbox measured a 5 GiB app budget on Series X. — [NXbox](https://github.com/NspxMiguel/NXbox) [COMMUNITY]
- RPCS3 issue commenters claim about 4 GB in dev mode. — [RPCS3#15787](https://github.com/RPCS3/rpcs3/issues/15787) [COMMUNITY]

**Shader-model dependencies**
- Dozen's descriptor indexing, buffer device address and subgroupSizeControl need SM ≥ 6.6. In the rodrigoandrigo fork, custom border color needs SM ≥ 6.7. — [dzn_device.c (upstream)](https://github.com/chaotic-cx/mesa-mirror/blob/main/src/microsoft/vulkan/dzn_device.c), [Mesa-UWP fork](https://github.com/rodrigoandrigo/Mesa-UWP) [VERIFIED-SRC]

### Inferences
- **Feature level 11.0 does not by itself rule out optional caps.** ROVs, typed UAV loads, conservative rasterization, programmable sample positions, wave ops and native 16-bit ops are queried through `CheckFeatureSupport` (`D3D12_OPTIONS*`), independent of the feature level the device was created at. RDNA2 supports them in hardware.
- **Dozen bindless is the critical unknown.** If the Xbox UWP runtime really caps at SM 6.4, Dozen loses descriptor indexing, so RPCS3's mandatory `runtimeDescriptorArray` fails. The fact that Mesa compiled at SM 6.7/6.8 on the console and the shaders ran suggests the runtime may report a higher SM than Walbourn's 2022 table. This needs a direct `D3D12_FEATURE_SHADER_MODEL` probe on hardware.
- **Signing is probably not a blocker.** Unsigned-DXIL acceptance plus packageable DXIL.dll and dxcompiler.dll mean Dozen's validator requirement can likely be met. Note the "validator 1.8 fails with LLVM_UNREACHABLE" quirk.
- **The 5 GiB budget hits every option.** PS3 main RAM and VRAM mirrors, the PPU/SPU LLVM JIT, and shader caches have to fit, and double translation (Dozen, GLOn12) adds compiler memory. This favors native D3D12 or a lean Dozen configuration.

### Gaps
- No primary source listing the actual `D3D12_FEATURE_DATA_D3D12_OPTIONS*` values on Series X UWP: resource binding tier, ROVs, typed UAV loads, conservative rasterization, programmable sample positions, wave lane count, 16-bit ops, or highest shader model. The rodrigoandrigo "Teste-HW" app probes Xbox UWP capabilities, but its README does not publish these values.
- No official word on whether the Agility SDK (D3D12Core redistributable) works in UWP on Xbox. Field evidence suggests the console's built-in runtime is used.

---

## Q8. Comparison against the recommendation criteria

### Takeaway
| Criterion | A: Dozen (Vulkan backend unchanged) | B: Resurrect old D3D12 | C: New native D3D12 | D: GLOn12 (GL backend) |
|---|---|---|---|---|
| Time to first pixel | **Shortest**, if a UWP Dozen fork (rodrigoandrigo/Mesa-UWP) is used. Needs WSI/loader glue and the RPCS3 hard requirements (UBO unsized arrays, SM 6.6 bindless) satisfied. | Long. 2019 code is incompatible with current RSX common code. | Longest (months; NXbox analog). | Short to medium. Proven GL 4.6 on console, but needs a single-context, sync-shader setup. |
| Upstream maintainability | Medium-poor. RPCS3 code nearly unchanged, but you own a Mesa fork that must track new RPCS3 Vulkan requirements (e.g. Apr 2026). RPCS3 flags Dozen as unsupported. | Poor. Dead code with no upstream interest. | Poor-medium. A separate backend that shares Common/Program, with risk on every RSX refactor (about 440–576 RSX commits a year). | Medium. GL backend unchanged, but it is RPCS3's secondary backend, plus a Mesa fork. |
| Performance | Medium-low. Double shader translation and command translation overhead; Mesa-on-desktop anecdote shows about 2× slower than native in one game. | Unknown or poor (SM 5.0 FXC, old design). | **Best potential** (direct D3D12, Xbox-tuned). | Low. GL→Gallium→D3D12; sync shader compile only, no interpreter. |
| Correctness | Medium. Missing logicOp, wide lines, depth_range_unrestricted; Dozen non-conformant; RPCS3 fallbacks exist. | Poor (MSAA, vertex textures and others missing in 2019). | High in principle, but each feature must be rebuilt (NXbox still lacks GS, xfb, MSAA). | Medium-low. Mesa d3d12 bugs seen on Xbox (queries, ASTC compute). |

### Cited Findings
- See the citations in Q1–Q7 for every cell above.
- Key anchors:
  - RPCS3's hard Vulkan requirements and its Dozen warning. — [device.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/vkutils/device.cpp), [VKHelpers.cpp](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/RSX/VK/VKHelpers.cpp)
  - The Dozen feature table. — [features.txt](https://github.com/chaotic-cx/mesa-mirror/blob/main/docs/features.txt)
  - The UWP Dozen fork aimed at RPCS3. — [rodrigoandrigo/Mesa-UWP](https://github.com/rodrigoandrigo/Mesa-UWP)
  - The native D3D12 effort in NXbox. — [NXbox d3d12-backend.md](https://github.com/NspxMiguel/NXbox/blob/main/docs/d3d12-backend.md)
  - The Xbox UWP FL/SM limits. — [Walbourn 2022](https://walbourn.github.io/directx-and-uwp-on-xbox-series-x-s/)
  - The D3D12 removal and its age. — [RPCS3 00bc3fe65](https://github.com/RPCS3/rpcs3/commit/00bc3fe658685b107ffa8ebef61b288ddf1c67f2)

### Inferences
- **Recommended staging (my inference, not sourced):**
  1. **Phase 1, Option A:** run RPCS3's Vulkan backend unchanged on a UWP Dozen fork. Start from rodrigoandrigo/Mesa-UWP; aerisarn's fork is the reference for CoreWindow WSI. This is the fastest way to validate the whole port (JIT, memory, I/O) and get first pixels. Gate it on an early on-console probe of `HighestShaderModel ≥ 6.6`, `Native16BitShaderOpsSupported` and `WaveOps`. If SM 6.6 is not exposed, Dozen bindless fails, and RPCS3's mandatory `runtimeDescriptorArray` breaks unless Dozen or RPCS3 is patched.
  2. **Phase 2, Option C, only if Dozen overhead or correctness proves limiting:** a native D3D12 backend that reuses RPCS3's GLSL decompilers via glslang → SPIR-V, then SPIRV-Cross → DXC (DXC proven loadable on Xbox UWP) or `spirv_to_dxil`. Use the Vulkan backend as the architectural template.
- **Options B and D are not recommended as primary paths.**
  - B gives no real head start.
  - D forces RPCS3 GL into sync-only compilation on a driver with known Xbox bugs, and NXbox itself is moving away from GL.

### Gaps
- No on-console performance data for any RPCS3 configuration exists. All performance comparisons are inferred from analogs: NXbox GL vs. native D3D12, and a Doom-on-Dozen anecdote.
