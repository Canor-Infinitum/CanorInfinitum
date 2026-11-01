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

#include <string.h>

/* Fixed static tables (AGENTS.md invariant 2): the sieve is process state built
 * once, so it lives in sized storage rather than on a heap. */
static int32_t g_spf[CANOR_PRIME_LIMIT + 1];   /* smallest prime factor */
static int32_t g_primes[CANOR_PRIME_COUNT_MAX];
static int32_t g_nprimes;
static int32_t g_limit;                        /* 0 until built */

int32_t canor_prime_count(void) { return g_nprimes; }
int32_t canor_prime_at(int32_t i) {
  return (i >= 0 && i < g_nprimes) ? g_primes[i] : 0;
}

bool canor_prime_init(int32_t limit) {
  int32_t i, j;
  if (limit < 2) limit = 2;
  if (limit > CANOR_PRIME_LIMIT) return false;   /* refused, never clamped */
  if (g_limit >= limit) return true;

  memset(g_spf, 0, ((size_t)limit + 1) * sizeof *g_spf);
  g_limit = 0;
  g_nprimes = 0;

  for (i = 2; i <= limit; ++i) {
    if (g_spf[i] == 0) {                 /* i has no smaller factor: prime */
      g_spf[i] = i;
      if (g_nprimes >= CANOR_PRIME_COUNT_MAX) return false;   /* full: refuse */
      g_primes[g_nprimes++] = i;
    }
    for (j = 0; j < g_nprimes; ++j) {
      int32_t p = g_primes[j];
      if (p > g_spf[i]) break;           /* THE correctness condition */
      if ((int64_t)p * i > limit) break;
      g_spf[p * i] = p;                  /* crossed off by its least factor */
    }
  }
  g_limit = limit;
  return true;
}

bool canor_is_prime(int32_t n) {
  if (n < 2 || n > g_limit) return false;
  return g_spf[n] == n;
}

int32_t canor_smallest_factor(int32_t n) {
  if (n < 2 || n > g_limit) return 0;
  return g_spf[n];
}

