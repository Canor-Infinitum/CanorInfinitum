#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <windows.h>
#include "place.h"

#define SOUL_PATH_MAX 1024

static int path_sep(char c) {
    return c == '/' || c == '\\';
}

static int mkdir_parents(const char *path) {
    char buf[SOUL_PATH_MAX];
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

static int exe_dir(char *out, size_t cap) {
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
    char *slash;
    size_t len;
    if (!n || n >= MAX_PATH) return -1;
    slash = buf + n;
    while (slash > buf && !path_sep(slash[-1])) slash--;
    if (slash == buf) return -1;
    len = (size_t)(slash - buf);
    while (len > 0 && path_sep(buf[len - 1])) len--;
    if (len == 0 || len >= cap) return -1;
    memcpy(out, buf, len);
    out[len] = 0;
    return 0;
}

static int join2(char *out, size_t cap, const char *dir, const char *name) {
    int wrote = snprintf(out, cap, "%s\\%s", dir, name);
    return wrote < 0 || (size_t)wrote >= cap ? -1 : 0;
}

#define SOUL_MIP 64
#define SOUL_SRC 20

static int append(char *buf, size_t cap, size_t *n, const char *s) {
    while (*s) {
        if (*n + 1 >= cap) return 0;
        buf[(*n)++] = *s++;
    }
    buf[*n] = 0;
    return 1;
}

int soul_header(char *buf, size_t cap) {
    size_t n = 0;
    buf[0] = 0;
    if (!append(buf, cap, &n, "&soul::Header:{\n    $cornerstones:\"")) return -1;
    for (int i = 1; i <= 26; i++) {
        char name[8];
        snprintf(name, sizeof name, "%sCS%02d", i == 1 ? "" : ",", i);
        if (!append(buf, cap, &n, name)) return -1;
    }
    if (!append(buf, cap, &n, "\";\n};\n")) return -1;
    return 0;
}

int soul_body(char *buf, size_t cap) {
    size_t n = 0;
    buf[0] = 0;
    for (int i = 1; i <= 26; i++) {
        char line[160];
        snprintf(line, sizeof line,
                 "&CS%02d::Soul:{\n    &CS%02d.strategy::Strategy(bases=\"i\", templates=\"diff\");\n"
                 "    $vector:\"default/CS%02d.svg\";\n};\n",
                 i, i, i);
        if (!append(buf, cap, &n, line)) return -1;
    }
    return 0;
}

static void fill_rect(unsigned char *mask, int x, int y, int w, int h) {
    if (w < 0) {
        x += w;
        w = -w;
    }
    if (h < 0) {
        y += h;
        h = -h;
    }
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            if (xx >= 0 && yy >= 0 && xx < SOUL_SRC && yy < SOUL_SRC) mask[yy * SOUL_SRC + xx] = 1;
        }
    }
}

static int svg_mask(const char *text, unsigned char *mask) {
    const char *d = strstr(text, "d=\"");
    memset(mask, 0, SOUL_SRC * SOUL_SRC);
    if (!d) return -1;
    d += 3;
    while (*d && *d != '"') {
        int x, y, w, h, back;
        if (*d == 'M' && sscanf(d, "M%d %dh%dv%dh%dz", &x, &y, &w, &h, &back) == 5) fill_rect(mask, x, y, w, h);
        d++;
    }
    return 0;
}

static void fit_mask(const unsigned char *src, int src_side, unsigned char *dst) {
    for (int y = 0; y < SOUL_MIP; y++) {
        int sy = y * src_side / SOUL_MIP;
        for (int x = 0; x < SOUL_MIP; x++) {
            int sx = x * src_side / SOUL_MIP;
            dst[y * SOUL_MIP + x] = src[sy * src_side + sx];
        }
    }
}

static int action_byte(const char *spec, int x, int y, int channel, int scale) {
    int result;
    if (!spec) return 255;
    if (!strcmp(spec, "m = n (mod o)")) result = (x % channel) == (y % channel);
    else if (!strncmp(spec, "(mod ", 5)) {
        int mod = atoi(spec + 5);
        if (mod < 1) return -1;
        result = (x + y) % mod;
    } else return -1;
    result *= scale;
    return result > 255 ? 255 : result;
}

