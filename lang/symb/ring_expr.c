#include "ring_expr.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

enum {
    ND_IMM = 1,
    ND_P,
    ND_A,
    ND_N,
    ND_PREV,
    ND_KMOD,
    ND_CMOD,
    ND_NEG,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_MOD,
    ND_TERN
};

typedef struct {
    const char *cur;
    RingProg *prog;
    char *err;
    size_t err_cap;
} Ps;

static void err_set(char *err, size_t cap, const char *msg) {
    size_t n = 0;
    if (!err || !cap) return;
    while (msg[n] && n + 1 < cap) {
        err[n] = msg[n];
        n++;
    }
    err[n] = 0;
}

static void skip(Ps *ps) {
    while (*ps->cur && isspace((unsigned char)*ps->cur)) ps->cur++;
}

static int node(Ps *ps, uint8_t op, int64_t imm, int a, int b, int c) {
    RingNode nd;
    if (ps->prog->n >= RING_MAX_NODES) {
        err_set(ps->err, ps->err_cap, "expression is too large");
        return -1;
    }
    nd.op = op;
    nd.imm = imm;
    nd.a = (int16_t)a;
    nd.b = (int16_t)b;
    nd.c = (int16_t)c;
    ps->prog->nodes[ps->prog->n] = nd;
    return ps->prog->n++;
}

static int parse_expr(Ps *ps);

static int parse_factor(Ps *ps) {
    skip(ps);
    if (!*ps->cur) {
        err_set(ps->err, ps->err_cap, "expression ended early");
        return -1;
    }
    if (*ps->cur == '-') {
        int inner;
        ps->cur++;
        inner = parse_factor(ps);
        if (inner < 0) return -1;
        return node(ps, ND_NEG, 0, inner, -1, -1);
    }
    if (*ps->cur == '(') {
        int inner;
        ps->cur++;
        inner = parse_expr(ps);
        if (inner < 0) return -1;
        skip(ps);
        if (*ps->cur != ')') {
            err_set(ps->err, ps->err_cap, "unbalanced parenthesis");
            return -1;
        }
        ps->cur++;
        return inner;
    }
    if (isdigit((unsigned char)*ps->cur)) {
        int64_t v = 0;
        while (isdigit((unsigned char)*ps->cur)) {
            int d = *ps->cur - '0';
            if (v > (INT64_MAX - d) / 10) {
                err_set(ps->err, ps->err_cap, "integer literal overflow");
                return -1;
            }
            v = v * 10 + d;
            ps->cur++;
        }
        return node(ps, ND_IMM, v, -1, -1, -1);
    }
    if (isalpha((unsigned char)*ps->cur)) {
        char name = *ps->cur++;
        if (name == 'k' || name == 'c') {
            int64_t mod = 0;
            skip(ps);
            if (*ps->cur != '%') {
                err_set(ps->err, ps->err_cap, "k and c are legal only as k % <positive integer> or c % <positive integer>");
                return -1;
            }
            ps->cur++;
            skip(ps);
            if (!isdigit((unsigned char)*ps->cur)) {
                err_set(ps->err, ps->err_cap, "modulus must be a positive integer literal");
                return -1;
            }
            while (isdigit((unsigned char)*ps->cur)) {
                int d = *ps->cur - '0';
                if (mod > (INT64_MAX - d) / 10) {
                    err_set(ps->err, ps->err_cap, "integer literal overflow");
                    return -1;
                }
                mod = mod * 10 + d;
                ps->cur++;
            }
            if (mod <= 0 || mod > 4294967295ll) {
                err_set(ps->err, ps->err_cap, "modulus must be a positive integer literal");
                return -1;
            }
            return node(ps, name == 'k' ? ND_KMOD : ND_CMOD, mod, -1, -1, -1);
        }
        if (name == 'R') {
            char n2 = *ps->cur;
            int idx = -1;
            if (n2 == 'P') idx = 7;
            else if (n2 == 'D') idx = 6;
            else if (n2 == 'C') idx = 5;
            else if (n2 == 'U') idx = 4;
            if (idx >= 0) {
                ps->cur++;
                return node(ps, ND_PREV, idx, -1, -1, -1);
            }
        }
        if (name == 'p') return node(ps, ND_P, 0, -1, -1, -1);
        if (name == 'a') return node(ps, ND_A, 0, -1, -1, -1);
        if (name == 'n') return node(ps, ND_N, 0, -1, -1, -1);
        if (name == 'P') return node(ps, ND_PREV, 0, -1, -1, -1);
        if (name == 'D') return node(ps, ND_PREV, 1, -1, -1, -1);
        if (name == 'C') return node(ps, ND_PREV, 2, -1, -1, -1);
        if (name == 'U') return node(ps, ND_PREV, 3, -1, -1, -1);
        err_set(ps->err, ps->err_cap, "unknown name");
        return -1;
    }
    err_set(ps->err, ps->err_cap, "unexpected character");
    return -1;
}

