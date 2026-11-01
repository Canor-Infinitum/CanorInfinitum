/* Generalised symbolic graphing. See fc_sym.h for why the axis is measured
 * rather than declared.
 *
 * Evaluation is in fc_rational throughout, so it inherits the poison
 * behaviour: an overflow produces a value that fails fc_rat_valid and
 * propagates, rather than a wrapped number that looks plottable. A symbolic
 * grapher that silently plotted wrapped arithmetic would be worse than one
 * that refused, because the plot would still look like a curve.
 */

#include "fc_sym.h"

#include <math.h>
#include <string.h>

void fc_sym_init(fc_sym *e) {
  memset(e, 0, sizeof *e);
  e->root = -1;
}

static int sym_push(fc_sym *e, fc_sym_op op, int a, int b, int k,
                    fc_rational v) {
  fc_sym_term *t;
  if (e->n >= FC_SYM_MAX) return -1;
  t = &e->t[e->n];
  t->op = op;
  t->a = a;
  t->b = b;
  t->k = k;
  t->value = v;
  e->root = e->n;
  return e->n++;
}

int fc_sym_const(fc_sym *e, fc_rational v) { return sym_push(e, FC_SYM_CONST, -1, -1, 0, v); }
int fc_sym_var(fc_sym *e) { return sym_push(e, FC_SYM_VAR, -1, -1, 0, fc_rat(0, 1)); }
int fc_sym_bin(fc_sym *e, fc_sym_op op, int a, int b) {
  if (a < 0 || b < 0 || a >= e->n || b >= e->n) return -1;
  return sym_push(e, op, a, b, 0, fc_rat(0, 1));
}
int fc_sym_neg(fc_sym *e, int a) {
  if (a < 0 || a >= e->n) return -1;
  return sym_push(e, FC_SYM_NEG, a, -1, 0, fc_rat(0, 1));
}
int fc_sym_pow(fc_sym *e, int a, int k) {
  if (a < 0 || a >= e->n) return -1;
  return sym_push(e, FC_SYM_POW, a, -1, k, fc_rat(0, 1));
}
void fc_sym_set_root(fc_sym *e, int idx) { if (idx >= 0 && idx < e->n) e->root = idx; }

static fc_rational sym_eval_at(const fc_sym *e, int idx, fc_rational t, int depth) {
  const fc_sym_term *n;
  fc_rational a, b;
  /* Terms only ever reference EARLIER indices, so the graph is acyclic by
   * construction; the depth guard is belt and braces against a hand-built
   * pool that violated that. */
  if (idx < 0 || idx >= e->n || depth > FC_SYM_MAX) return fc_rat(0, 0);
  n = &e->t[idx];
  switch (n->op) {
    case FC_SYM_CONST: return n->value;
    case FC_SYM_VAR:   return t;
    case FC_SYM_NEG:
      a = sym_eval_at(e, n->a, t, depth + 1);
      if (!fc_rat_valid(a)) return a;
      return fc_rat(-a.num, a.den);
    case FC_SYM_POW: {
      int i;
      fc_rational r = fc_rat(1, 1);
      a = sym_eval_at(e, n->a, t, depth + 1);
      if (!fc_rat_valid(a)) return a;
      if (n->k < 0) {
        if (a.num == 0) return fc_rat(0, 0);       /* negative power of zero */
        a = fc_rat(a.den, a.num);
        for (i = 0; i < -n->k; ++i) { r = fc_rat_mul(r, a); if (!fc_rat_valid(r)) return r; }
        return r;
      }
      for (i = 0; i < n->k; ++i) { r = fc_rat_mul(r, a); if (!fc_rat_valid(r)) return r; }
      return r;
    }
    default: break;
  }
  a = sym_eval_at(e, n->a, t, depth + 1);
  if (!fc_rat_valid(a)) return a;
  b = sym_eval_at(e, n->b, t, depth + 1);
  if (!fc_rat_valid(b)) return b;
  switch (n->op) {
    case FC_SYM_ADD: return fc_rat_add(a, b);
    case FC_SYM_SUB: return fc_rat_add(a, fc_rat(-b.num, b.den));
    case FC_SYM_MUL: return fc_rat_mul(a, b);
    case FC_SYM_DIV:
      if (b.num == 0) return fc_rat(0, 0);         /* refused, not infinity */
      return fc_rat_mul(a, fc_rat(b.den, b.num));
    default: return fc_rat(0, 0);
  }
}

fc_rational fc_sym_eval(const fc_sym *e, fc_rational t) {
  if (!e || e->root < 0) return fc_rat(0, 0);
  return sym_eval_at(e, e->root, t, 0);
}

/* --- growth and axes -------------------------------------------------------- */

const char *fc_growth_name(fc_growth g) {
  return g == FC_GROWTH_BOUNDED ? "bounded" :
         g == FC_GROWTH_POLY ? "polynomial" :
         g == FC_GROWTH_GEOMETRIC ? "geometric" :
         g == FC_GROWTH_DOUBLE ? "doubly exponential" : "undefined";
}
const char *fc_axis_name(fc_axis a) {
  return a == FC_AXIS_LINEAR ? "linear" :
         a == FC_AXIS_SIGNED_LOG ? "signed log" : "log-log (potential)";
}

double fc_axis_apply(fc_axis a, double v) {
  switch (a) {
    case FC_AXIS_SIGNED_LOG:
      return (v < 0 ? -1.0 : 1.0) * log10(1.0 + fabs(v));
    case FC_AXIS_LOGLOG: {
      /* log of log. The inner 1+ keeps it defined at small |v|; the outer one
       * keeps it defined where the inner log is below 1. This is the
       * Douady-Hubbard potential up to scale. */
      double inner = log10(1.0 + fabs(v));
      return (v < 0 ? -1.0 : 1.0) * log10(1.0 + inner);
    }
    default: return v;
  }
}

