/* Property tests for the integer-only, fixed-capacity bignum and the modules
 * built on it: canor_big, canor_prime, canor_poly, canor_sym (AGENTS.md
 * invariants 1-2). Expected transcendental values are Q20 (value . 2^20)
 * constants computed offline; the code under test never sees a float. */

#include <string.h>

#include "canor_big.h"
#include "canor_poly.h"
#include "canor_prime.h"
#include "canor_sym.h"
#include "canor_test.h"

#define Q 20
/* Q20 references: floor(v . 2^20) of the exact real value. */
#define REF_E_Q20 INT64_C(2850325)            /* e */
#define REF_E2_Q20 INT64_C(7747986)           /* e^2 */
#define REF_EXPM1_Q20 INT64_C(385749)         /* e^-1 */
#define REF_ML_HALF_M1_Q20 INT64_C(448353)    /* E_{1/2}(-1) = e . erfc(1) */
#define REF_ML_HALF_M2_Q20 INT64_C(267801)    /* E_{1/2}(-2) = e^4 . erfc(2) */

static int64_t iabs64(int64_t v) { return v < 0 ? -v : v; }

static int64_t q20(const canor_big_rat_t *r) {
  int64_t v = 0;
  CANOR_CHECK(canor_big_rat_to_fixed(&r->num, &r->den, Q, &v) == CANOR_OK);
  return v;
}

static void test_format_parse(void) {
  canor_big_t a, b;
  char buf[256];
  const char *txt = "-123456789012345678901234567890123456789";
  CANOR_CHECK(canor_big_parse(&a, txt) == CANOR_OK);
  CANOR_CHECK(canor_big_format(&a, buf, sizeof buf) == (int)strlen(txt));
  CANOR_CHECK(strcmp(buf, txt) == 0);
  CANOR_CHECK(canor_big_parse(&b, "12x") == CANOR_ERR_ARGS);
  canor_big_init(&a);
  CANOR_CHECK(canor_big_format(&a, buf, sizeof buf) == 1 && strcmp(buf, "0") == 0);
}

/* a == q*b + r with |r| < |b| and sign(r) = sign(a), over random wide values. */
static void test_divmod_identity(void) {
  uint64_t seed = 42;
  int it;
  for (it = 0; it < 200; ++it) {
    canor_big_t a, b, q, r, back;
    int i, na = 1 + (int)(canor_test_next(&seed) % 12), nb = 1 + (int)(canor_test_next(&seed) % 6);
    canor_big_init(&a); canor_big_init(&b);
    for (i = 0; i < na; ++i) a.d[i] = (uint32_t)(canor_test_next(&seed) >> 32);
    for (i = 0; i < nb; ++i) b.d[i] = (uint32_t)(canor_test_next(&seed) >> 32) | 1u;
    a.n = na; a.sign = (it & 1) ? -1 : 1;
    b.n = nb; b.sign = (it & 2) ? -1 : 1;
    while (a.n && !a.d[a.n - 1]) --a.n;
    if (!a.n) a.sign = 0;
    CANOR_CHECK(canor_big_divmod(&q, &r, &a, &b) == CANOR_OK);
    CANOR_CHECK(canor_big_cmp_abs(&r, &b) < 0);
    CANOR_CHECK(r.sign == 0 || r.sign == a.sign);
    CANOR_CHECK(canor_big_mul(&back, &q, &b) == CANOR_OK);
    CANOR_CHECK(canor_big_add(&back, &back, &r) == CANOR_OK);
    CANOR_CHECK(canor_big_cmp(&back, &a) == 0);
  }
  {
    canor_big_t a, z;
    canor_big_set_i64(&a, 7);
    canor_big_init(&z);
    CANOR_CHECK(canor_big_divmod(&a, NULL, &a, &z) == CANOR_ERR_UNDEFINED);
  }
}

/* Capacity: the widest value fits, one bit more is CANOR_ERR_FULL, and a
 * refused operation leaves its destination untouched. */
