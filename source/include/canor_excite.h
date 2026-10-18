#ifndef CANOR_EXCITE_H
#define CANOR_EXCITE_H

/* Excitations: Cayley-Dickson-extended Lie brackets over all imaginary
 * channels simultaneously, optionally with Ore-skew scalars.
 *
 * WHAT IS AND IS NOT CLAIMED HERE. The bracket structure below is ordinary
 * mathematics and every number it produces is exact and checkable. The
 * interpretation layer around it - that these are local generators of
 * evolution, measurement operators in observer-dependent frames, or distortion
 * metrics for inter-observer communication - is a MODELLING CLAIM, and no
 * amount of arithmetic in this file can establish it. The code computes
 * brackets, Jacobiators and frame differences. Whether a Jacobiator is a
 * "distortion metric for inter-observer communication" is asserted by the
 * framework, not demonstrated by the compiler, and this header says so once so
 * that the results below are not mistaken for evidence of it.
 *
 * THE BRACKET. For basis elements of a Cayley-Dickson algebra,
 *
 *     [e_a, e_b]  =  e_a . e_b  -  e_b . e_a
 *
 * and because a product of two basis elements is always +-e_k, the bracket is
 * either exactly 0 (they commute) or exactly +-2.e_k. So the whole structure is
 * integer, and "infinitesimal change across all imaginary channels
 * simultaneously" is a finite integer tensor c[a][b] -> (coefficient, unit),
 * not a limit.
 *
 * WHY THIS IS WORTH COMPUTING RATHER THAN ASSUMING. The commutator of an
 * associative algebra is always a Lie algebra. Cayley-Dickson algebras stop
 * being associative at dimension 8, so the Jacobi identity
 *
 *     J(x,y,z)  =  [[x,y],z] + [[y,z],x] + [[z,x],y]  =  0
 *
 * is guaranteed only up to the quaternions. At the octonions and beyond it
 * fails, and the Jacobiator J is the exact measure of the failure. That gives
 * this module a prediction fixed in advance:
 *
 *     dim 2  (complex)      every bracket is zero - commutative
 *     dim 4  (quaternion)   Jacobi HOLDS exactly; the brackets close as su(2)
 *     dim 8  (octonion)     Jacobi FAILS on many triples
 *     dim 16 (sedenion)     fails, and more of them
 *
 * An implementation that reported Jacobi holding at dimension 8 would be
 * wrong, and the acceptance suite checks both the holding and the failing
 * cases. A structure that only ever confirmed itself would not be worth having.
 *
 * ORE-SKEW SCALARS. The bracket's coefficient is 2 over the integers. Under a
 * skew scalar the coefficient is drawn from a Strategy's Ore operator instead,
 * so the same bracket carries a different weight at each index - which is what
 * makes the structure scale-variant rather than fixed. The UNIT is unchanged;
 * only the coefficient is skewed. That distinction matters: skewing the unit
 * would change the algebra, while skewing the coefficient changes the weight a
 * frame assigns to a channel, which is the intended reading.
 *
 * SURREAL SCALARS are named in the specification and are NOT implemented. A
 * surreal coefficient would need a birthday-indexed {L|R} representation with
 * its own arithmetic, and infinitesimals cannot be represented in the exact
 * rationals this core is built on. Declaring the field and filling it with
 * rationals would be worse than leaving it out, so it is left out and said so.
 *
 * FRAMES. A frame here is a channel-to-unit assignment - exactly the
 * `metaflows[].unit` map that is already configurable. Two observers with
 * different frames see different brackets between the same channels, and the
 * distortion between them is computable as the number of channel pairs whose
 * bracket differs. That is a real, exact quantity; it is what an
 * inter-observer distortion metric would have to be built from.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_flow.h"
#include "canor_ore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EX_MAX_DIM 16

/* A bracket value: coeff * e_unit. coeff == 0 means the pair commutes, and
 * `unit` is then meaningless and set to 0. */
typedef struct { int32_t coeff; uint8_t unit; } ex_bracket;

/* A vector over the basis, which is what a Jacobiator generally is: the three
 * nested brackets need not land on the same unit. */
typedef struct { int32_t c[EX_MAX_DIM]; } ex_vec;

/* [e_a, e_b]. Requires canor_imag_init. Returns false if the product does not
 * close, which is a property of the table rather than of the arguments. */
bool ex_bracket_of(uint8_t a, uint8_t b, ex_bracket *out);

/* J(e_a, e_b, e_c) as a vector. Zero exactly when Jacobi holds for the triple. */
bool ex_jacobiator(uint8_t a, uint8_t b, uint8_t c, ex_vec *out);
bool ex_vec_is_zero(const ex_vec *v);

/* The associator (e_a e_b) e_c - e_a (e_b e_c), which is what actually fails
 * first: the algebra stops being associative at dimension 8 and the Jacobiator
 * inherits the failure. Reported separately so the two are not confused. */
bool ex_associator(uint8_t a, uint8_t b, uint8_t c, ex_vec *out);

/* Counts over all triples drawn from the first `dim` basis elements. */
typedef struct {
  long triples, jacobi_fail, assoc_fail;
  long pairs, commuting;
} ex_census;
bool ex_census_run(int dim, ex_census *out);

/* An Excitation: the full bracket tensor over `dim` channels, with an optional
 * Ore-skew scalar weight per bracket. */
typedef struct {
  int dim;
  ex_bracket b[EX_MAX_DIM][EX_MAX_DIM];
  canor_rational w[EX_MAX_DIM][EX_MAX_DIM];   /* skew weight; 1 when unskewed */
  bool skewed;
} ex_excitation;

