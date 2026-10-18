#ifndef CANOR_STRATEGY_H
#define CANOR_STRATEGY_H

/* Pluggable Strategies and baseline Flows.
 *
 * FLOWALGEBRA section 13 / VERIFIABLEINTELLIGENCE section 13, Differential
 * Families:
 *
 *     di, dj, dk : STRATEGY x Z x Z x Z -/-> FLOWVALUE
 *     Interpretations are strategy-indexed.
 *     The operators sin_S cos_S tan_S are abstract operators and are not
 *     required to satisfy conventional trigonometric identities.
 *
 * THAT LAST SENTENCE DECIDES THE DESIGN. `sin_S` is not sine. It is a
 * Strategy-indexed map from an integer to a FLOWVALUE, and the name is
 * borrowed for its shape - three related operators over a periodic argument -
 * not for its analysis. Implementing it as `sin()` would be wrong twice: it
 * would import identities the specification explicitly declines, and it would
 * put floating point into a substrate section 27 forbids it from.
 *
 * So the abstract operators are integer functions drawn from the Strategy's
 * own deterministic stream, and a Strategy may replace any of them.
 *
 * ARITY. The documents give three arguments (theta, phi, sigma); the worked
 * example supplies four (x_1..x_4). Both are accommodated - a family declares
 * how many it reads, and asking for an argument that was not supplied is an
 * error rather than a zero.
 *
 * PRECEDENCE. The example mixes + - with * /:
 *
 *     dj := + cos(x1) * sin(x2) - cos(x3) * tan(x4)
 *
 * which is a SUM OF SIGNED PRODUCTS, not a left-to-right fold. A flat
 * evaluator would compute ((cos x1 * sin x2) - cos x3) * tan x4 and be quietly
 * wrong, so the representation is a sum of products and the structure carries
 * the precedence rather than the evaluator having to.
 *
 * BASELINE FLOWS are words over the imaginary units. `ijkkji` is a sequence of
 * quaternion basis elements, and folding it under Hamilton's rules gives a
 * signed unit - which is exactly what FOLLOW-CORE's BASE already is ("a
 * doubly-linked list of IMAGINARY ... a permutation group") and what
 * canor_base_eval already computes. The four defaults are registered here; a
 * caller may add more, and any word over the declared alphabet is admissible.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANOR_FAMILY_ARGS_MAX 8
#define CANOR_FAMILY_TERMS_MAX 8
#define CANOR_FAMILY_FACTORS_MAX 4
#define CANOR_FLOW_WORD_MAX 32
#define CANOR_REGISTRY_MAX 32

/* --- FLOWVALUE arithmetic beyond addition ---------------------------------
 * The documents fix addition (section 11 / section 15) and say nothing about
 * the rest. These are therefore DECLARED, not derived, and each says what it
 * chose:
 *
 *   negate   swaps the pole as well as the sign, so negating a latent positive
 *            null gives a latent negative null. Without that, negation would
 *            silently discard the representational distinction section 10
 *            requires be kept.
 *   multiply numeric product; a zero operand yields a zero whose POLE is the
 *            product of the two poles, so sign information survives
 *            multiplication by zero. That is the one place this algebra
 *            differs usefully from the integers.
 *   divide   by a zero-valued operand is UNDEFINED and is reported, not
 *            papered over with a zero or an infinity. */
FLOWVALUE canor_fv_negate(FLOWVALUE v);
FLOWVALUE canor_fv_mul(FLOWVALUE a, FLOWVALUE b);
bool canor_fv_div(FLOWVALUE a, FLOWVALUE b, FLOWVALUE *out);   /* false if undefined */
const char *canor_fv_name(FLOWVALUE v);

/* --- abstract operators ---------------------------------------------------- */

typedef enum { CANOR_ABS_SIN = 0, CANOR_ABS_COS = 1, CANOR_ABS_TAN = 2 } canor_absop;

/* A Strategy may supply its own. NULL uses the default, which is derived from
 * canor_rand and is therefore reproducible and Strategy-dependent without being
 * trigonometric. */
typedef FLOWVALUE (*canor_absop_fn)(const STRATEGY *s, canor_absop op, int32_t x);

/* --- differential families -------------------------------------------------
 *     family := sum of signed products
 *     product := factor { (* | /) factor }
 *     factor  := op(x_index)
 */

typedef enum { CANOR_JOIN_MUL, CANOR_JOIN_DIV } canor_join;

typedef struct {
  canor_absop op;
  uint8_t arg;              /* index into the argument vector */
  canor_join join;             /* how this factor joins the PREVIOUS one */
} canor_factor;

typedef struct {
  int sign;                 /* +1 or -1, the leading sign of the term */
  canor_factor f[CANOR_FAMILY_FACTORS_MAX];
  int nf;
} canor_product;

typedef struct {
  char name[16];
  canor_product term[CANOR_FAMILY_TERMS_MAX];
  int nterms;
  uint8_t arity;            /* arguments this family reads */
} canor_family;

/* --- a pluggable Strategy --------------------------------------------------- */

/* FIVE TRIFECTAS, not one.
 *
 * A Flow is a word over IMAGINARY, and IMAGINARY carries 15 units in five
 * trifectas - so a Strategy with three differential families could only ever
 * differentiate along the first trifecta {i,j,k}. The other twelve units had
 * no family of their own, which made the Strategy strictly less expressive
 * than the Flows it was supposed to act on.
 *
 * d[t][c] is the family for trifecta t, component c. di, dj and dk are
 * d[0][0..2] and keep their names, so everything written against the old
 * three-family Strategy still means what it did.
 *
 * A Strategy may declare fewer than five: ntrifecta says how many are live,
 * and asking for a family beyond it is an error rather than a zero, on the
 * same principle as canor_family_eval refusing a missing argument. */
