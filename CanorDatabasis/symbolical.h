#ifndef CANORDB_SYMBOLICAL_H
#define CANORDB_SYMBOLICAL_H
/* Symbolicals: real (numeric), imaginary (permutohedron index), complex (pair). */
#include <stdint.h>
typedef enum { CANORDB_REAL = 1, CANORDB_IMAG = 2, CANORDB_COMPLEX = 3 } canordb_sym_kind;
typedef struct {
  canordb_sym_kind kind;
  int64_t real;
  int64_t imag;     /* permutohedron index when kind is imaginary */
  uint8_t perm_n;   /* levels of that index, 0..96 */
} canordb_symbolical;
int canordb_symbolical_make(canordb_symbolical *s, canordb_sym_kind kind,
                            int64_t real, int64_t imag, uint8_t perm_n);
#endif
