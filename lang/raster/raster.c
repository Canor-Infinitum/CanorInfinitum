#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <string.h>

/* Inclusive fold. has_value 0 selects nominal. *quot is spans removed. Returns low-1 on error. */
int raster_fold(int has_value, int value, int low, int nominal, int high, int *quot) {
    int span, rem, v;
    if (high < low || nominal < low || nominal > high) return low - 1;
    if (!has_value) {
        if (quot) *quot = 0;
        return nominal;
    }
    span = high - low + 1;
    v = value - low;
    rem = v % span;
    if (rem < 0) rem += span;
    if (quot) *quot = (v - rem) / span;
    return low + rem;
}

static unsigned long crc_table[256];
static int crc_ready = 0;

static void crc_init(void) {
    if (crc_ready) return;
    for (unsigned long n = 0; n < 256; n++) {
        unsigned long c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
        crc_table[n] = c;
    }
    crc_ready = 1;
}

static unsigned long crc32(const unsigned char *data, size_t n) {
    unsigned long c = 0xFFFFFFFFUL;
    crc_init();
    for (size_t i = 0; i < n; i++) c = crc_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFUL;
}

static unsigned long adler32(const unsigned char *data, size_t n) {
    unsigned long a = 1, b = 0;
    for (size_t i = 0; i < n; i++) {
        a = (a + data[i]) % 65521UL;
        b = (b + a) % 65521UL;
    }
    return (b << 16) | a;
}

static void put_le32(unsigned char *p, unsigned long v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void put_be32(unsigned char *p, unsigned long v) {
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

/* 32-bit bottom-up BGRA bitmap. Returns malloc'd bytes; *out_n is the length. */
unsigned char *raster_bitmap(int width, int height, const unsigned char *rgba, size_t *out_n) {
    size_t row = (size_t)width * 4;
    size_t pixels = row * (size_t)height;
    size_t total = 14 + 40 + pixels;
    unsigned char *out;
    if (width < 1 || height < 1 || !rgba || !out_n) return 0;
    out = (unsigned char *)malloc(total);
    if (!out) return 0;
    out[0] = 'B';
    out[1] = 'M';
    put_le32(out + 2, (unsigned long)total);
    put_le32(out + 6, 0);
    put_le32(out + 10, 54);
    put_le32(out + 14, 40);
    put_le32(out + 18, (unsigned long)width);
    put_le32(out + 22, (unsigned long)height);
    out[26] = 1;
    out[27] = 0;
    out[28] = 32;
    out[29] = 0;
    put_le32(out + 30, 0);
    put_le32(out + 34, (unsigned long)pixels);
    put_le32(out + 38, 2835);
    put_le32(out + 42, 2835);
    put_le32(out + 46, 0);
    put_le32(out + 50, 0);
    for (int y = 0; y < height; y++) {
        const unsigned char *src = rgba + (size_t)(height - 1 - y) * row;
        unsigned char *dst = out + 54 + (size_t)y * row;
        for (int x = 0; x < width; x++) {
            dst[x * 4 + 0] = src[x * 4 + 2];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 2] = src[x * 4 + 0];
            dst[x * 4 + 3] = src[x * 4 + 3];
        }
    }
    *out_n = total;
    return out;
}

static unsigned char *png_chunk(const char *tag, const unsigned char *data, size_t n, size_t *out_n) {
    unsigned char *out = (unsigned char *)malloc(12 + n);
    unsigned char head[4];
    unsigned long c;
    unsigned char *crc_src;
    if (!out) return 0;
    put_be32(out, (unsigned long)n);
    memcpy(out + 4, tag, 4);
    if (n) memcpy(out + 8, data, n);
    crc_src = out + 4;
    c = crc32(crc_src, 4 + n);
    put_be32(out + 8 + n, c);
    *out_n = 12 + n;
    (void)head;
    return out;
}

/* RGBA8 PNG, color type 6, zlib-compressed scanlines with filter 0. */
unsigned char *raster_png(int width, int height, const unsigned char *rgba, size_t *out_n) {
    size_t raw_n, z_n, ihdr_n, idat_n, iend_n, total;
    unsigned char *raw, *z, *ihdr, *idat, *iend, *out;
    unsigned char ihdr_data[13];
    if (width < 1 || height < 1 || !rgba || !out_n) return 0;
    raw_n = (size_t)height * ((size_t)width * 4 + 1);
    raw = (unsigned char *)malloc(raw_n);
    if (!raw) return 0;
    for (int y = 0; y < height; y++) {
        raw[(size_t)y * ((size_t)width * 4 + 1)] = 0;
        memcpy(raw + (size_t)y * ((size_t)width * 4 + 1) + 1, rgba + (size_t)y * (size_t)width * 4, (size_t)width * 4);
    }
    /* Uncompressed zlib: header, one or more stored blocks, Adler32. */
    z_n = 2 + ((raw_n / 65535) + 1) * 5 + raw_n + 4;
    z = (unsigned char *)malloc(z_n);
    if (!z) {
        free(raw);
        return 0;
    }
    z[0] = 0x78;
    z[1] = 0x01;
    {
        size_t at = 2, src = 0;
        unsigned long sum;
        while (src < raw_n || src == 0) {
            size_t take = raw_n - src;
            int final;
            if (take > 65535) take = 65535;
            final = src + take >= raw_n;
            z[at++] = (unsigned char)final;
            z[at++] = (unsigned char)(take & 0xFF);
            z[at++] = (unsigned char)((take >> 8) & 0xFF);
            z[at++] = (unsigned char)((take ^ 0xFFFF) & 0xFF);
            z[at++] = (unsigned char)(((take ^ 0xFFFF) >> 8) & 0xFF);
            if (take) memcpy(z + at, raw + src, take);
            at += take;
            src += take;
            if (final) {
                sum = adler32(raw, raw_n);
                put_be32(z + at, sum);
                at += 4;
                z_n = at;
                break;
            }
            if (src == 0) break;
        }
    }
    free(raw);
    put_be32(ihdr_data, (unsigned long)width);
    put_be32(ihdr_data + 4, (unsigned long)height);
    ihdr_data[8] = 8;
    ihdr_data[9] = 6;
    ihdr_data[10] = 0;
    ihdr_data[11] = 0;
    ihdr_data[12] = 0;
    ihdr = png_chunk("IHDR", ihdr_data, 13, &ihdr_n);
    idat = png_chunk("IDAT", z, z_n, &idat_n);
    iend = png_chunk("IEND", 0, 0, &iend_n);
    free(z);
    if (!ihdr || !idat || !iend) {
        free(ihdr);
        free(idat);
        free(iend);
        return 0;
    }
    total = 8 + ihdr_n + idat_n + iend_n;
    out = (unsigned char *)malloc(total);
    if (!out) {
        free(ihdr);
        free(idat);
        free(iend);
        return 0;
    }
    memcpy(out, "\x89PNG\r\n\x1a\n", 8);
    memcpy(out + 8, ihdr, ihdr_n);
    memcpy(out + 8 + ihdr_n, idat, idat_n);
    memcpy(out + 8 + ihdr_n + idat_n, iend, iend_n);
    free(ihdr);
    free(idat);
    free(iend);
    *out_n = total;
    return out;
}
