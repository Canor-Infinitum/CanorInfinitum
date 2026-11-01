#ifndef CANOR_HOUSE_H
#define CANOR_HOUSE_H
/* In-house products for Lingua, Databasis, Instinctus, and Instinctivus.
 * Built only from the ported core types. One definition, shared by all four. */
#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANOR_HOUSE_MAX
#define CANOR_HOUSE_MAX 4
#endif

typedef enum {
  CANOR_LEN_NOP = 0,
  CANOR_LEN_STARTVERTEX,
  CANOR_LEN_STOPVERTEX,
  CANOR_LEN_EDGES,
  CANOR_LEN_SEGMENT,
  CANOR_LEN_LEFTRAY,
  CANOR_LEN_RIGHTRAY,
  CANOR_LEN_LINE,
  CANOR_LEN_WHOLE
} canor_len_op;

typedef struct { canor_real real; canor_base base; } canor_elementary_number;
typedef struct { canor_real real; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_unique_number;
typedef struct { canor_real reals[CANOR_HOUSE_MAX]; int n_reals; canor_base base; } canor_composite_number;
typedef struct { canor_real reals[CANOR_HOUSE_MAX]; int n_reals; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_unknown_number;

typedef struct { canor_interval span; canor_base base; } canor_elementary_arbitrary;
typedef struct { canor_interval span; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_unique_arbitrary;
typedef struct { canor_interval spans[CANOR_HOUSE_MAX]; int n_spans; canor_base base; } canor_composite_arbitrary;
typedef struct { canor_interval spans[CANOR_HOUSE_MAX]; int n_spans; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_unknown_arbitrary;

typedef struct { canor_elementary_arbitrary body; canor_len_op len; } canor_elementary_length;
typedef struct { canor_unique_arbitrary body; canor_len_op len; } canor_unique_length;
typedef struct { canor_composite_arbitrary body; canor_len_op len; } canor_composite_length;
typedef struct { canor_unknown_arbitrary body; canor_len_op len; } canor_unknown_length;

/* One real, one interval, one base. Not MEASUREMENT. */
typedef struct { canor_real real; canor_interval span; canor_base base; } canor_unit;
typedef struct { canor_real real; canor_interval span; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_uniter;
typedef struct { canor_real real; canor_interval spans[CANOR_HOUSE_MAX]; int n_spans; canor_base base; } canor_unity;
typedef struct { canor_real real; canor_interval spans[CANOR_HOUSE_MAX]; int n_spans; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_unital;

typedef struct { canor_real reals[CANOR_HOUSE_MAX]; int n_reals; canor_interval span; canor_base base; } canor_point;
typedef struct { canor_real reals[CANOR_HOUSE_MAX]; int n_reals; canor_interval span; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_axis;
typedef struct { canor_real reals[CANOR_HOUSE_MAX]; int n_reals; canor_interval spans[CANOR_HOUSE_MAX]; int n_spans; canor_base base; } canor_composition;
typedef struct { canor_real reals[CANOR_HOUSE_MAX]; int n_reals; canor_interval spans[CANOR_HOUSE_MAX]; int n_spans; canor_base bases[CANOR_HOUSE_MAX]; int n_bases; } canor_unbeknownst;

typedef struct { canor_unit body; canor_len_op len; } canor_pure;
typedef struct { canor_uniter body; canor_len_op len; } canor_axiom;
typedef struct { canor_unity body; canor_len_op len; } canor_lemma;
typedef struct { canor_unital body; canor_len_op len; } canor_hypothesis;
typedef struct { canor_point body; canor_len_op len; } canor_proposition;
typedef struct { canor_axis body; canor_len_op len; } canor_conjecture;
typedef struct { canor_composition body; canor_len_op len; } canor_model;
typedef struct { canor_unbeknownst body; canor_len_op len; } canor_theory;

canor_status canor_house_push_base(canor_base *slot, int *n, int cap, canor_imaginary v);
canor_status canor_house_push_real(canor_real *slot, int *n, int cap, canor_real v);
canor_status canor_house_push_interval(canor_interval *slot, int *n, int cap, canor_interval v);
bool canor_len_op_named(canor_len_op op);

#ifdef __cplusplus
}
#endif
#endif
