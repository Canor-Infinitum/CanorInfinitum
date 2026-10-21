/* canor_demo_poly.c - poly - rational roots, Pade, Mittag-Leffler
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* poly - rational roots, Pade, Mittag-Leffler                                */
/* ========================================================================== */

static int qchecks, qfails;
static void qcheck(int cond, const char *what) {
  ++qchecks;
  if (!cond) { ++qfails; printf("  FAIL  %s\n", what); }
}

static void poly_show(const canor_poly *p) {
  int i, first = 1;
  for (i = p->degree; i >= 0; --i) {
    char b[48];
    if (p->c[i].num == 0 && p->degree > 0) continue;
    canor_demo_rat_show(p->c[i], b, sizeof b);
    printf("%s%s", first ? "" : " + ", b);
    if (i > 0) printf("x^%d", i);
    first = 0;
  }
  if (first) printf("0");
}

int canor_demo_cmd_poly(void) {
  canor_poly p, res;
  canor_poly_factors f;
  canor_rational co[CANOR_POLY_MAX];
  fb_rat v;
  int i;

  printf("Rational roots over the sieve, Pade, Mittag-Leffler\n\n");
  if (!canor_prime_init(100000)) return 2;

  /* (x-1)(x+2)(x-3)(2x+1) = a quartic with four rational roots. */
  canor_poly_init(&p);
  /* (x-1)(x+2)(x-3)(2x+1) = 2x^4 - 3x^3 - 12x^2 + 7x + 6, expanded rather
   * than remembered - the first version's coefficients did not match the
   * factors they claimed, and the root finder was right where the test was
   * wrong. */
  co[0] = canor_rat(6, 1); co[1] = canor_rat(7, 1); co[2] = canor_rat(-12, 1);
  co[3] = canor_rat(-3, 1); co[4] = canor_rat(2, 1);
  canor_poly_set(&p, 4, co);
  printf("  quartic  "); poly_show(&p); printf("\n");
  canor_poly_rational_roots(&p, &f);
  printf("  rational roots:");
  for (i = 0; i < f.n; ++i) {
    char b[48];
    canor_demo_rat_show(f.root[i], b, sizeof b);
    printf(" %s%s", b, f.mult[i] > 1 ? "(x2)" : "");
  }
  printf("\n  remainder degree %d, complete %s\n",
         f.remainder_degree, f.complete ? "yes" : "no");
  qcheck(f.n >= 3, "a quartic built from rational factors yields its roots");
  {
    int allroot = 1;
    for (i = 0; i < f.n; ++i)
      if (canor_poly_eval(&p, f.root[i]).num != 0) allroot = 0;
    qcheck(allroot, "every returned root really is a root of the original");
  }

  /* x^2 - 2: no rational roots, and it must say so rather than approximate. */
  canor_poly_init(&p);
  co[0] = canor_rat(-2, 1); co[1] = canor_rat(0, 1); co[2] = canor_rat(1, 1);
  canor_poly_set(&p, 2, co);
  canor_poly_rational_roots(&p, &f);
  printf("\n  x^2 - 2  -> %d rational roots, remainder degree %d\n",
         f.n, f.remainder_degree);
  qcheck(f.n == 0 && f.remainder_degree == 2,
         "x^2-2 has NO rational root and the quadratic is handed back intact");

  /* A quartic with one rational root and an irrational remainder. */
  canor_poly_init(&p);
  co[0] = canor_rat(2, 1); co[1] = canor_rat(-2, 1); co[2] = canor_rat(1, 1);
  co[3] = canor_rat(-1, 1); co[4] = canor_rat(0, 1);
  canor_poly_set(&p, 3, co);
  canor_poly_rational_roots(&p, &f);
  printf("  x^3 - x^2 - 2x + 2 -> %d rational root(s), remainder degree %d\n",
         f.n, f.remainder_degree);
  qcheck(f.n == 1 && f.remainder_degree == 2,
         "one rational root peeled off, the irrational pair left as a quadratic");

  /* The resolvent cubic exists and is a cubic. */
  canor_poly_init(&p);
  co[0] = canor_rat(-1, 1); co[1] = canor_rat(0, 1); co[2] = canor_rat(0, 1);
  co[3] = canor_rat(0, 1); co[4] = canor_rat(1, 1);
  canor_poly_set(&p, 4, co);
  if (canor_poly_resolvent_cubic(&p, &res)) {
    printf("\n  resolvent cubic of x^4 - 1:  "); poly_show(&res); printf("\n");
    qcheck(res.degree == 3, "the resolvent of a quartic is a cubic");
  }

  /* Pade against Taylor: same function, and the Pade needs far less work. */
  printf("\n  exp(1) by two routes\n");
  {
    fb_rat pv, tv;
    canor_exp_report er;
    int sq = 0;
    int64_t fx;
    double pd = 0.0, td = 0.0;
    fb_rat_init(&pv); fb_rat_init(&tv);
    if (canor_pade_exp(canor_rat(1, 1), 6, 6, &pv, &sq) &&
        fb_rat_to_fixed(&pv.num, &pv.den, 24, &fx))
      pd = (double)fx / (double)(1 << 24);
    if (canor_exp_rat(canor_rat(1, 1), 24, &tv, &er) &&
        fb_rat_to_fixed(&tv.num, &tv.den, 24, &fx))
      td = (double)fx / (double)(1 << 24);
    printf("    Pade [6/6]   %.10f   (%d squarings)\n", pd, sq);
    printf("    Taylor x24   %.10f   (%d terms, %d squarings)\n",
           td, er.terms, er.squarings);
    printf("    e            2.7182818285\n");
    qcheck(pd > 2.718281 && pd < 2.718283, "Pade [6/6] hits e to six places");
    qcheck(td > 2.718281 && td < 2.718283, "and so does the Taylor route");
    fb_rat_free(&pv); fb_rat_free(&tv);
  }

  /* Mittag-Leffler must BE exp at alpha = beta = 1 - the identity that makes
   * it a generalisation rather than a different function. */
  printf("\n  Mittag-Leffler E(alpha,1; z)\n");
  {
    canor_ml_report mr;
    int64_t fx;
    double e11 = 0.0;
    fb_rat_init(&v);
    if (canor_mittag_leffler(canor_rat(1, 1), canor_rat(1, 1), canor_rat(1, 1), 30, &v, &mr) &&
        fb_rat_to_fixed(&v.num, &v.den, 24, &fx))
      e11 = (double)fx / (double)(1 << 24);
    printf("    E(1,1; 1) = %.6f   exp(1) = 2.718282\n", e11);
    qcheck(e11 > 2.7178 && e11 < 2.7188,
           "E(1,1;z) reproduces exp(z) - it is a strict generalisation");
    fb_rat_free(&v);
  }

  /* Fractional relaxation: the damping the PFISD loop wants. At alpha = 1 it
   * is exponential decay; below 1 it is power-law, i.e. with memory. */
  printf("\n  fractional relaxation  E(alpha,1; -t^alpha)   lambda = 1\n");
  printf("    %8s %12s %12s %12s\n", "t", "alpha=1", "alpha=1/2", "alpha=1/4");
  {
    static const int tt[4] = { 1, 2, 4, 8 };
    int j;
    for (j = 0; j < 4; ++j) {
      double vals[3] = { 0, 0, 0 };
      int ok_a[3] = { 0, 0, 0 };
      static const int an[3] = { 1, 1, 1 };
      static const int ad[3] = { 1, 2, 4 };
      int a;
      for (a = 0; a < 3; ++a) {
        canor_ml_report mr;
        int64_t fx;
        fb_rat_init(&v);
        /* 80, not 40. Gamma(alpha.k + 1) grows more slowly than k! for
         * alpha < 1, so the fractional cases need roughly twice the terms;
         * the loop stops early once it has actually converged. */
        ok_a[a] = canor_fractional_relax(canor_rat(an[a], ad[a]), canor_rat(1, 1),
                                      canor_rat(tt[j], 1), 80, &v, &mr);
        if (ok_a[a] && fb_rat_to_fixed(&v.num, &v.den, 24, &fx))
          vals[a] = (double)fx / (double)(1 << 24);
        fb_rat_free(&v);
      }
      printf("    %8d %12.6f %12.6f %12.6f\n", tt[j], vals[0], vals[1], vals[2]);
    }
  }
  /* Checked against a closed form, not against expectation: for alpha = 1/2,
   * E(1/2, 1; -x) = exp(x^2).erfc(x) exactly. */
  {
    canor_ml_report mr;
    fb_rat v2;
    int64_t fx;
    double got = 0.0, want = exp(8.0) * erfc(2.8284271);
    fb_rat_init(&v2);
    if (canor_mittag_leffler(canor_rat(1, 2), canor_rat(1, 1),
                          canor_rat(-28284271, 10000000), 80, &v2, &mr) &&
        fb_rat_to_fixed(&v2.num, &v2.den, 24, &fx))
      got = (double)fx / (double)(1 << 24);
    printf("\n  closed-form check: E(1/2,1;-2.8284271)\n");
    printf("    computed %.8f   exp(x^2).erfc(x) = %.8f   terms %d\n",
           got, want, mr.terms);
    /* 5e-5, which is what the method actually delivers rather than what
     * would have been nice. The coefficients are 1/Gamma to about nine
     * significant digits, the terms peak near 205 before cancelling down to
     * 0.19, and the accumulated absolute error across that cancellation is
     * the limit - roughly four correct significant figures. Asserting five
     * would be asserting a precision the construction does not have. */
    qcheck(got > want - 5e-5 && got < want + 5e-5,
           "the fractional path matches its closed form to ~4 figures");
    printf("    difference %.2e   (9-digit coefficients across a series that "
           "peaks at 205)\n", got - want);
    fb_rat_free(&v2);
  }

  printf("\n  L^-1{ s^(a-1)/(s^a + lambda) } = E(a,1; -lambda.t^a), so a\n"
         "  fractional pole in s IS a Mittag-Leffler response in t. At a = 1\n"
         "  that is exp(-lambda.t) and the first column shows it; below 1 the\n"
         "  decay is power-law rather than exponential, which is the memory a\n"
         "  damped PFISD loop is asking for.\n");

  printf("\n%d checks, %d failures\n", qchecks, qfails);
  return qfails ? 1 : 0;
}
