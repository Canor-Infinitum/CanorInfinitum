#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "place.h"

static int append(char *buf, size_t cap, size_t *n, const char *s) {
    while (*s) {
        if (*n + 1 >= cap) return 0;
        buf[(*n)++] = *s++;
    }
    buf[*n] = 0;
    return 1;
}

static int append_int(char *buf, size_t cap, size_t *n, int v) {
    char tmp[16], rev[16];
    int k = 0;
    if (v == 0) return append(buf, cap, n, "0");
    while (v) {
        rev[k++] = (char)('0' + v % 10);
        v /= 10;
    }
    for (int i = 0; i < k; i++) tmp[i] = rev[k - 1 - i];
    tmp[k] = 0;
    return append(buf, cap, n, tmp);
}

/* m, n, o < 0 means unbound. Returns 0 or -1. */
int action_format(char *buf, size_t cap, const char *name, int m, int n, int o) {
    size_t len = 0;
    if (!buf || !name) return -1;
    buf[0] = 0;
    if (!append(buf, cap, &len, "&") || !append(buf, cap, &len, name) || !append(buf, cap, &len, "::Action("))
        return -1;
    if (m < 0 && n < 0 && o < 0) {
        if (!append(buf, cap, &len, "m, n, o")) return -1;
    } else {
        if (m < 0) {
            if (!append(buf, cap, &len, "m")) return -1;
        } else if (!append(buf, cap, &len, "m=") || !append_int(buf, cap, &len, m)) return -1;
        if (n < 0) {
            if (!append(buf, cap, &len, ", n")) return -1;
        } else if (!append(buf, cap, &len, ", n=") || !append_int(buf, cap, &len, n)) return -1;
        if (o < 0) {
            if (!append(buf, cap, &len, ", o")) return -1;
        } else if (!append(buf, cap, &len, ", o=") || !append_int(buf, cap, &len, o)) return -1;
    }
    if (!append(buf, cap, &len, "):\"m = n (mod o)\";\n")) return -1;
    return 0;
}

#ifndef ACTION_GEN_NO_HOST

static int self_test(void) {
    char buf[256];
    if (action_format(buf, sizeof buf, "default", -1, -1, -1) != 0) return 0;
    if (strcmp(buf, "&default::Action(m, n, o):\"m = n (mod o)\";\n") != 0) return 0;
    if (action_format(buf, sizeof buf, "default", 2, 3, 4) != 0) return 0;
    if (strcmp(buf, "&default::Action(m=2, n=3, o=4):\"m = n (mod o)\";\n") != 0) return 0;
    printf("action_gen self-test ok\n");
    return 1;
}

static int check_name(const char *name) {
    if (!name || !(isalpha((unsigned char)name[0]) || name[0] == '_')) return -1;
    for (const char *p = name; *p; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '_')) return -1;
    }
    return 0;
}

static int check_param(int v) {
    return (v >= 1 && v <= 288) ? 0 : -1;
}

int main(int argc, char **argv) {
    const char *name = "default";
    const char *dir_override = 0;
    int m = -1, n = -1, o = -1, do_test = 0, do_stdout = 0;
    char buf[256];
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--self-test")) do_test = 1;
        else if (!strcmp(argv[i], "--stdout")) do_stdout = 1;
        else if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "--dir") && i + 1 < argc) dir_override = argv[++i];
        else if (!strcmp(argv[i], "--m") && i + 1 < argc) m = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--n") && i + 1 < argc) n = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--o") && i + 1 < argc) o = atoi(argv[++i]);
        else {
            fprintf(stderr, "usage: action_gen [--name NAME] [--m N] [--n N] [--o N] [--dir DIR] [--stdout]\n");
            return 2;
        }
    }
    if (do_test) return self_test() ? 0 : 1;
    if (check_name(name) != 0 || (m >= 0 && check_param(m) != 0) || (n >= 0 && check_param(n) != 0) ||
        (o >= 0 && check_param(o) != 0) || action_format(buf, sizeof buf, name, m, n, o) != 0) {
        fprintf(stderr, "action_gen: bad argument\n");
        return 1;
    }
    {
        char placed[1024], path[1200];
        FILE *fh;
        if (place_tool(placed, sizeof placed, name, dir_override) != 0) {
            fprintf(stderr, "action_gen: cannot choose an output directory\n");
            return 1;
        }
        snprintf(path, sizeof path, "%s\\%s.cas", placed, name);
        fh = fopen(path, "wb");
        if (!fh) { fprintf(stderr, "action_gen: cannot write %s\n", path); return 1; }
        fwrite(buf, 1, strlen(buf), fh);
        fclose(fh);
    }
    if (do_stdout) fputs(buf, stdout);
    return 0;
}

#endif
