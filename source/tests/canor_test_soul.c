/* Property tests for canor_soul: the §4 count stream (n + 1 rows, decrements,
 * duplicates counted, ring indices categorical, Order validation, auxiliary
 * channel), Action residues, and the four Real(s) x Base(s) representations.
 * The worked example is the /soul skill's: Flow i, j, k, i, L->R, mod 4. */

#include "canor_soul.h"
#include "canor_test.h"

#include <string.h>

enum { I = 1, J = 2, K = 3, L = 4, ONE = 0 };

static int vec_is(const canor_soul_stream_t *s, uint16_t step, int t, int a, int b, int c) {
  return s->v[step][t][0] == a && s->v[step][t][1] == b && s->v[step][t][2] == c;
}

static void test_example(void) {
  static const canor_imag w[] = {I, J, K, I};
  canor_soul_flow_t f, before;
  canor_soul_stream_t s, r;
  CANOR_CHECK(canor_soul_flow_make(w, 4, &f) == CANOR_OK);
  before = f;
  CANOR_CHECK(canor_soul_stream(&f, NULL, CANOR_SOUL_L2R, &s) == CANOR_OK);
  CANOR_CHECK(s.n == 4);
  /* Duplicates count: two i's. */
  CANOR_CHECK(vec_is(&s, 0, 0, 2, 1, 1));
  CANOR_CHECK(vec_is(&s, 1, 0, 1, 1, 1) && s.pos[0] == 0);
  CANOR_CHECK(vec_is(&s, 2, 0, 1, 0, 1));
  CANOR_CHECK(vec_is(&s, 3, 0, 1, 0, 0));
  CANOR_CHECK(vec_is(&s, 4, 0, 0, 0, 0));     /* every atom consumed: final zero */
  CANOR_CHECK(vec_is(&s, 2, 1, 0, 0, 0));     /* other trifectas untouched */
  CANOR_CHECK(memcmp(&f, &before, sizeof f) == 0);   /* the Flow is never rewritten */

  CANOR_CHECK(canor_soul_residues(&s, 4, &r) == CANOR_OK && vec_is(&r, 0, 0, 2, 1, 1));
  CANOR_CHECK(canor_soul_residues(&s, 2, &r) == CANOR_OK && vec_is(&r, 0, 0, 0, 1, 1));
  CANOR_CHECK(canor_soul_residues(&s, 0, &r) == CANOR_ERR_UNDEFINED);
}

static void test_orders(void) {
  static const canor_imag w[] = {I, J, K, I};
  static const uint16_t mid[] = {1, 2, 0, 3};
  static const uint16_t bad[] = {0, 1, 1, 3};
  canor_soul_flow_t f;
  canor_soul_stream_t s;
  canor_soul_flow_make(w, 4, &f);
  CANOR_CHECK(canor_soul_stream(&f, NULL, CANOR_SOUL_R2L, &s) == CANOR_OK);
  CANOR_CHECK(s.pos[0] == 3 && vec_is(&s, 1, 0, 1, 1, 1) && vec_is(&s, 2, 0, 1, 1, 0));
  CANOR_CHECK(canor_soul_stream(&f, mid, CANOR_SOUL_L2R, &s) == CANOR_OK);
  CANOR_CHECK(s.pos[0] == 1 && vec_is(&s, 1, 0, 2, 0, 1) && vec_is(&s, 4, 0, 0, 0, 0));
  CANOR_CHECK(canor_soul_order_check(bad, 4) == CANOR_ERR_ARGS);
  CANOR_CHECK(canor_soul_stream(&f, bad, CANOR_SOUL_L2R, &s) == CANOR_ERR_ARGS);
  CANOR_CHECK(canor_soul_stream(&f, NULL, (canor_soul_direction)7, &s) == CANOR_ERR_ARGS);
}

static void test_rings_and_aux(void) {
  canor_soul_flow_t f;
  canor_soul_stream_t s;
  /* i0, i1, one, l: the ring is a category, so i1 still counts as an i. */
  f.n = 4;
  f.at[0].atom = I;   f.at[0].ring = 0;
  f.at[1].atom = I;   f.at[1].ring = 1;
  f.at[2].atom = ONE; f.at[2].ring = 0;
  f.at[3].atom = L;   f.at[3].ring = 7;
  CANOR_CHECK(canor_soul_stream(&f, NULL, CANOR_SOUL_L2R, &s) == CANOR_OK);
  CANOR_CHECK(vec_is(&s, 0, 0, 2, 0, 0) && vec_is(&s, 0, 1, 1, 0, 0));
  CANOR_CHECK(s.aux[0] == 1 && s.aux[2] == 1 && s.aux[3] == 0);
  CANOR_CHECK(f.at[1].ring == 1 && f.at[3].ring == 7);   /* carried, not consumed */
  /* Capacity. */
  canor_imag many[CANOR_SOUL_FLOW_MAX + 1] = {0};
  CANOR_CHECK(canor_soul_flow_make(many, CANOR_SOUL_FLOW_MAX + 1, &f) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_soul_flow_make(many, CANOR_SOUL_FLOW_MAX, &f) == CANOR_OK);
  many[0] = CANOR_IMAG_COUNT;
  CANOR_CHECK(canor_soul_flow_make(many, 1, &f) == CANOR_ERR_RANGE);
}

