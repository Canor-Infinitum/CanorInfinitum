/* Property tests for canor_enum: counts, rank/unrank bijections, the Ore
 * Strategies (exact, period 6), the complex-fractal invocations (reals + gaps
 * = support at every depth), Soul count streams and representations, the
 * Action space exhaustively, and Maneuver residues. */

#include "canor_enum.h"
#include "canor_test.h"

#include <string.h>

#define SOUL_STRIDE 997u          /* prime, so the sample crosses every radix */
#define REP_STRIDE 7919u

static int q_is(canor_rational q, int64_t num, int64_t den) { return q.num == num && q.den == den; }

static void test_imag(void) {
  static const char *const names = "ijklmnopqrstefg";
  canor_imag u;
  int s;
  CANOR_CHECK(canor_imag_init() == CANOR_OK);
  for (int k = 1; k < CANOR_IMAG_COUNT; ++k)
    CANOR_CHECK(canor_imag_name((canor_imag)k)[0] == names[k - 1] &&
                canor_imag_name((canor_imag)k)[1] == '\0');
  CANOR_CHECK(canor_imag_mul(1, 2, &u, &s) == CANOR_OK && u == 3 && s == 1);    /* i*j = k  */
  CANOR_CHECK(canor_imag_mul(2, 1, &u, &s) == CANOR_OK && u == 3 && s == -1);   /* j*i = -k */
}

static void test_counts(void) {
  CANOR_CHECK(canor_enum_count(CANOR_ENUM_STRATEGY) == 90);
  CANOR_CHECK(canor_enum_count(CANOR_ENUM_FLOW) == 7776);
  CANOR_CHECK(canor_enum_count(CANOR_ENUM_METAFLOW) == 24);
  CANOR_CHECK(canor_enum_count(CANOR_ENUM_INVOCATION) == 2160);
  CANOR_CHECK(canor_enum_count(CANOR_ENUM_SOUL) == UINT64_C(16796160));
  CANOR_CHECK(canor_enum_count(CANOR_ENUM_ACTION) == UINT64_C(24389000));
  CANOR_CHECK(canor_enum_count(CANOR_ENUM_MANEUVER) == UINT64_C(409641546240000));
  /* Per element: Elementary 16 x 15, Unique 16, Composite 15, Unknown 1. */
  CANOR_CHECK(canor_enum_rep_count(CANOR_ENUM_SOUL, CANOR_SOUL_ELEMENTARY) == UINT64_C(16796160) * 240);
  CANOR_CHECK(canor_enum_rep_count(CANOR_ENUM_SOUL, CANOR_SOUL_UNIQUE) == UINT64_C(16796160) * 16);
  CANOR_CHECK(canor_enum_rep_count(CANOR_ENUM_SOUL, CANOR_SOUL_COMPOSITE) == UINT64_C(16796160) * 15);
  CANOR_CHECK(canor_enum_rep_count(CANOR_ENUM_SOUL, CANOR_SOUL_UNKNOWN) == UINT64_C(16796160));
  CANOR_CHECK(canor_enum_rep_count(CANOR_ENUM_MANEUVER, CANOR_SOUL_ELEMENTARY) ==
              UINT64_C(409641546240000) * 240);
  CANOR_CHECK(canor_enum_rep_count(CANOR_ENUM_FLOW, CANOR_SOUL_UNKNOWN) == 0);
}