int soul_raster(const unsigned char *fitted, const char *action, int scale, unsigned char *mip) {
    memset(mip, 0, SOUL_MIP * SOUL_MIP * 4);
    for (int y = 0; y < SOUL_MIP; y++) {
        for (int x = 0; x < SOUL_MIP; x++) {
            if (!fitted[y * SOUL_MIP + x]) continue;
            for (int c = 0; c < 4; c++) {
                int byte = action_byte(action, x, y, c + 1, scale);
                if (byte < 0) return -1;
                mip[(y * SOUL_MIP + x) * 4 + c] = (unsigned char)byte;
            }
        }
    }
    return 0;
}

static int label_components(const unsigned char *mask, int comp[], int *ncomp) {
    int seen[SOUL_SRC * SOUL_SRC];
    memset(seen, 0, sizeof seen);
    *ncomp = 0;
    for (int i = 0; i < SOUL_SRC * SOUL_SRC; i++) {
        int stack[SOUL_SRC * SOUL_SRC];
        int sp = 0;
        if (!mask[i] || seen[i]) continue;
        if (*ncomp >= 8) return -1;
        stack[sp++] = i;
        seen[i] = 1;
        while (sp) {
            int here = stack[--sp];
            int x = here % SOUL_SRC, y = here / SOUL_SRC;
            comp[here] = *ncomp + 1;
            for (int k = 0; k < 4; k++) {
                int nx = x + (k == 0 ? -1 : k == 1 ? 1 : 0);
                int ny = y + (k == 2 ? -1 : k == 3 ? 1 : 0);
                int nxt;
                if (nx < 0 || ny < 0 || nx >= SOUL_SRC || ny >= SOUL_SRC) continue;
                nxt = ny * SOUL_SRC + nx;
                if (mask[nxt] && !seen[nxt]) {
                    seen[nxt] = 1;
                    stack[sp++] = nxt;
                }
            }
        }
        (*ncomp)++;
    }
    return 0;
}

static void paint_direction(const unsigned char *mask, int axis, int ascending, unsigned char *mip) {
    int comp[SOUL_SRC * SOUL_SRC];
    int ncomp = 0;
    long sum[8][2];
    int count[8];
    long num = 0, den = 1, max_d = 1;
    memset(comp, 0, sizeof comp);
    memset(sum, 0, sizeof sum);
    memset(count, 0, sizeof count);
    label_components(mask, comp, &ncomp);
    for (int y = 0; y < SOUL_SRC; y++) {
        for (int x = 0; x < SOUL_SRC; x++) {
            int id = comp[y * SOUL_SRC + x];
            if (!id) continue;
            sum[id][0] += x;
            sum[id][1] += y;
            count[id]++;
        }
    }
    if (ncomp == 2 && count[1] && count[2]) {
        num = sum[1][axis] * count[2] + sum[2][axis] * count[1];
        den = 2L * count[1] * count[2];
        max_d = 1;
        for (int y = 0; y < SOUL_SRC; y++) {
            for (int x = 0; x < SOUL_SRC; x++) {
                long coord, dist;
                if (!comp[y * SOUL_SRC + x]) continue;
                coord = (axis == 0 ? x : y);
                dist = coord * den - num;
                if (dist < 0) dist = -dist;
                if (dist > max_d) max_d = dist;
            }
        }
    }
    memset(mip, 0, SOUL_MIP * SOUL_MIP * 4);
    for (int y = 0; y < SOUL_MIP; y++) {
        int sy = y * SOUL_SRC / SOUL_MIP;
        for (int x = 0; x < SOUL_MIP; x++) {
            int sx = x * SOUL_SRC / SOUL_MIP;
            int value, pixel;
            if (!mask[sy * SOUL_SRC + sx]) continue;
            if (ncomp == 2) {
                long coord = (axis == 0 ? sx : sy);
                long dist = coord * den - num;
                if (dist < 0) dist = -dist;
                value = (int)(((ascending ? (max_d - dist) : dist) * 252) / max_d);
            } else {
                int coord = axis == 0 ? x : y;
                value = (ascending ? coord : (SOUL_MIP - 1 - coord)) * 4;
            }
            pixel = (y * SOUL_MIP + x) * 4;
            mip[pixel] = mip[pixel + 1] = mip[pixel + 2] = (unsigned char)value;
            mip[pixel + 3] = 255;
        }
    }
}