static int parse_term(Ps *ps) {
    int left = parse_factor(ps);
    if (left < 0) return -1;
    for (;;) {
        int op, right;
        skip(ps);
        if (*ps->cur != '*' && *ps->cur != '%') return left;
        op = *ps->cur == '*' ? ND_MUL : ND_MOD;
        ps->cur++;
        right = parse_factor(ps);
        if (right < 0) return -1;
        left = node(ps, (uint8_t)op, 0, left, right, -1);
        if (left < 0) return -1;
    }
}

static int parse_sum(Ps *ps) {
    int left = parse_term(ps);
    if (left < 0) return -1;
    for (;;) {
        int op, right;
        skip(ps);
        if (*ps->cur != '+' && *ps->cur != '-') return left;
        op = *ps->cur == '+' ? ND_ADD : ND_SUB;
        ps->cur++;
        right = parse_term(ps);
        if (right < 0) return -1;
        left = node(ps, (uint8_t)op, 0, left, right, -1);
        if (left < 0) return -1;
    }
}

static int parse_expr(Ps *ps) {
    int cond = parse_sum(ps);
    if (cond < 0) return -1;
    skip(ps);
    if (*ps->cur == '?') {
        int then_n, else_n;
        ps->cur++;
        then_n = parse_expr(ps);
        if (then_n < 0) return -1;
        skip(ps);
        if (*ps->cur != ':') {
            err_set(ps->err, ps->err_cap, "ternary is missing ':'");
            return -1;
        }
        ps->cur++;
        else_n = parse_expr(ps);
        if (else_n < 0) return -1;
        return node(ps, ND_TERN, 0, cond, then_n, else_n);
    }
    return cond;
}

int ring_compile(const char *source, RingProg *prog, char *err, size_t err_cap) {
    Ps ps;
    int ch;
    if (!source || !prog) return -1;
    memset(prog, 0, sizeof *prog);
    ps.cur = source;
    ps.prog = prog;
    ps.err = err;
    ps.err_cap = err_cap;
    skip(&ps);
    if (!*ps.cur) {
        err_set(err, err_cap, "symbolic expression is empty");
        return -1;
    }
    for (ch = 0; ch < 8; ch++) {
        int root = parse_expr(&ps);
        if (root < 0) return -1;
        prog->root[ch] = root;
        skip(&ps);
        if (ch < 7) {
            if (*ps.cur != ',') {
                err_set(err, err_cap, "symbolic expression needs eight channels");
                return -1;
            }
            ps.cur++;
        }
    }
    skip(&ps);
    if (*ps.cur) {
        err_set(err, err_cap, "symbolic expression needs eight channels");
        return -1;
    }
    return 0;
}

static int mul_ok(int64_t a, int64_t b, int64_t *out) {
    if (a == 0 || b == 0) {
        *out = 0;
        return 1;
    }
    if (a > 0 && b > 0 && a > INT64_MAX / b) return 0;
    if (a < 0 && b < 0 && a < INT64_MAX / b) return 0;
    if (a > 0 && b < 0 && b < INT64_MIN / a) return 0;
    if (a < 0 && b > 0 && a < INT64_MIN / b) return 0;
    *out = a * b;
    return 1;
}

static int pymod(int64_t a, int64_t b, int64_t *out) {
    int64_t r;
    if (b == 0) return 0;
    r = a % b;
    if ((r < 0 && b > 0) || (r > 0 && b < 0)) r += b;
    *out = r;
    return 1;
}

