#include "symb.h"
#include <string.h>
#include <stdlib.h>

static canor_real real_of(int64_t v) {
  canor_real r;
  r.val = canor_rat(v, 1);
  return r;
}

static canor_status parse_i64(const char *s, const char **end, int64_t *out) {
  char *e = 0;
  long long v;
  if (!s || !*s) return CANOR_ERR_ARGS;
  v = strtoll(s, &e, 10);
  if (e == s) return CANOR_ERR_ARGS;
  *out = (int64_t)v;
  if (end) *end = e;
  return CANOR_OK;
}

canor_status symb_range(const char *text, canor_interval *out) {
  const char *rest = 0;
  int64_t n, N;
  if (!text || !out) return CANOR_ERR_ARGS;
  if (parse_i64(text, &rest, &n) != CANOR_OK) return CANOR_ERR_ARGS;
  if (rest[0] != '.' || rest[1] != '.') return CANOR_ERR_ARGS;
  if (parse_i64(rest + 2, &rest, &N) != CANOR_OK) return CANOR_ERR_ARGS;
  if (*rest != 0) return CANOR_ERR_ARGS;
  if (n < 0 || N < n) return CANOR_ERR_RANGE;
  out->lo = canor_rat(n, 1);
  out->hi = canor_rat(N, 1);
  if (!canor_interval_ordered(out)) return CANOR_ERR_RANGE;
  return CANOR_OK;
}

canor_status symb_numeric_add(canor_real a, canor_real b, canor_real *out) {
  if (!out) return CANOR_ERR_ARGS;
  out->val = canor_rat_add(a.val, b.val);
  return canor_rat_valid(out->val) ? CANOR_OK : CANOR_ERR_UNDEFINED;
}

canor_status symb_numeric_mul(canor_real a, canor_real b, canor_real *out) {
  if (!out) return CANOR_ERR_ARGS;
  out->val = canor_rat_mul(a.val, b.val);
  return canor_rat_valid(out->val) ? CANOR_OK : CANOR_ERR_UNDEFINED;
}

canor_status symb_interval_add(canor_interval a, canor_interval b, canor_interval *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (!canor_interval_ordered(&a) || !canor_interval_ordered(&b)) return CANOR_ERR_RANGE;
  out->lo = canor_rat_add(a.lo, b.lo);
  out->hi = canor_rat_add(a.hi, b.hi);
  return canor_interval_ordered(out) ? CANOR_OK : CANOR_ERR_RANGE;
}

canor_status symb_logical_and(bool a, bool b, bool *out) {
  if (!out) return CANOR_ERR_ARGS;
  *out = a && b;
  return CANOR_OK;
}
canor_status symb_logical_or(bool a, bool b, bool *out) {
  if (!out) return CANOR_ERR_ARGS;
  *out = a || b;
  return CANOR_OK;
}
canor_status symb_logical_not(bool a, bool *out) {
  if (!out) return CANOR_ERR_ARGS;
  *out = !a;
  return CANOR_OK;
}
canor_status symb_logical_cmp(canor_real a, canor_real b, int *out) {
  if (!out) return CANOR_ERR_ARGS;
  *out = canor_rat_cmp(a.val, b.val);
  return CANOR_OK;
}

canor_status symb_func_apply(const char *body, canor_real x, canor_real *out) {
  if (!body || !out) return CANOR_ERR_ARGS;
  if (strcmp(body, "x") == 0) { *out = x; return CANOR_OK; }
  {
    const char *end = 0;
    int64_t c;
    if (parse_i64(body, &end, &c) == CANOR_OK && *end == 0) {
      *out = real_of(c);
      return CANOR_OK;
    }
  }
  return CANOR_ERR_UNDEFINED;
}

canor_status symb_parametric(const char *name, symb_geometry *out) {
  if (!name || !out) return CANOR_ERR_ARGS;
  if (strcmp(name, "parabolic") == 0 || strcmp(name, "euclidean") == 0) {
    *out = SYMB_GEOM_PARABOLIC;
    return CANOR_OK;
  }
  if (strcmp(name, "elliptic") == 0) { *out = SYMB_GEOM_ELLIPTIC; return CANOR_OK; }
  if (strcmp(name, "hyperbolic") == 0) { *out = SYMB_GEOM_HYPERBOLIC; return CANOR_OK; }
  return CANOR_ERR_ARGS;
}

canor_status symb_tensorial_make(const char *symmetry, size_t group_id, canor_tensorial *out) {
  if (!symmetry || !out) return CANOR_ERR_ARGS;
  canor_tensorial_init(out, group_id);
  out->regular = false;
  out->symmetric = false;
  out->antisymmetric = false;
  out->unknownsymmetric = false;
  if (strcmp(symmetry, "regular") == 0) out->regular = true;
  else if (strcmp(symmetry, "symmetric") == 0) out->symmetric = true;
  else if (strcmp(symmetry, "antisymmetric") == 0) out->antisymmetric = true;
  else if (strcmp(symmetry, "unknown") == 0) out->unknownsymmetric = true;
  else return CANOR_ERR_ARGS;
  if (!canor_tensorial_flags_consistent(out)) return CANOR_ERR_ARGS;
  return CANOR_OK;
}

