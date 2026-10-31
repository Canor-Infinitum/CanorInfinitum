/* Rational polynomials, Pade approximants, Mittag-Leffler. See canor_poly.h. */

#include "canor_poly.h"
#include "canor_prime.h"

#include <string.h>

void canor_poly_init(canor_poly *p) {
  int i;
  memset(p, 0, sizeof *p);
  for (i = 0; i < CANOR_POLY_MAX; ++i) p->c[i] = canor_rat(0, 1);
}

bool canor_poly_set(canor_poly *p, int degree, const canor_rational *coeffs) {
  int i;
  if (!p || !coeffs || degree < 0 || degree >= CANOR_POLY_MAX) return false;
  canor_poly_init(p);
  for (i = 0; i <= degree; ++i) {
    if (!canor_rat_valid(coeffs[i])) return false;
    p->c[i] = coeffs[i];
  }
  p->degree = degree;
  while (p->degree > 0 && p->c[p->degree].num == 0) --p->degree;
  return true;
}

canor_rational canor_poly_eval(const canor_poly *p, canor_rational x) {
  /* Horner: fewest multiplications, and each one is a checked rational
   * multiply, so a coefficient blow-up shows up as poison rather than as a
   * wrapped value. */
  canor_rational acc = canor_rat(0, 1);
  int i;
  if (!p) return canor_rat(0, 0);
  for (i = p->degree; i >= 0; --i) {
    acc = canor_rat_mul(acc, x);
    if (!canor_rat_valid(acc)) return acc;
    acc = canor_rat_add(acc, p->c[i]);
    if (!canor_rat_valid(acc)) return acc;
  }
  return acc;
}

bool canor_poly_deflate(canor_poly *p, canor_rational r, canor_poly *out) {
  canor_rational carry = canor_rat(0, 1);
  int i;
  if (!p || !out || p->degree < 1) return false;
  /* Refuse unless r really is a root: a quotient computed from a non-root is
   * a polynomial that means nothing, and returning it would let a caller
   * "factor" by any number at all. */
  if (canor_poly_eval(p, r).num != 0) return false;
  canor_poly_init(out);
  out->degree = p->degree - 1;
  for (i = p->degree; i >= 1; --i) {
    carry = canor_rat_add(p->c[i], canor_rat_mul(carry, r));
    if (!canor_rat_valid(carry)) return false;
    out->c[i - 1] = carry;
  }
  return true;
}

/* Divisors of |n|, from the sieve's factorisation. */
static int divisors_of(int64_t n, int64_t *out, int cap) {
  canor_factorisation f;
  int count = 1, i, k;
  if (n < 0) n = -n;
  if (n == 0 || n > 1000000) return 0;
  if (!canor_factorise((int32_t)n, &f)) return 0;
  out[0] = 1;
  for (i = 0; i < f.n; ++i) {
    int base = count;
    int64_t pk = 1;
    for (k = 0; k < f.e[i]; ++k) {
      int j;
      pk *= f.p[i];
      for (j = 0; j < base; ++j) {
        if (count >= cap) return count;
        out[count++] = out[j] * pk;
      }
    }
  }
  return count;
}

