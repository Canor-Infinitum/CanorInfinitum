#include "frame.h"
static int sq(int64_t v, int64_t *out) {
  if (v > 3037000499LL || v < -3037000499LL) return -1;
  *out = v * v;
  return 0;
}
int carmen_interval2(const carmen_frame *f, int64_t *s2) {
  int64_t t2, x2, y2, z2, acc;
  if (!f || !s2) return -1;
  if (sq(f->t, &t2) || sq(f->x, &x2) || sq(f->y, &y2) || sq(f->z, &z2)) return -2;
  acc = -t2;
  if ((x2 > 0 && acc > INT64_MAX - x2) || (y2 > 0 && acc > INT64_MAX - y2)) return -2;
  acc = acc + x2 + y2;
  if (z2 > 0 && acc > INT64_MAX - z2) return -2;
  *s2 = acc + z2;
  return 0;
}
