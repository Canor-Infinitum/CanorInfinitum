#ifndef CANOR_BIG_H
#define CANOR_BIG_H

/* Bounded big integers and rationals.
 *
 * WHY THIS EXISTS. The fixed-width core is correct and bounded, and for the
 * type tower that is the right trade. It is the wrong trade for exact
 * arithmetic through an iterated map: squaring doubles the denominator every
 * step, so int64 rationals survive exactly three Mandelbrot iterations
 * (6 -> 12 -> 24 -> 48 -> dead). Watching an exact orbit, or summing an exact
 * series, needs wider integers.
 *
 * FIXED CAPACITY (AGENTS.md invariant 2). A value is a caller-placed struct of
 * CANOR_BIG_LIMBS_MAX 32-bit limbs; nothing is allocated. CANOR_BIG_LIMBS_MAX
 * defaults to 128 limbs = 4096 bits, the widest canor_swar lane
 * (CANOR_SWAR_WIDE_MAX). An operation whose exact result needs more limbs is
 * REFUSED with CANOR_ERR_FULL and leaves its destination untouched - never
 * truncated, never approximated. That bound plays the role the old global limb
 * budget played: a caller that runs out of room learns that it did, which is
 * the difference between `undetermined` and a wrong answer.
 *
 * INTEGER ONLY (invariant 1). No floating point anywhere.
 *
 * REPRESENTATION. Sign-magnitude, 32-bit limbs, least significant first, no
 * leading zero limbs, and `sign == 0` iff the value is zero - so a canonical
 * value has exactly one encoding and canor_big_cmp is a total order.
 * Rationals keep den > 0 and gcd(num, den) == 1 at all times; reduction is by
 * binary GCD, which needs only shifts, subtractions and comparisons.
 *
 * ALIASING. Every binary operation accumulates into a local and then assigns,
 * so a destination may alias either operand.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANOR_BIG_LIMBS_MAX
#define CANOR_BIG_LIMBS_MAX 128          /* 32-bit limbs: 4096 bits */
#endif
#define CANOR_BIG_LIMB_BITS 32
#define CANOR_BIG_BITS_MAX (CANOR_BIG_LIMBS_MAX * CANOR_BIG_LIMB_BITS)
/* Decimal rendering works in chunks of 10^9 (the largest power of ten below
 * 2^32); a 4096-bit value has at most 1234 digits = 138 chunks. */
#define CANOR_BIG_DEC_CHUNK 1000000000u
#define CANOR_BIG_DEC_CHUNK_DIGITS 9
#define CANOR_BIG_DEC_CHUNKS_MAX ((CANOR_BIG_BITS_MAX * 31) / (CANOR_BIG_DEC_CHUNK_DIGITS * 100) + 2)
/* Longest text canor_big_rat_parse accepts. */
#ifndef CANOR_BIG_TEXT_MAX
#define CANOR_BIG_TEXT_MAX 512
#endif

typedef struct {
  uint32_t d[CANOR_BIG_LIMBS_MAX];
  int n;        /* limbs in use */
  int sign;     /* -1, 0, +1; 0 iff n == 0 */
} canor_big_t;

/* Zero. Cannot fail. */
void canor_big_init(canor_big_t *a);
void canor_big_set_i64(canor_big_t *a, int64_t v);      /* always fits */
bool canor_big_is_zero(const canor_big_t *a);
bool canor_big_is_one(const canor_big_t *a);
int  canor_big_bits(const canor_big_t *a);   /* highest set bit position + 1, 0 for zero */
int  canor_big_ctz(const canor_big_t *a);    /* trailing zero bits, 0 for zero */
int  canor_big_cmp_abs(const canor_big_t *a, const canor_big_t *b);
int  canor_big_cmp(const canor_big_t *a, const canor_big_t *b);
void canor_big_neg(canor_big_t *a);
void canor_big_abs(canor_big_t *a);

/* Each returns CANOR_OK, or CANOR_ERR_FULL when the exact result needs more
 * than CANOR_BIG_LIMBS_MAX limbs (r is then unchanged). r may alias a or b. */
