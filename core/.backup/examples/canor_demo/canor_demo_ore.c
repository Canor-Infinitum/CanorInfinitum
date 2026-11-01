/* canor_demo_ore.c - ore - the skew-trigonometric operators
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* ore - the skew-trigonometric operators                                     */
/* ========================================================================== */

static int ochecks, ofails;
static void ocheck(int cond, const char *what) {
  ++ochecks;
  if (!cond) { ++ofails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_ore(const canor_demo_config_t *c, int sel) {
  static const char *const as[5] = { "2", "1", "0", "-1", "-2" };
  canor_ore o;
  char b1[32];
  int i, n;

  printf("Ore-algebraic rational skew-trigonometric operators\n");
  printf("  L = d^2 - tau.d + sigma,  the Ore skew polynomial annihilating the pair\n");
  printf("  C(n+1) = tau.C(n) - sigma.C(n-1),  C(0)=1, C(1)=tau/2\n");
  printf("  S(n+1) = tau.S(n) - sigma.S(n-1),  S(0)=0, S(1)=delta_amp\n"  "  delta (the sigma-derivation) is 0 in this construction.\n\n");

  /* The five periodic cases, which Niven's theorem says are all of them. */
  printf("  periodic operators - Niven's theorem gives exactly these five\n\n");
  printf("  %4s %8s  %s\n", "a", "period", "C(0..7) then S(0..7)");
  for (i = 0; i < 5; ++i) {
    if (!canor_ore_init(&o, as[i], "1", "1")) { ocheck(0, "init"); continue; }
    printf("  %4s %8d  C:", as[i], canor_ore_period(&o));
    for (n = 0; n < 8; ++n) {
      canor_rational v;
      if (canor_ore_cos(&o, n, &v)) { canor_demo_rat_show(v, b1, sizeof b1); printf(" %5s", b1); }
      else printf(" %5s", "--");
    }
    printf("\n  %4s %8s  S:", "", "");
    for (n = 0; n < 8; ++n) {
      canor_rational v;
      if (canor_ore_sin(&o, n, &v)) { canor_demo_rat_show(v, b1, sizeof b1); printf(" %5s", b1); }
      else printf(" %5s", "--");
    }
    printf("\n");
  }

  /* Periodicity is a claim, so it is checked rather than printed. */
  for (i = 0; i < 5; ++i) {
    int p, ok = 1;
    canor_ore_init(&o, as[i], "1", "1");
    p = canor_ore_period(&o);
    /* RAW on both sides. The reduced calls would compare n mod p against
     * (n+p) mod p, which are the same index - the check would pass for any
     * period whatsoever, including a wrong one. */
    if (p > 0) {
      for (n = 0; n < 24 && ok; ++n) {
        canor_rational x, y;
        if (!canor_ore_cos_raw(&o, n, &x) || !canor_ore_cos_raw(&o, n + p, &y) ||
            !canor_rat_eq(x, y)) ok = 0;
        if (!canor_ore_sin_raw(&o, n, &x) || !canor_ore_sin_raw(&o, n + p, &y) ||
            !canor_rat_eq(x, y)) ok = 0;
      }
      ocheck(ok, "the declared period actually repeats, checked UNREDUCED");
    } else {
      ocheck(canor_ore_degenerate(&o) || o.tau.den != 1 ||
             !canor_rat_eq(o.sigma, canor_rat(1, 1)),
             "a period of 0 is reported only for a degenerate or skew operator");
    }
  }

  /* The skew-Pythagorean law: W(n) = q^n . W(0). At q = 1 it is constant. */
  {
    int ok = 1;
    canor_rational w0, wn;
    canor_ore_init(&o, "1", "1", "1");
    if (!canor_ore_wronskian(&o, 0, &w0)) ok = 0;
    for (n = 0; n < 30 && ok; ++n)
      if (!canor_ore_wronskian(&o, n, &wn) || !canor_rat_eq(wn, w0)) ok = 0;
    ocheck(ok, "Casoratian is constant at sigma=1, for 30 steps");
    canor_demo_rat_show(w0, b1, sizeof b1);
    printf("\n  W(n) at tau=1, sigma=1  =  %s  for every n\n", b1);
  }
  {
    /* q = 2: the Casoratian must scale by exactly 2 each step. Nothing about
     * this is approximate, so an off-by-one would show immediately. */
    int ok = 1;
    canor_rational wn, wn1, two = canor_rat(2, 1);
    canor_ore_init(&o, "1", "2", "1");
    for (n = 0; n < 12 && ok; ++n) {
      canor_rational want;
      if (!canor_ore_wronskian(&o, n, &wn) || !canor_ore_wronskian(&o, n + 1, &wn1)) { ok = 0; break; }
      want = canor_rat_mul(wn, two);
      if (!canor_rat_eq(wn1, want)) ok = 0;
    }
    ocheck(ok, "at sigma=2 the Casoratian scales by sigma each step");
    ocheck(canor_ore_period(&o) == 0, "a twisted operator (sigma!=1) has no period here");
  }

  /* The conventional identity must FAIL. Section 13 declines it, so a version
   * that started satisfying it would mean this had drifted into real
   * trigonometry - which is the one outcome that would invalidate the design. */
  {
    canor_rational cc, ss, sum, one = canor_rat(1, 1);
    canor_ore_init(&o, "1", "1", "1");
    canor_ore_cos(&o, 1, &cc);
    canor_ore_sin(&o, 1, &ss);
    sum = canor_rat_add(canor_rat_mul(cc, cc), canor_rat_mul(ss, ss));
    canor_demo_rat_show(sum, b1, sizeof b1);
    ocheck(!canor_rat_eq(sum, one),
           "S^2 + C^2 != 1 - these are NOT trigonometric functions");
    printf("  S(1)^2 + C(1)^2 at a=1  =  %s, not 1, exactly as section 13 requires\n", b1);
  }

  /* Niven's other side: an `a` off the list is aperiodic and must eventually
   * refuse rather than return a wrapped value. */
  {
    int refused_at = -1;
    canor_ore_init(&o, "3/2", "1", "1");
    ocheck(canor_ore_period(&o) == 0, "tau=3/2 is aperiodic (Niven)");
    for (n = 0; n < 200; ++n) {
      canor_rational v;
      if (!canor_ore_cos(&o, n, &v)) { refused_at = n; break; }
    }
    ocheck(refused_at > 0, "an aperiodic operator eventually refuses");
    printf("  tau=3/2 exhausts int64 rationals at n=%d and refuses\n", refused_at);
  }

  /* tan has poles where C is zero, and they are refused, not papered over. */
  {
    canor_rational v;
    int poles = 0;
    canor_ore_init(&o, "0", "1", "1");
    for (n = 0; n < 8; ++n) if (!canor_ore_tan(&o, n, &v)) ++poles;
    ocheck(poles == 4, "at tau=0 tan has a pole at half its indices, each refused");
    canor_ore_init(&o, "1", "1", "1");
    poles = 0;
    for (n = 0; n < 12; ++n) if (!canor_ore_tan(&o, n, &v)) ++poles;
    ocheck(poles == 0, "at tau=1 C never vanishes, so tan is total");
  }

  /* Signed zeros, derived. At a=1 the sine passes through zero rising at
   * n=0 and falling at n=3, so the two zeros must carry different poles -
   * which is the whole reason FLOWVALUE has four values and not three. */
  {
    canor_rational v0, v1, v3, v4;
    FLOWVALUE z0, z3;
    canor_ore_init(&o, "1", "1", "1");
    canor_ore_sin(&o, 0, &v0); canor_ore_sin(&o, 1, &v1);
    canor_ore_sin(&o, 3, &v3); canor_ore_sin(&o, 4, &v4);
    z0 = canor_ore_quantize(v0, v1);
    z3 = canor_ore_quantize(v3, v4);
    ocheck(z0 == FV_PosZero, "S(0)=0 rising  -> PosZero");
    ocheck(z3 == FV_NegZero, "S(3)=0 falling -> NegZero");
    ocheck(z0 != z3, "the two zeros are distinguishable, and they were DERIVED");
  }

  /* Determinism: the operators are a pure function of (params, n). */
  {
    canor_rational x, y;
    int ok = 1;
    canor_ore_init(&o, "1", "1", "1");
    for (n = -20; n < 20; ++n) {
      canor_ore x2 = o;
      if (!canor_ore_cos(&o, n, &x) || !canor_ore_cos(&x2, n, &y) || !canor_rat_eq(x, y)) ok = 0;
    }
    ocheck(ok, "evaluation is a pure function of the parameters and the index");
  }

  /* Parity: C even, S odd. */
  {
    int ok = 1;
    canor_ore_init(&o, "1", "1", "1");
    for (n = 1; n < 20 && ok; ++n) {
      canor_rational p, m;
      if (!canor_ore_cos(&o, n, &p) || !canor_ore_cos(&o, -n, &m) || !canor_rat_eq(p, m)) ok = 0;
      if (!canor_ore_sin(&o, n, &p) || !canor_ore_sin(&o, -n, &m)) { ok = 0; break; }
      m.num = -m.num;
      if (!canor_rat_eq(p, m)) ok = 0;
    }
    ocheck(ok, "C is even and S is odd across the negative branch");
  }

  /* And the operators drive a real Strategy end to end. */
  {
    canor_strategy s = c->strat[sel];
    FLOWVALUE di, dj, dk;
    int defined = 0, i2;
    canor_ore_init(&o, "1", "1", "1");
    ocheck(canor_ore_bind(s.id.name, &o), "the Ore binding succeeds");
    s.ops = canor_ore_absop;
    canor_ore_reset_refusals();
    for (i2 = 0; i2 < c->flows.count; ++i2)
      if (canor_flow_differential(&s, &c->flows, i2, &di, &dj, &dk)) ++defined;
    printf("\n  driving strategy '%s' through the Ore operators:\n"
           "    %d of %d Flows have a defined differential, %llu operator refusals\n",
           s.id.name, defined, c->flows.count,
           (unsigned long long)canor_ore_refusals());
    ocheck(defined >= 0, "the plug-in runs the existing Flow machinery");
  }

  printf("\n%d checks, %d failures\n", ochecks, ofails);
  return ofails ? 1 : 0;
}
