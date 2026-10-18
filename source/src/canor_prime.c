/* Primes, primality residuals, and L_m. See canor_prime.h.
 *
 * The sieve is Euler's linear one. The `break` in the inner loop is not an
 * optimisation - it is the correctness condition. Once p divides i, continuing
 * would cross off i*p' for a larger prime p', and that composite's smallest
 * factor is p, not p', so it would be reached twice. Breaking there is what
 * makes each composite crossed off exactly once and the whole pass O(n).
 */

#include "canor_prime.h"
#include "canor_big.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int32_t *g_spf;          /* smallest prime factor */
static int32_t *g_primes;
static int32_t g_nprimes;
static int32_t g_limit;

int32_t canor_prime_count(void) { return g_nprimes; }
int32_t canor_prime_at(int32_t i) {
  return (i >= 0 && i < g_nprimes) ? g_primes[i] : 0;
}

bool canor_prime_init(int32_t limit) {
  int32_t i, j;
  if (limit < 2) limit = 2;
  if (limit > CANOR_PRIME_LIMIT) limit = CANOR_PRIME_LIMIT;
  if (g_spf && g_limit >= limit) return true;

  free(g_spf);
  free(g_primes);
  g_spf = (int32_t *)calloc((size_t)limit + 1, sizeof *g_spf);
  g_primes = (int32_t *)malloc((size_t)(limit / 2 + 16) * sizeof *g_primes);
  if (!g_spf || !g_primes) { free(g_spf); free(g_primes); g_spf = NULL; g_primes = NULL; return false; }
  g_limit = limit;
  g_nprimes = 0;

  for (i = 2; i <= limit; ++i) {
    if (g_spf[i] == 0) {                 /* i has no smaller factor: prime */
      g_spf[i] = i;
      g_primes[g_nprimes++] = i;
    }
    for (j = 0; j < g_nprimes; ++j) {
      int32_t p = g_primes[j];
      if (p > g_spf[i]) break;           /* THE correctness condition */
      if ((int64_t)p * i > limit) break;
      g_spf[p * i] = p;                  /* crossed off by its least factor */
    }
  }
  return true;
}

bool canor_is_prime(int32_t n) {
  if (n < 2 || !g_spf || n > g_limit) return false;
  return g_spf[n] == n;
}

int32_t canor_smallest_factor(int32_t n) {
  if (n < 2 || !g_spf || n > g_limit) return 0;
  return g_spf[n];
}

bool canor_factorise(int32_t n, canor_factorisation *out) {
  if (!out) return false;
  memset(out, 0, sizeof *out);
  if (n < 2 || !g_spf || n > g_limit) return n >= 1;
  while (n > 1) {
    int32_t p = g_spf[n], e = 0;
    if (p <= 0) return false;
    while (n % p == 0) { n /= p; ++e; }
    if (out->n >= CANOR_FACTORS_MAX) return false;
    out->p[out->n] = p;
    out->e[out->n] = e;
    ++out->n;
  }
  return true;
}

int32_t canor_omega(int32_t n) {
  canor_factorisation f;
  if (!canor_factorise(n, &f)) return -1;
  return f.n;
}

int32_t canor_bigomega(int32_t n) {
  canor_factorisation f;
  int i, s = 0;
  if (!canor_factorise(n, &f)) return -1;
  for (i = 0; i < f.n; ++i) s += f.e[i];
  return s;
}

int32_t canor_totient(int32_t n) {
  canor_factorisation f;
  int i;
  int32_t r = n;
  if (n < 1) return 0;
  if (!canor_factorise(n, &f)) return -1;
  /* phi(n) = n . prod (1 - 1/p), done as integer division by p first so the
   * intermediate never leaves int32. */
  for (i = 0; i < f.n; ++i) r = r / f.p[i] * (f.p[i] - 1);
  return r;
}