int soul_mip_svg(const char *svg, const char *action, int scale, unsigned char *mip) {
    unsigned char mask[SOUL_SRC * SOUL_SRC];
    unsigned char fitted[SOUL_MIP * SOUL_MIP];
    if (svg_mask(svg, mask) != 0) return -1;
    fit_mask(mask, SOUL_SRC, fitted);
    return soul_raster(fitted, action, scale, mip);
}

unsigned char *raster_bitmap(int width, int height, const unsigned char *rgba, size_t *out_n);
unsigned char *raster_png(int width, int height, const unsigned char *rgba, size_t *out_n);

#ifndef SOUL_GEN_NO_HOST

static int self_test(void) {
    char header[4096], body[8192];
    unsigned char mip[SOUL_MIP * SOUL_MIP * 4];
    FILE *fh;
    char *svg;
    long n;
    int offset = (30 * SOUL_MIP + 0) * 4;
    if (soul_header(header, sizeof header) != 0) return 0;
    if (!strstr(header, "$cornerstones:\"CS01,CS02,")) return 0;
    if (strstr(header, "x.asc")) return 0;
    if (soul_body(body, sizeof body) != 0) return 0;
    if (strstr(body, "::Flow") || strstr(body, "::MetaFlow")) return 0;
    char exe[SOUL_PATH_MAX], svg_path[SOUL_PATH_MAX];
    if (exe_dir(exe, sizeof exe) != 0) return 0;
    if (join2(svg_path, sizeof svg_path, exe, "default\\CS01.svg") != 0) return 0;
    fh = fopen(svg_path, "rb");
    if (!fh) return 0;
    fseek(fh, 0, SEEK_END);
    n = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    svg = (char *)malloc((size_t)n + 1);
    if (!svg || fread(svg, 1, (size_t)n, fh) != (size_t)n) {
        free(svg);
        fclose(fh);
        return 0;
    }
    svg[n] = 0;
    fclose(fh);
    if (soul_mip_svg(svg, 0, 1, mip) != 0) {
        free(svg);
        return 0;
    }
    if (mip[offset] != 255 || mip[offset + 1] != 255 || mip[offset + 2] != 255 || mip[offset + 3] != 255) {
        free(svg);
        return 0;
    }
    if (mip[0] != 0) {
        free(svg);
        return 0;
    }
    if (soul_mip_svg(svg, "(mod 255)", 1, mip) != 0 || mip[offset] != (unsigned char)((0 + 30) % 255)) {
        free(svg);
        return 0;
    }
    free(svg);
    printf("soul_gen self-test ok\n");
    return 1;
}

#endif

static const int SOUL_COS1024[91] = {
    1024, 1024, 1023, 1023, 1022, 1020, 1018, 1016, 1014, 1011, 1008, 1005, 1002, 998, 994, 989, 984, 979, 974, 968,
    962, 956, 949, 943, 935, 928, 920, 912, 904, 896, 887, 878, 868, 859, 849, 839, 828, 818, 807, 796, 784, 773,
    761, 749, 737, 724, 711, 698, 685, 672, 658, 644, 630, 616, 602, 587, 573, 558, 543, 527, 512, 496, 481, 465,
    449, 433, 416, 400, 384, 367, 350, 333, 316, 299, 282, 265, 248, 230, 213, 195, 178, 160, 143, 125, 107, 89, 71, 54, 36, 18, 0
};

static int norm_degrees(int degrees) {
    int d = degrees % 360;
    return d < 0 ? d + 360 : d;
}

static int cos1024(int degrees) {
    int d = norm_degrees(degrees);
    if (d <= 90) return SOUL_COS1024[d];
    if (d <= 180) return -SOUL_COS1024[180 - d];
    if (d <= 270) return -SOUL_COS1024[d - 180];
    return SOUL_COS1024[360 - d];
}

