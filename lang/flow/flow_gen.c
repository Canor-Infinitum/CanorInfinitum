#define _CRT_SECURE_NO_WARNINGS
#include "flow_gen.h"

#include <ctype.h>
#include <string.h>

static void big_zero(FlowBig *a) { memset(a, 0, sizeof *a); }

static int big_is_zero(const FlowBig *a) {
    for (int i = 0; i < FLOW_LIMBS; i++)
        if (a->d[i]) return 0;
    return 1;
}

static int big_cmp(const FlowBig *a, const FlowBig *b) {
    for (int i = FLOW_LIMBS - 1; i >= 0; i--) {
        if (a->d[i] < b->d[i]) return -1;
        if (a->d[i] > b->d[i]) return 1;
    }
    return 0;
}

static int big_mul_u32(FlowBig *a, uint32_t m) {
    uint64_t carry = 0;
    for (int i = 0; i < FLOW_LIMBS; i++) {
        carry += (uint64_t)a->d[i] * m;
        a->d[i] = (uint32_t)carry;
        carry >>= 32;
    }
    return carry == 0;
}

static int big_add_u32(FlowBig *a, uint32_t v) {
    uint64_t carry = v;
    for (int i = 0; i < FLOW_LIMBS && carry; i++) {
        carry += a->d[i];
        a->d[i] = (uint32_t)carry;
        carry >>= 32;
    }
    return carry == 0;
}

static int big_add(FlowBig *a, const FlowBig *b) {
    uint64_t carry = 0;
    for (int i = 0; i < FLOW_LIMBS; i++) {
        carry += (uint64_t)a->d[i] + b->d[i];
        a->d[i] = (uint32_t)carry;
        carry >>= 32;
    }
    return carry == 0;
}

static int big_sub(FlowBig *a, const FlowBig *b) {
    uint64_t borrow = 0;
    for (int i = 0; i < FLOW_LIMBS; i++) {
        uint64_t need = (uint64_t)b->d[i] + borrow;
        if (a->d[i] >= need) {
            a->d[i] = (uint32_t)(a->d[i] - need);
            borrow = 0;
        } else {
            a->d[i] = (uint32_t)((1ull << 32) + a->d[i] - need);
            borrow = 1;
        }
    }
    return borrow == 0;
}

static uint32_t big_div_u32(FlowBig *a, uint32_t d) {
    uint64_t rem = 0;
    for (int i = FLOW_LIMBS - 1; i >= 0; i--) {
        rem = (rem << 32) | a->d[i];
        a->d[i] = (uint32_t)(rem / d);
        rem %= d;
    }
    return (uint32_t)rem;
}

static int big_from_dec(FlowBig *a, const char *s) {
    big_zero(a);
    if (!s || !*s) return 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9') return 0;
        if (!big_mul_u32(a, 10) || !big_add_u32(a, (uint32_t)(*s - '0'))) return 0;
    }
    return 1;
}

static int big_to_dec(const FlowBig *a, char *out, size_t cap) {
    FlowBig t = *a;
    char rev[FLOW_DEC_MAX];
    size_t n = 0;
    if (cap < 2) return 0;
    if (big_is_zero(&t)) {
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    while (!big_is_zero(&t)) {
        if (n + 1 >= sizeof rev) return 0;
        rev[n++] = (char)('0' + big_div_u32(&t, 10));
    }
    if (n + 1 > cap) return 0;
    for (size_t i = 0; i < n; i++) out[i] = rev[n - 1 - i];
    out[n] = 0;
    return 1;
}

static int big_pow_u32(FlowBig *out, uint32_t base, int exp) {
    big_zero(out);
    out->d[0] = 1;
    for (int i = 0; i < exp; i++)
        if (!big_mul_u32(out, base)) return 0;
    return 1;
}

static void err_set(char *err, size_t cap, const char *msg) {
    if (!err || cap == 0) return;
    size_t n = 0;
    while (msg[n] && n + 1 < cap) {
        err[n] = msg[n];
        n++;
    }
    err[n] = 0;
}

static int parse_bases(const char *text, char *bases, int *n_out) {
    int n = 0;
    const char *p = text;
    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == '{' || *p == '}')) p++;
        if (!*p) break;
        if (!isalpha((unsigned char)*p) || (p[1] && isalpha((unsigned char)p[1]))) return 0;
        for (int i = 0; i < n; i++)
            if (bases[i] == *p) return 0;
        if (n >= FLOW_MAX_BASES) return 0;
        bases[n++] = *p++;
    }
    if (!n) return 0;
    *n_out = n;
    return 1;
}

