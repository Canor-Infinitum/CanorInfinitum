/* Ore-algebraic rational skew-trigonometric operators. See vi_ore.h for why
 * these exist; this file is the recurrence and nothing more.
 *
 * ITERATION, NOT RECURSION. The pair is advanced with a two-term window, so
 * evaluating at n costs n steps and constant space. A doubling scheme would be
 * faster and would need the addition formulas, which is exactly the structure
 * section 13 declines to assume - so the slow version is also the honest one.
 *
 * EVERY STEP IS CHECKED. fc_rat_mul and fc_rat_add return poison rather than
 * wrapping, and poison propagates, so an aperiodic operator does not produce
 * wrong values before it produces no values. That is the whole reason the
 * rational overflow fix had to land before this could.
 */

#include "vi_ore.h"

#include <stdio.h>
#include <string.h>

/* --- construction ---------------------------------------------------------- */

/* Parses "n", "-n" or "n/d" into an fc_rational. Deliberately narrow: a
 * decimal would invite a value that is not exactly representable, and the
 * point of this module is that every value is exact. */
static bool rat_parse(const char *s, fc_rational *out) {
  long long num = 0, den = 1;
  int sign = 1, any = 0;
  if (!s) return false;
  while (*s == ' ') ++s;
  if (*s == '-') { sign = -1; ++s; }
  else if (*s == '+') ++s;
  for (; *s >= '0' && *s <= '9'; ++s) {
    if (num > (long long)1 << 40) return false;
    num = num * 10 + (*s - '0');
    any = 1;
  }
  if (!any) return false;
  if (*s == '/') {
    ++s;
    den = 0;
    any = 0;
    for (; *s >= '0' && *s <= '9'; ++s) {
      if (den > (long long)1 << 40) return false;
      den = den * 10 + (*s - '0');
      any = 1;
    }
    if (!any || den == 0) return false;
  }
  if (*s) return false;
  *out = fc_rat((int64_t)(sign * num), (int64_t)den);
  return fc_rat_valid(*out);
}

bool vi_ore_init(vi_ore *o, const char *tau, const char *sigma,
                 const char *delta_amp) {
  if (!o) return false;
  memset(o, 0, sizeof *o);
  if (!rat_parse(tau ? tau : "1", &o->tau)) return false;
  if (!rat_parse(sigma ? sigma : "1", &o->sigma)) return false;
  if (!rat_parse(delta_amp ? delta_amp : "1", &o->delta_amp)) return false;
  o->reduce = 0;
  return true;
}

/* Niven's theorem: 2cos(r.pi) is rational only at 0, +-1, +-2, so those are
 * the only `a` for which the pair can repeat. Anything else grows. */
bool vi_ore_degenerate(const vi_ore *o) {
  fc_rational t2, foursigma;
  if (!o || !fc_rat_valid(o->tau) || !fc_rat_valid(o->sigma)) return false;
  t2 = fc_rat_mul(o->tau, o->tau);
  foursigma = fc_rat_mul(fc_rat(4, 1), o->sigma);
  if (!fc_rat_valid(t2) || !fc_rat_valid(foursigma)) return false;
  return fc_rat_eq(t2, foursigma);
}

int vi_ore_period(const vi_ore *o) {
  /* DETECTED, not tabulated. The previous version switched on `a` and returned
   * 0 for every q != 1, which is wrong: a = 0, q = -1 has period 2, and the
   * table called it aperiodic. More generally, periodicity is not governed by
   * `a` alone - it is governed by whether both roots of x^2 - a.x + q lie on
   * the unit circle, which involves q as much as a.
   *
   * So run the recurrence and look for the STATE (C(n), C(n+1)) repeating.
   * The values are exact rationals, so a repeated state is genuine periodicity
   * rather than a numerical coincidence, and the test needs no case analysis:
   * it handles q = 1, q = -1, degenerate roots and irrational angles the same
   * way. Values that outgrow int64 report aperiodic, which is correct - an
   * orbit that escapes every bound cannot return to its starting state.
   *
   * The cap is FC_LEVELS_MAX, the declared topology ceiling; nothing in this
   * system reasons about a period longer than that. */
  fc_rational p0, p1, prev, cur;
  int n;
  if (!o || !fc_rat_valid(o->tau) || !fc_rat_valid(o->sigma)) return 0;
  if (vi_ore_degenerate(o)) return 0;      /* repeated root: n.r^n, unbounded */

  p0 = fc_rat(1, 1);
  p1 = fc_rat_mul(o->tau, fc_rat(1, 2));
  if (!fc_rat_valid(p1)) return 0;
  prev = p0;
  cur = p1;
  for (n = 1; n <= (int)FC_LEVELS_MAX; ++n) {
    fc_rational t1 = fc_rat_mul(o->tau, cur);
    fc_rational t2 = fc_rat_mul(o->sigma, prev);
    fc_rational nx;
    if (!fc_rat_valid(t1) || !fc_rat_valid(t2)) return 0;
    t2.num = -t2.num;
    nx = fc_rat_add(t1, t2);
    if (!fc_rat_valid(nx)) return 0;
    prev = cur;
    cur = nx;
    if (fc_rat_eq(prev, p0) && fc_rat_eq(cur, p1)) return n;
  }
  return 0;
}

