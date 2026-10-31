#ifndef CANOR_ALG_H
#define CANOR_ALG_H

/* Algebraic numbers: exact, deterministic, and without radicals.
 *
 * THE REPRESENTATION. A real algebraic number is (minimal polynomial,
 * isolating interval) - a rational polynomial that it is a root of, together
 * with a rational interval containing that root and no other. That pair names
 * the number EXACTLY: sqrt(2) is (x^2 - 2, [1, 2]) and nothing about it is
 * approximate. The interval can be narrowed to any width on demand by exact
 * rational bisection, so a caller that needs digits gets them without the
 * representation ever having been a float.
 *
 * WHY NOT RADICALS. The quartic is solvable by radicals, and writing the
 * solution out needs cube roots of expressions containing square roots - a
 * tower of nested surds whose arithmetic is far harder to do exactly than the
 * field arithmetic below, and whose sign determination is harder still. The
 * radical formula is a closed form, not a representation.
 *
 * WHY NOT SURREALS. sqrt(2) is a surreal, but its birthday is omega: the
 * representation is an infinite cut, so nothing finite comes out. Exactness in
 * principle is not computability.
 *
 * ROOT COUNTING IS STURM'S, so isolation is rigorous rather than heuristic. A
 * sign change between two points shows an ODD number of roots between them,
 * which is not enough to isolate one; Sturm's sequence gives the EXACT count in
 * any interval, so bisection can be driven until every interval holds exactly
 * one root and no root has been missed.
 *
 * FIELD ARITHMETIC. Elements of Q(alpha) are polynomials in alpha of degree
 * below deg(minpoly), with rational coefficients. Sum is coefficientwise;
 * product is polynomial multiplication followed by remainder modulo the
 * minimal polynomial. Both are exact rational operations, so the field is
 * closed and deterministic with no transcendental anywhere.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_poly.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANOR_ALG_MAX_ROOTS 8

/* Polynomial division with remainder over Q. */
bool canor_poly_divmod(const canor_poly *a, const canor_poly *b, canor_poly *q, canor_poly *r);
bool canor_poly_derivative(const canor_poly *p, canor_poly *out);
/* Removes the content so a Sturm chain does not grow denominators without
 * bound; the sign structure is unchanged, which is all Sturm needs. */
bool canor_poly_primitive(canor_poly *p);

#define CANOR_STURM_MAX 10
typedef struct {
  canor_poly p[CANOR_STURM_MAX];
  int n;
} canor_sturm;

bool canor_sturm_build(const canor_poly *p, canor_sturm *out);
/* Sign changes in the chain evaluated at x. */
int canor_sturm_variations(const canor_sturm *s, canor_rational x);
/* Exact count of distinct real roots in (a, b]. */
int canor_sturm_count(const canor_sturm *s, canor_rational a, canor_rational b);

/* A real algebraic number: a root of `min`, the unique one in [lo, hi]. */
typedef struct {
  canor_poly min;
  canor_rational lo, hi;
  bool exact_rational;      /* true when lo == hi, i.e. the root is rational */
} canor_alg;

/* Cauchy's bound: every real root lies in [-B, B]. Exact, from the
 * coefficients, so isolation never has to guess a search window. */
bool canor_alg_root_bound(const canor_poly *p, canor_rational *B);

/* Isolates every distinct real root into disjoint intervals, each containing
 * exactly one - verified by Sturm rather than assumed from a sign change. */
bool canor_alg_isolate(const canor_poly *p, canor_alg *out, int cap, int *found);

/* Halves the interval `iters` times, exactly. */
bool canor_alg_refine(canor_alg *a, int iters);
/* Midpoint of the current interval - the best rational approximation this
 * representation currently commits to, and it says how wide it still is. */
canor_rational canor_alg_approx(const canor_alg *a, canor_rational *width);

/* --- arithmetic in Q(alpha) ------------------------------------------------- */

typedef struct {
  canor_rational c[CANOR_POLY_MAX];   /* coefficients in alpha */
  int degree;
} canor_alg_elem;

void canor_alg_elem_init(canor_alg_elem *e);
bool canor_alg_elem_add(const canor_alg *f, const canor_alg_elem *a, const canor_alg_elem *b,
                     canor_alg_elem *out);
bool canor_alg_elem_mul(const canor_alg *f, const canor_alg_elem *a, const canor_alg_elem *b,
                     canor_alg_elem *out);
/* Evaluates an element at the field generator's current approximation. Marked
 * as an approximation because that is what it is - the ELEMENT is exact, the
 * decimal rendering of it is not. */
canor_rational canor_alg_elem_approx(const canor_alg *f, const canor_alg_elem *e);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_ALG_H */
