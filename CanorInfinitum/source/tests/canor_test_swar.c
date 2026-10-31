/* Property tests for canor_swar: every narrow lane operation is checked lane by
 * lane against scalar arithmetic, and every wide width against identities
 * (q * d + r = a, factoradic round trip, signed overflow at the boundary). */

#include "canor_swar.h"
#include "canor_test.h"

#include <string.h>

#define TEST_VECTORS 2000

static uint64_t lane_mask(unsigned w) { return w >= 64 ? UINT64_MAX : ((UINT64_C(1) << w) - 1); }

static int64_t lane_signed(uint64_t v, unsigned w) {
  if (w >= 64) return (int64_t)v;
  uint64_t sign = UINT64_C(1) << (w - 1);
  return (v & sign) ? (int64_t)v - (int64_t)(UINT64_C(1) << w) : (int64_t)v;
}

/* Instantiated once per narrow width, so the macros are exercised with W as
 * a true compile-time constant, then cross-checked against the runtime path. */
#define TEST_NARROW(W)                                                                     \
  static void test_narrow_##W(uint64_t *rng) {                                             \
    CANOR_CHECK(CANOR_SWAR_LANES(W) * (W) == 64);                                          \
    CANOR_CHECK(CANOR_SWAR_GET(W, CANOR_SWAR_BROADCAST(W, 1), 0) == 1);                    \
    for (int v = 0; v < TEST_VECTORS; ++v) {                                               \
      uint64_t x = canor_test_next(rng), y = canor_test_next(rng);                         \
      if (v % 7 == 0) y = x; /* equal lanes exercise the tie-breaks */                     \
      uint64_t add = CANOR_SWAR_ADD(W, x, y), sub = CANOR_SWAR_SUB(W, x, y);               \
      uint64_t cy = CANOR_SWAR_CARRY(W, x, y), bw = CANOR_SWAR_BORROW(W, x, y);            \
      uint64_t mn = CANOR_SWAR_MIN(W, x, y), mx = CANOR_SWAR_MAX(W, x, y);                 \
      uint64_t mns = CANOR_SWAR_MINS(W, x, y), mxs = CANOR_SWAR_MAXS(W, x, y);             \
      for (unsigned i = 0; i < CANOR_SWAR_LANES(W); ++i) {                                 \
        uint64_t a = CANOR_SWAR_GET(W, x, i), b = CANOR_SWAR_GET(W, y, i);                 \
        uint64_t m = lane_mask(W);                                                         \
        uint64_t top = UINT64_C(1) << ((W) - 1);                                           \
        CANOR_CHECK(CANOR_SWAR_GET(W, add, i) == ((a + b) & m));                           \
        CANOR_CHECK(CANOR_SWAR_GET(W, sub, i) == ((a - b) & m));                           \
        CANOR_CHECK(((CANOR_SWAR_GET(W, cy, i) & top) != 0) == (((a + b) & m) < a));       \
        CANOR_CHECK(((CANOR_SWAR_GET(W, bw, i) & top) != 0) == (a < b));                   \
        CANOR_CHECK(CANOR_SWAR_GET(W, mn, i) == (a < b ? a : b));                          \
        CANOR_CHECK(CANOR_SWAR_GET(W, mx, i) == (a < b ? b : a));                          \
        int64_t sa = lane_signed(a, W), sb = lane_signed(b, W);                            \
        CANOR_CHECK(CANOR_SWAR_GET(W, mns, i) == (sa < sb ? a : b));                       \
        CANOR_CHECK(CANOR_SWAR_GET(W, mxs, i) == (sa < sb ? b : a));                       \
      }                                                                                    \
      uint64_t r;                                                                          \
      CANOR_CHECK(canor_swar_lanes_apply(W, CANOR_SWAR_OP_ADD, x, y, &r) == CANOR_OK &&    \
                  r == add);                                                               \
      CANOR_CHECK(canor_swar_lanes_apply(W, CANOR_SWAR_OP_MINS, x, y, &r) == CANOR_OK &&   \
                  r == mns);                                                               \
      CANOR_CHECK(CANOR_SWAR_GET(W, CANOR_SWAR_SET(W, x, 0, 1), 0) == 1);                  \
    }                                                                                      \
    /* Intervals: [x, x + 1] per lane overflows exactly when a hi lane carries. */          \
    {                                                                                      \
      canor_swar_lanes_t a, b, o;                                                          \
      a.lo = 0;                                                                            \
      a.hi = CANOR_SWAR_BROADCAST(W, 1);                                                   \
      b = a;                                                                               \
      CANOR_CHECK(canor_swar_iv##W##_add(a, b, &o) == CANOR_OK);                           \
      CANOR_CHECK(CANOR_SWAR_GET(W, o.hi, 0) == 2u);                     \
      a.hi = CANOR_SWAR_BROADCAST(W, lane_mask(W));                                        \
      CANOR_CHECK(canor_swar_iv##W##_add(a, a, &o) == CANOR_ERR_RANGE);                    \
      b.lo = CANOR_SWAR_BROADCAST(W, 1);                                                   \
      b.hi = b.lo;                                                                         \
      CANOR_CHECK(canor_swar_iv##W##_sub(a, b, &o) == CANOR_ERR_RANGE); /* 0 - 1 */        \
      CANOR_CHECK(canor_swar_iv##W##_sub(b, b, &o) == CANOR_OK && o.lo == 0 && o.hi == 0); \
      a.lo = CANOR_SWAR_BROADCAST(W, 1);                                                   \
      a.hi = 0;                                                                            \
      CANOR_CHECK(!canor_swar_iv##W##_valid(a));                                           \
      CANOR_CHECK(canor_swar_iv##W##_add(a, b, &o) == CANOR_ERR_ARGS);                     \
    }                                                                                      \
  }

TEST_NARROW(2)
TEST_NARROW(4)
TEST_NARROW(8)
TEST_NARROW(16)
TEST_NARROW(32)
TEST_NARROW(64)

static void test_mul64(uint64_t *rng) {
  uint64_t hi, lo = canor_swar_mul64(UINT64_MAX, UINT64_MAX, &hi);
  CANOR_CHECK(lo == 1 && hi == UINT64_MAX - 1);   /* (2^64-1)^2 = 2^128 - 2^65 + 1 */
  for (int v = 0; v < TEST_VECTORS; ++v) {
    /* Check against 32-bit operands, whose product fits one word exactly. */
    uint64_t a = canor_test_next(rng) >> 32, b = canor_test_next(rng) >> 32;
    lo = canor_swar_mul64(a, b, &hi);
    CANOR_CHECK(hi == 0 && lo == a * b);
  }
}

#define TEST_WIDE(W)                                                                        \
  static void test_wide_##W(uint64_t *rng) {                                                \
    canor_swar_u##W##_t a, b, c, q, one, zero, max, min;                                    \
    uint64_t r, u;                                                                          \
    const size_t n = CANOR_SWAR_LIMBS(W);                                                   \
    canor_swar_u##W##_from_u64(&one, 1);                                                    \
    canor_swar_u##W##_from_u64(&zero, 0);                                                   \
    memset(&max, 0xFF, sizeof max);                                                         \
    max.w[n - 1] >>= 1;                       /* signed maximum */                          \
    memset(&min, 0, sizeof min);                                                            \
    min.w[n - 1] = UINT64_C(1) << 63;         /* signed minimum */                          \
    /* Unsigned wrap: all-ones + 1 = 0 with a carry out. */                                 \
    memset(&a, 0xFF, sizeof a);                                                             \
    CANOR_CHECK(canor_swar_u##W##_add(&c, &a, &one) == 1);                                  \
    CANOR_CHECK(canor_swar_u##W##_cmp(&c, &zero) == 0);                                     \
    CANOR_CHECK(canor_swar_u##W##_sub(&c, &zero, &one) == 1);                               \
    CANOR_CHECK(canor_swar_u##W##_cmp(&c, &a) == 0);                                        \
    /* Signed boundaries are diagnosed, not wrapped. */                                     \
    CANOR_CHECK(canor_swar_limbs_add_s(c.w, max.w, one.w, n) == CANOR_ERR_RANGE);           \
    CANOR_CHECK(canor_swar_limbs_sub_s(c.w, min.w, one.w, n) == CANOR_ERR_RANGE);           \
    CANOR_CHECK(canor_swar_limbs_sub_s(c.w, zero.w, one.w, n) == CANOR_OK);                 \
    CANOR_CHECK(canor_swar_u##W##_cmp_s(&c, &zero) < 0 && canor_swar_u##W##_cmp(&c, &zero) > 0); \
    canor_swar_u##W##_from_i64(&b, -1);                                                     \
    CANOR_CHECK(canor_swar_u##W##_cmp(&b, &c) == 0);                                        \
    CANOR_CHECK(canor_swar_u##W##_to_u64(&b, &u) == CANOR_ERR_RANGE);                       \
    /* Division identity a = q * d + r with r < d. */                                       \
    for (int v = 0; v < 200; ++v) {                                                         \
      uint64_t d = canor_test_next(rng) >> (v % 60), hi;                                    \
      if (d == 0) d = 1;                                                                    \
      for (size_t k = 0; k < n; ++k) a.w[k] = canor_test_next(rng);                         \
      CANOR_CHECK(canor_swar_u##W##_divmod_u64(&q, &a, d, &r) == CANOR_OK && r < d);        \
      hi = canor_swar_u##W##_mul_u64(&c, &q, d);                                            \
      CANOR_CHECK(hi == 0);                                                                 \
      canor_swar_u##W##_from_u64(&b, r);                                                    \
      CANOR_CHECK(canor_swar_u##W##_add(&c, &c, &b) == 0);                                  \
      CANOR_CHECK(canor_swar_u##W##_cmp(&c, &a) == 0);                                      \
    }                                                                                       \
    CANOR_CHECK(canor_swar_u##W##_divmod_u64(&q, &a, 0, &r) == CANOR_ERR_UNDEFINED);        \
    /* Factoradic round trip, including the all-ones value. */                              \
    {                                                                                       \
      static uint16_t dig[CANOR_SWAR_FACTORADIC_MAX];                                       \
      size_t cnt;                                                                           \
      for (int v = 0; v < 20; ++v) {                                                        \
        if (v == 0) memset(&a, 0xFF, sizeof a);                                             \
        else for (size_t k = 0; k < n; ++k) a.w[k] = canor_test_next(rng);                  \
        CANOR_CHECK(canor_swar_u##W##_to_factoradic(&a, dig, CANOR_SWAR_FACTORADIC_MAX,     \
                                                    &cnt) == CANOR_OK);                     \
        int digits_ok = dig[0] == 0;                                                        \
        for (size_t k = 0; k < cnt; ++k) digits_ok &= dig[k] <= k;                          \
        CANOR_CHECK(digits_ok);                                                             \
        CANOR_CHECK(canor_swar_u##W##_from_factoradic(&b, dig, cnt) == CANOR_OK);           \
        CANOR_CHECK(canor_swar_u##W##_cmp(&a, &b) == 0);                                    \
      }                                                                                     \
      CANOR_CHECK(canor_swar_u##W##_to_factoradic(&a, dig, 3, &cnt) == CANOR_ERR_FULL);     \
    }                                                                                       \
    /* Signed intervals. */                                                                 \
    {                                                                                       \
      canor_swar_iv##W##_t x, y, z;                                                         \
      int64_t lo, hi;                                                                       \
      canor_swar_u##W##_from_i64(&x.lo, 1);                                                 \
      canor_swar_u##W##_from_i64(&x.hi, 2);                                                 \
      canor_swar_u##W##_from_i64(&y.lo, 3);                                                 \
      canor_swar_u##W##_from_i64(&y.hi, 4);                                                 \
      CANOR_CHECK(canor_swar_iv##W##_add(&z, &x, &y) == CANOR_OK);                          \
      CANOR_CHECK(canor_swar_limbs_to_i64(z.lo.w, n, &lo) == CANOR_OK && lo == 4);          \
      CANOR_CHECK(canor_swar_limbs_to_i64(z.hi.w, n, &hi) == CANOR_OK && hi == 6);          \
      CANOR_CHECK(canor_swar_iv##W##_sub(&z, &x, &y) == CANOR_OK);                          \
      CANOR_CHECK(canor_swar_limbs_to_i64(z.lo.w, n, &lo) == CANOR_OK && lo == -3);         \
      CANOR_CHECK(canor_swar_limbs_to_i64(z.hi.w, n, &hi) == CANOR_OK && hi == -1);         \
      CANOR_CHECK(canor_swar_iv##W##_sub(&y, &x, &y) == CANOR_OK); /* aliasing */           \
      CANOR_CHECK(canor_swar_limbs_to_i64(y.lo.w, n, &lo) == CANOR_OK && lo == -3);         \
      CANOR_CHECK(canor_swar_limbs_to_i64(y.hi.w, n, &hi) == CANOR_OK && hi == -1);         \
      z.lo = x.hi;                                                                          \
      z.hi = x.lo;                                                                          \
      CANOR_CHECK(!canor_swar_iv##W##_valid(&z));                                           \
      CANOR_CHECK(canor_swar_iv##W##_add(&z, &z, &x) == CANOR_ERR_ARGS);                    \
    }                                                                                       \
  }

TEST_WIDE(128)
TEST_WIDE(256)
TEST_WIDE(512)
TEST_WIDE(1024)
TEST_WIDE(2048)
TEST_WIDE(4096)

static void test_factoradic_known(void) {
  canor_swar_u128_t a, b;
  uint16_t dig[8];
  size_t cnt;
  /* 23 = 3*3! + 2*2! + 1*1!, the last permutation of four letters. */
  canor_swar_u128_from_u64(&a, 23);
  CANOR_CHECK(canor_swar_u128_to_factoradic(&a, dig, 8, &cnt) == CANOR_OK);
  CANOR_CHECK(cnt == 4 && dig[0] == 0 && dig[1] == 1 && dig[2] == 2 && dig[3] == 3);
  /* 24 = 1*4!. */
  canor_swar_u128_from_u64(&a, 24);
  CANOR_CHECK(canor_swar_u128_to_factoradic(&a, dig, 8, &cnt) == CANOR_OK);
  CANOR_CHECK(cnt == 5 && dig[4] == 1 && dig[3] == 0 && dig[1] == 0);
  canor_swar_u128_from_u64(&a, 0);
  CANOR_CHECK(canor_swar_u128_to_factoradic(&a, dig, 8, &cnt) == CANOR_OK && cnt == 1);
  dig[0] = 0; dig[1] = 2;                       /* digit 1 may not exceed 1 */
  CANOR_CHECK(canor_swar_u128_from_factoradic(&b, dig, 2) == CANOR_ERR_RANGE);
}

static void test_runtime_width(void) {
  uint64_t r;
  CANOR_CHECK(canor_swar_lanes_apply(3, CANOR_SWAR_OP_ADD, 1, 1, &r) == CANOR_ERR_ARGS);
  CANOR_CHECK(canor_swar_lanes_apply(128, CANOR_SWAR_OP_ADD, 1, 1, &r) == CANOR_ERR_ARGS);
  CANOR_CHECK(canor_swar_lanes_apply(8, CANOR_SWAR_OP_ADD, 1, 1, NULL) == CANOR_ERR_ARGS);
  CANOR_CHECK(CANOR_SWAR_IS_NARROW(2) && !CANOR_SWAR_IS_NARROW(128));
  CANOR_CHECK(CANOR_SWAR_IS_WIDE(4096) && !CANOR_SWAR_IS_WIDE(64));
  /* The hypervector width is one wide lane. */
  CANOR_CHECK(sizeof(canor_swar_u1024_t) * 8 == 1024);
}

int main(void) {
  uint64_t rng = 0x43414E4F52u;   /* "CANOR" */
  test_narrow_2(&rng);
  test_narrow_4(&rng);
  test_narrow_8(&rng);
  test_narrow_16(&rng);
  test_narrow_32(&rng);
  test_narrow_64(&rng);
  test_mul64(&rng);
  test_wide_128(&rng);
  test_wide_256(&rng);
  test_wide_512(&rng);
  test_wide_1024(&rng);
  test_wide_2048(&rng);
  test_wide_4096(&rng);
  test_factoradic_known();
  test_runtime_width();
  return CANOR_TEST_DONE("canor_test_swar");
}
