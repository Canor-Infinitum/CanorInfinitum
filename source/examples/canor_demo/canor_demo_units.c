/* canor_demo_units.c - units - unital pairs, and the two errors they would have caught
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* units - unital pairs, and the two errors they would have caught            */
/* ========================================================================== */

static int uchecks, ufails;
static void ucheck(int cond, const char *what) {
  ++uchecks;
  if (!cond) { ++ufails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_units(void) {
  char b1[64], b2[64], b3[64];

  printf("Unital pairs: every quantity carries a dimension\n\n");

  canor_dim_format(canor_dim_ms(), b1, sizeof b1);
  canor_dim_format(canor_dim_ms_per_step(), b2, sizeof b2);
  canor_dim_format(canor_dim_ratio(), b3, sizeof b3);
  printf("  latency            %s\n", b1);
  printf("  latency gradient   %s   <- the universal one\n", b2);
  printf("  jitter (CV)        %s\n", b3);
  canor_dim_format(canor_dim_log2_milli(), b1, sizeof b1);
  canor_dim_format(canor_dim_order_milli(), b2, sizeof b2);
  printf("  log2, milli        %s\n", b1);
  printf("  ORDER, milli       %s   <- log OF log\n", b2);

  /* --- THE TEST THAT MATTERS -------------------------------------------
   * Both real errors in this codebase were the same shape: subtracting a
   * log-of-log from a log2, both stored as bare int64 milli-units, both
   * silently producing a plausible number. If the dimension system does not
   * refuse this, it is not worth having. */
  printf("\n  the two errors this exists to catch\n");
  {
    canor_quantity order = canor_q(13937, canor_dim_order_milli());   /* Kapprox order */
    canor_quantity lg    = canor_q(9490, canor_dim_log2_milli());     /* log2(n!) */
    canor_quantity bad   = canor_q_sub(order, lg);
    canor_dim_format(bad.dim, b1, sizeof b1);
    printf("    order_milli - fact_milli  ->  %s   (value would have been %lld)\n",
           b1, (long long)bad.value);
    ucheck(!canor_q_ok(bad),
           "log-of-log minus log2 is REFUSED - the Kapprox and L_m error");
  }
  {
    /* The same subtraction with matching levels must go through, or the
     * checker is simply rejecting everything. */
    canor_quantity a = canor_q(9490, canor_dim_log2_milli());
    canor_quantity b = canor_q(3319, canor_dim_log2_milli());
    canor_quantity good = canor_q_sub(a, b);
    ucheck(canor_q_ok(good) && good.value == 6171,
           "log2 minus log2 is allowed and correct");
  }
  {
    /* Scale: milli against unit. Both int64, both 'log2', different scale. */
    canor_quantity milli = canor_q(9490, canor_dim_log2_milli());
    canor_quantity unit  = canor_q(9, canor_dim_log2());
    canor_quantity bad   = canor_q_add(milli, unit);
    ucheck(!canor_q_ok(bad), "milli-units plus unit-units is REFUSED");
  }
  {
    /* Latency: a gradient may not be added to a latency. */
    canor_quantity lat  = canor_q(80, canor_dim_ms());
    canor_quantity grad = canor_q(3, canor_dim_ms_per_step());
    ucheck(!canor_q_ok(canor_q_add(lat, grad)),
           "a latency gradient plus a latency is REFUSED");
    ucheck(canor_q_ok(canor_q_add(lat, canor_q(20, canor_dim_ms()))),
           "latency plus latency is allowed");
  }
  {
    /* And the gradient must be CONSTRUCTIBLE the obvious way: ms divided by
     * steps. If the algebra did not produce it, the named dimension would be
     * a label rather than a derivation. */
    canor_dim g = canor_dim_div(canor_dim_ms(), canor_dim_make(0, 1, 0, 0, 0));
    ucheck(canor_dim_equal(g, canor_dim_ms_per_step()),
           "ms / step really is the latency gradient dimension");
  }
  {
    /* Taking a log raises the level, so the ladder's own construction is
     * type-correct by the same rule that refuses the error. */
    canor_dim once = canor_dim_take_log(canor_dim_make(0, 1, 0, 0, 0));
    canor_dim twice = canor_dim_take_log(once);
    ucheck(once.log == 1 && twice.log == 2,
           "log2 then log2 again gives level 2 - an ORDER");
    ucheck(!canor_dim_equal(once, twice), "and level 1 is not level 2");
    ucheck(!canor_q_ok(canor_q_add(canor_q(1, once), canor_q(1, twice))),
           "so adding them is refused by construction, not by a special case");
  }
  {
    /* Multiplication is permissive where addition is strict - that asymmetry
     * is the point, since products of unlike quantities are meaningful. */
    canor_dim r = canor_dim_mul(canor_dim_ms(), canor_dim_make(0, -1, 0, 0, 0));
    ucheck(canor_dim_ok(r) && canor_dim_equal(r, canor_dim_ms_per_step()),
           "ms times step^-1 is allowed and is the gradient");
    ucheck(!canor_dim_ok(canor_dim_mul(canor_dim_log2(), canor_dim_log2())),
           "but a product of two logs is refused - it is a log of nothing");
  }
  {
    /* Poison propagates, exactly like the rational poison. */
    canor_quantity bad = canor_q_sub(canor_q(1, canor_dim_order_milli()),
                               canor_q(1, canor_dim_log2_milli()));
    canor_quantity later = canor_q_mul(bad, canor_q(2, canor_dim_none()));
    ucheck(!canor_q_ok(later), "poison survives a later multiplication");
  }

  /* --- c, as a convention ------------------------------------------------ */
  printf("\n  physical constants\n");
  {
    canor_quantity c = canor_const_c(), cms = canor_const_c_per_ms();
    canor_quantity lat = canor_q(80, canor_dim_ms());
    canor_dim_format(c.dim, b1, sizeof b1);
    canor_dim_format(cms.dim, b2, sizeof b2);
    printf("    c        = %lld  %s\n", (long long)c.value, b1);
    printf("    c        = %lld  %s   (same constant, per ms)\n",
           (long long)cms.value, b2);
    printf("    exact by the SI definition of the metre - a definition, not a\n"
           "    measurement, so there is no uncertainty to carry.\n");
    ucheck(c.value == 299792458LL, "c is 299792458 exactly");
    ucheck(canor_q_ok(c), "and carries a valid dimension");
    ucheck(!canor_q_ok(canor_q_add(c, lat)),
           "c plus a latency is REFUSED - velocity is not a duration");
    ucheck(!canor_q_ok(canor_q_add(c, cms)),
           "and per-second plus per-millisecond is REFUSED - a 1000x error");
    {
      /* Distance = c . time. The dimension must come out in metres, or the
       * constant is decorative rather than usable. */
      canor_quantity d = canor_q_mul(c, canor_q(1, canor_dim_second()));
      canor_dim_format(d.dim, b3, sizeof b3);
      printf("    c x 1 s  = %lld  %s\n", (long long)d.value, b3);
      ucheck(canor_dim_equal(d.dim, canor_dim_metre()),
             "c times a second is a LENGTH - the algebra produces it");
    }
    ucheck(canor_dim_equal(canor_dim_div(canor_dim_metre(), canor_dim_second()),
                        canor_dim_velocity()),
           "m / s really is the velocity dimension");
    printf("\n    FUTURE SCOPE: a Bekenstein-style derivation needs energy and\n"
           "    action as well - I <= 2.pi.R.E/(hbar.c.ln2) is dimensionally\n"
           "    BIT = LENGTH.ENERGY/(ACTION.VELOCITY), and the bit dimension is\n"
           "    already here. What is missing is any measurement path: nothing\n"
           "    in this system measures distance, so no telemetry has yet\n"
           "    earned the right to check this constant. It is a CONVENTION.\n");
  }

  printf("\n  Multiplication is permissive, addition is strict. That asymmetry\n"
         "  is deliberate: products of unlike quantities are meaningful and\n"
         "  sums of them are not, and every error this catches was a sum.\n");

  printf("\n%d checks, %d failures\n", uchecks, ufails);
  return ufails ? 1 : 0;
}
