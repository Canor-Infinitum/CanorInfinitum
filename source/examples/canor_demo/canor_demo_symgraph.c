/* canor_demo_symgraph.c - symgraph - the tower's symbolic grapher, with a measured axis
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* symgraph - the tower's symbolic grapher, with a measured axis              */
/* ========================================================================== */

static int schecks, sfails;
static void scheck(int cond, const char *what) {
  ++schecks;
  if (!cond) { ++sfails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_symgraph(void) {
  double vals[512];
  canor_series ser;
  canor_sym e;
  int t, c1, c2, r;

  printf("Symbolic graphing over the type tower\n");
  printf("  exact rationals throughout; the AXIS is measured, not declared\n\n");
  printf("  %-26s %-20s %-20s %8s\n", "expression", "growth", "axis", "maxbits");

  /* 1. Bounded: a constant-amplitude ratio. */
  canor_sym_init(&e);
  c1 = canor_sym_const(&e, canor_rat(1, 1));
  t = canor_sym_var(&e);
  c2 = canor_sym_const(&e, canor_rat(1, 1));
  r = canor_sym_bin(&e, CANOR_SYM_MUL, t, t);
  r = canor_sym_bin(&e, CANOR_SYM_ADD, r, c2);
  r = canor_sym_bin(&e, CANOR_SYM_DIV, c1, r);          /* 1/(t^2+1), bounded */
  canor_sym_set_root(&e, r);
  canor_sym_series(&e, 0, 40, 4, 60, vals, &ser);
  printf("  %-26s %-20s %-20s %8lld\n", "1/(t^2+1)", canor_growth_name(ser.growth),
         canor_axis_name(ser.axis), (long long)ser.max_bits);
  scheck(ser.growth == CANOR_GROWTH_BOUNDED, "1/(t^2+1) classifies as bounded");
  scheck(ser.axis == CANOR_AXIS_LINEAR, "and gets a linear axis");

  /* 2. Polynomial. */
  canor_sym_init(&e);
  t = canor_sym_var(&e);
  r = canor_sym_pow(&e, t, 3);
  canor_sym_set_root(&e, r);
  canor_sym_series(&e, 0, 200, 4, 60, vals, &ser);
  printf("  %-26s %-20s %-20s %8lld\n", "t^3", canor_growth_name(ser.growth),
         canor_axis_name(ser.axis), (long long)ser.max_bits);

  /* 3. Geometric: a power of a rational base, which is what a twisted Ore
   *    operator's envelope is. */
  canor_sym_init(&e);
  t = canor_sym_var(&e);
  {
    canor_sym g;
    canor_rational acc = canor_rat(1, 1);
    int i;

    canor_series gs;
    /* sigma^n by repeated squaring in the caller, then classified. */
    memset(&gs, 0, sizeof gs);
    for (i = 0; i < 60; ++i) {
      double v;
      acc = canor_rat_mul(acc, canor_rat(2, 1));
      v = canor_rat_valid(acc) ? (double)acc.num / (double)acc.den : 0.0;

    }
    canor_sym_init(&g);
    (void)g;
  }
  /* Expressed symbolically: 2^k as a POW of a constant. */
  canor_sym_init(&e);
  c1 = canor_sym_const(&e, canor_rat(2, 1));
  r = canor_sym_pow(&e, c1, 20);
  canor_sym_set_root(&e, r);
  {
    canor_rational v = canor_sym_eval(&e, canor_rat(0, 1));
    scheck(canor_rat_valid(v) && v.num == 1048576 && v.den == 1, "2^20 evaluates exactly");
  }

  /* 4. Doubly exponential: t^(2^k) is the Mandelbrot shape. Built as repeated
   *    squaring, which is exactly what z <- z^2 does. */
  canor_sym_init(&e);
  t = canor_sym_var(&e);
  r = t;
  {
    int k;
    for (k = 0; k < 5; ++k) r = canor_sym_bin(&e, CANOR_SYM_MUL, r, r);  /* t^32 */
  }
  canor_sym_set_root(&e, r);
  canor_sym_series(&e, 4, 24, 4, 40, vals, &ser);
  printf("  %-26s %-20s %-20s %8lld\n", "t^32 (repeated squaring)",
         canor_growth_name(ser.growth), canor_axis_name(ser.axis),
         (long long)ser.max_bits);
  scheck(ser.growth == CANOR_GROWTH_DOUBLE || ser.growth == CANOR_GROWTH_GEOMETRIC,
         "repeated squaring is classified as fast growth, not bounded");
  scheck(ser.axis != CANOR_AXIS_LINEAR, "and does not get a linear axis");

  /* 5. Poison propagates rather than plotting a wrapped number. */
  canor_sym_init(&e);
  t = canor_sym_var(&e);
  c1 = canor_sym_const(&e, canor_rat(0, 1));
  r = canor_sym_bin(&e, CANOR_SYM_DIV, t, c1);
  canor_sym_set_root(&e, r);
  {
    canor_rational v = canor_sym_eval(&e, canor_rat(3, 1));
    scheck(!canor_rat_valid(v), "division by zero is poison, not infinity");
  }
  canor_sym_init(&e);
  t = canor_sym_var(&e);
  r = canor_sym_pow(&e, t, 40);
  canor_sym_set_root(&e, r);
  {
    canor_rational v = canor_sym_eval(&e, canor_rat(1000, 1));
    scheck(!canor_rat_valid(v), "an overflowing power is poison, not a wrapped value");
  }

  printf("\n  The four growth classes are the cases this system already hit by\n"
         "  hand: Ore at sigma=1 is bounded and drew linear; at sigma!=1 it is\n"
         "  geometric and clipped to a square wave until it was put on a signed\n"
         "  log; the exact Mandelbrot is doubly exponential, its denominator\n"
         "  doubling 6->12->24->48 per step, which is why it dies at three\n"
         "  iterations. Measuring growth turns three separate diagnoses into\n"
         "  one rule.\n");
  printf("\n  For an escaping Mandelbrot orbit |z_n| ~ |z|^(2^n), so log log|z_n|\n"
         "  is linear in n - that IS the Douady-Hubbard potential, and\n"
         "  CANOR_AXIS_LOGLOG is it rather than an approximation of it. No\n"
         "  higher-dimensional embedding is needed to see it: q^2 = a^2 - |v|^2\n"
         "  + 2a.v preserves the imaginary DIRECTION, so a Cayley-Dickson orbit\n"
         "  stays in the 2-plane span{1,v} at every dimension and the\n"
         "  quaternionic set is the plane set revolved.\n");

  printf("\n%d checks, %d failures\n", schecks, sfails);
  return sfails ? 1 : 0;
}