static int check_name(const char *name) {
    if (!name || !isalpha((unsigned char)name[0])) return 0;
    for (const char *p = name + 1; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (!isalnum(c) && c != '_' && c != '-') return 0;
        if ((size_t)(p - name) + 1 >= FLOW_NAME_MAX) return 0;
    }
    return 1;
}

static int eq_ci(const char *a, const char *b) {
    for (; *a && *b; a++, b++) {
        unsigned char ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
    }
    return *a == 0 && *b == 0;
}

static void digits_of(const FlowBig *index, int n_bases, int order, int *digits) {
    FlowBig t = *index;
    for (int i = order - 1; i >= 0; i--)
        digits[i] = (int)big_div_u32(&t, (uint32_t)n_bases);
}

static void bump(int *digits, int order, int n_bases) {
    for (int i = order - 1; i >= 0; i--) {
        digits[i]++;
        if (digits[i] < n_bases) return;
        digits[i] = 0;
    }
}

static int append(char *buf, size_t cap, size_t *n, const char *s) {
    while (*s) {
        if (*n + 1 >= cap) return 0;
        buf[(*n)++] = *s++;
    }
    buf[*n] = 0;
    return 1;
}

static int append_ch(char *buf, size_t cap, size_t *n, char c) {
    if (*n + 1 >= cap) return 0;
    buf[(*n)++] = c;
    buf[*n] = 0;
    return 1;
}

int flow_session_begin(FlowSession *s, const char *bases_text, int order,
                       const char *index_dec, const char *count_dec,
                       const char *direction, const char *name,
                       char *err, size_t err_cap) {
    FlowBig index, card, count;
    memset(s, 0, sizeof *s);
    if (order < 1 || order > FLOW_MAX_ORDER) {
        err_set(err, err_cap, "order must be in 1..288");
        return -1;
    }
    if (!parse_bases(bases_text, s->bases, &s->n_bases)) {
        err_set(err, err_cap, "basis alphabet must be unique single letters");
        return -1;
    }
    if (!check_name(name)) {
        err_set(err, err_cap, "name must be a letter followed by letters, digits, '_' or '-'");
        return -1;
    }
    if (!direction || eq_ci(direction, "ltr")) s->rtl = 0;
    else if (eq_ci(direction, "rtl")) s->rtl = 1;
    else {
        err_set(err, err_cap, "direction must be ltr or rtl");
        return -1;
    }
    if (!big_from_dec(&index, index_dec)) {
        err_set(err, err_cap, "index must be a decimal integer");
        return -1;
    }
    if (!big_pow_u32(&card, (uint32_t)s->n_bases, order)) {
        err_set(err, err_cap, "cardinality overflow");
        return -1;
    }
    if (count_dec && eq_ci(count_dec, "all")) {
        if (big_cmp(&index, &card) > 0) {
            err_set(err, err_cap, "index is past the end of the space");
            return -1;
        }
        count = card;
        if (!big_sub(&count, &index)) {
            err_set(err, err_cap, "count underflow");
            return -1;
        }
    } else if (!big_from_dec(&count, count_dec)) {
        err_set(err, err_cap, "count must be a decimal integer or 'all'");
        return -1;
    }
    {
        FlowBig end = index;
        if (big_cmp(&index, &card) > 0 || !big_add(&end, &count) || big_cmp(&end, &card) > 0) {
            err_set(err, err_cap, "slice exceeds cardinality");
            return -1;
        }
        s->end = end;
    }
    s->order = order;
    s->cursor = index;
    s->count = count;
    {
        size_t i = 0;
        while (name[i] && i + 1 < FLOW_NAME_MAX) {
            s->name[i] = name[i];
            i++;
        }
        s->name[i] = 0;
    }
    if (!big_is_zero(&count)) {
        digits_of(&index, s->n_bases, order, s->digits);
        s->active = 1;
    }
    return 0;
}