bool canor_poly_rational_roots(const canor_poly *p, canor_poly_factors *out) {
  canor_poly work;
  int64_t pd[256], qd[256];
  int np, nq, i, j, sgn;
  int64_t a0n = 0, adn = 0;

  if (!p || !out) return false;
  memset(out, 0, sizeof *out);
  canor_poly_init(&out->remainder);
  work = *p;
  if (work.degree < 1) { out->remainder = work; out->complete = true; return true; }

  /* The rational root theorem needs INTEGER leading and trailing coefficients.
   * Clearing denominators would change the polynomial's scale but not its
   * roots; doing it here keeps the search exact. */
  {
    int64_t lcm = 1, k;
    for (i = 0; i <= work.degree; ++i) {
      int64_t d = work.c[i].den, g = lcm, h = d, t;
      while (h) { t = g % h; g = h; h = t; }
      if (g == 0) g = 1;
      if (d / g > 0 && lcm > 1000000000LL / (d / g)) return false;   /* refuse */
      lcm = lcm / g * d;
    }
    for (i = 0; i <= work.degree; ++i) {
      canor_rational m = canor_rat_mul(work.c[i], canor_rat(lcm, 1));
      if (!canor_rat_valid(m) || m.den != 1) return false;
      work.c[i] = m;
    }
    (void)k;
  }
  a0n = work.c[0].num;
  adn = work.c[work.degree].num;
  if (adn == 0) return false;

  /* x = 0 is a root exactly when the constant term is zero, and the divisor
   * search below cannot see it because 0 = 0/q is not of the form p/q with
   * p | a_0 when a_0 is 0. Handled first rather than missed. */
  while (work.degree >= 1 && work.c[0].num == 0) {
    canor_poly q;
    if (!canor_poly_deflate(&work, canor_rat(0, 1), &q)) break;
    if (out->n == 0 || out->root[out->n - 1].num != 0) {
      out->root[out->n] = canor_rat(0, 1);
      out->mult[out->n] = 1;
      ++out->n;
    } else {
      ++out->mult[out->n - 1];
    }
    work = q;
    a0n = work.c[0].num;
    adn = work.c[work.degree].num;
  }

  if (work.degree >= 1) {
    np = divisors_of(a0n, pd, 256);
    nq = divisors_of(adn, qd, 256);
    for (i = 0; i < np; ++i)
      for (j = 0; j < nq; ++j)
        for (sgn = 1; sgn >= -1; sgn -= 2) {
          canor_rational cand = canor_rat(sgn * pd[i], qd[j]);
          int mult = 0;
          if (!canor_rat_valid(cand)) continue;
          while (work.degree >= 1 && canor_poly_eval(&work, cand).num == 0) {
            canor_poly q;
            if (!canor_poly_deflate(&work, cand, &q)) break;
            work = q;
            ++mult;
          }
          if (mult && out->n < CANOR_POLY_MAX) {
            out->root[out->n] = cand;
            out->mult[out->n] = mult;
            ++out->n;
          }
        }
  }
  out->remainder = work;
  out->remainder_degree = work.degree;
  out->complete = (work.degree == 0);
  return true;
}

bool canor_poly_resolvent_cubic(const canor_poly *quartic, canor_poly *out) {
  /* For x^4 + p x^2 + q x + r (depressed), the resolvent is
   * y^3 - p y^2 - 4r y + (4pr - q^2). The quartic is depressed first by the
   * substitution x -> x - b/(4a), which is exact in rationals. */
  canor_rational a, b, c, d, e, p, q, r, shift, t1, t2;
  canor_rational co[4];
  if (!quartic || !out || quartic->degree != 4) return false;
  a = quartic->c[4]; b = quartic->c[3]; c = quartic->c[2];
  d = quartic->c[1]; e = quartic->c[0];
  if (a.num == 0) return false;
  {
    canor_rational inv = canor_rat(a.den, a.num);
    b = canor_rat_mul(b, inv); c = canor_rat_mul(c, inv);
    d = canor_rat_mul(d, inv); e = canor_rat_mul(e, inv);
  }
  shift = canor_rat_mul(b, canor_rat(1, 4));
  /* p = c - 3b^2/8 */
  t1 = canor_rat_mul(canor_rat_mul(b, b), canor_rat(3, 8));
  p = canor_rat_add(c, canor_rat(-t1.num, t1.den));
  /* q = d - bc/2 + b^3/8 */
  t1 = canor_rat_mul(canor_rat_mul(b, c), canor_rat(1, 2));
  t2 = canor_rat_mul(canor_rat_mul(canor_rat_mul(b, b), b), canor_rat(1, 8));
  q = canor_rat_add(canor_rat_add(d, canor_rat(-t1.num, t1.den)), t2);
  /* r = e - bd/4 + b^2 c/16 - 3b^4/256 */
  t1 = canor_rat_mul(canor_rat_mul(b, d), canor_rat(1, 4));
  r = canor_rat_add(e, canor_rat(-t1.num, t1.den));
  t1 = canor_rat_mul(canor_rat_mul(canor_rat_mul(b, b), c), canor_rat(1, 16));
  r = canor_rat_add(r, t1);
  t1 = canor_rat_mul(canor_rat_mul(canor_rat_mul(canor_rat_mul(b, b), b), b), canor_rat(3, 256));
  r = canor_rat_add(r, canor_rat(-t1.num, t1.den));
  (void)shift;

  co[3] = canor_rat(1, 1);
  co[2] = canor_rat(-p.num, p.den);
  co[1] = canor_rat_mul(r, canor_rat(-4, 1));
  t1 = canor_rat_mul(canor_rat_mul(p, r), canor_rat(4, 1));
  t2 = canor_rat_mul(q, q);
  co[0] = canor_rat_add(t1, canor_rat(-t2.num, t2.den));
  { int i; for (i = 0; i < 4; ++i) if (!canor_rat_valid(co[i])) return false; }
  return canor_poly_set(out, 3, co);
}