bool fc_sym_series(const fc_sym *e, int64_t lo_num, int64_t hi_num, int64_t den,
                   int nsamples, double *out_vals, fc_series *out) {
  int i;
  int64_t prev_bits = 0, prev_delta = 0;
  int rising = 0, accel = 0, valid = 0;
  double lo = 0.0, hi = 0.0;
  static double raw[4096];

  if (!e || !out || nsamples < 2 || nsamples > 4096 || den == 0) return false;
  memset(out, 0, sizeof *out);
  out->samples = nsamples;

  /* TWO DIFFERENT THINGS, and conflating them was a real error.
   *
   *   MAGNITUDE  |value|. This is what the axis must serve: whether the curve
   *              is bounded, or multiplies per step, or squares per step.
   *   WIDTH      bits of the RATIONAL REPRESENTATION. This is what decides
   *              feasibility - it is why the exact Mandelbrot dies at three
   *              iterations - and it is NOT a growth of the value.
   *
   * The first version classified on width, so 1/(t^2+1) - a bounded function
   * whose denominator grows without bound - came out "doubly exponential" and
   * was handed a log-log axis. Bounded values with fat representations are
   * common in exact arithmetic, so this was not an edge case. Magnitude
   * decides the axis; width is reported alongside as the cost. */
  for (i = 0; i < nsamples; ++i) {
    int64_t num = lo_num + (hi_num - lo_num) * i / (nsamples - 1);
    fc_rational t = fc_rat(num, den);
    fc_rational v = fc_sym_eval(e, t);
    int64_t bits = 0;
    double mag, lg;
    if (!fc_rat_valid(v)) { raw[i] = 0.0; continue; }
    ++valid;
    raw[i] = (double)v.num / (double)v.den;
    {
      int64_t m = v.num < 0 ? -v.num : v.num;
      if (m < v.den) m = v.den;
      while (m) { ++bits; m >>= 1; }
    }
    if (bits > out->max_bits) out->max_bits = bits;

    /* log2|value|, in milli-units, as an integer so the comparison below has
     * no floating-point tie-breaking in it. */
    mag = fabs(raw[i]);
    lg = (mag > 0.0) ? log(mag) / log(2.0) : -1000.0;
    {
      int64_t lgm = (int64_t)(lg * 1000.0);
      if (valid > 1) {
        int64_t d = lgm - prev_bits;
        if (d > 250) ++rising;                  /* value grew by >= 2^0.25 */
        if (valid > 2 && d > prev_delta + 250) ++accel;
        prev_delta = d;
      }
      prev_bits = lgm;
    }
  }
  out->valid = valid;
  if (valid < 2) {
    out->growth = FC_GROWTH_UNDEFINED;
    out->axis = FC_AXIS_LINEAR;
    return true;
  }

  /* Thresholds are fractions of the sample count, so the answer does not move
   * with resolution. */
  if (accel * 4 >= valid) out->growth = FC_GROWTH_DOUBLE;
  else if (rising * 2 >= valid) out->growth = FC_GROWTH_GEOMETRIC;
  else if (rising * 5 >= valid) out->growth = FC_GROWTH_POLY;
  else out->growth = FC_GROWTH_BOUNDED;

  out->axis = out->growth == FC_GROWTH_DOUBLE ? FC_AXIS_LOGLOG :
              out->growth == FC_GROWTH_GEOMETRIC ? FC_AXIS_SIGNED_LOG :
              FC_AXIS_LINEAR;
  if (valid) out->ratio_milli = (int64_t)(1000 * out->max_bits / valid);

  for (i = 0; i < nsamples; ++i) {
    double v = fc_axis_apply(out->axis, raw[i]);
    if (out_vals) out_vals[i] = v;
    if (i == 0 || v < lo) lo = v;
    if (i == 0 || v > hi) hi = v;
  }
  out->lo = lo;
  out->hi = hi;
  return true;
}


/* --- OrigoDelta as an expression --------------------------------------------- */

void fc_sympool_init(fc_sympool *p) { memset(p, 0, sizeof *p); }

int fc_sympool_add(fc_sympool *p, const fc_sym *e) {
  if (!p || !e || p->n >= FC_SYMPOOL_MAX) return -1;
  p->e[p->n] = *e;
  return p->n++;
}

static bool sym_at(const fc_sympool *p, int idx, fc_rational at, fc_rational *out) {
  fc_rational v;
  if (!p || idx < 0 || idx >= p->n) { *out = fc_rat(0, 1); return true; }  /* absent = 0 */
  v = fc_sym_eval(&p->e[idx], at);
  if (!fc_rat_valid(v)) return false;
  *out = v;
  return true;
}

bool fc_origo_expr_eval(const fc_sympool *p, const fc_origo_expr *oe,
                        fc_rational at, fc_origo *out) {
  if (!oe || !out) return false;
  /* All four or none. A boundary missing one coordinate is not a boundary. */
  if (!sym_at(p, oe->t, at, &out->t)) return false;
  if (!sym_at(p, oe->x, at, &out->x)) return false;
  if (!sym_at(p, oe->y, at, &out->y)) return false;
  if (!sym_at(p, oe->z, at, &out->z)) return false;
  return true;
}

bool fc_origo_expr_interval(const fc_sympool *p, const fc_origo_expr *oe,
                            fc_rational at, fc_rational *s2) {
  fc_origo o;
  fc_rational v;
  if (!fc_origo_expr_eval(p, oe, at, &o)) return false;
  v = fc_origo_interval2(&o);
  if (!fc_rat_valid(v)) return false;
  if (s2) *s2 = v;
  return true;
}