bool canor_factorise(int32_t n, canor_factorisation *out) {
  if (!out) return false;
  memset(out, 0, sizeof *out);
  if (n < 2 || n > g_limit) return n >= 1;
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

/* Chains canor_big calls: each runs only while every earlier one succeeded. */
#define PRIME_TRY(st, e) do { if ((st) == CANOR_OK) (st) = (e); } while (0)

/* Range-reduction ceiling: |x| is halved at most this many times. */
#define PRIME_SQUARINGS_MAX 40
#define PRIME_EXP_TERMS_MIN 4
#define PRIME_EXP_TERMS_MAX 64
#define PRIME_TRIG_TERMS_MAX 40
#define PRIME_MILLI 1000

/* Sums a Taylor series. `alt` alternates signs (cos, sin); `step` is 1 for exp,
 * 2 for cos and sin; `start` is the first power. */
static canor_status taylor(const canor_big_rat_t *x, int terms, int start, int step, int alt,
                           canor_big_rat_t *out, int *used) {
  canor_big_rat_t term, acc, xp, den, inv, mul;
  int i, k;
  canor_status st = CANOR_OK;

  canor_big_rat_init(&acc);
  PRIME_TRY(st, canor_big_rat_set_i64(&xp, 1, 1));
  for (i = 0; i < start; ++i) PRIME_TRY(st, canor_big_rat_mul(&xp, &xp, x));
  {
    int64_t f = 1;
    for (i = 2; i <= start; ++i) f *= i;
    PRIME_TRY(st, canor_big_rat_set_i64(&den, f, 1));
  }
  for (k = 0; st == CANOR_OK && k < terms; ++k) {
    PRIME_TRY(st, canor_big_rat_inv(&inv, &den));
    PRIME_TRY(st, canor_big_rat_mul(&term, &xp, &inv));
    if (st) break;
    if (alt && (k & 1)) canor_big_neg(&term.num);
    PRIME_TRY(st, canor_big_rat_add(&acc, &acc, &term));
    for (i = 0; i < step; ++i) PRIME_TRY(st, canor_big_rat_mul(&xp, &xp, x));
    {
      int64_t base = (int64_t)start + (int64_t)(k + 1) * step, j;
      for (j = base - step + 1; j <= base; ++j) {
        PRIME_TRY(st, canor_big_rat_set_i64(&mul, j, 1));
        PRIME_TRY(st, canor_big_rat_mul(&den, &den, &mul));
      }
    }
  }
  if (st == CANOR_OK) *out = acc;
  if (used) *used = k;
  return st;
}

bool canor_exp_rat(canor_rational x, int terms, canor_big_rat_t *out, canor_exp_report *rep) {
  canor_big_rat_t xr, half, absx;
  int squarings = 0, used = 0, i;
  canor_status st = CANOR_OK;
  if (!out || !canor_rat_valid(x)) { if (rep) memset(rep, 0, sizeof *rep); return false; }
  if (terms < PRIME_EXP_TERMS_MIN) terms = PRIME_EXP_TERMS_MIN;
  if (terms > PRIME_EXP_TERMS_MAX) terms = PRIME_EXP_TERMS_MAX;

  PRIME_TRY(st, canor_big_rat_set_i64(&xr, x.num, x.den));
  PRIME_TRY(st, canor_big_rat_set_i64(&half, 1, 2));
  /* RANGE REDUCTION. Halve until |x| <= 1/2, then square the result back. */
  while (st == CANOR_OK && squarings < PRIME_SQUARINGS_MAX) {
    int cmp;
    absx = xr;
    canor_big_abs(&absx.num);
    cmp = canor_big_rat_cmp(&absx, &half, &st);
    if (st || cmp <= 0) break;
    PRIME_TRY(st, canor_big_rat_mul(&xr, &xr, &half));
    ++squarings;
  }
  PRIME_TRY(st, taylor(&xr, terms, 0, 1, 0, out, &used));
  for (i = 0; i < squarings; ++i) PRIME_TRY(st, canor_big_rat_mul(out, out, out));
  if (rep) {
    rep->ok = (st == CANOR_OK);
    rep->terms = used;
    rep->squarings = squarings;
    /* |remainder| <= |x^n / n!| with |x| <= 1/2, so log2 <= -n. Reported
     * before the squarings, which multiply the relative error by 2^squarings. */
    rep->bound_log2_milli = -(int64_t)PRIME_MILLI * used + (int64_t)PRIME_MILLI * squarings;
  }
  return st == CANOR_OK;
}

static bool trig_rat(canor_rational x, int terms, int start, canor_big_rat_t *out,
                     canor_exp_report *rep) {
  canor_big_rat_t xr;
  int used = 0;
  canor_status st = CANOR_OK;
  if (!out || !canor_rat_valid(x)) return false;
  if (terms < PRIME_EXP_TERMS_MIN) terms = PRIME_EXP_TERMS_MIN;
  if (terms > PRIME_TRIG_TERMS_MAX) terms = PRIME_TRIG_TERMS_MAX;
  PRIME_TRY(st, canor_big_rat_set_i64(&xr, x.num, x.den));
  PRIME_TRY(st, taylor(&xr, terms, start, 2, 1, out, &used));
  if (rep) {
    rep->ok = (st == CANOR_OK); rep->terms = used; rep->squarings = 0;
    rep->bound_log2_milli = -(int64_t)PRIME_MILLI * used;
  }
  return st == CANOR_OK;
}

bool canor_cos_rat(canor_rational x, int terms, canor_big_rat_t *out, canor_exp_report *rep) {
  return trig_rat(x, terms, 0, out, rep);
}

bool canor_sin_rat(canor_rational x, int terms, canor_big_rat_t *out, canor_exp_report *rep) {
  return trig_rat(x, terms, 1, out, rep);
}

/* --- integer exp2 ------------------------------------------------------------
 * 2^(f/1000) for 0 <= f < 1000, in Q30. exp(f.ln2/1000) by Taylor series in
 * Q30 integers: x < ln2 < 1, so every term and product fits in 64 bits and the
 * series converges below one Q30 unit within 16 terms. Replaces pow(2, .) so
 * the Arbitrary-class order stays integer (AGENTS.md invariant 1). */
#define PRIME_Q 30
#define PRIME_Q_ONE ((int64_t)1 << PRIME_Q)
#define PRIME_LN2_Q30 INT64_C(744261118)      /* round(ln 2 * 2^30) */
#define PRIME_LOG2E_MICRO INT64_C(1442695)    /* log2(e) * 10^6 */
#define PRIME_MICRO_PER_MILLI 1000
#define PRIME_EXP2_TERMS 16
/* Above this log2(m) the order saturates outright (was the same in the
 * floating-point version). */
#define PRIME_ARBITRARY_LG_MAX_MILLI 60000
#define PRIME_SATURATED (INT64_MAX / 4)

static int64_t exp2_frac_q30(int64_t f_milli) {
  int64_t x = f_milli * PRIME_LN2_Q30 / PRIME_MILLI;   /* Q30, < ln 2 */
  int64_t term = PRIME_Q_ONE, acc = PRIME_Q_ONE;
  int k;
  for (k = 1; k < PRIME_EXP2_TERMS && term; ++k) {
    term = term * x / PRIME_Q_ONE / k;
    acc += term;
  }
  return acc;
}

static int bits_u64(uint64_t v) { int b = 0; while (v) { ++b; v >>= 1; } return b; }

/* log2((2(n!)+1)!) in milli by Stirling in the log domain:
 * m.log2(m) - m.log2(e), with log2(m) = lg_m / 1000 and m = 2^(lg_m/1000).
 * Saturated when it leaves the int64 range a caller can do arithmetic with -
 * the class is DIVERGENT either way, and a saturated magnitude is honest where
 * a wrapped one is not. */
static int64_t arbitrary_order_milli(int64_t lg_m) {
  int64_t ip, e2f, v;
  int shift;
  if (lg_m > PRIME_ARBITRARY_LG_MAX_MILLI) return PRIME_SATURATED;
  ip = lg_m / PRIME_MILLI;
  e2f = exp2_frac_q30(lg_m % PRIME_MILLI);
  /* v = 2^frac (Q30) * (log2 m - log2 e) in milli: at most 2^31 * 6e4. */
  v = e2f * ((lg_m * PRIME_MICRO_PER_MILLI - PRIME_LOG2E_MICRO) / PRIME_MICRO_PER_MILLI);
  if (v < 0) return 0;
  shift = (int)ip - PRIME_Q;
  if (shift >= 0) {
    if (bits_u64((uint64_t)v) + shift > 61) return PRIME_SATURATED;
    v <<= shift;
  } else {
    v >>= -shift;
  }
  return v > PRIME_SATURATED ? PRIME_SATURATED : v;
}

/* --- L_m -------------------------------------------------------------------- */

const char *canor_resid_class_name(canor_resid_class c) {
  return c == CANOR_RESID_UNIT ? "unit (L -> 1)" :
         c == CANOR_RESID_CONVERGENT ? "convergent" :
         c == CANOR_RESID_DIVERGENT ? "divergent" : "undefined";
}

#define PRIME_RESID_TERMS 24       /* declared, not tuned: the bound is 2^-24 */
#define PRIME_RATIO_LG_CLAMP 40    /* the ratio rational is clamped to 2^+-40 */

bool canor_residual_L(canor_kclass m, uint64_t n, int64_t x_milli,
                   canor_rational theta, canor_rational phi, canor_residual *out) {
  int64_t order_milli, fact_milli, ratio_milli;
  canor_big_rat_t er, cs, sn, re, im, ratio;
  canor_exp_report r1, r2, r3;
  canor_status st = CANOR_OK;

  if (!out) return false;
  memset(out, 0, sizeof *out);
  canor_big_rat_init(&out->exact_re);
  canor_big_rat_init(&out->exact_im);
  if (n == 0) { out->cls = CANOR_RESID_UNDEFINED; return false; }

  /* Everything in log2 milli-units, the only scale on which these quantities
   * coexist. UNITS: canor_kapprox_order_milli returns log2 of log2 while
   * canor_log2_factorial_milli is a plain log2, so log2 of the COUNT is
   * computed per class:
   *
   *   General      log2(2n+1)
   *   Specific     log2(2.n! + 1)  =  1 + log2(n!) to within 2^-log2(n!)
   *   Distinctive  log2((2n+1)!)
   *   Arbitrary    log2((2(n!)+1)!) - astronomically large, and saturated
   *
   * Specific then gives ratio log2 = 1 exactly, a limit of 2 - the analytic
   * prediction, so this is a check on the arithmetic. */
  fact_milli = canor_log2_factorial_milli(n);
  switch (m) {
    case CANOR_K_GENERAL:     order_milli = canor_log2_milli(2 * n + 1); break;
    case CANOR_K_SPECIFIC:    order_milli = PRIME_MILLI + fact_milli; break;
    case CANOR_K_DISTINCTIVE: order_milli = canor_log2_factorial_milli(2 * n + 1); break;
    default:                  order_milli = arbitrary_order_milli(PRIME_MILLI + fact_milli); break;
  }
  ratio_milli = order_milli - x_milli - fact_milli;
  out->log2_ratio_milli = ratio_milli;

  /* THE CLASS IS EXACT. It is read off the ladder, not off a number. */
  switch (m) {
    case CANOR_K_GENERAL:  out->cls = CANOR_RESID_UNIT; break;
    case CANOR_K_SPECIFIC: out->cls = CANOR_RESID_CONVERGENT; break;
    default:               out->cls = CANOR_RESID_DIVERGENT; break;
  }
  if (n <= 2) out->cls = CANOR_RESID_UNDEFINED;   /* the ladder does not separate below 3 */
  out->exact = true;

  /* A DIVERGENT CLASS HAS NO VALUE: forming one would mean exponentiating an
   * invented clamp. The class is returned; the value is not. */
  if (out->cls == CANOR_RESID_DIVERGENT || out->cls == CANOR_RESID_UNDEFINED) {
    out->deterministic = false;
    out->exact = false;
    return true;
  }
  if (!canor_rat_valid(theta) || !canor_rat_valid(phi)) return false;

  if (!canor_exp_rat(theta, PRIME_RESID_TERMS, &er, &r1) ||
      !canor_cos_rat(phi, PRIME_RESID_TERMS, &cs, &r2) ||
      !canor_sin_rat(phi, PRIME_RESID_TERMS, &sn, &r3)) st = CANOR_ERR_FULL;

  /* The ratio as a rational, from its log2, clamped: a value is reported for
   * convergent classes only, and those sit well inside the clamp. */
  {
    int64_t lg = ratio_milli / PRIME_MILLI;
    if (lg > PRIME_RATIO_LG_CLAMP) lg = PRIME_RATIO_LG_CLAMP;
    if (lg < -PRIME_RATIO_LG_CLAMP) lg = -PRIME_RATIO_LG_CLAMP;
    if (lg >= 0) PRIME_TRY(st, canor_big_rat_set_i64(&ratio, (int64_t)1 << lg, 1));
    else PRIME_TRY(st, canor_big_rat_set_i64(&ratio, 1, (int64_t)1 << (-lg)));
  }
  /* ratio . e^theta . (cos phi + i sin phi) */
  PRIME_TRY(st, canor_big_rat_mul(&re, &ratio, &er));
  PRIME_TRY(st, canor_big_rat_mul(&re, &re, &cs));
  PRIME_TRY(st, canor_big_rat_mul(&im, &ratio, &er));
  PRIME_TRY(st, canor_big_rat_mul(&im, &im, &sn));
  /* exp(re + im.i) = e^re . (cos im + i sin im), through the same
   * deterministic routines after narrowing to Q20 rationals; a value too wide
   * to narrow has no representable exponential and is refused. */
  if (st == CANOR_OK) {
    canor_big_rat_t outer, ccos, ssin;
    int64_t rn = 0, in_ = 0, dn = (int64_t)1 << CANOR_RESIDUAL_Q;
    PRIME_TRY(st, canor_big_rat_to_fixed(&re.num, &re.den, CANOR_RESIDUAL_Q, &rn));
    PRIME_TRY(st, canor_big_rat_to_fixed(&im.num, &im.den, CANOR_RESIDUAL_Q, &in_));
    if (st == CANOR_OK &&
        (!canor_exp_rat(canor_rat(rn, dn), PRIME_RESID_TERMS, &outer, &r1) ||
         !canor_cos_rat(canor_rat(in_, dn), PRIME_RESID_TERMS, &ccos, &r2) ||
         !canor_sin_rat(canor_rat(in_, dn), PRIME_RESID_TERMS, &ssin, &r3)))
      st = CANOR_ERR_FULL;
    PRIME_TRY(st, canor_big_rat_mul(&out->exact_re, &outer, &ccos));
    PRIME_TRY(st, canor_big_rat_mul(&out->exact_im, &outer, &ssin));
  }
  if (st == CANOR_OK) {
    int64_t fx = 0;
    if (canor_big_rat_to_fixed(&out->exact_re.num, &out->exact_re.den, CANOR_RESIDUAL_Q, &fx) == CANOR_OK)
      out->value_re_q20 = fx;
    if (canor_big_rat_to_fixed(&out->exact_im.num, &out->exact_im.den, CANOR_RESIDUAL_Q, &fx) == CANOR_OK)
      out->value_im_q20 = fx;
    out->deterministic = true;
    out->bound_log2_milli = r1.bound_log2_milli;
  }
  out->exact = false;        /* the CLASS is exact; the VALUE is not */
  return st == CANOR_OK;
}
