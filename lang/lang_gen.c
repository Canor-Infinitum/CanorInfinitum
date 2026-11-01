#define _CRT_SECURE_NO_WARNINGS
#include <string.h>

#define LANG_NAME_MAX 64
#define LANG_EXTRA_MAX 32
#define LANG_PATH_MAX 1024

static int append(char *buf, size_t cap, size_t *n, const char *s) {
    while (*s) {
        if (*n + 1 >= cap) return 0;
        buf[(*n)++] = *s++;
    }
    buf[*n] = 0;
    return 1;
}

static const char *template_decls =
    "\\Profile:{};\n"
    "\\Settings:{};\n"
    "\\Strategy:{};\n"
    "\\Header:{};\n"
    "\\Body:{};\n"
    "\\Flow:{};\n"
    "\\MetaFlow:{};\n"
    "\n";

static int format_cap(char *buf, size_t cap, const char *name) {
    size_t n = 0;
    buf[0] = 0;
    if (!append(buf, cap, &n, template_decls)) return -1;
    if (!append(buf, cap, &n, "&") || !append(buf, cap, &n, name) || !append(buf, cap, &n, "::Profile:{\n")) return -1;
    if (!append(buf, cap, &n, "    $manifesto:\"") || !append(buf, cap, &n, name) || !append(buf, cap, &n, "\";\n")) return -1;
    if (!append(buf, cap, &n,
                "    &Strategy::Settings:{\n"
                "        $bases:\"i,j,k,l,m,n,o,p,q,r,s,t,e,f,g,R,G,B,A\";\n"
                "        $templates:\"diff,d,partial,del,nabla\";\n"
                "    };\n"
                "    &Flow::Settings:{\n"
                "        $bases:\"i,j,k,l,m,n,o,p,q,r,s,t,e,f,g\";\n"
                "        $order:1;\n"
                "        $order_min:1;\n"
                "        $order_max:288;\n"
                "        $direction:\"LTR\";\n"
                "        $count:1;\n"
                "    };\n"
                "    &MetaFlow::Settings:{\n"
                "        $bases:\"R,G,B,A\";\n"
                "        $order:1;\n"
                "        $order_min:1;\n"
                "        $order_max:288;\n"
                "        $count:1;\n"
                "    };\n"
                "    &Action::Settings:{\n"
                "        $min:1;\n"
                "        $max:288;\n"
                "    };\n"
                "    &Soul::Settings:{\n"
                "        $mip:64;\n"
                "        $scale_min:1;\n"
                "        $scale_nom:1;\n"
                "        $scale_max:255;\n"
                "    };\n};\n")) return -1;
    return 0;
}

static int format_cas(char *buf, size_t cap, const char *name, const char *const strategies[], int n_strategies,
                      const char *const extra[], int n_extra) {
    size_t n = 0;
    buf[0] = 0;
    if (!append(buf, cap, &n, template_decls)) return -1;
    if (!append(buf, cap, &n, "&") || !append(buf, cap, &n, name) || !append(buf, cap, &n, "::Settings:{\n")) return -1;
    if (!append(buf, cap, &n, "    $profile:\"") || !append(buf, cap, &n, name) || !append(buf, cap, &n, ".cap\";\n")) return -1;
    if (!append(buf, cap, &n, "    $header:\"main.cah\";\n    $body:\"main.cab\";\n")) return -1;
    if (!append(buf, cap, &n, "    $object:\"") || !append(buf, cap, &n, name) || !append(buf, cap, &n, ".cao\";\n")) return -1;
    if (!append(buf, cap, &n, "    $file:\"strategy.cas\";\n")) return -1;
    for (int i = 0; i < n_strategies; i++) {
        if (!append(buf, cap, &n, "    $file:\"") || !append(buf, cap, &n, strategies[i]) || !append(buf, cap, &n, ".cas\";\n"))
            return -1;
    }
    if (!append(buf, cap, &n, "    $file:\"flows.cah\";\n    $file:\"flows.cab\";\n")) return -1;
    if (!append(buf, cap, &n, "    $file:\"metaflows.cah\";\n    $file:\"metaflows.cab\";\n")) return -1;
    if (!append(buf, cap, &n, "    $file:\"action.cas\";\n    $file:\"soul.cah\";\n    $file:\"soul.cab\";\n")) return -1;
    for (int i = 0; i < n_extra; i++) {
        if (!append(buf, cap, &n, "    $file:\"") || !append(buf, cap, &n, extra[i]) || !append(buf, cap, &n, "\";\n"))
            return -1;
    }
    if (!append(buf, cap, &n, "};\n")) return -1;
    return 0;
}

