/* canor_timing.c - op/sec observables (Canor WD v1 §9 Aether).
 *
 * The readings declared in canor_timing.h are implemented once, over closed
 * integer intervals. Integer mode is the degenerate case lo == hi, so the two
 * compile-time modes cannot drift apart: they share every line below except
 * the conversion into canor_timing_prop_t. */

#include "canor_timing.h"

#include "canor_swar.h"

#define TIMING_U128_LIMBS 2
#define TIMING_PHASE_BIT(p) ((uint8_t)(1u << (p)))
#define TIMING_ALL_PHASES ((uint8_t)((1u << CANOR_TIMING_AXIS) - 1u))

/* --- property conversion ---------------------------------------------------- */

static canor_timing_prop_t timing_make(int64_t lo, int64_t hi) {
#if CANOR_TIMING_INTERVAL
  canor_timing_prop_t p;
  p.lo = lo;
  p.hi = hi;
  return p;
#else
  (void)hi;
  return lo;
#endif
}

int64_t canor_timing_prop_lo(canor_timing_prop_t p) {
#if CANOR_TIMING_INTERVAL
  return p.lo;
#else
  return p;
#endif
}

int64_t canor_timing_prop_hi(canor_timing_prop_t p) {
#if CANOR_TIMING_INTERVAL
  return p.hi;
#else
  return p;
#endif
}

bool canor_timing_interval_mode(void) { return CANOR_TIMING_INTERVAL != 0; }

/* --- one sample ------------------------------------------------------------- */

/* floor and ceil of ops * hz / den, exactly, through a 128-bit product. */
static canor_status timing_div(uint64_t ops, uint64_t hz, uint64_t den, int64_t *fl, int64_t *cl) {
  uint64_t num[TIMING_U128_LIMBS], q[TIMING_U128_LIMBS], rem;
  num[0] = canor_swar_mul64(ops, hz, &num[1]);
  canor_status s = canor_swar_limbs_divmod_u64(q, num, den, TIMING_U128_LIMBS, &rem);
  if (s != CANOR_OK) return s;
  if (q[1] || q[0] > (uint64_t)INT64_MAX) return CANOR_ERR_RANGE;
  if (rem && q[0] == (uint64_t)INT64_MAX) return CANOR_ERR_RANGE;
  *fl = (int64_t)q[0];
  *cl = (int64_t)q[0] + (rem != 0);
  return CANOR_OK;
}

/* The three enclosures as intervals, before the mode decides what to keep. */
static canor_status timing_rate_iv(uint64_t ops, uint64_t ticks, uint64_t hz,
                                   canor_timing_iv_t iv[CANOR_TIMING_AXIS]) {
  int64_t fl, cl;
  canor_status s;
  if (hz == 0) return CANOR_ERR_ARGS;
  if (ticks < CANOR_TIMING_TICKS_MIN) return CANOR_ERR_UNDEFINED;
  if (ticks == UINT64_MAX) return CANOR_ERR_RANGE;   /* t + 1 does not exist */

  s = timing_div(ops, hz, ticks + 1, &fl, &cl);
  if (s != CANOR_OK) return s;
  iv[CANOR_TIMING_LOW].lo = fl;
  iv[CANOR_TIMING_LOW].hi = CANOR_TIMING_INTERVAL ? cl : fl;

  s = timing_div(ops, hz, ticks, &fl, &cl);
  if (s != CANOR_OK) return s;
  iv[CANOR_TIMING_VALUE].lo = fl;
  iv[CANOR_TIMING_VALUE].hi = CANOR_TIMING_INTERVAL ? cl : fl;

  s = timing_div(ops, hz, ticks - 1, &fl, &cl);
  if (s != CANOR_OK) return s;
  /* HIGH rounds up in integer mode so that [LOW, HIGH] still encloses. */
  iv[CANOR_TIMING_HIGH].lo = CANOR_TIMING_INTERVAL ? fl : cl;
  iv[CANOR_TIMING_HIGH].hi = cl;
  return CANOR_OK;
}

