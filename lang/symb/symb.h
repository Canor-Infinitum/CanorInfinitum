#ifndef CANOR_SYMB_H
#define CANOR_SYMB_H
/* Symbolic tools. Arithmetic and products are the core types. */
#include "canor_house.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SYMB_GEOM_PARABOLIC = 0, /* Euclidean, curvature 0 */
  SYMB_GEOM_ELLIPTIC = 1,
  SYMB_GEOM_HYPERBOLIC = -1
} symb_geometry;

typedef enum {
  SYMB_AXIS_NONE = 0,
  SYMB_AXIS_NUMERIC,
  SYMB_AXIS_FUNC,
  SYMB_AXIS_PARABOLIC,
  SYMB_AXIS_QUATERNION
} symb_axis_kind;

typedef struct {
  symb_axis_kind kind;
  canor_real numeric;
  canor_real func_at;
  int curvature;
  canor_imag quaternion[4];
  int n_quat;
} symb_axis;

canor_status symb_range(const char *text, canor_interval *out);
canor_status symb_numeric_add(canor_real a, canor_real b, canor_real *out);
canor_status symb_numeric_mul(canor_real a, canor_real b, canor_real *out);
canor_status symb_interval_add(canor_interval a, canor_interval b, canor_interval *out);
canor_status symb_logical_and(bool a, bool b, bool *out);
canor_status symb_logical_or(bool a, bool b, bool *out);
canor_status symb_logical_not(bool a, bool *out);
canor_status symb_logical_cmp(canor_real a, canor_real b, int *out);
canor_status symb_func_apply(const char *body, canor_real x, canor_real *out);
canor_status symb_parametric(const char *name, symb_geometry *out);
canor_status symb_tensorial_make(const char *symmetry, size_t group_id, canor_tensorial *out);
canor_status symb_axis_set(symb_axis *axis, symb_axis_kind kind, canor_real value);
int symb_selftest(void);
int symb_tensor_selftest(void);

#ifdef __cplusplus
}
#endif
#endif
