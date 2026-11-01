/* Acceptance suite for the Follow-Core type tower.
 *
 * Exits nonzero on failure. The checks that matter are the ones that would
 * still "work" if the implementation were wrong: the Cayley-Dickson identities,
 * the factoradic invariant, the Lehmer round trip, and the ordering contracts.
 */

#include "fc.h"

#include <stdio.h>
#include <string.h>

static int checks = 0, failures = 0;
static void check(int c, const char *what) {
  ++checks;
  if (!c) { ++failures; printf("  FAIL  %s\n", what); }
}

/* The rational arithmetic itself. These are the cases where the previous
 * implementation was undefined rather than merely imprecise: it multiplied
 * int64 numerators with no check, so every one of the overflow cases below
 * wrapped and returned a plausible wrong rational. */
static void test_rational(void) {
  fc_rational a, b, r;
  const int64_t BIG = 3037000500LL;         /* just over sqrt(INT64_MAX) */
  printf("RATIONAL - exact, reduced, and checked\n");

  check(fc_rat_valid(fc_rat(1, 2)), "1/2 is a value");
  check(!fc_rat_valid(fc_rat(1, 0)), "a zero denominator is poison, not 0/1");
  check(fc_rat_eq(fc_rat(2, 4), fc_rat(1, 2)), "2/4 reduces to 1/2");
  check(fc_rat_eq(fc_rat(1, -2), fc_rat(-1, 2)), "the sign moves to the numerator");

  /* Cross-cancellation: the naive product overflows, the answer is 1. */
  a = fc_rat(BIG, 7);
  b = fc_rat(7, BIG);
  r = fc_rat_mul(a, b);
  check(fc_rat_valid(r) && fc_rat_eq(r, fc_rat(1, 1)),
        "(BIG/7)*(7/BIG) = 1 without overflowing on the way");

  /* A product that genuinely does not fit must be refused, not wrapped. */
  a = fc_rat(BIG, 1);
  r = fc_rat_mul(a, a);
  check(!fc_rat_valid(r), "a product too large for int64 is poison");

  /* Addition uses the least common denominator, so this stays in range. */
  a = fc_rat(1, BIG);
  b = fc_rat(1, BIG);
  r = fc_rat_add(a, b);
  check(fc_rat_valid(r) && fc_rat_eq(r, fc_rat(2, BIG)),
        "1/BIG + 1/BIG = 2/BIG, no denominator squaring");
  /* BIG*(BIG-1) still fits, so that pair is a value; BIG*(BIG+1) does not.
   * The boundary is worth pinning from both sides - a check that only ever
   * sees the refusal cannot tell a correct limit from one set too low. */
  a = fc_rat(1, BIG);
  b = fc_rat(1, BIG - 1);
  r = fc_rat_add(a, b);
  check(fc_rat_valid(r), "1/BIG + 1/(BIG-1) still fits, and is computed");
  a = fc_rat(1, BIG);
  b = fc_rat(1, BIG + 1);
  r = fc_rat_add(a, b);
  check(!fc_rat_valid(r), "1/BIG + 1/(BIG+1) does not fit, and says so");

  /* Poison propagates and never becomes a number again. */
  r = fc_rat_mul(fc_rat(1, 0), fc_rat(1, 1));
  check(!fc_rat_valid(r), "poison propagates through multiply");
  r = fc_rat_add(fc_rat(1, 0), fc_rat(1, 1));
  check(!fc_rat_valid(r), "poison propagates through add");
  check(!fc_rat_eq(fc_rat(1, 0), fc_rat(1, 0)),
        "two failures agreeing they failed is not equality");

  /* Comparison never multiplies, so it is exact at any magnitude. */
  check(fc_rat_cmp(fc_rat(1, 3), fc_rat(1, 2)) < 0, "1/3 < 1/2");
  check(fc_rat_cmp(fc_rat(-1, 3), fc_rat(-1, 2)) > 0, "-1/3 > -1/2");
  check(fc_rat_cmp(fc_rat(3, 4), fc_rat(3, 4)) == 0, "3/4 = 3/4");
  check(fc_rat_cmp(fc_rat(INT64_MAX, INT64_MAX - 1),
                   fc_rat(INT64_MAX - 1, INT64_MAX - 2)) < 0,
        "two near-1 ratios whose cross product would overflow, ordered exactly");
  check(fc_rat_cmp(fc_rat(-5, 2), fc_rat(5, 2)) < 0, "negatives order below positives");
  check(fc_rat_cmp(fc_rat(0, 1), fc_rat(-1, BIG)) > 0, "0 > -1/BIG");

  /* INT64_MIN has no positive magnitude, so it cannot be reduced. */
  check(!fc_rat_valid(fc_rat(INT64_MIN, 1)), "INT64_MIN numerator is refused");
  check(!fc_rat_valid(fc_rat(1, INT64_MIN)), "INT64_MIN denominator is refused");
}