static void paint_degrees(const unsigned char *mask, int degrees, unsigned char *mip) {
    int comp[SOUL_SRC * SOUL_SRC];
    int ncomp = 0;
    long sum[8][2];
    int count[8];
    int cosine = cos1024(degrees);
    int sine = cos1024(90 - degrees);
    memset(comp, 0, sizeof comp);
    memset(sum, 0, sizeof sum);
    memset(count, 0, sizeof count);
    memset(mip, 0, SOUL_MIP * SOUL_MIP * 4);
    label_components(mask, comp, &ncomp);
    for (int y = 0; y < SOUL_SRC; y++) {
        for (int x = 0; x < SOUL_SRC; x++) {
            int id = comp[y * SOUL_SRC + x];
            if (!id) continue;
            sum[id][0] += x;
            sum[id][1] += y;
            count[id]++;
        }
    }
    if (ncomp == 2 && count[1] && count[2]) {
        long den = 2L * count[1] * count[2];
        long numx = sum[1][0] * count[2] + sum[2][0] * count[1];
        long numy = sum[1][1] * count[2] + sum[2][1] * count[1];
        long center = numx * cosine + numy * sine;
        long max_d = 1;
        for (int y = 0; y < SOUL_SRC; y++) {
            for (int x = 0; x < SOUL_SRC; x++) {
                long dist;
                if (!comp[y * SOUL_SRC + x]) continue;
                dist = (x * cosine + y * sine) * den - center;
                if (dist < 0) dist = -dist;
                if (dist > max_d) max_d = dist;
            }
        }
        for (int y = 0; y < SOUL_MIP; y++) {
            int sy = y * SOUL_SRC / SOUL_MIP;
            for (int x = 0; x < SOUL_MIP; x++) {
                int sx = x * SOUL_SRC / SOUL_MIP;
                long dist;
                int value, pixel;
                if (!mask[sy * SOUL_SRC + sx]) continue;
                dist = (sx * (long)cosine + sy * (long)sine) * den - center;
                if (dist < 0) dist = -dist;
                value = (int)((max_d - dist) * 252 / max_d);
                pixel = (y * SOUL_MIP + x) * 4;
                mip[pixel] = mip[pixel + 1] = mip[pixel + 2] = (unsigned char)value;
                mip[pixel + 3] = 255;
            }
        }
        return;
    }
    {
        long low = 0, high = 0;
        int any = 0;
        for (int y = 0; y < SOUL_MIP; y++) {
            int sy = y * SOUL_SRC / SOUL_MIP;
            for (int x = 0; x < SOUL_MIP; x++) {
                long proj;
                if (!mask[sy * SOUL_SRC + x * SOUL_SRC / SOUL_MIP]) continue;
                proj = x * (long)cosine + y * (long)sine;
                if (!any || proj < low) low = proj;
                if (!any || proj > high) high = proj;
                any = 1;
            }
        }
        if (!any) return;
        if (high == low) high = low + 1;
        for (int y = 0; y < SOUL_MIP; y++) {
            int sy = y * SOUL_SRC / SOUL_MIP;
            for (int x = 0; x < SOUL_MIP; x++) {
                long proj;
                int value, pixel;
                if (!mask[sy * SOUL_SRC + x * SOUL_SRC / SOUL_MIP]) continue;
                proj = x * (long)cosine + y * (long)sine;
                value = (int)((proj - low) * 252 / (high - low));
                pixel = (y * SOUL_MIP + x) * 4;
                mip[pixel] = mip[pixel + 1] = mip[pixel + 2] = (unsigned char)value;
                mip[pixel + 3] = 255;
            }
        }
    }
}