canor_status symb_axis_set(symb_axis *axis, symb_axis_kind kind, canor_real value) {
  if (!axis) return CANOR_ERR_ARGS;
  memset(axis, 0, sizeof *axis);
  axis->kind = kind;
  if (kind == SYMB_AXIS_NUMERIC) axis->numeric = value;
  if (kind == SYMB_AXIS_FUNC) axis->func_at = value;
  if (kind == SYMB_AXIS_PARABOLIC) axis->curvature = 0;
  if (kind == SYMB_AXIS_QUATERNION) {
    axis->quaternion[0] = 1;
    axis->quaternion[1] = 0;
    axis->quaternion[2] = 0;
    axis->quaternion[3] = 0;
    axis->n_quat = 4;
  }
  return CANOR_OK;
}

static canor_modulation mod_of(int64_t real, int64_t lo, int64_t hi) {
  canor_modulation m;
  memset(&m, 0, sizeof m);
  m.real = real_of(real);
  m.bounds.lo = canor_rat(lo, 1);
  m.bounds.hi = canor_rat(hi, 1);
  canor_base_init(&m.base);
  return m;
}

static canor_status push_mod(canor_tensorial *t, int64_t real, int64_t lo, int64_t hi) {
  canor_modulation m = mod_of(real, lo, hi);
  return canor_tensorial_push(t, &m);
}

int symb_selftest(void) {
  canor_interval span, span2, sum;
  canor_real a, b, c;
  bool bit;
  int cmp;
  symb_geometry g;
  canor_unit unit;
  canor_measurement measurement;
  canor_imaginary imag;
  int n = 0;
  if (symb_range("1..4", &span) != CANOR_OK) return 1;
  if (symb_range("-1..4", &span) == CANOR_OK) return 2;
  if (symb_range("5..1", &span) == CANOR_OK) return 3;
  a = real_of(2); b = real_of(3);
  if (symb_numeric_add(a, b, &c) != CANOR_OK || !canor_rat_eq(c.val, canor_rat(5, 1))) return 4;
  if (symb_numeric_mul(a, b, &c) != CANOR_OK || !canor_rat_eq(c.val, canor_rat(6, 1))) return 5;
  if (symb_range("1..2", &span) != CANOR_OK || symb_range("3..4", &span2) != CANOR_OK) return 6;
  if (symb_interval_add(span, span2, &sum) != CANOR_OK) return 7;
  if (!canor_rat_eq(sum.lo, canor_rat(4, 1)) || !canor_rat_eq(sum.hi, canor_rat(6, 1))) return 8;
  if (symb_logical_and(true, false, &bit) != CANOR_OK || bit) return 9;
  if (symb_logical_or(true, false, &bit) != CANOR_OK || !bit) return 10;
  if (symb_logical_not(false, &bit) != CANOR_OK || !bit) return 11;
  if (symb_logical_cmp(a, b, &cmp) != CANOR_OK || cmp >= 0) return 12;
  if (symb_func_apply("x", a, &c) != CANOR_OK || !canor_rat_eq(c.val, a.val)) return 13;
  if (symb_parametric("parabolic", &g) != CANOR_OK || g != SYMB_GEOM_PARABOLIC) return 14;
  if (symb_parametric("euclidean", &g) != CANOR_OK || g != SYMB_GEOM_PARABOLIC) return 15;
  if (symb_parametric("elliptic", &g) != CANOR_OK || g != SYMB_GEOM_ELLIPTIC) return 16;
  if (symb_parametric("hyperbolic", &g) != CANOR_OK || g != (symb_geometry)(-1)) return 17;
  memset(&unit, 0, sizeof unit);
  unit.real = a;
  unit.span = span;
  canor_base_init(&unit.base);
  imag.imag = 0; imag.ring = 0;
  if (canor_base_push(&unit.base, imag) != CANOR_OK) return 18;
  if (!canor_len_op_named(CANOR_LEN_SEGMENT)) return 19;
  canor_measurement_init(&measurement, CANOR_T_VALUE);
  if (canor_measurement_add_real(&measurement, a) != CANOR_OK) return 20;
  if (!canor_measurement_is_atom(&measurement)) return 21;
  {
    canor_real slot[CANOR_HOUSE_MAX];
    if (canor_house_push_real(slot, &n, CANOR_HOUSE_MAX, a) != CANOR_OK || n != 1) return 22;
  }
  return 0;
}