int flow_session_header(const FlowSession *s, const char *index_dec, char *buf, size_t cap) {
    char count_s[FLOW_DEC_MAX];
    char card_s[FLOW_DEC_MAX];
    FlowBig card, count_copy;
    size_t n = 0;
    if (!big_pow_u32(&card, (uint32_t)s->n_bases, s->order)) return -1;
    count_copy = s->count;
    if (!big_to_dec(&count_copy, count_s, sizeof count_s)) return -1;
    if (!big_to_dec(&card, card_s, sizeof card_s)) return -1;
    buf[0] = 0;
    if (!append(buf, cap, &n, "bases = { ")) return -1;
    for (int i = 0; i < s->n_bases; i++) {
        if (i && !append(buf, cap, &n, ", ")) return -1;
        if (!append_ch(buf, cap, &n, s->bases[i])) return -1;
    }
    if (!append(buf, cap, &n, " }\norder = ")) return -1;
    {
        char tmp[16];
        int v = s->order;
        int k = 0;
        char rev[16];
        if (v == 0) rev[k++] = '0';
        while (v) {
            rev[k++] = (char)('0' + v % 10);
            v /= 10;
        }
        for (int i = 0; i < k; i++) tmp[i] = rev[k - 1 - i];
        tmp[k] = 0;
        if (!append(buf, cap, &n, tmp)) return -1;
    }
    if (!append(buf, cap, &n, "\ndirection = ")) return -1;
    if (!append(buf, cap, &n, s->rtl ? "RTL" : "LTR")) return -1;
    if (!append(buf, cap, &n, "\nindex = ")) return -1;
    if (!append(buf, cap, &n, index_dec)) return -1;
    if (!append(buf, cap, &n, "\ncount = ")) return -1;
    if (!append(buf, cap, &n, count_s)) return -1;
    if (!append(buf, cap, &n, "\ncardinality = ")) return -1;
    if (!append(buf, cap, &n, card_s)) return -1;
    if (!append_ch(buf, cap, &n, '\n')) return -1;
    return 0;
}

int flow_session_next(FlowSession *s, char *buf, size_t cap) {
    char index_s[FLOW_DEC_MAX];
    char count_s[FLOW_DEC_MAX];
    size_t n = 0;
    if (!s->active || big_cmp(&s->cursor, &s->end) >= 0) return 0;
    if (!big_to_dec(&s->cursor, index_s, sizeof index_s)) return -1;
    if (!big_to_dec(&s->count, count_s, sizeof count_s)) return -1;
    buf[0] = 0;
    if (!append_ch(buf, cap, &n, '@')) return -1;
    if (!append(buf, cap, &n, s->name)) return -1;
    if (!append(buf, cap, &n, "::Flow.")) return -1;
    {
        char tmp[16];
        int v = s->order, k = 0;
        char rev[16];
        if (v == 0) rev[k++] = '0';
        while (v) {
            rev[k++] = (char)('0' + v % 10);
            v /= 10;
        }
        for (int i = 0; i < k; i++) tmp[i] = rev[k - 1 - i];
        tmp[k] = 0;
        if (!append(buf, cap, &n, tmp)) return -1;
    }
    if (!append_ch(buf, cap, &n, '.')) return -1;
    if (!append(buf, cap, &n, index_s)) return -1;
    if (!append_ch(buf, cap, &n, '.')) return -1;
    if (!append(buf, cap, &n, count_s)) return -1;
    if (!append(buf, cap, &n, ":{ ")) return -1;
    for (int i = 0; i < s->order; i++) {
        int src = s->rtl ? s->order - 1 - i : i;
        if (i && !append(buf, cap, &n, ", ")) return -1;
        if (!append_ch(buf, cap, &n, s->bases[s->digits[src]])) return -1;
    }
    if (!append(buf, cap, &n, " };")) return -1;
    bump(s->digits, s->order, s->n_bases);
    if (!big_add_u32(&s->cursor, 1)) return -1;
    if (big_cmp(&s->cursor, &s->end) >= 0) s->active = 0;
    return 1;
}

int flow_big_mod_u32(const FlowBig *a, uint32_t m, uint32_t *out) {
    FlowBig t;
    if (!m || !out) return -1;
    t = *a;
    *out = big_div_u32(&t, m);
    return 0;
}

int flow_big_to_dec(const FlowBig *a, char *buf, size_t cap) {
    return big_to_dec(a, buf, cap) ? 0 : -1;
}

int flow_session_cardinality(const FlowSession *s, char *buf, size_t cap) {
    FlowBig card;
    if (!big_pow_u32(&card, (uint32_t)s->n_bases, s->order)) return -1;
    return flow_big_to_dec(&card, buf, cap);
}

int flow_session_advance(FlowSession *s) {
    if (!s->active || big_cmp(&s->cursor, &s->end) >= 0) return 0;
    bump(s->digits, s->order, s->n_bases);
    if (!big_add_u32(&s->cursor, 1)) return -1;
    if (big_cmp(&s->cursor, &s->end) >= 0) s->active = 0;
    return 1;
}

