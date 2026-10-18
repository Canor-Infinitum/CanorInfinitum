#ifndef CANOR_ORE_H
#define CANOR_ORE_H

/* Ore-algebraic rational skew-trigonometric operators.
 *
 * WHAT THIS REPLACES AND WHY. The default abstract operators were a hash of
 * the Strategy and the argument. That satisfied the letter of FLOWALGEBRA
 * section 13 - the operators are Strategy-indexed and are not required to obey
 * trigonometric identities - but it threw away the reason the names sin/cos/tan
 * were chosen at all. A hash has no structure to lose: it obeys no identity,
 * has no invariant, and nothing about it can be checked beyond "it is
 * reproducible". Tagging an operator with a Strategy subscript is only
 * meaningful if the subscript indexes a FAMILY OF OPERATORS, and a family
 * needs an algebra to live in.
 *
 * THE ALGEBRA. An Ore extension K[x][d; sigma, delta] - a skew polynomial ring
 * in which d does not commute with the coefficients:
 *
 *     d . f  =  sigma(f) . d  +  delta(f)
 *
 * With sigma the shift and delta zero, this is the shift algebra, and the
 * classical trigonometric pair is the solution space of the second-order skew
 * polynomial
 *
 *     L  =  d^2  -  tau . d  +  sigma
 *
 * which is the operator that ANNIHILATES the pair. Reading it as a recurrence:
 *
 *     C(0) = 1     C(1) = tau/2      C(n+1) = tau.C(n) - sigma.C(n-1)
 *     S(0) = 0     S(1) = delta_amp       S(n+1) = tau.S(n) - sigma.S(n-1)
 *     T(n) = S(n) / C(n)                     undefined where C(n) = 0
 *
 * Every value stays in the rationals. No floating point, no transcendentals,
 * no approximation - section 27 satisfied by construction rather than by care.
 * `tau` plays the role of 2.sqrt(sigma).cos(theta) and `sigma` is the twist; sigma = 1 gives
 * the ordinary shift algebra and q != 1 a genuine skew one.
 *
 * THE INVARIANT, WHICH IS THE POINT. The step matrix is [[a, -q], [1, 0]] with
 * determinant sigma, so the Casoratian - the discrete Wronskian -
 *
 *     W(n)  =  C(n).S(n+1)  -  C(n+1).S(n)
 *
 * satisfies W(n) = sigma^n . W(0) exactly. That is the skew-Pythagorean law, and
 * it is what replaces sin^2 + cos^2 = 1. It is an identity these operators DO
 * satisfy, it is checkable in exact arithmetic at every n, and for sigma = 1 it is
 * literally constant. A hash has nothing of the kind.
 *
 * The conventional identity is NOT satisfied and is asserted not to be: at
 * tau = 1, n = 1, S^2 + C^2 = 1 + 1/4 = 5/4. Section 13 declines those
 * identities, so a suite that accidentally started passing them would mean the
 * implementation had drifted into real trigonometry.
 *
 * NIVEN'S THEOREM BOUNDS THE PERIODIC CASE, and this is not a design choice.
 * 2cos(r.pi) is rational for rational r only at 0, +-1, +-2. So over the
 * rationals there are exactly five values of `a` whose operators are periodic;
 * every other rational `a` gives an aperiodic sequence whose numerators and
 * denominators grow without bound and which therefore EXHAUSTS int64 and is
 * refused. That is a theorem constraining the design rather than a limit of
 * this code, and the suite checks both sides of it.
 *
 *     a =  2   period 1     a =  1   period 6     a =  0   period 4
 *     a = -1   period 3     a = -2   period 2     otherwise aperiodic
 *
 * SIGNED ZEROS FINALLY HAVE A SOURCE. FLOWVALUE distinguishes NegZero from
 * PosZero, and until now that distinction was declared and formal. Here a zero
 * of C or S is a zero of a real sequence, and the sequence is either rising or
 * falling through it. The forward difference decides the pole - rising through
 * zero is PosZero, falling is NegZero - so the four-valued encoding is derived
 * from the operator rather than stipulated.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_flow.h"
#include "canor_strategy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NAMES. These are the Ore extension's own parameters, so they carry its
 * names rather than ad-hoc letters:
 *
 *   sigma  THE TWIST. In K[x][d; sigma, delta] this is the endomorphism, and
 *          here it is exactly what the Casoratian scales by: W(n) = sigma^n .
 *          W(0). sigma = 1 is the untwisted shift algebra. Calling it "q"
 *          hid that it is the same object the algebra is named after.
 *
 *   tau    THE TRACE of the step matrix [[tau, -sigma], [1, 0]], and the
 *          coefficient of d in L = d^2 - tau.d + sigma. It plays the role of
 *          2.sqrt(sigma).cos(theta), so it sets the angle while sigma sets the
 *          amplitude. "a" said nothing about which of the two it was.
 *
 *   delta_amp  THE delta-SOLUTION'S AMPLITUDE. The pair is spanned by two
 *          solutions distinguished by their initial data: C starts (1, tau/2),
 *          and S starts at the Kronecker delta, S(n) = delta_{n,1}, scaled by
 *          this. So S is the delta-initialised solution and this is how much of
 *          it there is - which is what "s0" was, without saying so.
 *
 * delta ITSELF - the sigma-derivation of the Ore extension - is ZERO in this
 * construction and is not a field here. A nonzero derivation would make
 * d.f = sigma(f).d + delta(f) genuinely inhomogeneous and is a larger change
 * than renaming; leaving the field out is more honest than carrying one that
 * is always zero.
 *
 * The old names a, q, s0 remain accepted in configuration files, so nothing
 * already written stops working. */
