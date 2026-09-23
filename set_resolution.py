#!/usr/bin/env python3
"""
Baldies Resolution Selector Utility
Allows instant switching between modern resolution presets or custom resolutions.
"""

import os
import re
import sys
import subprocess

INI_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ddraw.ini")

PRESETS = {
    "1": ("1024 x 768", 1024, 768, False, "Classic 4:3 Window"),
    "2": ("1280 x 960", 1280, 960, False, "Standard HD 4:3 Window (Default)"),
    "3": ("1440 x 1080", 1440, 1080, False, "Full-Height 1080p 4:3 Window"),
    "4": ("1600 x 1200", 1600, 1200, False, "UXGA 4:3 Window (2.5x Integer Scale)"),
    "5": ("1920 x 1440", 1920, 1440, False, "QHD 4:3 Window (3x Integer Scale)"),
    "6": ("1920 x 1080", 1920, 1080, True, "Borderless Fullscreen (1080p)"),
    "7": ("2560 x 1440", 2560, 1440, True, "Borderless Fullscreen (1440p)"),
    "8": ("3840 x 2160", 3840, 2160, True, "Borderless Fullscreen (4K UHD)"),
}

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

    # Replace only in the main [ddraw] section (first match)
    content = re.sub(r"^width=\d*", f"width={width}", content, count=1, flags=re.MULTILINE)
    content = re.sub(r"^height=\d*", f"height={height}", content, count=1, flags=re.MULTILINE)
    fs_val = "true" if fullscreen else "false"
    content = re.sub(r"^fullscreen=(true|false)", f"fullscreen={fs_val}", content, count=1, flags=re.MULTILINE)

    with open(INI_PATH, "w", encoding="latin1") as f:
        f.write(content)
    mode_str = "Borderless Fullscreen" if fullscreen else "Windowed"
    print(f"\n[OK] Applied {width}x{height} ({mode_str}) to ddraw.ini")
    return True

def launch_game():
    game_exe = os.path.join(os.path.dirname(os.path.abspath(__file__)), "baldies.exe")
    if os.path.exists(game_exe):
        print("\nLaunching Baldies...")
        subprocess.Popen([game_exe], cwd=os.path.dirname(game_exe))
    else:
        print(f"Error: {game_exe} not found!")

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

        print("\n" + "=" * 56)
        print("          BALDIES - RESOLUTION SELECTOR")
        print("=" * 56)
        print(f" Current setting: {cur_w} x {cur_h} ({cur_mode})\n")
        print(" Presets:")
        for k, v in sorted(PRESETS.items()):
            active = " [ACTIVE]" if (cur_w == v[1] and cur_h == v[2] and cur_fs == v[3]) else ""
            print(f"   [{k}]  {v[0]:12} - {v[4]}{active}")
        print("\n   [9]  Custom Resolution (enter Width and Height)")
        print("   [L]  Launch Baldies now")
        print("   [Q]  Quit")
        print("=" * 56)

        choice = input("Enter your choice [1-9, L, Q]: ").strip().upper()

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
