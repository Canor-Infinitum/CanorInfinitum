/* canor_demo_json.c - minimal JSON scanner
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* minimal JSON scanner                                                       */
/* ========================================================================== */

void canor_demo_jfail(canor_demo_jscan_t *j, const char *what) {
  if (!j->err[0])
    snprintf(j->err, sizeof j->err, "%s at byte %ld", what,
             (long)(j->p - (j->end - (j->end - j->p)) ));
}
void canor_demo_jws(canor_demo_jscan_t *j) {
  while (j->p < j->end && (*j->p == ' ' || *j->p == '\t' ||
                           *j->p == '\n' || *j->p == '\r')) ++j->p;
}
bool canor_demo_jlit(canor_demo_jscan_t *j, char c) {
  canor_demo_jws(j);
  if (j->p < j->end && *j->p == c) { ++j->p; return true; }
  return false;
}
/* Copies a string value into `out`. Handles \" \\ \/ \n \t and nothing else;
 * anything further is refused rather than passed through half-decoded. */
bool canor_demo_jstr(canor_demo_jscan_t *j, char *out, int cap) {
  int n = 0;
  canor_demo_jws(j);
  if (!canor_demo_jlit(j, '"')) { canor_demo_jfail(j, "expected string"); return false; }
  while (j->p < j->end && *j->p != '"') {
    char c = *j->p++;
    if (c == '\\') {
      if (j->p >= j->end) { canor_demo_jfail(j, "escape at end of input"); return false; }
      switch (*j->p++) {
        case '"':  c = '"';  break;
        case '\\': c = '\\'; break;
        case '/':  c = '/';  break;
        case 'n':  c = '\n'; break;
        case 't':  c = '\t'; break;
        default: canor_demo_jfail(j, "unsupported escape"); return false;
      }
    }
    if (n + 1 >= cap) { canor_demo_jfail(j, "string too long"); return false; }
    out[n++] = c;
  }
  if (j->p >= j->end) { canor_demo_jfail(j, "unterminated string"); return false; }
  ++j->p;
  out[n] = 0;
  return true;
}
bool canor_demo_jnum(canor_demo_jscan_t *j, long *out) {
  char *e;
  long v;
  canor_demo_jws(j);
  v = strtol(j->p, &e, 10);
  if (e == j->p) { canor_demo_jfail(j, "expected number"); return false; }
  j->p = e;
  *out = v;
  return true;
}
/* Skips one value of any type, so unknown keys are tolerated. */
bool canor_demo_jskip(canor_demo_jscan_t *j) {
  char buf[512];
  long dummy;
  canor_demo_jws(j);
  if (j->p >= j->end) return false;
  if (*j->p == '"') return canor_demo_jstr(j, buf, sizeof buf);
  if (*j->p == '{' || *j->p == '[') {
    char open = *j->p, close = (open == '{') ? '}' : ']';
    int depth = 0;
    while (j->p < j->end) {
      if (*j->p == '"') { if (!canor_demo_jstr(j, buf, sizeof buf)) return false; continue; }
      if (*j->p == open) ++depth;
      else if (*j->p == close && --depth == 0) { ++j->p; return true; }
      ++j->p;
    }
    canor_demo_jfail(j, "unterminated container");
    return false;
  }
  if (!strncmp(j->p, "true", 4))  { j->p += 4; return true; }
  if (!strncmp(j->p, "false", 5)) { j->p += 5; return true; }
  if (!strncmp(j->p, "null", 4))  { j->p += 4; return true; }
  return canor_demo_jnum(j, &dummy);
}