static int format_cah(char *buf, size_t cap, const char *name) {
    size_t n = 0;
    buf[0] = 0;
    if (!append(buf, cap, &n, "&") || !append(buf, cap, &n, name) || !append(buf, cap, &n, ".main::Header:{\n")) return -1;
    if (!append(buf, cap, &n, "    &master.strategy::Strategy;\n};\n")) return -1;
    return 0;
}

#ifndef LANG_GEN_NO_HOST

#include <ctype.h>
#include <errno.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "place.h"

int soul_write(const char *svg_dir, const char *out_dir, const char *degrees);
int action_format(char *buf, size_t cap, const char *name, int m, int n, int o);

static int path_sep(char c) {
    return c == '/' || c == '\\';
}

static int path_absolute(const char *path) {
    if (!path || !path[0]) return 0;
    if (path_sep(path[0])) return 1;
    return isalpha((unsigned char)path[0]) && path[1] == ':';
}

static const char *path_name(const char *path) {
    const char *name = path ? path : "";
    for (const char *p = name; *p; p++) if (path_sep(*p)) name = p + 1;
    return name;
}

static int mkdir_parents(const char *path) {
    char buf[LANG_PATH_MAX];
    size_t n = path ? strlen(path) : 0;
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
        if (buf[0] && _mkdir(buf) != 0 && errno != EEXIST) return -1;
        *p = '\\';
    }
    if (_mkdir(buf) != 0 && errno != EEXIST) return -1;
    return 0;
}

static int path_join(char *out, size_t cap, const char *dir, const char *name) {
    int wrote;
    if (path_absolute(name)) wrote = snprintf(out, cap, "%s", name);
    else wrote = snprintf(out, cap, "%s\\%s", dir, name);
    return wrote < 0 || (size_t)wrote >= cap ? -1 : 0;
}

static int write_text(const char *path, const char *text) {
    char parent[LANG_PATH_MAX];
    const char *name = path_name(path);
    FILE *fh;
    size_t n;
    if (!name || name == path) {
        parent[0] = '.';
        parent[1] = 0;
    } else {
        n = (size_t)(name - path);
        if (n >= sizeof parent) return -1;
        memcpy(parent, path, n);
        parent[n] = 0;
        while (n > 0 && path_sep(parent[n - 1])) parent[--n] = 0;
        if (n == 2 && parent[1] == ':') {
            parent[2] = '\\';
            parent[3] = 0;
        }
    }
    if (parent[0] && mkdir_parents(parent) != 0) {
        fprintf(stderr, "lang_gen: cannot create %s\n", parent);
        return -1;
    }
    fh = fopen(path, "wb");
    if (!fh) {
        fprintf(stderr, "lang_gen: cannot write %s\n", path);
        return -1;
    }
    n = strlen(text);
    if (fwrite(text, 1, n, fh) != n) {
        fprintf(stderr, "lang_gen: cannot write %s\n", path);
        fclose(fh);
        return -1;
    }
    fclose(fh);
    return 0;
}

#include "flow_gen.h"

int strategy_format(char *buf, size_t cap, const char bases[], int n_bases,
                    const char *const templates[], int n_templates, const char *name);