typedef struct {
  canor_rational tau;        /* trace; was `a` */
  canor_rational sigma;      /* twist; was `q` */
  canor_rational delta_amp;  /* amplitude of the delta-initialised solution; was s0 */
  int32_t reduce;
} canor_ore;

/* q = 1, s0 = 1, and `a` as given. Returns false if `a` or `s0` will not
 * parse, so a Strategy cannot be half-configured. */
bool canor_ore_init(canor_ore *o, const char *tau, const char *sigma,
                 const char *delta_amp);

/* The period, or 0 when the operators are aperiodic. Decided by Niven's
 * theorem on `a` when q = 1; any q != 1 is reported aperiodic, because the
 * Casoratian then grows geometrically and the pair cannot repeat. */
int canor_ore_period(const canor_ore *o);

/* C(n), S(n), T(n) exactly. Each returns false rather than a wrong value:
 * cos and sin when the rationals overflow (which is what an aperiodic operator
 * does, and quickly), tan additionally when C(n) is zero. Negative n reflects
 * the classical parity - C is even, S is odd - which keeps the domain total
 * without inventing values. */
bool canor_ore_cos(const canor_ore *o, int32_t n, canor_rational *out);
bool canor_ore_sin(const canor_ore *o, int32_t n, canor_rational *out);
bool canor_ore_tan(const canor_ore *o, int32_t n, canor_rational *out);

/* UNREDUCED evaluation: runs the recurrence n steps with no index reduction.
 *
 * Any periodicity check MUST use this. Comparing canor_ore_cos(n) against
 * canor_ore_cos(n+p) is self-fulfilling, because both are reduced modulo the very
 * period being tested - the check cannot fail whatever the recurrence does.
 * That flaw hid a real defect at a = +-2 (see canor_ore_period). */
bool canor_ore_cos_raw(const canor_ore *o, int32_t n, canor_rational *out);
bool canor_ore_sin_raw(const canor_ore *o, int32_t n, canor_rational *out);

/* True when the characteristic polynomial x^2 - a.x + q has a REPEATED root,
 * i.e. a^2 = 4q. There the solution space is not spanned by two oscillations
 * but by {r^n, n.r^n}, so the sine grows linearly and no period exists however
 * well-behaved the cosine looks. a = +-2 with q = 1 is exactly this case. */
bool canor_ore_degenerate(const canor_ore *o);   /* tau^2 = 4.sigma */

/* W(n) = C(n).S(n+1) - C(n+1).S(n), which must equal sigma^n . W(0). */
bool canor_ore_wronskian(const canor_ore *o, int32_t n, canor_rational *out);

/* Quantisation to the four-valued FLOWVALUE. `next` is the following term, and
 * it is what gives a zero its pole: a sequence rising through zero yields
 * PosZero, falling yields NegZero. Passing a poison `next` at a zero resolves
 * to PosZero, declared. */
FLOWVALUE canor_ore_quantize(canor_rational v, canor_rational next);

/* The plug-in. Assign to canor_strategy.ops and the whole existing machinery -
 * families, flows, clouds, trees - runs on Ore operators instead of the hash,
 * with no other change. The canor_ore used is the one most recently bound by
 * canor_ore_bind for that Strategy name. */
/* Returns false when the binding table is full. A silent failure here is
 * particularly dangerous: an unbound Strategy falls back to returning a
 * CONSTANT from every operator, which produces a plausible-looking run with no
 * structure in it at all. Callers must check. */
bool canor_ore_bind(const char *strategy_name, const canor_ore *o);

/* PER-OPERATOR binding. sin_S, cos_S and tan_S are three DIFFERENT operators;
 * giving them one shared (a, q, s0) made them three readings of a single
 * sequence, which is a much smaller family than section 13 describes. Each now
 * carries its own skew polynomial, so a Strategy is a triple of Ore operators
 * rather than one wearing three hats.
 *
 * canor_ore_bind sets all three at once and remains the shorthand for the case
 * where they genuinely are the same operator. */
bool canor_ore_bind_op(const char *strategy_name, canor_absop op, const canor_ore *o);
/* Reads back what is bound, so a caller can render or report it. Returns NULL
 * when nothing is bound for that name. */
const canor_ore *canor_ore_bound(const char *strategy_name, canor_absop op);
FLOWVALUE canor_ore_absop(const STRATEGY *s, canor_absop op, int32_t x);

/* An operator that cannot be evaluated has to become SOMETHING when it is used
 * as an abstract operator, because canor_absop_fn has no error channel. The
 * declared answer is the refusal pole, PosZero, and canor_ore_refusals counts how
 * often it was taken - so a run that quietly degenerated into constants is
 * visible instead of merely plausible. */
uint64_t canor_ore_refusals(void);
void canor_ore_reset_refusals(void);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_ORE_H */