static void test_basis(void) {
  printf("BASIS - regex-generated, evaluates to a rational\n");
  fc_basis b;
  check(fc_basis_parse("+7", &b) == FC_OK && b.v == 7, "+7 parses");
  check(fc_basis_parse("-3", &b) == FC_OK && b.v == -3, "-3 parses");
  /* The regex requires an explicit sign and a nonzero leading digit. */
  check(fc_basis_parse("7", &b) != FC_OK, "a bare 7 is refused (sign required)");
  check(fc_basis_parse("+0", &b) != FC_OK, "+0 is refused (leading digit 1-9)");
  check(fc_basis_parse("+07", &b) != FC_OK, "+07 is refused");
  check(fc_basis_parse("", &b) != FC_OK, "empty is refused");
  check(fc_basis_parse("+1a", &b) != FC_OK, "trailing junk is refused");
  check(fc_basis_make(0, &b) == FC_ERR_RANGE, "BASIS may not be zero");

  fc_basis_parse("+7", &b);
  check(fc_rat_eq(fc_basis_eval(b), fc_rat(7, 1)), "eval(+7) = 7");
  fc_basis_parse("-4", &b);
  /* The asymmetry that makes eval return a rational at all. */
  check(fc_rat_eq(fc_basis_eval(b), fc_rat(1, 4)), "eval(-4) = 1/4, not -4");
}

static void test_num(void) {
  printf("NUM - four-valued signage, exact evaluation\n");
  fc_num n;
  fc_rational r;
  check(fc_num_make(FC_POS_ONE, 5, 2, 3, &n) == FC_OK, "NUM built");
  check(fc_num_eval(&n, &r) == FC_OK && fc_rat_eq(r, fc_rat(40, 1)),
        "+1 * 5 * 2^3 = 40");
  fc_num_make(FC_NEG_ONE, 5, 2, 3, &n);
  fc_num_eval(&n, &r);
  check(fc_rat_eq(r, fc_rat(-40, 1)), "-1 signage negates");

  /* A zero pole evaluates to zero whatever the magnitude - it is a pole, not
   * a scale - and the two poles are numerically equal, representationally
   * distinct. That is the same law as FLOWALGEBRA's signed zeros. */
  fc_num_make(FC_POS_ZERO, 5, 2, 3, &n);
  fc_num_eval(&n, &r);
  check(fc_rat_eq(r, fc_rat(0, 1)), "+0 signage evaluates to zero");
  fc_num a, b;
  fc_rational ra, rb;
  fc_num_make(FC_NEG_ZERO, 9, 2, 1, &a);
  fc_num_make(FC_POS_ZERO, 3, 5, 1, &b);
  fc_num_eval(&a, &ra); fc_num_eval(&b, &rb);
  check(fc_rat_eq(ra, rb), "NegZero and PosZero are numerically equal");
  check(a.sign != b.sign, "and representationally distinct");

  /* A negative exponent basis is a fractional power, i.e. a root, which is not
   * generally rational - refused rather than approximated. */
  fc_num_make(FC_POS_ONE, 4, 9, -2, &n);
  check(fc_num_eval(&n, &r) == FC_ERR_UNDEFINED, "a fractional exponent is refused");
  check(fc_num_make(FC_POS_ONE, 1, 0, 1, &n) == FC_ERR_RANGE, "zero basis refused");
  check(fc_num_make((fc_sign)0, 1, 2, 1, &n) == FC_ERR_RANGE, "sign 0 is not in the signage");
}