static void test_strategy(void) {
  /* d^k under tau = sigma = 1: C = 1/2, -1/2, -1, -1/2, 1/2, 1 and
   * S = 1, 1, 0, -1, -1, 0 - one full period over Orders 1..6. */
  static const int64_t cn[6] = {1, -1, -1, -1, 1, 1}, cd[6] = {2, 2, 1, 2, 2, 1};
  static const int64_t sn[6] = {1, 1, 0, -1, -1, 0};
  canor_enum_strategy_t s;
  canor_enum_ore_t o;
  char buf[CANOR_ENUM_SYMBOL_MAX];
  uint64_t r;
  int ore_ok = 1;
  for (uint64_t k = 0; k < CANOR_ENUM_STRATEGY_COUNT; ++k) {
    CANOR_CHECK(canor_enum_strategy_unrank(k, &s) == CANOR_OK);
    CANOR_CHECK(canor_enum_strategy_rank(&s, &r) == CANOR_OK && r == k);
    ore_ok &= canor_enum_strategy_ore(&s, &o) == CANOR_OK;
    ore_ok &= q_is(o.c, cn[s.order - 1], cd[s.order - 1]) && q_is(o.s, sn[s.order - 1], 1);
    /* T = S / C, defined because C is never 0 on this period. */
    ore_ok &= o.t_status == CANOR_OK && o.t.num * o.c.num * o.s.den == o.s.num * o.c.den * o.t.den;
  }
  CANOR_CHECK(ore_ok);
  CANOR_CHECK(canor_enum_strategy_unrank(0, &s) == CANOR_OK);
  CANOR_CHECK(canor_enum_strategy_format(&s, buf, sizeof buf) == CANOR_OK && !strcmp(buf, "di^1"));
  CANOR_CHECK(canor_enum_strategy_unrank(89, &s) == CANOR_OK);
  CANOR_CHECK(canor_enum_strategy_format(&s, buf, sizeof buf) == CANOR_OK && !strcmp(buf, "dg^6"));
  CANOR_CHECK(canor_enum_strategy_unrank(90, &s) == CANOR_ERR_RANGE);
  s.order = 7;
  CANOR_CHECK(canor_enum_strategy_ore(&s, &o) == CANOR_ERR_RANGE);
}

static void test_flow(void) {
  canor_enum_flow_t f;
  canor_soul_stream_t st;
  char buf[CANOR_ENUM_SYMBOL_MAX];
  uint64_t r;
  int ok = 1;
  for (uint64_t k = 0; k < CANOR_ENUM_FLOW_COUNT; ++k) {
    ok &= canor_enum_flow_unrank(k, &f) == CANOR_OK;
    ok &= canor_enum_flow_rank(&f.flow, &r) == CANOR_OK && r == k;
    /* Each of these Flows is a permutation, so every initial count is 1. */
    ok &= canor_soul_stream(&f.flow, NULL, CANOR_SOUL_L2R, &st) == CANOR_OK && st.n == 15;
    for (int t = 0; t < 5; ++t)
      ok &= st.v[0][t][0] == 1 && st.v[0][t][1] == 1 && st.v[0][t][2] == 1 && st.v[15][t][0] == 0;
  }
  CANOR_CHECK(ok);
  CANOR_CHECK(canor_enum_flow_unrank(0, &f) == CANOR_OK);
  CANOR_CHECK(canor_enum_flow_format(&f.flow, buf, sizeof buf) == CANOR_OK &&
              !strcmp(buf, "ijk|lmn|opq|rst|efg"));
  CANOR_CHECK(canor_enum_flow_unrank(CANOR_ENUM_FLOW_COUNT - 1, &f) == CANOR_OK);
  CANOR_CHECK(canor_enum_flow_format(&f.flow, buf, sizeof buf) == CANOR_OK &&
              !strcmp(buf, "kji|nml|qpo|tsr|gfe"));
  /* Values outside the product space are reported, not coerced. */
  canor_enum_flow_unrank(0, &f);
  f.flow.at[1].atom = 1;                                     /* i, i, k: a repeat */
  CANOR_CHECK(canor_enum_flow_rank(&f.flow, &r) == CANOR_ERR_NOT_FOUND);
  canor_enum_flow_unrank(0, &f);
  f.flow.at[0].ring = 1;                                     /* i1 is another instance */
  CANOR_CHECK(canor_enum_flow_rank(&f.flow, &r) == CANOR_ERR_NOT_FOUND);
  CANOR_CHECK(canor_enum_flow_format(&f.flow, buf, sizeof buf) == CANOR_OK && !strncmp(buf, "i1jk", 4));
  CANOR_CHECK(canor_enum_flow_unrank(CANOR_ENUM_FLOW_COUNT, &f) == CANOR_ERR_RANGE);
}

