#ifndef CANORDB_TENSOR_H
#define CANORDB_TENSOR_H
/* Pure tensor: fixed components for a contraction or a HaPPY-code slot. */
#include <stddef.h>
#include <stdint.h>
#define CANORDB_TENSOR_MAX 16
typedef struct {
  int64_t component[CANORDB_TENSOR_MAX];
  int rank;
  int n;
  int contract; /* index contracted, or -1 */
} canordb_tensor;
int canordb_tensor_contract(const canordb_tensor *a, const canordb_tensor *b, int64_t *out);
#endif
