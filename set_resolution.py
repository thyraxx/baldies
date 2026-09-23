#!/usr/bin/env python3
"""
Baldies Resolution Selector Utility
Allows instant switching between modern resolution presets or custom resolutions.
Configures both ddraw.ini (window wrapper) and baldies.exe (internal engine viewport,
offscreen buffers, DirectDraw surface, camera, tile bounds, and clipping rects) so that the actual
in-game view scales seamlessly to fill the chosen window resolution.
"""

import os
import re
import sys
import struct
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
INI_PATH = os.path.join(SCRIPT_DIR, "ddraw.ini")
GAME_EXE = os.path.join(SCRIPT_DIR, "baldies.exe")
WIN11_EXE = os.path.join(SCRIPT_DIR, "baldies_win11.exe")

PRESETS = {
    "0": ("640 x 480", 640, 480, False, "Original Classic (Default Engine Res)"),
    "1": ("1024 x 768", 1024, 768, False, "Classic 4:3 Window (Expanded View)"),
    "2": ("1280 x 960", 1280, 960, False, "Standard HD 4:3 Window (Recommended)"),
    "3": ("1440 x 1080", 1440, 1080, False, "Full-Height 1080p 4:3 Window"),
    "4": ("1600 x 1200", 1600, 1200, False, "UXGA 4:3 Window (2.5x World View)"),
    "5": ("1920 x 1440", 1920, 1440, False, "QHD 4:3 Window (3x World View)"),
    "6": ("1920 x 1080", 1920, 1080, True, "Borderless Fullscreen (1080p Widescreen)"),
    "7": ("2560 x 1440", 2560, 1440, True, "Borderless Fullscreen (1440p Widescreen)"),
    "8": ("3840 x 2160", 3840, 2160, True, "Borderless Fullscreen (4K UHD Widescreen)"),
}

