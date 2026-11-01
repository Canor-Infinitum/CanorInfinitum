/* Ore-algebraic rational skew-trigonometric operators. See canor_ore.h for why
 * these exist; this file is the recurrence and nothing more.
 *
 * ITERATION, NOT RECURSION. The pair is advanced with a two-term window, so
 * evaluating at n costs n steps and constant space. A doubling scheme would be
 * faster and would need the addition formulas, which is exactly the structure
 * section 13 declines to assume - so the slow version is also the honest one.
 *
 * EVERY STEP IS CHECKED. canor_rat_mul and canor_rat_add return poison rather than
 * wrapping, and poison propagates, so an aperiodic operator does not produce
 * wrong values before it produces no values. That is the whole reason the
 * rational overflow fix had to land before this could.
 */

#include "canor_ore.h"

#include <stdio.h>
#include <string.h>

/* --- construction ---------------------------------------------------------- */

/* Parses "n", "-n" or "n/d" into an canor_rational. Deliberately narrow: a
 * decimal would invite a value that is not exactly representable, and the
 * point of this module is that every value is exact. */
static bool rat_parse(const char *s, canor_rational *out) {
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
  *out = canor_rat((int64_t)(sign * num), (int64_t)den);
  return canor_rat_valid(*out);
}

bool canor_ore_init(canor_ore *o, const char *tau, const char *sigma,
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
bool canor_ore_degenerate(const canor_ore *o) {
  canor_rational t2, foursigma;
  if (!o || !canor_rat_valid(o->tau) || !canor_rat_valid(o->sigma)) return false;
  t2 = canor_rat_mul(o->tau, o->tau);
  foursigma = canor_rat_mul(canor_rat(4, 1), o->sigma);
  if (!canor_rat_valid(t2) || !canor_rat_valid(foursigma)) return false;
  return canor_rat_eq(t2, foursigma);
}

int canor_ore_period(const canor_ore *o) {
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
   * The cap is CANOR_LEVELS_MAX, the declared topology ceiling; nothing in this
   * system reasons about a period longer than that. */
  canor_rational p0, p1, prev, cur;
  int n;
  if (!o || !canor_rat_valid(o->tau) || !canor_rat_valid(o->sigma)) return 0;
  if (canor_ore_degenerate(o)) return 0;      /* repeated root: n.r^n, unbounded */

  p0 = canor_rat(1, 1);
  p1 = canor_rat_mul(o->tau, canor_rat(1, 2));
  if (!canor_rat_valid(p1)) return 0;
  prev = p0;
  cur = p1;
  for (n = 1; n <= (int)CANOR_LEVELS_MAX; ++n) {
    canor_rational t1 = canor_rat_mul(o->tau, cur);
    canor_rational t2 = canor_rat_mul(o->sigma, prev);
    canor_rational nx;
    if (!canor_rat_valid(t1) || !canor_rat_valid(t2)) return 0;
    t2.num = -t2.num;
    nx = canor_rat_add(t1, t2);
    if (!canor_rat_valid(nx)) return 0;
    prev = cur;
    cur = nx;
    if (canor_rat_eq(prev, p0) && canor_rat_eq(cur, p1)) return n;
  }
  return 0;
}

/* --- the recurrence --------------------------------------------------------- */

/* Advances the pair (prev, cur) n steps. `n` must be non-negative. */
static bool ore_run(const canor_ore *o, int32_t n, canor_rational u0, canor_rational u1,
                    canor_rational *out) {
  canor_rational prev = u0, cur = u1;
  int32_t i;
  if (!canor_rat_valid(prev) || !canor_rat_valid(cur)) return false;
  if (n == 0) { *out = prev; return true; }
  for (i = 1; i < n; ++i) {
    /* next = a.cur - q.prev */
    canor_rational t1 = canor_rat_mul(o->tau, cur);
    canor_rational t2 = canor_rat_mul(o->sigma, prev);
    canor_rational next;
    if (!canor_rat_valid(t1) || !canor_rat_valid(t2)) return false;
    t2.num = -t2.num;
    next = canor_rat_add(t1, t2);
    if (!canor_rat_valid(next)) return false;        /* the aperiodic wall */
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
static int32_t ore_index(const canor_ore *o, int32_t n, int *negate) {
  int p = canor_ore_period(o);
  int32_t m = n;
  *negate = 0;
  if (m < 0) { m = -m; *negate = 1; }
  if (p > 0) m = m % p;
  return m;
}

bool canor_ore_cos_raw(const canor_ore *o, int32_t n, canor_rational *out) {
  canor_rational c1, half = canor_rat(1, 2);
  if (!o || !out) return false;
  c1 = canor_rat_mul(o->tau, half);
  if (!canor_rat_valid(c1)) return false;
  return ore_run(o, n < 0 ? -n : n, canor_rat(1, 1), c1, out);
}

bool canor_ore_sin_raw(const canor_ore *o, int32_t n, canor_rational *out) {
  int neg = n < 0;
  if (!o || !out) return false;
  if (!ore_run(o, neg ? -n : n, canor_rat(0, 1), o->delta_amp, out)) return false;
  if (neg) out->num = -out->num;
  return true;
}

bool canor_ore_cos(const canor_ore *o, int32_t n, canor_rational *out) {
  canor_rational c0 = canor_rat(1, 1), c1, half;
  int neg = 0;
  int32_t m;
  if (!o || !out) return false;
  half = canor_rat(1, 2);
  c1 = canor_rat_mul(o->tau, half);
  if (!canor_rat_valid(c1)) return false;
  m = ore_index(o, n, &neg);
  /* C is even, so the reflection needs no sign change. */
  return ore_run(o, m, c0, c1, out);
}

bool canor_ore_sin(const canor_ore *o, int32_t n, canor_rational *out) {
  canor_rational s0 = canor_rat(0, 1);
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

bool canor_ore_tan(const canor_ore *o, int32_t n, canor_rational *out) {
  canor_rational c, s, inv;
  if (!canor_ore_cos(o, n, &c) || !canor_ore_sin(o, n, &s)) return false;
  if (c.num == 0) return false;                   /* a pole, and it is refused */
  inv = canor_rat(c.den, c.num);
  if (!canor_rat_valid(inv)) return false;
  *out = canor_rat_mul(s, inv);
  return canor_rat_valid(*out);
}

bool canor_ore_wronskian(const canor_ore *o, int32_t n, canor_rational *out) {
  canor_rational cn, cn1, sn, sn1, l, r;
  if (!canor_ore_cos(o, n, &cn) || !canor_ore_cos(o, n + 1, &cn1) ||
      !canor_ore_sin(o, n, &sn) || !canor_ore_sin(o, n + 1, &sn1)) return false;
  l = canor_rat_mul(cn, sn1);
  r = canor_rat_mul(cn1, sn);
  if (!canor_rat_valid(l) || !canor_rat_valid(r)) return false;
  r.num = -r.num;
  *out = canor_rat_add(l, r);
  return canor_rat_valid(*out);
}

/* --- quantisation ----------------------------------------------------------- */

FLOWVALUE canor_ore_quantize(canor_rational v, canor_rational next) {
  if (!canor_rat_valid(v)) return FV_PosZero;        /* the declared refusal pole */
  if (v.num > 0) return FV_PosOne;
  if (v.num < 0) return FV_NegOne;
  /* At a zero the pole is decided by which way the sequence is going through
   * it. This is the only place in the system where a signed zero is DERIVED
   * rather than declared, and it is why these operators are worth having. */
  if (!canor_rat_valid(next)) return FV_PosZero;
  if (next.num > 0) return FV_PosZero;
  if (next.num < 0) return FV_NegZero;
  return FV_PosZero;                              /* flat through zero: declared */
}

/* --- the abstract-operator plug-in ------------------------------------------ */

/* A small binding table rather than a single global: several Strategies are
 * live at once in the tool, and a single global would silently give them all
 * whichever parameters were bound last. */
#define ORE_BIND_MAX 256
static struct { char name[32]; canor_ore o[3]; int used; } g_bind[ORE_BIND_MAX];
static canor_ore g_fallback;
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

bool canor_ore_bind_op(const char *strategy_name, canor_absop op, const canor_ore *o) {
  int k;
  if (!o || (int)op < 0 || (int)op > 2) return false;
  if (!strategy_name) { g_fallback = *o; g_fallback_set = 1; return true; }
  k = bind_slot(strategy_name);
  if (k < 0) return false;
  g_bind[k].o[(int)op] = *o;
  return true;
}

bool canor_ore_bind(const char *strategy_name, const canor_ore *o) {
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

const canor_ore *canor_ore_bound(const char *strategy_name, canor_absop op) {
  int i;
  if (!strategy_name || (int)op < 0 || (int)op > 2) return NULL;
  for (i = 0; i < ORE_BIND_MAX; ++i)
    if (g_bind[i].used && !strcmp(g_bind[i].name, strategy_name))
      return &g_bind[i].o[(int)op];
  return g_fallback_set ? &g_fallback : NULL;
}

static const canor_ore *ore_for(const STRATEGY *s, canor_absop op) {
  int i;
  if (s && s->name)
    for (i = 0; i < ORE_BIND_MAX; ++i)
      if (g_bind[i].used && !strcmp(g_bind[i].name, s->name))
        return &g_bind[i].o[(int)op];
  return g_fallback_set ? &g_fallback : NULL;
}

uint64_t canor_ore_refusals(void) { return g_refusals; }
void canor_ore_reset_refusals(void) { g_refusals = 0; }

FLOWVALUE canor_ore_absop(const STRATEGY *s, canor_absop op, int32_t x) {
  const canor_ore *o = ore_for(s, op);
  canor_rational v, nx;
  bool okv, okn;
  if (!o) { ++g_refusals; return FV_PosZero; }
  switch (op) {
    case CANOR_ABS_SIN: okv = canor_ore_sin(o, x, &v); okn = canor_ore_sin(o, x + 1, &nx); break;
    case CANOR_ABS_COS: okv = canor_ore_cos(o, x, &v); okn = canor_ore_cos(o, x + 1, &nx); break;
    default:         okv = canor_ore_tan(o, x, &v); okn = canor_ore_tan(o, x + 1, &nx); break;
  }
  if (!okv) { ++g_refusals; return FV_PosZero; }
  if (!okn) nx = canor_rat(0, 0);                    /* poison; quantize declares */
  return canor_ore_quantize(v, nx);
}
