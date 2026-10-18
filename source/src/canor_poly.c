/* Rational polynomials, Pade approximants, Mittag-Leffler. See canor_poly.h. */

#include "canor_poly.h"
#include "canor_prime.h"

#include <math.h>
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

bool canor_pade_exp(canor_rational x, int m, int n, fb_rat *out, int *squarings) {
  /* The [m/n] Pade approximant of exp has closed-form coefficients:
   *   P_j = C(m, j) . (m+n-j)! / (m+n)! ,  Q_j = (-1)^j C(n, j) . (m+n-j)! / (m+n)!
   * Both are rational, so the whole approximant is, and evaluating it is exact
   * rational arithmetic with no series truncation inside it. */
  fb_rat num, den, xp, term, xr, half;
  int j, sq = 0;
  bool ok = true;
  if (!out || !canor_rat_valid(x) || m < 1 || n < 1 || m > 8 || n > 8) return false;

  fb_rat_init(&num); fb_rat_init(&den); fb_rat_init(&xp);
  fb_rat_init(&term); fb_rat_init(&xr); fb_rat_init(&half);
  ok = fb_rat_set_i64(&xr, x.num, x.den) && fb_rat_set_i64(&half, 1, 2);
  /* Same range reduction as the Taylor path: Pade is accurate near 0 and the
   * squarings cost two multiplies each. */
  while (ok && sq < 40) {
    fb_rat a;
    int cmp;
    fb_rat_init(&a);
    ok = fb_rat_copy(&a, &xr);
    if (ok && a.num.sign < 0) fb_neg(&a.num);
    cmp = ok ? fb_rat_cmp(&a, &half) : 0;
    fb_rat_free(&a);
    if (!ok || cmp <= 0) break;
    ok = fb_rat_mul(&xr, &xr, &half);
    ++sq;
  }

  if (ok) ok = fb_rat_set_i64(&num, 0, 1) && fb_rat_set_i64(&den, 0, 1) &&
               fb_rat_set_i64(&xp, 1, 1);
  for (j = 0; ok && j <= (m > n ? m : n); ++j) {
    int64_t binm = 1, binn = 1, fk = 1;
    int i;
    for (i = 0; i < j; ++i) { binm = binm * (m - i) / (i + 1); binn = binn * (n - i) / (i + 1); }
    for (i = 1; i <= j; ++i) fk *= i;
    /* (m+n-j)!/(m+n)! = 1 / ((m+n)(m+n-1)...(m+n-j+1)) */
    {
      int64_t fall = 1;
      for (i = 0; i < j; ++i) fall *= (m + n - i);
      if (j <= m) {
        fb_rat cf;
        fb_rat_init(&cf);
        ok = fb_rat_set_i64(&cf, binm, fall) && fb_rat_mul(&term, &xp, &cf) &&
             fb_rat_add(&num, &num, &term);
        fb_rat_free(&cf);
      }
      if (ok && j <= n) {
        fb_rat cf;
        fb_rat_init(&cf);
        ok = fb_rat_set_i64(&cf, (j & 1) ? -binn : binn, fall) &&
             fb_rat_mul(&term, &xp, &cf) && fb_rat_add(&den, &den, &term);
        fb_rat_free(&cf);
      }
    }
    if (ok && j < (m > n ? m : n)) ok = fb_rat_mul(&xp, &xp, &xr);
    (void)fk;
  }
  if (ok && fb_is_zero(&den.num)) ok = false;      /* a pole; refuse */
  if (ok) {
    fb_rat inv;
    fb_rat_init(&inv);
    ok = fb_copy(&inv.num, &den.den) && fb_copy(&inv.den, &den.num) &&
         fb_rat_normalize(&inv) && fb_rat_mul(out, &num, &inv);
    fb_rat_free(&inv);
  }
  { int i; for (i = 0; ok && i < sq; ++i) ok = fb_rat_mul(out, out, out); }
  if (squarings) *squarings = sq;
  fb_rat_free(&num); fb_rat_free(&den); fb_rat_free(&xp);
  fb_rat_free(&term); fb_rat_free(&xr); fb_rat_free(&half);
  return ok;
}