static void test_number(void) {
  printf("NUMBER - factoradic\n");
  fc_number f;
  uint64_t v;
  check(fc_number_from_u64(0, &f) == FC_OK && fc_number_valid(&f), "zero encodes");
  check(fc_number_to_u64(&f, &v) == FC_OK && v == 0, "and decodes");

  int all = 1;
  for (uint64_t x = 0; x < 5000; ++x) {
    if (fc_number_from_u64(x, &f) != FC_OK) { all = 0; break; }
    if (!fc_number_valid(&f)) { all = 0; break; }
    if (fc_number_to_u64(&f, &v) != FC_OK || v != x) { all = 0; break; }
  }
  check(all, "round trip exact for 0..4999");

  /* The factoradic invariant: digit i <= i, so digit 0 is always zero. */
  fc_number_from_u64(4999, &f);
  check(f.d[0].value == 0, "digit 0 is always zero");
  f.d[3].value = 9;
  check(!fc_number_valid(&f), "digit i > i is detected");
}

static void test_lehmer(void) {
  printf("LEHMER - the n!-level topology index\n");
  fc_lehmer l;
  fc_lehmer_zero(&l, 6);
  check(fc_lehmer_valid(&l), "6 levels is valid (the practical minimum)");
  check(l.levels == FC_LEVELS_MIN, "minimum enforced");

  fc_lehmer_zero(&l, FC_LEVELS_MAX);
  check(fc_lehmer_valid(&l) && l.levels == 96, "96 levels is valid (the maximum)");

  /* 96! is about 1e149, so it is never counted to. to_u64 refuses rather than
   * wrapping, because an aliased address is worse than an error. */
  uint64_t v;
  check(fc_lehmer_to_u64(&l, &v) == FC_ERR_RANGE, "96 levels refuses a u64 value");
  fc_lehmer_zero(&l, 20);
  check(fc_lehmer_to_u64(&l, &v) == FC_OK, "20 levels still fits a u64");

  int all = 1;
  for (uint64_t x = 0; x < 720; ++x) {          /* 6! = 720, the whole space */
    fc_lehmer c;
    uint64_t back;
    if (fc_lehmer_from_u64(x, 6, &c) != FC_OK) { all = 0; break; }
    if (!fc_lehmer_valid(&c)) { all = 0; break; }
    if (fc_lehmer_to_u64(&c, &back) != FC_OK || back != x) { all = 0; break; }
  }
  check(all, "every address in a 6-level topology round-trips");

  /* The odometer walks the whole space and then reports the end rather than
   * wrapping to zero. */
  fc_lehmer_zero(&l, 6);
  int steps = 0;
  while (fc_lehmer_inc(&l) == FC_OK) ++steps;
  check(steps == 719, "increment visits all 720 addresses exactly once");

  /* A Lehmer code IS a permutation - the tie between NUMBER and BASE. */
  uint8_t perm[FC_LEVELS_MAX], perm2[FC_LEVELS_MAX];
  fc_lehmer_from_u64(371, 6, &l);
  check(fc_lehmer_to_permutation(&l, perm, FC_LEVELS_MAX) == FC_OK, "decodes to a permutation");
  int is_perm = 1, seen[6] = {0};
  for (int i = 0; i < 6; ++i) { if (perm[i] >= 6 || seen[perm[i]]) is_perm = 0; else seen[perm[i]] = 1; }
  check(is_perm, "and it is a genuine permutation");
  fc_lehmer back;
  check(fc_lehmer_from_permutation(perm, 6, &back) == FC_OK, "re-encodes");
  check(fc_lehmer_cmp(&l, &back) == 0, "permutation round trip is exact");

  /* Distinct addresses give distinct permutations across the whole 6! space. */
  int distinct = 1;
  for (uint64_t x = 0; x < 720 && distinct; ++x) {
    fc_lehmer c;
    fc_lehmer_from_u64(x, 6, &c);
    fc_lehmer_to_permutation(&c, perm, FC_LEVELS_MAX);
    for (uint64_t y = x + 1; y < 720; ++y) {
      fc_lehmer c2;
      fc_lehmer_from_u64(y, 6, &c2);
      fc_lehmer_to_permutation(&c2, perm2, FC_LEVELS_MAX);
      if (memcmp(perm, perm2, 6) == 0) { distinct = 0; break; }
    }
  }
  check(distinct, "the index is injective onto permutations");

  uint8_t bad[6] = { 0, 0, 2, 3, 4, 5 };
  check(fc_lehmer_from_permutation(bad, 6, &back) == FC_ERR_RANGE,
        "a non-permutation is refused");
}

