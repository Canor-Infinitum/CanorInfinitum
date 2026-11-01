#include "symb.h"
#include <string.h>

static canor_status load_group(canor_tensor *tensor, int covariant, const char *symmetry,
                               size_t group_id, int64_t real, int64_t lo, int64_t hi) {
  canor_tensorial group;
  canor_modulation mod;
  canor_status st = symb_tensorial_make(symmetry, group_id, &group);
  if (st != CANOR_OK) return st;
  memset(&mod, 0, sizeof mod);
  mod.real.val = canor_rat(real, 1);
  mod.bounds.lo = canor_rat(lo, 1);
  mod.bounds.hi = canor_rat(hi, 1);
  canor_base_init(&mod.base);
  st = canor_tensorial_push(&group, &mod);
  if (st != CANOR_OK) return st;
  if (covariant) return canor_tensor_add_covariant(tensor, &group);
  return canor_tensor_add_contravariant(tensor, &group);
}

static canor_status one_case(int covariant, int64_t real, canor_rational expect) {
  canor_tensor tensor;
  canor_modulation out;
  int contra = 0, cov = 0;
  canor_tensor_init(&tensor);
  if (load_group(&tensor, covariant, "symmetric", 1, real, 0, real) != CANOR_OK) return CANOR_ERR_ARGS;
  if (canor_tensor_eval(&tensor, &out) != CANOR_OK) return CANOR_ERR_ARGS;
  canor_tensor_rank(&tensor, &contra, &cov);
  if (covariant) {
    if (cov != 1 || contra != 0) return CANOR_ERR_RANGE;
  } else if (contra != 1 || cov != 0) return CANOR_ERR_RANGE;
  if (!canor_rat_eq(out.real.val, expect)) return CANOR_ERR_RANGE;
  return CANOR_OK;
}

int symb_tensor_selftest(void) {
  symb_axis axis;
  canor_real x;
  canor_interval span;
  canor_pure pure;
  canor_theory theory;
  canor_elementary_number number;
  if (canor_imag_init() != CANOR_OK && canor_imag_init() != CANOR_OK) return 1;
  x.val = canor_rat(7, 1);
  if (one_case(0, 7, canor_rat(7, 1)) != CANOR_OK) return 2;
  if (symb_axis_set(&axis, SYMB_AXIS_NONE, x) != CANOR_OK || axis.kind != SYMB_AXIS_NONE) return 3;
  if (symb_axis_set(&axis, SYMB_AXIS_NUMERIC, x) != CANOR_OK) return 4;
  if (!canor_rat_eq(axis.numeric.val, x.val)) return 5;
  if (one_case(1, 4, canor_rat(4, 1)) != CANOR_OK) return 6;
  if (symb_axis_set(&axis, SYMB_AXIS_PARABOLIC, x) != CANOR_OK || axis.curvature != 0) return 7;
  if (symb_func_apply("x", x, &axis.func_at) != CANOR_OK) return 8;
  if (symb_axis_set(&axis, SYMB_AXIS_FUNC, axis.func_at) != CANOR_OK) return 9;
  if (symb_axis_set(&axis, SYMB_AXIS_QUATERNION, x) != CANOR_OK || axis.n_quat != 4) return 10;
  if (symb_range("0..7", &span) != CANOR_OK) return 11;
  memset(&number, 0, sizeof number);
  number.real = x;
  canor_base_init(&number.base);
  memset(&pure, 0, sizeof pure);
  pure.body.real = x;
  pure.body.span = span;
  canor_base_init(&pure.body.base);
  pure.len = CANOR_LEN_SEGMENT;
  memset(&theory, 0, sizeof theory);
  if (canor_house_push_real(theory.body.reals, &theory.body.n_reals, CANOR_HOUSE_MAX, x) != CANOR_OK) return 12;
  if (canor_house_push_interval(theory.body.spans, &theory.body.n_spans, CANOR_HOUSE_MAX, span) != CANOR_OK) return 13;
  theory.len = CANOR_LEN_WHOLE;
  if (theory.body.n_reals != 1 || theory.len != CANOR_LEN_WHOLE) return 14;
  return 0;
}
