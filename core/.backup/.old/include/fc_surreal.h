#ifndef FC_SURREAL_H
#define FC_SURREAL_H

/* Surreal values in Conway normal form, with a dyadic fast path.
 *
 *     x  =  sum_i  omega^(e_i) . r_i        e_1 > e_2 > ... , r_i != 0
 *
 * EPSILON IS A VALUE HERE, NOT A RATIO. The telemetry model asks for
 * "epsilon as material fact, empty-set-adjacent: an observation below the
 * resolution of its scale - real, recorded, not yet countable at that level".
 * A fraction in [0,1] cannot carry that: a sub-resolution observation stored as
 * 0.0 has been rounded away, and one stored as a small rational is claiming a
 * countable magnitude it does not have. So epsilon = omega^(-1) is its own
 * term, ordered strictly between 0 and every positive rational, and it stays
 * distinguishable however many finite operations are applied to it.
 *
 * THE ONE CLAIM THAT MAKES THE MODEL WORK, and it is checkable:
 *
 *     n . epsilon  is infinitesimal for every finite n
 *     omega . epsilon  =  1
 *
 * So sub-resolution facts accumulate harmlessly at any finite scale and become
 * COUNTABLE at transfinite scale. That is "transfinite smoothing -> singularity"
 * stated exactly rather than by analogy: the breakdown is the point where an
 * omega-fold accumulation of epsilon crosses into the finite band. A model
 * built on rationals cannot express that crossing at all, because it rounds the
 * epsilon to zero long before reaching it.
 *
 * WHAT SUBSET THIS IS, said plainly. Exponents here are RATIONAL, not surreal.
 * That gives omega^1, omega^0 = 1 and omega^(-1) = epsilon - the whole
 * infinite/finite/infinitesimal band structure - and it is closed under the
 * arithmetic below. Full Conway surreals allow surreal exponents, which
 * recurses; that is deliberately not built, because nothing in the telemetry
 * model needs omega^omega and pretending to support it would mean carrying a
 * representation that cannot be compared in bounded time.
 *
 * DYADIC FAST PATH. A value with one term at exponent 0 is an ordinary
 * rational, and fc_surreal_is_real reports it. The fast path is an OPTIMISATION
 * AND NEVER A SEMANTIC DISTINCTION: 1/2 and 1/3 differ in cost only. If they
 * ever differed in behaviour the surreal floor would have leaked upward, and
 * the claim that everything derives from two atoms would go with it.
 *
 * EXACT AND BOUNDED. Coefficients are fc_rational, so poison propagates as it
 * does everywhere else. Term count is capped and the operation REFUSES rather
 * than truncating - a truncated normal form is a plausible wrong number, which
 * is the failure this core exists to avoid.
 */

#include <stdbool.h>
#include <stdint.h>

#include "fc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FS_MAX_TERMS 8

typedef struct {
  fc_rational exp;    /* the omega exponent: 1 = omega, 0 = real, -1 = epsilon */
  fc_rational coef;   /* non-zero */
} fs_term;

typedef struct {
  fs_term t[FS_MAX_TERMS];
  int n;              /* 0 is the surreal zero */
  bool valid;         /* false once a term count or a coefficient overflowed */
} fc_surreal;

/* --- construction ---------------------------------------------------------- */
void fc_surreal_zero(fc_surreal *x);
bool fc_surreal_real(fc_surreal *x, fc_rational r);   /* omega^0 . r */
bool fc_surreal_omega(fc_surreal *x);                 /* omega^1 . 1 */
bool fc_surreal_eps(fc_surreal *x);                   /* omega^-1 . 1 */
/* omega^e . c, the general single term. */
bool fc_surreal_term(fc_surreal *x, fc_rational e, fc_rational c);

/* --- arithmetic ------------------------------------------------------------ */
bool fc_surreal_add(const fc_surreal *a, const fc_surreal *b, fc_surreal *o);
bool fc_surreal_neg(const fc_surreal *a, fc_surreal *o);
bool fc_surreal_sub(const fc_surreal *a, const fc_surreal *b, fc_surreal *o);
/* Convolution of the terms. Refuses when the product needs more than
 * FS_MAX_TERMS distinct exponents. */
bool fc_surreal_mul(const fc_surreal *a, const fc_surreal *b, fc_surreal *o);

/* --- order ----------------------------------------------------------------- */
/* Lexicographic on (exponent, coefficient) from the leading term, which IS the
 * surreal order for normal forms: a larger leading exponent dominates whatever
 * follows, however large its coefficient. -1, 0 or 1; 0 also when either side
 * is invalid, so test fc_surreal_valid when that distinction matters. */
int  fc_surreal_cmp(const fc_surreal *a, const fc_surreal *b);
bool fc_surreal_valid(const fc_surreal *x);
int  fc_surreal_sign(const fc_surreal *x);            /* -1, 0, +1 */

/* --- band classification --------------------------------------------------- */
/* The three bands the telemetry model distinguishes. A value is INFINITE when
 * its leading exponent is positive, REAL when the leading exponent is zero, and
 * INFINITESIMAL when every exponent is negative. Zero is none of the three and
 * reports as real with sign 0 - it is the absence, and epsilon is not it. */
bool fc_surreal_is_infinite(const fc_surreal *x);
bool fc_surreal_is_real(const fc_surreal *x);
bool fc_surreal_is_infinitesimal(const fc_surreal *x);

/* The real part: the coefficient at exponent 0, or zero. This is what a
 * consumer that cannot represent the other bands should read - and it is a
 * LOSSY projection, so a caller reporting it as the value owes the declared
 * record the telemetry model requires. */
fc_rational fc_surreal_real_part(const fc_surreal *x);

/* "3 + 2w - 1/2e" style, for reports. cap >= 96 recommended. */
void fc_surreal_format(const fc_surreal *x, char *buf, int cap);

#ifdef __cplusplus
}
#endif
#endif /* FC_SURREAL_H */