/* --- Pade ------------------------------------------------------------------- */

/* Chains canor_big calls: each runs only while every earlier one succeeded. */
#define POLY_TRY(st, e) do { if ((st) == CANOR_OK) (st) = (e); } while (0)

#define POLY_PADE_ORDER_MAX 8
#define POLY_SQUARINGS_MAX 40

bool canor_pade_exp(canor_rational x, int m, int n, canor_big_rat_t *out, int *squarings) {
  /* The [m/n] Pade approximant of exp has closed-form coefficients:
   *   P_j = C(m, j) . (m+n-j)! / (m+n)! ,  Q_j = (-1)^j C(n, j) . (m+n-j)! / (m+n)!
   * Both are rational, so the whole approximant is, and evaluating it is exact
   * rational arithmetic with no series truncation inside it. */
  canor_big_rat_t num, den, xp, term, xr, half, absx, cf, inv, res;
  int j, i, sq = 0, top = (m > n ? m : n);
  canor_status st = CANOR_OK;
  if (!out || !canor_rat_valid(x) || m < 1 || n < 1 ||
      m > POLY_PADE_ORDER_MAX || n > POLY_PADE_ORDER_MAX) return false;

  POLY_TRY(st, canor_big_rat_set_i64(&xr, x.num, x.den));
  POLY_TRY(st, canor_big_rat_set_i64(&half, 1, 2));
  /* Same range reduction as the Taylor path: Pade is accurate near 0 and the
   * squarings cost one multiply each. */
  while (st == CANOR_OK && sq < POLY_SQUARINGS_MAX) {
    int cmp;
    absx = xr;
    canor_big_abs(&absx.num);
    cmp = canor_big_rat_cmp(&absx, &half, &st);
    if (st || cmp <= 0) break;
    POLY_TRY(st, canor_big_rat_mul(&xr, &xr, &half));
    ++sq;
  }

  canor_big_rat_init(&num);
  canor_big_rat_init(&den);
  POLY_TRY(st, canor_big_rat_set_i64(&xp, 1, 1));
  for (j = 0; st == CANOR_OK && j <= top; ++j) {
    int64_t binm = 1, binn = 1, fall = 1;
    for (i = 0; i < j; ++i) { binm = binm * (m - i) / (i + 1); binn = binn * (n - i) / (i + 1); }
    /* (m+n-j)!/(m+n)! = 1 / ((m+n)(m+n-1)...(m+n-j+1)) */
    for (i = 0; i < j; ++i) fall *= (m + n - i);
    if (j <= m) {
      POLY_TRY(st, canor_big_rat_set_i64(&cf, binm, fall));
      POLY_TRY(st, canor_big_rat_mul(&term, &xp, &cf));
      POLY_TRY(st, canor_big_rat_add(&num, &num, &term));
    }
    if (j <= n) {
      POLY_TRY(st, canor_big_rat_set_i64(&cf, (j & 1) ? -binn : binn, fall));
      POLY_TRY(st, canor_big_rat_mul(&term, &xp, &cf));
      POLY_TRY(st, canor_big_rat_add(&den, &den, &term));
    }
    if (j < top) POLY_TRY(st, canor_big_rat_mul(&xp, &xp, &xr));
  }
  POLY_TRY(st, canor_big_rat_inv(&inv, &den));     /* a pole is UNDEFINED: refused */
  POLY_TRY(st, canor_big_rat_mul(&res, &num, &inv));
  for (i = 0; i < sq; ++i) POLY_TRY(st, canor_big_rat_mul(&res, &res, &res));
  if (squarings) *squarings = sq;
  if (st == CANOR_OK) *out = res;
  return st == CANOR_OK;
}

