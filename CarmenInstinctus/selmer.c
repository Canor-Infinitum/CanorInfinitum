#include "selmer.h"
static int mul(int64_t a, int64_t b, int64_t *o) {
  if (a == 0 || b == 0) { *o = 0; return 0; }
  if (a > 0 && b > 0 && a > INT64_MAX / b) return -1;
  if (a < 0 && b < 0 && a < INT64_MAX / b) return -1;
  if (a > 0 && b < 0 && b < INT64_MIN / a) return -1;
  if (a < 0 && b > 0 && a < INT64_MIN / b) return -1;
  *o = a * b;
  return 0;
}
int carmen_selmer_bracket(const carmen_vec3 *a, const carmen_vec3 *b, carmen_vec3 *out) {
  int64_t aybz, azby, azbx, axbz, axby, aybx;
  if (!a || !b || !out) return -1;
  if (mul(a->y, b->z, &aybz) || mul(a->z, b->y, &azby)) return -2;
  if (mul(a->z, b->x, &azbx) || mul(a->x, b->z, &axbz)) return -2;
  if (mul(a->x, b->y, &axby) || mul(a->y, b->x, &aybx)) return -2;
  out->x = aybz - azby;
  out->y = azbx - axbz;
  out->z = axby - aybx;
  return 0;
}
