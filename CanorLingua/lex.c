#include "lex.h"
static int letter(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
static int cont(unsigned char c) {
  return letter(c) || (c >= '0' && c <= '9') || c == '_' || c == '-';
}
int canor_lex_name(const char *s, size_t n, size_t *out_len) {
  size_t i;
  if (!s || !out_len || n == 0 || !letter((unsigned char)s[0])) return -1;
  for (i = 1; i < n && cont((unsigned char)s[i]); i++) {}
  *out_len = i;
  return 0;
}
