#include "hypersurreal.h"
int canordb_hypersurreal_make(canordb_hypersurreal *o, int64_t ln, int64_t ld,
                              int64_t rn, int64_t rd, uint8_t unit) {
  if (!o || ld == 0 || rd == 0 || unit > 15) return -1;
  o->left_num = ln; o->left_den = ld;
  o->right_num = rn; o->right_den = rd;
  o->unit = unit;
  return 0;
}
