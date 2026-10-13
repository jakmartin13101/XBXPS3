# RPCS3 codebase porting inventory for a Windows UWP (Xbox Series X Dev Mode) port

**Snapshot I analyzed:** RPCS3 `master` at commit `2ba2f75bcea7ef6bdd687046c7fcfb790faf9853`, dated 2026-10-11 ("Wiimote to GunCon3 (#18154)"). I confirmed it was the remote master HEAD on 2026-10-12 with `git ls-remote`. It was a shallow clone, without submodules. Submodules were inspected by pinned SHA through `git ls-tree` and raw files.

Every `rpcs3/...` or `Utilities/...` path below points to that commit. The link base is `https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/`.

**"In the UWP list" convention:** a Win32 API is called "in the UWP list" when it appears in Microsoft's *APIs present on all Windows devices* page. That page lists the exports of `WindowsApp.lib`, the umbrella library for UWP apps. I used the docs-source copy: [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md), which renders at learn.microsoft.com/uwp/win32-and-com/win32-apis. learn.microsoft.com was blocked by the proxy from this sandbox, so I read the MicrosoftDocs GitHub sources instead.

---

## 1. Top-level structure, core and GUI separation, emu_callbacks, and whether a headless or non-Qt build works

### Takeaway

The emulator core is a static library, `rpcs3_emu`, that does not link Qt. The Qt GUI (`rpcs3_ui`) and the frontend glue (`rpcs3_lib`, `main_application`, `gui_application`, `headless_application`) sit on top of it. The core talks to any frontend only through the global `emu_callbacks g_emu_callbacks` struct. In this snapshot the old `EmuCallbacks` / `Emu.SetCallbacks()` API has been replaced by that global.

A desktop non-Qt build has no switch today. There is no `USE_QT` or `WITHOUT_GUI` option; even `--headless` mode is a `QCoreApplication` and accepts only the Null renderer.

The core does build without Qt for Android, through `if (NOT ANDROID)` guards. That gives a ready template for a UWP frontend.

The native RSX overlay system is Qt-free and lives in `rpcs3_emu`. It covers message dialogs, the OSK, save and user lists, trophies, the home menu and the new Big Picture Mode game launcher. It needs only a Vulkan or GL renderer plus a few callbacks: fonts, localized strings and images.

### Cited findings

