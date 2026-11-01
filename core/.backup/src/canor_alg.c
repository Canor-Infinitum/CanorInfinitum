/* Algebraic numbers. See canor_alg.h. Everything here is exact rational
 * arithmetic; the only approximation any caller ever sees is the width of an
 * isolating interval, and that is reported rather than hidden. */

#include "canor_alg.h"

#include <string.h>

static canor_rational rneg(canor_rational a) { return canor_rat(-a.num, a.den); }
static canor_rational rsub(canor_rational a, canor_rational b) { return canor_rat_add(a, rneg(b)); }
static canor_rational rinv(canor_rational a) {
  if (a.num == 0) return canor_rat(0, 0);
  return canor_rat(a.den, a.num);
}

bool canor_poly_derivative(const canor_poly *p, canor_poly *out) {
  int i;
  if (!p || !out) return false;
  canor_poly_init(out);
  if (p->degree < 1) { out->degree = 0; return true; }
  for (i = 1; i <= p->degree; ++i) {
    canor_rational m = canor_rat_mul(p->c[i], canor_rat(i, 1));
    if (!canor_rat_valid(m)) return false;
    out->c[i - 1] = m;
  }
  out->degree = p->degree - 1;
  while (out->degree > 0 && out->c[out->degree].num == 0) --out->degree;
  return true;
}

bool canor_poly_divmod(const canor_poly *a, const canor_poly *b, canor_poly *q, canor_poly *r) {
  canor_poly rem;
  int i, shift;
  if (!a || !b || b->degree < 0 || b->c[b->degree].num == 0) return false;
  rem = *a;
  if (q) canor_poly_init(q);
  while (rem.degree >= b->degree && !(rem.degree == 0 && rem.c[0].num == 0)) {
    canor_rational factor = canor_rat_mul(rem.c[rem.degree], rinv(b->c[b->degree]));
    if (!canor_rat_valid(factor)) return false;
    shift = rem.degree - b->degree;
    if (q && shift < CANOR_POLY_MAX) {
      q->c[shift] = factor;
      if (shift > q->degree) q->degree = shift;
    }
    for (i = 0; i <= b->degree; ++i) {
      canor_rational t = canor_rat_mul(b->c[i], factor);
      if (!canor_rat_valid(t)) return false;
      rem.c[i + shift] = rsub(rem.c[i + shift], t);
      if (!canor_rat_valid(rem.c[i + shift])) return false;
    }
    /* The leading term must cancel exactly; if it has not, the arithmetic
     * has gone wrong and looping forever would hide that. */
    if (rem.c[rem.degree].num != 0) return false;
    while (rem.degree > 0 && rem.c[rem.degree].num == 0) --rem.degree;
    if (rem.degree == 0 && rem.c[0].num == 0) break;
  }
  if (r) *r = rem;
  return true;
}

bool canor_poly_primitive(canor_poly *p) {
  /* Scales by the lcm of denominators and divides by the gcd of numerators.
   * Sturm cares only about SIGNS, so this changes nothing it depends on while
   * keeping the coefficients from growing through the chain. */
  int64_t lcm = 1, g = 0;
  int i;
  if (!p) return false;
  for (i = 0; i <= p->degree; ++i) {
    int64_t d = p->c[i].den, a = lcm, b = d, t;
    while (b) { t = a % b; a = b; b = t; }
    if (a == 0) a = 1;
    if (lcm > 4000000000000000000LL / (d / a)) return false;
    lcm = lcm / a * d;
  }
  for (i = 0; i <= p->degree; ++i) {
    canor_rational m = canor_rat_mul(p->c[i], canor_rat(lcm, 1));
    if (!canor_rat_valid(m) || m.den != 1) return false;
    p->c[i] = m;
    {
      int64_t a = g < 0 ? -g : g, b = m.num < 0 ? -m.num : m.num, t;
      while (b) { t = a % b; a = b; b = t; }
      g = a;
    }
  }
  if (g > 1) for (i = 0; i <= p->degree; ++i) p->c[i] = canor_rat(p->c[i].num / g, 1);
  return true;
}

