#include "platform.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

static HWND g_hwnd = NULL;
static HDC g_hdc = NULL;
static bool g_is_fullscreen = false;
static RECT g_windowed_rect = { 100, 100, 1380, 1060 };
static int g_win_w = 1280;
static int g_win_h = 960;
static platform_input_t g_current_input = {0};
static bool g_prev_left = false;
static bool g_prev_right = false;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            g_current_input.quit_requested = true;
            PostQuitMessage(0);
            return 0;

        case WM_MOUSEMOVE: {
            int mx = (int)(short)LOWORD(lParam);
            int my = (int)(short)HIWORD(lParam);
            g_current_input.mouse_x = mx;
            g_current_input.mouse_y = my;
            return 0;
        }

        case WM_LBUTTONDOWN:
            g_current_input.mouse_left_down = true;
            SetCapture(hWnd);
            return 0;

        case WM_LBUTTONUP:
            g_current_input.mouse_left_down = false;
            ReleaseCapture();
            return 0;

        case WM_RBUTTONDOWN:
            g_current_input.mouse_right_down = true;
            return 0;

        case WM_RBUTTONUP:
            g_current_input.mouse_right_down = false;
            return 0;

        case WM_KEYDOWN: {
            bool is_down = true;
            switch (wParam) {
                case VK_LEFT:  case 'A': g_current_input.key_left = is_down; break;
                case VK_RIGHT: case 'D': g_current_input.key_right = is_down; break;
                case VK_UP:    case 'W': g_current_input.key_up = is_down; break;
                case VK_DOWN:  case 'S': g_current_input.key_down = is_down; break;
                case VK_ESCAPE:          g_current_input.key_escape = is_down; break;
                case VK_SPACE:           g_current_input.key_space = is_down; break;
                case '1':                g_current_input.key_1 = is_down; break;
                case '2':                g_current_input.key_2 = is_down; break;
                case '3':                g_current_input.key_3 = is_down; break;
                case '4':                g_current_input.key_4 = is_down; break;
                case 'M':                g_current_input.key_m = is_down; break;
                case VK_RETURN:
                    if (GetKeyState(VK_MENU) & 0x8000) {
                        g_current_input.key_toggle_fullscreen = true;
                    }
                    break;
            }
            return 0;
        }

        case WM_KEYUP: {
            bool is_down = false;
            switch (wParam) {
                case VK_LEFT:  case 'A': g_current_input.key_left = is_down; break;
                case VK_RIGHT: case 'D': g_current_input.key_right = is_down; break;
                case VK_UP:    case 'W': g_current_input.key_up = is_down; break;
                case VK_DOWN:  case 'S': g_current_input.key_down = is_down; break;
                case VK_ESCAPE:          g_current_input.key_escape = is_down; break;
                case VK_SPACE:           g_current_input.key_space = is_down; break;
                case '1':                g_current_input.key_1 = is_down; break;
                case '2':                g_current_input.key_2 = is_down; break;
                case '3':                g_current_input.key_3 = is_down; break;
                case '4':                g_current_input.key_4 = is_down; break;
                case 'M':                g_current_input.key_m = is_down; break;
            }
            return 0;
        }

        case WM_SIZE:
            g_win_w = LOWORD(lParam);
            g_win_h = HIWORD(lParam);
            return 0;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(NULL);
                return TRUE;
            }
            break;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

bool platform_init(const char *title, int width, int height, bool fullscreen) {
    timeBeginPeriod(1);

    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.lpszClassName = "BaldiesC_WindowClass";

    if (!RegisterClassExA(&wc)) {
        return false;
    }

    g_win_w = width;
    g_win_h = height;
    g_is_fullscreen = fullscreen;

    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    if (fullscreen) {
        style = WS_POPUP | WS_VISIBLE;
    }

    RECT rc = { 0, 0, width, height };
    AdjustWindowRectEx(&rc, style, FALSE, 0);

    g_hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        title ? title : "Baldies (C Engine)",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!g_hwnd) {
        return false;
    }

    g_hdc = GetDC(g_hwnd);
    SetStretchBltMode(g_hdc, COLORONCOLOR);
    return true;
}