static int emit_files(const char *picture_dir, const char *bmp_dir, const char *stem, const unsigned char *mip) {
    char path[SOUL_PATH_MAX], leaf[64];
    unsigned char *bmp;
    unsigned char *png;
    size_t bmp_n = 0, png_n = 0;
    FILE *fh;
    snprintf(leaf, sizeof leaf, "%s.mip", stem);
    if (join2(path, sizeof path, picture_dir, leaf) != 0) return -1;
    fh = fopen(path, "wb");
    if (!fh) return -1;
    fwrite(mip, 1, SOUL_MIP * SOUL_MIP * 4, fh);
    fclose(fh);
    bmp = raster_bitmap(SOUL_MIP, SOUL_MIP, mip, &bmp_n);
    png = raster_png(SOUL_MIP, SOUL_MIP, mip, &png_n);
    if (!bmp || !png) {
        free(bmp);
        free(png);
        return -1;
    }
    snprintf(leaf, sizeof leaf, "%s.bmp", stem);
    if (join2(path, sizeof path, bmp_dir, leaf) != 0) {
        free(bmp);
        free(png);
        return -1;
    }
    fh = fopen(path, "wb");
    if (!fh) {
        free(bmp);
        free(png);
        return -1;
    }
    fwrite(bmp, 1, bmp_n, fh);
    fclose(fh);
    free(bmp);
    snprintf(leaf, sizeof leaf, "%s.png", stem);
    if (join2(path, sizeof path, picture_dir, leaf) != 0) {
        free(png);
        return -1;
    }
    fh = fopen(path, "wb");
    if (!fh) {
        free(png);
        return -1;
    }
    fwrite(png, 1, png_n, fh);
    fclose(fh);
    free(png);
    return 0;
}

static void paint_plain(const unsigned char *mask, unsigned char *mip) {
    memset(mip, 0, SOUL_MIP * SOUL_MIP * 4);
    for (int y = 0; y < SOUL_MIP; y++) {
        int sy = y * SOUL_SRC / SOUL_MIP;
        for (int x = 0; x < SOUL_MIP; x++) {
            int pixel;
            if (!mask[sy * SOUL_SRC + x * SOUL_SRC / SOUL_MIP]) continue;
            pixel = (y * SOUL_MIP + x) * 4;
            mip[pixel] = mip[pixel + 1] = mip[pixel + 2] = mip[pixel + 3] = 255;
        }
    }
}

static int parse_degree_spec(const char *spec, int labels[], int cap, int *count) {
    int start = 0, stop = 0, n = 0;
    char extra = 0;
    *count = 0;
    if (!spec || !spec[0]) return 0;
    if (sscanf(spec, "%d,%d%c", &start, &stop, &extra) == 2) {
        if (start < 0 || stop > 360 || start > stop) return -1;
        for (int value = start; value <= stop; value++) {
            if (n >= cap) return -1;
            labels[n++] = value;
        }
    } else if (sscanf(spec, "%d%c", &start, &extra) == 1) {
        if (start < 0 || start > 360) return -1;
        labels[n++] = start;
    } else {
        return -1;
    }
    *count = n;
    return 0;
}

static int write_stream(const char *dir, const char *stem, const char *svg) {
    char folder[SOUL_PATH_MAX], path[SOUL_PATH_MAX];
    const char *d = strstr(svg, "d=\"");
    FILE *out;
    int seen[SOUL_SRC * SOUL_SRC];
    int step = 0;
    if (!d) return -1;
    if (join2(folder, sizeof folder, dir, "cornerstones\\streamcount") != 0) return -1;
    if (mkdir_parents(folder) != 0) return -1;
    if (join2(path, sizeof path, folder, stem) != 0) return -1;
    {
        size_t n = strlen(path);
        if (n + 8 >= sizeof path) return -1;
        memcpy(path + n, ".stream", 8);
    }
    out = fopen(path, "wb");
    if (!out) return -1;
    memset(seen, 0, sizeof seen);
    d += 3;
    while (*d && *d != '"') {
        int x, y, w, h, back;
        if (*d == 'M' && sscanf(d, "M%d %dh%dv%dh%dz", &x, &y, &w, &h, &back) == 5) {
            if (w < 0) { x += w; w = -w; }
            if (h < 0) { y += h; h = -h; }
            for (int yy = y; yy < y + h; yy++) {
                for (int xx = x; xx < x + w; xx++) {
                    int at;
                    if (xx < 0 || yy < 0 || xx >= SOUL_SRC || yy >= SOUL_SRC) continue;
                    at = yy * SOUL_SRC + xx;
                    seen[at] += 1;
                    fprintf(out, "%d %d\n", step, seen[at]);
                    step++;
                }
            }
        }
        d++;
    }
    fclose(out);
    return 0;
}