static int write_words(const char *dir, const char *stem, const char *bases_text, int order,
                       const char *atom, const char *kind) {
    FlowSession session;
    char err[160], card[FLOW_DEC_MAX], index_s[FLOW_DEC_MAX], count_s[FLOW_DEC_MAX];
    char cah_path[LANG_PATH_MAX], cab_path[LANG_PATH_MAX];
    FILE *cah, *cab;
    if (flow_session_begin(&session, bases_text, order, "0", "all", "LTR", atom, err, sizeof err) != 0) return -1;
    if (flow_session_cardinality(&session, card, sizeof card) != 0) return -1;
    if (flow_big_to_dec(&session.count, count_s, sizeof count_s) != 0) return -1;
    if (path_join(cah_path, sizeof cah_path, dir, stem) != 0) return -1;
    {
        size_t used = strlen(cah_path);
        if (used + 4 >= sizeof cah_path) return -1;
        memcpy(cah_path + used, ".cah", 5);
    }
    if (path_join(cab_path, sizeof cab_path, dir, stem) != 0) return -1;
    {
        size_t used = strlen(cab_path);
        if (used + 4 >= sizeof cab_path) return -1;
        memcpy(cab_path + used, ".cab", 5);
    }
    {
        char parent[LANG_PATH_MAX];
        const char *leaf = path_name(cah_path);
        size_t used = (size_t)(leaf - cah_path);
        if (used >= sizeof parent) return -1;
        memcpy(parent, cah_path, used);
        parent[used] = 0;
        if (parent[0] && mkdir_parents(parent) != 0) return -1;
    }
    cah = fopen(cah_path, "wb");
    cab = fopen(cab_path, "wb");
    if (!cah || !cab) {
        if (cah) fclose(cah);
        if (cab) fclose(cab);
        return -1;
    }
    fprintf(cah, "@%s::%s.%d.0.%s:{\n    $bases:{ ", atom, kind, order, count_s);
    for (int i = 0; i < session.n_bases; i++) fprintf(cah, "%s%c", i ? ", " : "", session.bases[i]);
    fprintf(cah, " };\n    $direction:\"LTR\";\n    $symbolic:\"0,0,0,0,0,0,0,0\";\n    $cardinality:%s;\n};\n", card);
    while (session.active) {
        if (flow_big_to_dec(&session.cursor, index_s, sizeof index_s) != 0) break;
        fprintf(cab, "@%s::%s.%d.%s.%s:{ ", atom, kind, order, index_s, count_s);
        for (int i = 0; i < session.order; i++) fprintf(cab, "%s%c", i ? ", " : "", session.bases[session.digits[i]]);
        fprintf(cab, " };\n");
        if (flow_session_advance(&session) < 0) break;
    }
    fclose(cah);
    fclose(cab);
    return 0;
}

static int write_strategy(const char *dir, const char *filename, const char *name) {
    static const char bases[] = "ijklmnopqrstefgRGBA";
    static const char *templates[] = {"diff", "d", "partial", "del", "nabla"};
    char *text;
    char path[LANG_PATH_MAX];
    size_t cap = 19 * 5 * 220 + 512;
    text = (char *)malloc(cap);
    if (!text || strategy_format(text, cap, bases, 19, templates, 5, name) != 0) {
        free(text);
        return -1;
    }
    if (path_join(path, sizeof path, dir, filename) != 0 || write_text(path, text) != 0) {
        free(text);
        return -1;
    }
    free(text);
    return 0;
}

static int write_main_cab(const char *dir, const char *name) {
    static const char bases[] = "ijklmnopqrstefgRGBA";
    static const char *templates[] = {"diff", "d", "partial", "del", "nabla"};
    char *text;
    char path[LANG_PATH_MAX];
    FILE *fh;
    size_t cap = 19 * 5 * 220 + 512;
    text = (char *)malloc(cap);
    if (!text || strategy_format(text, cap, bases, 19, templates, 5, "master.strategy") != 0) {
        free(text);
        return -1;
    }
    if (path_join(path, sizeof path, dir, "main.cab") != 0) {
        free(text);
        return -1;
    }
    {
        char parent[LANG_PATH_MAX];
        const char *leaf = path_name(path);
        size_t used = leaf == path ? 0 : (size_t)(leaf - path);
        if (used >= sizeof parent) {
            free(text);
            return -1;
        }
        if (used == 0) {
            parent[0] = '.';
            parent[1] = 0;
        } else {
            memcpy(parent, path, used);
            parent[used] = 0;
        }
        if (mkdir_parents(parent) != 0) {
            free(text);
            return -1;
        }
    }
    fh = fopen(path, "wb");
    if (!fh) {
        free(text);
        return -1;
    }
    fprintf(fh, "&%s.main::Body:{\n", name);
    fputs("    ", fh);
    for (char *p = text; *p; p++) {
        fputc(*p, fh);
        if (*p == '\n' && p[1]) fputs("    ", fh);
    }
    fputs("};\n", fh);
    fclose(fh);
    free(text);
    return 0;
}

