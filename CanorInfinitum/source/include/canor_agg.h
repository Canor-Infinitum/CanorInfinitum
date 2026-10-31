#ifndef CANOR_AGG_H
#define CANOR_AGG_H

/* Cross-frame aggregation: declared equivalence, and the singularity detector.
 *
 * This is the checkable form of the telemetry model. A "unifying model" that
 * holds everywhere is not testable, so nothing here asserts universality.
 * Instead an aggregation must DECLARE its frame, its resolution, its smoothing
 * radius and which equivalence it used - and the detector then fires, visibly,
 * on the aggregations where the model breaks down. A model that can fail on a
 * given input is worth more than one true by construction.
 *
 * --- CARDINALITY-EQUIVALENCE IS NOT EQUALITY --------------------------------
 *
 * Two units may have "the same number" of a thing without those counts being
 * the same object: different provenance, different frame. This core already
 * enforced that twice without noticing they were one principle - canor_rat_eq
 * refuses to call two poisons equal ("two failed computations agreeing that
 * they failed is not evidence they agree"), and two Golay profiles landing on
 * one codeword are within distance 6, NOT identical.
 *
 * So the relation is part of the claim, not an implementation detail. An
 * aggregate records which relation it established, and canor_agg_same REFUSES to
 * report identity when only cardinality was shown. You cannot get an identity
 * answer out of a cardinality comparison by asking twice.
 *
 * --- THE SINGULARITY DETECTOR ------------------------------------------------
 *
 * Sub-resolution observations are recorded as epsilon, not as zero. Finite
 * accumulation stays infinitesimal - canor_surreal proves it - so an aggregate
 * absorbs any finite number of them without distortion. The breakdown is the
 * crossing: when accumulated sub-resolution mass stops being infinitesimal, it
 * has become countable at a scale where it was never observed, and the
 * aggregate is reporting magnitude it did not measure.
 *
 * That test is exact and needs no threshold: `!canor_surreal_is_infinitesimal(eps)`
 * on a non-zero accumulator. There is nothing to tune, which is the point -
 * a tuned threshold would be a taste, and this is a band boundary.
 *
 * The second trigger is the Golay one, and it is likewise proved rather than
 * chosen: the [23,12,7] code is perfect, so a snap moves at most 3. A required
 * displacement beyond the declared radius means the bounded smoothing no longer
 * bounds anything.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_surreal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Which relation an aggregate established. Ordered by strength: IDENTITY
 * implies CARDINALITY, never the reverse. */
typedef enum {
  CANOR_EQ_NONE = 0,      /* nothing established */
  CANOR_EQ_CARDINALITY,   /* same count; different objects, frames, provenance */
  CANOR_EQ_SNAP,          /* within a proved radius after bounded smoothing */
  CANOR_EQ_IDENTITY       /* the same object */
} canor_equiv;

const char *canor_equiv_name(canor_equiv e);

#define CANOR_AGG_FRAME_MAX 32

typedef struct {
  char frame[CANOR_AGG_FRAME_MAX];  /* the declared CRS / observing frame */
  canor_rational resolution;        /* the smallest countable magnitude here */
  int radius;                    /* smoothing radius; 3 for the Golay snap */
  canor_surreal counted;            /* observations at or above resolution */
  canor_surreal sub;                /* sub-resolution mass, carried as epsilon */
  long n_counted, n_sub;
  long n_surgery;                /* caps glued in */
  canor_rational synthetic;         /* how much of the total is a cap */
  bool declared;
} canor_agg;

/* Every field is required. An aggregation without a declared frame and
 * resolution cannot be checked, so it is refused rather than defaulted. */
bool canor_agg_declare(canor_agg *a, const char *frame, canor_rational resolution,
                    int radius);

/* Observe a magnitude. Anything below the declared resolution is recorded as
 * epsilon - real, recorded, not yet countable at this scale - and never as
 * zero, which would discard it, nor as its own magnitude, which would claim a
 * countability the frame does not have. */
bool canor_agg_observe(canor_agg *a, canor_rational v);

/* True once accumulated sub-resolution mass has left the infinitesimal band.
 * `why` receives a short reason; pass NULL to ignore. */
bool canor_agg_singular(const canor_agg *a, const char **why);

/* Compare two aggregates under a REQUESTED relation. Returns the strongest
 * relation actually established, which may be weaker than the one asked for -
 * asking for IDENTITY does not produce it. */
canor_equiv canor_agg_same(const canor_agg *a, const canor_agg *b, canor_equiv want);

/* The total, and whether reading it is honest. `lossy` is set when a
 * sub-resolution mass was dropped by the projection, or when part of the
 * aggregate is a surgical cap - either way the declared lossy-transformation
 * record is owed. */
canor_rational canor_agg_total(const canor_agg *a, bool *lossy);

/* --- surgery ----------------------------------------------------------------
 *
 * The generalization of what `ricci` does to a pinched spread chart, lifted to
 * any declared aggregate. When the detector fires, the breakdown region is
 * EXCISED and CAPPED with a standard piece, and the cut is recorded.
 *
 * THE CAP IS NOT A MEASUREMENT. It replaces a region that could not be
 * measured at this frame's resolution; a total read afterwards is part observed
 * and part synthetic, and saying so is the whole obligation. `ricci` already
 * reports that 3 of 15 channels end capped and that a reading taken without
 * excluding them reports the cap as evidence - this carries that discipline
 * into every aggregation rather than leaving it in one command.
 *
 * TERMINATION comes from the same ordinal argument. With
 *
 *     Phi = omega . (singular ? 1 : 0) + n_sub
 *
 * a surgery clears the sub-resolution accumulator, so the omega term falls and
 * Phi strictly decreases even though the finite part may not behave. Surgeries
 * therefore cannot accumulate, which is the property Hamilton-Perelman needs
 * and the one an integer potential could not supply without a tuned constant. */
typedef struct {
  long n_excised;                 /* sub-resolution observations cut away */
  canor_rational capped;             /* the standard piece glued in */
  char frame[CANOR_AGG_FRAME_MAX];   /* the frame it was performed in */
  bool synthetic;                 /* always true: a cap is never evidence */
} canor_agg_cap;

/* Performs one surgery if the aggregate is singular. Returns false when none
 * was needed - which is not a failure, and the caller should not retry. */
bool canor_agg_surgery(canor_agg *a, canor_agg_cap *rec);

/* How much of this aggregate is now a cap rather than an observation. */
canor_rational canor_agg_synthetic(const canor_agg *a);
long canor_agg_surgeries(const canor_agg *a);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_AGG_H */