/* Matches CANOR_IMAG_TRIFECTAS. Written as a literal rather than including fc.h
 * here: this header is included by canor_ore.h, and pulling the whole type tower
 * in through it would make a small dependency into a large one. A mismatch
 * would be a real bug, so the acceptance suite asserts the two are equal. */
#define CANOR_TRIFECTAS 5

typedef struct {
  STRATEGY id;              /* name and version; both feed the RNG */
  BiasMode bias;            /* how a cancelling sum resolves */
  canor_absop_fn ops;          /* NULL for the default abstract operators */
  canor_family d[CANOR_TRIFECTAS][3];
  int ntrifecta;            /* how many of the five are declared */
} canor_strategy;

/* d[0][0..2] under their historical names. */
#define CANOR_DI(s) ((s)->d[0][0])
#define CANOR_DJ(s) ((s)->d[0][1])
#define CANOR_DK(s) ((s)->d[0][2])

/* The example from the brief, as the default:
 *     di := - sin(x1) + tan(x2) + sin(x3) - sin(x4)
 *     dj := + cos(x1) * sin(x2) - cos(x3) * tan(x4)
 *     dk := + cos(x1) - sin(x2) * tan(x3) / sin(x4)
 * Everything is replaceable: swap `ops` for different abstract operators, or
 * assign a different family to di/dj/dk. */
void canor_strategy_default(canor_strategy *s, const char *name, uint32_t version);

/* Parses a family from a compact text form, so a Strategy can be plugged in
 * without recompiling:
 *     "-sin1 +tan2 +sin3 -sin4"          sum of signed factors
 *     "+cos1*sin2 -cos3*tan4"            products bind tighter, as written
 *     "+cos1 -sin2*tan3/sin4"
 * Returns false on anything it cannot read, rather than a partial family. */
bool canor_family_parse(canor_family *f, const char *name, const char *text);
/* Renders a family back to that form. Round-trips, so a plugged-in Strategy
 * can be printed and re-read. */
int canor_family_format(const canor_family *f, char *out, int cap);

/* Evaluates a family. `argc` must be at least the family's arity; supplying
 * fewer is an error, not a zero-fill. Returns false if the family divides by a
 * zero-valued operand, with `out` untouched. */
bool canor_family_eval(const canor_strategy *s, const canor_family *f,
                    const int32_t *args, int argc, FLOWVALUE *out);

bool canor_di(const canor_strategy *s, const int32_t *args, int argc, FLOWVALUE *out);
bool canor_dj(const canor_strategy *s, const int32_t *args, int argc, FLOWVALUE *out);
bool canor_dk(const canor_strategy *s, const int32_t *args, int argc, FLOWVALUE *out);

/* The general form: trifecta t, component c. Returns false when t is beyond
 * the Strategy's declared ntrifecta - an undeclared trifecta has no family and
 * saying so is better than evaluating an empty one to a plausible zero. */
bool canor_dtc(const canor_strategy *s, int t, int c,
            const int32_t *args, int argc, FLOWVALUE *out);


/* --- baseline Flows --------------------------------------------------------
 * A word over the imaginary units. Folded under Hamilton's rules, which is
 * FOLLOW-CORE's BASE - and the fold is non-commutative, so the word order is
 * the content and not a presentation detail.
 */

typedef struct {
  char name[24];
  char word[CANOR_FLOW_WORD_MAX];    /* over i, j, k (and 1 for the identity) */
  uint8_t unit[CANOR_FLOW_WORD_MAX]; /* resolved basis indices */
  int len;
} canor_baseline;

typedef struct {
  canor_baseline flow[CANOR_REGISTRY_MAX];
  int count;
} canor_flow_registry;

/* Registers the four defaults:
 *     Ordinary  ijkkji
 *     Peaceful  iiikji
 *     Formful   jikkkj
 *     Stormful  kkjikk */
void canor_flows_default(canor_flow_registry *r);
bool canor_flows_add(canor_flow_registry *r, const char *name, const char *word);
int  canor_flows_find(const canor_flow_registry *r, const char *name);

/* Folds a baseline word under the Cayley-Dickson product, left to right.
 * Writes the resulting basis index and its sign. Requires canor_imag_init to have
 * succeeded; returns false otherwise, since a product that does not close is
 * not a product. */
bool canor_flow_fold(const canor_flow_registry *r, int idx, uint8_t *unit, int *sign);
const char *canor_unit_name(uint8_t unit);

/* Applies a Strategy's differential family to a baseline Flow: each unit in
 * the word supplies an argument, so the Flow drives the family rather than the
 * caller inventing numbers. This is what makes the two halves plug together. */
bool canor_flow_differential(const canor_strategy *s, const canor_flow_registry *r,
                          int idx, FLOWVALUE *di, FLOWVALUE *dj, FLOWVALUE *dk);
/* The differential of a Flow across EVERY declared trifecta: out[t][c].
 * `defined` receives a bitmask of which (t,c) evaluated - a MultiFlow is
 * partial by nature, since a family can divide by a zero-valued operand, and
 * reporting which parts exist is the difference between a MultiFlow and a
 * MultiFlow with holes silently filled. */
bool canor_flow_multiflow(const canor_strategy *s, const canor_flow_registry *r, int idx,
                       FLOWVALUE out[CANOR_TRIFECTAS][3], uint32_t *defined);


#ifdef __cplusplus
}
#endif
#endif /* CANOR_STRATEGY_H */