static int eval_node(const RingProg *prog, int idx, const RingEnv *env, int64_t *out, char *err, size_t err_cap) {
    const RingNode *nd = &prog->nodes[idx];
    int64_t left, right;
    switch (nd->op) {
    case ND_IMM:
        *out = nd->imm;
        return 0;
    case ND_P:
        *out = env->p;
        return 0;
    case ND_A:
        *out = env->a;
        return 0;
    case ND_N:
        *out = env->n;
        return 0;
    case ND_PREV:
        *out = env->prev[nd->imm];
        return 0;
    case ND_KMOD:
    case ND_CMOD: {
        uint32_t rem = 0;
        int (*fn)(void *, uint32_t, uint32_t *) = nd->op == ND_KMOD ? env->mod_k : env->mod_c;
        if (!fn || fn(env->ctx, (uint32_t)nd->imm, &rem) != 0) {
            err_set(err, err_cap, "modulo failed");
            return -1;
        }
        *out = rem;
        return 0;
    }
    case ND_NEG:
        if (eval_node(prog, nd->a, env, &left, err, err_cap) != 0) return -1;
        if (left == INT64_MIN) {
            err_set(err, err_cap, "overflow");
            return -1;
        }
        *out = -left;
        return 0;
    case ND_TERN:
        if (eval_node(prog, nd->a, env, &left, err, err_cap) != 0) return -1;
        return eval_node(prog, left != 0 ? nd->b : nd->c, env, out, err, err_cap);
    default:
        break;
    }
    if (eval_node(prog, nd->a, env, &left, err, err_cap) != 0) return -1;
    if (eval_node(prog, nd->b, env, &right, err, err_cap) != 0) return -1;
    if (nd->op == ND_ADD) {
        if ((right > 0 && left > INT64_MAX - right) || (right < 0 && left < INT64_MIN - right)) {
            err_set(err, err_cap, "overflow");
            return -1;
        }
        *out = left + right;
        return 0;
    }
    if (nd->op == ND_SUB) {
        if ((right < 0 && left > INT64_MAX + right) || (right > 0 && left < INT64_MIN + right)) {
            err_set(err, err_cap, "overflow");
            return -1;
        }
        *out = left - right;
        return 0;
    }
    if (nd->op == ND_MUL) {
        if (!mul_ok(left, right, out)) {
            err_set(err, err_cap, "overflow");
            return -1;
        }
        return 0;
    }
    if (nd->op == ND_MOD) {
        if (!pymod(left, right, out)) {
            err_set(err, err_cap, "modulo by zero");
            return -1;
        }
        return 0;
    }
    err_set(err, err_cap, "bad expression node");
    return -1;
}

int ring_eval8(const RingProg *prog, const RingEnv *env, int64_t out[8], char *err, size_t err_cap) {
    for (int i = 0; i < 8; i++) {
        if (eval_node(prog, prog->root[i], env, &out[i], err, err_cap) != 0) return -1;
        if (out[i] < 0) out[i] = 0;
    }
    return 0;
}

static int append_ch(char *buf, size_t cap, size_t *n, char c) {
    if (*n + 1 >= cap) return 0;
    buf[(*n)++] = c;
    buf[*n] = 0;
    return 1;
}

static int append_i64(char *buf, size_t cap, size_t *n, int64_t v) {
    char rev[24];
    int k = 0;
    uint64_t u;
    if (v < 0) return 0;
    u = (uint64_t)v;
    if (u == 0) return append_ch(buf, cap, n, '0');
    while (u) {
        rev[k++] = (char)('0' + (u % 10));
        u /= 10;
    }
    while (k--)
        if (!append_ch(buf, cap, n, rev[k])) return 0;
    return 1;
}

int ring_decorate(char symbol, const int64_t vals[8], char *buf, size_t cap) {
    static const char left_mark[4] = {'\'', '.', '^', '_'};
    static const char right_mark[4] = {'_', '^', '.', '\''};
    size_t n = 0;
    if (!buf || cap == 0) return -1;
    buf[0] = 0;
    for (int i = 0; i < 4; i++) {
        if (vals[i] <= 0) continue;
        if (!append_i64(buf, cap, &n, vals[i]) || !append_ch(buf, cap, &n, left_mark[i])) return -1;
    }
    if (!append_ch(buf, cap, &n, symbol)) return -1;
    for (int i = 0; i < 4; i++) {
        if (vals[4 + i] <= 0) continue;
        if (!append_ch(buf, cap, &n, right_mark[i]) || !append_i64(buf, cap, &n, vals[4 + i])) return -1;
    }
    return 0;
}
