#ifndef CANOR_BIG_H
#define CANOR_BIG_H

/* Arbitrary-precision integers and rationals.
 *
 * WHY THIS EXISTS. The fixed-width core is correct and bounded, and for the
 * type tower that is the right trade. It is the wrong trade for one thing: the
 * exact-arithmetic path through an iterated map. Squaring doubles the
 * denominator every step, so int64 rationals survive exactly three Mandelbrot
 * iterations (6 -> 12 -> 24 -> 48 -> dead) regardless of how the grid is
 * chosen. That is not a precision shortfall to be tuned away; it is the
 * representation reaching its end. Anything that wants to watch an exact orbit
 * for longer than three steps needs unbounded integers.
 *
 * THE BUDGET IS THE POINT. Unbounded does not mean unlimited. Every allocation
 * goes through one limb budget, and exceeding it is REFUSED - fb_* returns
 * false and the value is left untouched, never truncated or approximated.
 * FLOWALGEBRA section 30 bounds a computation by fuel so that it terminates;
 * here the same bound is expressed in memory, because for exact rational
 * iteration those are the same bound wearing different clothes. A caller that
 * runs out of budget learns it did, which is the whole difference between
 * `undetermined` and a wrong answer.
 *
 * NO FLOATING POINT anywhere, per section 27.
 *
 * REPRESENTATION. Sign-magnitude, 32-bit limbs, least significant first, no
 * leading zero limbs, and `sign == 0` iff the value is zero - so a canonical
 * value has exactly one encoding and fb_cmp can be a total order rather than a
 * best effort. Rationals keep den > 0 and gcd(num, den) == 1 at all times;
 * normalization is by binary GCD, which needs only shifts, subtractions and
 * comparisons and therefore no division routine.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- the budget ------------------------------------------------------------
 * Global and deliberately so: it is a property of the run, not of a value, and
 * threading it through every operand would obscure the arithmetic without
 * making the bound any more honest. Default is 1<<20 limbs (4 MB). Setting it
 * to 0 means unlimited, which is available and is not recommended. */
void   fb_budget_set(size_t limbs);
size_t fb_budget_get(void);
size_t fb_budget_used(void);   /* peak limbs held across live values */
void   fb_budget_reset_peak(void);

typedef struct {
  uint32_t *d;
  int n;        /* limbs in use */
  int cap;
  int sign;     /* -1, 0, +1; 0 iff n == 0 */
} fb_int;

/* Every fb_int must be fb_init'd before use and fb_free'd after. fb_init
 * cannot fail; it allocates nothing. */
void fb_init(fb_int *a);
void fb_free(fb_int *a);
bool fb_set_i64(fb_int *a, int64_t v);
bool fb_copy(fb_int *dst, const fb_int *src);
bool fb_is_zero(const fb_int *a);
int  fb_bits(const fb_int *a);          /* position of the highest set bit, 0 for zero */
int  fb_ctz(const fb_int *a);           /* trailing zero bits, 0 for zero */

int  fb_cmp_abs(const fb_int *a, const fb_int *b);
int  fb_cmp(const fb_int *a, const fb_int *b);

/* r may alias a or b. Each returns false only on budget exhaustion or an
 * allocation failure, and leaves r unchanged when it does. */
bool fb_add(fb_int *r, const fb_int *a, const fb_int *b);
bool fb_sub(fb_int *r, const fb_int *a, const fb_int *b);
bool fb_mul(fb_int *r, const fb_int *a, const fb_int *b);
bool fb_shl(fb_int *r, const fb_int *a, unsigned k);
bool fb_shr(fb_int *r, const fb_int *a, unsigned k);   /* toward zero */
bool fb_gcd(fb_int *r, const fb_int *a, const fb_int *b);  /* of |a|, |b| */
void fb_neg(fb_int *a);

/* Truncating division: q = a / b toward zero, rem carries the sign of a, and
 * a == q*b + rem exactly. Shift-and-subtract, so it needs no reciprocal and no
 * rounding mode. Either output pointer may be NULL. Returns false if b is zero
 * or the budget is exhausted. */
bool fb_divmod(fb_int *q, fb_int *rem, const fb_int *a, const fb_int *b);
/* Value of a/b scaled by 2^shift, truncated toward zero, as an int64. Returns
 * false when the result does not fit - which a caller comparing against a
 * fixed-point path needs to know rather than receive as a saturated value. */
bool fb_rat_to_fixed(const fb_int *num, const fb_int *den, unsigned shift,
                     int64_t *out);

/* Decimal rendering. Returns the number of characters that would be written,
 * so a caller can size a buffer; writes nothing beyond cap. Uses repeated
 * division by 10^9, which is the only place a division appears. */
int fb_format(const fb_int *a, char *out, int cap);
/* Parses an optionally signed decimal integer. Refuses trailing junk. */
bool fb_parse(fb_int *a, const char *s);

/* --- rationals -------------------------------------------------------------- */

typedef struct { fb_int num, den; } fb_rat;

void fb_rat_init(fb_rat *q);
void fb_rat_free(fb_rat *q);
bool fb_rat_set_i64(fb_rat *q, int64_t n, int64_t d);      /* d != 0 */
bool fb_rat_copy(fb_rat *dst, const fb_rat *src);
/* Reduces to lowest terms with den > 0. Called by every operation below, so a
 * caller never sees an unreduced value - which matters because an unreduced
 * denominator grows without bound and the budget would fire on bookkeeping
 * rather than on real magnitude. */
bool fb_rat_normalize(fb_rat *q);
bool fb_rat_add(fb_rat *r, const fb_rat *a, const fb_rat *b);
bool fb_rat_sub(fb_rat *r, const fb_rat *a, const fb_rat *b);
bool fb_rat_mul(fb_rat *r, const fb_rat *a, const fb_rat *b);
/* Comparison against a small integer, by cross-multiplication - so no division
 * is performed and no rounding can occur. Returns -1, 0, +1. */
int  fb_rat_cmp_i64(const fb_rat *a, int64_t v);
int  fb_rat_cmp(const fb_rat *a, const fb_rat *b);
/* Widest of the two limb counts - the natural measure of how much room a value
 * is occupying, and what the Mandelbrot sweep reports. */
int  fb_rat_bits(const fb_rat *a);
/* "n/d", or "n" when d is 1. */
int  fb_rat_format(const fb_rat *a, char *out, int cap);
/* Parses "n", "n/d", or a terminating decimal such as "-0.3125", which is
 * exact: it becomes n/10^k and is then reduced. Decimals that do not terminate
 * cannot be written and so cannot be misread. */
bool fb_rat_parse(fb_rat *q, const char *s);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_BIG_H */
