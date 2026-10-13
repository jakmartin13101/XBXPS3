# Frontend, input, audio and 10-foot UI design for an RPCS3 UWP port on Xbox Series X|S (Dev Mode) — state as of Oct 2026

(Work in progress — sections are being filled incrementally.)

## Q1. Qt on UWP: what replaces RPCS3's Qt GUI

### Takeaway
PENDING

### Cited Findings
- PENDING

### Inferences
- PENDING

### Gaps
- PENDING

## Q2. UI framework options for an Xbox UWP C++ app (XAML/WinUI 2, Dear ImGui, SDL3, RetroArch-style, RPCS3 RSX overlays)

### Takeaway
PENDING

### Cited Findings
**RPCS3 RSX overlays and Big Picture Mode (upstream, checkout `2ba2f75`)**
- `big_picture_dialog` is a `user_interface` overlay. Its main menu is a `home_menu_sidebar_page` with three entries: a game grid page (home icon), the in-game home-menu settings page (`home_menu_settings`) and an "Exit" item. — `rpcs3/Emu/RSX/Overlays/BigPicture/overlay_big_picture_main_menu.cpp` ([GitHub](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/BigPicture/overlay_big_picture_main_menu.cpp))
- The game grid is fixed at `m_columns = 5`, `m_tile_size = 200` (virtual px), keeps ICON0.PNG's 320x176 aspect, enumerates games on a background `named_thread`, and opens a full-screen `big_picture_game_details` panel (PIC1 background, ICON0, optional video via `video_view`, title, info text, Start/Back hints). — [overlay_big_picture_game_grid.h](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/BigPicture/overlay_big_picture_game_grid.h), [overlay_big_picture_game_details.h](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/BigPicture/overlay_big_picture_game_details.h)
- Overlays draw in a fixed 1280x720 virtual canvas (`virtual_width = 1280`, `virtual_height = 720`). — [overlays.h#L47-L48](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/overlays.h#L47-L48)
- Overlay input is read from `pad::get_pad_thread()` virtual DS3 pads (CELL_PAD_CTRL_* bits), honoring `g_cfg.sys.enter_button_assignment` (cross vs circle as confirm), with auto-repeat timers. So any `PadHandlerBase` (e.g. a Windows.Gaming.Input one) automatically drives the overlay UI. — [overlays.cpp#L223-L410](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/overlays.cpp#L223-L410)
- The in-game home menu (`home_menu_main_menu`) already has: Resume, Settings, Friends (if RPCN configured), Trophies (if game has trophies), Screenshot, Recording, Toggle fullscreen, Savestates page, Restart, Exit Game. — [overlay_home_menu_main_menu.cpp](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/HomeMenu/overlay_home_menu_main_menu.cpp)
- `home_menu_settings` has 7 pages (Audio, Video, Input, Advanced, Overlays, Performance overlay, Debug) built from generic `add_checkbox`/`add_dropdown`/`add_signed_slider`/`add_unsigned_slider`/`add_float_slider` helpers bound directly to `g_cfg` entries; only runtime-changeable ("dynamic") settings are exposed (volume, buffering, resolution scale %, vsync, frame limit, AF, output scaling, RCAS, SPU threads, vblank, pad mode, perf overlay, etc.). — [overlay_home_menu_settings.cpp#L16-L273](https://github.com/RPCS3/rpcs3/blob/2ba2f75bcea7ef6bdd687046c7fcfb790faf9853/rpcs3/Emu/RSX/Overlays/HomeMenu/overlay_home_menu_settings.cpp)

### Inferences
- PENDING

### Gaps
- PENDING

## Q3. Microsoft "Designing for Xbox and TV" guidance: concrete values

### Takeaway
PENDING

### Cited Findings
- Xbox renders UWP apps at 1920x1080; XAML apps are scaled 200% by default (HTML 150%), so the design canvas is 960x540 epx; "Designing for Xbox is a little different from designing for PC because you only need to worry about one resolution, 1920 x 1080." — [Designing for Xbox and TV](https://learn.microsoft.com/en-us/windows/apps/design/devices/designing-for-tv)
- Interactive elements min height 32 epx; edge-to-edge navigation should take no more than six clicks; main text 15 epx minimum, non-critical text 12 epx minimum; "The amount of information displayed on a TV should be comparable to what you'd see on a mobile phone, rather than on a desktop." — same
- TV-safe area: draw only non-essential visuals within 5% of the screen edges; avoid essential UI within 27 epx top/bottom and 48 epx left/right (of 960x540 epx); `ApplicationView.GetForCurrentView().SetDesiredBoundsMode(ApplicationViewBoundsMode.UseCoreWindow)` extends the window to the screen edges. — same
- Scale-factor opt-out: `ApplicationViewScaling.TrySetDisableLayoutScaling(true)` (can only switch to 100%). — same
- TV-safe colors: RGB 16-235 are "generally safe"; since Fall Creators Update Xbox auto-scales full-range content; D3D swap chains can declare color space via `IDXGISwapChain3::SetColorSpace1`. — same
- Recommendations: View button opens the nav pane; avoid Tooltips on TV; Y button as search accelerator (Segoe Xbox MDL2 Symbol glyph `&#xE3CC;`); open the on-screen keyboard immediately when search opens. — same

### Inferences
- PENDING

### Gaps
- PENDING

## Q4. Layout inspiration and essential launcher features

### Takeaway
PENDING

### Cited Findings
- PENDING

### Inferences
- PENDING

### Gaps
- PENDING

## Q5. Input: Windows.Gaming.Input, XInput, system buttons, vibration, DS3 mapping, keyboard/mouse, PlayStation pads

### Takeaway
PENDING

### Cited Findings
- PENDING

### Inferences
- PENDING

### Gaps
- PENDING

## Q6. Audio: XAudio2, WASAPI, cubeb, FAudio, microphone

### Takeaway
PENDING

### Cited Findings
- PENDING

### Inferences
- PENDING

### Gaps
- PENDING

## Q7. App lifecycle and display: suspend/resume, Game mode, 4K/HDR/120 Hz, DisplayRequest

### Takeaway
PENDING

### Cited Findings
- PENDING

### Inferences
- PENDING

### Gaps
- PENDING

## Q8. Text input: on-screen keyboard (CoreInputView / InputPane) for RPCN and the PS3 OSK

### Takeaway
PENDING

### Cited Findings
- PENDING

### Inferences
- PENDING

### Gaps
- PENDING

## Q9. Proposed UI architecture and screen map with button mappings

### Takeaway
PENDING

### Cited Findings
- PENDING

### Inferences
- PENDING

### Gaps
- PENDING
