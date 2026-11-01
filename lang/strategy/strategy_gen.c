#define _CRT_SECURE_NO_WARNINGS
#include <string.h>

#define STRATEGY_MAX_BASES 64
#define STRATEGY_MAX_TEMPLATES 32
#define STRATEGY_NAME_MAX 64

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

/* Formats one .cas into buf. Each template is used once per basis. Returns 0 or -1. */
int strategy_format(char *buf, size_t cap, const char bases[], int n_bases,
                    const char *const templates[], int n_templates, const char *name) {
    size_t n = 0;
    int fn = 1;
    if (!buf || cap == 0 || !bases || n_bases < 1 || n_bases > STRATEGY_MAX_BASES || !name)
        return -1;
    if (!templates || n_templates < 1 || n_templates > STRATEGY_MAX_TEMPLATES) return -1;
    buf[0] = 0;
    if (!append(buf, cap, &n, "&") || !append(buf, cap, &n, name)) return -1;
    if (!append(buf, cap, &n, "::Strategy(bases=\"")) return -1;
    for (int i = 0; i < n_bases; i++) {
        char one[2];
        if (i && !append(buf, cap, &n, ",")) return -1;
        one[0] = bases[i];
        one[1] = 0;
        if (!append(buf, cap, &n, one)) return -1;
    }
    if (!append(buf, cap, &n, "\", templates=\"")) return -1;
    for (int t = 0; t < n_templates; t++) {
        if (t && !append(buf, cap, &n, ",")) return -1;
        if (!append(buf, cap, &n, templates[t])) return -1;
    }
    if (!append(buf, cap, &n, "\"):{\n")) return -1;
    for (int i = 0; i < n_bases; i++) {
        char one[2];
        one[0] = bases[i];
        one[1] = 0;
        for (int t = 0; t < n_templates; t++) {
            if (!append(buf, cap, &n, "    $d.") || !append(buf, cap, &n, one)) return -1;
            if (!append(buf, cap, &n, "::") || !append(buf, cap, &n, templates[t])) return -1;
            if (!append(buf, cap, &n, ":(\n        %fn_") || !append_int(buf, cap, &n, fn)) return -1;
            /* skew names the Ore field: tau (trace), sigma (twist), delta_amp. */
            if (!strcmp(templates[t], "skew")) {
                if (!append(buf, cap, &n, "::(tau, sigma, delta_amp);\n    );\n")) return -1;
            } else if (!append(buf, cap, &n, "::(...);\n    );\n")) return -1;
            fn++;
        }
    }
    if (!append(buf, cap, &n, "};\n\n")) return -1;
    fn = 1;
    for (int i = 0; i < n_bases; i++) {
        for (int t = 0; t < n_templates; t++) {
            if (!append(buf, cap, &n, "%fn_") || !append_int(buf, cap, &n, fn)) return -1;
            if (!strcmp(templates[t], "skew")) {
                if (!append(buf, cap, &n, "::(tau, sigma, delta_amp): ore;\n")) return -1;
            } else if (!append(buf, cap, &n, "::(...);\n")) return -1;
            fn++;
        }
    }
    return 0;
}

#ifndef STRATEGY_GEN_NO_HOST

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "place.h"

static int parse_bases(const char *text, char *out, int cap, char *err, size_t err_cap) {
    int n = 0;
    for (const char *p = text; *p;) {
        while (*p == ' ' || *p == ',' || *p == '{' || *p == '}') p++;
        if (!*p) break;
        if (!isalpha((unsigned char)*p) || (p[1] && p[1] != ' ' && p[1] != ',' && p[1] != '{' && p[1] != '}')) {
            snprintf(err, err_cap, "basis must be a single letter");
            return -1;
        }
        for (int i = 0; i < n; i++) {
            if (out[i] == *p) {
                snprintf(err, err_cap, "basis alphabet has duplicates");
                return -1;
            }
        }
        if (n >= cap) {
            snprintf(err, err_cap, "too many bases");
            return -1;
        }
        out[n++] = *p++;
    }
    if (!n) {
        snprintf(err, err_cap, "basis alphabet is empty");
        return -1;
    }
    return n;
}