static int self_test(void) {
    char cap[4096], cas[4096], cah[512];
    const char *extra[] = {"notes.cah"};
    if (format_cap(cap, sizeof cap, "project") != 0) return 0;
    if (!strstr(cap, "\\Profile:{};")) return 0;
    if (!strstr(cap, "\\MetaFlow:{};")) return 0;
    if (strstr(cap, "$toolchain")) return 0;
    if (!strstr(cap, "$manifesto:\"project\";")) return 0;
    if (!strstr(cap, "$mip:64;")) return 0;
    if (!strstr(cap, "$scale_nom:1;")) return 0;
    if (!strstr(cap, "$order_max:288;")) return 0;
    if (format_cas(cas, sizeof cas, "project", 0, 0, extra, 1) != 0) return 0;
    if (!strstr(cas, "\\Settings:{};")) return 0;
    if (!strstr(cas, "$object:\"project.cao\";")) return 0;
    if (!strstr(cas, "$file:\"notes.cah\";")) return 0;
    if (format_cah(cah, sizeof cah, "project") != 0) return 0;
    if (strcmp(cah, "&project.main::Header:{\n    &master.strategy::Strategy;\n};\n") != 0) return 0;
    printf("lang_gen self-test ok\n");
    return 1;
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
    }
    return 0;
}

static int empty_source(const char *path, char *buf, size_t cap, char *err, size_t err_cap) {
    const char *base = path_name(path);
    const char *dot = strrchr(base, '.');
    const char *kind = 0;
    char stem[LANG_NAME_MAX];
    size_t n;
    if (!dot) {
        snprintf(err, err_cap, "extra file must end with .cap, .cas, .cah, or .cab");
        return -1;
    }
    if (!strcmp(dot, ".cap")) kind = "Profile";
    else if (!strcmp(dot, ".cas")) kind = "Settings";
    else if (!strcmp(dot, ".cah")) kind = "Header";
    else if (!strcmp(dot, ".cab")) kind = "Body";
    else {
        snprintf(err, err_cap, "extra file must end with .cap, .cas, .cah, or .cab");
        return -1;
    }
    n = (size_t)(dot - base);
    if (n == 0 || n >= LANG_NAME_MAX) {
        snprintf(err, err_cap, "file stem is invalid");
        return -1;
    }
    memcpy(stem, base, n);
    stem[n] = 0;
    if (check_name(stem, err, err_cap) != 0) return -1;
    snprintf(buf, cap, "&%s::%s:{};\n", stem, kind);
    return 0;
}

static int check_source(const char *path, char *err, size_t err_cap) {
    const char *dot = strrchr(path, '.');
    if (!dot || (strcmp(dot, ".cap") && strcmp(dot, ".cas") && strcmp(dot, ".cah") && strcmp(dot, ".cab"))) {
        snprintf(err, err_cap, "extra file must end with .cap, .cas, .cah, or .cab");
        return -1;
    }
    return 0;
}

static int write_out(const char *dir, const char *filename, const char *text) {
    char path[LANG_PATH_MAX];
    if (path_join(path, sizeof path, dir, filename) != 0) return -1;
    return write_text(path, text);
}