int soul_write(const char *svg_dir, const char *dir, const char *degrees_spec) {
    char header[4096], path[SOUL_PATH_MAX], svg_path[SOUL_PATH_MAX];
    unsigned char mip[SOUL_MIP * SOUL_MIP * 4];
    FILE *fh;
    FILE *cab;
    char defaults[SOUL_PATH_MAX], backup[SOUL_PATH_MAX], linear[SOUL_PATH_MAX], linear_backup[SOUL_PATH_MAX];
    int labels[361];
    int nlabels = 0;
    soul_header(header, sizeof header);
    if (parse_degree_spec(degrees_spec, labels, 361, &nlabels) != 0) return -1;
    if (join2(defaults, sizeof defaults, dir, "cornerstones\\default") != 0) return -1;
    if (join2(backup, sizeof backup, dir, "cornerstones\\.backup\\default") != 0) return -1;
    if (join2(linear, sizeof linear, dir, "cornerstones\\generated\\linear") != 0) return -1;
    if (join2(linear_backup, sizeof linear_backup, dir, "cornerstones\\.backup\\generated\\linear") != 0) return -1;
    if (mkdir_parents(defaults) != 0 || mkdir_parents(backup) != 0) return -1;
    if (mkdir_parents(linear) != 0 || mkdir_parents(linear_backup) != 0) return -1;
    if (join2(path, sizeof path, dir, "soul.cah") != 0) return -1;
    fh = fopen(path, "wb");
    if (!fh) {
        fprintf(stderr, "soul_gen: cannot write %s\n", path);
        return -1;
    }
    fwrite(header, 1, strlen(header), fh);
    fclose(fh);
    if (join2(path, sizeof path, dir, "soul.cab") != 0) return -1;
    cab = fopen(path, "wb");
    if (!cab) {
        fprintf(stderr, "soul_gen: cannot write %s\n", path);
        return -1;
    }
    for (int i = 1; i <= 26; i++) {
        char *svg;
        char leaf[32];
        long n;
        snprintf(leaf, sizeof leaf, "CS%02d.svg", i);
        if (join2(svg_path, sizeof svg_path, svg_dir, leaf) != 0) return -1;
        fh = fopen(svg_path, "rb");
        if (!fh) {
            fprintf(stderr, "soul_gen: cannot read %s\n", svg_path);
            fclose(cab);
            return -1;
        }
        fseek(fh, 0, SEEK_END);
        n = ftell(fh);
        fseek(fh, 0, SEEK_SET);
        svg = (char *)malloc((size_t)n + 1);
        if (!svg || fread(svg, 1, (size_t)n, fh) != (size_t)n) {
            free(svg);
            fclose(fh);
            fprintf(stderr, "soul_gen: cannot read %s\n", svg_path);
            fclose(cab);
            return -1;
        }
        svg[n] = 0;
        fclose(fh);
        {
            const char *draw = strstr(svg, "d=\"");
            const char *draw_end;
            char copy_path[SOUL_PATH_MAX];
            if (!draw) {
                fprintf(stderr, "soul_gen: %s has no draw specification\n", svg_path);
                free(svg);
                fclose(cab);
                return -1;
            }
            unsigned char mask[SOUL_SRC * SOUL_SRC];
            draw += 3;
            draw_end = strchr(draw, '"');
            if (!draw_end || svg_mask(svg, mask) != 0) {
                fprintf(stderr, "soul_gen: cannot rasterize %s\n", svg_path);
                free(svg);
                fclose(cab);
                return -1;
            }
            {
                char stem[16];
                snprintf(stem, sizeof stem, "CS%02d", i);
                if (write_stream(dir, stem, svg) != 0) {
                    fprintf(stderr, "soul_gen: cannot write streamcount for %s\n", stem);
                    free(svg);
                    fclose(cab);
                    return -1;
                }
            }
            snprintf(leaf, sizeof leaf, "CS%02d.svg", i);
            if (join2(copy_path, sizeof copy_path, defaults, leaf) != 0) {
                free(svg);
                fclose(cab);
                return -1;
            }
            fh = fopen(copy_path, "wb");
            if (!fh) {
                fprintf(stderr, "soul_gen: cannot write %s\n", copy_path);
                free(svg);
                fclose(cab);
                return -1;
            }
            fwrite(svg, 1, (size_t)n, fh);
            fclose(fh);
            paint_plain(mask, mip);
            snprintf(leaf, sizeof leaf, "CS%02d", i);
            if (emit_files(defaults, backup, leaf, mip) != 0) {
                free(svg);
                fclose(cab);
                return -1;
            }
            paint_degrees(mask, 0, mip);
            if (emit_files(linear, linear_backup, leaf, mip) != 0) {
                free(svg);
                fclose(cab);
                return -1;
            }
            fprintf(cab, "&CS%02d::Soul:{\n    &CS%02d.linear::Strategy(bases=\"x,y\", templates=\"linear\"):{\n        $d.x::linear:( %%fn::(x); );\n        $d.y::linear:( %%fn::(y); );\n    };\n    %%fn::(x): x;\n    %%fn::(y): y;\n    $vector:\"", i, i);
            fwrite(draw, 1, (size_t)(draw_end - draw), cab);
            fprintf(cab, "\";\n};\n");
            for (int d = 0; d < nlabels; d++) {
                char turned_dir[SOUL_PATH_MAX], turned_backup[SOUL_PATH_MAX], sub[32];
                snprintf(sub, sizeof sub, "d%d", labels[d]);
                if (join2(turned_dir, sizeof turned_dir, linear, sub) != 0 || join2(turned_backup, sizeof turned_backup, linear_backup, sub) != 0) {
                    free(svg);
                    fclose(cab);
                    return -1;
                }
                if (mkdir_parents(turned_dir) != 0 || mkdir_parents(turned_backup) != 0) {
                    free(svg);
                    fclose(cab);
                    return -1;
                }
                paint_degrees(mask, labels[d] == 360 ? 0 : labels[d], mip);
                if (emit_files(turned_dir, turned_backup, leaf, mip) != 0) {
                    free(svg);
                    fclose(cab);
                    return -1;
                }
                fprintf(cab, "&CS%02d.d%d::Soul:{\n    &CS%02d.linear::Strategy(bases=\"x,y\", templates=\"linear,%d\"):{\n        $d.x::linear:( %%fn::(x); );\n        $d.y::linear:( %%fn::(y); );\n    };\n    %%fn::(x): x;\n    %%fn::(y): y;\n    $vector:\"", i, labels[d], i, labels[d]);
                fwrite(draw, 1, (size_t)(draw_end - draw), cab);
                fprintf(cab, "\";\n};\n");
            }
        }
        free(svg);
    }
    fclose(cab);
    return 0;
}

