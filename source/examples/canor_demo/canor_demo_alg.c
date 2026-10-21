/* canor_demo_alg.c - alg - algebraic numbers: the irrational roots, exactly
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* alg - algebraic numbers: the irrational roots, exactly                     */
/* ========================================================================== */

static int achecks, afails;
static void acheck(int cond, const char *what) {
  ++achecks;
  if (!cond) { ++afails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_alg(void) {
  canor_poly p;
  canor_rational co[CANOR_POLY_MAX];
  canor_alg roots[CANOR_ALG_MAX_ROOTS];
  int found = 0, i;

  printf("Algebraic numbers: (minimal polynomial, isolating interval)\n");
  printf("  No radicals, no floats, no surreals. Sturm counts the roots;\n");
  printf("  bisection narrows them; the pair names each one exactly.\n\n");

  /* x^2 - 2: the case rational roots cannot reach. */
  canor_poly_init(&p);
  co[0] = canor_rat(-2, 1); co[1] = canor_rat(0, 1); co[2] = canor_rat(1, 1);
  canor_poly_set(&p, 2, co);
  if (!canor_alg_isolate(&p, roots, CANOR_ALG_MAX_ROOTS, &found)) return 2;
  printf("  x^2 - 2  ->  %d real roots\n", found);
  acheck(found == 2, "x^2 - 2 has exactly two real roots, counted by Sturm");
  for (i = 0; i < found; ++i) {
    canor_rational w;
    canor_alg_refine(&roots[i], 40);
    printf("    root %d  ~ %.10f   interval width %.3g\n", i,
           canor_demo_rat_d(canor_alg_approx(&roots[i], &w)), canor_demo_rat_d(w));
  }
  if (found == 2) {
    canor_rational w, v;
    v = canor_alg_approx(&roots[1], &w);                  /* ascending: last is + */
    acheck(canor_demo_rat_d(v) > 1.41421355 && canor_demo_rat_d(v) < 1.41421357,
           "the positive root is sqrt(2) to eight places");
    /* The achieved width is asserted, not an assumed one: int64 rational
     * bisection stops when the denominators exhaust the type, and the honest
     * claim is about what was reached rather than what was requested. */
    acheck(canor_demo_rat_d(w) < 1e-6, "and the interval was genuinely narrowed");
    printf("    achieved width %.3g  (int64 bisection ceiling, not an error)\n",
           canor_demo_rat_d(w));
  }

  /* The quartic the rational-root pass leaves behind. */
  canor_poly_init(&p);
  co[0] = canor_rat(-1, 1); co[1] = canor_rat(0, 1); co[2] = canor_rat(0, 1);
  co[3] = canor_rat(0, 1); co[4] = canor_rat(1, 1);
  canor_poly_set(&p, 4, co);
  canor_alg_isolate(&p, roots, CANOR_ALG_MAX_ROOTS, &found);
  printf("\n  x^4 - 1  ->  %d real roots (the other two are complex)\n", found);
  acheck(found == 2, "x^4 - 1 has two REAL roots; Sturm does not count complex ones");

  /* A quartic with four distinct irrational reals. */
  canor_poly_init(&p);
  co[0] = canor_rat(1, 1); co[1] = canor_rat(0, 1); co[2] = canor_rat(-5, 1);
  co[3] = canor_rat(0, 1); co[4] = canor_rat(1, 1);
  canor_poly_set(&p, 4, co);
  canor_alg_isolate(&p, roots, CANOR_ALG_MAX_ROOTS, &found);
  printf("\n  x^4 - 5x^2 + 1  ->  %d real roots\n", found);
  acheck(found == 4, "a quartic with four distinct real roots yields four");
  for (i = 0; i < found; ++i) {
    canor_rational w;
    canor_alg_refine(&roots[i], 44);
    printf("    root %d  ~ %.10f\n", i, canor_demo_rat_d(canor_alg_approx(&roots[i], &w)));
  }
  {
    /* Each isolating interval must hold its root and nothing else - checked by
     * evaluating the polynomial at both ends and requiring a sign change. */
    int ok = 1;
    for (i = 0; i < found; ++i) {
      canor_rational a = canor_poly_eval(&roots[i].min, roots[i].lo);
      canor_rational b = canor_poly_eval(&roots[i].min, roots[i].hi);
      if (a.num == 0 || b.num == 0) continue;
      if ((a.num > 0) == (b.num > 0)) ok = 0;
    }
    acheck(ok, "every isolating interval brackets a genuine sign change");
    ok = 1;
    for (i = 1; i < found; ++i)
      if (canor_rat_cmp(roots[i].lo, roots[i - 1].hi) < 0) ok = 0;
    acheck(ok, "and the intervals are disjoint, so no root is counted twice");
  }

  /* Arithmetic in Q(sqrt 2): (1 + a)^2 must be 3 + 2a, exactly. */
  printf("\n  arithmetic in Q(alpha), alpha = sqrt(2)\n");
  {
    canor_alg f;
    canor_alg_elem one_plus_a, sq;
    canor_poly_init(&p);
    co[0] = canor_rat(-2, 1); co[1] = canor_rat(0, 1); co[2] = canor_rat(1, 1);
    canor_poly_set(&p, 2, co);
    canor_alg_isolate(&p, roots, CANOR_ALG_MAX_ROOTS, &found);
    f = roots[found - 1];                 /* ascending, so this is +sqrt(2) */
    canor_alg_refine(&f, 40);

    canor_alg_elem_init(&one_plus_a);
    one_plus_a.c[0] = canor_rat(1, 1);
    one_plus_a.c[1] = canor_rat(1, 1);
    one_plus_a.degree = 1;
    canor_alg_elem_mul(&f, &one_plus_a, &one_plus_a, &sq);
    printf("    (1 + a)^2 = %lld + %lld a      expected 3 + 2a\n",
           (long long)sq.c[0].num, (long long)sq.c[1].num);
    acheck(sq.c[0].num == 3 && sq.c[0].den == 1 &&
           sq.c[1].num == 2 && sq.c[1].den == 1,
           "(1+a)^2 reduces to 3 + 2a modulo x^2-2 - exact field arithmetic");
    printf("    numerically  %.10f      (1+sqrt2)^2 = 5.8284271247\n",
           canor_demo_rat_d(canor_alg_elem_approx(&f, &sq)));
  }

  printf("\n  The element is EXACT; only its decimal rendering is not, and the\n"
         "  interval width says how far that rendering can be trusted. That is\n"
         "  the difference between an algebraic number and a float.\n");

  printf("\n%d checks, %d failures\n", achecks, afails);
  return afails ? 1 : 0;
}
