#ifndef CANORDB_MANIFOLD_H
#define CANORDB_MANIFOLD_H
/* Index-addressed textual manifold. Populations are counts, never tables.
 * Cayley-Dickson charts are stages e = 2..15, dimension 2^e. */
#include "dim_catalog.h"
#include "../CanorInfinitum/source/include/canor_core.h"
#include <stdint.h>

typedef enum {
  CANORDB_GENERAL = 0,
  CANORDB_SPECIFIC = 1,
  CANORDB_DISTINCTIVE = 2,
  CANORDB_ARBITRARY = 3
} canordb_order;

typedef struct {
  int exact;
  uint64_t value;
  int64_t log2_milli;
} canordb_pop;

typedef struct {
  int dim;
  canordb_order order;
  int n;
  canor_lehmer code;
} canordb_flow;

typedef struct {
  int stage;
  uint32_t dimension;
  uint8_t rgba[4];
  uint8_t rgba_rank;
  int exact_support;
  uint64_t support;
  int64_t support_log2_milli;
} canordb_chart;

typedef struct {
  int dim;
  int n;
  canordb_order order;
  canor_lehmer code;
  canordb_chart chart;
} canordb_inference;

int canordb_population(canordb_order order, int n, canordb_pop *out);
int canordb_flow_address(int dim, canordb_order order, int n, canordb_flow *out);
int canordb_flow_cmp(const canordb_flow *a, const canordb_flow *b);

/* e in 2..15. Dimension is 2^e. Support is 4^e. */
int canordb_chart_stage(int stage, uint8_t rgba_rank, canordb_chart *out);
/* Rejects a dimension that is not 2^e for e in 2..15. */
int canordb_chart_dimension(uint32_t dimension, uint8_t rgba_rank, canordb_chart *out);
int canordb_chart_for_n(int n, uint8_t rgba_rank, canordb_chart *out);

int canordb_infer(const char *text, canordb_inference *out);
/* spec is "d_i, d_j, d_k". Compares the three selected Lehmer digits. */
int canordb_strategy_cmp(const char *spec, const canordb_inference *a, const canordb_inference *b);
#endif
