/* Property tests for canor_timing. Built twice: once with integer properties
 * (CANOR_TIMING_INTERVAL=0, 27 scalars per field) and once with LO/HI
 * interval properties (=1, 54). The same expectations hold in both modes
 * wherever the exact rate is an integer. */

#include "canor_timing.h"
#include "canor_test.h"

#define HZ 1000u

static int64_t lo(canor_timing_prop_t p) { return canor_timing_prop_lo(p); }
static int64_t hi(canor_timing_prop_t p) { return canor_timing_prop_hi(p); }

static void test_shape(void) {
  CANOR_CHECK(CANOR_TIMING_PROPS_PER_FIELD == 27);
  CANOR_CHECK(CANOR_TIMING_SCALARS_PER_FIELD == (canor_timing_interval_mode() ? 54 : 27));
  CANOR_CHECK(sizeof(canor_field3_t) == CANOR_TIMING_PROPS_PER_FIELD * sizeof(canor_timing_prop_t));
  CANOR_CHECK(sizeof(canor_timing_prop_t) ==
              CANOR_TIMING_SCALARS_PER_PROP * sizeof(int64_t));
}

static void test_rate(void) {
  canor_timing_prop_t r[3];
  /* 1000 ops in 100 ticks at 1 kHz: exactly 10000 ops/sec. */
  CANOR_CHECK(canor_timing_rate(1000, 100, HZ, r) == CANOR_OK);
  CANOR_CHECK(lo(r[CANOR_TIMING_VALUE]) == 10000 && hi(r[CANOR_TIMING_VALUE]) == 10000);
  /* LOW = 1e6/101 = 9900.99..., HIGH = 1e6/99 = 10101.01... */
  CANOR_CHECK(lo(r[CANOR_TIMING_LOW]) == 9900);
  CANOR_CHECK(hi(r[CANOR_TIMING_HIGH]) == 10102);
  if (canor_timing_interval_mode()) {
    CANOR_CHECK(hi(r[CANOR_TIMING_LOW]) == 9901);
    CANOR_CHECK(lo(r[CANOR_TIMING_HIGH]) == 10101);
  } else {
    CANOR_CHECK(lo(r[CANOR_TIMING_HIGH]) == 10102);   /* rounded up: still encloses */
  }
  /* Enclosure order, and every interval closed. */
  CANOR_CHECK(lo(r[CANOR_TIMING_LOW]) <= lo(r[CANOR_TIMING_VALUE]));
  CANOR_CHECK(hi(r[CANOR_TIMING_VALUE]) <= hi(r[CANOR_TIMING_HIGH]));
  for (int p = 0; p < 3; ++p) CANOR_CHECK(lo(r[p]) <= hi(r[p]));

  /* A 128-bit intermediate: 2^40 ops * 2^30 Hz / 2^20 ticks = 2^50 exactly. */
  CANOR_CHECK(canor_timing_rate(UINT64_C(1) << 40, UINT64_C(1) << 20, UINT64_C(1) << 30, r) ==
              CANOR_OK);
  CANOR_CHECK(lo(r[CANOR_TIMING_VALUE]) == (INT64_C(1) << 50));

  /* Diagnosed, never filled. */
  CANOR_CHECK(canor_timing_rate(1, 1, HZ, r) == CANOR_ERR_UNDEFINED);
  CANOR_CHECK(canor_timing_rate(1, 0, HZ, r) == CANOR_ERR_UNDEFINED);
  CANOR_CHECK(canor_timing_rate(1, 10, 0, r) == CANOR_ERR_ARGS);
  CANOR_CHECK(canor_timing_rate(UINT64_MAX, 2, UINT64_MAX, r) == CANOR_ERR_RANGE);
  CANOR_CHECK(canor_timing_rate(1, UINT64_MAX, HZ, r) == CANOR_ERR_RANGE);
  CANOR_CHECK(canor_timing_rate(0, 10, HZ, r) == CANOR_OK && lo(r[CANOR_TIMING_HIGH]) == 0);
}

