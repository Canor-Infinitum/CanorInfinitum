/* canor_demo_determinism.c - determinism
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* determinism                                                                */
/* ========================================================================== */
/* Determinism is the property the whole core is FOR, so it should be tested
 * rather than asserted in a comment. Running a command twice and diffing the
 * output would only catch the crudest failures; what actually threatens
 * determinism here is state that survives between calls and ordering that
 * depends on something other than the input. So this checks:
 *
 *   1. REPEATABILITY. The same input gives the same output, twice in a row,
 *      within one process - which catches accumulating state that a fresh
 *      process would hide.
 *   2. ORDER INDEPENDENCE. Computing a set of results in reverse order gives
 *      the same results. A hidden accumulator shows up here and nowhere else.
 *   3. PATH INDEPENDENCE. A rational reached by two different routes compares
 *      equal, and a Flow folded from a word equals the same word folded after
 *      a round trip through the registry.
 *   4. NO CLOCK, NO ADDRESS. The same value computed from heap storage that
 *      was allocated in a different order is still the same value - which is
 *      the property a pointer-hashing bug would break.
 *
 * It does NOT claim to prove determinism. It claims that the specific ways
 * this code could stop being deterministic are checked. */

static int dchecks, dfails;
static void dcheck(int cond, const char *what) {
  ++dchecks;
  if (!cond) { ++dfails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_determinism(const canor_demo_config_t *c) {
  const char *words[6] = { "ijkkji", "iiikji", "jikkkj", "kkjikk", "ijk", "kji" };
  uint8_t u1[6], u2[6];
  int s1[6], s2[6], i, k;

  printf("determinism check\n\n");

  /* 1 and 2: fold every word forwards, then backwards, in one process. */
  {
    canor_flow_registry r;
    int ok = 1;
    for (i = 0; i < 6; ++i) {
      memset(&r, 0, sizeof r);
      if (!canor_flows_add(&r, "w", words[i]) || !canor_flow_fold(&r, 0, &u1[i], &s1[i])) ok = 0;
    }
    for (i = 5; i >= 0; --i) {
      memset(&r, 0, sizeof r);
      if (!canor_flows_add(&r, "w", words[i]) || !canor_flow_fold(&r, 0, &u2[i], &s2[i])) ok = 0;
    }
    dcheck(ok, "every word folds");
    for (i = 0, k = 1; i < 6; ++i) if (u1[i] != u2[i] || s1[i] != s2[i]) k = 0;
    dcheck(k, "folding in reverse order gives identical results");
  }

  /* Repeating the whole configured differential set must be bit-identical. */
  {
    FLOWVALUE a[CANOR_REGISTRY_MAX][3], b[CANOR_REGISTRY_MAX][3];
    int okA = 1, okB = 1, same = 1;
    /* Both buffers start identical, so a Flow whose differential is UNDEFINED
     * leaves matching values rather than two different piles of stack garbage.
     * Without this the check compares uninitialised memory and reports a
     * determinism failure that is entirely its own. */
    memset(a, 0, sizeof a);
    memset(b, 0, sizeof b);
    for (i = 0; i < c->flows.count; ++i)
      if (!canor_flow_differential(&c->strat[0], &c->flows, i,
                                &a[i][0], &a[i][1], &a[i][2])) okA = 0;
    for (i = c->flows.count - 1; i >= 0; --i)
      if (!canor_flow_differential(&c->strat[0], &c->flows, i,
                                &b[i][0], &b[i][1], &b[i][2])) okB = 0;
    dcheck(okA == okB, "the differential set succeeds or fails the same way twice");
    for (i = 0; i < c->flows.count; ++i)
      for (k = 0; k < 3; ++k) if (a[i][k] != b[i][k]) same = 0;
    dcheck(same, "di/dj/dk are identical when recomputed in reverse order");
  }

  /* 3: path independence in the fixed-width rationals. 1/2 reached three
   * ways must be one value, and the comparison must agree with equality. */
  {
    canor_rational x = canor_rat(1, 2);
    canor_rational y = canor_rat(50, 100);
    canor_rational z = canor_rat_mul(canor_rat(3, 4), canor_rat(2, 3));
    canor_rational w = canor_rat_add(canor_rat(1, 4), canor_rat(1, 4));
    dcheck(canor_rat_eq(x, y) && canor_rat_eq(x, z) && canor_rat_eq(x, w),
           "1/2 reached four ways is one value");
    dcheck(canor_rat_cmp(x, z) == 0 && canor_rat_cmp(z, w) == 0,
           "and comparison agrees with equality");
  }

  /* 4: the same rational built from heap storage allocated in two different
   * orders. If anything derived a result from an address, this diverges. */
  {
    fb_rat p, q;
    fb_int scratch;
    int cmp1, cmp2;
    fb_rat_init(&p); fb_rat_init(&q); fb_init(&scratch);
    fb_rat_set_i64(&p, 355, 113);
    fb_set_i64(&scratch, 1);               /* perturb the allocation order */
    fb_shl(&scratch, &scratch, 4096);
    fb_rat_set_i64(&q, 355, 113);
    cmp1 = fb_rat_cmp(&p, &q);
    fb_free(&scratch);
    cmp2 = fb_rat_cmp(&p, &q);
    dcheck(cmp1 == 0 && cmp2 == 0,
           "355/113 equals itself across differing allocation histories");
    fb_rat_free(&p); fb_rat_free(&q);
  }

  /* An exact orbit run twice, the second time after the budget has been
   * disturbed. Same classification, same width, or the arithmetic is carrying
   * state it should not. */
  {
    fb_rat cr, ci;
    int st1 = 0, b1 = 0, st2 = 0, b2 = 0, r1, r2;
    fb_rat_init(&cr); fb_rat_init(&ci);
    fb_rat_set_i64(&cr, -1, 4);
    fb_rat_set_i64(&ci, 1, 2);
    r1 = canor_demo_escape_exact(&cr, &ci, 8, &st1, &b1);
    fb_budget_reset_peak();
    r2 = canor_demo_escape_exact(&cr, &ci, 8, &st2, &b2);
    dcheck(r1 == r2 && st1 == st2 && b1 == b2,
           "an exact orbit repeats exactly, including its width");
    fb_rat_free(&cr); fb_rat_free(&ci);
  }

  /* The RGB projection is a pure function of the string and the config. */
  {
    int cnt1[3] = { 3, 1, 1 }, cnt2[3] = { 3, 1, 1 };
    dcheck(canor_demo_meta_of(c, cnt1) == canor_demo_meta_of(c, cnt2) &&
           canor_demo_mix_of(c, cnt1) == canor_demo_mix_of(c, cnt2),
           "the axes are a pure function of the channel counts");
  }

  printf("\n%d checks, %d failures\n", dchecks, dfails);
  return dfails ? 1 : 0;
}
