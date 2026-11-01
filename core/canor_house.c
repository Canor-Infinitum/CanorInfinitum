#include "canor_house.h"

canor_status canor_house_push_base(canor_base *slot, int *n, int cap, canor_imaginary v) {
  canor_status st;
  if (!slot || !n || *n < 0 || *n >= cap) return CANOR_ERR_FULL;
  canor_base_init(&slot[*n]);
  st = canor_base_push(&slot[*n], v);
  if (st != CANOR_OK) return st;
  (*n)++;
  return CANOR_OK;
}

canor_status canor_house_push_real(canor_real *slot, int *n, int cap, canor_real v) {
  if (!slot || !n || *n < 0 || *n >= cap) return CANOR_ERR_FULL;
  slot[(*n)++] = v;
  return CANOR_OK;
}

canor_status canor_house_push_interval(canor_interval *slot, int *n, int cap, canor_interval v) {
  if (!slot || !n || *n < 0 || *n >= cap) return CANOR_ERR_FULL;
  if (!canor_interval_ordered(&v)) return CANOR_ERR_RANGE;
  slot[(*n)++] = v;
  return CANOR_OK;
}

bool canor_len_op_named(canor_len_op op) {
  return op >= CANOR_LEN_NOP && op <= CANOR_LEN_WHOLE;
}
