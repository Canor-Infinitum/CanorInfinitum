/* canor_demo_util.c - small helpers shared by several commands
 *
 * Rational formatting, base64 output for the browser pages, and the check
 * counter used by the newer self-verifying commands (scales, ricci, spacetime,
 * ...). Collected here because each was previously defined next to the first
 * command that happened to need it and then reached for by others.
 */

#include "canor_demo.h"

void canor_demo_rat_show(canor_rational r, char *buf, int cap) {
  if (!canor_rat_valid(r)) snprintf(buf, cap, "--");
  else if (r.den == 1) snprintf(buf, cap, "%lld", (long long)r.num);
  else snprintf(buf, cap, "%lld/%lld", (long long)r.num, (long long)r.den);
}

double canor_demo_rat_d(canor_rational r) {
  return canor_rat_valid(r) ? (double)r.num / (double)r.den : 0.0;
}

static const char B64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void canor_demo_b64_out(FILE *f, const unsigned char *p, size_t n) {
  size_t i;
  for (i = 0; i + 2 < n; i += 3) {
    unsigned v = ((unsigned)p[i] << 16) | ((unsigned)p[i + 1] << 8) | p[i + 2];
    fputc(B64[(v >> 18) & 63], f); fputc(B64[(v >> 12) & 63], f);
    fputc(B64[(v >> 6) & 63], f);  fputc(B64[v & 63], f);
  }
  if (i + 1 == n) {
    unsigned v = (unsigned)p[i] << 16;
    fputc(B64[(v >> 18) & 63], f); fputc(B64[(v >> 12) & 63], f);
    fputc('=', f); fputc('=', f);
  } else if (i + 2 == n) {
    unsigned v = ((unsigned)p[i] << 16) | ((unsigned)p[i + 1] << 8);
    fputc(B64[(v >> 18) & 63], f); fputc(B64[(v >> 12) & 63], f);
    fputc(B64[(v >> 6) & 63], f);  fputc('=', f);
  }
}

int canor_demo_check_fail, canor_demo_check_ran;

void canor_demo_check(int ok, const char *what) {
  printf("    %-58s %s\n", what, ok ? "ok" : "FAILED");
  ++canor_demo_check_ran;
  if (!ok) ++canor_demo_check_fail;
}
