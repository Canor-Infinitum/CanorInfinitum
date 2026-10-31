#include "tensor.h"
int canordb_tensor_contract(const canordb_tensor *a, const canordb_tensor *b, int64_t *out) {
  int i;
  int64_t acc = 0;
  if (!a || !b || !out || a->n != b->n || a->n < 1 || a->n > CANORDB_TENSOR_MAX) return -1;
  if (a->contract < 0 || b->contract < 0) return -2;
  for (i = 0; i < a->n; i++) {
    if (a->component[i] != 0 && b->component[i] > INT64_MAX / a->component[i]) return -3;
    acc += a->component[i] * b->component[i];
  }
  *out = acc;
  return 0;
}