static void test_metaflow(void) {
  canor_enum_metaflow_t m;
  canor_enum_fractal_t x;
  char buf[8];
  uint64_t r;
  for (uint64_t k = 0; k < CANOR_ENUM_METAFLOW_COUNT; ++k) {
    CANOR_CHECK(canor_enum_metaflow_unrank(k, &m) == CANOR_OK);
    CANOR_CHECK(canor_enum_metaflow_rank(&m, &r) == CANOR_OK && r == k);
  }
  CANOR_CHECK(canor_enum_metaflow_unrank(0, &m) == CANOR_OK);
  CANOR_CHECK(canor_enum_metaflow_format(&m, buf, sizeof buf) == CANOR_OK && !strcmp(buf, "RGBA"));
  CANOR_CHECK(canor_enum_metaflow_unrank(23, &m) == CANOR_OK);
  CANOR_CHECK(canor_enum_metaflow_format(&m, buf, sizeof buf) == CANOR_OK && !strcmp(buf, "ABGR"));

  /* Every invocation's fractal string balances: reals + gaps = support, and
   * each generation's multiplicity is (b-1)^j. */
  int ok = 1;
  for (uint64_t k = 0; k < CANOR_ENUM_INVOCATION_COUNT; ++k) {
    ok &= canor_enum_invocation_unrank(k, &x) == CANOR_OK;
    ok &= x.reals + x.gaps == x.support;
    ok &= x.depth == x.strategy.order && x.direction == x.strategy.differential;
    ok &= canor_enum_metaflow_rank(&x.slots, &r) == CANOR_OK && r == k / CANOR_ENUM_STRATEGY_COUNT;
    uint64_t mult = 1;
    for (unsigned j = 0; j < x.depth; ++j) {
      ok &= x.gap_mult[j] == mult;
      mult *= x.base - 1u;
    }
  }
  CANOR_CHECK(ok);
  CANOR_CHECK(canor_enum_invocation_unrank(5, &x) == CANOR_OK);        /* di^6 */
  CANOR_CHECK(x.support == 729 && x.reals == 64 && x.gaps == 665 && x.gap_len[0] == 243);
  CANOR_CHECK(canor_enum_invocation_unrank(CANOR_ENUM_INVOCATION_COUNT, &x) == CANOR_ERR_RANGE);
}

static void test_soul(void) {
  canor_enum_soul_t s;
  canor_soul_rep_t rep, all;
  char buf[CANOR_ENUM_SYMBOL_MAX];

  CANOR_CHECK(canor_enum_soul_unrank(0, &s) == CANOR_OK);
  CANOR_CHECK(canor_enum_soul_format(&s, buf, sizeof buf) == CANOR_OK &&
              !strcmp(buf, "di^1:ijk|lmn|opq|rst|efg:RGBA"));
  CANOR_CHECK(s.category == CANOR_SOUL_STORAGE);

  int laws = 1;
  for (uint64_t r = 0; r < CANOR_ENUM_SOUL_COUNT; r += SOUL_STRIDE) {
    laws &= canor_enum_soul_unrank(r, &s) == CANOR_OK && s.rank == r;
    laws &= (uint64_t)((s.strategy_rank * CANOR_ENUM_FLOW_COUNT + s.flow_rank) *
                           CANOR_ENUM_METAFLOW_COUNT + s.metaflow_rank) == r;
    laws &= s.stream.n == 15;                                /* n + 1 = 16 rows */
    for (int t = 0; t < 5; ++t) laws &= s.stream.v[15][t][0] + s.stream.v[15][t][1] + s.stream.v[15][t][2] == 0;
  }
  CANOR_CHECK(laws);

  /* Every representation agrees with the Unknown (whole-stream) view. */
  int agree = 1;
  for (int k = 0; k < CANOR_SOUL_KINDS; ++k) {
    uint64_t n = canor_enum_rep_count(CANOR_ENUM_SOUL, (canor_soul_kind)k);
    for (uint64_t r = 0; r < n; r += REP_STRIDE * (k == 0 ? 97u : 1u)) {
      agree &= canor_enum_soul_rep_unrank((canor_soul_kind)k, r, &s, &rep) == CANOR_OK;
      agree &= canor_soul_represent(&s.stream, CANOR_SOUL_UNKNOWN, 0, &all) == CANOR_OK;
      for (uint16_t i = 0; i < rep.nreals; ++i)
        for (uint16_t j = 0; j < rep.nbases; ++j) {
          uint16_t st = rep.step == CANOR_SOUL_ALL ? i : rep.step;
          uint16_t b = rep.base == CANOR_SOUL_ALL ? (uint16_t)(1 + j) : rep.base;
          agree &= rep.value[i * rep.nbases + j] == all.value[st * 15 + (b - 1)];
        }
    }
    CANOR_CHECK(canor_enum_soul_rep_unrank((canor_soul_kind)k, n, &s, &rep) == CANOR_ERR_RANGE);
  }
  CANOR_CHECK(agree);
}