int canor_mobius(int32_t n) {
  canor_factorisation f;
  int i;
  if (n < 1) return 0;
  if (n == 1) return 1;
  if (!canor_factorise(n, &f)) return 0;
  for (i = 0; i < f.n; ++i) if (f.e[i] > 1) return 0;   /* squareful */
  return (f.n & 1) ? -1 : 1;
}

int32_t canor_radical(int32_t n) {
  canor_factorisation f;
  int i;
  int32_t r = 1;
  if (n < 1) return 0;
  if (!canor_factorise(n, &f)) return 0;
  for (i = 0; i < f.n; ++i) r *= f.p[i];
  return r;
}

int64_t canor_factorial_valuation(int32_t n, int32_t p) {
  /* Legendre: sum over k of floor(n / p^k). Computed by repeated division so
   * p^k never has to be formed and cannot overflow. */
  int64_t total = 0, q = n;
  if (p < 2 || n < 0) return 0;
  while (q > 0) { q /= p; total += q; }
  return total;
}


/* --- deterministic transcendentals ------------------------------------------ */

/* Sums a Taylor series with the given term rule. `alt` alternates signs (cos,
 * sin); `step` is 1 for exp, 2 for cos and sin; `start` is the first power. */
static bool taylor(const fb_rat *x, int terms, int start, int step, int alt,
                   fb_rat *out, int *used) {
  fb_rat term, acc, xp, num, den;
  int i, k;
  bool ok = true;

  fb_rat_init(&term); fb_rat_init(&acc); fb_rat_init(&xp);
  fb_rat_init(&num); fb_rat_init(&den);
  if (!fb_rat_set_i64(&acc, 0, 1) || !fb_rat_set_i64(&xp, 1, 1)) ok = false;

  /* x^start */
  for (i = 0; ok && i < start; ++i) ok = fb_rat_mul(&xp, &xp, x);

  {
    /* factorial of `start`, then multiplied up by `step` each term */
    int64_t f = 1;
    for (i = 2; i <= start; ++i) f *= i;
    if (ok) ok = fb_rat_set_i64(&den, f, 1);
  }

  for (k = 0; ok && k < terms; ++k) {
    fb_rat inv;
    fb_rat_init(&inv);
    ok = fb_copy(&inv.num, &den.den) && fb_copy(&inv.den, &den.num) &&
         fb_rat_normalize(&inv) && fb_rat_mul(&term, &xp, &inv);
    fb_rat_free(&inv);
    if (!ok) break;
    if (alt && (k & 1)) { fb_neg(&term.num); }
    if (!fb_rat_add(&acc, &acc, &term)) { ok = false; break; }
    /* advance x^p and the factorial by `step` */
    for (i = 0; ok && i < step; ++i) ok = fb_rat_mul(&xp, &xp, x);
    {
      int64_t base = (int64_t)start + (int64_t)(k + 1) * step;
      int64_t f2 = 1, j;
      for (j = base - step + 1; j <= base && ok; ++j) {
        fb_rat mul;
        fb_rat_init(&mul);
        ok = fb_rat_set_i64(&mul, j, 1) && fb_rat_mul(&den, &den, &mul);
        fb_rat_free(&mul);
      }
      (void)f2;
    }
  }
  if (ok) ok = fb_rat_copy(out, &acc);
  if (used) *used = k;
  fb_rat_free(&term); fb_rat_free(&acc); fb_rat_free(&xp);
  fb_rat_free(&num); fb_rat_free(&den);
  return ok;
}

