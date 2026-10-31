#ifndef CARMEN_FRAME_H
#define CARMEN_FRAME_H
/* Unruh-Lorentzian frame. Interval s^2 = -t^2 + x^2 + y^2 + z^2, integers only.
 * Canor.md CarmenInstinctus, Discrete Quantum Relativity (Q-RT). */
#include <stdint.h>
typedef struct { int64_t t, x, y, z; } carmen_frame;
int carmen_interval2(const carmen_frame *f, int64_t *s2);
#endif
