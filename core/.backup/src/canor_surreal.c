/* Surreal values in Conway normal form. See canor_surreal.h. */

#include "canor_surreal.h"

#include <stdio.h>
#include <string.h>

static canor_rational rneg(canor_rational a) { return canor_rat(-a.num, a.den); }

void canor_surreal_zero(canor_surreal *x) {
  if (!x) return;
  x->n = 0;
  x->valid = true;
}

bool canor_surreal_term(canor_surreal *x, canor_rational e, canor_rational c) {
  if (!x) return false;
  canor_surreal_zero(x);
  if (!canor_rat_valid(e) || !canor_rat_valid(c)) { x->valid = false; return false; }
  /* A zero coefficient is not a term. Normal form has no zero terms, and
   * admitting one would make two equal values compare unequal by term count. */
  if (c.num == 0) return true;
  x->t[0].exp = e;
  x->t[0].coef = c;
  x->n = 1;
  return true;
}

bool canor_surreal_real(canor_surreal *x, canor_rational r) {
  return canor_surreal_term(x, canor_rat(0, 1), r);
}
bool canor_surreal_omega(canor_surreal *x) {
  return canor_surreal_term(x, canor_rat(1, 1), canor_rat(1, 1));
}
bool canor_surreal_eps(canor_surreal *x) {
  return canor_surreal_term(x, canor_rat(-1, 1), canor_rat(1, 1));
}

bool canor_surreal_valid(const canor_surreal *x) { return x && x->valid; }

/* Insert omega^e . c, keeping exponents strictly decreasing and dropping a
 * term whose coefficient cancels to zero. Refuses rather than truncating when
 * the form needs more room: a dropped high-order term is a silently wrong
 * magnitude, which is worse than a refusal a caller can see. */
static bool fs_accum(canor_surreal *o, canor_rational e, canor_rational c) {
  int i, j;
  if (!o->valid) return false;
  if (!canor_rat_valid(e) || !canor_rat_valid(c)) { o->valid = false; return false; }
  if (c.num == 0) return true;
  for (i = 0; i < o->n; ++i) {
    int r = canor_rat_cmp(e, o->t[i].exp);
    if (r == 0) {
      o->t[i].coef = canor_rat_add(o->t[i].coef, c);
      if (!canor_rat_valid(o->t[i].coef)) { o->valid = false; return false; }
      if (o->t[i].coef.num == 0) {
        for (j = i; j + 1 < o->n; ++j) o->t[j] = o->t[j + 1];
        --o->n;
      }
      return true;
    }
    if (r > 0) break;                    /* insert before a smaller exponent */
  }
  if (o->n >= FS_MAX_TERMS) { o->valid = false; return false; }
  for (j = o->n; j > i; --j) o->t[j] = o->t[j - 1];
  o->t[i].exp = e;
  o->t[i].coef = c;
  ++o->n;
  return true;
}

bool canor_surreal_add(const canor_surreal *a, const canor_surreal *b, canor_surreal *o) {
  int i;
  if (!a || !b || !o) return false;
  canor_surreal_zero(o);
  if (!a->valid || !b->valid) { o->valid = false; return false; }
  for (i = 0; i < a->n; ++i) if (!fs_accum(o, a->t[i].exp, a->t[i].coef)) return false;
  for (i = 0; i < b->n; ++i) if (!fs_accum(o, b->t[i].exp, b->t[i].coef)) return false;
  return true;
}

bool canor_surreal_neg(const canor_surreal *a, canor_surreal *o) {
  int i;
  if (!a || !o) return false;
  *o = *a;
  if (!a->valid) return false;
  for (i = 0; i < o->n; ++i) o->t[i].coef = rneg(o->t[i].coef);
  return true;
}

bool canor_surreal_sub(const canor_surreal *a, const canor_surreal *b, canor_surreal *o) {
  canor_surreal nb;
  if (!canor_surreal_neg(b, &nb)) return false;
  return canor_surreal_add(a, &nb, o);
}

bool canor_surreal_mul(const canor_surreal *a, const canor_surreal *b, canor_surreal *o) {
  int i, j;
  if (!a || !b || !o) return false;
  canor_surreal_zero(o);
  if (!a->valid || !b->valid) { o->valid = false; return false; }
  /* omega^p . omega^q = omega^(p+q), so the product is the convolution of the
   * term lists. This is where epsilon . omega = omega^(-1+1) = omega^0 = 1 -
   * the crossing from the infinitesimal band into the real one, which is the
   * whole mechanism the telemetry model rests on. */
  for (i = 0; i < a->n; ++i)
    for (j = 0; j < b->n; ++j) {
      canor_rational e = canor_rat_add(a->t[i].exp, b->t[j].exp);
      canor_rational c = canor_rat_mul(a->t[i].coef, b->t[j].coef);
      if (!fs_accum(o, e, c)) return false;
    }
  return true;
}