static int check_name(const char *name, char *err, size_t err_cap) {
    if (!name || !(isalpha((unsigned char)name[0]) || name[0] == '_')) {
        snprintf(err, err_cap, "name must start with a letter or underscore");
        return -1;
    }
    for (const char *p = name; *p; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '_')) {
            snprintf(err, err_cap, "name must be letters, digits, or underscore");
            return -1;
        }
        if ((size_t)(p - name) + 1 >= STRATEGY_NAME_MAX) {
            snprintf(err, err_cap, "name is too long");
            return -1;
        }
    }
    return 0;
}

static int self_test(void) {
    char buf[4096];
    const char *expect =
        "&default::Strategy(bases=\"i,j,k\", templates=\"diff\"):{\n"
        "    $d.i::diff:(\n"
        "        %fn_1::(...);\n"
        "    );\n"
        "    $d.j::diff:(\n"
        "        %fn_2::(...);\n"
        "    );\n"
        "    $d.k::diff:(\n"
        "        %fn_3::(...);\n"
        "    );\n"
        "};\n"
        "\n"
        "%fn_1::(...);\n"
        "%fn_2::(...);\n"
        "%fn_3::(...);\n";
    const char *diff_only[] = {"diff"};
    const char *two[] = {"diff", "d"};
    if (strategy_format(buf, sizeof buf, "ijk", 3, diff_only, 1, "default") != 0) return 0;
    if (strcmp(buf, expect) != 0) return 0;
    if (strategy_format(buf, sizeof buf, "R", 1, two, 2, "default") != 0) return 0;
    if (strcmp(buf,
               "&default::Strategy(bases=\"R\", templates=\"diff,d\"):{\n"
               "    $d.R::diff:(\n"
               "        %fn_1::(...);\n"
               "    );\n"
               "    $d.R::d:(\n"
               "        %fn_2::(...);\n"
               "    );\n"
               "};\n"
               "\n"
               "%fn_1::(...);\n"
               "%fn_2::(...);\n") != 0) return 0;
    if (strategy_format(buf, sizeof buf, "RGBA", 4, diff_only, 1, "default") != 0) return 0;
    if (!strstr(buf, "$d.R::diff:(")) return 0;
    if (!strstr(buf, "%fn_1::(...);")) return 0;
    if (strstr(buf, "yield")) return 0;
    {
        const char *skew_only[] = {"skew"};
        if (strategy_format(buf, sizeof buf, "ijk", 3, skew_only, 1, "skewtrig") != 0) return 0;
        if (!strstr(buf, "$d.i::skew:(") || !strstr(buf, "$d.j::skew:(") || !strstr(buf, "$d.k::skew:(")) return 0;
        if (!strstr(buf, "%fn_1::(tau, sigma, delta_amp): ore;")) return 0;
    }
    printf("strategy_gen self-test ok\n");
    return 1;
}

static void usage(void) {
    fprintf(stderr,
            "usage: strategy_gen [--flow] [--metaflow] [--bases LIST] [--templates LIST]\n"
            "                    [--name NAME] [--dir DIR] [--stdout]\n"
            "  The default alphabet is Flow then MetaFlow.\n"
            "  --flow or --metaflow alone selects that alphabet. --bases replaces it.\n"
            "  --templates defaults to diff. Each template is used once per basis.\n"
            "  The template skew declares an Ore field (tau, sigma, delta_amp) on each $d basis.\n");
}

