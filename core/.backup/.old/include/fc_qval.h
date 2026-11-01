#ifndef FC_QVAL_H
#define FC_QVAL_H

/* Quantum values: a tagged triple {lo, val, hi}.
 *
 * fc.h ALREADY HAS THE PAIR: `typedef struct { fc_rational lo, hi; } fc_interval`,
 * used for unit bounds. This is that type extended with a committed value and a
 * kind, not a replacement - fc_interval stays where a bare bound is all that is
 * meant, and fc_qval is used where a reading and its warrant travel together.
 *
 * WHY A TRIPLE AND NOT A PAIR. fc_alg isolates a root into [lo,hi] and then
 * fc_alg_approx has to CHOOSE a representative - it returns the midpoint. That
 * choice was implicit and unrecorded. The triple makes it a commitment: `val`
 * is the reading the producer stands behind, and the bounds are what it can
 * prove. A pair leaves "which value do I report?" with no canonical answer, and
 * every consumer then invents one.
 *
 * It also splits along the phases: the BOUND is what PRE constrains, `val` is
 * what PERI commits.
 *
 * VAL IS SURREAL, BOUNDS ARE RATIONAL. Sub-resolution mass lives in `val` as
 * epsilon while the bounds stay countable, which is exactly the band split the
 * telemetry model asks for. One consequence is worth stating rather than
 * discovering: if `val` carries epsilon and `hi` is tight, the invariant below
 * FAILS - and it should. It is saying the declared bound is too tight to hold
 * the sub-resolution mass it was given, which is a real defect in the bound and
 * not a rounding nuisance.
 *
 * --- THE KIND IS NOT DECORATION ---------------------------------------------
 *
 * An exact interval and a statistical interval have the same SHAPE and make
 * different CLAIMS:
 *
 *   EXACT        root isolation, algebraic bounds - the value is in here,
 *                certainly.
 *   STATISTICAL  sampling, thermodynamic, p-bit - in here with confidence c.
 *
 * Reporting the second as the first is the cardinality-versus-identity error in
 * new clothing, so the kind propagates the same way fc_equiv refuses to
 * promote and the non-holonomic class propagates upward:
 *
 *     EXACT       op EXACT        =  EXACT
 *     anything    op STATISTICAL  =  STATISTICAL
 *
 * Contagious upward, never promoted. A combination of a measured quantity with
 * a proved one is measured, and asking for EXACT does not make it so.
 *
 * WHERE THIS SITS IN THE DETERMINISM STORY. p-bit noise is physical and
 * thermal, so a SAMPLE can never be stamped as a value. What is stampable is
 * the distribution and the noise source's provenance. The interval is what
 * lets a stochastic result enter a deterministic framework without lying: the
 * sample is not exact, but the interval is a true statement about it - provided
 * it is labelled STATISTICAL, which is what the kind is for.
 */

#include <stdbool.h>
#include <stdint.h>

#include "fc.h"
#include "fc_surreal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  FC_QV_EXACT = 0,    /* proved bounds: isolation, algebraic */
  FC_QV_STATISTICAL   /* sampled bounds: confidence, not certainty */
} fc_qv_kind;

const char *fc_qv_kind_name(fc_qv_kind k);

typedef struct {
  fc_rational lo, hi;       /* the bound the producer can prove */
  fc_surreal  val;          /* the reading it commits to; may carry epsilon */
  fc_qv_kind  kind;
  int32_t     confidence;   /* per-mille; meaningful only when STATISTICAL */
  bool        valid;
} fc_qval;

/* Construction CHECKS lo <= val <= hi. A triple whose val escapes its bound is
 * a plausible wrong number, which is the failure class poison exists to
 * prevent - so it is refused rather than clamped. */
bool fc_qv_exact(fc_qval *o, fc_rational lo, fc_surreal val, fc_rational hi);
bool fc_qv_stat(fc_qval *o, fc_rational lo, fc_surreal val, fc_rational hi,
                int32_t confidence_permille);
/* A degenerate exact interval: lo = val = hi. */
bool fc_qv_point(fc_qval *o, fc_rational v);

bool fc_qv_valid(const fc_qval *x);
/* True when val lies within [lo,hi]. Re-checkable after any manipulation. */
bool fc_qv_contains_val(const fc_qval *x);
/* The width hi - lo, which is the resolution this interval actually carries. */
fc_rational fc_qv_width(const fc_qval *x);

/* --- arithmetic -------------------------------------------------------------
 * Bounds follow interval arithmetic; `val` follows the same operation on the
 * committed readings, so the triple stays coherent rather than the midpoint
 * drifting away from what the bounds allow. The kind is contagious. */
bool fc_qv_add(const fc_qval *a, const fc_qval *b, fc_qval *o);
bool fc_qv_sub(const fc_qval *a, const fc_qval *b, fc_qval *o);
bool fc_qv_mul(const fc_qval *a, const fc_qval *b, fc_qval *o);

/* The kind an operation over these two yields. Exposed so a caller can ask
 * before committing, and so the rule is testable on its own. */
fc_qv_kind fc_qv_join(fc_qv_kind a, fc_qv_kind b);

void fc_qv_format(const fc_qval *x, char *buf, int cap);

#ifdef __cplusplus
}
#endif
#endif /* FC_QVAL_H */