**CMake targets**
- `rpcs3/CMakeLists.txt` includes `3rdparty/qt6.cmake` only `if (NOT ANDROID)` (line 31). It adds `Emu` unconditionally (line 35) and `rpcs3qt` only when not Android (line 38). — [rpcs3/CMakeLists.txt#L26-L40](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/CMakeLists.txt#L26-L40)
- `rpcs3_lib` (STATIC) links `rpcs3_emu` publicly and `rpcs3_ui` + `3rdparty::qt6` privately. — [rpcs3/CMakeLists.txt#L45-L67](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/CMakeLists.txt#L45-L67)
  - On WIN32 it also links `ws2_32 Iphlpapi Winmm Psapi gdi32 setupapi`. — [rpcs3/CMakeLists.txt#L90-L91](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/CMakeLists.txt#L90-L91)
- The `rpcs3` executable is `add_executable(rpcs3 WIN32 MACOSX_BUNDLE)` with only `main.cpp`. — [rpcs3/CMakeLists.txt#L101-L118](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/CMakeLists.txt#L101-L118)
  - `main()` simply calls `run_rpcs3(argc, argv)`. — [rpcs3/main.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/main.cpp)
- `rpcs3_emu` is a STATIC library with:
  - core sources;
  - `../util/*` and `../../Utilities/*` (Thread, File, JITLLVM, JITASM, vm_native and others);
  - Crypto, Loader, Audio, Cell (PPU/SPU/lv2/HLE modules), CPU, Io, NP, Memory, RSX.
  - [rpcs3/Emu/CMakeLists.txt#L1-L95](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L1-L95)
  - Its link list (lines ~690-720) has no Qt target. It contains: llvm, asmjit, ffmpeg, sdl3, opengl, stblib, vulkan, glew, libusb, hidapi, wolfssl, openal, cubeb, soundtouch, miniupnpc, libevdev, protobuf, pugixml, VMA, glslang, libpng, rtmidi, yaml-cpp, zlib, zstd, libcurl. — [rpcs3/Emu/CMakeLists.txt#L690-L720](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L690-L720)
- `rpcs3_ui` is a STATIC library in `rpcs3/rpcs3qt/CMakeLists.txt` and links `3rdparty::qt6`. — [rpcs3/rpcs3qt/CMakeLists.txt#L1](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/CMakeLists.txt#L1), [#L221-L224](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/CMakeLists.txt#L221-L224)
  - It also compiles the frontend glue: `../headless_application.cpp`, `../main_application.cpp`, `../rpcs3.cpp`, `../display_sleep_control.cpp`.
  - It compiles **all of `rpcs3/Input/*`** (pad_thread, xinput, ds3/ds4/dualsense, sdl, mm_joystick, keyboard, raw_mouse and others). The input handlers therefore live outside the core library today. — [rpcs3/rpcs3qt/CMakeLists.txt#L150-L190](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/CMakeLists.txt#L150-L190)
- `3rdparty/qt6.cmake` requires Qt ≥ 6.7.0 with Widgets, Concurrent, Multimedia, MultimediaWidgets, Svg and SvgWidgets. — [3rdparty/qt6.cmake#L1-L7](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty/qt6.cmake#L1-L7)
  - BUILDING.md targets Qt 6.12.0, Vulkan SDK 1.4.341.1 and VS 2022/2026. — [BUILDING.md#L11-L24](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/BUILDING.md#L11-L24)
- The top-level `CMakeLists.txt` has options such as `WITH_LLVM`, `BUILD_LLVM`, `USE_VULKAN`, `USE_SDL`, `USE_FAUDIO`, `USE_DISCORD_RPC` and `USE_SYSTEM_*`. It has **no** `USE_QT`, `WITHOUT_GUI` or `HEADLESS` option. — [CMakeLists.txt#L31-L67](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/CMakeLists.txt#L31-L67)
- Windows has two build systems:
  - an MSBuild solution (`rpcs3.sln` with `emucore.vcxproj`, `VKGSRender.vcxproj`, `rpcs3.vcxproj` and others; BUILDING.md calls it "the preferred building solution");
  - CMake presets (`msvc`, `msvc-release`).
  - [BUILDING.md#L26](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/BUILDING.md#L26)
  - MSVC links with `/STACK:8388608` (8 MB main-thread stack). — [buildfiles/cmake/ConfigureCompiler.cmake#L21-L22](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/buildfiles/cmake/ConfigureCompiler.cmake#L21-L22)

**Frontend classes**
- `main_application` is an abstract base with `virtual void Init() = 0` and `virtual QThread* get_thread() = 0`. It has `static InitializeEmulator(user, show_gui, headless)`, `virtual create_callbacks()`, a `std::shared_ptr<render_creator>` and a `gs_frame* m_game_window`. It includes `rpcs3qt/render_creator.h` and `<QThread>`, so the base itself is Qt-coupled. — [rpcs3/main_application.h](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/main_application.h)
- `headless_application : public QCoreApplication, public main_application`. — [rpcs3/headless_application.h](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/headless_application.h)
- `gui_application : public QApplication, public main_application`. — [rpcs3/rpcs3qt/gui_application.h#L37](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/gui_application.h#L37)
- `create_application()` in `rpcs3/rpcs3.cpp` returns `new headless_application` when `--headless` is passed, else `new gui_application`. — [rpcs3/rpcs3.cpp#L443-L532](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L443-L532)
- `main_application::InitializeEmulator` does:
  1. `Emu.SetHasGui()`, `Emu.SetHeadless()`, `Emu.SetUsr()`, `Emu.Init()`;
  2. logs `utils::get_firmware_version()`.
  - [rpcs3/main_application.cpp#L95-L108](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/main_application.cpp#L95-L108)
- The `main_application` constructor probes the renderers through `render_creator` (Qt) and calls `Emu.SetDefaultRenderer(video_renderer::vulkan)` when a Vulkan adapter exists. — [rpcs3/main_application.cpp#L66-L93](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/main_application.cpp#L66-L93)

**The callback struct**
- `struct emu_callbacks` is declared in `rpcs3/Emu/emu_callbacks.h` with `extern emu_callbacks g_emu_callbacks;` (line 68). It has 50+ `std::function` members, including:
  - threading and lifecycle: `call_from_main_thread`, `on_run`, `on_pause`, `on_resume`, `on_stop`, `on_ready`, `on_missing_fw`, `on_emulation_stop_no_response`, `on_save_state_progress`, `enable_disc_eject`, `enable_disc_insert`, `try_to_quit`, `handle_taskbar_progress`;
  - input: `init_kb_handler`, `init_mouse_handler`, `init_pad_handler(title_id)`;
  - settings: `update_emu_settings`, `save_emu_settings`;
  - video: `close_gs_frame`, `get_gs_frame`, `init_gs_render`;
  - devices and audio: `get_camera_handler`, `get_music_handler`, `get_audio`, `get_audio_enumerator`;
  - dialogs: `get_msg_dialog`, `get_osk_dialog`, `get_save_dialog`, `get_sendmessage_dialog`, `get_recvmessage_dialog`, `get_trophy_notification_dialog`;
  - strings: `get_localized_string`, `get_localized_u32string`, `get_localized_setting`;
  - media and paths: `get_photo_path`, `play_sound`, `get_image_info`, `get_scaled_image`, `resolve_path`, `resolve_path_may_not_exist` (both with trivial defaults), `get_font_dirs`, `on_install_pkgs`;
  - misc: `add_breakpoint`, `display_sleep_control_supported`, `enable_display_sleep`, `check_microphone_permissions`, `make_video_source`, `enable_gamemode`, `get_database_config`.
  - [rpcs3/Emu/emu_callbacks.h#L15-L68](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/emu_callbacks.h#L15-L68)
  - `grep` finds 192 `g_emu_callbacks.` uses in `.cpp` files across the tree (my count).
- `main_application::create_callbacks()` fills the shared callbacks — [rpcs3/main_application.cpp#L137-L500](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/main_application.cpp#L137-L500):
  - `init_kb_handler` and `init_mouse_handler` create Qt-based `basic_keyboard_handler` / `basic_mouse_handler` (`moveToThread`, `SetTargetWindow(QWindow*)`) or `raw_mouse_handler`;
  - `init_pad_handler` does `g_fxo->init<named_thread<pad_thread>>(get_thread(), m_game_window, title_id)`;
  - `get_audio` uses a switch over Null / XAudio2 (`_WIN32`) / Cubeb / FAudio, with a fallback to Null;
  - `get_image_info` and `get_scaled_image` use `QImageReader`;
  - `resolve_path` uses `QFileInfo`;
  - `get_font_dirs` uses `QStandardPaths::FontsLocation`;
  - `on_install_pkgs` calls `rpcs3::utils::install_pkg`;
  - `get_photo_path` uses `QDateTime`;
  - `get_database_config` uses Qt `config_database`.
- `headless_application::create_callbacks()` — [rpcs3/headless_application.cpp#L45-L182](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/headless_application.cpp#L45-L182):
  - provides `try_to_quit` and `call_from_main_thread` through a Qt signal;
  - `init_gs_render` builds only `NullGSRender` and throws for OpenGL or Vulkan: "Headless mode can only be used with the null video renderer";
  - `get_gs_frame` returns an empty pointer;
  - the dialog getters return `nullptr`;
  - the localized-string getters return empty strings;
  - the rest are no-ops.
- `gui_application` sets the remaining GUI callbacks (`get_msg_dialog` returns `msg_dialog_frame`, `get_save_dialog` returns `save_data_dialog`, and so on). `init_gs_render` there constructs `GLGSRender` (not Apple) or `VKGSRender` (`HAVE_VULKAN`). — [rpcs3/rpcs3qt/gui_application.cpp#L782-L874](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/gui_application.cpp#L782-L874)

**What a game window must implement**
- `class GSFrameBase` declares these pure virtuals:
  - `close`, `reset`, `shown`, `hide`, `show`, `toggle_fullscreen`;
  - `delete_context`, `make_context`, `set_current`, `flip`;
  - `client_width`, `client_height`, `client_display_rate`, `has_alpha`;
  - `display_handle_t handle() const`;
  - `can_consume_frame`, `present_frame`, `take_screenshot`, `update_title`.
  - [rpcs3/Emu/RSX/GSFrameBase.h](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/GSFrameBase.h)
- `display_handle_t` is `HWND` on `_WIN32` and `ANativeWindow*` on Android. — [rpcs3/Emu/RSX/display.h](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/display.h)
- The Qt `gs_frame::handle()` returns `reinterpret_cast<HWND>(winId())` and uses `GetClientRect(HWND)` for its size. — [rpcs3/rpcs3qt/gs_frame.cpp#L737-L800](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/gs_frame.cpp#L737-L800)

**Android groundwork already upstream**
- `fs::get_executable_path`, `get_config_dir` and `get_cache_dir` return `g_android_executable_dir` / `g_android_config_dir` / `g_android_cache_dir` globals under `#ifdef ANDROID`. — [Utilities/File.cpp#L18-L20](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/File.cpp#L18-L20), [#L2443-L2525](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/File.cpp#L2443-L2525)
- `pad_thread` drops the Qt `keyboard_pad_handler` under `#ifndef ANDROID`. — [rpcs3/Input/pad_thread.cpp#L18-L20](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/pad_thread.cpp#L18-L20), [#L157-L187](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/pad_thread.cpp#L157-L187)
- The `pad_thread` constructor takes `void* curthread, void* curwindow`, "instead of QThread * and QWindow * because of include in emucore". — [rpcs3/Input/pad_thread.h#L21](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/pad_thread.h#L21)
- The Android port itself is a separate downstream project ("RPCS3-Android") by RPCS3 co-founder DH, released as alphas in 2025. — [Time Extension](https://www.timeextension.com/news/2025/03/the-co-creator-of-the-ps3-emulator-rpcs3-is-working-on-an-android-port)

**Native overlays (Qt-free)**
- `rsx::thread` creates the `rsx::overlays::display_manager` only `if (g_cfg.misc.use_native_interface && (renderer == opengl || renderer == vulkan))`. The setting "Use native user interface" defaults to true. — [rpcs3/Emu/RSX/RSXThread.cpp#L858-L869](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/RSXThread.cpp#L858-L869), [rpcs3/Emu/system_config.h#L389](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/system_config.h#L389)
- `cellMsgDialog` uses `manager->create<rsx::overlays::message_dialog>()` when the manager exists, and calls `g_emu_callbacks.get_msg_dialog()` otherwise. — [rpcs3/Emu/Cell/Modules/cellMsgDialog.cpp#L79](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/Modules/cellMsgDialog.cpp#L79), [#L177-L191](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/Modules/cellMsgDialog.cpp#L177-L191)
- The OSK follows the same pattern. — [rpcs3/Emu/Cell/Modules/cellOskDialog.cpp#L125-L145](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/Modules/cellOskDialog.cpp#L125-L145)
- `rpcs3/Emu/RSX/Overlays/` (about 20.1k LOC) contains: message dialog, OSK, save dialog, user list, trophy notification and list, home menu (several pages, including savestates and settings), friends list, send and receive message, perf metrics, debug overlay, shader-compile notification, media list, and **BigPicture/** (game grid, details and main menu). — [rpcs3/Emu/RSX/Overlays](https://github.com/RPCS3/rpcs3/tree/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays)
- Big Picture Mode is a full-screen game launcher built on the RSX overlays. `Emulator::BootBigPictureMode()`:
  1. `vm::init()`;
  2. reserves a small main map;
  3. `init_gs_render(nullptr)`, `init_kb_handler`, `init_mouse_handler`, `init_pad_handler("")`;
  4. runs `rsx::overlays::open_big_picture_mode()` on a named thread. The code comment reads "No PS3 executable is loaded. GSRender/pad_thread only exist to host the Big Picture Mode overlay".
  - [rpcs3/Emu/System.cpp#L976-L1040](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/System.cpp#L976-L1040)
  - It was added in PR #19165 "Add Big Picture Mode using RSX overlay UI" (closed 2026-08-25), followed by #19318, #19334, #19358, #19362 and #19380, and `--big-picture` CLI #19509 (2026-09-16). — [PR #19165](https://github.com/RPCS3/rpcs3/pull/19165), [PR #19509](https://github.com/RPCS3/rpcs3/pull/19509)
  - Its header comments say `g_big_picture_mode_active` is "Checked by the Qt front-end so exiting the game re-enters Big Picture Mode". — [overlay_big_picture.h](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/BigPicture/overlay_big_picture.h)
- The overlay fonts call `g_emu_callbacks.get_font_dirs()` and also search `dev_flash/data/font/` and `dev_flash/data/font/SONY-CC/`. On Windows they fall back to `Arial.ttf`, `tahoma.ttf`, `seguisym.ttf`, `msyh.ttc` and so on. — [rpcs3/Emu/RSX/Overlays/overlay_fonts.cpp#L105-L160](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/overlay_fonts.cpp#L105-L160)
- The overlay text goes through `get_localized_string()`, which returns `g_emu_callbacks.get_localized_string(...)`. That callback is implemented only in the Qt `localized_emu` (383-line header of `tr()` strings); headless returns `{}`. — [rpcs3/Emu/localized_string.cpp#L5-L12](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/localized_string.cpp#L5-L12), [rpcs3/rpcs3qt/gui_application.cpp#L914-L924](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/gui_application.cpp#L914-L924)
- Qt dropped its UWP/WinRT backend in Qt 6; Qt 5.15 was the last version supporting native Qt UWP apps. — [Qt interest list, July 2020](https://lists.qt-project.org/pipermail/interest/2020-July/035500.html)

### Inferences
- A UWP frontend should be a third `main_application`-like class that is **not** derived from `QCoreApplication`. Steps:
  1. Copy the `headless_application::create_callbacks` shape.
  2. Replace the Qt-dependent pieces of `main_application::create_callbacks`: QImageReader with stb_image or WIC; QFileInfo with `std::filesystem`; QStandardPaths with LocalState/fonts; QDateTime with `std::chrono`; Qt kb/mouse with CoreWindow events or GameInput; `config_database` with libcurl or nothing.
  3. Supply a `GSFrameBase` implementation around a CoreWindow or SwapChainPanel.
- In practice `main_application.h/.cpp` must be split or `#ifdef`ed, because the base class pulls in `<QThread>` and `render_creator`.
- The cheapest UX path is to reuse the native overlays plus Big Picture Mode as the whole console UI:
  - set `use_native_interface=true`;
  - return `nullptr` from the dialog getters (the native overlay paths are used when `display_manager` exists);
  - boot into `Emu.BootBigPictureMode()`.
  - Then no Qt widget code is needed. The required work is a non-Qt `get_localized_string` table (port `rpcs3qt/localized_emu.h`) and a font directory containing Arial-like TTFs or the PS3 firmware fonts in dev_flash.
- The Android `#ifdef ANDROID` seams (paths, pad_thread without the keyboard handler, the CMake guards that skip Qt, OpenGL and OpenAL) are the closest template. A `UWP` / `WINAPI_FAMILY_APP` define could follow the same pattern.
- The Android Vulkan WSI shim upstream is a 24-line stub that uses `this->m_instance` inside a free `static` function and a `VkWin32SurfaceCreateInfoKHR` struct. This suggests the Android target is not compiled in upstream CI and needs downstream patches. — [rpcs3/Emu/RSX/VK/vkutils/swapchain_android.hpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/swapchain_android.hpp)

### Gaps
- I did not check whether `rpcs3_emu` compiles today with `ANDROID` defined and no Qt headers anywhere. `Emu/Memory/vm.h` includes `rpcs3qt/breakpoint_handler.h`, but only under `RPCS3_HAS_MEMORY_BREAKPOINTS`. I did not build anything (as instructed).
- The DH RPCS3-Android repository was not inspected; I could not confirm how much of its frontend is upstreamed.

---

## 2. Guest memory: reservation and mapping (`vm.cpp`, `util/vm_native.cpp`, `utils::shm`)

### Takeaway

RPCS3 reserves about **56 GiB of virtual address space** for the guest, plus 2 GiB for JIT and more for LLVM managers.

It relies on *fixed-address* file-mapping views. Each 4 GiB PS3 address space is mirrored twice from the same `shm` section (protected view `g_base_addr` and unprotected "sudo" view `g_sudo_addr`).

On Windows 10+ it uses `VirtualAlloc2` + `MEM_RESERVE_PLACEHOLDER` and `MapViewOfFile3` + `MEM_REPLACE_PLACEHOLDER`, loaded dynamically from KernelBase.dll. There is a racy fallback using `VirtualFree` and `MapViewOfFileEx` when those are missing.

Several shm objects are backed by sparse **files** in the temp dir (`rpcs3_vm_sparse.tmp`, 32 GiB "hook" region), using `FSCTL_SET_SPARSE`.

Every Win32 memory call needs swapping:

| Desktop API | UWP equivalent | Note |
|---|---|---|
| `VirtualAlloc` | `VirtualAllocFromApp` / `VirtualAlloc2FromApp` | |
| `VirtualAlloc2` | `VirtualAlloc2FromApp` | |
| `MapViewOfFile3` | `MapViewOfFile3FromApp` | |
| `MapViewOfFileEx` | `MapViewOfFile3FromApp` / `MapViewOfFileFromApp` | |
| `CreateFileMappingW` | `CreateFileMappingFromApp` | |
| `VirtualProtect` | `VirtualProtectFromApp` | **does not allow RWX** |
| `VirtualLock` | none | not in the UWP list |

### Cited findings

**Reservations**
- `vm::memory_reserve_4GiB(addr, size, is_memory_mapping)` probes upward in 4 GiB steps from a hint until `utils::memory_reserve` succeeds, below `0x8000'0000'0000`; otherwise it throws "Failed to reserve vm memory". — [rpcs3/Emu/Memory/vm.cpp#L29-L40](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Memory/vm.cpp#L29-L40)
- Static-initialized globals — [rpcs3/Emu/Memory/vm.cpp#L42-L64](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Memory/vm.cpp#L42-L64):
  - `g_base_addr = memory_reserve_4GiB(0x2'0000'0000, 0x2'0000'0000, true)`: 8 GiB, placeholder (mapping) region;
  - `g_sudo_addr = g_base_addr + 4 GiB`;
  - `g_exec_addr = memory_reserve_4GiB(g_sudo_addr, 0x300000000)`: 12 GiB, the PPU function-pointer table plus a segment table at `g_exec_addr_seg_offset`;
  - `g_hook_addr = memory_reserve_4GiB(g_exec_addr, 0x800000000)`: 32 GiB;
  - `g_stat_addr = memory_reserve_4GiB(g_hook_addr)`: 4 GiB;
  - `g_free_addr = g_stat_addr + 4 GiB`;
  - plus static arrays `g_reservations[65536/128*64]`, `g_shmem[65536]` and `g_pages[0x100000000/4096]`.
- `vm::init()` — [rpcs3/Emu/Memory/vm.cpp#L2309-L2349](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Memory/vm.cpp#L2309-L2349):
  - logs the ranges;
  - creates the fixed guest blocks: video `0xC0000000` (256 MiB, 64k pages, preallocated, i.e. the RSX local memory window), stack `0xD0000000` (256 MiB, 4k, guarded), SPU reserved `0xE0000000` (512 MiB);
  - on `_WIN32` releases `g_hook_addr` and maps `static utils::shm s_hook{0x800000000, ""}` there copy-on-write.
- Page mapping in `_page_map`:
  - `map_critical(g_base_addr + addr, prot)` and `map_critical(g_sudo_addr + addr, rw)`;
  - `utils::memory_protect`, `utils::memory_lock` on both views;
  - commits `g_exec_addr + addr*2` (size*2) and `g_stat_addr`.
  - [rpcs3/Emu/Memory/vm.cpp#L840-L858](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Memory/vm.cpp#L840-L858)
  - Whole-allocated blocks use `std::make_shared<utils::shm>(size, fmt::format("_block_x%08x", addr))` (file-backed). — [rpcs3/Emu/Memory/vm.cpp#L1366-L1372](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Memory/vm.cpp#L1366-L1372)
- lv2 shared memory (`sys_mmapper`) creates `utils::shm(size, 1 /* shareable flag */)`. — [rpcs3/Emu/Cell/lv2/sys_mmapper.cpp#L760-L840](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/lv2/sys_mmapper.cpp#L760-L840)
- SPU local storage is a `std::make_shared<utils::shm>(SPU_LS_SIZE)` mapped through `spu_thread::map_ls`. — [rpcs3/Emu/Cell/SPUThread.cpp#L1752](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/SPUThread.cpp#L1752), [#L1821](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/SPUThread.cpp#L1821)

**`util/vm_native.cpp` Win32 API inventory**

- **Dynamic imports**
  - `DYNAMIC_IMPORT("KernelBase.dll", VirtualAlloc2 / MapViewOfFile3 / UnmapViewOfFile2)`;
  - `has_win10_memory_mapping_api()` is true only if all three resolve. — [rpcs3/util/vm_native.cpp#L88-L96](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L88-L96)
- **Protection mapping**
  - `protection::wx` maps to `PAGE_EXECUTE_READWRITE` and `rx` to `PAGE_EXECUTE_READ`. — [rpcs3/util/vm_native.cpp#L200-L222](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L200-L222)
- **Core calls** — [rpcs3/util/vm_native.cpp#L225-L470](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L225-L470)

  | Function | Win32 calls |
  |---|---|
  | `memory_reserve` | `VirtualAlloc2(nullptr, addr, size, MEM_RESERVE \| MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS)` for mapping regions (registered in `s_is_mapping`); else `VirtualAlloc(addr, size, MEM_RESERVE, PAGE_NOACCESS)` |
  | `memory_commit` | `VirtualAlloc(MEM_COMMIT, prot)` |
  | `memory_decommit` | `VirtualFree(MEM_DECOMMIT)` |
  | `memory_reset` | decommit, then commit |
  | `memory_release` | `VirtualFree(MEM_RELEASE)` |
  | `memory_protect` | `VirtualProtect`, retried per 64 KiB block |
  | `memory_lock` | `VirtualLock` |
  | `memory_map_fd` | `// TODO return nullptr` on Windows |

- **`shm::shm(size, flags)`**
  - `CreateFileMappingW(INVALID_HANDLE_VALUE, …, PAGE_EXECUTE_READWRITE, …)`: a pagefile-backed section with execute rights. — [rpcs3/util/vm_native.cpp#L489-L496](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L489-L496)
- **`shm::shm(size, storage)`** — [rpcs3/util/vm_native.cpp#L533-L705](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L533-L705)
  - creates `fs::get_temp_dir() + "rpcs3_vm_sparse.tmp"` (or `_block_x…`), falling back to `fs::get_cache_dir()`;
  - sets `FILE_ATTRIBUTE_TEMPORARY` through `SetFileInformationByHandle`;
  - `DeviceIoControl(FSCTL_SET_SPARSE)` and `FSCTL_QUERY_ALLOCATED_RANGES`;
  - grows the file in 256 MiB steps ("bug workaround");
  - `CreateFileMappingW(fileHandle, PAGE_READWRITE)`;
  - on failure: `MessageBoxW(..."Can't find a filesystem with sparse file support (NTFS)")`.
- **`shm::map`**
  - `MapViewOfFileEx(handle, FILE_MAP_WRITE [|FILE_MAP_EXECUTE] [|FILE_MAP_COPY], …, ptr)`, then `VirtualProtect`. — [rpcs3/util/vm_native.cpp#L818-L857](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L818-L857)
- **`shm::map_critical`** — [rpcs3/util/vm_native.cpp#L906-L990](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L906-L990)
  1. `VirtualQuery` checks for `MEM_RESERVE`.
  2. For placeholder regions: split with `VirtualFree(MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)`, then `MapViewOfFile3(…, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE / PAGE_EXECUTE_READWRITE / WRITECOPY)`, then `VirtualProtect`.
  3. **Fallback** (no placeholder API): `VirtualFree(AllocationBase, MEM_RELEASE)`, re-`VirtualAlloc` the head and tail, then `MapViewOfFileEx` into the hole. This has a race window.
- **`shm::unmap_critical`** — [rpcs3/util/vm_native.cpp#L1041-L1090](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/vm_native.cpp#L1041-L1090)
  - `UnmapViewOfFile2(MEM_PRESERVE_PLACEHOLDER)`;
  - `VirtualFree(MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)`;
  - or the `UnmapViewOfFile` + `VirtualAlloc` re-reserve path.
- **Other files**
  - `rpcs3.cpp`: `SetProcessWorkingSetSize(GetCurrentProcess(), 0x80000000, 0xC0000000)` ("2-3 GiB"), fatal if it fails. — [rpcs3/rpcs3.cpp#L646-L650](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L646-L650)
  - `VKDMA.cpp`: `VirtualQuery` on guest memory before importing it as a Vulkan host pointer (NVIDIA only). — [rpcs3/Emu/RSX/VK/VKDMA.cpp#L287-L315](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/VKDMA.cpp#L287-L315)

**UWP availability (Microsoft docs)**
- In the UWP list:
  - `VirtualAllocFromApp`, `VirtualProtectFromApp`, `VirtualFree`, `VirtualQuery`, `MapViewOfFileFromApp`, `UnmapViewOfFile`, `UnmapViewOfFileEx`, `CreateFileMappingFromApp`;
  - `UnmapViewOfFile2` (since 10.0.17134);
  - `MapViewOfFile3FromApp` and `VirtualAlloc2FromApp` ("Introduced into api-ms-win-core-memory-l1-1-6.dll in 10.0.17134").
- **Not** in the UWP list: `VirtualAlloc`, `VirtualAlloc2`, `VirtualProtect`, `VirtualLock`, `MapViewOfFile`, `MapViewOfFileEx`, `MapViewOfFile3`, `CreateFileMappingW`, `SetProcessWorkingSetSize`.
- [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)
- Conflict: the per-function reference pages for `VirtualAlloc2FromApp` and `MapViewOfFile3FromApp` list "Windows 10 [desktop apps only]" in their requirements headers, which contradicts the WindowsApp.lib list above. — [nf-memoryapi-virtualalloc2fromapp.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-virtualalloc2fromapp.md), [nf-memoryapi-mapviewoffile3fromapp.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-mapviewoffile3fromapp.md)
- `VirtualAlloc2FromApp` documents `MEM_RESERVE_PLACEHOLDER`: "To create a placeholder, call … with `MEM_RESERVE | MEM_RESERVE_PLACEHOLDER` and PageProtection set to PAGE_NOACCESS". — [nf-memoryapi-virtualalloc2fromapp.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-virtualalloc2fromapp.md)
- `MapViewOfFile3FromApp` documents `MEM_REPLACE_PLACEHOLDER` and says "You can only successfully request executable protection if your app has the **codeGeneration** capability". — [nf-memoryapi-mapviewoffile3fromapp.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-mapviewoffile3fromapp.md)
- `CreateFileMappingFromApp` has the same `codeGeneration` note for executable protection. — [nf-memoryapi-createfilemappingfromapp.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-createfilemappingfromapp.md)

### Inferences
- The placeholder design maps almost 1:1 onto `VirtualAlloc2FromApp`, `MapViewOfFile3FromApp`, `UnmapViewOfFile2` and `VirtualFree` (all in WindowsApp.lib from SDK 17134). The `DYNAMIC_IMPORT("KernelBase.dll", …)` lookup must become direct linking, because `GetModuleHandleA` is not in the UWP list (see §4).
- These calls need conversion:
  - `CreateFileMappingW` → `CreateFileMappingFromApp`;
  - `MapViewOfFileEx` → `MapViewOfFile3FromApp` (with an address);
  - `VirtualProtect` → `VirtualProtectFromApp`;
  - `VirtualAlloc(MEM_RESERVE)` → `VirtualAlloc2FromApp`;
  - `VirtualLock` → no-op;
  - `SetProcessWorkingSetSize` → remove.
- The pagefile-backed `shm` is created with `PAGE_EXECUTE_READWRITE`. Under UWP that requires `codeGeneration`, and RWX may still be refused for *views* (VirtualProtectFromApp forbids RWX). The section can probably be created `PAGE_READWRITE`, because guest data views never need execute.
- The sparse-file shm (32 GiB `s_hook` and `_block_x…`) uses temp-dir files, `FSCTL_SET_SPARSE` and `MessageBoxW`. On UWP the temp dir would resolve to the package TempState. Whether the console's storage supports sparse files was not verifiable. A port could switch these to pagefile-backed `CreateFileMappingFromApp(INVALID_HANDLE_VALUE, …, SEC_RESERVE)` sections.
- The ~56 GiB guest reservation plus 2 GiB JIT is *virtual* only. The commit charge is what matters against the Xbox app memory budget (covered by other researchers).

### Gaps
- I could not test whether Xbox Dev Mode allows 56+ GiB of reserved VA per process, or sparse files in LocalState/TempState.
- I could not test whether `MapViewOfFile3FromApp` with `PAGE_EXECUTE_*` works there without the restricted `codeGeneration` capability.

---

## 3. JIT and executable memory (PPU LLVM, SPU LLVM, SPU ASMJIT, interpreter generators)

### Takeaway

All recompilers use **LLVM MCJIT** (legacy `ExecutionEngine` + `RTDyldMemoryManager`), not ORC. RPCS3 builds against **LLVM 22.1.x**; the submodule is at LLVM 22.1.8, and Windows uses a prebuilt `llvmlibs_mt.7z` 22.1.8.

Executable memory is committed **RWX** (`PAGE_EXECUTE_READWRITE`) on Windows. Only macOS/arm64 toggles W^X, through `pthread_jit_write_protect_np`.

asmjit-generated trampolines (`ppu_gateway`, `spu_runtime::g_gateway`, `g_escape`, `tr_*`, `spu_shufb`) and the x64 **PPU interpreter's per-instruction functions** are all emitted by `build_function_asm` into a 16 MiB RWX global runtime. Many are built during static initialization. There is therefore **no JIT-free mode**: even the "static" interpreter executes generated code.

UWP's `VirtualProtectFromApp` and `VirtualAllocFromApp` forbid RWX outright. A port needs either:
- W^X flipping (RW → RX after emission), or
- dual-mapped RW/RX views of one section (`CreateFileMappingFromApp` + two `MapViewOfFile3FromApp` views).

Both need the `codeGeneration` capability.

### Cited findings

**LLVM integration**
- LLVM linkage — [3rdparty/llvm/CMakeLists.txt](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty/llvm/CMakeLists.txt):
  - `find_package(LLVM 22.1 CONFIG)` when built from the submodule;
  - prebuilt or system LLVM must be ≥ 18;
  - components are `${LLVM_TARGETS_TO_BUILD} Core ExecutionEngine MCJIT Passes` (+ IntelJITEvents on Windows x86 / Linux, PerfJITEvents on Linux).
- The submodule `3rdparty/llvm/llvm` is pinned at llvm-project `ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`, whose `LLVMVersion.cmake` says 22.1.8. — [LLVMVersion.cmake @ca7933e](https://github.com/llvm/llvm-project/blob/ca7933e47d3a3451d81e72ac174dcb5aa28b59d1/cmake/Modules/LLVMVersion.cmake)
- BUILDING.md points Windows builders to prebuilt `https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win-22.1.8/llvmlibs_mt.7z`, or to the `llvm_build` / `llvm_build_clang_cl` vcxproj. — [BUILDING.md#L128-L139](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/BUILDING.md#L128-L139)
- `JITLLVM.cpp` calls `LLVMLinkInMCJIT()` (line 102). It builds `llvm::EngineBuilder(...).setEngineKind(JIT).setMCJITMemoryManager(mem).setOptLevel(Aggressive).setCodeModel(flags & 0x2 ? Large : Small).setRelocationModel(PIC_)`. It registers `createIntelJITEventListener()` and a `JITAnnouncer`. — [Utilities/JITLLVM.cpp#L102](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L102), [#L766-L800](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L766-L800)
- I found no ORC/LLJIT includes anywhere in `rpcs3/` or `Utilities/` (grep).
- Target triples — [Utilities/JITLLVM.cpp#L634-L660](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L634-L660):
  - `jit_compiler::triple1()` is the process triple on Windows (COFF);
  - `triple2()` on `_WIN32 && ARCH_X64` is `"x86_64-unknown-linux-gnu"`, an ELF object even on Windows; it is used with `MemoryManager2`.
- `ObjectCache` writes compiled objects to disk for PPU/SPU caches. — [Utilities/JITLLVM.cpp#L442-L460](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L442-L460)

**Memory managers**
- **`MemoryManager1`** (PPU / linked modules) — [Utilities/JITLLVM.cpp#L222-L378](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L222-L378):
  - reserves `utils::memory_reserve(c_max_size * 3, true)`, where `c_max_size = 256 MiB`;
  - commits in 2 MiB units;
  - `allocateCodeSection` uses `utils::protection::wx`; data sections use `rw`;
  - the destructor only decommits ("Hack: don't release to prevent reuse of address space");
  - `finalizeMemory` returns false;
  - **`registerEHFrames` / `deregisterEHFrames` are empty**.
- **`MemoryManager2`** — [Utilities/JITLLVM.cpp#L380-L440](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L380-L440):
  - `allocateCodeSection` → `jit_runtime::alloc(size, align, true)`; data → `jit_runtime::alloc(..., false)`;
  - EH frame registration is also empty.

**`jit_runtime` and asmjit**
- `JITASM.cpp` `get_jit_memory()` — [Utilities/JITASM.cpp#L12-L15](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITASM.cpp#L12-L15), [#L120-L133](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITASM.cpp#L120-L133):
  - reserves 2 GiB (`memory_reserve(0x80000000, true)`);
  - on Linux only (`CAN_OVERCOMMIT`) it commits everything up front and protects the first 1 GiB `wx`.
- `add_jit_memory<s_code_pos, 0x0, protection::wx>` (code: first 1 GiB) and `<s_data_pos, 0x40000000, protection::rw>` (data: second 1 GiB) commit on demand in 2 MiB steps with `memory_commit(..., Prot)`. That means `VirtualAlloc(MEM_COMMIT, PAGE_EXECUTE_READWRITE)` on Windows. — [Utilities/JITASM.cpp#L150-L215](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITASM.cpp#L150-L215), [#L269-L296](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITASM.cpp#L269-L296)
- `jit_runtime::finalize()` resets the 2 GiB region and calls `pthread_jit_write_protect_np` only on Apple. — [Utilities/JITASM.cpp#L312-L333](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITASM.cpp#L312-L333)
- `asmjit::get_global_runtime()` is a 16 MiB `custom_runtime` that does `memory_reserve(size, true)` then `memory_commit(m_pos, size, utils::protection::wx)`: RWX for the whole lifetime. — [Utilities/JITASM.cpp#L342-L382](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITASM.cpp#L342-L382)
- `inline_runtime::~inline_runtime()` protects to `rx` after writing. This is the only place with W^X-like behavior. — [Utilities/JITASM.cpp#L388-L403](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITASM.cpp#L388-L403)
- `build_function_asm(name, builder, custom_runtime = nullptr, …)` defaults to `get_global_runtime()`. — [Utilities/JIT.h#L461-L467](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JIT.h#L461-L467)
- Users of `build_function_asm` include:
  - `ppu_gateway` and `ppu_escape` in PPUThread.cpp — [#L185](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/PPUThread.cpp#L185), [#L412](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/PPUThread.cpp#L412);
  - `spu_runtime::g_gateway`, `g_escape`, `g_tail_escape`, `tr_dispatch`, `tr_branch`, `tr_interpreter` and `tr_all` in SPUCommonRecompiler.cpp — [#L224-L573](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/SPUCommonRecompiler.cpp#L224-L573);
  - `optimized_shufb` in SPUInterpreter.cpp — [#L1992](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/SPUInterpreter.cpp#L1992);
  - PPUFunction.cpp trampolines — [#L1979-L1993](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/PPUFunction.cpp#L1979-L1993).
- The x64 PPU interpreter's `RETURN(...)` macro builds each instruction handler with `build_function_asm<ppu_intrp_func_t, asmjit::ppu_builder>("ppu_"s + __func__, …)`. — [rpcs3/Emu/Cell/PPUInterpreter.cpp#L120-L135](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/PPUInterpreter.cpp#L120-L135)
- Decoder options: `ppu_decoder_type { _static, llvm }` (default llvm) and `spu_decoder_type { _static, dynamic, asmjit, llvm }` (default llvm). — [rpcs3/Emu/system_config_types.h#L3-L15](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/system_config_types.h#L3-L15), [rpcs3/Emu/system_config.h#L17-L27](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/system_config.h#L17-L27)
- Compile flags: `JITLLVM.cpp` and `JITASM.cpp` are compiled with `/GR-` (no RTTI); `yaml.cpp` with `/EHsc`. — [rpcs3/Emu/CMakeLists.txt#L101-L110](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L101-L110)

**LLVM's own Windows code** (submodule)
- `lib/Support/Windows/Memory.inc` uses `::VirtualAlloc`, `VirtualProtect` (incl. `PAGE_EXECUTE_READWRITE`) and `FlushInstructionCache`. — [Memory.inc @ca7933e](https://github.com/llvm/llvm-project/blob/ca7933e47d3a3451d81e72ac174dcb5aa28b59d1/llvm/lib/Support/Windows/Memory.inc)
- `DynamicLibrary.inc` uses `LoadLibraryW` and `EnumProcessModulesEx`. — [DynamicLibrary.inc @ca7933e](https://github.com/llvm/llvm-project/blob/ca7933e47d3a3451d81e72ac174dcb5aa28b59d1/llvm/lib/Support/Windows/DynamicLibrary.inc)
- RPCS3's memory managers fall back to `RTDyldMemoryManager::getSymbolAddress(name)`, which ends in process-wide symbol search. — [Utilities/JITLLVM.cpp#L268-L272](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L268-L272)

**UWP rules**
- `VirtualProtectFromApp`: "The following constants generate an error: PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY. The following constants are allowed only for apps that have the codeGeneration capability: PAGE_EXECUTE, PAGE_EXECUTE_READ … does not allow you to set both write and execute permissions at the same time." — [nf-memoryapi-virtualprotectfromapp.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-virtualprotectfromapp.md)
- `VirtualAllocFromApp` rejects every `PAGE_EXECUTE*` value: "does not allow the creation of executable pages". — [nf-memoryapi-virtualallocfromapp.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/memoryapi/nf-memoryapi-virtualallocfromapp.md)
- `FlushInstructionCache` is in the UWP list. — [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)

### Inferences
- Every `utils::protection::wx` site must change: `memory_commit(..., wx)` in the global runtime, `jit_runtime` code region, `MemoryManager1` code sections, and the `vm_native` `PAGE_EXECUTE_READWRITE` mapping. Two designs:
  - **(a) Dual mapping.** One `CreateFileMappingFromApp(PAGE_EXECUTE_READWRITE)` section with an RW view for writers and an RX view for execution. MCJIT must write at the RW address but resolve relocations for the RX address. `RTDyldMemoryManager` does not separate the load address from the write address. `notifyObjectLoaded` / `mapSectionAddress` can, but that requires `RuntimeDyld` remapping.
  - **(b) Flip after emission.** Commit RW, flip to RX in `finalizeMemory` and after `build_function_asm`, using `VirtualProtectFromApp(PAGE_EXECUTE_READ)`.
- (b) is simpler but breaks code that writes into executable pages later. One example is SPU `spu_runtime` dispatcher tables that are patched in place (`tr_*`); this needs to be audited.
- Because generated stubs run at static-init time, a missing `codeGeneration` capability would crash before `main()`.
- SPU LLVM uses an ELF triple on Windows and no unwind registration. That avoids `RtlAddFunctionTable`, which is not in the UWP list anyway. C++ exceptions must never propagate through JIT frames (RPCS3 already escapes through `ppu_escape` / `spu_runtime::g_escape`).
- LLVM itself must be built for the UWP partition, or its desktop-only calls (`VirtualAlloc`, `LoadLibraryW`, `EnumProcessModulesEx`) avoided. RPCS3 only uses MCJIT + RuntimeDyld with custom memory managers, but symbol lookup still goes through `sys::DynamicLibrary`.

### Gaps
- I could not confirm the Xbox Dev Mode policy for `codeGeneration` / RWX. Prior UWP emulators reportedly JIT on Xbox, but that is outside this inventory.
- I did not audit every SPU/PPU path that patches code after emission, such as `spu_runtime` relinking or the PPU `ppu_ref` table (the latter is a data table in `g_exec_addr`, not code).

---

## 4. Exceptions, signals, and undocumented ntdll use

### Takeaway

RPCS3's memory model depends on a **vectored exception handler** catching `EXCEPTION_ACCESS_VIOLATION`. The handler is installed at static-init time and covers:
- guest page faults and reservations;
- RSX texture-cache write tracking (`rsx::g_access_violation_handler`);
- RawSPU MMIO, emulated by decoding the faulting x86 instruction and writing CPU registers in the `CONTEXT`.

`AddVectoredExceptionHandler` is documented "desktop apps only" and is absent from the UWP API list. `SetUnhandledExceptionFilter` and `RtlLookupFunctionEntry` are present.

RPCS3 does **not** register unwind info for JIT code (no `RtlAddFunctionTable`).

It uses ntdll through `DYNAMIC_IMPORT` (which is `GetProcAddress(GetModuleHandleA(lib))`):
- `NtWaitForKeyedEvent`, `NtReleaseKeyedEvent`, `NtWaitForSingleObject`, `NtDelayExecution` (declared but unused), `NtWaitForAlertByThreadId`, `NtAlertThreadByThreadId`;
- `NtQueryTimerResolution`, `NtSetTimerResolution`, `RtlGetVersion`.

`GetModuleHandleA` / `GetModuleHandleW` are not in the UWP list.

### Cited findings

**The handler**
- `s_exception_handler_set` calls `AddVectoredExceptionHandler(1, exception_handler)` (or `FALSE` under ASAN) and `SetUnhandledExceptionFilter(exception_filter)`. A failure calls `report_fatal_error`. — [Utilities/Thread.cpp#L2504-L2520](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L2504-L2520)
- `exception_handler` — [Utilities/Thread.cpp#L2308-L2374](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L2308-L2374):
  - maps the faulting address through `vm::try_get_addr`, or through `g_exec_addr` / `g_exec_addr_seg_offset`;
  - calls `handle_access_violation(addr, is_writing, is_exec, pExp->ContextRecord)`;
  - returns `EXCEPTION_CONTINUE_EXECUTION`.
- `handle_access_violation` — [Utilities/Thread.cpp#L1551-L1640](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L1551-L1640):
  - calls `rsx::g_access_violation_handler` first;
  - then decodes the instruction at `RIP(context)` with `decode_x64_reg_op` and emulates RawSPU MMIO loads and stores;
  - uses `get_x64_reg_value` / `put_x64_reg_value` / `set_x64_cmp_flags` on the Win32 `CONTEXT` (`X64REG(context, reg) (&(&(context)->Rax)[reg])`).
  - Supporting code: [Utilities/Thread.cpp#L306](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L306), [#L877-L1040](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L877-L1040)
- The RSX thread installs `g_access_violation_handler = [this](u32 address, bool is_writing){ return on_access_violation(address, is_writing); }`. — [rpcs3/Emu/RSX/RSXThread.cpp#L840-L842](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/RSXThread.cpp#L840-L842)
- `exception_filter` (crash report) uses `EnumProcessModules`, `GetModuleInformation`, `GetModuleBaseNameA`, `RtlLookupFunctionEntry` and `utils::get_backtrace`. — [Utilities/Thread.cpp#L2376-L2502](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L2376-L2502)
  - `Utilities/stack_trace.cpp` uses dbghelp `SymInitialize` / `StackWalk64` / `RtlCaptureContext`, and `rpcs3_emu` links `pdh bcrypt dbghelp`. — [rpcs3/Emu/CMakeLists.txt#L180](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L180)
- No `RtlAddFunctionTable` / `RtlInstallFunctionTableCallback` use exists; the JIT `registerEHFrames` is empty. — [Utilities/JITLLVM.cpp#L370-L376](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/JITLLVM.cpp#L370-L376)

**ntdll and other dynamic imports**
- `Utilities/sync.h` declares `DYNAMIC_IMPORT("ntdll.dll", …)` for NtWaitForKeyedEvent, NtReleaseKeyedEvent, NtWaitForSingleObject, NtDelayExecution, NtWaitForAlertByThreadId and NtAlertThreadByThreadId. — [Utilities/sync.h#L24-L35](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/sync.h#L24-L35)
- `util/atomic.cpp` uses `NtWaitForAlertByThreadId` / `NtAlertThreadByThreadId` if present, else `NtWaitForKeyedEvent` / `NtReleaseKeyedEvent`. There is no third Windows fallback: the `USE_STD` condition_variable path is enabled only for `!_WIN32 && !linux && !apple`. — [rpcs3/util/atomic.cpp#L9-L13](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/atomic.cpp#L9-L13), [#L295-L350](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/atomic.cpp#L295-L350), [#L1110-L1200](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/atomic.cpp#L1110-L1200)
- `rpcs3.cpp` imports `NtQueryTimerResolution` / `NtSetTimerResolution` and sets maximum timer resolution when `--high-res-timer 1` is passed. — [rpcs3/rpcs3.cpp#L45-L46](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L45-L46), [#L1049-L1075](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L1049-L1075)
- `sysinfo.cpp` imports `RtlGetVersion`. — [rpcs3/util/sysinfo.cpp#L18](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/sysinfo.cpp#L18)
- `Thread.cpp` imports `SetThreadDescription` from Kernel32.dll. — [Utilities/Thread.cpp#L32](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L32)
- `utils::get_proc_address(lib, name)` is `GetProcAddress(GetModuleHandleA(lib), name)`. `dynamic_library::load` uses `LoadLibraryA/W`. — [rpcs3/util/dyn_lib.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/dyn_lib.cpp)

**UWP availability**
- `AddVectoredExceptionHandler`'s reference page says "Windows XP [desktop apps only]". — [nf-errhandlingapi-addvectoredexceptionhandler.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/errhandlingapi/nf-errhandlingapi-addvectoredexceptionhandler.md)
- The UWP list's errorhandling modules contain only `GetLastError`, `RaiseException`, `SetLastError`, `RaiseFailFastException`, `SetUnhandledExceptionFilter`, `Get/SetThreadErrorMode` and `SetErrorMode`. — [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)
- `GetProcAddress` is "[desktop apps | UWP apps]". `GetModuleHandleA` is "[desktop apps only]". `LoadPackagedLibrary` is UWP-available. — [nf-libloaderapi-getprocaddress.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/libloaderapi/nf-libloaderapi-getprocaddress.md), [nf-libloaderapi-getmodulehandlea.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/libloaderapi/nf-libloaderapi-getmodulehandlea.md), [nf-winbase-loadpackagedlibrary.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/winbase/nf-winbase-loadpackagedlibrary.md)
- In the UWP list: `RtlLookupFunctionEntry`, `RtlCaptureStackBackTrace`, `RtlUnwind`, `WaitOnAddress`, `WakeByAddressSingle`.
- Not in the UWP list: `RtlAddFunctionTable`, `RtlCaptureContext`, `StackWalk64`, `SymInitialize`, `EnumProcessModules`, `GetModuleHandleW/A`, `LoadLibraryW/A`.
- [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)

### Inferences
- **Critical path:** if VEH registration is not permitted in a UWP container, RPCS3's design does not work at all. Guest memory protection, reservations, RSX write tracking and RawSPU MMIO all fault on purpose. The port must confirm on hardware that `AddVectoredExceptionHandler` (kernel32/kernelbase export) can be resolved and called, or use a frame-based `__try/__except` around every guest-executing thread entry. SEH via `__try` is compiler-level and works in UWP. Frame-based SEH cannot catch faults inside JIT frames that lack unwind info, though, so a JIT-code `RtlAddFunctionTable` / `RtlInstallFunctionTableCallback` would then become necessary, and those are also not in the UWP list.
- Every `DYNAMIC_IMPORT("ntdll.dll", …)` must be replaced:
  - `atomic.cpp` wait/notify: switch to `WaitOnAddress` / `WakeByAddress*` (in the UWP list), or enable the existing `USE_STD` path;
  - `NtSetTimerResolution`: drop (no UWP equivalent; `timeBeginPeriod` is also desktop-only);
  - `RtlGetVersion`: hard-code or use WinRT `AnalyticsInfo`.
- Today `get_proc_address` would return null under UWP, because `GetModuleHandleA` is not linkable. `atomic.cpp` then calls `NtReleaseKeyedEvent` unguarded and crashes.

### Gaps
- I could not test VEH behavior or availability on Xbox Series X Dev Mode. The documentation gives only desktop-only status, not runtime behavior.

---

## 5. Threads, timing, and CPU feature detection

### Takeaway

Thread control uses standard Win32:
- `_beginthreadex` + `ResumeThread`;
- `SetThreadDescription` (dynamic);
- `SetThreadPriority`, `SetThreadAffinityMask`, `GetLogicalProcessorInformationEx`, `GetCurrentThreadStackLimits`;
- `QueryThreadCycleTime`, `GetThreadTimes`.

Most of these are in the UWP list. `QueryThreadCycleTime` is not.

Timing uses `QueryPerformanceCounter` plus RDTSC calibration (`utils::get_tsc_freq`), waitable timers in `sys_rsxaudio`, and ntdll timer resolution (desktop-only). CPU features come from `__cpuid` / `__cpuidex` / `_xgetbv` in `util/sysinfo.cpp`.

### Cited findings
- **Thread creation:** `_beginthreadex(nullptr, 0, entry_point, this, CREATE_SUSPENDED, nullptr)` then `ResumeThread`. — [Utilities/Thread.cpp#L2759-L2762](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L2759-L2762)
- **Naming:** `SetThreadDescriptionImport(GetCurrentThread(), …)`. — [Utilities/Thread.cpp#L2851-L2854](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L2851-L2854)
- **Priority:** `thread_ctrl::set_native_priority` uses `SetThreadPriority` with `ABOVE_NORMAL` or `BELOW_NORMAL`. — [Utilities/Thread.cpp#L3831-L3846](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L3831-L3846)
- **Affinity:**
  - `set_thread_affinity_mask` uses `SetThreadAffinityMask`. — [Utilities/Thread.cpp#L3887-L3897](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L3887-L3897)
  - It is called from `CPUThread.cpp` and from the RSX threads. — [rpcs3/Emu/CPU/CPUThread.cpp#L660](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CPU/CPUThread.cpp#L660)
  - The topology probe uses `GetLogicalProcessorInformationEx`. — [Utilities/Thread.cpp#L3575-L3600](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L3575-L3600)
  - The "Thread Scheduler Mode" config defaults to `os`. — [rpcs3/Emu/system_config.h#L25](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/system_config.h#L25)
- **Stack:** `GetCurrentThreadStackLimits`. — [Utilities/Thread.cpp#L3992-L3996](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/Thread.cpp#L3992-L3996)
- **CPU feature detection:** `utils::has_ssse3/sse41/avx/avx2/rtm/tsx_force_abort/mpx/avx512*/avx10/xop/clwb/invariant_tsc/fma3/fma4/erms/fsrm/waitx/waitpkg` …, `get_cpu_brand`. — [rpcs3/util/sysinfo.cpp#L83-L580](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/sysinfo.cpp#L83-L580)
  - The OS version comes from `RtlGetVersion`. — [rpcs3/util/sysinfo.cpp#L796-L840](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/sysinfo.cpp#L796-L840)
  - Memory via `GlobalMemoryStatusEx` and thread count via `GetNativeSystemInfo`. — [rpcs3/util/sysinfo.cpp#L1130-L1150](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/sysinfo.cpp#L1130-L1150)
- **TSC calibration:** RDTSC sampled against `QueryPerformanceCounter`. — [rpcs3/util/sysinfo.cpp#L960-L1125](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/sysinfo.cpp#L960-L1125)
  - `sys_time.cpp` uses QPC, `GetSystemTimeAsFileTime` and `GetTimeZoneInformation`.
- **Timers:** `sys_rsxaudio` uses `CreateWaitableTimer` / `SetWaitableTimerEx` / `CancelWaitableTimer`. — [rpcs3/Emu/Cell/lv2/sys_rsxaudio.cpp#L2000-L2090](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/lv2/sys_rsxaudio.cpp#L2000-L2090)
- **CPU stats:** `util/cpu_stats.cpp` uses `PdhOpenQuery` and `CreateToolhelp32Snapshot`. — [rpcs3/util/cpu_stats.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/cpu_stats.cpp)
- **UWP list membership:**
  - YES: `SetThreadAffinityMask` (api-ms-win-core-processtopology-obsolete), `SetThreadPriority`, `SetThreadDescription`, `SetThreadSelectedCpuSets`, `GetSystemCpuSetInformation`, `GetLogicalProcessorInformationEx`, `GetCurrentThreadStackLimits`, `CreateThread`, `ResumeThread`, `GetThreadTimes`, `QueryPerformanceCounter`, `CreateWaitableTimerW/ExW`, `SetWaitableTimerEx`, `CancelWaitableTimer`, `GlobalMemoryStatusEx`, `GetNativeSystemInfo`, `GetProcessAffinityMask`.
  - NO: `QueryThreadCycleTime`, `timeBeginPeriod`, `PdhOpenQuery`, `CreateToolhelp32Snapshot`.
  - [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)
  - `timeBeginPeriod` is "[desktop apps only]". — [nf-timeapi-timebeginperiod.md](https://github.com/MicrosoftDocs/sdk-api/blob/docs/sdk-api-src/content/timeapi/nf-timeapi-timebeginperiod.md)

### Inferences
- Threading needs only small edits:
  - `_beginthreadex` is a UCRT function, likely available in UWP CRT builds; `CreateThread` is a fallback;
  - remove `QueryThreadCycleTime` (use `GetThreadTimes`);
  - stub out the PDH / Toolhelp CPU stats;
  - drop the `NtSetTimerResolution` path.
- The Xbox Series X CPU is a Zen 2 derivative (AVX2, no AVX-512, no TSX). RPCS3's runtime detection already handles that combination, and LLVM's host-CPU detection would choose a znver2-class target. This is an inference; console CPU specifics are covered by other researchers.

### Gaps
- I did not check whether UCRT `_beginthreadex` is allowed under the UWP CRT, or how the console scheduler reacts to `SetThreadAffinityMask` (games normally use `SetThreadSelectedCpuSets`).

---

## 6. Renderers: backends, WSI, shader pipeline, Vulkan requirements, and D3D12 history

### Takeaway

Current backends are **Vulkan** (`rpcs3/Emu/RSX/VK`, about 33.1k LOC), **OpenGL** (`RSX/GL`, about 18.2k LOC; excluded on Android and Apple) and **Null**. There is no D3D backend.

The Vulkan instance requests API **1.2**. On Windows the WSI is `VK_KHR_win32_surface`, created from the Qt `gs_frame`'s `HWND`. There is also a CPU-copy GDI fallback (`swapchain_WIN32`: `GetDC`, `CreateDIBSection`, `BitBlt`).

Shaders flow RSX fragment/vertex microcode → GLSL (decompilers) → **glslang** → SPIR-V (Vulkan 1.2 target).

RPCS3 explicitly detects Microsoft's **Dozen** (Vulkan-on-D3D12) driver and warns that it is unsupported with a `MessageBox`.

The old D3D12 backend was removed in **PR #6834 "Drop d3d12 backend"** (kd-11, merged 2019-10-22, merge `00bc3fe`, commit `42ce18793fd25283207f0c1491690c66baf240f6`). Before that it had been "refactored out" because "it was holding back progress with shared code" (PR #6055, June 2019). At removal, `rpcs3/Emu/RSX/D3D12/` was **22 files / 7,091 lines**. At the same parent commit, VK was 17,858 lines and GL 11,755.

### Cited findings

**Build and selection**
- `if(NOT ANDROID AND NOT APPLE)` adds the GL sources; `if(TARGET 3rdparty_vulkan)` adds the VK sources. The Null renderer is always built. — [rpcs3/Emu/CMakeLists.txt#L509](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L509), [#L593-L683](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L593-L683)
- 3rdparty OpenGL is `WITHOUT_OPENGL=1` on Android and Apple. — [3rdparty/CMakeLists.txt#L98-L118](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty/CMakeLists.txt#L98-L118)
- Platform macros in `VulkanAPI.h`: `VK_USE_PLATFORM_WIN32_KHR` on `_WIN32`, `METAL_EXT`, `ANDROID_KHR`, `XLIB`, `WAYLAND`. `<vulkan/vulkan.h>` calls are linked directly (`Vulkan::Vulkan`), not through volk. — [rpcs3/Emu/RSX/VK/VulkanAPI.h#L1-L40](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/VulkanAPI.h#L1-L40), [3rdparty/CMakeLists.txt#L185-L200](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty/CMakeLists.txt#L185-L200)
- On Windows, `module_verifier.cpp` checks that `vulkan-1.dll` and the MSVC runtime DLLs are loaded, using `GetModuleHandle` / `GetModuleFileName`. — [rpcs3/module_verifier.cpp#L30-L55](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/module_verifier.cpp#L30-L55)

**WSI and swapchain**
- `make_WSI_surface` fills `VkWin32SurfaceCreateInfoKHR{ .hwnd = window_handle }` and calls `vkCreateWin32SurfaceKHR`. — [rpcs3/Emu/RSX/VK/vkutils/swapchain_win32.hpp#L93-L108](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/swapchain_win32.hpp#L93-L108)
- `swapchain_WIN32` is the CPU fallback: `GetClientRect`, `CreateCompatibleDC`, `CreateDIBSection`, `BitBlt`, `GetDC` / `ReleaseDC`. — [rpcs3/Emu/RSX/VK/vkutils/swapchain_win32.hpp#L1-L90](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/swapchain_win32.hpp#L1-L90)
- `swapchain_WSI` on `_WIN32` uses `MonitorFromWindow` and `VK_EXT_full_screen_exclusive` structures. — [rpcs3/Emu/RSX/VK/vkutils/swapchain.cpp#L150-L195](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/swapchain.cpp#L150-L195), [#L310-L320](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/swapchain.cpp#L310-L320)

**Instance and device requirements**
- The instance uses `app.apiVersion = VK_API_VERSION_1_2`. Extensions: `VK_KHR_surface`, `VK_KHR_win32_surface` (Windows), optional `VK_EXT_debug_report`, `VK_KHR_get_surface_capabilities2`, `VK_EXT_debug_utils` (renderdoc mode), and portability enumeration on Apple. — [rpcs3/Emu/RSX/VK/vkutils/instance.cpp#L100-L200](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/instance.cpp#L100-L200)
  - VMA also uses `vulkanApiVersion = VK_API_VERSION_1_2`. — [rpcs3/Emu/RSX/VK/vkutils/memory.cpp#L173](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/memory.cpp#L173)
- **Device extensions** — [rpcs3/Emu/RSX/VK/vkutils/device.cpp#L530-L596](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/device.cpp#L530-L596)
  - Required: `VK_KHR_swapchain`.
  - Optional (when supported): `VK_EXT_custom_border_color`, `VK_EXT_multi_draw`, `VK_EXT_conditional_rendering`, `VK_EXT_depth_range_unrestricted`, `VK_EXT_external_memory_host`, `VK_EXT_shader_stencil_export`, `VK_EXT_attachment_feedback_loop_layout`, `VK_KHR_fragment_shader_barycentric`, `VK_KHR_synchronization2`, `VK_EXT_device_fault`, `VK_EXT_provoking_vertex`, `VK_KHR_portability_subset`.
  - Probed in `physical_device::get_physical_device_features`: `VK_EXT_border_color_swizzle`, descriptor-indexing update-after-bind bits (core 1.2), shaderFloat16/Int8 (1.2 features), shaderFloat64. — [rpcs3/Emu/RSX/VK/vkutils/device.cpp#L40-L160](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/device.cpp#L40-L160)
- **Device features** — [rpcs3/Emu/RSX/VK/vkutils/device.cpp#L599-L760](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/device.cpp#L599-L760)
  - Requested core features: robustBufferAccess, fullDrawIndexUint32, independentBlend, logicOp, depthClamp, depthBounds, wideLines, largePoints, shaderFloat64, sampleRateShading, alphaToOne, shaderStorageImageMultisample, shaderStorageImageWriteWithoutFormat, occlusionQueryPrecise, shaderClipDistance, samplerAnisotropy, textureCompressionBC, shaderStorageBufferArrayDynamicIndexing.
  - Missing features are mostly *downgraded with log errors*, for example "Your GPU does not support shader clip distance. Graphics will not render correctly."
  - Vulkan 1.2 features enabled: `runtimeDescriptorArray`, `uniformBufferStandardLayout`, descriptor-indexing UAB bits, `shaderFloat16` / `shaderInt8`.
- **Dozen handling**
  - `driver_vendor::DOZEN` is detected via `VK_DRIVER_ID_MESA_DOZEN` or a GPU name containing "Microsoft Direct3D12". — [rpcs3/Emu/RSX/VK/vkutils/device.cpp#L292-L302](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/device.cpp#L292-L302), [#L382-L383](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/vkutils/device.cpp#L382-L383)
  - On Windows it shows `MessageBox(…"You're attempting to run rpcs3 on Microsoft's Dozen driver that emulates vulkan on top of Direct3D12. This driver is unsupported…")`. — [rpcs3/Emu/RSX/VK/VKHelpers.cpp#L132-L142](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/VKHelpers.cpp#L132-L142)
- **Passthrough DMA** (`VK_EXT_external_memory_host` on guest memory): `backend_config.supports_passthrough_dma = m_device->get_external_memory_host_support()`, with per-vendor disables. — [rpcs3/Emu/RSX/VK/VKGSRender.cpp#L705-L760](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/VKGSRender.cpp#L705-L760)

**Shader pipeline**
- `spirv::compile_glsl_to_spv(…, ::glsl::glsl_rules_vulkan)` is called from `VKProgramPipeline.cpp`. It uses glslang `EShClientVulkan`, `EShTargetVulkan_1_2` and `SPIRV/GlslangToSpv.h`. — [rpcs3/Emu/RSX/VK/VKProgramPipeline.cpp#L110](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/VK/VKProgramPipeline.cpp#L110), [rpcs3/Emu/RSX/Program/SPIRVCommon.cpp#L15-L145](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Program/SPIRVCommon.cpp#L15-L145)

**OpenGL on Windows**
- Uses `wglGetProcAddress` and `wglSwapIntervalEXT`, with the context from Qt's `gl_gs_frame`. — [rpcs3/Emu/RSX/GL/OpenGL.cpp#L32-L47](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/GL/OpenGL.cpp#L32-L47)

**D3D12 history**
- PR #6834 "Drop d3d12 backend" by kd-11, merged 2019-10-22 (merge `00bc3fe`); the reviewer note was "Builds with msvc + cmake without issues". — [PR #6834](https://github.com/RPCS3/rpcs3/pull/6834)
- Commit `42ce18793fd25283207f0c1491690c66baf240f6` changed 72 files: +11 / −92,125, including deletion of `3rdparty/minidx12` headers and libs. — [commit 42ce187](https://github.com/RPCS3/rpcs3/commit/42ce18793fd25283207f0c1491690c66baf240f6)
- Follow-up PR #6853 "Remove VS D3D12 Dependencies" (Megamouse). — [PR #6853](https://github.com/RPCS3/rpcs3/pull/6853)
- Earlier PR #6055 "rsx: Enable multisampling" (kd-11, June 2019): "Completely refactors out D3D12 as it was holding back progress with shared code." — [PR #6055](https://github.com/RPCS3/rpcs3/pull/6055)
- I measured the sizes by running `git ls-tree` / `git show` on parent `586fe11e2243d74b9d40f8fab6c60e53ccc4c73b` (blobless fetch):
  - D3D12 = 22 files, 7,091 lines;
  - VK = 17,858 lines;
  - GL = 11,755 lines.
  - Files included `D3D12GSRender.cpp/.h`, `D3D12PipelineState`, `D3D12Texture.cpp`, `D3D12FragmentProgramDecompiler` / `VertexProgramDecompiler` (HLSL), `D3D12RenderTargetSets`, `D3D12MemoryHelpers`, `D3D12Overlay.cpp` and `d3dx12.h`. — [commit 42ce187](https://github.com/RPCS3/rpcs3/commit/42ce18793fd25283207f0c1491690c66baf240f6)
- I found no RPCS3 blog post giving a fuller rationale. A web search returned only Dolphin's (different project's) D3D12 removal.

### Inferences
- **For Xbox UWP there is no native Vulkan driver.** The realistic options:
  - **(a) Vulkan through Mesa Dozen** (VK on D3D12), with a CoreWindow WSI. RPCS3 actively warns against Dozen, so expect missing features: `VK_EXT_external_memory_host` passthrough, custom border color, float64, logicOp, depthBounds, and so on. Those mostly degrade with log errors rather than hard-fail.
  - **(b) Resurrect or rewrite a D3D12 backend.** The 2019 code is 7k lines against a 2019 RSX common layer (now about 128k LOC of RSX). It is useful only as a reference; today's VK backend is 33k LOC.
  - **(c) Translate at the SPIR-V level** with spirv-cross → HLSL + a D3D12 backend.
- A UWP WSI needs a new `swapchain_*.hpp` (24 lines for Android show the minimal shape) and a `display_handle_t` variant for `IUnknown*` CoreWindow / SwapChainPanel. Win32-only bits must be removed: `MonitorFromWindow`, `VK_EXT_full_screen_exclusive`, `GetClientRect` / `GetDC` / `BitBlt` in the CPU fallback, `MessageBox`.
- Under UWP, `vulkan-1.dll` (Khronos loader) is not a system component. It would need to be packaged, or RPCS3 linked straight to the ICD's `vkGetInstanceProcAddr`.

### Gaps
- I did not check Dozen's current feature and extension coverage against RPCS3's list (other researchers cover Mesa/Dozen).
- I found no official statement beyond the PR #6055 sentence on why D3D12 was dropped.

---

## 7. Input: pad handlers and pad_thread

### Takeaway

Pad handlers, under `rpcs3/Input` and compiled into the Qt `rpcs3_ui` target:

| Group | Handlers |
|---|---|
| HID via hidapi | DS3, DS4, DualSense, Skateboard, PS Move |
| Windows | XInput (`#ifdef _WIN32`), MMJoystick (`joyGetPosEx`, `#ifdef _WIN32`) |
| Cross-platform | SDL3 (`HAVE_SDL3`) |
| Linux | evdev (`HAVE_LIBEVDEV`) |
| Qt-based | Keyboard, plus Qt keyboard and mouse handlers |
| Other | raw mouse (Win32 RawInput), mouse-gyro, wiimote |

`pad_thread::get_handler_for` instantiates them from `enum class pad_handler`.

For UWP:
- **XInput can work** if it is linked against `xinputuap`. Today it is `LoadLibrary("xinput1_3.dll"/"xinput1_4.dll")` plus the ordinal-100 hidden `XInputGetStateEx`.
- hidapi's Windows backend (`cfgmgr32` + `hid.dll` via `LoadLibraryW`), MMJoystick, RawInput and Qt keyboard will not work.
- SDL3 dropped UWP/WinRT support entirely.
- A Windows.Gaming.Input or GameInput handler must be written. There is no existing reference in the tree.

### Cited findings
- `enum class pad_handler { null, keyboard, ds3, ds4, dualsense, skateboard, move, xinput, mm (_WIN32), sdl (HAVE_SDL3), evdev (HAVE_LIBEVDEV) }`. — [rpcs3/Emu/Io/pad_config_types.h#L5-L24](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Io/pad_config_types.h#L5-L24)
- The handler factory is at [rpcs3/Input/pad_thread.cpp#L855-L890](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/pad_thread.cpp#L855-L890). The keyboard handler is Qt (`moveToThread(QThread*)`, `SetTargetWindow(QWindow*)`) and excluded on Android. — [rpcs3/Input/pad_thread.cpp#L157-L187](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/pad_thread.cpp#L157-L187)
- `PadHandlerBase` is the interface to implement:
  - `Init`, `list_devices`, `init_config`, `bindPadToDevice`, `process`, `get_device`;
  - `update_connection`, `get_button_values`, `get_extended_info`, `apply_pad_data`;
  - `SetPadData` (rumble/LED), `get_battery_level`, `get_mapping`, …
  - [rpcs3/Emu/Io/PadHandler.h#L129-L360](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Io/PadHandler.h#L129-L360)
- `xinput_pad_handler` loads `L"xinput1_3.dll"` then `L"xinput1_4.dll"` through `utils::dynamic_library` (LoadLibrary). It resolves `XInputGetStateEx` by ordinal 100, plus `XInputGetExtended`, `XInputGetCustomData`, `XInputSetState` and `XInputGetBatteryInformation`. It is 595 + 144 lines. — [rpcs3/Input/xinput_pad_handler.cpp#L10-L15](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/xinput_pad_handler.cpp#L10-L15), [#L375-L395](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/xinput_pad_handler.cpp#L375-L395)
- The UWP list includes `XInputGetState`, `XInputSetState` and `XInputGetBatteryInformation` from `xinputuap.dll`. `joyGetPosEx`, `RegisterRawInputDevices`, `SendInput` and `SetupDiGetClassDevsW` are not in the list. — [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)
- The pinned hidapi submodule (`d6b2a974…`) Windows backend `windows/hid.c` does `LoadLibraryW(L"hid.dll")` and `LoadLibraryW(L"cfgmgr32.dll")`, resolves `CM_Get_Device_Interface_ListW` and friends, and opens devices with `CreateFileW`. — [hidapi windows/hid.c @d6b2a97](https://github.com/libusb/hidapi/blob/d6b2a974608dec3b76fb1e36c189f22b9cf3650c/windows/hid.c)
  - On Android RPCS3 already links `hidapi::libusb` and uses an injected `g_android_usb_devices` list. — [3rdparty/hidapi/CMakeLists.txt](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty/hidapi/CMakeLists.txt), [rpcs3/Input/hid_pad_handler.h#L9-L33](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/hid_pad_handler.h#L9-L33)
- `raw_mouse_handler` uses `RegisterRawInputDevices`, `GetRawInputData` and `GetRawInputDeviceList`. `gui_pad_thread` uses `SendInput`. — [rpcs3/Input/raw_mouse_handler.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/raw_mouse_handler.cpp), [rpcs3/Input/gui_pad_thread.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Input/gui_pad_thread.cpp)
- No `Windows.Gaming.Input`, GameInput or C++/WinRT usage exists anywhere in `rpcs3/` or `Utilities/` (grep).
- SDL3 removed WinRT/UWP support: issue #10724 "Remove UWP support", closed 2024-09-06 via PR #10731, commit `154452a` "winrt: Removed WinRT/Windows Phone/UWP support". — [libsdl-org/SDL#10724](https://github.com/libsdl-org/SDL/issues/10724)

### Inferences
- Minimum viable UWP input:
  - make `xinput_pad_handler` link `xinputuap.lib` directly (drop LoadLibrary and the ordinal-100 call; no Guide button);
  - **or** write a ~600-line `wgi_pad_handler` modeled on `xinput_pad_handler`, using `Windows.Gaming.Input.Gamepad` (vibration, triggers, battery via `RawGameController`).
- DualShock/DualSense over USB on Xbox would need a WinRT HID path (`Windows.Devices.HumanInterfaceDevice`) behind hidapi's API. That is likely restricted on console.
- Moving `rpcs3/Input/*` out of `rpcs3_ui` into a non-Qt library is needed regardless.

### Gaps
- I did not check how Xbox Dev Mode exposes non-Xbox controllers.

---

## 8. Audio backends and microphone

### Takeaway

Output backends are Null, **Cubeb** (WASAPI on Windows), **XAudio2** (`_WIN32`) and **FAudio** (optional, requires SDL3 ≥ 3.2). The microphone (`cellMic`) uses **OpenAL Soft** capture.

- XAudio2 itself is in the UWP list. RPCS3's XAudio2 backend and enumerator call `CoCreateInstance(MMDeviceEnumerator)`, which is not UWP-available (UWP needs `CoCreateInstanceFromApp` / `ActivateAudioInterfaceAsync`), so they need modest changes.
- Cubeb's WASAPI backend is built on `IMMDeviceEnumerator` and `avrt`, which are not UWP-compatible.
- FAudio depends on SDL3, which has no UWP support.
- OpenAL Soft has an explicit UWP build mode (`ALSOFT_UWP` when `CMAKE_SYSTEM_NAME == WindowsStore`).

### Cited findings
- The CMake source list: Cubeb always; FAudio if SDL3 ≥ 3.2.0; XAudio2 on WIN32 (MinGW links `xaudio2_9`); `_WIN32_WINNT=0x0A00`. — [rpcs3/Emu/CMakeLists.txt#L155-L181](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L155-L181)
- Backend selection happens in `main_application::create_callbacks` (`get_audio`, `get_audio_enumerator`). — [rpcs3/main_application.cpp#L222-L260](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/main_application.cpp#L222-L260)
- `XAudio2Backend` calls `CoInitializeEx(COINIT_MULTITHREADED)`, `XAudio2Create(&instance, 0, XAUDIO2_DEFAULT_PROCESSOR)` and `CoCreateInstance(__uuidof(MMDeviceEnumerator), …)` for device-change notifications. — [rpcs3/Emu/Audio/XAudio2/XAudio2Backend.cpp#L55-L85](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Audio/XAudio2/XAudio2Backend.cpp#L55-L85)
  - `xaudio2_enumerator` also uses `IMMDeviceEnumerator`. — [rpcs3/Emu/Audio/XAudio2/xaudio2_enumerator.cpp#L28](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Audio/XAudio2/xaudio2_enumerator.cpp#L28)
- `cellMic.cpp` uses `alcCaptureOpenDevice` / `alcCaptureStart` / `alcCaptureStop`. — [rpcs3/Emu/Cell/Modules/cellMic.cpp#L380-L560](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/Cell/Modules/cellMic.cpp#L380-L560)
- `CoInitializeEx` is also called in `cellAudio.cpp`, `sys_rsxaudio.cpp` and the Cubeb backend/enumerator.
- The UWP list has `XAudio2Create` (xaudio2_9.dll), `CoInitializeEx`, `CoCreateInstanceFromApp` and `ActivateAudioInterfaceAsync` (mmdevapi.dll). It does **not** have `CoCreateInstance`. — [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)
- The pinned cubeb (`484857522c…`) `src/cubeb_wasapi.cpp` includes `<avrt.h>`, calls `AvSetMmThreadCharacteristicsA`, and calls `CoCreateInstance(__uuidof(MMDeviceEnumerator) …)` in several places. — [cubeb_wasapi.cpp @4848575](https://github.com/mozilla/cubeb/blob/484857522c73318c06f18ba0a3e17525fa98c608/src/cubeb_wasapi.cpp)
- The pinned openal-soft (`b2c48f77…`) CMake has `elseif(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") set(ALSOFT_UWP TRUE)` and "Building with UWP support". — [openal-soft CMakeLists.txt @b2c48f7](https://github.com/kcat/openal-soft/blob/b2c48f7718ef3fcf67921a8b6534c4914e328970/CMakeLists.txt)
- The pinned FAudio (`6839b88e…`) has options `BUILD_SDL3` (default ON) and `PLATFORM_WIN32` (native Win32 platform instead of SDL, default OFF). — [FAudio CMakeLists.txt @6839b88](https://github.com/FNA-XNA/FAudio/blob/6839b88e304a046ae1a609ff14a085371e02a4e0/CMakeLists.txt)

### Inferences
- Make XAudio2 the UWP default:
  - drop or replace the `IMMDeviceEnumerator` notification client (use `Windows.Media.Devices.MediaDevice.DefaultAudioRenderDeviceChanged`);
  - make the enumerator return just the default device.
- Disable Cubeb and FAudio on UWP.
- For the microphone, build OpenAL Soft in `WindowsStore` mode. It needs the `microphone` capability (and `check_microphone_permissions` callback).

### Gaps
- I did not inspect OpenAL Soft's UWP capture backend.

---

## 9. Other OS dependencies: networking, file system, dynamic loading, registry, crash handling, firmware, game formats, VFS paths

### Takeaway

- **Networking** is plain Winsock (in the UWP list, apart from `GetAdaptersInfo` and `gethostbyname`) plus wolfSSL, libcurl, miniupnpc and protobuf (RPCN). It needs the internetClientServer and privateNetworkClientServer capabilities.
- **File I/O** (`Utilities/File.cpp`) uses `CreateFileW` with `\\?\` long paths, `FindFirstFileExW`, `MoveFileExW`, `CopyFileW`, `CreateSymbolicLinkW`, `DeviceIoControl` and similar. `CreateFileW` and `CreateSymbolicLinkW` are not in the UWP list (`CreateFile2` / `CreateFileFromApp` are the equivalents).
- **Paths:** config, cache and the default VFS root (`$(EmulatorDir)` = `fs::get_config_dir()`) default to **the executable's directory** on Windows, unless a `portable/` dir or `RPCS3_CONFIG_DIR` env var exists. A UWP port must redirect these to LocalState, which an Android-style global path hook makes easy.
- **Firmware:** PUP installation logic currently lives in the Qt `main_window`.
- **Games:** PKG install, disc folders and **ISO** (including encrypted Redump/3k3y keys) are in core.

### Cited findings

**Networking**
- `rpcs3.cpp` calls `WSAStartup(MAKEWORD(2,2))`. — [rpcs3/rpcs3.cpp#L652-L656](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L652-L656)
- `WSAPoll` is used in `sys_net` (`lv2_socket`, `lv2_socket_native`, `network_context`, `sys_net_helpers`), `rpcn_client` and `SkylanderPortalIPC`.
- `getaddrinfo` / `ioctlsocket` are used in `rpcn_client`, `ip_address` and `GDB.cpp`; `gethostbyname` in `sys_net_.cpp`; `WSAIoctl` in `ip_address.cpp`.
- `GetAdaptersInfo` is used in `np_handler.cpp`. — [rpcs3/Emu/NP/np_handler.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/NP/np_handler.cpp)
- UPnP lives in `rpcs3/Emu/NP/upnp_handler.cpp` (miniupnpc). RPCN and clans use wolfSSL / libcurl (`clans_client.cpp`). — [rpcs3/Emu/NP](https://github.com/RPCS3/rpcs3/tree/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/NP)
- The pinned miniupnpc has explicit UWP guards: `#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) && WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP` and "the IPv4 only function is not available in UWP apps". — [miniupnpc minissdpc.c @d66872e](https://github.com/miniupnp/miniupnp/blob/d66872e34d9ff83a07f8b71371b13419b2089953/miniupnpc/src/minissdpc.c)
- The UWP list has `WSAStartup`, `WSAPoll`, `ioctlsocket`, `getaddrinfo` and `GetAdaptersAddresses`, but not `GetAdaptersInfo`. — [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)

**File system**
- `to_wchar()` builds `\\?\` / `\\?\UNC\` paths with `GetFullPathNameW`. — [Utilities/File.cpp#L31-L70](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/File.cpp#L31-L70)
- Other calls in the same file:
  - `GetFileAttributesExW`, `FindFirstFileExW(…, FIND_FIRST_EX_CASE_SENSITIVE)`, `GetDiskFreeSpaceExW`;
  - `CreateDirectoryW`, `RemoveDirectoryW`, `CreateSymbolicLinkW`, `CopyFileW`, `DeleteFileW`;
  - `CreateFileW(…FILE_FLAG_BACKUP_SEMANTICS…)`, `GetFileInformationByHandle`;
  - `DeviceIoControl` for raw devices.
  - [Utilities/File.cpp#L687-L710](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/File.cpp#L687-L710), [#L1059-L1110](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/File.cpp#L1059-L1110), [#L1430-L1850](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/File.cpp#L1430-L1850)
- `Utilities/File.cpp` has 33 `_WIN32` conditionals, the most of any file (my grep count).
- UWP list status: `CreateFile2`, `GetFileAttributesExW`, `FindFirstFileExW`, `MoveFileExW`, `CopyFileW`, `CopyFile2`, `DeleteFileW`, `Create/RemoveDirectoryW`, `SetFileInformationByHandle`, `GetFileInformationByHandleEx`, `DeviceIoControl`, `GetDiskFreeSpaceExW`, `GetFullPathNameW`, `ReadFile`, `WriteFile` and `GetTempPathW` are **in** the list. `CreateFileW`, `GetFileInformationByHandle` and `CreateSymbolicLinkW` are **not**. — [win32-apis.md](https://github.com/MicrosoftDocs/winrt-related/blob/docs/winrt-related-src/win32-and-com/win32-apis.md)

**Paths and VFS**
- `fs::get_config_dir()` on Windows returns `<exe dir>/portable/` if it exists. Otherwise it reads the `RPCS3_CONFIG_DIR` env var, falling back to the directory of `GetModuleFileName(nullptr)`. Its subdirectory `config/` holds `config.yml`. It uses `MessageBoxA` on errors.
- `get_cache_dir()` equals the config dir on Windows; `get_log_dir()` is `config/log/`; `get_temp_dir()` is `GetTempPathW`.
- [Utilities/File.cpp#L2518-L2700](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities/File.cpp#L2518-L2700)
- VFS defaults are `emulator_dir = "$(EmulatorDir)"` (empty means `fs::get_config_dir()`) and `dev_hdd0`, `dev_hdd1`, `dev_flash`, `dev_flash2`, `dev_flash3`, `dev_bdvd`, `games`, `dev_usb000` under `$(EmulatorDir)…`. — [rpcs3/Emu/vfs_config.h#L12-L24](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/vfs_config.h#L12-L24), [rpcs3/Emu/vfs_config.cpp#L30-L52](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/vfs_config.cpp#L30-L52)
- `config.yml` lives at `fs::get_config_dir(true) + "config.yml"`. — [rpcs3/Emu/System.cpp#L509-L517](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/System.cpp#L509-L517)
- `rpcs3.cpp` refuses to run from OneDrive paths, reading `HKCU\Software\Microsoft\OneDrive\Accounts\Personal` with `RegOpenKeyW` / `RegQueryValueExW`. It errors that "RPCS3 cannot be installed in Program Files or similar directories with limited permissions" when the log or lock file cannot be created. — [rpcs3/rpcs3.cpp#L109-L160](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L109-L160), [#L630-L645](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L630-L645)
- `steam_utils.cpp` (Qt) also uses the registry.

**Dynamic loading**
- `utils::dynamic_library` (`LoadLibraryA/W`, `GetProcAddress`, `FreeLibrary`) is used only by `xinput_pad_handler`. `DYNAMIC_IMPORT` is used as listed in §4. — [rpcs3/util/dyn_lib.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/dyn_lib.cpp)

**Crash and fatal-error handling**
- `report_fatal_error` re-spawns the process with `_wspawnl(_P_WAIT, exe, "--error", text)` to show a Qt dialog, or calls `ExitProcess`. — [rpcs3/rpcs3.cpp#L262-L282](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3.cpp#L262-L282)
- `IsDebuggerPresent` and `OutputDebugStringA` are used for logging.
- `util/console.cpp` uses `AttachConsole` / `AllocConsole`.
- `display_sleep_control.cpp` uses `SetThreadExecutionState`, which is not in the UWP list (UWP uses `Windows.System.Display.DisplayRequest`).

**Firmware**
- `utils::get_firmware_version()` reads `dev_flash/vsh/etc/version.txt`. — [rpcs3/util/sysinfo.cpp#L684-L736](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/util/sysinfo.cpp#L684-L736)
- The PUP parser is `rpcs3/Loader/PUP.cpp` (core), but **installation** is implemented in the Qt `main_window.cpp`: `pup_object pup(...)`, `pup.get_file(0x300)` (update_files.tar), `tar_object`, extraction of `dev_flash_*` entries into `/dev_flash`, and a disk-space check via `fs::statfs`. — [rpcs3/rpcs3qt/main_window.cpp#L1490-L1640](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt/main_window.cpp#L1490-L1640)
- The `on_missing_fw` callback exists for frontends.

**Game formats**
- PKG: `rpcs3::utils::install_pkg()` uses `package_reader::extract_data` (`Crypto/unpkg.cpp`). — [rpcs3/Emu/system_utils.cpp#L73-L90](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/system_utils.cpp#L73-L90)
- ISO: `Loader/ISO.cpp/.h` (`is_iso_file`, `iso_archive`, `iso_device` VFS device) handles encrypted ISOs using Redump `.dkey` / `.key` or a 3k3y watermark. — [rpcs3/Loader/ISO.h#L1-L60](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Loader/ISO.h#L1-L60)
  - Boot logic checks `is_iso_file` and mounts `iso_device::virtual_device_name`. — [rpcs3/Emu/System.cpp#L1373-L1395](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/System.cpp#L1373-L1395)
  - An `iso_cache` and IRD validation also exist. — [rpcs3/Emu/CMakeLists.txt#L118-L132](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/CMakeLists.txt#L118-L132)

**USB passthrough**
- `sys_usbd.cpp` and `Io/usb_device.cpp` use libusb (Skylanders, Infinity and other portals, real USB devices).
- The pinned libusb `windows_winusb.c` loads SetupAPI (`DLL_LOAD_FUNC_PREFIXED(SetupAPI, …SetupDiGetClassDevsA…)`), with 34 `SetupDi` references. — [libusb windows_winusb.c @87a5563](https://github.com/libusb/libusb/blob/87a55632db62c9bdc58cd31d3ccfa673f1bb017f/libusb/os/windows_winusb.c)

### Inferences
- Add a `#ifdef UWP` branch beside the existing `#ifdef ANDROID` in `fs::get_executable_dir`, `get_config_dir`, `get_cache_dir` and `get_temp_dir`, set at startup from `ApplicationData::Current().LocalFolder()` / `LocalCacheFolder` / `TemporaryFolder`. This automatically moves `dev_hdd0`, `dev_flash`, `config.yml`, caches and logs into LocalState.
- Game files on external USB drives would need broadFileSystemAccess or `removableStorage` capabilities, or a copy into LocalState (other researchers cover console storage).
- Replace or remove: `CreateFileW` → `CreateFile2`; `CreateSymbolicLinkW` → remove; `GetFileInformationByHandle` → `GetFileInformationByHandleEx`; the registry and OneDrive checks → remove; `_wspawnl` → remove; `MessageBoxA/W` in core → logging.
- Firmware install must be lifted out of `main_window.cpp` into a core helper. It is a few dozen lines using core `pup_object` and `tar_object`.

### Gaps
- I did not enumerate every Win32 call in `rpcs3qt/` (to be discarded) or in each lv2 `sys_net` file (Winsock is generally UWP-available).
- Loopback / IPC behavior (GDB server, PINE IPC `IPC_socket.cpp`) inside an AppContainer is untested.

---

## 10. 3rdparty dependencies: inventory and UWP status

### Takeaway

There are **28 git submodules** plus vendored or header-only code. The most UWP-hostile are:
- Qt (no UWP in Qt 6);
- SDL3 (UWP removed) and therefore FAudio;
- cubeb (WASAPI via MMDevice);
- hidapi and libusb (SetupAPI / cfgmgr32 / hid.dll);
- LLVM (desktop APIs in Support; needs a WINAPI_FAMILY_APP build or desktop-API linking);
- the prebuilt ffmpeg-core (desktop static libs);
- Vulkan (no system loader or driver).

Explicitly UWP-capable at the pinned versions: openal-soft (`ALSOFT_UWP`), curl (`WINDOWS_STORE` paths, not Schannel), miniupnpc (WINAPI_PARTITION_APP guards), rtmidi (`__WINDOWS_UWP__` API).

### Cited findings

**Submodules**, from `.gitmodules` with SHAs from `git ls-tree HEAD` — [.gitmodules](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/.gitmodules):

| Submodule | Source | Pinned SHA |
|---|---|---|
| ffmpeg | RPCS3/ffmpeg-core | 649d9d93 |
| asmjit | asmjit/asmjit | 416f7356 |
| llvm | llvm/llvm-project | ca7933e4, 22.1.8 |
| glslang | KhronosGroup/glslang | f0bd0257 |
| zlib | madler/zlib | da607da7 |
| hidapi | libusb/hidapi | d6b2a974 |
| pugixml | zeux/pugixml | c8033ce9 |
| yaml-cpp | RPCS3/yaml-cpp | 51a5d623 |
| libpng | glennrp/libpng | cd952f49 |
| libusb | libusb/libusb | 87a55632 |
| FAudio | FNA-XNA/FAudio | 6839b88e |
| curl | curl/curl | 01346829 |
| wolfssl | wolfSSL/wolfssl | 3c5eead4 |
| cubeb | mozilla/cubeb | 48485752 |
| SoundTouch | RPCS3/soundtouch | a0fba77b |
| SDL | libsdl-org/SDL, SDL3 | 829a65d7 |
| miniupnp | miniupnp/miniupnp | d66872e3 |
| rtmidi | thestk/rtmidi | 1e5b4992 |
| zstd | facebook/zstd | f8745da6 |
| 7zip | ip7z/7zip | 9128b80e |
| OpenAL | kcat/openal-soft | b2c48f77 |
| stb | nothings/stb | 013ac3be |
| opencv | Megamouse/opencv_minimal | d4dde8af |
| fusion | xioTechnologies/Fusion | 93254240 |
| discord-rpc | Vestrel/discord-rpc | fb04b176 |
| VulkanMemoryAllocator | GPUOpen | 3aa92122 |
| gamemode | FeralInteractive | c54d6d42 |
| protobuf | protocolbuffers/protobuf | edaa823d |

**Non-submodule 3rdparty**: `GL/` (glext headers), `MoltenVK/` (CMake only), `bcdec/bcdec.hpp`, `pine/pine_server.h`, `unordered_dense/`, `qt6.cmake`, `DetectArchitecture.cmake`. — [3rdparty/](https://github.com/RPCS3/rpcs3/tree/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty)

**How `3rdparty/CMakeLists.txt` wires them** — [3rdparty/CMakeLists.txt](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty/CMakeLists.txt):
- MSVC builds libusb from the submodule.
- On MSVC, OpenGL links `dxgi.lib`, `d2d1.lib` and `dwrite.lib`.
- Vulkan comes from `find_package(Vulkan)`.
- SDL3 is static from the submodule unless `USE_SYSTEM_SDL`.
- OpenAL is skipped on Android.
- FAudio requires SDL3 ≥ 3.2.
- ffmpeg is a prebuilt download (not on Android).
- GLEW is used on non-MSVC builds.
- discord-rpc is built only if `USE_DISCORD_RPC` (default OFF).

**Per-library UWP status**
- **ffmpeg-core** at `649d9d93` ("Bump to ffmpeg 8.1.1", 2026-06-11) ships `lib/windows/x86_64/*.lib` (avcodec, avdevice, avfilter, avformat, avutil, swresample, swscale). Its CMake downloads `ffmpeg-windows-x64.zip` from the GitHub releases. Builds use vcpkg triplets `x64-win-llvm-static-release`. — [RPCS3/ffmpeg-core @649d9d9](https://github.com/RPCS3/ffmpeg-core/tree/649d9d9353184a17ae78d9df4a6fbbe7456f0428)
- **curl** at `01346829` has `WINDOWS_STORE` handling ("UWP does not support Schannel", telnet disabled for UWP). RPCS3 configures it `CURL_USE_WOLFSSL ON`, `HTTP_ONLY ON`. — [curl CMakeLists.txt @0134682](https://github.com/curl/curl/blob/01346829096c61b372692f6dc43ffa778c6caccd/CMakeLists.txt), [3rdparty/curl/CMakeLists.txt](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/3rdparty/curl/CMakeLists.txt)
- **rtmidi** at `1e5b4992` has a `__WINDOWS_UWP__` backend (`MidiInWinUWP` / `MidiOutWinUWP`, C++/WinRT). — [RtMidi.cpp @1e5b499](https://github.com/thestk/rtmidi/blob/1e5b49925aa60065db52de44c366d446a902547b/RtMidi.cpp)
- **openal-soft**, **miniupnpc**, **hidapi**, **libusb** and **cubeb**: see §7–§9 for the cited source lines.
- **SDL3**: UWP removed (§7). **Qt 6**: UWP removed (§1).
- **wolfSSL** `settings.h` defines `USE_WINDOWS_API` on Windows. I found no WINAPI_FAMILY-specific handling in a quick grep. — [wolfssl settings.h @3c5eead](https://github.com/wolfSSL/wolfssl/blob/3c5eead44904df64e6a5a1f4ebdce377d35a849a/wolfssl/wolfcrypt/settings.h)

### Inferences

Per-dependency UWP plan (my assessment, not tested):

- **Pure-compute libraries, no or trivial OS calls (expected to build for UWP with WINAPI_FAMILY_APP):** zlib, zstd, libpng, pugixml, yaml-cpp, stb, bcdec, unordered_dense, SoundTouch, Fusion, 7zip SDK (its Windows file APIs may need checking), protobuf, glslang, VMA, asmjit (RPCS3 supplies its own memory).
- **Rebuild needed:**
  - ffmpeg: FFmpeg has a documented WinRT/UWP configure path. I did not fetch it here, so it is a gap.
  - LLVM: build with `-DWINAPI_FAMILY=WINAPI_FAMILY_APP` patches, or accept desktop imports in Dev Mode.
  - wolfSSL: needs BCrypt RNG settings checked.
- **Drop:** Qt, SDL3, FAudio, cubeb, discord-rpc, gamemode (Linux-only), libevdev (Linux), opencv (optional; PS Move tracking only), MoltenVK.
- **Replace:** hidapi and libusb with WinRT HID/USB shims or stubs.
- **Keep:** openal-soft (UWP mode), curl (wolfSSL backend), miniupnpc, rtmidi (UWP API).

### Gaps
- I did not verify whether the RPCS3 ffmpeg-core vcpkg triplets can target UWP, or whether 7zip and protobuf compile under WINAPI_FAMILY_APP.

---

## 11. Code size, for effort estimates

### Takeaway

The non-3rdparty C++ tree is about **666k lines**:

| Area | Lines | Note |
|---|---|---|
| `rpcs3/Emu` | ~500k | includes ~77k of generated protobuf code in `Emu/NP/generated` |
| `rpcs3/rpcs3qt` (Qt GUI) | ~71k | + 12.8k lines of `.ui` XML |
| `rpcs3/Input` | ~19k | |
| `Utilities` | ~25.8k | |
| `rpcs3/util` | ~22.2k | |
| VK backend | ~33k | |

Windows-specific code is concentrated in ~152 files with 511 `_WIN32` conditionals. The top hotspots are `Utilities/File.cpp` (33), `Utilities/Thread.cpp` (24), `util/vm_native.cpp` (20), `util/sysinfo.cpp` (20), `rpcs3.cpp` (18) and `sys_net/lv2_socket_native.cpp` (18).

### Cited findings

All counts are `wc -l` over `*.cpp/*.h/*.hpp/*.inl/*.mm` at commit `2ba2f75`, measured by me in the scratch clone. Directory roots: [rpcs3/Emu](https://github.com/RPCS3/rpcs3/tree/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu), [rpcs3/rpcs3qt](https://github.com/RPCS3/rpcs3/tree/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/rpcs3qt), [Utilities](https://github.com/RPCS3/rpcs3/tree/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/Utilities).

**Directories**

| Directory | Lines |
|---|---|
| `rpcs3/Emu` | 500,273 |
| — `Emu/Cell` | 257,073 |
| — — `Cell/Modules` | 134,151 |
| — — `Cell/lv2` | 43,541 |
| — `Emu/RSX` | 128,091 |
| — — `RSX/VK` | 33,089 |
| — — `RSX/GL` | 18,193 |
| — — `RSX/Overlays` | 20,150 |
| — — `RSX/Common` | 17,163 |
| — — `RSX/Program` | 10,719 |
| — `Emu/NP` | 55,796 (of which `NP/generated` 76,691?) |
| — `Emu/Io` | 18,454 |
| — `Emu/CPU` | 18,132 |
| — `Emu/Memory` | 4,026 |
| — `Emu/Audio` | 3,037 |
| `rpcs3/rpcs3qt` | 71,269 (134 `.cpp` files) + 12,819 `.ui` lines |
| `rpcs3/Input` | 19,241 |
| `Utilities` | 25,800 |
| `rpcs3/util` | 22,198 |
| `rpcs3/Crypto` | 12,049 |
| `rpcs3/Loader` | 7,306 |
| top-level `rpcs3/*.cpp/h` | 2,592 |
| **all non-3rdparty** | **666,173** |

The `NP/generated` figure exceeds the NP total, so the generated protobuf code is probably not under `Emu/NP` in the counted form. Treat the 77k figure as approximate.

**Key files**

| File | Lines |
|---|---|
| `SPULLVMRecompiler.cpp` | 10,535 |
| `SPUThread.cpp` | 7,569 |
| `PPUThread.cpp` | 6,166 |
| `PPUTranslator.cpp` | 5,644 |
| `System.cpp` | 5,127 |
| `SPUASMJITRecompiler.cpp` | 4,933 |
| `CPUTranslator.h` | 4,743 |
| `Thread.cpp` | 4,123 |
| `File.cpp` | 3,246 |
| `vm.cpp` | 2,583 |
| `atomic.cpp` | 1,414 |
| `gui_application.cpp` | 1,541 |
| `rpcs3.cpp` | 1,393 |
| `gs_frame.cpp` | 1,304 |
| `vm_native.cpp` | 1,103 |
| `JITLLVM.cpp` | 1,061 |
| `pad_thread.cpp` | 949 |
| `JITASM.cpp` | 870 |
| `main_application.cpp` | 501 |
| `headless_application.cpp` | 196 |
| `swapchain_win32.hpp` | 111 |
| `swapchain_android.hpp` | 24 |

**`_WIN32` conditionals:** 511 across 152 files (`grep -c` of `#ifdef _WIN32`, `#ifndef _WIN32`, `#if … _WIN32` and `defined(_WIN32)`).

### Inferences

The Windows-to-UWP delta is concentrated in about 15 core files plus a new frontend:
- `util/vm_native.cpp`, `Utilities/JITASM.cpp`, `JITLLVM.cpp`, `Thread.cpp`, `sync.h`, `util/atomic.cpp`, `File.cpp`, `sysinfo.cpp`, `dyn_lib.cpp`, `cpu_stats.cpp`, `console.cpp`;
- `VK/vkutils/swapchain*`, `VKHelpers.cpp`, `XAudio2*`, `xinput_pad_handler`;
- the new frontend (rough sizing: a few thousand lines, following `headless_application` (196 lines) + `main_application` (501 lines) + a CoreWindow `GSFrameBase` + a WGI pad handler).

The 71k-line Qt GUI would be discarded in favor of RSX overlays and Big Picture Mode.

The largest risks are not line count but platform policy:
- VEH availability;
- RWX / `codeGeneration` for JIT, including static-init-time stub generation;
- 56+ GiB VA reservations;
- the Vulkan driver (Dozen is explicitly unsupported by RPCS3).

### Gaps
- My line counts include comments and blank lines; I did not run a cloc-style breakdown.
- The `NP/generated` count above looks inconsistent with the NP total and should be re-checked.