bool canor_sturm_build(const canor_poly *p, canor_sturm *out) {
  canor_poly d, r;
  int i;
  if (!p || !out || p->degree < 1) return false;
  memset(out, 0, sizeof *out);
  out->p[0] = *p;
  if (!canor_poly_derivative(p, &d)) return false;
  out->p[1] = d;
  out->n = 2;
  for (i = 2; i < CANOR_STURM_MAX; ++i) {
    if (out->p[i - 1].degree == 0 && out->p[i - 1].c[0].num == 0) break;
    if (!canor_poly_divmod(&out->p[i - 2], &out->p[i - 1], NULL, &r)) return false;
    if (r.degree == 0 && r.c[0].num == 0) break;
    { int k; for (k = 0; k <= r.degree; ++k) r.c[k] = rneg(r.c[k]); }
    canor_poly_primitive(&r);
    out->p[i] = r;
    out->n = i + 1;
    if (r.degree == 0) break;
  }
  return true;
}

int canor_sturm_variations(const canor_sturm *s, canor_rational x) {
  int i, v = 0, last = 0;
  for (i = 0; i < s->n; ++i) {
    canor_rational y = canor_poly_eval(&s->p[i], x);
    int sg;
    if (!canor_rat_valid(y) || y.num == 0) continue;   /* zeros are skipped */
    sg = y.num > 0 ? 1 : -1;
    if (last != 0 && sg != last) ++v;
    last = sg;
  }
  return v;
}

int canor_sturm_count(const canor_sturm *s, canor_rational a, canor_rational b) {
  return canor_sturm_variations(s, a) - canor_sturm_variations(s, b);
}

bool canor_alg_root_bound(const canor_poly *p, canor_rational *B) {
  /* Cauchy: |root| <= 1 + max|a_i / a_n|. Exact, and it removes any need to
   * guess a search window. */
  canor_rational lead, m = canor_rat(0, 1);
  int i;
  if (!p || !B || p->degree < 1) return false;
  lead = p->c[p->degree];
  if (lead.num == 0) return false;
  for (i = 0; i < p->degree; ++i) {
    canor_rational q = canor_rat_mul(p->c[i], rinv(lead));
    if (!canor_rat_valid(q)) return false;
    if (q.num < 0) q = rneg(q);
    if (canor_rat_cmp(q, m) > 0) m = q;
  }
  *B = canor_rat_add(m, canor_rat(1, 1));
  return canor_rat_valid(*B);
}

bool canor_alg_isolate(const canor_poly *p, canor_alg *out, int cap, int *found) {
  canor_sturm s;
  canor_rational B, lo, hi;
  canor_rational stack_lo[64], stack_hi[64];
  int sp = 0, n = 0, guard = 0;

  if (!p || !out || !found || p->degree < 1) return false;
  *found = 0;
  if (!canor_sturm_build(p, &s)) return false;
  if (!canor_alg_root_bound(p, &B)) return false;
  lo = rneg(B);
  hi = B;

  stack_lo[sp] = lo;
  stack_hi[sp] = hi;
  ++sp;
  while (sp > 0 && n < cap && guard++ < 4000) {
    canor_rational a = stack_lo[--sp], b = stack_hi[sp];
    int c = canor_sturm_count(&s, a, b);
    if (c <= 0) continue;
    if (c == 1) {
      /* Exactly one root, verified by Sturm rather than inferred from a sign
       * change - which would only have shown an ODD number. */
      out[n].min = *p;
      out[n].lo = a;
      out[n].hi = b;
      out[n].exact_rational = false;
      if (canor_poly_eval(p, b).num == 0) {
        out[n].lo = b; out[n].hi = b; out[n].exact_rational = true;
      }
      ++n;
      continue;
    }
    if (sp + 2 < 64) {
      canor_rational mid = canor_rat_mul(canor_rat_add(a, b), canor_rat(1, 2));
      if (!canor_rat_valid(mid)) continue;
      stack_lo[sp] = a; stack_hi[sp] = mid; ++sp;
      stack_lo[sp] = mid; stack_hi[sp] = b; ++sp;
    }
  }
  /* ASCENDING. The search stack pops last-in-first-out, so roots emerged in
   * descending order - which is not wrong but is not what any caller expects,
   * and taking "the positive root" as the last element then silently picked
   * the negative one. Sorted here so the order is a property of the result
   * rather than of the traversal. */
  {
    int i2, j2;
    for (i2 = 0; i2 < n; ++i2)
      for (j2 = i2 + 1; j2 < n; ++j2)
        if (canor_rat_cmp(out[j2].lo, out[i2].lo) < 0) {
          canor_alg t = out[i2]; out[i2] = out[j2]; out[j2] = t;
        }
  }
  *found = n;
  return true;
}

