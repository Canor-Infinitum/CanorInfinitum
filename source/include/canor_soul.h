#ifndef CANOR_SOUL_H
#define CANOR_SOUL_H

/* Soul(TM) Symbols: the count stream of a Flow traversal (Canor WD v1 §4).
 *
 * FLOW VALUES (§1, SCHEMA.md "Flow / MetaFlow"). A Flow is a length n and a
 * total function over Index(n) = {0..n-1} into AtomUsage = (atom, ring). It is
 * immutable, and repeated atoms are DISTINCT POSITIONS: every occurrence
 * counts, nothing is deduplicated. The ring index is CATEGORICAL - it selects
 * an indexed instance (i0, i1, ...) of the same atom name and is carried with
 * the occurrence, never folded into arithmetic. An omitted index is 0.
 *
 * COUNT STREAM (/soul skill, "Definition"). Per registered trifecta:
 *   - the initial vector counts each of the trifecta's three names in the Flow;
 *   - each consumed position decrements EVERY component whose name matches;
 *     all other components are unchanged;
 *   - exactly n + 1 vectors come out: the initial one and one per position.
 * The traversal reads an explicit Order (a bijection on Index(n)) or a
 * direction: L->R is the identity, R->L its reverse. The Flow is never
 * rewritten. Matching is by atom name; the ring rides along as a category.
 * Atoms outside every trifecta (`one`, index 0) go to the auxiliary channel,
 * counted the same way; its interpretation is the configured policy.
 *
 * ACTION maps every component to its canonical residue 0 <= r < N. N = 0 has
 * no canonical residue and is CANOR_ERR_UNDEFINED, never a silent value.
 *
 * REPRESENTATIONS (CanorDB §3.2 "Elementary, Unique, Composite, Unknown").
 * A Soul's stream is Reals (its count values, one per traversal step) over
 * Bases (the IMAG units). The four kinds are the singular / plural choices:
 *
 *     Elementary  Real,  Base    one step, one base          -> 1 value
 *     Unique      Real,  Bases   one step, every base        -> 15 values
 *     Composite   Reals, Base    every step, one base        -> n + 1 values
 *     Unknown     Reals, Bases   every step, every base      -> (n + 1) * 15
 *
 * CATEGORY. Storage: the Soul Symbol stored in a database (CanorDB, or a
 * .fwb written by a tool). Learning: the symbol used by a learning system
 * (neural networks, symbolic rewrite engines, ...). Operation: the symbol used
 * at runtime, in environments and I/O. The category is the USE, so one symbol
 * may carry different categories in different contexts.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANOR_SOUL_FLOW_MAX
#define CANOR_SOUL_FLOW_MAX 32                    /* positions in one Flow */
#endif
#define CANOR_SOUL_TRIFECTAS CANOR_IMAG_TRIFECTAS
#define CANOR_SOUL_COMPONENTS 3                   /* a trifecta is a triple */
#define CANOR_SOUL_BASES (CANOR_IMAG_COUNT - 1)   /* IMAG units 1..15 */
#define CANOR_SOUL_STEPS_MAX (CANOR_SOUL_FLOW_MAX + 1)
#define CANOR_SOUL_ALL UINT16_MAX                 /* "every step" / "every base" */

/* AtomUsage. */
typedef struct {
  canor_imag atom;
  uint32_t ring;
} canor_soul_usage_t;

typedef struct {
  uint16_t n;
  canor_soul_usage_t at[CANOR_SOUL_FLOW_MAX];
} canor_soul_flow_t;

typedef enum { CANOR_SOUL_L2R = 0, CANOR_SOUL_R2L = 1 } canor_soul_direction;

/* v[step][trifecta][component]; step 0 is the initial vector. pos[s] is the
 * Flow position consumed to produce row s + 1. aux[step] is the auxiliary
 * channel's remaining count. */
typedef struct {
  uint16_t n;
  uint16_t pos[CANOR_SOUL_FLOW_MAX];
  uint16_t v[CANOR_SOUL_STEPS_MAX][CANOR_SOUL_TRIFECTAS][CANOR_SOUL_COMPONENTS];
  uint16_t aux[CANOR_SOUL_STEPS_MAX];
} canor_soul_stream_t;

typedef enum {
  CANOR_SOUL_ELEMENTARY = 0,   /* Real,  Base  */
  CANOR_SOUL_UNIQUE = 1,       /* Real,  Bases */
  CANOR_SOUL_COMPOSITE = 2,    /* Reals, Base  */
  CANOR_SOUL_UNKNOWN = 3       /* Reals, Bases */
} canor_soul_kind;
#define CANOR_SOUL_KINDS 4

typedef enum {
  CANOR_SOUL_STORAGE = 0,
  CANOR_SOUL_LEARNING = 1,
  CANOR_SOUL_OPERATION = 2
} canor_soul_category;

/* One representation. value[r * nbases + b]: r runs over the selected steps,
 * b over the selected bases. `step` / `base` name the single one chosen, or
 * are CANOR_SOUL_ALL when the kind is plural on that axis. */
typedef struct {
  canor_soul_kind kind;
  uint16_t step;
  uint16_t base;
  uint16_t nreals, nbases;
  uint16_t value[CANOR_SOUL_STEPS_MAX * CANOR_SOUL_BASES];
} canor_soul_rep_t;

/* A Flow from atoms with ring 0, as an omitted index means. */
canor_status canor_soul_flow_make(const canor_imag *atoms, uint16_t n, canor_soul_flow_t *out);
/* CANOR_ERR_ARGS unless `order` has each of 0..n-1 exactly once. */
canor_status canor_soul_order_check(const uint16_t *order, uint16_t n);
/* The count stream. `order` NULL means: follow `dir`. */
canor_status canor_soul_stream(const canor_soul_flow_t *f, const uint16_t *order,
                               canor_soul_direction dir, canor_soul_stream_t *out);
/* Action: every component (and the auxiliary channel) to its residue mod N. */
canor_status canor_soul_residues(const canor_soul_stream_t *s, uint32_t modulus,
                                 canor_soul_stream_t *out);
/* The count of IMAG unit `base` (1..15) at `step`. */
canor_status canor_soul_value(const canor_soul_stream_t *s, uint16_t step, canor_imag base,
                              uint16_t *out);

bool canor_soul_kind_reals(canor_soul_kind k);   /* plural Reals */
bool canor_soul_kind_bases(canor_soul_kind k);   /* plural Bases */
/* Representations of one Soul whose Flow has length n. */
uint64_t canor_soul_kind_count(canor_soul_kind k, uint16_t n);
/* Representation `idx` of the stream, idx < canor_soul_kind_count(k, n):
 *     Elementary  idx = step * 15 + (base - 1)
 *     Unique      idx = step
 *     Composite   idx = base - 1
 *     Unknown     idx = 0 */
canor_status canor_soul_represent(const canor_soul_stream_t *s, canor_soul_kind k, uint64_t idx,
                                  canor_soul_rep_t *out);

const char *canor_soul_kind_name(canor_soul_kind k);
const char *canor_soul_kind_shape(canor_soul_kind k);        /* "Real, Base" ... */
const char *canor_soul_category_name(canor_soul_category c);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_SOUL_H */
