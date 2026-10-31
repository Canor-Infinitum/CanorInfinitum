#include "symbolical.h"
int canordb_symbolical_make(canordb_symbolical *s, canordb_sym_kind kind,
                            int64_t real, int64_t imag, uint8_t perm_n) {
  if (!s || perm_n > 96) return -1;
  if (kind < CANORDB_REAL || kind > CANORDB_COMPLEX) return -1;
  s->kind = kind;
  s->real = real;
  s->imag = imag;
  s->perm_n = perm_n;
  return 0;
}