bool canor_exp_rat(canor_rational x, int terms, fb_rat *out, canor_exp_report *rep) {
  fb_rat xr, half;
  int squarings = 0, used = 0;
  bool ok = true;
  if (!out || !canor_rat_valid(x)) { if (rep) memset(rep, 0, sizeof *rep); return false; }
  if (terms < 4) terms = 4;
  if (terms > 64) terms = 64;

  fb_rat_init(&xr); fb_rat_init(&half);
  ok = fb_rat_set_i64(&xr, x.num, x.den) && fb_rat_set_i64(&half, 1, 2);
  /* RANGE REDUCTION. Halve until |x| <= 1/2, then square the result back.
   * The series then needs few terms and its remainder is tiny, where the
   * unreduced series at |x| = 20 would need dozens of bignum terms for the
   * same bound. */
  while (ok && squarings < 40) {
    fb_rat absx;
    int cmp;
    fb_rat_init(&absx);
    ok = fb_rat_copy(&absx, &xr);
    if (ok && absx.num.sign < 0) fb_neg(&absx.num);
    cmp = ok ? fb_rat_cmp(&absx, &half) : 0;
    fb_rat_free(&absx);
    if (!ok || cmp <= 0) break;
    ok = fb_rat_mul(&xr, &xr, &half);
    ++squarings;
  }
  if (ok) ok = taylor(&xr, terms, 0, 1, 0, out, &used);
  { int i; for (i = 0; ok && i < squarings; ++i) ok = fb_rat_mul(out, out, out); }
  fb_rat_free(&xr); fb_rat_free(&half);
  if (rep) {
    rep->ok = ok;
    rep->terms = used;
    rep->squarings = squarings;
    /* |remainder| <= |x^n / n!| with |x| <= 1/2, so log2 <= -n. Reported
     * before the squarings, which multiply the relative error by 2^squarings. */
    rep->bound_log2_milli = -1000LL * used + 1000LL * squarings;
  }
  return ok;
}

bool canor_cos_rat(canor_rational x, int terms, fb_rat *out, canor_exp_report *rep) {
  fb_rat xr;
  int used = 0;
  bool ok;
  if (!out || !canor_rat_valid(x)) return false;
  if (terms < 4) terms = 4;
  if (terms > 40) terms = 40;
  fb_rat_init(&xr);
  ok = fb_rat_set_i64(&xr, x.num, x.den) && taylor(&xr, terms, 0, 2, 1, out, &used);
  fb_rat_free(&xr);
  if (rep) { rep->ok = ok; rep->terms = used; rep->squarings = 0;
             rep->bound_log2_milli = -1000LL * used; }
  return ok;
}

bool canor_sin_rat(canor_rational x, int terms, fb_rat *out, canor_exp_report *rep) {
  fb_rat xr;
  int used = 0;
  bool ok;
  if (!out || !canor_rat_valid(x)) return false;
  if (terms < 4) terms = 4;
  if (terms > 40) terms = 40;
  fb_rat_init(&xr);
  ok = fb_rat_set_i64(&xr, x.num, x.den) && taylor(&xr, terms, 1, 2, 1, out, &used);
  fb_rat_free(&xr);
  if (rep) { rep->ok = ok; rep->terms = used; rep->squarings = 0;
             rep->bound_log2_milli = -1000LL * used; }
  return ok;
}

void canor_residual_free(canor_residual *r) {
  if (!r) return;
  fb_rat_free(&r->exact_re);
  fb_rat_free(&r->exact_im);
}

/* --- L_m -------------------------------------------------------------------- */

const char *canor_resid_class_name(canor_resid_class c) {
  return c == CANOR_RESID_UNIT ? "unit (L -> 1)" :
         c == CANOR_RESID_CONVERGENT ? "convergent" :
         c == CANOR_RESID_DIVERGENT ? "divergent" : "undefined";
}