static void test_imaginary(void) {
  printf("IMAG / IMAGINARY / BASE - Cayley-Dickson\n");
  fc_status s = fc_imag_init();
  check(s == FC_OK, "the declared dimension closes");
  if (s != FC_OK) { printf("      %s\n", fc_strerror(s)); return; }

  fc_imag idx;
  int sign;
  /* Quaternion identities, which live in the first four units whatever the
   * total dimension. If the doubling is wrong these are the first casualties. */
  check(fc_imag_mul(1, 1, &idx, &sign) == FC_OK && idx == 0 && sign == -1, "i*i = -1");
  check(fc_imag_mul(2, 2, &idx, &sign) == FC_OK && idx == 0 && sign == -1, "j*j = -1");
  check(fc_imag_mul(1, 2, &idx, &sign) == FC_OK && idx == 3 && sign == 1, "i*j = k");
  check(fc_imag_mul(2, 1, &idx, &sign) == FC_OK && idx == 3 && sign == -1, "j*i = -k");
  check(fc_imag_mul(2, 3, &idx, &sign) == FC_OK && idx == 1 && sign == 1, "j*k = i");
  check(fc_imag_mul(3, 1, &idx, &sign) == FC_OK && idx == 2 && sign == 1, "k*i = j");
  check(fc_imag_mul(0, 5, &idx, &sign) == FC_OK && idx == 5 && sign == 1, "one is the identity");

  /* Non-commutative from dimension 4 up. */
  fc_imag p, q;
  int sp, sq;
  fc_imag_mul(1, 2, &p, &sp);
  fc_imag_mul(2, 1, &q, &sq);
  check(p == q && sp == -sq, "the product is non-commutative");

  /* Non-associative from dimension 8 up: (e1 e2) e4 differs from e1 (e2 e4).
   * This is the property that distinguishes octonions from quaternions, and a
   * hand-written table almost always gets it wrong. */
  if (FC_IMAG_COUNT >= 8) {
    fc_imag ab, abc, bc, abc2;
    int s1, s2, s3, s4;
    fc_imag_mul(1, 2, &ab, &s1);
    fc_imag_mul(ab, 4, &abc, &s2);
    fc_imag_mul(2, 4, &bc, &s3);
    fc_imag_mul(1, bc, &abc2, &s4);
    check(abc == abc2 && (s1 * s2) != (s3 * s4), "non-associative at dimension 8");
  }

  /* BASE is asserted to be a permutation group; checked rather than believed. */
  int all_perm = 1;
  for (int i = 0; i < FC_IMAG_COUNT; ++i) if (!fc_base_is_permutation((fc_imag)i)) all_perm = 0;
  check(all_perm, "left multiplication permutes the units, for every unit");

  check(fc_imag_conj_sign(0) == 1 && fc_imag_conj_sign(1) == -1,
        "conjugation fixes one and negates the rest");

  /* The doubly-linked BASE list folds in list order - and order matters, since
   * the product is not commutative. */
  fc_base base;
  fc_base_init(&base);
  fc_imaginary u;
  u.ring = 0;
  u.imag = 1; check(fc_base_push(&base, u) == FC_OK, "push i");
  u.imag = 2; check(fc_base_push(&base, u) == FC_OK, "push j");
  fc_imaginary res;
  int rs;
  check(fc_base_eval(&base, &res, &rs) == FC_OK && res.imag == 3 && rs == 1,
        "BASE folds i then j to +k");

  fc_base rev;
  fc_base_init(&rev);
  u.imag = 2; fc_base_push(&rev, u);
  u.imag = 1; fc_base_push(&rev, u);
  fc_base_eval(&rev, &res, &rs);
  check(res.imag == 3 && rs == -1, "and j then i to -k: list order is the contract");
}

