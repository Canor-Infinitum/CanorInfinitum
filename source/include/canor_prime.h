#ifndef CANOR_PRIME_H
#define CANOR_PRIME_H

/* Primes, primality residuals, and the L_m residual function.
 *
 * WHY PRIMES BELONG IN THE TOWER AT ALL. Ore polynomials need a provenance of
 * primes: the skew polynomial ring K[x][d; sigma, delta] factors differently
 * from the commutative one, and which factorisations exist depends on the
 * arithmetic of the base. A residual function over n! also needs the
 * factorisation of n! itself, which is a statement about primes below n
 * (Legendre's formula). So this is not a utility bolted on the side; it is the
 * arithmetic the symbolic layer stands on.
 *
 * THE SIEVE IS EULER'S, NOT ERATOSTHENES'. Eratosthenes crosses a composite
 * off once per distinct prime factor, so 60 = 2^2.3.5 is visited three times
 * and the total work is n log log n. Euler's linear sieve crosses each
 * composite off exactly ONCE - by its smallest prime factor - and is O(n). The
 * inner loop breaks the moment p divides i, and that break is the whole
 * algorithm: it is what guarantees each composite is reached only through its
 * least factor.
 *
 * The smallest-prime-factor table falls out of the same pass for free, which
 * turns factorisation into a walk of length omega(n) rather than a trial
 * division.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_big.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANOR_PRIME_LIMIT
#define CANOR_PRIME_LIMIT 1000000
#endif

/* Builds the sieve to `limit` (clamped to CANOR_PRIME_LIMIT). Idempotent. */
bool canor_prime_init(int32_t limit);
int32_t canor_prime_count(void);
int32_t canor_prime_at(int32_t i);          /* i-th prime, 0-based; 0 if out of range */
bool canor_is_prime(int32_t n);
int32_t canor_smallest_factor(int32_t n);   /* 0 for n < 2 */

#define CANOR_FACTORS_MAX 20
typedef struct {
  int32_t p[CANOR_FACTORS_MAX];
  int32_t e[CANOR_FACTORS_MAX];
  int n;
} canor_factorisation;

bool canor_factorise(int32_t n, canor_factorisation *out);

/* --- primality residuals ---------------------------------------------------
 * "Residual" here means what is left of n after a structural question is asked
 * of it, and each of these is exact and cheap once the sieve exists.
 *
 *   omega     distinct prime factors
 *   Omega     prime factors with multiplicity
 *   phi       Euler totient: how many residues below n are coprime to it
 *   mu        Moebius: 0 when squareful, else (-1)^omega
 *   radical   the squarefree kernel, product of the distinct primes
 *
 * phi and mu are the two that carry weight downstream: phi is the order of the
 * unit group, which is what decides whether a modular Ore operator can be
 * periodic at all, and mu is the inversion kernel for any sum over divisors. */
int32_t canor_omega(int32_t n);
int32_t canor_bigomega(int32_t n);
int32_t canor_totient(int32_t n);
int canor_mobius(int32_t n);
int32_t canor_radical(int32_t n);

/* Legendre: the exponent of prime p in n!, computed as a sum of floors rather
 * than by factorising n!, which is not representable past 20!. */
int64_t canor_factorial_valuation(int32_t n, int32_t p);

/* --- the L_m residual function ---------------------------------------------
 *
 *     L_m(n; Theta, Phi) = exp( (1/n!) . {Order_m - X} . exp(Theta + Phi.i) )
 *
 * where Order_m is the Kapprox order of MetaFlow class m and X is the
 * Flow/MetaFlow/MultiFlow term.
 *
 * TWO LAYERS, AND THEY MUST NOT BE CONFUSED.
 *
 * exp(Theta + Phi.i) is transcendental. It cannot be evaluated in the exact
 * rationals this core is built on, and section 27 forbids putting floating
 * point in the deterministic path. So L_m's VALUE is a report-layer quantity,
 * computed in double, and it is marked as such.
 *
 * What IS exact, and what turns out to matter more, is the RATIO
 * (Order_m - X)/n! - because its limit separates the four MetaFlow classes
 * outright:
 *
 *   General      (2n+1)/n!        -> 0        so L -> exp(0) = 1
 *   Specific     (2(n!)+1)/n!     -> 2        so L -> exp(2.exp(Theta+Phi.i))
 *   Distinctive  (2n+1)!/n!       -> infinity  L diverges
 *   Arbitrary    (2(n!)+1)!/n!    -> infinity  L diverges faster
 *
 * That is a real property of the residual function rather than a numerical
 * observation about it: the four classes have four different limit behaviours,
 * and the classification needs no exponential at all. Callers that want to
 * separate MetaFlows should read the class; callers that want a number should
 * read the value and accept that it is floating point. */

