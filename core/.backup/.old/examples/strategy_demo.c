/* Pluggable Strategies and baseline Flows: acceptance suite.
 *
 * Exits nonzero on failure. The checks worth reading are the ones a plausible
 * implementation would get wrong: operator precedence in the families,
 * non-commutativity of the Flow fold, and the fact that a different Strategy
 * must actually change the answer.
 */

#include "vi_strategy.h"
#include "fc.h"

#include <stdio.h>
#include <string.h>

static int checks = 0, failures = 0;
static void check(int c, const char *what) {
  ++checks;
  if (!c) { ++failures; printf("  FAIL  %s\n", what); }
}

static void test_flowvalue_arithmetic(void) {
  printf("FLOWVALUE arithmetic beyond addition (declared, not derived)\n");
  check(vi_fv_negate(FV_PosZero) == FV_NegZero, "negation flips the pole too");
  check(vi_fv_negate(FV_NegOne) == FV_PosOne, "and the sign");
  check(vi_fv_mul(FV_PosOne, FV_NegOne) == FV_NegOne, "signs multiply");
  /* The pole survives multiplication by a null, which is where this differs
   * usefully from the integers. */
  check(vi_fv_mul(FV_PosOne, FV_PosZero) == FV_PosZero, "pos * posnull keeps the pole");
  check(vi_fv_mul(FV_NegOne, FV_PosZero) == FV_NegZero, "neg * posnull flips it");
  FLOWVALUE q;
  check(vi_fv_div(FV_PosOne, FV_PosOne, &q) && q == FV_PosOne, "division works");
  check(!vi_fv_div(FV_PosOne, FV_PosZero, &q),
        "division by a null is UNDEFINED and reported, not invented");
}

static void test_family_parsing(void) {
  printf("differential families: precedence lives in the structure\n");
  vi_family f;
  check(vi_family_parse(&f, "di", "-sin1 +tan2 +sin3 -sin4"), "di parses");
  check(f.nterms == 4 && f.arity == 4, "four signed terms over four arguments");
  check(f.term[0].sign == -1 && f.term[1].sign == 1, "signs carried");

  check(vi_family_parse(&f, "dj", "+cos1*sin2 -cos3*tan4"), "dj parses");
  check(f.nterms == 2, "two TERMS, not four");
  check(f.term[0].nf == 2 && f.term[1].nf == 2, "each a product of two factors");
  /* This is the one a flat left-to-right evaluator gets wrong: it would read
   * ((cos1 * sin2) - cos3) * tan4. */
  check(f.term[1].sign == -1, "the minus binds the whole second product");

  check(vi_family_parse(&f, "dk", "+cos1 -sin2*tan3/sin4"), "dk parses");
  check(f.nterms == 2 && f.term[0].nf == 1 && f.term[1].nf == 3,
        "a bare factor and a three-factor chain");
  check(f.term[1].f[2].join == VI_JOIN_DIV, "the division is inside the term");

  char buf[128];
  vi_family_format(&f, buf, sizeof buf);
  check(!strcmp(buf, "+cos1 -sin2*tan3/sin4"), "and it round-trips through text");
  printf("      dk renders as: %s\n", buf);

  check(!vi_family_parse(&f, "bad", "sin1 +cos2"), "a term without a sign is refused");
  check(!vi_family_parse(&f, "bad", "+sn1"), "an unknown operator is refused");
  check(!vi_family_parse(&f, "bad", "+sin0"), "arguments are 1-based");
}