static void usage(void) {
    fprintf(stderr,
            "usage: lang_gen --default [--name NAME] [--dir DIR] [--strategy NAME]...\n"
            "       lang_gen --language strategy|flow|metaflow|action|soul...\n"
            "       lang_gen --file PATH...\n"
            "  --default writes the project files and every language default.\n"
            "  --language writes that language's files with their default contents.\n"
            "  main.cah keeps &NAME.main::Master::Strategy.\n"
            "  strategy.cas is the default strategy and holds its functional definitions.\n"
            "  --strategy NAME writes another NAME.cas with its own functional definitions.\n"
            "  flows are order-3, count all. metaflows are order-4, count all.\n"
            "  --file writes an empty Profile, Settings, Header, or Body file.\n"
            "  NAME.cao is declared as the future object and is not written.\n");
}

int main(int argc, char **argv) {
    const char *name = "project";
    const char *dir_arg = 0;
    const char *extra[LANG_EXTRA_MAX];
    const char *strategies[LANG_EXTRA_MAX];
    int n_extra = 0, n_strategies = 0, do_test = 0, do_default = 0;
    int want_strategy = 0, want_flow = 0, want_meta = 0, want_action = 0, want_soul = 0;
    char err[160];
    char cap[4096], cas[4096], cah[512];
    char cap_name[LANG_NAME_MAX + 8], cas_name[LANG_NAME_MAX + 8];
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--self-test")) do_test = 1;
        else if (!strcmp(argv[i], "--default")) do_default = 1;
        else if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "--dir") && i + 1 < argc) dir_arg = argv[++i];
        else if (!strcmp(argv[i], "--strategy") && i + 1 < argc) {
            if (n_strategies >= LANG_EXTRA_MAX) {
                fprintf(stderr, "lang_gen: too many strategies\n");
                return 1;
            }
            strategies[n_strategies++] = argv[++i];
        } else if (!strcmp(argv[i], "--language") && i + 1 < argc) {
            const char *language = argv[++i];
            if (!strcmp(language, "strategy")) want_strategy = 1;
            else if (!strcmp(language, "flow")) want_flow = 1;
            else if (!strcmp(language, "metaflow")) want_meta = 1;
            else if (!strcmp(language, "action")) want_action = 1;
            else if (!strcmp(language, "soul")) want_soul = 1;
            else {
                fprintf(stderr, "lang_gen: language must be strategy, flow, metaflow, action, or soul\n");
                return 1;
            }
        } else if (!strcmp(argv[i], "--file") && i + 1 < argc) {
            if (n_extra >= LANG_EXTRA_MAX) {
                fprintf(stderr, "lang_gen: too many extra files\n");
                return 1;
            }
            extra[n_extra++] = argv[++i];
        } else {
            usage();
            return 2;
        }
    }
    if (do_test) return self_test() ? 0 : 1;
    if (!do_default && n_extra == 0 && !want_strategy && !want_flow && !want_meta && !want_action && !want_soul) {
        do_default = 1;
    }
    char placed[1024];
    const char *dir = dir_arg;
    if (dir_arg) {
        if (place_absolute(placed, sizeof placed, dir_arg) != 0) {
            fprintf(stderr, "lang_gen: cannot resolve %s\n", dir_arg);
            return 1;
        }
        dir = placed;
    } else if (place_tool(placed, sizeof placed, name, 0) != 0) {
        fprintf(stderr, "lang_gen: cannot choose an output directory\n");
        return 1;
    } else {
        dir = placed;
    }
    if (check_name(name, err, sizeof err) != 0) {
        fprintf(stderr, "lang_gen: %s\n", err);
        return 1;
    }
    for (int i = 0; i < n_strategies; i++) {
        if (check_name(strategies[i], err, sizeof err) != 0) {
            fprintf(stderr, "lang_gen: %s\n", err);
            return 1;
        }
    }
    for (int i = 0; i < n_extra; i++) {
        if (check_source(extra[i], err, sizeof err) != 0) {
            fprintf(stderr, "lang_gen: %s\n", err);
            return 1;
        }
    }
    if (do_default) {
        if (format_cap(cap, sizeof cap, name) != 0 ||
            format_cas(cas, sizeof cas, name, strategies, n_strategies, extra, n_extra) != 0 ||
            format_cah(cah, sizeof cah, name) != 0) {
            fprintf(stderr, "lang_gen: format failed\n");
            return 1;
        }
        snprintf(cap_name, sizeof cap_name, "%s.cap", name);
        snprintf(cas_name, sizeof cas_name, "%s.cas", name);
        if (write_out(dir, cap_name, cap) != 0 || write_out(dir, cas_name, cas) != 0 || write_out(dir, "main.cah", cah) != 0 ||
            write_strategy(dir, "strategy.cas", name) != 0 || write_main_cab(dir, name) != 0) {
            fprintf(stderr, "lang_gen: cannot write into %s\n", dir);
            return 1;
        }
        for (int i = 0; i < n_strategies; i++) {
            char filename[LANG_NAME_MAX + 8];
            snprintf(filename, sizeof filename, "%s.cas", strategies[i]);
            if (write_strategy(dir, filename, strategies[i]) != 0) {
                fprintf(stderr, "lang_gen: cannot write into %s\n", dir);
                return 1;
            }
        }
        if (write_words(dir, "flows", "i,j,k,l,m,n,o,p,q,r,s,t,e,f,g", 3, "flow", "Flow") != 0 ||
            write_words(dir, "metaflows", "R,G,B,A", 4, "metaflow", "MetaFlow") != 0) {
            fprintf(stderr, "lang_gen: cannot write into %s\n", dir);
            return 1;
        }
    }
    if (do_default || want_action) {
        char action[128];
        if (action_format(action, sizeof action, "default", -1, -1, -1) != 0 || write_out(dir, "action.cas", action) != 0) {
            fprintf(stderr, "lang_gen: cannot write action.cas\n");
            return 1;
        }
    }
    if (!do_default && want_strategy && write_strategy(dir, "strategy.cas", name) != 0) {
        fprintf(stderr, "lang_gen: cannot write strategy.cas\n");
        return 1;
    }
    if (!do_default && want_flow && write_words(dir, "flows", "i,j,k,l,m,n,o,p,q,r,s,t,e,f,g", 3, "flow", "Flow") != 0) {
        fprintf(stderr, "lang_gen: cannot write flows\n");
        return 1;
    }
    if (!do_default && want_meta && write_words(dir, "metaflows", "R,G,B,A", 4, "metaflow", "MetaFlow") != 0) {
        fprintf(stderr, "lang_gen: cannot write metaflows\n");
        return 1;
    }
    if (do_default || want_soul) {
        char home[MAX_PATH], svg_dir[LANG_PATH_MAX];
        DWORD got = GetModuleFileNameA(NULL, home, MAX_PATH);
        char *slash;
        if (!got || got >= MAX_PATH) {
            fprintf(stderr, "lang_gen: cannot locate soul cornerstones\n");
            return 1;
        }
        slash = home + got;
        while (slash > home && slash[-1] != '\\' && slash[-1] != '/') slash--;
        *slash = 0;
        if (snprintf(svg_dir, sizeof svg_dir, "%ssoul\\default", home) >= (int)sizeof svg_dir) {
            fprintf(stderr, "lang_gen: soul path is too long\n");
            return 1;
        }
        {
            char svg_abs[LANG_PATH_MAX];
            DWORD attr;
            if (place_absolute(svg_abs, sizeof svg_abs, svg_dir) != 0) {
                fprintf(stderr, "lang_gen: cannot resolve %s\n", svg_dir);
                return 1;
            }
            attr = GetFileAttributesA(svg_abs);
            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                fprintf(stderr, "lang_gen: soul cornerstones are not at %s\n", svg_abs);
                return 1;
            }
            memcpy(svg_dir, svg_abs, strlen(svg_abs) + 1);
        }
        if (soul_write(svg_dir, dir, 0) != 0) {
            fprintf(stderr, "lang_gen: cannot write soul cornerstones into %s\n", dir);
            return 1;
        }
    }
    for (int i = 0; i < n_extra; i++) {
        char body[128];
        if (empty_source(extra[i], body, sizeof body, err, sizeof err) != 0 || write_out(dir, extra[i], body) != 0) {
            fprintf(stderr, "lang_gen: %s\n", err[0] ? err : "cannot write file");
            return 1;
        }
    }
    return 0;
}

#endif