def patch_executable_resolution(exe_path, width, height):
    """
    Patches a Baldies executable so its internal DirectDraw surface, offscreen buffers,
    viewport, camera bounding box, clipping rectangles, and buffer pitch dynamically match (width, height).
    """
    if not os.path.exists(exe_path):
        return False

    try:
        with open(exe_path, "rb") as f:
            data = bytearray(f.read())

        w_u16 = struct.pack("<H", width)
        h_u16 = struct.pack("<H", height)
        h_sub32_u16 = struct.pack("<H", max(0, height - 32))
        w_u32 = struct.pack("<I", width)
        h_u32 = struct.pack("<I", height)

        # 1. DirectDraw & Engine resolution assignments
        data[0x00621A:0x00621C] = w_u16          # VA 0x00406E1A: ScreenWidth
        data[0x006223:0x006225] = h_u16          # VA 0x00406E23: ScreenHeight
        data[0x006503:0x006505] = h_sub32_u16    # VA 0x00407103: Bottom bound (H-32)
        data[0x006755:0x006759] = h_u32          # VA 0x00407355: SetDisplayMode height
        data[0x00675A:0x00675E] = w_u32          # VA 0x0040735A: SetDisplayMode width
        data[0x006784:0x006786] = w_u16          # VA 0x00407384: ScreenWidth
        data[0x00678D:0x00678F] = h_u16          # VA 0x0040738D: ScreenHeight
        data[0x0071EB:0x0071ED] = w_u16          # VA 0x00407DEB: ScreenWidth
        data[0x0071F4:0x0071F6] = h_u16          # VA 0x00407DF4: ScreenHeight
        data[0x007AE5:0x007AE7] = w_u16          # VA 0x004086E5: ScreenWidth
        data[0x007AEE:0x007AF0] = h_u16          # VA 0x004086EE: ScreenHeight
        data[0x007B09:0x007B0B] = h_sub32_u16    # VA 0x00408709: Bottom bound (H-32)
        data[0x0080EC:0x0080EE] = w_u16          # VA 0x00408CEC: ScreenWidth
        data[0x0080F5:0x0080F7] = h_u16          # VA 0x00408CF5: ScreenHeight
        data[0x008110:0x008114] = h_u32          # VA 0x00408D10: ClearRect height
        data[0x008115:0x008119] = w_u32          # VA 0x00408D15: ClearRect width
        data[0x00812B:0x00812F] = h_u32          # VA 0x00408D2B: ClearRect height
        data[0x008130:0x008134] = w_u32          # VA 0x00408D30: ClearRect width
        data[0x00816F:0x008173] = w_u32          # VA 0x00408D6F: ClearScreen width
        data[0x008193:0x008195] = h_u16          # VA 0x00408D93: ClearScreen height
        data[0x009DA0:0x009DA4] = w_u32          # VA 0x0040A9A0: ViewportWidth
        data[0x009DAC:0x009DB0] = h_u32          # VA 0x0040A9AC: ViewportHeight
        data[0x009E06:0x009E08] = w_u16          # VA 0x0040AA06: ScreenWidth
        data[0x009E0F:0x009E11] = h_u16          # VA 0x0040AA0F: ScreenHeight
        data[0x009EF9:0x009EFB] = w_u16          # VA 0x0040AAF9: ClipRight
        data[0x009F02:0x009F04] = h_u16          # VA 0x0040AB02: ClipBottom
        data[0x014844:0x014846] = w_u16          # VA 0x00415444: ScreenWidth
        data[0x01484D:0x01484F] = h_u16          # VA 0x0041544D: ScreenHeight

        # 2. Dynamic pitch patch at VA 0x00416FE9 (Raw 0x0163E7)
        # Replaces: mov dword ptr [0x004514AC], 640 (10 bytes)
        # With:     mov eax, [0x0045C01C]; mov [0x004514AC], eax (10 bytes exact)
        pitch_patch = bytes([0xA1, 0x1C, 0xC0, 0x45, 0x00, 0xA3, 0xAC, 0x14, 0x45, 0x00])
        data[0x0163E7:0x0163F1] = pitch_patch
        data[0x0163F8:0x0163FA] = h_sub32_u16    # VA 0x00416FF8: World bottom clip (H-32)

        # 3. In-game rendering clip bounds
        data[0x01659A:0x01659C] = w_u16          # VA 0x0041719A: ClipRight
        data[0x0165A3:0x0165A5] = h_u16          # VA 0x004171A3: ClipBottom
        data[0x016608:0x01660A] = w_u16          # VA 0x00417208: ClipRight
        data[0x016611:0x016613] = h_u16          # VA 0x00417211: ClipBottom
        data[0x01663C:0x01663E] = w_u16          # VA 0x0041723C: ClipRight
        data[0x016645:0x016647] = h_u16          # VA 0x00417245: ClipBottom
        data[0x017D30:0x017D32] = w_u16          # VA 0x00418930: ClipRight
        data[0x017D39:0x017D3B] = h_u16          # VA 0x00418939: ClipBottom

        # 4. Critical Offscreen Buffer Allocations & Full Screen Clears
        # Prevents the game view from being trapped in a 640x480 box / 1/4 window
        data[0x007BDE:0x007BE2] = h_u32          # VA 0x004087DD: ClearRect height
        data[0x007BE3:0x007BE7] = w_u32          # VA 0x004087E2: ClearRect width
        data[0x0144E0:0x0144E4] = h_u32          # VA 0x004150DF: Offscreen World Buffer [0x00461DC0] height
        data[0x0144E5:0x0144E9] = w_u32          # VA 0x004150E4: Offscreen World Buffer [0x00461DC0] width
        data[0x014988:0x01498C] = h_u32          # VA 0x00415587: Screen Buffer [0x004614FC] height
        data[0x01498D:0x014991] = w_u32          # VA 0x0041558C: Screen Buffer [0x004614FC] width
        data[0x01504D:0x015051] = h_u32          # VA 0x00415C4C: ClearRect height
        data[0x015052:0x015056] = w_u32          # VA 0x00415C51: ClearRect width
        data[0x044913:0x044917] = h_u32          # VA 0x00445512: ClearRect height
        data[0x044918:0x04491C] = w_u32          # VA 0x00445517: ClearRect width
        data[0x04492E:0x044932] = h_u32          # VA 0x0044552D: ClearRect height
        data[0x044933:0x044937] = w_u32          # VA 0x00445532: ClearRect width
        data[0x044E9D:0x044EA1] = h_u32          # VA 0x00445A9C: ClearRect height
        data[0x044EA2:0x044EA6] = w_u32          # VA 0x00445AA1: ClearRect width
        data[0x044EB8:0x044EBC] = h_u32          # VA 0x00445AB7: ClearRect height
        data[0x044EBD:0x044EC1] = w_u32          # VA 0x00445ABC: ClearRect width

        with open(exe_path, "wb") as f:
            f.write(data)
        return True
    except Exception as e:
        print(f"Warning: Could not patch {exe_path}: {e}")
        return False