/* --- the recurrence --------------------------------------------------------- */

/* Advances the pair (prev, cur) n steps. `n` must be non-negative. */
static bool ore_run(const vi_ore *o, int32_t n, fc_rational u0, fc_rational u1,
                    fc_rational *out) {
  fc_rational prev = u0, cur = u1;
  int32_t i;
  if (!fc_rat_valid(prev) || !fc_rat_valid(cur)) return false;
  if (n == 0) { *out = prev; return true; }
  for (i = 1; i < n; ++i) {
    /* next = a.cur - q.prev */
    fc_rational t1 = fc_rat_mul(o->tau, cur);
    fc_rational t2 = fc_rat_mul(o->sigma, prev);
    fc_rational next;
    if (!fc_rat_valid(t1) || !fc_rat_valid(t2)) return false;
    t2.num = -t2.num;
    next = fc_rat_add(t1, t2);
    if (!fc_rat_valid(next)) return false;        /* the aperiodic wall */
    prev = cur;
    cur = next;
  }
  *out = cur;
  return true;
}

/* Index reduction. For a periodic operator the reduction is an IDENTITY - the
 * sequence really does repeat - so it costs nothing and bounds the work. For
 * an aperiodic one there is no period to reduce by and none is applied; the
 * evaluation runs until the rationals give out, which is the honest outcome. */
static int32_t ore_index(const vi_ore *o, int32_t n, int *negate) {
  int p = vi_ore_period(o);
  int32_t m = n;
  *negate = 0;
  if (m < 0) { m = -m; *negate = 1; }
  if (p > 0) m = m % p;
  return m;
}

bool vi_ore_cos_raw(const vi_ore *o, int32_t n, fc_rational *out) {
  fc_rational c1, half = fc_rat(1, 2);
  if (!o || !out) return false;
  c1 = fc_rat_mul(o->tau, half);
  if (!fc_rat_valid(c1)) return false;
  return ore_run(o, n < 0 ? -n : n, fc_rat(1, 1), c1, out);
}

bool vi_ore_sin_raw(const vi_ore *o, int32_t n, fc_rational *out) {
  int neg = n < 0;
  if (!o || !out) return false;
  if (!ore_run(o, neg ? -n : n, fc_rat(0, 1), o->delta_amp, out)) return false;
  if (neg) out->num = -out->num;
  return true;
}

bool vi_ore_cos(const vi_ore *o, int32_t n, fc_rational *out) {
  fc_rational c0 = fc_rat(1, 1), c1, half;
  int neg = 0;
  int32_t m;
  if (!o || !out) return false;
  half = fc_rat(1, 2);
  c1 = fc_rat_mul(o->tau, half);
  if (!fc_rat_valid(c1)) return false;
  m = ore_index(o, n, &neg);
  /* C is even, so the reflection needs no sign change. */
  return ore_run(o, m, c0, c1, out);
}

bool vi_ore_sin(const vi_ore *o, int32_t n, fc_rational *out) {
  fc_rational s0 = fc_rat(0, 1);
  int neg = 0;
  int32_t m;
  if (!o || !out) return false;
  m = ore_index(o, n, &neg);
  if (!ore_run(o, m, s0, o->delta_amp, out)) return false;
  /* S is odd. Reflecting rather than running the recurrence backwards keeps
   * the negative branch exact and costs nothing. */
  if (neg) out->num = -out->num;
  return true;
}

bool vi_ore_tan(const vi_ore *o, int32_t n, fc_rational *out) {
  fc_rational c, s, inv;
  if (!vi_ore_cos(o, n, &c) || !vi_ore_sin(o, n, &s)) return false;
  if (c.num == 0) return false;                   /* a pole, and it is refused */
  inv = fc_rat(c.den, c.num);
  if (!fc_rat_valid(inv)) return false;
  *out = fc_rat_mul(s, inv);
  return fc_rat_valid(*out);
}

bool vi_ore_wronskian(const vi_ore *o, int32_t n, fc_rational *out) {
  fc_rational cn, cn1, sn, sn1, l, r;
  if (!vi_ore_cos(o, n, &cn) || !vi_ore_cos(o, n + 1, &cn1) ||
      !vi_ore_sin(o, n, &sn) || !vi_ore_sin(o, n + 1, &sn1)) return false;
  l = fc_rat_mul(cn, sn1);
  r = fc_rat_mul(cn1, sn);
  if (!fc_rat_valid(l) || !fc_rat_valid(r)) return false;
  r.num = -r.num;
  *out = fc_rat_add(l, r);
  return fc_rat_valid(*out);
}