bool canor_alg_refine(canor_alg *a, int iters) {
  int i;
  if (!a) return false;
  if (a->exact_rational) return true;
  /* THE INT64 CEILING IS REAL AND IS NOT AN ERROR. Each halving doubles the
   * denominator, and evaluating a degree-d polynomial there raises it to the
   * d-th power, so a quadratic exhausts int64 after about thirty halvings.
   * STOP and keep the last valid interval rather than failing: a width of
   * 2^-28 known exactly beats an error, and canor_alg_approx reports the width so
   * a caller sees how far the narrowing actually got. Returning false here
   * discarded the refinement entirely and left intervals several units wide. */
  for (i = 0; i < iters; ++i) {
    canor_rational mid = canor_rat_mul(canor_rat_add(a->lo, a->hi), canor_rat(1, 2));
    canor_rational fm, fl;
    if (!canor_rat_valid(mid)) return true;
    fm = canor_poly_eval(&a->min, mid);
    if (!canor_rat_valid(fm)) return true;
    if (fm.num == 0) { a->lo = a->hi = mid; a->exact_rational = true; return true; }
    fl = canor_poly_eval(&a->min, a->lo);
    if (!canor_rat_valid(fl)) return true;
    if ((fl.num > 0) != (fm.num > 0)) a->hi = mid;
    else a->lo = mid;
  }
  return true;
}

canor_rational canor_alg_approx(const canor_alg *a, canor_rational *width) {
  if (width) *width = rsub(a->hi, a->lo);
  return canor_rat_mul(canor_rat_add(a->lo, a->hi), canor_rat(1, 2));
}

/* --- Q(alpha) ---------------------------------------------------------------- */

void canor_alg_elem_init(canor_alg_elem *e) {
  int i;
  memset(e, 0, sizeof *e);
  for (i = 0; i < CANOR_POLY_MAX; ++i) e->c[i] = canor_rat(0, 1);
}

bool canor_alg_elem_add(const canor_alg *f, const canor_alg_elem *a, const canor_alg_elem *b,
                     canor_alg_elem *out) {
  int i, d;
  (void)f;
  if (!a || !b || !out) return false;
  canor_alg_elem_init(out);
  d = a->degree > b->degree ? a->degree : b->degree;
  for (i = 0; i <= d; ++i) {
    out->c[i] = canor_rat_add(a->c[i], b->c[i]);
    if (!canor_rat_valid(out->c[i])) return false;
  }
  out->degree = d;
  while (out->degree > 0 && out->c[out->degree].num == 0) --out->degree;
  return true;
}

bool canor_alg_elem_mul(const canor_alg *f, const canor_alg_elem *a, const canor_alg_elem *b,
                     canor_alg_elem *out) {
  canor_poly prod, rem, min;
  int i, j;
  if (!f || !a || !b || !out) return false;
  canor_poly_init(&prod);
  for (i = 0; i <= a->degree; ++i)
    for (j = 0; j <= b->degree; ++j) {
      canor_rational t;
      if (i + j >= CANOR_POLY_MAX) return false;
      t = canor_rat_mul(a->c[i], b->c[j]);
      if (!canor_rat_valid(t)) return false;
      prod.c[i + j] = canor_rat_add(prod.c[i + j], t);
      if (!canor_rat_valid(prod.c[i + j])) return false;
      if (i + j > prod.degree) prod.degree = i + j;
    }
  /* Reduce modulo the minimal polynomial: that is what keeps Q(alpha) closed
   * and is the whole reason the field is finite-dimensional. */
  min = f->min;
  if (!canor_poly_divmod(&prod, &min, NULL, &rem)) return false;
  canor_alg_elem_init(out);
  for (i = 0; i <= rem.degree && i < CANOR_POLY_MAX; ++i) out->c[i] = rem.c[i];
  out->degree = rem.degree;
  return true;
}

canor_rational canor_alg_elem_approx(const canor_alg *f, const canor_alg_elem *e) {
  canor_rational x = canor_alg_approx(f, NULL), acc = canor_rat(0, 1);
  int i;
  for (i = e->degree; i >= 0; --i) {
    acc = canor_rat_mul(acc, x);
    if (!canor_rat_valid(acc)) return acc;
    acc = canor_rat_add(acc, e->c[i]);
    if (!canor_rat_valid(acc)) return acc;
  }
  return acc;
}