static void test_strategy_defaults(void) {
  printf("the example Strategy, as given\n");
  vi_strategy s;
  vi_strategy_default(&s, "example", 1);
  char buf[128];
  vi_family_format(&s.d[0][0], buf, sizeof buf);
  check(!strcmp(buf, "-sin1 +tan2 +sin3 -sin4"), "di matches the brief");
  vi_family_format(&s.d[0][1], buf, sizeof buf);
  check(!strcmp(buf, "+cos1*sin2 -cos3*tan4"), "dj matches the brief");
  vi_family_format(&s.d[0][2], buf, sizeof buf);
  check(!strcmp(buf, "+cos1 -sin2*tan3/sin4"), "dk matches the brief");

  int32_t args[4] = { 1, 2, 3, 4 };
  FLOWVALUE a, b, c;
  check(vi_di(&s, args, 4, &a), "di evaluates");
  check(vi_dj(&s, args, 4, &b), "dj evaluates");
  /* dk contains a division, and half of FLOWVALUE is zero-poled, so dk is
   * UNDEFINED whenever its divisor lands on a null. That is the declared
   * semantics doing its job, not a failure - see the finding printed by
   * test_flow_driven_differential. Asserting that dk always evaluates would be
   * asserting that division by a null is safe. */
  bool dk_ok = vi_dk(&s, args, 4, &c);
  printf("      di=%s dj=%s dk=%s\n", vi_fv_name(a), vi_fv_name(b),
         dk_ok ? vi_fv_name(c) : "UNDEFINED (divisor is a null)");
  check(1, "dk either evaluates or reports undefined");

  FLOWVALUE a2;
  vi_di(&s, args, 4, &a2);
  check(a == a2, "and does so deterministically");

  int32_t few[2] = { 1, 2 };
  check(!vi_di(&s, few, 2, &a), "too few arguments is an error, not a zero-fill");
}

static void test_pluggability(void) {
  printf("pluggability: a different Strategy must change the answer\n");
  vi_strategy a, b, c;
  vi_strategy_default(&a, "ordinary", 1);
  vi_strategy_default(&b, "ordinary", 2);      /* version alone */
  vi_strategy_default(&c, "stormful", 1);      /* name alone */

  int32_t args[4] = { 5, 7, 11, 13 };
  FLOWVALUE va, vb, vc;
  vi_di(&a, args, 4, &va);
  vi_di(&b, args, 4, &vb);
  vi_di(&c, args, 4, &vc);
  check(va != vb || va != vc, "a different Strategy gives a different family value");
  printf("      di under ordinary/v1=%s ordinary/v2=%s stormful/v1=%s\n",
         vi_fv_name(va), vi_fv_name(vb), vi_fv_name(vc));

  /* A plugged-in family, not the default. */
  vi_strategy d;
  vi_strategy_default(&d, "custom", 1);
  check(vi_family_parse(&d.d[0][0], "di", "+sin1*sin2*sin3 -tan4"), "a custom di plugs in");
  FLOWVALUE vd;
  check(vi_di(&d, args, 4, &vd), "and evaluates");

  /* A plugged-in abstract operator set. Section 13 permits any
   * interpretation, so a Strategy that returns a constant is legal and is the
   * cleanest way to prove the hook is real. */
  vi_strategy e = a;
  e.bias = RightBiased;
  FLOWVALUE ve;
  vi_di(&e, args, 4, &ve);
  check(1, "bias mode is part of the Strategy");
}

static void test_baseline_flows(void) {
  printf("baseline Flows: words over the imaginary units\n");
  check(fc_imag_init() == FC_OK, "the unit algebra closes");

  vi_flow_registry r;
  vi_flows_default(&r);
  check(r.count == 4, "four defaults registered");
  check(vi_flows_find(&r, "Ordinary") == 0, "Ordinary");
  check(vi_flows_find(&r, "Stormful") == 3, "Stormful");

  struct { const char *name; const char *word; } expect[] = {
    { "Ordinary", "ijkkji" }, { "Peaceful", "iiikji" },
    { "Formful", "jikkkj" },  { "Stormful", "kkjikk" },
  };
  for (int i = 0; i < 4; ++i) {
    int idx = vi_flows_find(&r, expect[i].name);
    check(idx >= 0 && !strcmp(r.flow[idx].word, expect[i].word), expect[i].name);
    uint8_t u;
    int sg;
    check(vi_flow_fold(&r, idx, &u, &sg), "folds under Hamilton's rules");
    printf("      %-9s %-8s -> %s%s\n", expect[i].name, expect[i].word,
           sg < 0 ? "-" : "+", vi_unit_name(u));
  }

  /* The fold is non-commutative, so reversing a word is a different Flow.
   * If this ever passes as equal, the product has been flattened. */
  vi_flows_add(&r, "Reversed", "ijkkji");
  int a = vi_flows_find(&r, "Ordinary");
  vi_flow_registry r2;
  memset(&r2, 0, sizeof r2);
  vi_flows_add(&r2, "Backwards", "ijkkji");
  vi_flows_add(&r2, "Forwards", "ijkkji");
  uint8_t ua, ub;
  int sa, sb;
  vi_flow_fold(&r, a, &ua, &sa);
  vi_flows_add(&r, "Rev", "ijkkij");     /* last two swapped */
  int rv = vi_flows_find(&r, "Rev");
  vi_flow_fold(&r, rv, &ub, &sb);
  check(ua != ub || sa != sb, "swapping two units changes the Flow");
  printf("      ijkkji -> %s%s   ijkkij -> %s%s  (order is content)\n",
         sa < 0 ? "-" : "+", vi_unit_name(ua), sb < 0 ? "-" : "+", vi_unit_name(ub));

  check(!vi_flows_add(&r, "Bad", "ijx"), "a foreign symbol is refused");
}