static void test_representations(void) {
  static const canor_imag w[] = {I, J, K, I};
  canor_soul_flow_t f;
  canor_soul_stream_t s;
  canor_soul_rep_t r;
  uint16_t v;
  canor_soul_flow_make(w, 4, &f);
  canor_soul_stream(&f, NULL, CANOR_SOUL_L2R, &s);

  CANOR_CHECK(canor_soul_kind_count(CANOR_SOUL_ELEMENTARY, 4) == 5 * 15);
  CANOR_CHECK(canor_soul_kind_count(CANOR_SOUL_UNIQUE, 4) == 5);
  CANOR_CHECK(canor_soul_kind_count(CANOR_SOUL_COMPOSITE, 4) == 15);
  CANOR_CHECK(canor_soul_kind_count(CANOR_SOUL_UNKNOWN, 4) == 1);
  CANOR_CHECK(!canor_soul_kind_reals(CANOR_SOUL_ELEMENTARY) && !canor_soul_kind_bases(CANOR_SOUL_ELEMENTARY));
  CANOR_CHECK(!canor_soul_kind_reals(CANOR_SOUL_UNIQUE) && canor_soul_kind_bases(CANOR_SOUL_UNIQUE));
  CANOR_CHECK(canor_soul_kind_reals(CANOR_SOUL_COMPOSITE) && !canor_soul_kind_bases(CANOR_SOUL_COMPOSITE));
  CANOR_CHECK(canor_soul_kind_reals(CANOR_SOUL_UNKNOWN) && canor_soul_kind_bases(CANOR_SOUL_UNKNOWN));

  /* Elementary (Real, Base): step 0, base i -> 2. */
  CANOR_CHECK(canor_soul_represent(&s, CANOR_SOUL_ELEMENTARY, 0, &r) == CANOR_OK);
  CANOR_CHECK(r.nreals == 1 && r.nbases == 1 && r.step == 0 && r.base == I && r.value[0] == 2);
  /* Unique (Real, Bases): step 2 across every base. */
  CANOR_CHECK(canor_soul_represent(&s, CANOR_SOUL_UNIQUE, 2, &r) == CANOR_OK);
  CANOR_CHECK(r.nreals == 1 && r.nbases == 15 && r.base == CANOR_SOUL_ALL);
  CANOR_CHECK(r.value[0] == 1 && r.value[1] == 0 && r.value[2] == 1 && r.value[3] == 0);
  /* Composite (Reals, Base): base i over every step: 2, 1, 1, 1, 0. */
  CANOR_CHECK(canor_soul_represent(&s, CANOR_SOUL_COMPOSITE, 0, &r) == CANOR_OK);
  CANOR_CHECK(r.nreals == 5 && r.nbases == 1 && r.step == CANOR_SOUL_ALL);
  CANOR_CHECK(r.value[0] == 2 && r.value[1] == 1 && r.value[3] == 1 && r.value[4] == 0);
  /* Unknown (Reals, Bases) agrees with every Elementary. */
  CANOR_CHECK(canor_soul_represent(&s, CANOR_SOUL_UNKNOWN, 0, &r) == CANOR_OK);
  int agree = r.nreals == 5 && r.nbases == 15;
  for (uint16_t st = 0; st <= 4; ++st)
    for (canor_imag b = 1; b <= 15; ++b)
      agree &= canor_soul_value(&s, st, b, &v) == CANOR_OK && v == r.value[st * 15 + (b - 1)];
  CANOR_CHECK(agree);
  CANOR_CHECK(canor_soul_represent(&s, CANOR_SOUL_UNKNOWN, 1, &r) == CANOR_ERR_RANGE);
  CANOR_CHECK(canor_soul_value(&s, 5, I, &v) == CANOR_ERR_RANGE);
  CANOR_CHECK(canor_soul_value(&s, 0, ONE, &v) == CANOR_ERR_RANGE);
  CANOR_CHECK(canor_soul_kind_shape(CANOR_SOUL_COMPOSITE)[0] == 'R');
  CANOR_CHECK(canor_soul_category_name(CANOR_SOUL_LEARNING)[0] == 'L');
}

int main(void) {
  test_example();
  test_orders();
  test_rings_and_aux();
  test_representations();
  return CANOR_TEST_DONE("canor_test_soul");
}