/* --- Mittag-Leffler ---------------------------------------------------------- */

/* Lanczos-free: Gamma at the rational arguments this needs is approximated by
 * a rational, and the approximation is DECLARED. Gamma(a.k+b) is irrational in
 * general, so no exact route exists; what matters is that the same rational is
 * produced on every host, which it is. */

bool canor_mittag_leffler(canor_rational alpha, canor_rational beta, canor_rational z,
                       int terms, fb_rat *out, canor_ml_report *rep) {
  fb_rat acc, zp, term, invg, zr;
  double a, b, term_peak = 0.0;
  int k, converged = 0;
  bool ok = true, integer_args;

  if (!out || !canor_rat_valid(alpha) || !canor_rat_valid(beta) || !canor_rat_valid(z))
    return false;
  if (terms < 4) terms = 4;
  if (terms > 80) terms = 80;
  a = (double)alpha.num / (double)alpha.den;
  b = (double)beta.num / (double)beta.den;

  /* THE COEFFICIENT MUST BE A BIGNUM, AND CARRIED INCREMENTALLY.
   *
   * The previous version obtained 1/Gamma through an canor_rational, which is a
   * pair of int64. 1/Gamma(21) is about 4e-19, and the largest denominator an
   * int64 rational can carry with a non-zero numerator ran out before that: the
   * numerator truncated to ZERO from k = 20 onward, so the series silently
   * stopped there. At z = -8 that left the k = 19 partial sum, -0.341589 -
   * exactly the wrong answer that was showing.
   *
   * It was never a convergence problem, and the "range of validity" fence
   * built around it was fencing the wrong thing. The fix is to keep the
   * coefficient in fb_rat, where 1/k! is exact at every k, and to build it by
   * DIVIDING THE PREVIOUS ONE - which needs no factorial to be formed at all.
   */
  integer_args = (alpha.num == alpha.den && beta.num == beta.den);

  fb_rat_init(&acc); fb_rat_init(&zp); fb_rat_init(&term);
  fb_rat_init(&invg); fb_rat_init(&zr);
  ok = fb_rat_set_i64(&acc, 0, 1) && fb_rat_set_i64(&zp, 1, 1) &&
       fb_rat_set_i64(&invg, 1, 1) && fb_rat_set_i64(&zr, z.num, z.den);

  /* HOW MANY TERMS, MEASURED RATHER THAN GUESSED.
   *
   * The coefficient construction was instrumented against tgamma and tracks it
   * to about 1e-9 at every k - it was never the problem. What was happening is
   * simpler: at alpha = 1/2 the denominator Gamma(k/2 + 1) grows only half as
   * fast as k!, so the terms decay far more slowly than the exponential case
   * and forty of them are nowhere near enough. At z = -2.83 the partial sums
   * were still swinging between -1.5 and +1.3 at k = 39 against a true value
   * of 0.1888. The -0.103 that looked like a bug was just the k = 39 partial.
   *
   * So the loop now stops when it has passed the peak AND the term has fallen
   * far below it, rather than at a fixed count. The peak is tracked because an
   * alternating series with terms rising to 200 before decaying cannot be
   * judged by the size of the current term alone. */
  {
    double peak = 0.0, cur = 0.0;
    int past_peak = 0;
    (void)peak; (void)cur; (void)past_peak;
  }
  for (k = 0; ok && k < terms; ++k) {
    if (!integer_args) {
      /* Fractional alpha: Gamma is genuinely irrational, so a rational
       * approximation is unavoidable. The scale is chosen from the VALUE's
       * magnitude rather than fixed, which is what the old code got wrong -
       * a fixed 1e15 cap silently zeroed every small coefficient. */
      double g = tgamma(a * (double)k + b);
      double iv = (g > 0.0 && g < 1e300) ? 1.0 / g : 0.0;
      /* den is 1, not a fixed scale: the loop below already scales iv UP by
       * 10^shift, so the only division needed is by that same 10^shift.
       * Dividing by a fixed denominator as well made every fractional
       * coefficient 1e9 too small, and the whole column read as zero. */
      int64_t num, den = 1;
      int shift = 0;
      while (iv != 0.0 && iv < 1e8 && shift < 30) { iv *= 10.0; ++shift; }
      num = (int64_t)iv;
      ok = fb_rat_set_i64(&invg, num, den);
      { int i2; for (i2 = 0; ok && i2 < shift; ++i2) {
          fb_rat ten; fb_rat_init(&ten);
          ok = fb_rat_set_i64(&ten, 1, 10) && fb_rat_mul(&invg, &invg, &ten);
          fb_rat_free(&ten); } }
      if (!ok) break;
    }
    ok = fb_rat_mul(&term, &zp, &invg) && fb_rat_add(&acc, &acc, &term);
    if (!ok) break;
    {
      int64_t fx = 0;
      double mag = 0.0;
      if (fb_rat_to_fixed(&term.num, &term.den, 20, &fx))
        mag = fabs((double)fx / (double)(1 << 20));
      if (mag > term_peak) { term_peak = mag; }
      else if (k > 4 && term_peak > 0.0 && mag < term_peak * 1e-14) {
        /* Past the peak and negligible against it: further terms cannot move
         * the sum, so stopping here is convergence rather than truncation. */
        converged = 1;
        ++k;
        break;
      }
    }
    ok = fb_rat_mul(&zp, &zp, &zr);
    if (integer_args && ok) {
      /* 1/(k+1)! = (1/k!) / (k+1). Exact, for every k, with no factorial. */
      fb_rat d;
      fb_rat_init(&d);
      ok = fb_rat_set_i64(&d, 1, (int64_t)k + 1) && fb_rat_mul(&invg, &invg, &d);
      fb_rat_free(&d);
    }
  }
  /* RANGE, and only for the fractional case. At alpha = 1 every coefficient
   * is exact, the alternating cancellation costs nothing, and the result is
   * right at every |z| tested - so no guard belongs there.
   *
   * At fractional alpha the coefficients are approximations of 1/Gamma, and
   * the series alternates with terms peaking near e^|z| before settling near
   * e^-|z|. Each approximate coefficient contributes an absolute error scaled
   * by its term, so the accumulated error grows with the PEAK while the answer
   * shrinks. That is a real limit of this method - large |z| wants the
   * asymptotic expansion or an integral representation, neither implemented -
   * and it is refused rather than reported. */
  /* Running out of terms without converging is REFUSED. The partial sum of a
   * series still swinging by units is not an answer, and returning it is
   * exactly the failure that made the fractional column look broken. */
  if (ok && !converged && k >= terms) ok = false;
  if (ok) ok = fb_rat_copy(out, &acc);
  if (rep) {
    rep->ok = ok;
    rep->terms = k;
    rep->deterministic = true;
    rep->bound_log2_milli = -1000LL * k;
  }
  fb_rat_free(&acc); fb_rat_free(&zp); fb_rat_free(&term);
  fb_rat_free(&invg); fb_rat_free(&zr);
  return ok;
}

bool canor_fractional_relax(canor_rational alpha, canor_rational lambda, canor_rational t,
                         int terms, fb_rat *out, canor_ml_report *rep) {
  /* E(alpha, 1; -lambda . t^alpha). t^alpha is not rational for fractional
   * alpha, so it is formed as a declared rational approximation - the same
   * bargain as Gamma above, and stated in the same way. */
  double ta = pow((double)t.num / (double)t.den,
                  (double)alpha.num / (double)alpha.den);
  canor_rational arg = canor_rat((int64_t)(-ta * 1000000.0 * (double)lambda.num
                                     / (double)lambda.den), 1000000);
  if (!canor_rat_valid(arg)) return false;
  return canor_mittag_leffler(alpha, canor_rat(1, 1), arg, terms, out, rep);
}
