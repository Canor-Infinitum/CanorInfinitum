/* canor_demo_primes.c - primes - the sieve, the residuals, and L_m
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* primes - the sieve, the residuals, and L_m                                 */
/* ========================================================================== */

static int pchecks, pfails;
static void pcheck(int cond, const char *what) {
  ++pchecks;
  if (!cond) { ++pfails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_primes(void) {
  canor_factorisation f;
  int i;

  printf("Primes, primality residuals, and the L_m residual function\n\n");
  if (!canor_prime_init(200000)) {
    fprintf(stderr, "follow: sieve allocation failed\n");
    return 2;
  }
  printf("  Euler linear sieve to 200000: %d primes\n", canor_prime_count());
  printf("  first ten:");
  for (i = 0; i < 10; ++i) printf(" %d", canor_prime_at(i));
  printf("\n");
  pcheck(canor_prime_count() == 17984, "pi(200000) = 17984");
  pcheck(canor_prime_at(0) == 2 && canor_prime_at(9) == 29, "the first ten are right");
  /* Cross-check the sieve against trial division rather than against a
   * remembered fact - I asserted 199999 composite here without checking, and
   * the sieve disagreed. It was right. */
  {
    int ok = 1;
    int32_t n3;
    for (n3 = 2; n3 <= 5000 && ok; ++n3) {
      int32_t d;
      int isp = 1;
      for (d = 2; (int64_t)d * d <= n3; ++d) if (n3 % d == 0) { isp = 0; break; }
      if (isp != (canor_is_prime(n3) ? 1 : 0)) ok = 0;
    }
    pcheck(ok, "the sieve agrees with trial division for every n <= 5000");
  }
  pcheck(canor_is_prime(104729), "104729 is the 10000th prime");

  /* Each composite is crossed off exactly once: verified by checking that the
   * smallest-factor table really holds the SMALLEST factor. */
  {
    int ok = 1;
    int32_t n2;
    for (n2 = 2; n2 <= 20000 && ok; ++n2) {
      int32_t sp = canor_smallest_factor(n2), d;
      if (sp <= 0 || n2 % sp != 0) { ok = 0; break; }
      for (d = 2; d < sp; ++d) if (n2 % d == 0) { ok = 0; break; }
    }
    pcheck(ok, "spf[n] is genuinely the smallest prime factor, for all n <= 20000");
  }

  printf("\n  factorisation via the spf table\n");
  {
    static const int32_t sample[4] = { 60, 5040, 65536, 123456 };
    for (i = 0; i < 4; ++i) {
      int k;
      canor_factorise(sample[i], &f);
      printf("    %8ld =", (long)sample[i]);
      for (k = 0; k < f.n; ++k)
        printf("%s %ld^%ld", k ? " ." : "", (long)f.p[k], (long)f.e[k]);
      printf("\n");
    }
    canor_factorise(60, &f);
    pcheck(f.n == 3 && f.p[0] == 2 && f.e[0] == 2 && f.p[2] == 5,
           "60 = 2^2 . 3 . 5");
  }

  printf("\n  %8s %8s %8s %8s %8s %8s\n", "n", "omega", "Omega", "phi", "mu", "rad");
  {
    static const int32_t ns[6] = { 12, 30, 36, 97, 210, 1024 };
    for (i = 0; i < 6; ++i)
      printf("  %8ld %8ld %8ld %8ld %8d %8ld\n", (long)ns[i],
             (long)canor_omega(ns[i]), (long)canor_bigomega(ns[i]),
             (long)canor_totient(ns[i]), canor_mobius(ns[i]), (long)canor_radical(ns[i]));
  }
  pcheck(canor_totient(36) == 12, "phi(36) = 12");
  pcheck(canor_totient(97) == 96, "phi(p) = p-1 for prime p");
  pcheck(canor_mobius(30) == -1, "mu(30) = -1, three distinct primes");
  pcheck(canor_mobius(12) == 0, "mu(12) = 0, squareful");
  pcheck(canor_radical(1024) == 2, "rad(2^10) = 2");
  /* phi is multiplicative on coprime arguments - a property, not a spot value. */
  {
    int ok = 1;
    int32_t a, b;
    for (a = 2; a <= 40 && ok; ++a)
      for (b = 2; b <= 40; ++b) {
        int32_t g = a, h = b, t;
        while (h) { t = g % h; g = h; h = t; }
        if (g != 1) continue;
        if (canor_totient(a * b) != canor_totient(a) * canor_totient(b)) { ok = 0; break; }
      }
    pcheck(ok, "phi is multiplicative on coprime pairs");
  }

  /* Legendre, checked against a directly computed factorial where one fits. */
  printf("\n  Legendre: the exponent of p in n!\n");
  printf("    v_2(20!) = %lld   v_3(20!) = %lld   v_5(100!) = %lld\n",
         (long long)canor_factorial_valuation(20, 2),
         (long long)canor_factorial_valuation(20, 3),
         (long long)canor_factorial_valuation(100, 5));
  pcheck(canor_factorial_valuation(20, 2) == 18, "v_2(20!) = 18");
  pcheck(canor_factorial_valuation(20, 3) == 8, "v_3(20!) = 8");
  pcheck(canor_factorial_valuation(100, 5) == 24, "v_5(100!) = 24");

  /* --- L_m ---------------------------------------------------------------- */
  printf("\n  L_m(n; Theta, Phi) = exp( (1/n!).{Order_m - X}.exp(Theta+Phi.i) )\n\n");
  printf("  The RATIO (Order_m - X)/n! is exact and its limit separates the\n");
  printf("  four MetaFlow classes outright. The VALUE needs exp of a complex\n");
  printf("  argument and is report-layer double; both are shown, and which is\n");
  printf("  which is marked.\n\n");
  printf("  %-14s %6s %16s %-16s %s\n", "class", "n", "log2 ratio", "limit class", "|L| (double)");
  {
    static const uint64_t ns[3] = { 6, 12, 24 };
    int c, j;
    for (c = 0; c < 4; ++c)
      for (j = 0; j < 3; ++j) {
        canor_residual r;
        double mag;
        if (!canor_residual_L((canor_kclass)c, ns[j], 0, 0.0, 0.0, &r)) continue;
        mag = sqrt(r.value_re * r.value_re + r.value_im * r.value_im);
        printf("  %-14s %6llu %16lld %-16s %.4g %s\n",
               c == 0 ? "General" : c == 1 ? "Specific" :
               c == 2 ? "Distinctive" : "Arbitrary",
               (unsigned long long)ns[j], (long long)r.log2_ratio_milli,
               canor_resid_class_name(r.cls), mag,
               r.deterministic ? "det" : "-");
        canor_residual_free(&r);
      }
  }
  {
    canor_residual g6, g24, s6, d6;
    canor_residual_L(CANOR_K_GENERAL, 6, 0, 0.0, 0.0, &g6);
    canor_residual_L(CANOR_K_GENERAL, 24, 0, 0.0, 0.0, &g24);
    canor_residual_L(CANOR_K_SPECIFIC, 6, 0, 0.0, 0.0, &s6);
    canor_residual_L(CANOR_K_DISTINCTIVE, 6, 0, 0.0, 0.0, &d6);
    pcheck(g24.log2_ratio_milli < g6.log2_ratio_milli,
           "General's ratio falls as n rises - it vanishes against n!");
    pcheck(g6.cls == CANOR_RESID_UNIT, "General is the unit class, L -> 1");
    pcheck(s6.cls == CANOR_RESID_CONVERGENT, "Specific converges: 2(n!)/n! -> 2");
    pcheck(d6.cls == CANOR_RESID_DIVERGENT, "Distinctive diverges");
    pcheck(!g6.exact, "the VALUE is marked inexact - it is a truncated series");
    pcheck(g6.deterministic,
           "but it IS deterministic: exact-rational Taylor, identical on every host");
    /* Reproducibility is the property that matters, so it is the one checked:
     * two independent evaluations must agree bit for bit. */
    {
      canor_residual a, b;
      canor_residual_L(CANOR_K_SPECIFIC, 8, 0, 0.25, 0.5, &a);
      canor_residual_L(CANOR_K_SPECIFIC, 8, 0, 0.25, 0.5, &b);
      pcheck(fb_rat_cmp(&a.exact_re, &b.exact_re) == 0 &&
             fb_rat_cmp(&a.exact_im, &b.exact_im) == 0,
             "two evaluations agree exactly - reproducible, hence stampable");
      canor_residual_free(&a);
      canor_residual_free(&b);
    }
    canor_residual_free(&g6); canor_residual_free(&g24);
    canor_residual_free(&s6); canor_residual_free(&d6);
  }
  printf("\n  So the residual function is a fourth independent route to the same\n"
         "  four-way split the Kapprox ladder, the MetaFlow axis and the\n"
         "  Mixture axis already make - and this one separates them by LIMIT\n"
         "  BEHAVIOUR rather than by magnitude, which needs no exponential at\n"
         "  all. The transcendental is only in the reported value.\n");

  printf("\n%d checks, %d failures\n", pchecks, pfails);
  return pfails ? 1 : 0;
}
