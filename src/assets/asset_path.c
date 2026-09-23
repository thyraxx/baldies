#include "asset_path.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

static char g_exe_dir[MAX_PATH] = {0};
static char g_exe_parent[MAX_PATH] = {0};
static bool g_path_initialized = false;

void asset_system_init(void) {
    if (g_path_initialized) return;

    if (GetModuleFileNameA(NULL, g_exe_dir, MAX_PATH) > 0) {
        char *last_bs = strrchr(g_exe_dir, '\\');
        char *last_fs = strrchr(g_exe_dir, '/');
        char *slash = (last_bs > last_fs) ? last_bs : last_fs;
        if (slash) {
            *slash = '\0';
        }

        // Parent directory
        strncpy(g_exe_parent, g_exe_dir, MAX_PATH);
        last_bs = strrchr(g_exe_parent, '\\');
        last_fs = strrchr(g_exe_parent, '/');
        slash = (last_bs > last_fs) ? last_bs : last_fs;
        if (slash) {
            *slash = '\0';
        }
    }

    // Auto-fix current working directory if launched from bin/ subdirectory
    DWORD attr = GetFileAttributesA("BALS");
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        // If BALS is in parent directory, set current directory to parent
        char test_parent[MAX_PATH];
        snprintf(test_parent, sizeof(test_parent), "%s\\BALS", g_exe_parent);
        DWORD p_attr = GetFileAttributesA(test_parent);
        if (p_attr != INVALID_FILE_ATTRIBUTES && (p_attr & FILE_ATTRIBUTE_DIRECTORY)) {
            SetCurrentDirectoryA(g_exe_parent);
        }
    }

    g_path_initialized = true;
}

static bool file_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool asset_resolve_path(const char *subpath, char *out_full_path, size_t out_size) {
    if (!subpath || !out_full_path || out_size == 0) return false;
    asset_system_init();

    // Check direct / current directory first
    if (file_exists(subpath)) {
        GetFullPathNameA(subpath, (DWORD)out_size, out_full_path, NULL);
        return true;
    }

    // Candidate prefixes
    char candidate[MAX_PATH];
    const char *prefixes[6];
    prefixes[0] = "";
    prefixes[1] = g_exe_parent[0] ? g_exe_parent : "..";
    prefixes[2] = g_exe_dir[0] ? g_exe_dir : ".";
    prefixes[3] = "..";
    prefixes[4] = "../..";
    prefixes[5] = "I:/Baldies";

    for (int i = 0; i < 6; i++) {
        if (!prefixes[i] || !prefixes[i][0]) continue;

        // Try with backslash
        snprintf(candidate, sizeof(candidate), "%s\\%s", prefixes[i], subpath);
        if (file_exists(candidate)) {
            GetFullPathNameA(candidate, (DWORD)out_size, out_full_path, NULL);
            return true;
        }

        // Try with forward slash
        snprintf(candidate, sizeof(candidate), "%s/%s", prefixes[i], subpath);
        if (file_exists(candidate)) {
            GetFullPathNameA(candidate, (DWORD)out_size, out_full_path, NULL);
            return true;
        }
    }

    return false;
}

FILE* asset_open_file(const char *subpath, const char *mode) {
    if (!subpath || !mode) return NULL;
    asset_system_init();

    char resolved[MAX_PATH];
    if (asset_resolve_path(subpath, resolved, sizeof(resolved))) {
        return fopen(resolved, mode);
    }

    // Fallback: try fopen directly
    return fopen(subpath, mode);
}

#else

void asset_system_init(void) {}

bool asset_resolve_path(const char *subpath, char *out_full_path, size_t out_size) {
    if (!subpath || !out_full_path || out_size == 0) return false;
    strncpy(out_full_path, subpath, out_size);
    out_full_path[out_size - 1] = '\0';
    return true;
}

FILE* asset_open_file(const char *subpath, const char *mode) {
    return fopen(subpath, mode);
}

#endif