#ifndef SOUL_GEN_NO_HOST

int main(int argc, char **argv) {
    const char *dir_override = 0;
    const char *name = "soul";
    const char *degrees = 0;
    char home[SOUL_PATH_MAX], svg_dir[SOUL_PATH_MAX], out_dir[SOUL_PATH_MAX];
    int do_test = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--self-test")) do_test = 1;
        else if (!strcmp(argv[i], "--dir") && i + 1 < argc) dir_override = argv[++i];
        else if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "--degrees") && i + 1 < argc) degrees = argv[++i];
        else {
            fprintf(stderr, "usage: soul_gen [--name NAME] [--dir DIR] [--degrees N|START,STOP] [--self-test]\n");
            return 2;
        }
    }
    if (do_test) return self_test() ? 0 : 1;
    if (exe_dir(home, sizeof home) != 0) {
        fprintf(stderr, "soul_gen: cannot locate the executable\n");
        return 1;
    }
    if (join2(svg_dir, sizeof svg_dir, home, "default") != 0) return 1;
    if (place_tool(out_dir, sizeof out_dir, name, dir_override) != 0) {
        fprintf(stderr, "soul_gen: cannot choose an output directory\n");
        return 1;
    }
    return soul_write(svg_dir, out_dir, degrees) == 0 ? 0 : 1;
}

#endif