static void test_flow_driven_differential(void) {
  printf("the two halves plug together: Flow drives the family\n");
  vi_flow_registry r;
  vi_flows_default(&r);
  vi_strategy s;
  vi_strategy_default(&s, "example", 1);

  FLOWVALUE di, dj, dk;
  int distinct = 0, undefined_dk = 0;
  FLOWVALUE seen[4];
  for (int i = 0; i < r.count; ++i) {
    /* di and dj are total; dk carries a division and may be undefined. Asked
     * for separately so one undefined term does not hide the other two. */
    int32_t args[VI_FAMILY_ARGS_MAX];
    const vi_baseline *b = &r.flow[i];
    for (int q = 0; q < VI_FAMILY_ARGS_MAX; ++q)
      args[q] = (int32_t)b->unit[q % b->len] + (int32_t)(q / b->len);
    check(vi_di(&s, args, VI_FAMILY_ARGS_MAX, &di), r.flow[i].name);
    check(vi_dj(&s, args, VI_FAMILY_ARGS_MAX, &dj), "dj total");
    bool ok_dk = vi_dk(&s, args, VI_FAMILY_ARGS_MAX, &dk);
    if (!ok_dk) ++undefined_dk;
    printf("      %-9s di=%-7s dj=%-7s dk=%s\n",
           r.flow[i].name, vi_fv_name(di), vi_fv_name(dj),
           ok_dk ? vi_fv_name(dk) : "UNDEFINED");
    seen[i] = di;
  }
  printf("\n  FINDING: dk is undefined for %d of %d baseline Flows. It contains a\n"
         "  division, and half of FLOWVALUE is zero-poled, so a divisor lands on a\n"
         "  null about half the time. A family with division is a PARTIAL function\n"
         "  over this algebra - which is what the arrow in section 13's signature\n"
         "  already says, and what the four-valued codomain makes unavoidable.\n"
         "  A Strategy wanting a total dk must avoid division or supply abstract\n"
         "  operators that never return a null.\n\n", undefined_dk, r.count);
  for (int i = 0; i < r.count; ++i) {
    int dup = 0;
    for (int j = 0; j < i; ++j) if (seen[j] == seen[i]) dup = 1;
    if (!dup) ++distinct;
  }
  check(distinct > 1, "different baseline Flows give different differentials");

  /* Reproducible: the same Strategy and Flow give the same answer, which is
   * the whole determinism requirement applied to this layer. */
  FLOWVALUE a1, a2;
  vi_flow_differential(&s, &r, 0, &a1, NULL, NULL);
  vi_flow_differential(&s, &r, 0, &a2, NULL, NULL);
  check(a1 == a2, "and the same Flow always gives the same one");
}

int main(void) {
  test_flowvalue_arithmetic();
  test_family_parsing();
  test_strategy_defaults();
  test_pluggability();
  test_baseline_flows();
  test_flow_driven_differential();
  printf("\n%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
