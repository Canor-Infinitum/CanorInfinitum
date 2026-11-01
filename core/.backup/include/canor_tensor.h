#ifndef CANOR_TENSOR_H
#define CANOR_TENSOR_H

/* Generalized quaternionic and multi-quaternionic tensor product algebras.
 *
 *     H^(x)m  =  H (x) H (x) ... (x) H     (m factors, over Q)
 *
 * THIS IS THE OTHER WAY TO GROW FROM H, AND IT FAILS OPPOSITELY.
 *
 * Cayley-Dickson doubling (canor_excite) keeps division and loses structure:
 * associativity goes at dimension 8, alternativity at 16, and by the sedenions
 * there are zero divisors. Tensoring keeps structure and loses division:
 * H^(x)m is ASSOCIATIVE for every m, but already H (x) H = M_4(R) has zero
 * divisors and is not a division algebra.
 *
 * The consequence is sharp and it is why this module exists separately rather
 * than inside canor_excite:
 *
 *     THE JACOBIATOR VANISHES IDENTICALLY HERE.
 *
 * The commutator of any associative algebra is a Lie algebra, so J(x,y,z) = 0
 * on every triple - the Jacobi zero-set is everything. In the Cayley-Dickson
 * direction it is 105 of 1365 channel-triples, and that split is what the
 * relative-bias probe extracts over. So these two constructions are
 * complementary rather than alternative: the tensor side supplies exact
 * associative structure to compute WITH, and the Cayley-Dickson side supplies
 * the graded failure to measure. A tool that used the tensor side for the
 * probe would find no structure at all, and that is a prediction this header
 * fixes in advance rather than a caveat added afterwards.
 *
 * WHAT IS ESTABLISHED MATHEMATICS AND WHAT IS NOT. That H (x) H = M_4(R), that
 * C (x) H = M_2(C), that these are central simple algebras classified by the
 * Brauer group, and that a tensor product of associative algebras is
 * associative - all standard. The acceptance suite CHECKS them rather than
 * assuming them, because an implementation that quietly got a sign wrong would
 * still typecheck. Nothing in this file claims a physical or semantic reading
 * of the algebra; that belongs to the framework.
 *
 * ALREADY PRESENT IN THE FRAMEWORK. WORLD_MODEL Sec.8.1 carries OrigoDelta on
 * B = C (x) H, the biquaternions. So the framework already uses one
 * multi-quaternionic tensor product; this module generalizes it rather than
 * introducing the idea.
 *
 * THE REPRESENTATION IS EXACT AND SMALL. A basis element of H^(x)m is an
 * m-tuple over {1,i,j,k}, packed two bits per factor. Quaternion index
 * multiplication is XOR - i.j = k is 1 XOR 2 = 3 - so componentwise XOR is a
 * single word XOR, and the product of two basis elements is
 *
 *     e_a . e_b  =  s(a,b) . e_(a XOR b),      s(a,b) in {+1,-1}
 *
 * exactly as in the Cayley-Dickson case. The sign is the product of the m
 * per-factor signs. So the whole structure is integer: no rounding enters, and
 * dimension 4^m is reachable without ever forming a 4^m by 4^m table.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Two bits per factor, in a uint32_t: m <= 15 without widening. Beyond that
 * the packing would need a wider word, which is a real limit and is reported
 * rather than silently wrapped. */
#define FT_MAX_FACTORS 15

/* dim(H^(x)m) = 4^m. Returns 0 if m is out of range or the dimension would
 * overflow a uint64_t. */
uint64_t ft_dim(int m);

/* The sign of e_a . e_b in H^(x)m. Returns false if a or b is not a valid
 * basis index for m factors - an out-of-range index is a caller error, not a
 * value to be masked into range. */
bool ft_sign(int m, uint32_t a, uint32_t b, int *sign);

/* e_a . e_b = coeff . e_unit. `unit` is always a XOR b; only the sign varies. */
typedef struct { int32_t coeff; uint32_t unit; } ft_term;
bool ft_mul(int m, uint32_t a, uint32_t b, ft_term *out);

/* [e_a, e_b] = e_a.e_b - e_b.e_a, which is 0 or +-2 . e_(a XOR b) and never
 * anything else - the same shape the Cayley-Dickson bracket has, so the two
 * can be compared directly. */
bool ft_bracket(int m, uint32_t a, uint32_t b, ft_term *out);

/* The associator (e_a e_b) e_c - e_a (e_b e_c). Zero on every triple, because
 * the algebra is associative. Exposed so the suite can CHECK that rather than
 * trust it. */
bool ft_associator(int m, uint32_t a, uint32_t b, uint32_t c, ft_term *out);

/* The Jacobiator J(e_a,e_b,e_c). Zero on every triple, for the same reason.
 * A non-zero result would mean the implementation is wrong, not that the
 * mathematics is interesting. */
bool ft_jacobiator(int m, uint32_t a, uint32_t b, uint32_t c, ft_term *out);

/* A census over the first `cap` basis elements - full when cap >= 4^m. Kept
 * bounded because 4^5 = 1024 gives 1024^3 triples, which is not a census
 * anyone wants to wait for; `sampled` says whether it was complete. */
typedef struct {
  uint64_t dim;
  long pairs, commuting;
  long triples, assoc_fail, jacobi_fail;
  long central;              /* basis elements commuting with all sampled */
  bool sampled;
} ft_census;
bool ft_census_run(int m, long cap, ft_census *out);

/* --- rational rotation ------------------------------------------------------
 * A rotation of one tensor factor's 3-axis by conjugation, v -> q v q~ / |q|^2.
 *
 * EXACT, AND THAT IS THE POINT. With q having rational components the whole
 * conjugation stays in Q: no trigonometry, no irrational axis, nothing to
 * round. The cost is that not every angle is reachable - only those with
 * rational direction cosines - which is a real restriction and is stated
 * rather than hidden behind a float. Callers wanting an arbitrary angle need
 * an approximation policy and should declare it.
 *
 * q need not be a unit quaternion; the division by |q|^2 normalizes it. A zero
 * quaternion is refused rather than producing a poisoned result. */
typedef struct { canor_rational w, x, y, z; } ft_quat;
bool ft_rotate3(const ft_quat *q, const canor_rational v[3], canor_rational out[3]);

/* |q|^2, exact. */
canor_rational ft_quat_norm2(const ft_quat *q);

/* --- convolution over an axis ----------------------------------------------
 * Discrete convolution of two rational sequences: (f * g)[n] = sum f[i] g[n-i].
 * Used to combine per-chart profiles along the chart axis. Exact, and the
 * output length is nf + ng - 1 as usual - a caller that wanted a same-length
 * result has to say which windowing it means, so no default is chosen here. */
bool ft_convolve(const canor_rational *f, int nf, const canor_rational *g, int ng,
                 canor_rational *out, int cap);

/* --- k-d voxelization -------------------------------------------------------
 * A voxel zoom is an exact quantization of the key space: voxel = key >> level,
 * with arithmetic shift so negative coordinates floor consistently rather than
 * folding towards zero and putting two different regions in one bin.
 *
 * Integer shift rather than division by a scale factor: shifting is exact at
 * every level and composes (two zooms of 1 equal one zoom of 2), which a
 * rounding division does not. */
void ft_voxel(const int64_t key[CANOR_KD], int level, int64_t out[CANOR_KD]);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_TENSOR_H */