canor_status canor_big_add(canor_big_t *r, const canor_big_t *a, const canor_big_t *b);
canor_status canor_big_sub(canor_big_t *r, const canor_big_t *a, const canor_big_t *b);
canor_status canor_big_mul(canor_big_t *r, const canor_big_t *a, const canor_big_t *b);
canor_status canor_big_mul_i64(canor_big_t *r, const canor_big_t *a, int64_t m);
canor_status canor_big_shl(canor_big_t *r, const canor_big_t *a, unsigned k);
void canor_big_shr(canor_big_t *r, const canor_big_t *a, unsigned k);   /* toward zero */
void canor_big_gcd(canor_big_t *r, const canor_big_t *a, const canor_big_t *b);  /* of |a|, |b| */
canor_status canor_big_pow_u32(canor_big_t *r, const canor_big_t *a, uint32_t e);

/* Truncating division: q = a / b toward zero, rem carries the sign of a, and
 * a == q*b + rem exactly. Either output may be NULL. CANOR_ERR_UNDEFINED when
 * b is zero. */
canor_status canor_big_divmod(canor_big_t *q, canor_big_t *rem,
                              const canor_big_t *a, const canor_big_t *b);
/* floor(sqrt(a)) and floor(a^(1/k)) for a >= 0, k >= 1. CANOR_ERR_ARGS for a
 * negative a or k == 0. Bit-by-bit, so exact for every input. */
canor_status canor_big_isqrt(canor_big_t *r, const canor_big_t *a);
canor_status canor_big_iroot(canor_big_t *r, const canor_big_t *a, uint32_t k);

/* a as an int64. CANOR_ERR_RANGE when it does not fit. */
canor_status canor_big_to_i64(const canor_big_t *a, int64_t *out);
/* num/den scaled by 2^shift, truncated toward zero, as an int64. CANOR_ERR_RANGE
 * when the result does not fit - a caller comparing against a fixed-point path
 * needs to know rather than receive a saturated value. */
canor_status canor_big_rat_to_fixed(const canor_big_t *num, const canor_big_t *den,
                                    unsigned shift, int64_t *out);

/* Decimal rendering. Returns the number of characters that would be written,
 * so a caller can size a buffer; writes nothing beyond cap. */
int canor_big_format(const canor_big_t *a, char *out, int cap);
/* Optionally signed decimal integer. Trailing junk is CANOR_ERR_ARGS. */
canor_status canor_big_parse(canor_big_t *a, const char *s);

/* --- rationals -------------------------------------------------------------- */

typedef struct { canor_big_t num, den; } canor_big_rat_t;

void canor_big_rat_init(canor_big_rat_t *q);                   /* 0/1 */
canor_status canor_big_rat_set_i64(canor_big_rat_t *q, int64_t n, int64_t d);  /* d != 0 */
/* Reduces to lowest terms with den > 0. Called by every operation below, so a
 * caller never sees an unreduced value - an unreduced denominator grows
 * without bound and would hit the capacity on bookkeeping rather than on real
 * magnitude. CANOR_ERR_UNDEFINED for a zero denominator. */
canor_status canor_big_rat_normalize(canor_big_rat_t *q);
canor_status canor_big_rat_add(canor_big_rat_t *r, const canor_big_rat_t *a, const canor_big_rat_t *b);
canor_status canor_big_rat_sub(canor_big_rat_t *r, const canor_big_rat_t *a, const canor_big_rat_t *b);
canor_status canor_big_rat_mul(canor_big_rat_t *r, const canor_big_rat_t *a, const canor_big_rat_t *b);
/* 1/a. CANOR_ERR_UNDEFINED for a == 0. */
canor_status canor_big_rat_inv(canor_big_rat_t *r, const canor_big_rat_t *a);
/* Comparisons by cross-multiplication - no division, no rounding. -1, 0, +1.
 * A product that would not fit compares as 0 and sets *st (when non-NULL). */
int  canor_big_rat_cmp_i64(const canor_big_rat_t *a, int64_t v, canor_status *st);
int  canor_big_rat_cmp(const canor_big_rat_t *a, const canor_big_rat_t *b, canor_status *st);
/* Widest of the two bit counts: how much room a value occupies. */
int  canor_big_rat_bits(const canor_big_rat_t *a);
/* "n/d", or "n" when d is 1. */
int  canor_big_rat_format(const canor_big_rat_t *a, char *out, int cap);
/* "n", "n/d", or a terminating decimal such as "-0.3125" (exact: n/10^k). */
canor_status canor_big_rat_parse(canor_big_rat_t *q, const char *s);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_BIG_H */