/* --- 1/Gamma, integer only ---------------------------------------------------
 * Gamma at a rational argument is irrational in general, so no exact route
 * exists; what matters is that the same rational is produced on every host.
 *
 * 1/Gamma(z) = sum_{k=1..26} c_k z^k (Abramowitz & Stegun 6.1.34). 1/Gamma is
 * entire, and on 0 < z <= 1 the 26 tabulated coefficients give it to about
 * 1e-16 absolute. The coefficients are exact integers over 10^16, so the sum is
 * exact rational arithmetic over a DECLARED table - no libm, no tgamma, and
 * bit-identical everywhere (AGENTS.md invariant 1). Any other argument is
 * carried into (0, 1] exactly by Gamma(z+1) = z.Gamma(z). */
#define POLY_RGAMMA_TERMS 26
#define POLY_RGAMMA_SCALE INT64_C(10000000000000000)   /* 10^16 */
static const int64_t poly_rgamma_c[POLY_RGAMMA_TERMS] = {
  INT64_C(10000000000000000), INT64_C(5772156649015329), INT64_C(-6558780715202538),
  INT64_C(-420026350340952),  INT64_C(1665386113822915), INT64_C(-421977345555443),
  INT64_C(-96219715278770),   INT64_C(72189432466630),   INT64_C(-11651675918591),
  INT64_C(-2152416741149),    INT64_C(1280502823882),    INT64_C(-201348547807),
  INT64_C(-12504934821),      INT64_C(11330272320),      INT64_C(-2056338417),
  INT64_C(61160950),          INT64_C(50020075),         INT64_C(-11812746),
  INT64_C(1043427),           INT64_C(77823),            INT64_C(-36968),
  INT64_C(5100),              INT64_C(-206),             INT64_C(-54),
  INT64_C(14),                INT64_C(1)
};
/* Unit steps the recurrence may take before the argument is refused as out
 * of range: Gamma(a.k + b) for the term counts used here stays far inside. */
#define POLY_RGAMMA_SHIFT_MAX 512
/* Significant bits kept in a quantized 1/Gamma. The table is good to ~2^-53,
 * so 64 bits carries all of its information while keeping the series'
 * denominators small enough to stay inside CANOR_BIG_LIMBS_MAX. */
#define POLY_RGAMMA_BITS 64

static canor_status poly_rgamma(const canor_big_rat_t *x, canor_big_rat_t *out) {
  canor_big_rat_t y, one, up, down, acc, yp, c, t;
  int steps = 0, k;
  canor_status st = CANOR_OK;

  y = *x;
  (void)canor_big_rat_set_i64(&one, 1, 1);   /* cannot fail: d != 0 */
  up = one;      /* product of factors that multiply 1/Gamma */
  down = one;    /* product of factors that divide it */
  /* z > 1:  1/Gamma(z) = 1/Gamma(z-1) / (z-1) */
  while (st == CANOR_OK && canor_big_rat_cmp_i64(&y, 1, &st) > 0) {
    if (++steps > POLY_RGAMMA_SHIFT_MAX) return CANOR_ERR_RANGE;
    POLY_TRY(st, canor_big_rat_sub(&y, &y, &one));
    POLY_TRY(st, canor_big_rat_mul(&down, &down, &y));
  }
  /* z <= 0:  1/Gamma(z) = z / Gamma(z+1). At a non-positive integer a factor
   * is zero and so is 1/Gamma - its exact value, not a guard. */
  while (st == CANOR_OK && canor_big_rat_cmp_i64(&y, 0, &st) <= 0) {
    if (++steps > POLY_RGAMMA_SHIFT_MAX) return CANOR_ERR_RANGE;
    POLY_TRY(st, canor_big_rat_mul(&up, &up, &y));
    POLY_TRY(st, canor_big_rat_add(&y, &y, &one));
  }
  if (st) return st;
  /* 0 < y <= 1: the series, Horner-free so each power is formed once. */
  canor_big_rat_init(&acc);
  yp = y;
  for (k = 0; st == CANOR_OK && k < POLY_RGAMMA_TERMS; ++k) {
    POLY_TRY(st, canor_big_rat_set_i64(&c, poly_rgamma_c[k], POLY_RGAMMA_SCALE));
    POLY_TRY(st, canor_big_rat_mul(&t, &c, &yp));
    POLY_TRY(st, canor_big_rat_add(&acc, &acc, &t));
    if (k + 1 < POLY_RGAMMA_TERMS) POLY_TRY(st, canor_big_rat_mul(&yp, &yp, &y));
  }
  POLY_TRY(st, canor_big_rat_mul(&acc, &acc, &up));
  POLY_TRY(st, canor_big_rat_inv(&down, &down));
  POLY_TRY(st, canor_big_rat_mul(&acc, &acc, &down));
  if (st) return st;

  /* Quantize to POLY_RGAMMA_BITS significant bits over a power of two:
   * num' = trunc(num . 2^s / den), den' = 2^s. Deterministic, and it stops
   * the exact Pochhammer denominators from compounding through the series. */
  if (!canor_big_is_zero(&acc.num)) {
    int s = POLY_RGAMMA_BITS - (canor_big_bits(&acc.num) - canor_big_bits(&acc.den));
    if (s > 0) {
      canor_big_t sh;
      POLY_TRY(st, canor_big_shl(&sh, &acc.num, (unsigned)s));
      POLY_TRY(st, canor_big_divmod(&acc.num, NULL, &sh, &acc.den));
      canor_big_set_i64(&sh, 1);
      POLY_TRY(st, canor_big_shl(&acc.den, &sh, (unsigned)s));
      POLY_TRY(st, canor_big_rat_normalize(&acc));
    }
  }
  if (st == CANOR_OK) *out = acc;
  return st;
}