/* --- deterministic transcendentals -----------------------------------------
 * THE DISTINCTION THAT MATTERS. Section 27 forbids floating-point
 * NONDETERMINISM, not approximation. The objection to double is not that it is
 * inexact - it is that two hosts, two compilers or two summation orders
 * disagree, so a result cannot be reproduced or stamped. Approximation that is
 * bit-identical everywhere does not have that problem.
 *
 * So exp is computed as a truncated Taylor series IN EXACT RATIONALS. Every
 * operation is rational arithmetic, the term count is declared, and the
 * remainder is bounded by the first omitted term. The result is not exact and
 * does not claim to be; it is REPRODUCIBLE, with a bound attached, which is
 * what lets the residual be carried in an artifact and covered by a stamp.
 *
 * Range reduction matters here: the Taylor series for exp converges everywhere
 * but needs many terms for large |x|, and each term costs bignum work. So x is
 * reduced by halving until |x| <= 1/2 and the result squared back up, which
 * turns a slow series into a fast one and keeps the bound tight. */

typedef struct {
  bool ok;
  int terms;               /* terms actually summed */
  int squarings;           /* range-reduction halvings undone */
  int64_t bound_log2_milli;/* log2 of the remainder bound, in milli-units */
} canor_exp_report;

/* exp(x) for exact rational x, to `terms` Taylor terms after range reduction.
 * Writes a bignum rational, so no precision is lost in the accumulation. */
bool canor_exp_rat(canor_rational x, int terms, fb_rat *out, canor_exp_report *rep);
/* cos and sin of an exact rational, same construction. */
bool canor_cos_rat(canor_rational x, int terms, fb_rat *out, canor_exp_report *rep);
bool canor_sin_rat(canor_rational x, int terms, fb_rat *out, canor_exp_report *rep);

typedef enum {
  CANOR_RESID_UNIT = 0,      /* ratio -> 0, L -> 1 */
  CANOR_RESID_CONVERGENT,    /* ratio -> a finite non-zero limit */
  CANOR_RESID_DIVERGENT,     /* ratio -> infinity */
  CANOR_RESID_UNDEFINED      /* the ratio could not be evaluated */
} canor_resid_class;

const char *canor_resid_class_name(canor_resid_class c);

typedef struct {
  canor_resid_class cls;
  /* log2 of the ratio, in milli-units, on the same scale as
   * canor_kapprox_order_milli - so it stays comparable where the ratio itself
   * long ago left every representable range. */
  int64_t log2_ratio_milli;
  /* DETERMINISTIC, and not the same thing as exact. The value below is a
   * truncated rational Taylor sum: bit-identical on every host, with the
   * remainder bounded by `bound_log2_milli`. `exact` stays false because it is
   * an approximation - but it is a reproducible one, so it may be stamped,
   * compared across runs and carried in an artifact. */
  bool exact;
  bool deterministic;
  int64_t bound_log2_milli;
  double value_re, value_im;   /* convenience rendering of the rationals below */
  fb_rat exact_re, exact_im;   /* the reproducible values themselves */
} canor_residual;

/* Frees the bignum members. */
void canor_residual_free(canor_residual *r);

/* m is an canor_kclass. `x_milli` is log2 of the X term, in milli-units, so a
 * caller can pass a Flow/MetaFlow/MultiFlow magnitude without ever forming it.
 * theta and phi are report-layer only and do not affect cls or the ratio. */
bool canor_residual_L(canor_kclass m, uint64_t n, int64_t x_milli,
                   double theta, double phi, canor_residual *out);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_PRIME_H */
