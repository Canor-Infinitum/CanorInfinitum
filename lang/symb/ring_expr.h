#ifndef RING_EXPR_H
#define RING_EXPR_H

/* Eight-channel ring expressions. No malloc.
   LTR: prime, dot, caret, underscore. RTL: underscore, caret, dot, prime.
   Previous LTR: P D C U. Previous RTL: RP RD RC RU.
   k and c are legal only as k % <positive literal> and c % <positive literal>. */

#include <stddef.h>
#include <stdint.h>

#define RING_MAX_NODES 256

typedef struct {
    uint8_t op;
    int64_t imm;
    int16_t a, b, c;
} RingNode;

typedef struct {
    RingNode nodes[RING_MAX_NODES];
    int n;
    int root[8];
} RingProg;

typedef struct {
    int p, a, n;
    int prev[8];
    int (*mod_k)(void *ctx, uint32_t m, uint32_t *out);
    int (*mod_c)(void *ctx, uint32_t m, uint32_t *out);
    void *ctx;
} RingEnv;

int ring_compile(const char *source, RingProg *prog, char *err, size_t err_cap);

/* Writes eight clamped (>= 0) channel values. 0 on success, -1 on a runtime error. */
int ring_eval8(const RingProg *prog, const RingEnv *env, int64_t out[8], char *err, size_t err_cap);

/* LTR scripts, the symbol, then RTL scripts. 0 on success, -1 if the buffer is too small. */
int ring_decorate(char symbol, const int64_t vals[8], char *buf, size_t cap);

#endif