canor_status canor_timing_rate(uint64_t ops, uint64_t ticks, uint64_t hz,
                               canor_timing_prop_t out[CANOR_TIMING_AXIS]) {
  canor_timing_iv_t iv[CANOR_TIMING_AXIS];
  if (!out) return CANOR_ERR_ARGS;
  canor_status s = timing_rate_iv(ops, ticks, hz, iv);
  if (s != CANOR_OK) return s;
  for (int p = 0; p < CANOR_TIMING_AXIS; ++p) out[p] = timing_make(iv[p].lo, iv[p].hi);
  return CANOR_OK;
}

/* --- statistics ------------------------------------------------------------- */

static void timing_sort(int64_t *a, size_t n) {
  for (size_t i = 1; i < n; ++i) {
    int64_t v = a[i];
    size_t j = i;
    while (j > 0 && a[j - 1] > v) {
      a[j] = a[j - 1];
      --j;
    }
    a[j] = v;
  }
}

/* MIN / NOM / MAX of a column of intervals. min, median and max are monotone,
 * so applying each to the lo bounds and to the hi bounds separately gives an
 * enclosure of the statistic. The median is the LOWER median. */
static void timing_bands(const canor_timing_iv_t *col, size_t n,
                         canor_timing_prop_t band[CANOR_TIMING_AXIS]) {
  int64_t lo[CANOR_TIMING_SAMPLES_MAX], hi[CANOR_TIMING_SAMPLES_MAX];
  for (size_t k = 0; k < n; ++k) {
    lo[k] = col[k].lo;
    hi[k] = col[k].hi;
  }
  timing_sort(lo, n);
  timing_sort(hi, n);
  band[CANOR_TIMING_MIN] = timing_make(lo[0], hi[0]);
  band[CANOR_TIMING_NOM] = timing_make(lo[(n - 1) / 2], hi[(n - 1) / 2]);
  band[CANOR_TIMING_MAX] = timing_make(lo[n - 1], hi[n - 1]);
}

/* |A - B| for A = [a,b], B = [c,d], all bounds non-negative. A - B is
 * [a-d, b-c]; its absolute value folds at zero. In integer mode a == b and
 * c == d, so this is exactly |a - c|. */
static canor_timing_iv_t timing_absdiff(canor_timing_iv_t x, canor_timing_iv_t y) {
  canor_timing_iv_t d, r;
  d.lo = x.lo - y.hi;
  d.hi = x.hi - y.lo;
  if (d.lo >= 0) return d;
  if (d.hi <= 0) {
    r.lo = -d.hi;
    r.hi = -d.lo;
    return r;
  }
  r.lo = 0;
  r.hi = -d.lo > d.hi ? -d.lo : d.hi;
  return r;
}