/* --- quantisation ----------------------------------------------------------- */

FLOWVALUE vi_ore_quantize(fc_rational v, fc_rational next) {
  if (!fc_rat_valid(v)) return FV_PosZero;        /* the declared refusal pole */
  if (v.num > 0) return FV_PosOne;
  if (v.num < 0) return FV_NegOne;
  /* At a zero the pole is decided by which way the sequence is going through
   * it. This is the only place in the system where a signed zero is DERIVED
   * rather than declared, and it is why these operators are worth having. */
  if (!fc_rat_valid(next)) return FV_PosZero;
  if (next.num > 0) return FV_PosZero;
  if (next.num < 0) return FV_NegZero;
  return FV_PosZero;                              /* flat through zero: declared */
}

/* --- the abstract-operator plug-in ------------------------------------------ */

/* A small binding table rather than a single global: several Strategies are
 * live at once in the tool, and a single global would silently give them all
 * whichever parameters were bound last. */
#define ORE_BIND_MAX 256
static struct { char name[32]; vi_ore o[3]; int used; } g_bind[ORE_BIND_MAX];
static vi_ore g_fallback;
static int g_fallback_set;
static uint64_t g_refusals;

/* Finds or creates the slot for a name. Returns -1 when the table is full. */
static int bind_slot(const char *strategy_name) {
  int i, free_slot = -1;
  for (i = 0; i < ORE_BIND_MAX; ++i) {
    if (g_bind[i].used && !strcmp(g_bind[i].name, strategy_name)) return i;
    if (!g_bind[i].used && free_slot < 0) free_slot = i;
  }
  if (free_slot < 0) return -1;
  snprintf(g_bind[free_slot].name, sizeof g_bind[free_slot].name, "%s", strategy_name);
  g_bind[free_slot].used = 1;
  return free_slot;
}

bool vi_ore_bind_op(const char *strategy_name, vi_absop op, const vi_ore *o) {
  int k;
  if (!o || (int)op < 0 || (int)op > 2) return false;
  if (!strategy_name) { g_fallback = *o; g_fallback_set = 1; return true; }
  k = bind_slot(strategy_name);
  if (k < 0) return false;
  g_bind[k].o[(int)op] = *o;
  return true;
}

bool vi_ore_bind(const char *strategy_name, const vi_ore *o) {
  int k;
  if (!o) return false;
  if (!strategy_name) { g_fallback = *o; g_fallback_set = 1; return true; }
  k = bind_slot(strategy_name);
  if (k < 0) return false;
  g_bind[k].o[0] = *o;
  g_bind[k].o[1] = *o;
  g_bind[k].o[2] = *o;
  return true;
}

const vi_ore *vi_ore_bound(const char *strategy_name, vi_absop op) {
  int i;
  if (!strategy_name || (int)op < 0 || (int)op > 2) return NULL;
  for (i = 0; i < ORE_BIND_MAX; ++i)
    if (g_bind[i].used && !strcmp(g_bind[i].name, strategy_name))
      return &g_bind[i].o[(int)op];
  return g_fallback_set ? &g_fallback : NULL;
}

static const vi_ore *ore_for(const STRATEGY *s, vi_absop op) {
  int i;
  if (s && s->name)
    for (i = 0; i < ORE_BIND_MAX; ++i)
      if (g_bind[i].used && !strcmp(g_bind[i].name, s->name))
        return &g_bind[i].o[(int)op];
  return g_fallback_set ? &g_fallback : NULL;
}

uint64_t vi_ore_refusals(void) { return g_refusals; }
void vi_ore_reset_refusals(void) { g_refusals = 0; }

FLOWVALUE vi_ore_absop(const STRATEGY *s, vi_absop op, int32_t x) {
  const vi_ore *o = ore_for(s, op);
  fc_rational v, nx;
  bool okv, okn;
  if (!o) { ++g_refusals; return FV_PosZero; }
  switch (op) {
    case VI_ABS_SIN: okv = vi_ore_sin(o, x, &v); okn = vi_ore_sin(o, x + 1, &nx); break;
    case VI_ABS_COS: okv = vi_ore_cos(o, x, &v); okn = vi_ore_cos(o, x + 1, &nx); break;
    default:         okv = vi_ore_tan(o, x, &v); okn = vi_ore_tan(o, x + 1, &nx); break;
  }
  if (!okv) { ++g_refusals; return FV_PosZero; }
  if (!okn) nx = fc_rat(0, 0);                    /* poison; quantize declares */
  return vi_ore_quantize(v, nx);
}