/* --- Mittag-Leffler ---------------------------------------------------------- */

#define POLY_ML_TERMS_MIN 4
#define POLY_ML_TERMS_MAX 80
#define POLY_ML_WARMUP 4          /* terms before convergence may be declared */
/* A term below 2^-20 in magnitude, or 46 bits (~1e-14) under the peak term,
 * cannot move the sum at the precision reported - the same two thresholds the
 * floating-point version used, now as integer bit counts. */
#define POLY_ML_ABS_BITS 20
#define POLY_ML_REL_BITS 46
#define POLY_MILLI 1000

/* log2 |v| to within one bit: bits(num) - bits(den). */
static int rat_mag_bits(const canor_big_rat_t *v) {
  return canor_big_bits(&v->num) - canor_big_bits(&v->den);
}

bool canor_mittag_leffler(canor_rational alpha, canor_rational beta, canor_rational z,
                       int terms, canor_big_rat_t *out, canor_ml_report *rep) {
  canor_big_rat_t acc, zp, term, invg, zr, ar, br, arg, d;
  int k, converged = 0, have_peak = 0, peak_bits = 0;
  canor_status st = CANOR_OK;
  bool integer_args;

  if (!out || !canor_rat_valid(alpha) || !canor_rat_valid(beta) || !canor_rat_valid(z))
    return false;
  if (terms < POLY_ML_TERMS_MIN) terms = POLY_ML_TERMS_MIN;
  if (terms > POLY_ML_TERMS_MAX) terms = POLY_ML_TERMS_MAX;

  /* At alpha = beta = 1 the coefficient 1/k! is exact and is carried
   * incrementally - 1/(k+1)! = (1/k!) / (k+1) - so no factorial is formed.
   * Otherwise each 1/Gamma(a.k + b) comes from the declared integer series. */
  integer_args = (alpha.num == alpha.den && beta.num == beta.den);

  canor_big_rat_init(&acc);
  POLY_TRY(st, canor_big_rat_set_i64(&zp, 1, 1));
  POLY_TRY(st, canor_big_rat_set_i64(&invg, 1, 1));
  POLY_TRY(st, canor_big_rat_set_i64(&zr, z.num, z.den));
  POLY_TRY(st, canor_big_rat_set_i64(&ar, alpha.num, alpha.den));
  POLY_TRY(st, canor_big_rat_set_i64(&br, beta.num, beta.den));

  /* HOW MANY TERMS, MEASURED RATHER THAN GUESSED. At alpha = 1/2 the
   * denominator grows half as fast as k!, so the terms rise to a peak before
   * decaying; the loop stops once it is past the peak AND the term is
   * negligible against it, not at a fixed count. */
  for (k = 0; st == CANOR_OK && k < terms; ++k) {
    if (!integer_args) {
      POLY_TRY(st, canor_big_rat_set_i64(&d, k, 1));
      POLY_TRY(st, canor_big_rat_mul(&arg, &ar, &d));
      POLY_TRY(st, canor_big_rat_add(&arg, &arg, &br));
      POLY_TRY(st, poly_rgamma(&arg, &invg));
    }
    POLY_TRY(st, canor_big_rat_mul(&term, &zp, &invg));
    POLY_TRY(st, canor_big_rat_add(&acc, &acc, &term));
    if (st) break;
    {
      int mag = rat_mag_bits(&term);
      int64_t fx = 0;
      /* Negligible exactly when the term truncates to zero in Q20 - an exact
       * test, where the bit-count estimate above is only good to one bit. */
      bool negligible = canor_big_is_zero(&term.num) ||
                        (mag < 0 && canor_big_rat_to_fixed(&term.num, &term.den,
                                                           POLY_ML_ABS_BITS, &fx) == CANOR_OK &&
                         fx == 0);
      if (!negligible && (!have_peak || mag > peak_bits)) { peak_bits = mag; have_peak = 1; }
      else if (k > POLY_ML_WARMUP && have_peak &&
               (negligible || peak_bits - mag > POLY_ML_REL_BITS)) {
        converged = 1;
        ++k;
        break;
      }
    }
    POLY_TRY(st, canor_big_rat_mul(&zp, &zp, &zr));
    if (integer_args) {
      POLY_TRY(st, canor_big_rat_set_i64(&d, 1, (int64_t)k + 1));
      POLY_TRY(st, canor_big_rat_mul(&invg, &invg, &d));
    }
  }
  /* Running out of terms without converging is REFUSED: the partial sum of a
   * series still swinging by units is not an answer. Capacity exhaustion is
   * refused the same way. */
  if (rep) {
    rep->ok = (st == CANOR_OK && converged);
    rep->terms = k;
    rep->deterministic = true;
    rep->bound_log2_milli = -(int64_t)POLY_MILLI * k;
  }
  if (st != CANOR_OK || !converged) return false;
  *out = acc;
  return true;
}