canor_status canor_timing_measure(const canor_timing_series_t series[CANOR_TIMING_AXIS],
                                  uint64_t hz, canor_timing_metric_t *out) {
  canor_timing_iv_t rate[CANOR_TIMING_AXIS][CANOR_TIMING_SAMPLES_MAX];
  canor_timing_iv_t col[CANOR_TIMING_SAMPLES_MAX];
  canor_timing_prop_t band[CANOR_TIMING_AXIS];
  canor_timing_metric_t m;

  if (!series || !out || hz == 0) return CANOR_ERR_ARGS;
  for (int ph = 0; ph < CANOR_TIMING_AXIS; ++ph) {
    if (series[ph].n > CANOR_TIMING_SAMPLES_MAX) return CANOR_ERR_FULL;
    if (series[ph].n && !series[ph].s) return CANOR_ERR_ARGS;
  }

  /* Undefined cells stay at a known value rather than stack garbage, but the
   * masks - not the zeros - are what declare them undefined. */
  for (int ph = 0; ph < CANOR_TIMING_AXIS; ++ph)
    for (int b = 0; b < CANOR_TIMING_AXIS; ++b)
      for (int p = 0; p < CANOR_TIMING_AXIS; ++p) {
        m.latency.v[ph][b][p] = timing_make(0, 0);
        m.jitter.v[ph][b][p] = timing_make(0, 0);
      }
  m.latency_defined = 0;
  m.jitter_defined = 0;

  for (int ph = 0; ph < CANOR_TIMING_AXIS; ++ph) {
    size_t n = series[ph].n;
    for (size_t k = 0; k < n; ++k) {
      canor_timing_iv_t iv[CANOR_TIMING_AXIS];
      canor_status s = timing_rate_iv(series[ph].s[k].ops, series[ph].s[k].ticks, hz, iv);
      if (s != CANOR_OK) return s;
      for (int p = 0; p < CANOR_TIMING_AXIS; ++p) rate[p][k] = iv[p];
    }
    if (n >= 1) {
      for (int p = 0; p < CANOR_TIMING_AXIS; ++p) {
        timing_bands(rate[p], n, band);
        for (int b = 0; b < CANOR_TIMING_AXIS; ++b) m.latency.v[ph][b][p] = band[b];
      }
      m.latency_defined |= TIMING_PHASE_BIT(ph);
    }
    if (n >= 2) {
      for (int p = 0; p < CANOR_TIMING_AXIS; ++p) {
        for (size_t k = 1; k < n; ++k) col[k - 1] = timing_absdiff(rate[p][k], rate[p][k - 1]);
        timing_bands(col, n - 1, band);
        for (int b = 0; b < CANOR_TIMING_AXIS; ++b) m.jitter.v[ph][b][p] = band[b];
      }
      m.jitter_defined |= TIMING_PHASE_BIT(ph);
    }
  }

  *out = m;
  return (m.latency_defined == TIMING_ALL_PHASES && m.jitter_defined == TIMING_ALL_PHASES)
             ? CANOR_OK
             : CANOR_ERR_UNDEFINED;
}

canor_status canor_timing_observe(const canor_timing_series_t update[CANOR_TIMING_AXIS],
                                  const canor_timing_series_t error[CANOR_TIMING_AXIS],
                                  uint64_t hz, uint32_t profile_version,
                                  uint32_t settings_version, canor_timing_observable_t *out) {
  canor_timing_observable_t o;
  if (!out) return CANOR_ERR_ARGS;
  canor_status su = canor_timing_measure(update, hz, &o.update_rate);
  if (su != CANOR_OK && su != CANOR_ERR_UNDEFINED) return su;
  canor_status se = canor_timing_measure(error, hz, &o.error_rate);
  if (se != CANOR_OK && se != CANOR_ERR_UNDEFINED) return se;
  o.tick_hz = hz;
  o.profile_version = profile_version;
  o.settings_version = settings_version;
  *out = o;
  return (su == CANOR_OK && se == CANOR_OK) ? CANOR_OK : CANOR_ERR_UNDEFINED;
}

/* --- names ------------------------------------------------------------------ */

const char *canor_timing_phase_name(canor_timing_phase p) {
  static const char *const n[CANOR_TIMING_AXIS] = {"PRE", "PERI", "POST"};
  return (unsigned)p < CANOR_TIMING_AXIS ? n[p] : "?";
}

const char *canor_timing_band_name(canor_timing_band b) {
  static const char *const n[CANOR_TIMING_AXIS] = {"MIN", "NOM", "MAX"};
  return (unsigned)b < CANOR_TIMING_AXIS ? n[b] : "?";
}

const char *canor_timing_prop_name(canor_timing_prop p) {
  static const char *const n[CANOR_TIMING_AXIS] = {"LOW", "VALUE", "HIGH"};
  return (unsigned)p < CANOR_TIMING_AXIS ? n[p] : "?";
}