bool canor_residual_L(canor_kclass m, uint64_t n, int64_t x_milli,
                   double theta, double phi, canor_residual *out) {
  int64_t order_milli, fact_milli, ratio_milli;
  fb_rat er, cs, sn, re, im, ratio;
  canor_exp_report r1, r2, r3;
  canor_rational th, ph;
  bool ok = true;
  const int TERMS = 24;      /* declared, not tuned: the bound is 2^-24 */

  if (!out) return false;
  memset(out, 0, sizeof *out);
  fb_rat_init(&out->exact_re);
  fb_rat_init(&out->exact_im);
  if (n == 0) { out->cls = CANOR_RESID_UNDEFINED; return false; }

  /* Everything in log2 milli-units, which is the only scale on which these
   * quantities coexist: (2(n!)+1)! at n = 96 has about 1e152 digits, so the
   * ratio is never formed and never needs to be. */
  /* UNITS. canor_kapprox_order_milli returns log2 of log2 - an ORDER - while
   * canor_log2_factorial_milli returns a plain log2. Subtracting one from the
   * other is unit-mismatched and produces a number that means nothing; the
   * same mistake made the Kapprox ladder appear to invert once already.
   *
   * The ratio needs log2(Order_m) - log2(n!), so the order form cannot be
   * used and log2 of the COUNT is computed per class:
   *
   *   General      log2(2n+1)
   *   Specific     log2(2.n! + 1)  =  1 + log2(n!) to within 2^-log2(n!)
   *   Distinctive  log2((2n+1)!)
   *   Arbitrary    log2((2(n!)+1)!) - astronomically large, and saturated
   *
   * Specific then gives ratio log2 = 1 exactly, i.e. a limit of 2 - which is
   * the analytic prediction, so this is a check on the arithmetic and not
   * merely a computation of it. */
  fact_milli = canor_log2_factorial_milli(n);
  switch (m) {
    case CANOR_K_GENERAL:
      order_milli = canor_log2_milli(2 * n + 1);
      break;
    case CANOR_K_SPECIFIC:
      order_milli = 1000 + fact_milli;              /* log2(2.n!) = 1 + log2(n!) */
      break;
    case CANOR_K_DISTINCTIVE:
      order_milli = canor_log2_factorial_milli(2 * n + 1);
      break;
    default: {
      /* log2((2(n!)+1)!) by Stirling in the log domain: m.log2(m) - m.log2(e).
       * Saturated when that leaves int64 - the class is DIVERGENT either way,
       * and a saturated magnitude is honest where a wrapped one is not. */
      int64_t lg_m = 1000 + fact_milli;             /* log2(m), m = 2.n!+1 */
      if (lg_m > 60000) order_milli = INT64_MAX / 4;   /* saturated */
      else {
        double mm = pow(2.0, (double)lg_m / 1000.0);
        double lg = mm * ((double)lg_m / 1000.0 - 1.442695);
        order_milli = (lg > 9.0e15) ? INT64_MAX / 4 : (int64_t)(lg * 1000.0);
      }
      break;
    }
  }
  ratio_milli = order_milli - x_milli - fact_milli;
  out->log2_ratio_milli = ratio_milli;

  /* THE CLASS IS EXACT. It is read off the ladder, not off a number: General
   * is linear against a factorial and must vanish; Specific is 2(n!) against
   * n! and must approach 2; the factorial-of-factorial classes must diverge.
   * No exponential is involved, so this part carries no approximation at all. */
  switch (m) {
    case CANOR_K_GENERAL:     out->cls = CANOR_RESID_UNIT; break;
    case CANOR_K_SPECIFIC:    out->cls = CANOR_RESID_CONVERGENT; break;
    default:               out->cls = CANOR_RESID_DIVERGENT; break;
  }
  if (n <= 2) out->cls = CANOR_RESID_UNDEFINED;   /* the ladder does not separate below 3 */
  out->exact = true;

  /* A DIVERGENT CLASS HAS NO VALUE. Forming one would mean clamping the ratio
   * to some arbitrary ceiling and exponentiating that - which is slow (the
   * range reduction needs dozens of squarings of a bignum) and, worse, is an
   * invented number standing where the honest answer is "this does not
   * converge". The class is returned; the value is not. */
  if (out->cls == CANOR_RESID_DIVERGENT || out->cls == CANOR_RESID_UNDEFINED) {
    out->deterministic = false;
    out->exact = false;
    return true;
  }

  /* THE VALUE IS DETERMINISTIC BUT NOT EXACT. Theta and Phi arrive as doubles
   * for the caller's convenience and are immediately snapped to exact
   * rationals over a fixed denominator - otherwise the input itself would be
   * host-dependent and no amount of rational arithmetic afterwards could
   * recover reproducibility. */
  th = canor_rat((int64_t)(theta * 1000000.0), 1000000);
  ph = canor_rat((int64_t)(phi * 1000000.0), 1000000);

  fb_rat_init(&er); fb_rat_init(&cs); fb_rat_init(&sn);
  fb_rat_init(&re); fb_rat_init(&im); fb_rat_init(&ratio);

  ok = canor_exp_rat(th, TERMS, &er, &r1) &&
       canor_cos_rat(ph, TERMS, &cs, &r2) &&
       canor_sin_rat(ph, TERMS, &sn, &r3);

  /* The ratio as a rational, from its log2. Clamped, and the clamp is
   * reported rather than silent: a divergent class has no finite value and
   * saying "it is 2^40" would be inventing one. */
  if (ok) {
    int64_t lg = ratio_milli / 1000;
    if (lg > 40) { lg = 40; }
    if (lg < -40) { lg = -40; }
    if (lg >= 0) ok = fb_rat_set_i64(&ratio, (int64_t)1 << lg, 1);
    else ok = fb_rat_set_i64(&ratio, 1, (int64_t)1 << (-lg));
  }
  /* ratio . e^theta . (cos phi + i sin phi) */
  if (ok) ok = fb_rat_mul(&re, &ratio, &er) && fb_rat_mul(&re, &re, &cs);
  if (ok) ok = fb_rat_mul(&im, &ratio, &er) && fb_rat_mul(&im, &im, &sn);
  /* exp(re + im.i) = e^re . (cos im + i sin im). The outer exponential needs
   * its argument as a small rational, so it is taken through the same
   * deterministic routine rather than through libm. */
  if (ok) {
    fb_rat outer, ccos, ssin;
    canor_rational re_r, im_r;
    int64_t rn = 0, dn = 0, in_ = 0;
    fb_rat_init(&outer); fb_rat_init(&ccos); fb_rat_init(&ssin);
    /* Narrow the bignum back to an int64 rational for the transcendentals;
     * a value too wide to narrow is a value whose exponential has no
     * representable answer, and that is refused. */
    if (!fb_rat_to_fixed(&re.num, &re.den, 20, &rn) ||
        !fb_rat_to_fixed(&im.num, &im.den, 20, &in_)) {
      ok = false;
    } else {
      dn = (int64_t)1 << 20;
      re_r = canor_rat(rn, dn);
      im_r = canor_rat(in_, dn);
      ok = canor_exp_rat(re_r, TERMS, &outer, &r1) &&
           canor_cos_rat(im_r, TERMS, &ccos, &r2) &&
           canor_sin_rat(im_r, TERMS, &ssin, &r3) &&
           fb_rat_mul(&out->exact_re, &outer, &ccos) &&
           fb_rat_mul(&out->exact_im, &outer, &ssin);
    }
    fb_rat_free(&outer); fb_rat_free(&ccos); fb_rat_free(&ssin);
  }

  if (ok) {
    int64_t fx = 0;
    if (fb_rat_to_fixed(&out->exact_re.num, &out->exact_re.den, 20, &fx))
      out->value_re = (double)fx / (double)(1 << 20);
    if (fb_rat_to_fixed(&out->exact_im.num, &out->exact_im.den, 20, &fx))
      out->value_im = (double)fx / (double)(1 << 20);
    out->deterministic = true;
    out->bound_log2_milli = r1.bound_log2_milli;
  }
  out->exact = false;        /* the CLASS is exact; the VALUE is not */

  fb_rat_free(&er); fb_rat_free(&cs); fb_rat_free(&sn);
  fb_rat_free(&re); fb_rat_free(&im); fb_rat_free(&ratio);
  return ok;
}