int canor_surreal_cmp(const canor_surreal *a, const canor_surreal *b) {
  int i;
  if (!a || !b || !a->valid || !b->valid) return 0;
  /* Compare term by term from the leading exponent. A term present on one side
   * and absent on the other is compared against zero at that exponent, which is
   * what makes 1 < 1 + epsilon rather than equal. */
  for (i = 0; i < a->n || i < b->n; ++i) {
    canor_rational ae = (i < a->n) ? a->t[i].exp : canor_rat(0, 1);
    canor_rational be = (i < b->n) ? b->t[i].exp : canor_rat(0, 1);
    canor_rational ac = (i < a->n) ? a->t[i].coef : canor_rat(0, 1);
    canor_rational bc = (i < b->n) ? b->t[i].coef : canor_rat(0, 1);
    if (i >= a->n) return (bc.num > 0) ? -1 : 1;
    if (i >= b->n) return (ac.num > 0) ? 1 : -1;
    {
      int r = canor_rat_cmp(ae, be);
      if (r != 0) {
        /* The side with the larger exponent dominates - but only in the
         * direction of its own sign. A large NEGATIVE leading term is smaller,
         * not larger, and reading magnitude as order would invert it. */
        if (r > 0) return (ac.num > 0) ? 1 : -1;
        return (bc.num > 0) ? -1 : 1;
      }
      r = canor_rat_cmp(ac, bc);
      if (r != 0) return r;
    }
  }
  return 0;
}

int canor_surreal_sign(const canor_surreal *x) {
  if (!x || !x->valid || x->n == 0) return 0;
  return x->t[0].coef.num > 0 ? 1 : -1;
}

bool canor_surreal_is_infinite(const canor_surreal *x) {
  if (!x || !x->valid || x->n == 0) return false;
  return x->t[0].exp.num > 0;
}

bool canor_surreal_is_real(const canor_surreal *x) {
  if (!x || !x->valid) return false;
  if (x->n == 0) return true;                 /* zero is real */
  return x->t[0].exp.num == 0 && x->n == 1;
}

bool canor_surreal_is_infinitesimal(const canor_surreal *x) {
  if (!x || !x->valid || x->n == 0) return false;   /* zero is NOT epsilon */
  return x->t[0].exp.num < 0;
}

canor_rational canor_surreal_real_part(const canor_surreal *x) {
  int i;
  if (!x || !x->valid) return canor_rat(0, 1);
  for (i = 0; i < x->n; ++i)
    if (x->t[i].exp.num == 0) return x->t[i].coef;
  return canor_rat(0, 1);
}

void canor_surreal_format(const canor_surreal *x, char *buf, int cap) {
  int i, k = 0;
  if (!buf || cap < 8) return;
  buf[0] = 0;
  if (!x || !x->valid) { snprintf(buf, (size_t)cap, "<invalid>"); return; }
  if (x->n == 0) { snprintf(buf, (size_t)cap, "0"); return; }
  for (i = 0; i < x->n && k < cap - 24; ++i) {
    const char *sep = (i && x->t[i].coef.num > 0) ? "+" : "";
    char c[40];
    if (x->t[i].coef.den == 1)
      snprintf(c, sizeof c, "%lld", (long long)x->t[i].coef.num);
    else
      snprintf(c, sizeof c, "%lld/%lld", (long long)x->t[i].coef.num,
               (long long)x->t[i].coef.den);
    if (x->t[i].exp.num == 0)
      k += snprintf(buf + k, (size_t)(cap - k), "%s%s", sep, c);
    else if (x->t[i].exp.num == 1 && x->t[i].exp.den == 1)
      k += snprintf(buf + k, (size_t)(cap - k), "%s%s*w", sep, c);
    else if (x->t[i].exp.num == -1 && x->t[i].exp.den == 1)
      k += snprintf(buf + k, (size_t)(cap - k), "%s%s*e", sep, c);
    else
      k += snprintf(buf + k, (size_t)(cap - k), "%s%s*w^(%lld/%lld)", sep, c,
                    (long long)x->t[i].exp.num, (long long)x->t[i].exp.den);
  }
}