static uint64_t expected_holds(void) {
  /* Independent count: for modulus o, sum over residues r of c_r^2, where c_r
   * is how many of 0..289 leave remainder r. */
  uint64_t total = 0, top = CANOR_ENUM_ACTION_BOUND - 1;
  for (uint64_t o = 1; o < CANOR_ENUM_ACTION_BOUND; ++o)
    for (uint64_t r = 0; r < o; ++r) {
      uint64_t cr = (top - r) / o + 1;
      total += cr * cr;
    }
  return total;
}

static void test_action(void) {
  canor_enum_action_t a;
  char buf[CANOR_ENUM_SYMBOL_MAX];
  uint64_t undefined = 0, holds = 0, r;
  int canonical = 1, roundtrip = 1;
  for (uint64_t k = 0; k < CANOR_ENUM_ACTION_COUNT; ++k) {
    canor_enum_action_unrank(k, &a);
    if (a.status == CANOR_ERR_UNDEFINED) {
      ++undefined;
      canonical &= a.o == 0 && !a.holds;
      continue;
    }
    canonical &= a.rm < a.o && a.rn < a.o && a.o > 0;
    holds += a.holds;
    roundtrip &= canor_enum_action_rank(a.m, a.n, a.o, &r) == CANOR_OK && r == k;
  }
  CANOR_CHECK(undefined == CANOR_ENUM_ACTION_UNDEFINED_COUNT);
  CANOR_CHECK(canonical && roundtrip);
  CANOR_CHECK(holds == expected_holds());
  CANOR_CHECK(canor_enum_action_rank(17, 5, 12, &r) == CANOR_OK);
  CANOR_CHECK(canor_enum_action_unrank(r, &a) == CANOR_OK && a.holds && a.rm == 5);
  CANOR_CHECK(canor_enum_action_format(&a, buf, sizeof buf) == CANOR_OK &&
              !strcmp(buf, "17 \xE2\x89\xA3 5 (mod 12)"));
  CANOR_CHECK(canor_enum_action_rank(290, 0, 1, &r) == CANOR_ERR_RANGE);
}

static void test_maneuver(void) {
  canor_enum_maneuver_t m;
  canor_soul_rep_t rep;
  uint64_t last = CANOR_ENUM_MANEUVER_COUNT - 1, r;

  CANOR_CHECK(canor_enum_maneuver_unrank(last, &m) == CANOR_OK);
  CANOR_CHECK(m.soul.rank == CANOR_ENUM_SOUL_COUNT - 1 && m.action.rank == CANOR_ENUM_ACTION_COUNT - 1);
  CANOR_CHECK(canor_enum_maneuver_unrank(CANOR_ENUM_MANEUVER_COUNT, &m) == CANOR_ERR_RANGE);

  /* The Action maps the stream to canonical residues: mod 2 of 1 is 1, of 0 is 0. */
  canor_enum_action_rank(17, 5, 2, &r);
  CANOR_CHECK(canor_enum_maneuver_unrank(123 * CANOR_ENUM_ACTION_COUNT + r, &m) == CANOR_OK);
  int canon = 1;
  for (uint16_t st = 0; st <= 15; ++st)
    for (int t = 0; t < 5; ++t)
      for (int c = 0; c < 3; ++c)
        canon &= m.residues.v[st][t][c] == m.soul.stream.v[st][t][c] % 2 && m.residues.v[st][t][c] < 2;
  CANOR_CHECK(canon && m.action.holds);

  /* o = 0: the Maneuver exists, its residues do not. */
  CANOR_CHECK(canor_enum_maneuver_rep_unrank(CANOR_SOUL_UNKNOWN, 0, &m, &rep) == CANOR_ERR_UNDEFINED);
  CANOR_CHECK(m.action.status == CANOR_ERR_UNDEFINED);
  CANOR_CHECK(canor_enum_maneuver_rep_unrank(CANOR_SOUL_ELEMENTARY, 240 * (r + 0), &m, &rep) == CANOR_OK);
  CANOR_CHECK(rep.nreals == 1 && rep.nbases == 1 && rep.value[0] < m.action.o);
  CANOR_CHECK(canor_enum_maneuver_rep_unrank(CANOR_SOUL_ELEMENTARY,
                                             canor_enum_rep_count(CANOR_ENUM_MANEUVER, CANOR_SOUL_ELEMENTARY),
                                             &m, &rep) == CANOR_ERR_RANGE);
}

int main(void) {
  test_imag();
  test_counts();
  test_strategy();
  test_flow();
  test_metaflow();
  test_soul();
  test_action();
  test_maneuver();
  return CANOR_TEST_DONE("canor_test_enum");
}