/* Builds the tensor. `o` may be NULL for integer coefficients; when supplied,
 * the weight at (a,b) is the Ore operator evaluated at the index a*dim+b, so
 * the weighting is a deterministic function of the channel pair and the
 * operator - not of anything external. */
bool ex_build(ex_excitation *e, int dim, const canor_ore *o, canor_absop op);

/* Frame distortion: the number of channel pairs on which two Excitations
 * disagree, split into disagreement in the UNIT (a structural difference: the
 * observers do not even see the same channel) and in the WEIGHT alone (the
 * same channel valued differently). Conflating those two would hide the
 * difference between an observer who sees something else and one who sees the
 * same thing differently. */
typedef struct { long pairs, unit_differs, weight_differs; } ex_distortion;

/* --- groupings of the imaginary units --------------------------------------
 * Three ways to partition or cover the imaginary units, and they are NOT
 * interchangeable - each answers a different question, so the caller has to
 * say which it means rather than inherit whichever the implementation picked.
 *
 *   EX_GROUP_INDEX       units 3t+1..3t+3, five disjoint triples. A labelling
 *                        convention. Separable as written; only T0 = {i,j,k}
 *                        closes under multiplication, so words over the other
 *                        four fold OUT of their own group. Use when the
 *                        components must be independently addressable and
 *                        closure is not required.
 *
 *   EX_GROUP_QUATERNION  every triple (a,b,c) that actually spans a quaternion
 *                        subalgebra: a.b = +-c and cyclically. These OVERLAP -
 *                        a unit belongs to many triples - so this is a COVER,
 *                        not a partition, and "repeated" is the accurate word
 *                        for it. Use when each component must fold inside the
 *                        algebra it is written over.
 *
 *   EX_GROUP_FANO        the seven lines of the Fano plane over e1..e7, which
 *                        are exactly the quaternion triples of the octonions.
 *                        A partition of the 21 unordered pairs, though not of
 *                        the units. Use when the octonion structure itself is
 *                        the object.
 *
 * The counts are computed, never hard-coded, so a change to the multiplication
 * table shows up here rather than being contradicted by it. */
typedef enum {
  EX_GROUP_INDEX = 0,
  EX_GROUP_QUATERNION = 1,
  EX_GROUP_FANO = 2,
  /* EX_GROUP_SPREAD  a PARTITION of e1..e15 into five quaternion triples:
   *                  disjoint AND every group closes. Both properties at once,
   *                  which the other three each provide only half of - index
   *                  is disjoint but does not close, quaternion and fano close
   *                  but overlap. There are 56 such partitions; the one built
   *                  here is the lexicographically first, chosen so that the
   *                  grouping is a function of the multiplication table alone
   *                  and not of a search order that might change. */
  EX_GROUP_SPREAD = 3
} ex_grouping;

#define EX_MAX_GROUPS 64

typedef struct {
  ex_grouping kind;
  int count;
  uint8_t u[EX_MAX_GROUPS][3];
  bool closes[EX_MAX_GROUPS];
  bool partition;          /* true when the groups are disjoint */
} ex_groups;

/* Does {a,b,c} span a quaternion subalgebra? Requires a.b = +-c, b.c = +-a and
 * c.a = +-b, which is the closure condition; associativity then follows. */
bool ex_is_quaternion_triple(uint8_t a, uint8_t b, uint8_t c);

/* Builds the requested grouping over the first `dim` basis elements. */
bool ex_groups_build(ex_groups *g, ex_grouping kind, int dim);
const char *ex_grouping_name(ex_grouping k);
bool ex_distort(const ex_excitation *a, const ex_excitation *b, ex_distortion *out);


/* --- spreads and their symmetry -------------------------------------------
 * The 15 imaginary units with the 35 quaternion triples form PG(3,2), and a
 * SPREAD is a partition of the 15 into 5 pairwise disjoint lines. There are 56
 * of them - this code finds them rather than quoting the number.
 *
 * The automorphism group is GL(4,2) ~ A8, order 20160, acting on the unit
 * INDICES read as nonzero vectors of F2^4. That identification rests on
 * e_a . e_b = +-e_(a XOR b), which is a property of the Cayley-Dickson table
 * and is CHECKED before the group is used - if it failed, every orbit below
 * would be computed with the wrong group and still look plausible.
 *
 * Why the orbit split matters: it says how many INEQUIVALENT spread groupings
 * exist. One orbit means every spread is a relabelling of every other, so
 * choosing one is arbitrary and nothing downstream can depend on which. Two or
 * more means the choice is a real parameter and an extraction service has to
 * declare it. */

#define EX_SPREADS_MAX 64

typedef struct {
  int count;
  uint8_t line[EX_SPREADS_MAX][5][3];
} ex_spread_set;

/* True when e_a . e_b = +-e_(a XOR b) for every pair of imaginary units. */
bool ex_index_xor_holds(void);

/* All spreads: partitions of the 15 units into 5 disjoint quaternion triples. */
bool ex_spreads_all(ex_spread_set *out);

typedef struct {
  int norbits;
  int size[EX_SPREADS_MAX];
  int rep[EX_SPREADS_MAX];       /* index into the spread set */
  long group_order;              /* |GL(4,2)|, computed */
  long stabiliser[EX_SPREADS_MAX];
} ex_spread_orbits;

/* Orbits of GL(4,2) on the spreads. */
bool ex_spread_orbit_split(const ex_spread_set *sp, ex_spread_orbits *out);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_EXCITE_H */
