#ifndef CANOR_LEX_H
#define CANOR_LEX_H
/* NAME from atoms.ebnf: a letter, then letters, digits, '_' or '-'. */
#include <stddef.h>
int canor_lex_name(const char *s, size_t n, size_t *out_len);
#endif