void platform_poll_events(platform_input_t *out_input) {
    // Reset one-shot clicks
    g_current_input.mouse_left_clicked = false;
    g_current_input.mouse_right_clicked = false;
    g_current_input.mouse_left_released = false;
    g_current_input.key_toggle_fullscreen = false;

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    // Detect click edge
    if (g_current_input.mouse_left_down && !g_prev_left) {
        g_current_input.mouse_left_clicked = true;
    }
    if (!g_current_input.mouse_left_down && g_prev_left) {
        g_current_input.mouse_left_released = true;
    }
    if (g_current_input.mouse_right_down && !g_prev_right) {
        g_current_input.mouse_right_clicked = true;
    }
    g_prev_left = g_current_input.mouse_left_down;
    g_prev_right = g_current_input.mouse_right_down;

    if (g_current_input.key_toggle_fullscreen) {
        platform_toggle_fullscreen();
    }

    if (out_input) {
        *out_input = g_current_input;
    }
}

void platform_present(const surface_t *surface) {
    if (!g_hdc || !surface || !surface->pixels) return;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)surface->width;
    bmi.bmiHeader.biHeight = -(LONG)surface->height; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    RECT clientRect;
    GetClientRect(g_hwnd, &clientRect);
    int dest_w = clientRect.right - clientRect.left;
    int dest_h = clientRect.bottom - clientRect.top;

    if (dest_w == (int)surface->width && dest_h == (int)surface->height) {
        SetDIBitsToDevice(
            g_hdc,
            0, 0,
            surface->width, surface->height,
            0, 0,
            0, surface->height,
            surface->pixels,
            &bmi,
            DIB_RGB_COLORS
        );
    } else {
        StretchDIBits(
            g_hdc,
            0, 0, dest_w, dest_h,
            0, 0, surface->width, surface->height,
            surface->pixels,
            &bmi,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }
}

uint64_t platform_get_time_ms(void) {
    static LARGE_INTEGER freq;
    static BOOL initialized = FALSE;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = TRUE;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint64_t)((now.QuadPart * 1000) / freq.QuadPart);
}

void platform_sleep_ms(uint32_t ms) {
    Sleep(ms);
}

void platform_toggle_fullscreen(void) {
    if (!g_hwnd) return;

    g_is_fullscreen = !g_is_fullscreen;
    if (g_is_fullscreen) {
        GetWindowRect(g_hwnd, &g_windowed_rect);
        SetWindowLongPtrA(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        int scr_w = GetSystemMetrics(SM_CXSCREEN);
        int scr_h = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(g_hwnd, HWND_TOP, 0, 0, scr_w, scr_h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        SetWindowLongPtrA(g_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(
            g_hwnd, HWND_NOTOPMOST,
            g_windowed_rect.left, g_windowed_rect.top,
            g_windowed_rect.right - g_windowed_rect.left,
            g_windowed_rect.bottom - g_windowed_rect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW
        );
    }
}

void platform_get_window_size(int *out_w, int *out_h) {
    if (out_w) *out_w = g_win_w;
    if (out_h) *out_h = g_win_h;
}

void platform_shutdown(void) {
    if (g_hdc && g_hwnd) {
        ReleaseDC(g_hwnd, g_hdc);
        g_hdc = NULL;
    }
    if (g_hwnd) {
        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }
    timeEndPeriod(1);
}

#else

bool platform_init(const char *title, int width, int height, bool fullscreen) { return false; }
void platform_poll_events(platform_input_t *out_input) { if (out_input) out_input->quit_requested = true; }
void platform_present(const surface_t *surface) {}
uint64_t platform_get_time_ms(void) { return 0; }
void platform_sleep_ms(uint32_t ms) {}
void platform_toggle_fullscreen(void) {}
void platform_get_window_size(int *out_w, int *out_h) {}
void platform_shutdown(void) {}

#endif