static void test_data_and_units(void) {
  printf("DATA / REAL / INTERVAL / UNIT / EXPRESSION\n");
  fc_triple t = { fc_rat(1, 2), fc_rat(3, 4), fc_rat(1, 1) };
  check(fc_triple_ordered(&t), "min <= nom <= max holds");
  fc_triple bad = { fc_rat(1, 1), fc_rat(1, 2), fc_rat(1, 1) };
  check(!fc_triple_ordered(&bad), "and is detected when it does not");

  fc_interval i = { fc_rat(-1, 2), fc_rat(1, 2) };
  check(fc_interval_ordered(&i), "interval ordered");

  static fc_unit atom, change, force;
  fc_unit_init(&atom, FC_T_VALUE);
  fc_unit_init(&change, FC_T_REFERENCE);
  fc_unit_init(&force, FC_T_FUNCTIONAL);
  check(fc_unit_is_atom(&atom), "ATOM is UNIT[VALUE]");
  check(fc_unit_is_change(&change), "CHANGE is UNIT[REFERENCE]");
  check(fc_unit_is_force(&force), "FORCE is UNIT[FUNCTIONAL]");
  check(!fc_unit_is_atom(&change), "and the three are distinguishable");

  fc_interval inverted = { fc_rat(1, 1), fc_rat(0, 1) };
  check(fc_unit_add_interval(&atom, inverted) == FC_ERR_RANGE,
        "an inverted interval is refused at the boundary");

  /* static, not a local. EXPRESSION nests UNIT -> TENSOR -> TENSORIAL ->
   * MODULATION -> BASE, so it is far too large for a stack frame even at the
   * reduced capacities - the size is printed below rather than asserted,
   * because it is a property of the chosen capacities and not of the design. */
  static fc_expression e;
  fc_expression_init(&e);
  check(fc_expression_add(&e, &atom) == FC_OK && e.n_output == 1,
        "EXPRESSION collects UNITs");
  printf("      sizeof(fc_expression) = %zu bytes, sizeof(fc_bbst) = %zu bytes\n",
         sizeof(fc_expression), sizeof(fc_bbst));
}

static void test_tensor(void) {
  printf("TENSORIAL / TENSOR / MODULATION\n");
  static fc_tensorial tl;
  fc_tensorial_init(&tl, 1);
  check(fc_tensorial_flags_consistent(&tl), "fresh flags are consistent");
  tl.unknownsymmetric = false;
  tl.symmetric = true;
  tl.antisymmetric = true;
  check(!fc_tensorial_flags_consistent(&tl), "symmetric and antisymmetric together is refused");
  tl.antisymmetric = false;
  check(fc_tensorial_flags_consistent(&tl), "symmetric alone is fine");

  static fc_modulation m;
  memset(&m, 0, sizeof m);
  fc_base_init(&m.base);
  m.real.val = fc_rat(3, 2);
  m.bounds.lo = fc_rat(1, 1);
  m.bounds.hi = fc_rat(2, 1);
  check(fc_tensorial_push(&tl, &m) == FC_OK, "MODULATION pushed");

  m.real.val = fc_rat(1, 2);
  m.bounds.lo = fc_rat(0, 1);
  m.bounds.hi = fc_rat(5, 1);
  fc_tensorial_push(&tl, &m);

  static fc_tensor tn;
  fc_tensor_init(&tn);
  check(fc_tensor_add_covariant(&tn, &tl) == FC_OK, "covariant added");
  int co, cv;
  fc_tensor_rank(&tn, &co, &cv);
  check(co == 0 && cv == 1, "rank is (contravariant, covariant)");

  static fc_modulation out;
  check(fc_tensor_eval(&tn, &out) == FC_OK, "CALCULUS eval returns a MODULATION");
  check(fc_rat_eq(out.real.val, fc_rat(2, 1)), "reals sum exactly: 3/2 + 1/2 = 2");
  check(fc_rat_eq(out.bounds.lo, fc_rat(0, 1)) && fc_rat_eq(out.bounds.hi, fc_rat(5, 1)),
        "bounds widen to cover every contributor");
}