static void test_capacity(void) {
  canor_big_t one, top, r, keep;
  canor_big_set_i64(&one, 1);
  CANOR_CHECK(canor_big_shl(&top, &one, CANOR_BIG_BITS_MAX - 1) == CANOR_OK);
  CANOR_CHECK(canor_big_bits(&top) == CANOR_BIG_BITS_MAX);
  canor_big_set_i64(&r, 99);
  keep = r;
  CANOR_CHECK(canor_big_shl(&r, &top, 1) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_big_cmp(&r, &keep) == 0);
  CANOR_CHECK(canor_big_add(&r, &top, &top) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_big_cmp(&r, &keep) == 0);
  {
    canor_big_t h;
    CANOR_CHECK(canor_big_shl(&h, &one, CANOR_BIG_BITS_MAX / 2) == CANOR_OK);
    CANOR_CHECK(canor_big_mul(&r, &h, &h) == CANOR_ERR_FULL);
    CANOR_CHECK(canor_big_cmp(&r, &keep) == 0);
    canor_big_shr(&h, &h, 1);
    CANOR_CHECK(canor_big_mul(&r, &h, &h) == CANOR_OK);
    CANOR_CHECK(canor_big_bits(&r) == CANOR_BIG_BITS_MAX - 1);
  }
}

static void test_roots(void) {
  canor_big_t a, r, e, one;
  canor_big_set_i64(&one, 1);
  CANOR_CHECK(canor_big_parse(&a, "10000000000000000000000000000000000000000") == CANOR_OK);
  CANOR_CHECK(canor_big_isqrt(&r, &a) == CANOR_OK);
  CANOR_CHECK(canor_big_parse(&e, "100000000000000000000") == CANOR_OK);
  CANOR_CHECK(canor_big_cmp(&r, &e) == 0);
  CANOR_CHECK(canor_big_sub(&a, &a, &one) == CANOR_OK);       /* 10^40 - 1 */
  CANOR_CHECK(canor_big_isqrt(&r, &a) == CANOR_OK);
  CANOR_CHECK(canor_big_sub(&e, &e, &one) == CANOR_OK);
  CANOR_CHECK(canor_big_cmp(&r, &e) == 0);
  CANOR_CHECK(canor_big_shl(&a, &one, 300) == CANOR_OK);
  CANOR_CHECK(canor_big_iroot(&r, &a, 3) == CANOR_OK);
  CANOR_CHECK(canor_big_shl(&e, &one, 100) == CANOR_OK);
  CANOR_CHECK(canor_big_cmp(&r, &e) == 0);
  canor_big_neg(&a);
  CANOR_CHECK(canor_big_isqrt(&r, &a) == CANOR_ERR_ARGS);
}

static void test_rationals(void) {
  canor_big_rat_t a, b, c, h;
  char buf[64];
  CANOR_CHECK(canor_big_rat_set_i64(&a, 1, 3) == CANOR_OK);
  CANOR_CHECK(canor_big_rat_set_i64(&b, 1, 6) == CANOR_OK);
  CANOR_CHECK(canor_big_rat_add(&c, &a, &b) == CANOR_OK);
  CANOR_CHECK(canor_big_rat_set_i64(&h, 1, 2) == CANOR_OK);
  CANOR_CHECK(canor_big_rat_cmp(&c, &h, NULL) == 0);
  CANOR_CHECK(canor_big_rat_set_i64(&a, 6, -4) == CANOR_OK);   /* normalized */
  canor_big_rat_format(&a, buf, sizeof buf);
  CANOR_CHECK(strcmp(buf, "-3/2") == 0);
  CANOR_CHECK(canor_big_rat_parse(&a, "-0.3125") == CANOR_OK);
  canor_big_rat_format(&a, buf, sizeof buf);
  CANOR_CHECK(strcmp(buf, "-5/16") == 0);
  CANOR_CHECK(canor_big_rat_set_i64(&a, 1, 0) == CANOR_ERR_UNDEFINED);
  canor_big_rat_init(&a);
  CANOR_CHECK(canor_big_rat_inv(&b, &a) == CANOR_ERR_UNDEFINED);
}

static void test_transcendentals(void) {
  canor_big_rat_t v;
  canor_exp_report er;
  canor_ml_report mr;
  int sq = 0;
  CANOR_CHECK(canor_exp_rat(canor_rat(1, 1), 24, &v, &er));
  CANOR_CHECK(iabs64(q20(&v) - REF_E_Q20) <= 1);
  CANOR_CHECK(canor_pade_exp(canor_rat(1, 1), 6, 6, &v, &sq));
  CANOR_CHECK(iabs64(q20(&v) - REF_E_Q20) <= 1);
  /* E(1,1; z) = exp(z): the identity is checked, not assumed. */
  CANOR_CHECK(canor_mittag_leffler(canor_rat(1, 1), canor_rat(1, 1), canor_rat(1, 1), 60, &v, &mr));
  CANOR_CHECK(mr.ok && iabs64(q20(&v) - REF_E_Q20) <= 2);
  /* Fractional alpha through the integer 1/Gamma table. */
  CANOR_CHECK(canor_mittag_leffler(canor_rat(1, 2), canor_rat(1, 1), canor_rat(-1, 1), 80, &v, &mr));
  CANOR_CHECK(mr.ok && iabs64(q20(&v) - REF_ML_HALF_M1_Q20) <= 4);
  /* t^(1/2) at t = 4 is exactly 2, so relax(1/2, 1, 4) = E_{1/2}(-2). */
  CANOR_CHECK(canor_fractional_relax(canor_rat(1, 2), canor_rat(1, 1), canor_rat(4, 1), 80, &v, &mr));
  CANOR_CHECK(mr.ok && iabs64(q20(&v) - REF_ML_HALF_M2_Q20) <= 4);
  CANOR_CHECK(canor_fractional_relax(canor_rat(1, 1), canor_rat(1, 1), canor_rat(1, 1), 60, &v, &mr));
  CANOR_CHECK(iabs64(q20(&v) - REF_EXPM1_Q20) <= 2);
  /* Too few terms to converge is refused, not returned as a partial sum. */
  CANOR_CHECK(!canor_mittag_leffler(canor_rat(1, 2), canor_rat(1, 1), canor_rat(-8, 1), 4, &v, &mr));
}