/* t^alpha needs a root; it is taken exactly as an integer q-th root, so the
 * argument is floor(t^alpha . 10^6) on every host. */
#define POLY_RELAX_SCALE_DIGITS 6
#define POLY_RELAX_SCALE INT64_C(1000000)
#define POLY_ROOT_DEGREE_MAX 64
#define POLY_DEC_BASE 10

bool canor_fractional_relax(canor_rational alpha, canor_rational lambda, canor_rational t,
                         int terms, canor_big_rat_t *out, canor_ml_report *rep) {
  /* E(alpha, 1; -lambda . t^alpha). For fractional alpha = p/q, t^alpha is
   * irrational in general and is formed as the DECLARED approximation
   *     floor( (t.num^p . 10^(6q) / t.den^p)^(1/q) ) / 10^6,
   * computed exactly by integer root extraction - the same bargain as Gamma
   * above, stated the same way, and integer only. */
  canor_big_t a, b, ten, scale, x, r;
  int64_t ta, argnum;
  canor_status st = CANOR_OK;
  canor_rational arg;
  if (!canor_rat_valid(alpha) || !canor_rat_valid(lambda) || !canor_rat_valid(t)) return false;
  if (alpha.num < 0 || t.num < 0 || alpha.den > POLY_ROOT_DEGREE_MAX) return false;

  canor_big_set_i64(&a, t.num);
  canor_big_set_i64(&b, t.den);
  canor_big_set_i64(&ten, POLY_DEC_BASE);
  POLY_TRY(st, canor_big_pow_u32(&a, &a, (uint32_t)alpha.num));
  POLY_TRY(st, canor_big_pow_u32(&b, &b, (uint32_t)alpha.num));
  POLY_TRY(st, canor_big_pow_u32(&scale, &ten, (uint32_t)(POLY_RELAX_SCALE_DIGITS * alpha.den)));
  POLY_TRY(st, canor_big_mul(&x, &a, &scale));
  POLY_TRY(st, canor_big_divmod(&x, NULL, &x, &b));
  POLY_TRY(st, canor_big_iroot(&r, &x, (uint32_t)alpha.den));
  POLY_TRY(st, canor_big_mul_i64(&r, &r, lambda.num));
  canor_big_set_i64(&b, lambda.den);
  POLY_TRY(st, canor_big_divmod(&r, NULL, &r, &b));   /* toward zero */
  POLY_TRY(st, canor_big_to_i64(&r, &ta));
  if (st != CANOR_OK || ta == INT64_MIN) return false;
  argnum = -ta;
  arg = canor_rat(argnum, POLY_RELAX_SCALE);
  if (!canor_rat_valid(arg)) return false;
  return canor_mittag_leffler(alpha, canor_rat(1, 1), arg, terms, out, rep);
}
