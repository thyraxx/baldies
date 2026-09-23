# Baldies (Windows 11 Modernized Edition)

A modernized, patched, and portable version of the classic 1995/1996 real-time strategy game **Baldies**, configured to run smoothly on **Windows 10 & Windows 11**.

---

## About the Game

*Baldies* is a quirky real-time strategy / god-game originally developed by Creative Edge Software and published by Atari and Panasonic. Players guide a population of small, bald humanoids to build settlements, research bizarre inventions, reproduce, and wage war against their enemies (the "Hairies").

Baldies are divided into four distinct colored roles:
- **Red (Workers)**: Generate energy to breed new Baldies and build your population.
- **Blue (Builders)**: Construct, upgrade, and repair houses, barracks, and laboratories.
- **White (Scientists)**: Research traps and wacky inventions (landmines, grenades, attack helicopters, cow disguises).
- **Grey (Soldiers)**: Arm themselves with weapons and lead military assaults against enemy armies.

---

## Why This Modernization Was Needed

When running the original 1995/1996 Windows 95 retail release on modern Windows 11, several game-breaking issues occurred:
1. **DirectDraw Error (`DDERR.OUT`)**: The game failed immediately on launch with `IDirectDraw_SetDisplayMode DDERR_UNSUPPORTED` because modern graphics drivers no longer support legacy exclusive 8-bit paletted fullscreen modes (`320x240x8` / `640x480x8`).
2. **Hyper-Speed Gameplay**: The game loop relied on `IDirectDraw::WaitForVerticalBlank` for 60Hz frame pacing. On modern multi-GHz systems without VSync emulation, the game ran at thousands of ticks per second, causing units and logic to move at uncontrollable speeds.
3. **Broken Mouse Tracking**: The game engine repeatedly yanked the mouse cursor to the center of the screen via `SetCursorPos` every frame to calculate relative deltas. In upscaled windowed mode, this dragged the Windows cursor away and broke input.
4. **Hardcoded Paths & Missing CD**: The asset loader required an absolute installation path in `baldies.bal` and triggered a *"Please Insert Baldies CD"* check if files were moved.
5. **Multiplayer Crash**: `baldnet.exe` crashed on launch due to the absence of legacy DirectX 2/3 `DPLAY.DLL`.

---

## Applied Fixes & Features

- **Modern DirectDraw Translation**: Integrated **cnc-ddraw v7.1.0.0** (`ddraw.dll`), translating legacy DirectDraw calls into modern Direct3D 9 / 11 with GPU-accelerated 8-bit palette emulation.
- **Dynamic In-Game Viewport & Camera Scaling**: The game engine's internal rendering pipeline, DirectDraw surface allocation, camera boundaries, and clipping rects have been patched to scale dynamically to modern resolutions. Selecting higher resolutions actually expands the in-game field of view (showing more tiles, units, and world map simultaneously) rather than simply stretching low-res pixels.
- **Crisp Upscaled Windowed Mode**: Pre-configured to render in an expanded **1280x960** window preserving the authentic 4:3 aspect ratio.
- **Smooth 30 FPS Pacing**: Throttled game ticks via `PeekMessage` hooking (`maxgameticks=30`, `limiter_type=4`, `vsync=true`), restoring original simulation and animation speed.
- **Mouse Centering Virtualization**: Enabled `center_cursor_fix=true` and `hook_peekmessage=true` to virtualize `SetCursorPos`, providing smooth and responsive cursor control.
- **Standalone Portable Executable (`baldies_win11.exe`)**: Reverse-engineered and byte-patched to default to the local directory without requiring `baldies.bal`.
- **Restored Multiplayer**: Packaged the required 32-bit `DPLAY.DLL` alongside `baldnet.exe` for DirectPlay network sessions.

---

## How to Run & Play

### Singleplayer
Double-click either:
- **`baldies.exe`** (Standard executable, uses portable relative asset path)
- **`baldies_win11.exe`** (Patched standalone executable, completely self-contained)

### Resolution & In-Game View Selector
- Double-click **`set_resolution.bat`** (or run `python set_resolution.py` / `set_resolution.ps1`) to choose between modern resolution presets. This configures both the window wrapper and patches the game's internal camera canvas so that higher resolutions give an expanded view of the map:
  - `[0]` **640 x 480** (Original Classic — 20x14 tile field of view)
  - `[1]` **1024 x 768** (Classic 4:3 Window — 32x23 tile field of view)
  - `[2]` **1280 x 960** (Standard HD 4:3 Window — 40x29 tile field of view, Default)
  - `[3]` **1440 x 1080** (Full-Height 1080p 4:3 Window)
  - `[4]` **1600 x 1200** (UXGA 4:3 Window — 50x36 tile field of view)
  - `[5]` **1920 x 1440** (QHD 4:3 Window — 60x45 tile field of view)
  - `[6]` **1920 x 1080** (Borderless Fullscreen — 1080p Widescreen)
  - `[7]` **2560 x 1440** (Borderless Fullscreen — 1440p Widescreen)
  - `[8]` **3840 x 2160** (Borderless Fullscreen — 4K UHD Widescreen)
  - `[9]` **Custom Resolution** (Any custom width x height)

### Multiplayer
- Double-click **`baldnet.exe`**.

### In-Game Hotkeys & Controls
- **Alt + Enter**: Toggle seamlessly between windowed mode and borderless fullscreen native resolution.
- **Alt + PageDown**: Maximize window to fill monitor height while preserving the 4:3 aspect ratio.
- **Ctrl + Tab** or **Right Alt**: Unlock mouse cursor from game window.
- **Mouse Controls**:
  - **Left Click**: Select Baldies, place buildings, assign roles inside houses, drop inventions.
  - **Right Click / Drag**: Scroll map view, drop selected Baldies.

---

## Customizing Display & Speed

You can easily customize graphics, shaders, and game speed:
1. **Resolution Selector Tool**: Run **`set_resolution.bat`** to switch resolutions instantly.
2. **Interactive Config Utility**: Run **`cnc-ddraw config.exe`** to select renderers (Direct3D 9, Direct3D 11, OpenGL), display resolutions, and post-processing shaders (xBRZ, CRT scanlines, bilinear filtering).
3. **Manual Configuration (`ddraw.ini`)**:
   - **Game Speed**: Adjust `maxgameticks=30` (e.g. `25` for slower pace, `30` for default, `45` or `60` for fast pace).
   - **Window Resolution**: Modify `width=1280` and `height=960` to your preferred window size.
   - **Fullscreen**: Set `fullscreen=true` to launch in borderless fullscreen by default.

---

## Project Structure & Architecture

For detailed reverse-engineering documentation, disassembly addresses, memory maps, and asset file formats, see:
- **[`PROJECT_MAP.md`](PROJECT_MAP.md)**: Detailed technical map of entry points, component structures, data flow, and `.BAL` file specifications.
- **[`BALDMAN2.TXT`](BALDMAN2.TXT)**: The original Windows 95 game manual.