static void test_prime(void) {
  canor_residual r;
  CANOR_CHECK(canor_prime_init(100));
  CANOR_CHECK(canor_prime_count() == 25);
  CANOR_CHECK(canor_is_prime(97) && !canor_is_prime(91));
  CANOR_CHECK(!canor_prime_init(CANOR_PRIME_LIMIT + 1));     /* refused, not clamped */
  CANOR_CHECK(canor_prime_count() == 25);
  CANOR_CHECK(canor_totient(36) == 12 && canor_mobius(30) == -1);

  /* Specific class: ratio -> 2, so L = exp(2 . e^0 . (cos 0 + i sin 0)) = e^2. */
  CANOR_CHECK(canor_residual_L(CANOR_K_SPECIFIC, 6, 0, canor_rat(0, 1), canor_rat(0, 1), &r));
  CANOR_CHECK(r.cls == CANOR_RESID_CONVERGENT && r.deterministic && !r.exact);
  CANOR_CHECK(r.log2_ratio_milli == 1000);
  CANOR_CHECK(iabs64(r.value_re_q20 - REF_E2_Q20) <= 4 && iabs64(r.value_im_q20) <= 1);
  /* Arbitrary diverges; its order is integer and saturates past n = 20. */
  CANOR_CHECK(canor_residual_L(CANOR_K_ARBITRARY, 6, 0, canor_rat(0, 1), canor_rat(0, 1), &r));
  CANOR_CHECK(r.cls == CANOR_RESID_DIVERGENT && r.log2_ratio_milli > 10000000);
  CANOR_CHECK(canor_residual_L(CANOR_K_ARBITRARY, 40, 0, canor_rat(0, 1), canor_rat(0, 1), &r));
  CANOR_CHECK(r.log2_ratio_milli > INT64_MAX / 8);
}

static void test_sym(void) {
  canor_sym e;
  canor_series s;
  int64_t vals[16];
  int x;
  CANOR_CHECK(canor_axis_apply(CANOR_AXIS_LINEAR, canor_rat(3, 2)) == 1500);
  CANOR_CHECK(canor_axis_apply(CANOR_AXIS_SIGNED_LOG, canor_rat(3, 1)) == 2000);
  CANOR_CHECK(canor_axis_apply(CANOR_AXIS_SIGNED_LOG, canor_rat(-3, 1)) == -2000);
  CANOR_CHECK(iabs64(canor_axis_apply(CANOR_AXIS_LOGLOG, canor_rat(3, 1)) - 1585) <= 2);
  /* f(t) = t: bounded growth, linear axis, values t . 1000. */
  canor_sym_init(&e);
  x = canor_sym_var(&e);
  canor_sym_set_root(&e, x);
  CANOR_CHECK(canor_sym_series(&e, 0, 15, 1, 16, vals, &s));
  CANOR_CHECK(s.valid == 16 && s.axis == CANOR_AXIS_LINEAR);
  CANOR_CHECK(vals[0] == 0 && vals[15] == 15000);
  CANOR_CHECK(s.lo_milli == 0 && s.hi_milli == 15000);
  CANOR_CHECK(!canor_sym_series(&e, 0, 1, 1, CANOR_SYM_SAMPLES_MAX + 1, NULL, &s));
}

int main(void) {
  test_format_parse();
  test_divmod_identity();
  test_capacity();
  test_roots();
  test_rationals();
  test_transcendentals();
  test_prime();
  test_sym();
  return CANOR_TEST_DONE("canor_test_big");
}
