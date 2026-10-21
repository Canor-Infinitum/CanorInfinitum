#ifndef CANOR_TIMING_H
#define CANOR_TIMING_H

/* Physical Programming timing library: op/sec observables.
 *
 * Canor WD v1 §9 Aether, SCHEMA.md "Timing fields". Every measurement yields
 * an observable shaped
 *
 *     PRE / PERI / POST  x  MIN / NOM / MAX  x  LOW / VALUE / HIGH
 *
 * held in canor_field3_t. Each property is fixed AT COMPILE TIME to be either
 * an exact integer (27 scalars per field) or a closed LO/HI integer interval
 * (54 scalars per field):
 *
 *     CANOR_TIMING_INTERVAL 0   canor_timing_prop_t is int64_t       (27)
 *     CANOR_TIMING_INTERVAL 1   canor_timing_prop_t is {lo, hi}      (54)
 *
 * Fields go into a LATENCY metric and a JITTER coefficient, and both sit under
 * each of the two parental measurables UPDATE_RATE and ERROR_RATE.
 *
 * DECLARED READINGS (the draft names the axes, not their arithmetic):
 *
 *   unit      Every component is a flowrate in ops/sec (SCHEMA.md). LATENCY is
 *             the distribution of per-sample rates; JITTER is the distribution
 *             of |rate_k - rate_{k-1}| between consecutive samples.
 *   phase     PRE / PERI / POST are three caller-supplied sample series:
 *             before, during and after the measured operation.
 *   band      MIN / NOM / MAX are the minimum, the LOWER median, and the
 *             maximum over a series.
 *   property  LOW / VALUE / HIGH is the tick-quantization enclosure of one
 *             sample. A duration read as t ticks lies in (t-1, t+1) ticks, so
 *                 LOW   = ops * hz / (t + 1)    rounded down
 *                 VALUE = ops * hz / t          rounded down
 *                 HIGH  = ops * hz / (t - 1)    rounded up
 *             and [LOW, HIGH] encloses the true rate. In interval mode each
 *             property is instead [floor, ceil] of its exact rational.
 *
 * Undefined cases are diagnosed, never filled (AGENTS.md invariant 3): t < 2
 * ticks has an unbounded HIGH and is CANOR_ERR_UNDEFINED; an empty series
 * leaves its latency undefined, and fewer than two samples leave its jitter
 * undefined. The `*_defined` bitmasks say which phases hold real numbers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANOR_TIMING_INTERVAL
#define CANOR_TIMING_INTERVAL 0
#endif
#ifndef CANOR_TIMING_SAMPLES_MAX
#define CANOR_TIMING_SAMPLES_MAX 64      /* samples per phase series */
#endif

#define CANOR_TIMING_AXIS 3              /* every axis is a 3-tuple */
#define CANOR_TIMING_PROPS_PER_FIELD (CANOR_TIMING_AXIS * CANOR_TIMING_AXIS * CANOR_TIMING_AXIS)
#define CANOR_TIMING_TICKS_MIN 2         /* below this HIGH is unbounded */
#if CANOR_TIMING_INTERVAL
#define CANOR_TIMING_SCALARS_PER_PROP 2
#else
#define CANOR_TIMING_SCALARS_PER_PROP 1
#endif
#define CANOR_TIMING_SCALARS_PER_FIELD (CANOR_TIMING_PROPS_PER_FIELD * CANOR_TIMING_SCALARS_PER_PROP)

typedef enum { CANOR_TIMING_PRE = 0, CANOR_TIMING_PERI = 1, CANOR_TIMING_POST = 2 } canor_timing_phase;
typedef enum { CANOR_TIMING_MIN = 0, CANOR_TIMING_NOM = 1, CANOR_TIMING_MAX = 2 } canor_timing_band;
typedef enum { CANOR_TIMING_LOW = 0, CANOR_TIMING_VALUE = 1, CANOR_TIMING_HIGH = 2 } canor_timing_prop;

typedef struct { int64_t lo, hi; } canor_timing_iv_t;

#if CANOR_TIMING_INTERVAL
typedef canor_timing_iv_t canor_timing_prop_t;
#else
typedef int64_t canor_timing_prop_t;
#endif

/* v[phase][band][prop]. A struct so a field is assignable as a value. */
typedef struct {
  canor_timing_prop_t v[CANOR_TIMING_AXIS][CANOR_TIMING_AXIS][CANOR_TIMING_AXIS];
} canor_field3_t;

typedef struct {
  canor_field3_t latency;
  canor_field3_t jitter;
  uint8_t latency_defined;   /* bit p set: phase p holds a real latency */
  uint8_t jitter_defined;    /* bit p set: phase p holds a real jitter  */
} canor_timing_metric_t;

/* Provenance travels with the artifact (invariant 8). */
typedef struct {
  canor_timing_metric_t update_rate;
  canor_timing_metric_t error_rate;
  uint64_t tick_hz;
  uint32_t profile_version;
  uint32_t settings_version;
} canor_timing_observable_t;

/* One sample: `ops` operations observed over `ticks` ticks. */
typedef struct { uint64_t ops; uint64_t ticks; } canor_timing_sample_t;
/* A caller-owned series; the library never copies beyond the call. */
typedef struct { const canor_timing_sample_t *s; size_t n; } canor_timing_series_t;

/* Both readings of a property, whichever mode is compiled. In integer mode
 * lo == hi == the value. */
int64_t canor_timing_prop_lo(canor_timing_prop_t p);
int64_t canor_timing_prop_hi(canor_timing_prop_t p);
bool canor_timing_interval_mode(void);

/* LOW / VALUE / HIGH of one sample. hz = 0 is ARGS; ticks below
 * CANOR_TIMING_TICKS_MIN is UNDEFINED; a rate beyond INT64_MAX is RANGE. */
canor_status canor_timing_rate(uint64_t ops, uint64_t ticks, uint64_t hz,
                               canor_timing_prop_t out[CANOR_TIMING_AXIS]);

/* Fills latency and jitter for all three phases from series[PRE..POST].
 * Returns CANOR_OK when every phase is defined, CANOR_ERR_UNDEFINED when some
 * are not (the masks say which; the defined phases are still filled), and any
 * error from canor_timing_rate. A series beyond CANOR_TIMING_SAMPLES_MAX is
 * CANOR_ERR_FULL. */
canor_status canor_timing_measure(const canor_timing_series_t series[CANOR_TIMING_AXIS],
                                  uint64_t hz, canor_timing_metric_t *out);

/* UPDATE_RATE and ERROR_RATE together. A hard error from either is returned
 * at once; UNDEFINED from either is returned after both are filled. */
canor_status canor_timing_observe(const canor_timing_series_t update[CANOR_TIMING_AXIS],
                                  const canor_timing_series_t error[CANOR_TIMING_AXIS],
                                  uint64_t hz, uint32_t profile_version,
                                  uint32_t settings_version, canor_timing_observable_t *out);

const char *canor_timing_phase_name(canor_timing_phase p);
const char *canor_timing_band_name(canor_timing_band b);
const char *canor_timing_prop_name(canor_timing_prop p);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_TIMING_H */