def get_current_settings():
    width, height, fullscreen = 1280, 960, False
    if not os.path.exists(INI_PATH):
        return width, height, fullscreen
    with open(INI_PATH, "r", encoding="latin1") as f:
        content = f.read()
    m_w = re.search(r"^width=(\d+)", content, re.MULTILINE)
    m_h = re.search(r"^height=(\d+)", content, re.MULTILINE)
    m_f = re.search(r"^fullscreen=(true|false)", content, re.MULTILINE)
    if m_w: width = int(m_w.group(1))
    if m_h: height = int(m_h.group(1))
    if m_f: fullscreen = (m_f.group(1).lower() == "true")
    return width, height, fullscreen

def apply_resolution(width, height, fullscreen=False):
    if not os.path.exists(INI_PATH):
        print(f"Error: {INI_PATH} not found!")
        return False
    with open(INI_PATH, "r", encoding="latin1") as f:
        content = f.read()

    # Update ddraw.ini
    content = re.sub(r"^width=\d*", f"width={width}", content, count=1, flags=re.MULTILINE)
    content = re.sub(r"^height=\d*", f"height={height}", content, count=1, flags=re.MULTILINE)
    fs_val = "true" if fullscreen else "false"
    content = re.sub(r"^fullscreen=(true|false)", f"fullscreen={fs_val}", content, count=1, flags=re.MULTILINE)

    with open(INI_PATH, "w", encoding="latin1") as f:
        f.write(content)

    # Patch executables so in-game camera & canvas match the new resolution
    patched_count = 0
    if patch_executable_resolution(GAME_EXE, width, height):
        patched_count += 1
    if os.path.exists(WIN11_EXE):
        if patch_executable_resolution(WIN11_EXE, width, height):
            patched_count += 1

    mode_str = "Borderless Fullscreen" if fullscreen else "Windowed"
    print(f"\n[OK] Configured {width}x{height} ({mode_str}):")
    print(f"     - ddraw.ini updated")
    print(f"     - In-game offscreen buffers, viewport & camera engine patched ({patched_count} executable(s))")
    return True

def launch_game():
    if os.path.exists(GAME_EXE):
        print("\nLaunching Baldies...")
        subprocess.Popen([GAME_EXE], cwd=os.path.dirname(GAME_EXE))
    elif os.path.exists(WIN11_EXE):
        print("\nLaunching Baldies (Win11)...")
        subprocess.Popen([WIN11_EXE], cwd=os.path.dirname(WIN11_EXE))
    else:
        print(f"Error: Neither {GAME_EXE} nor {WIN11_EXE} found!")

def main():
    if len(sys.argv) >= 3:
        w = int(sys.argv[1])
        h = int(sys.argv[2])
        fs = (sys.argv[3].lower() in ["true", "1", "yes", "y"]) if len(sys.argv) > 3 else False
        apply_resolution(w, h, fs)
        return

    while True:
        cur_w, cur_h, cur_fs = get_current_settings()
        cur_mode = "Borderless Fullscreen" if cur_fs else "Windowed"

        print("\n" + "=" * 58)
        print("          BALDIES - RESOLUTION & VIEW SELECTOR")
        print("=" * 58)
        print(f" Current setting: {cur_w} x {cur_h} ({cur_mode})\n")
        print(" Presets (Changes Window size + In-game Camera field of view):")
        for k, v in sorted(PRESETS.items()):
            active = " [ACTIVE]" if (cur_w == v[1] and cur_h == v[2] and cur_fs == v[3]) else ""
            print(f"   [{k}]  {v[0]:12} - {v[4]}{active}")
        print("\n   [9]  Custom Resolution (enter Width and Height)")
        print("   [L]  Launch Baldies now")
        print("   [Q]  Quit")
        print("=" * 58)

        choice = input("Enter your choice [0-9, L, Q]: ").strip().upper()

        if choice in PRESETS:
            p = PRESETS[choice]
            apply_resolution(p[1], p[2], p[3])
            ask = input("Launch Baldies now? (Y/N): ").strip().upper()
            if ask == "Y":
                launch_game()
                break
        elif choice == "9":
            try:
                w = int(input("Enter Width  (e.g. 1920): ").strip())
                h = int(input("Enter Height (e.g. 1080): ").strip())
                fs_in = input("Fullscreen? (y/n): ").strip().lower()
                fs = fs_in == "y"
                apply_resolution(w, h, fs)
                ask = input("Launch Baldies now? (Y/N): ").strip().upper()
                if ask == "Y":
                    launch_game()
                    break
            except ValueError:
                print("Invalid input, please enter numbers.")
        elif choice == "L":
            launch_game()
            break
        elif choice == "Q":
            break
        else:
            print("Invalid choice, please try again.")

if __name__ == "__main__":
    main()