int main(int argc, char **argv) {
    const char *bases_s = 0;
    const char *templates_s = "diff";
    const char *name = "default";
    const char *dir_override = 0;
    int want_flow = 0, want_meta = 0, do_stdout = 0, do_test = 0;
    char bases[STRATEGY_MAX_BASES];
    char template_store[STRATEGY_MAX_TEMPLATES][STRATEGY_NAME_MAX];
    const char *templates[STRATEGY_MAX_TEMPLATES];
    char err[160];
    char *text = 0;
    int n_bases;
    size_t cap;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--self-test")) do_test = 1;
        else if (!strcmp(argv[i], "--flow")) want_flow = 1;
        else if (!strcmp(argv[i], "--metaflow")) want_meta = 1;
        else if (!strcmp(argv[i], "--stdout")) do_stdout = 1;
        else if (!strcmp(argv[i], "--bases") && i + 1 < argc) bases_s = argv[++i];
        else if (!strcmp(argv[i], "--templates") && i + 1 < argc) templates_s = argv[++i];
        else if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "--dir") && i + 1 < argc) dir_override = argv[++i];
        else {
            usage();
            return 2;
        }
    }
    if (do_test) return self_test() ? 0 : 1;
    if (!bases_s) {
        if (want_flow && !want_meta) bases_s = "i,j,k,l,m,n,o,p,q,r,s,t,e,f,g";
        else if (want_meta && !want_flow) bases_s = "R,G,B,A";
        else bases_s = "i,j,k,l,m,n,o,p,q,r,s,t,e,f,g,R,G,B,A";
    }
    n_bases = parse_bases(bases_s, bases, STRATEGY_MAX_BASES, err, sizeof err);
    int n_templates = 0;
    if (n_bases >= 0) {
        const char *p = templates_s;
        while (*p) {
            while (*p == ' ' || *p == ',') p++;
            if (!*p) break;
            if (n_templates >= STRATEGY_MAX_TEMPLATES) {
                snprintf(err, sizeof err, "too many templates");
                n_bases = -1;
                break;
            }
            int k = 0;
            if (!(isalpha((unsigned char)*p) || *p == '_')) {
                snprintf(err, sizeof err, "template must start with a letter or underscore");
                n_bases = -1;
                break;
            }
            while (*p && *p != ',' && *p != ' ') {
                if (!(isalnum((unsigned char)*p) || *p == '_') || k + 1 >= STRATEGY_NAME_MAX) {
                    snprintf(err, sizeof err, "template name is invalid");
                    n_bases = -1;
                    break;
                }
                template_store[n_templates][k++] = *p++;
            }
            if (n_bases < 0) break;
            template_store[n_templates][k] = 0;
            for (int t = 0; t < n_templates; t++) {
                if (!strcmp(template_store[t], template_store[n_templates])) {
                    snprintf(err, sizeof err, "template list has duplicates");
                    n_bases = -1;
                    break;
                }
            }
            if (n_bases < 0) break;
            templates[n_templates] = template_store[n_templates];
            n_templates++;
        }
        if (n_bases >= 0 && n_templates < 1) {
            snprintf(err, sizeof err, "template list is empty");
            n_bases = -1;
        }
    }
    if (n_bases < 0 || check_name(name, err, sizeof err) != 0) {
        fprintf(stderr, "strategy_gen: %s\n", err);
        return 1;
    }
    cap = (size_t)n_bases * (size_t)n_templates * 192 + 256;
    text = (char *)malloc(cap);
    if (!text || strategy_format(text, cap, bases, n_bases, templates, n_templates, name) != 0) {
        free(text);
        fprintf(stderr, "strategy_gen: format failed\n");
        return 1;
    }
    {
        char placed[1024], path[1200];
        FILE *fh;
        if (place_tool(placed, sizeof placed, name, dir_override) != 0) {
            free(text);
            fprintf(stderr, "strategy_gen: cannot choose an output directory\n");
            return 1;
        }
        snprintf(path, sizeof path, "%s\\%s.cas", placed, name);
        fh = fopen(path, "wb");
        if (!fh) {
            free(text);
            fprintf(stderr, "strategy_gen: cannot write %s\n", path);
            return 1;
        }
        fwrite(text, 1, strlen(text), fh);
        fclose(fh);
    }
    if (do_stdout) fputs(text, stdout);
    free(text);
    return 0;
}

#endif