static void test_measure(void) {
  /* VALUE rates 10000, 5000, 20000. */
  static const canor_timing_sample_t pre[] = {{1000, 100}, {1000, 200}, {1000, 50}};
  static const canor_timing_sample_t peri[] = {{500, 100}};
  canor_timing_series_t s[3] = {{pre, 3}, {peri, 1}, {NULL, 0}};
  canor_timing_metric_t m;

  CANOR_CHECK(canor_timing_measure(s, HZ, &m) == CANOR_ERR_UNDEFINED);
  CANOR_CHECK(m.latency_defined == 0x3);   /* PRE, PERI */
  CANOR_CHECK(m.jitter_defined == 0x1);    /* PRE only: PERI has one sample */

  CANOR_CHECK(lo(m.latency.v[CANOR_TIMING_PRE][CANOR_TIMING_MIN][CANOR_TIMING_VALUE]) == 5000);
  CANOR_CHECK(lo(m.latency.v[CANOR_TIMING_PRE][CANOR_TIMING_NOM][CANOR_TIMING_VALUE]) == 10000);
  CANOR_CHECK(lo(m.latency.v[CANOR_TIMING_PRE][CANOR_TIMING_MAX][CANOR_TIMING_VALUE]) == 20000);
  CANOR_CHECK(lo(m.latency.v[CANOR_TIMING_PERI][CANOR_TIMING_NOM][CANOR_TIMING_VALUE]) == 5000);
  /* Jitter |5000-10000| = 5000, |20000-5000| = 15000; lower median of two. */
  CANOR_CHECK(lo(m.jitter.v[CANOR_TIMING_PRE][CANOR_TIMING_MIN][CANOR_TIMING_VALUE]) == 5000);
  CANOR_CHECK(lo(m.jitter.v[CANOR_TIMING_PRE][CANOR_TIMING_NOM][CANOR_TIMING_VALUE]) == 5000);
  CANOR_CHECK(lo(m.jitter.v[CANOR_TIMING_PRE][CANOR_TIMING_MAX][CANOR_TIMING_VALUE]) == 15000);

  /* MIN <= NOM <= MAX and lo <= hi in every defined cell. */
  int ordered = 1;
  for (int ph = 0; ph < 2; ++ph)
    for (int p = 0; p < 3; ++p) {
      ordered &= lo(m.latency.v[ph][0][p]) <= lo(m.latency.v[ph][1][p]);
      ordered &= lo(m.latency.v[ph][1][p]) <= lo(m.latency.v[ph][2][p]);
      for (int b = 0; b < 3; ++b) ordered &= lo(m.latency.v[ph][b][p]) <= hi(m.latency.v[ph][b][p]);
    }
  CANOR_CHECK(ordered);

  /* Every phase with two samples: fully defined. */
  canor_timing_series_t full[3] = {{pre, 3}, {pre, 2}, {pre, 2}};
  CANOR_CHECK(canor_timing_measure(full, HZ, &m) == CANOR_OK);
  CANOR_CHECK(m.latency_defined == 0x7 && m.jitter_defined == 0x7);

  /* A bad sample anywhere is a hard error, and `out` is left alone. */
  static const canor_timing_sample_t bad[] = {{1, 1}};
  canor_timing_series_t sb[3] = {{pre, 3}, {bad, 1}, {pre, 2}};
  m.latency_defined = 0xAA;
  CANOR_CHECK(canor_timing_measure(sb, HZ, &m) == CANOR_ERR_UNDEFINED && m.latency_defined == 0xAA);

  /* Capacity is checked, not overrun. */
  static canor_timing_sample_t many[CANOR_TIMING_SAMPLES_MAX + 1];
  for (int k = 0; k <= CANOR_TIMING_SAMPLES_MAX; ++k) many[k].ops = 1, many[k].ticks = 10;
  canor_timing_series_t sf[3] = {{many, CANOR_TIMING_SAMPLES_MAX + 1}, {pre, 2}, {pre, 2}};
  CANOR_CHECK(canor_timing_measure(sf, HZ, &m) == CANOR_ERR_FULL);
}

static void test_observe(void) {
  static const canor_timing_sample_t up[] = {{1000, 100}, {1000, 100}};
  static const canor_timing_sample_t er[] = {{3, 100}, {1, 100}};
  canor_timing_series_t u[3] = {{up, 2}, {up, 2}, {up, 2}};
  canor_timing_series_t e[3] = {{er, 2}, {er, 2}, {er, 2}};
  canor_timing_observable_t o;
  CANOR_CHECK(canor_timing_observe(u, e, HZ, 7, 3, &o) == CANOR_OK);
  CANOR_CHECK(o.tick_hz == HZ && o.profile_version == 7 && o.settings_version == 3);
  /* Identical update samples: zero jitter. Error rates 30 and 10: jitter 20. */
  CANOR_CHECK(lo(o.update_rate.jitter.v[CANOR_TIMING_PERI][CANOR_TIMING_MAX][CANOR_TIMING_VALUE]) == 0);
  CANOR_CHECK(lo(o.error_rate.jitter.v[CANOR_TIMING_POST][CANOR_TIMING_NOM][CANOR_TIMING_VALUE]) == 20);
  CANOR_CHECK(lo(o.error_rate.latency.v[CANOR_TIMING_PRE][CANOR_TIMING_MAX][CANOR_TIMING_VALUE]) == 30);
}

static void test_names(void) {
  CANOR_CHECK(canor_timing_phase_name(CANOR_TIMING_PERI)[1] == 'E');
  CANOR_CHECK(canor_timing_band_name(CANOR_TIMING_NOM)[0] == 'N');
  CANOR_CHECK(canor_timing_prop_name(CANOR_TIMING_HIGH)[0] == 'H');
}

int main(void) {
  test_shape();
  test_rate();
  test_measure();
  test_observe();
  test_names();
  return CANOR_TEST_DONE(canor_timing_interval_mode() ? "canor_test_timing[interval]"
                                                      : "canor_test_timing[integer]");
}
