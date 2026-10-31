#ifndef CANORDB_HYPERSURREAL_H
#define CANORDB_HYPERSURREAL_H
/* Hypercomplex hypersurreal: a pair of exact integer ratios plus one
 * Cayley-Dickson unit. Canor.md "CanorDatabasis". No floating point. */
#include <stdint.h>
typedef struct {
  int64_t left_num, left_den;
  int64_t right_num, right_den;
  uint8_t unit; /* 0 = one, 1..15 sedenion units */
} canordb_hypersurreal;
int canordb_hypersurreal_make(canordb_hypersurreal *o, int64_t ln, int64_t ld,
                              int64_t rn, int64_t rd, uint8_t unit);
#endif