static void test_trees(void) {
  printf("BBST / TST / ORIGO\n");
  static fc_bbst t;
  fc_bbst_init(&t);
  /* Sizes matter here: see the note on fc_bbst_node. */

  for (int i = 0; i < 15; ++i) {
    int64_t key[FC_KD] = { i % 5, i % 3, i };
    check(fc_bbst_insert(&t, key, i) == FC_OK, "kd insert");
  }
  check(fc_bbst_build(&t) == FC_OK, "kd build");
  /* Balanced by median split: 15 nodes should not exceed depth 4. */
  check(fc_bbst_depth(&t) <= 4, "median split keeps it balanced");
  /* Keys are (i%5, i%3, i). i=2 gives (2,2,2); (2,2,7) was never inserted,
   * since 7%3 is 1 - the first version of this probe asserted a key that does
   * not exist and the suite was right to fail it. */
  int64_t probe[FC_KD] = { 2, 2, 2 };
  check(fc_bbst_find(&t, probe) >= 0, "find locates an inserted key");
  int64_t absent[FC_KD] = { 99, 99, 99 };
  check(fc_bbst_find(&t, absent) < 0, "and reports an absent one");

  /* Determinism: the same insertions build the same tree. */
  static fc_bbst t2;
  fc_bbst_init(&t2);
  for (int i = 0; i < 15; ++i) {
    int64_t key[FC_KD] = { i % 5, i % 3, i };
    fc_bbst_insert(&t2, key, i);
  }
  fc_bbst_build(&t2);
  check(t.root == t2.root && fc_bbst_depth(&t) == fc_bbst_depth(&t2),
        "the build is deterministic");

  /* Minkowski classification, exact - so the null case is actually reachable,
   * which it would not be in floating point. */
  fc_origo timelike = { fc_rat(2, 1), fc_rat(1, 1), fc_rat(0, 1), fc_rat(0, 1) };
  fc_origo null = { fc_rat(1, 1), fc_rat(1, 1), fc_rat(0, 1), fc_rat(0, 1) };
  fc_origo spacelike = { fc_rat(1, 1), fc_rat(2, 1), fc_rat(0, 1), fc_rat(0, 1) };
  check(fc_origo_classify(&timelike, true) == FC_INTERVAL_TIMELIKE, "timelike");
  check(fc_origo_classify(&null, true) == FC_INTERVAL_NULL, "null - reachable exactly");
  check(fc_origo_classify(&spacelike, true) == FC_INTERVAL_SPACELIKE, "spacelike");
  /* The fourth class comes first: poor provenance is its own answer, not a
   * guess at one of the other three. */
  check(fc_origo_classify(&timelike, false) == FC_INTERVAL_INDETERMINATE,
        "indeterminate when provenance is insufficient");

  static fc_tst tst;
  fc_tst_init(&tst);
  fc_origo root_o = { fc_rat(0, 1), fc_rat(0, 1), fc_rat(0, 1), fc_rat(0, 1) };
  fc_origo child_o = { fc_rat(1, 1), fc_rat(2, 1), fc_rat(0, 1), fc_rat(0, 1) };
  int root = fc_tst_add(&tst, &root_o);
  int kid = fc_tst_add(&tst, &child_o);
  check(root == 0 && kid == 1, "TST nodes added");
  check(fc_tst_link(&tst, root, 1, kid) == FC_OK, "linked as the middle child");

  fc_origo abs_o;
  check(fc_tst_absolute_origo(&tst, kid, &abs_o) == FC_OK, "absolute ORIGO folds the path");
  check(fc_rat_eq(abs_o.x, fc_rat(2, 1)), "and sums the deltas");

  /* The three leaves are strictly ordered and each stage needs the one before
   * it: a POST leaf with no PERI is malformed, not partial. */
  fc_iteration it;
  check(fc_tst_iterate(&tst, root, &it) == FC_OK, "iteration reports");
  check(!it.frame && !it.volumetric && !it.animated, "no leaves, no stages");

  int64_t k0[FC_KD] = { 0, 0, 0 };
  fc_bbst_insert(&tst.node[root].leaf[FC_LEAF_PRE], k0, 0);
  fc_tst_iterate(&tst, root, &it);
  check(it.frame && !it.volumetric, "PRE alone gives a frame and no more");
  fc_bbst_insert(&tst.node[root].leaf[FC_LEAF_POST], k0, 0);
  fc_tst_iterate(&tst, root, &it);
  check(!it.animated, "POST without PERI does not animate: the order is strict");
  fc_bbst_insert(&tst.node[root].leaf[FC_LEAF_PERI], k0, 0);
  fc_tst_iterate(&tst, root, &it);
  check(it.frame && it.volumetric && it.animated, "all three stages, in order");
}

int main(void) {
  test_rational();
  test_basis();
  test_num();
  test_number();
  test_lehmer();
  test_imaginary();
  test_data_and_units();
  test_tensor();
  test_trees();
  printf("\nIMAG profile: %d trifectas -> %d basis elements (%s)\n",
         FC_IMAG_TRIFECTAS, FC_IMAG_COUNT,
         fc_imag_init() == FC_OK ? "closes" : "DOES NOT CLOSE");
  printf("topology levels: %d min, %d max\n", FC_LEVELS_MIN, FC_LEVELS_MAX);
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
