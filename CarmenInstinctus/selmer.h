#ifndef CARMEN_SELMER_H
#define CARMEN_SELMER_H
/* Selmer-Lie bracket on an integer 3-vector: [a,b] = a×b. */
#include <stdint.h>
typedef struct { int64_t x, y, z; } carmen_vec3;
int carmen_selmer_bracket(const carmen_vec3 *a, const carmen_vec3 *b, carmen_vec3 *out);
#endif
