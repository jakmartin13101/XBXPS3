# Console UI

XBXPS3 has no desktop window, mouse or keyboard UI. Everything on screen is drawn by RPCS3's native overlay system (RSX overlays) through the same Vulkan renderer that runs games. It is driven entirely with the controller.

## Why RPCS3's own Big Picture Mode

The desktop RPCS3 UI is Qt 6, and Qt dropped UWP in Qt 6. Upstream RPCS3 gained a full-screen launcher built on its overlay system in 2026 (`Emulator::BootBigPictureMode`, `rpcs3/Emu/RSX/Overlays/BigPicture/`). It already has what a console front end needs:

- a game grid with cover art (ICON0/PIC1 from each game);
- game details and a start button;
- the PS-button home menu, with settings pages, save states, trophies and friends;
- message, save-data and user dialogs, and the on-screen keyboard games use;
- shader and PPU-module compile progress, and trophy pop-ups.

Reusing it is the same choice XBSX2 made with PCSX2's Big Picture mode. It keeps the port's UI work small and it stays in sync with upstream. The other options considered were XAML (C++/WinRT), Dear ImGui, SDL3 (which has no UWP support in SDL3) and a custom XMB.

## Screen flow

```
Launch ─► Splash ─► Big Picture: Games grid ──A──► Game details ──A (Start)──► Game
              │                      │                                          │
              │                      └─ Menu: "Reload Game List"               View+Menu
              │                                                                  ▼
              └─ first run: firmware/PKG import progress dialogs          Home menu (pause)
                                                                          resume / settings /
                                                                          save state / quit game
                                                                                  │
                                                         quit game ◄──────────────┘
                                                         └─► back to the Games grid
```

- **The app never "exits" to nothing.** When emulation stops for any reason (quit game, failed boot, "Reload Game List"), the core boots Big Picture Mode again. You leave the app with the Xbox button, as with any console app.
- **Suspend:** pressing the Xbox button suspends the app. XBXPS3 freezes emulation first and resumes it when you come back.

## Button map

| Context | A | B | X / Y | LB / RB | View + Menu |
|---|---|---|---|---|---|
| Games grid | Open game | – | per RPCS3 hints | page | – |
| Game details | Start | Back to grid | – | – | – |
| In game | Cross | Circle | Square / Triangle | L1 / R1 | Home menu (PS button) |
| Home menu | Select | Back / close | – | change page | Close |

The Xbox **B** button also raises the system "back" event. The shell swallows it so the console never navigates away; RPCS3 still sees B as Circle. The Xbox (Guide) button cannot be read by apps, so **View + Menu held together** stands in for the PS button. The shell detects the chord with Windows.Gaming.Input and raises RPCS3's `pad::g_home_menu_requested`.

## TV considerations

- Output follows the TV mode reported by `HdmiDisplayInformation`, capped at 4K on Series X and 1440p on Series S. XBSX2 found that a 4K swapchain crashes on Series S.
- The overlays scale from a 1280×720 virtual canvas, so text stays readable at couch distance.
- The display is kept awake while a game runs (`DisplayRequest`), and allowed to sleep in the launcher.
- Overlay text comes from RPCS3's English string table, extracted at build time (`core/tools/gen_localized_strings.py`). Xbox-specific wording lives in `core/tools/overrides.txt`. For example, the "No games found" text points at `E:\XBXPS3\games`.

## Planned UI work

These items are Xbox additions on top of Big Picture Mode, listed in [ROADMAP.md](ROADMAP.md):
1. A **Settings → Xbox** page: internal resolution presets, FPS overlay, USB vs internal storage, and re-run import.
2. An **Install** entry in the main menu that lists the Install folders and runs imports on demand, instead of only at startup.
3. Controller remapping UI. Today it is per-game YAML under `config/input_configs`.
4. Title-safe margins for TVs with overscan.
