#define _CRT_SECURE_NO_WARNINGS
#include "place.h"

#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static int is_dir(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static int mkdir_one(const char *path) {
    if (_mkdir(path) == 0 || errno == EEXIST) return 0;
    return -1;
}

static int mkdir_parents(const char *path) {
    char buf[1024];
    size_t n = strlen(path);
    char *start;
    if (n == 0 || n >= sizeof buf) return -1;
    memcpy(buf, path, n + 1);
    for (char *p = buf; *p; p++) if (*p == '/') *p = '\\';
    start = buf;
    if (isalpha((unsigned char)buf[0]) && buf[1] == ':') start = buf + 2;
    if (*start == '\\') start++;
    for (char *p = start; *p; p++) {
        if (*p != '\\') continue;
        *p = 0;
        if (buf[0] && mkdir_one(buf) != 0) return -1;
        *p = '\\';
    }
    return mkdir_one(buf);
}

static int join_claim(char *out, size_t cap, const char *parent, const char *name) {
    char candidate[1024];
    int wrote;
    if (mkdir_parents(parent) != 0) return -1;
    wrote = snprintf(candidate, sizeof candidate, "%s\\%s", parent, name);
    if (wrote < 0 || (size_t)wrote >= sizeof candidate) return -1;
    if (!is_dir(candidate) && GetFileAttributesA(candidate) == INVALID_FILE_ATTRIBUTES) {
        if (mkdir_one(candidate) != 0) return -1;
        wrote = snprintf(out, cap, "%s", candidate);
        return wrote < 0 || (size_t)wrote >= cap ? -1 : 0;
    }
    for (int index = 1; index < 10000; index++) {
        wrote = snprintf(candidate, sizeof candidate, "%s\\%s_%d", parent, name, index);
        if (wrote < 0 || (size_t)wrote >= sizeof candidate) return -1;
        if (GetFileAttributesA(candidate) != INVALID_FILE_ATTRIBUTES) continue;
        if (mkdir_one(candidate) != 0) return -1;
        wrote = snprintf(out, cap, "%s", candidate);
        return wrote < 0 || (size_t)wrote >= cap ? -1 : 0;
    }
    return -1;
}

static int full_join(char *out, size_t cap, const char *cwd, const char *rel) {
    char mixed[1024];
    DWORD n;
    int wrote = snprintf(mixed, sizeof mixed, "%s\\%s", cwd, rel);
    if (wrote < 0 || (size_t)wrote >= sizeof mixed) return -1;
    n = GetFullPathNameA(mixed, (DWORD)cap, out, NULL);
    return n == 0 || n >= cap ? -1 : 0;
}

int place_absolute(char *out, size_t cap, const char *path) {
    DWORD n;
    if (!out || cap < 2 || !path || !path[0]) return -1;
    n = GetFullPathNameA(path, (DWORD)cap, out, NULL);
    return n == 0 || n >= cap ? -1 : 0;
}

int place_dir(char *out, size_t cap, const char *family_dir, const char *name, const char *override, const char *cwd) {
    char parent[1024];
    if (!name || !name[0]) return -1;
    if (override && override[0]) {
        char full[1024];
        if (place_absolute(full, sizeof full, override) != 0) return -1;
        if (mkdir_parents(full) != 0) return -1;
        if (strlen(full) >= cap) return -1;
        memcpy(out, full, strlen(full) + 1);
        return 0;
    }
    (void)cwd;
    if (full_join(parent, sizeof parent, family_dir, "..\\..\\.repositorium") == 0 && is_dir(parent)) {
        return join_claim(out, cap, parent, name);
    }
    if (full_join(parent, sizeof parent, family_dir, "..\\generated") == 0 && join_claim(out, cap, parent, name) == 0) {
        return 0;
    }
    if (snprintf(parent, sizeof parent, "%s\\generated", family_dir) < (int)sizeof parent) {
        return join_claim(out, cap, parent, name);
    }
    return -1;
}

int place_tool(char *out, size_t cap, const char *name, const char *override) {
    char module_path[MAX_PATH], family[MAX_PATH], cwd[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, module_path, MAX_PATH);
    char *slash;
    DWORD cwd_n;
    size_t len;
    if (!n || n >= MAX_PATH) return -1;
    slash = module_path + n;
    while (slash > module_path && slash[-1] != '\\' && slash[-1] != '/') slash--;
    if (slash == module_path) return -1;
    len = (size_t)(slash - module_path);
    while (len > 0 && (module_path[len - 1] == '\\' || module_path[len - 1] == '/')) len--;
    if (len == 0 || len >= sizeof family) return -1;
    memcpy(family, module_path, len);
    family[len] = 0;
    cwd_n = GetCurrentDirectoryA((DWORD)sizeof cwd, cwd);
    if (cwd_n == 0 || cwd_n >= sizeof cwd) return -1;
    return place_dir(out, cap, family, name, override, cwd);
}
