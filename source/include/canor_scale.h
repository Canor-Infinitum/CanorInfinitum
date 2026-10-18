#ifndef CANOR_SCALE_H
#define CANOR_SCALE_H

/* Scale-relative operators, ordinal-indexed sampling, and PG(3,2) duality.
 *
 * THREE INDEPENDENT PIECES, and they are kept separable because they answer
 * different questions and could each be wrong on its own.
 *
 * ---------------------------------------------------------------------------
 * 1. ORDINALS IN CANTOR NORMAL FORM
 *
 *     lambda  =  omega^e1.c1 + omega^e2.c2 + ... + n,      e1 > e2 > ... > 0
 *
 * WHY ORDINALS AND NOT SURREALS. canor_excite declares surreal scalars out of
 * scope and correctly so: a surreal coefficient needs birthday-indexed {L|R}
 * data with infinitesimals, and infinitesimals cannot be represented in the
 * exact rationals this core is built on. Ordinals have no such problem. A CNF
 * ordinal is a short list of (exponent, coefficient) pairs - finite data - and
 * comparison is lexicographic on that list, so it is exact, total and
 * deterministic. Nothing is approximated and nothing is declared that cannot
 * be computed.
 *
 * WHAT THE INDEX IS FOR. A smoothing ladder needs a well-order, not merely a
 * long one. Under omega.k + n the finite part n counts refinements WITHIN a
 * growth class and the omega part k counts the classes themselves, so
 *
 *     omega.k + n  <  omega.k + (n+1)  <  ...  <  omega.(k+1)
 *
 * says exactly that no amount of refinement at class k reaches class k+1.
 * That is the real relationship between refining a root's interval and moving
 * up the Kapprox ladder, and an integer index would have flattened it into a
 * single sequence that suggests the wrong thing: that enough halvings of an
 * interval eventually amount to a change of growth class. They do not.
 *
 * ---------------------------------------------------------------------------
 * 2. SCALE-RELATIVE OPERATORS OVER Q(alpha)
 *
 * An algebraic number carries its own scale: canor_alg isolates a root into
 * [lo,hi] and canor_alg_refine halves that interval exactly. The WIDTH is a
 * known, exact quantity, so "sample this operator at scale s" is a statement
 * with a definite meaning here rather than a figure of speech.
 *
 * A scale-relative operator is therefore an element of Q(alpha) together with
 * an ordinal index that says how far alpha has been refined and which growth
 * class is in force. Sampling it at two indices gives two exactly-known
 * precisions, and the difference between the samples is attributable to the
 * scale rather than to accumulated error - which is the property that makes
 * the ladder worth having.
 *
 * ---------------------------------------------------------------------------
 * 3. PG(3,2) DUALITY
 *
 * THIS IS AN INTERPRETATION AND IS LABELLED AS ONE. It is offered as the
 * nearest exactly-defined involution to a request for a "Poincare-negation",
 * not as an implementation of that phrase. What is implemented is standard
 * projective duality and nothing more.
 *
 * Points of PG(3,2) are the 15 non-zero vectors of F_2^4. With the standard
 * symmetric bilinear form <u,v> = parity(popcount(u AND v)), which is
 * non-degenerate, each point v corresponds to the hyperplane
 *
 *     H_v  =  { x : <v,x> = 0 }
 *
 * and each line - a 2-dimensional subspace {u, v, u XOR v} - maps to the set
 * of vectors annihilating it, which is again a line. Because the form is
 * symmetric and non-degenerate the map is an INVOLUTION: dual(dual(L)) = L.
 * That is checked here rather than assumed, since a wrong form would still
 * produce plausible-looking triples.
 *
 * WHY IT IS THE USEFUL TEST. Duality permutes the 15 points, so a quantity
 * summed over all of them is invariant for trivial reasons. A PER-POINT
 * quantity generally is not. So asking whether a bias profile's Golay class
 * survives duality separates structure from frame artifact: an invariant
 * reading is about the algebra, a varying one is about the labelling. That
 * distinction is exactly what a relational bias measure needs and cannot get
 * from a single frame.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_alg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- ordinals --------------------------------------------------------------- */

#define FO_MAX_TERMS 4

typedef struct {
  int exp;          /* omega exponent; 0 is the finite term */
  int64_t coef;     /* > 0 */
} fo_term;

typedef struct {
  fo_term t[FO_MAX_TERMS];
  int n;            /* 0 is the ordinal zero */
} fo_ord;

void fo_zero(fo_ord *o);
/* omega^e . c + n. Refuses c <= 0 or e < 0 rather than normalising them away,
 * because a zero coefficient is a different ordinal from an absent term only
 * if it is allowed in, and CNF does not allow it. */
bool fo_make(fo_ord *o, int e, int64_t c, int64_t n);
/* Total order. Lexicographic on the CNF terms, which is the ordinal order. */
int fo_cmp(const fo_ord *a, const fo_ord *b);
/* A limit ordinal has no finite part and is not zero: nothing is its
 * immediate predecessor. */
bool fo_is_limit(const fo_ord *o);
/* Human form, e.g. "w*2+3". Buffer must hold at least 64 bytes. */
void fo_format(const fo_ord *o, char *buf, int cap);
/* The finite part, and the omega^1 coefficient - the two the sampler uses. */
int64_t fo_finite_part(const fo_ord *o);
int64_t fo_omega_part(const fo_ord *o);

/* --- scale-relative sampling ------------------------------------------------ */

typedef struct {
  canor_rational value;      /* the element, evaluated at this scale */
  canor_rational width;      /* the exact width of alpha's isolating interval */
  int kclass;             /* growth class in force, from the omega part */
  int refinements;        /* how many halvings were applied */
  bool valid;
} fs_sample;

/* Samples `e` in Q(alpha) at ordinal index `at`. `a` is refined in place to the
 * requested depth, so successive calls at increasing indices do no repeated
 * work and the width shrinks monotonically. Returns false if the ordinal
 * exceeds what the representation can honour, which is reported rather than
 * silently truncated. */
bool fs_sample_at(canor_alg *a, const canor_alg_elem *e, const fo_ord *at,
                  fs_sample *out);

/* --- PG(3,2) duality --------------------------------------------------------- */

/* <u,v> over F_2: parity of the popcount of the bitwise and. */
int pg_form(uint8_t u, uint8_t v);

/* The 3 points of the line spanned by u and v: u, v, u XOR v. Refuses u == v
 * or either zero - those do not span a line. */
bool pg_line(uint8_t u, uint8_t v, uint8_t out[3]);

/* The dual line: the vectors annihilating both u and v. Exactly 3 of them, and
 * `out` receives them sorted so two calls on the same line agree elementwise
 * rather than merely as sets. */
bool pg_dual_line(uint8_t u, uint8_t v, uint8_t out[3]);

/* Duality applied to a whole 15-bit point profile: bit p moves to the position
 * its dual hyperplane occupies. Under the standard form the hyperplane of
 * point v is indexed by v itself, so this is the identity on profiles - which
 * is stated here because it is a genuine and slightly surprising fact about
 * the standard form, not an unimplemented stub. A profile test that needs a
 * non-trivial relabelling has to supply its own correlation; see pg_dual_perm.
 */
uint32_t pg_dual_profile(uint32_t profile);

/* A duality composed with a permutation of coordinates - a correlation that is
 * NOT the identity on profiles, so invariance under it is a real test. `perm`
 * maps bit positions 0..3 of the F_2^4 coordinates. Refuses a non-permutation.
 */
bool pg_dual_perm(uint32_t profile, const int perm[4], uint32_t *out);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_SCALE_H */
