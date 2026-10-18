#ifndef CANOR_POLY_H
#define CANOR_POLY_H

/* Rational polynomials: exact roots where they exist, honest refusal where
 * they do not, plus Pade approximants and the Mittag-Leffler function.
 *
 * WHAT IS EXACTLY SOLVABLE AND WHAT IS NOT. A polynomial with rational
 * coefficients has rational roots only when the rational root theorem admits
 * them: p/q with p | a_0 and q | a_n. That is a FINITE set once a_0 and a_n are
 * factorised, which is exactly what the sieve provides, so every rational root
 * is found by construction rather than by search.
 *
 * The irrational roots are a different matter and the difference is not one of
 * effort. sqrt(2) is not a rational and no amount of exact rational arithmetic
 * will produce it; representing it needs either a radical tower or an algebraic
 * number field, and that is a larger object than this header carries. So the
 * contract here is: rational roots are returned exactly, the polynomial is
 * DEFLATED by them, and whatever remains is handed back as an irreducible
 * remainder with its degree stated. A quartic with two rational roots becomes
 * two exact roots and a quadratic remainder; nothing is approximated and
 * nothing is claimed.
 *
 * PADE. The Taylor series is one way to turn coefficients into values, and a
 * poor one near a pole - it can only ever produce a polynomial, and polynomials
 * have no poles. The Pade approximant [m/n] uses the SAME coefficients to build
 * a RATIONAL function, which can. For exp it converges far faster, so the
 * bignum work drops: [6/6] beats a 24-term sum, and being a ratio of rational
 * polynomials it stays exactly representable and therefore deterministic.
 *
 * MITTAG-LEFFLER. E(a,b; z) = sum z^k / Gamma(a.k + b), which is exp at
 * a = b = 1 and is otherwise the fundamental solution of fractional relaxation.
 * It is the inverse Laplace transform of s^(a-1) / (s^a + lambda), so a
 * fractional pole in s is a Mittag-Leffler response in t - which is why a
 * PFISD loop with damping wants it: at a = 1 the response decays
 * exponentially, and at a < 1 it decays as a power law, i.e. with memory.
 *
 * Gamma at rational arguments is not rational, so the series is evaluated with
 * a rational approximation to 1/Gamma at each term and the truncation is
 * declared - deterministic, bounded, not exact, exactly as canor_exp_rat.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_big.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANOR_POLY_MAX 9          /* degree 8; quartics and below are the target */

typedef struct {
  canor_rational c[CANOR_POLY_MAX];  /* c[i] is the coefficient of x^i */
  int degree;
} canor_poly;

void canor_poly_init(canor_poly *p);
bool canor_poly_set(canor_poly *p, int degree, const canor_rational *coeffs);
canor_rational canor_poly_eval(const canor_poly *p, canor_rational x);
/* Synthetic division by (x - r). Returns false when r is not a root, so the
 * quotient is never a silent approximation of one. */
bool canor_poly_deflate(canor_poly *p, canor_rational r, canor_poly *out);

typedef struct {
  canor_rational root[CANOR_POLY_MAX];
  int mult[CANOR_POLY_MAX];
  int n;                       /* distinct rational roots found */
  canor_poly remainder;           /* what could not be factored over Q */
  int remainder_degree;
  bool complete;               /* true when the remainder is a constant */
} canor_poly_factors;

/* Every rational root, by the rational root theorem over the sieve's divisors.
 * Requires canor_prime_init. Integer-valued numerators and denominators only -
 * a coefficient too large to factorise is refused rather than sampled. */
bool canor_poly_rational_roots(const canor_poly *p, canor_poly_factors *out);

/* The quartic's resolvent cubic. Its rational roots decide whether a quartic
 * with no rational roots nonetheless splits into two rational quadratics -
 * which is the one remaining case that stays inside Q. */
bool canor_poly_resolvent_cubic(const canor_poly *quartic, canor_poly *out);

/* --- Pade ------------------------------------------------------------------ */

/* Builds the [m/n] Pade approximant of exp about 0 and evaluates it at x. All
 * coefficients are rational, so the result is exact rational arithmetic and
 * the only approximation is the truncation the approximant itself represents. */
bool canor_pade_exp(canor_rational x, int m, int n, fb_rat *out, int *squarings);

/* --- Mittag-Leffler --------------------------------------------------------- */

typedef struct {
  bool ok;
  int terms;
  int64_t bound_log2_milli;
  bool deterministic;
} canor_ml_report;

/* E(alpha, beta; z) for rational alpha, beta, z. alpha = beta = 1 gives exp,
 * and that identity is checked in the suite rather than assumed. */
bool canor_mittag_leffler(canor_rational alpha, canor_rational beta, canor_rational z,
                       int terms, fb_rat *out, canor_ml_report *rep);

/* The damped PFISD response: E(alpha, 1; -lambda . t^alpha), which is the
 * inverse Laplace transform of s^(alpha-1)/(s^alpha + lambda). At alpha = 1
 * this is exp(-lambda.t). */
bool canor_fractional_relax(canor_rational alpha, canor_rational lambda, canor_rational t,
                         int terms, fb_rat *out, canor_ml_report *rep);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_POLY_H */
