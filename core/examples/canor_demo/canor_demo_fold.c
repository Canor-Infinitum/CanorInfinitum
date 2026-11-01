/* canor_demo_fold.c - extended and ringed imaginary bases
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* extended and ringed imaginary bases                                        */
/* ========================================================================== */
/* A Flow is a word over IMAGINARY, and IMAGINARY is not {i, j, k}. The default
 * profile closes at 16 basis elements - e0..e15, the sedenions - and the
 * quaternion triple is just the first three of them. Restricting Flow words to
 * i/j/k was a limitation of the word PARSER, not of the algebra, and it made
 * twelve of the sixteen units unreachable.
 *
 * RINGED BASES. canor_imaginary carries (imag, ring), and canor_base_eval SUMS the
 * ring across a fold while the unit MULTIPLIES under Cayley-Dickson. So a ring
 * is an additive tag riding alongside a multiplicative one - two coordinates
 * evolving under different laws in the same product. A Flow that declares
 * rings therefore folds to (unit, sign, ring), and two Flows can land on the
 * same unit while differing in ring. That is the point of having it: it
 * separates words the unit alone identifies.
 *
 * Declared as either
 *     "word":  "ijkkji"                    the old form, still exact
 *     "units": [1, 2, 3, 7, 12]            any of e0..e15
 *     "rings": [0, 1, 0, 2, 0]             optional, defaults to all zero
 */

bool canor_demo_ext_fold_word(const canor_demo_ext_flow_t *f, canor_demo_ext_fold_t *out) {
  canor_imag acc = 0;
  int sign = 1, i;
  int32_t ring = 0;
  for (i = 0; i < f->len; ++i) {
    canor_imag prod;
    int s2;
    if (canor_imag_mul(acc, f->unit[i], &prod, &s2) != CANOR_OK) return false;
    acc = prod;
    sign *= s2;
    ring += f->ring[i];          /* additive, per canor_base_eval */
  }
  out->unit = (uint8_t)acc;
  out->sign = sign;
  out->ring = ring;
  return true;
}

/* Accepts "ijkkji", "e1,e2,e3", or "1,2,3". The e-form exists because "e12" is
 * unambiguous where a bare letter alphabet would need twelve more letters. */
bool canor_demo_ext_parse(canor_demo_ext_flow_t *f, const char *name, const char *text) {
  const char *p = text;
  memset(f, 0, sizeof *f);
  snprintf(f->name, sizeof f->name, "%s", name);
  if (!text || !*text) return false;
  if (!strchr(text, ',') && !strchr(text, 'e')) {
    for (; *p; ++p) {                       /* the letter form */
      int u = (*p == '1') ? 0 : (*p == 'i') ? 1 : (*p == 'j') ? 2 :
              (*p == 'k') ? 3 : -1;
      if (u < 0 || f->len >= CANOR_DEMO_EXT_UNITS_MAX) return false;
      f->unit[f->len++] = (uint8_t)u;
    }
    return f->len > 0;
  }
  while (*p) {                              /* the index form */
    long v;
    char *e;
    while (*p == ' ' || *p == ',') ++p;
    if (!*p) break;
    if (*p == 'e') ++p;
    v = strtol(p, &e, 10);
    if (e == p || v < 0 || v >= (long)CANOR_LEVELS_MAX) return false;
    p = e;
    if (f->len >= CANOR_DEMO_EXT_UNITS_MAX) return false;
    f->unit[f->len++] = (uint8_t)v;
  }
  return f->len > 0;
}

/* Folds any word over e0..e15, with optional rings. The letter form ijk is
 * still accepted; it is now the special case it always was rather than the
 * whole alphabet. */
int canor_demo_cmd_fold(const canor_demo_config_t *c, const char *word, const char *rings) {
  canor_demo_ext_flow_t f;
  canor_demo_ext_fold_t fr;
  int i;
  (void)c;
  if (!canor_demo_ext_parse(&f, "adhoc", word)) {
    fprintf(stderr, "follow: '%s' is not a word over e0..e%d "
                    "(try \"ijk\" or \"e1,e2,e12\")\n", word, CANOR_LEVELS_MAX - 1);
    return 2;
  }
  if (rings) {
    const char *p = rings;
    int n = 0;
    while (*p && n < f.len) {
      char *e;
      long v;
      while (*p == ' ' || *p == ',') ++p;
      if (!*p) break;
      v = strtol(p, &e, 10);
      if (e == p) break;
      p = e;
      f.ring[n++] = (int32_t)v;
    }
    f.has_ring = n > 0;
  }
  if (!canor_demo_ext_fold_word(&f, &fr)) {
    fprintf(stderr, "follow: the product does not close\n");
    return 2;
  }
  printf("word   ");
  for (i = 0; i < f.len; ++i) printf("%se%u", i ? "." : "", (unsigned)f.unit[i]);
  printf("\n");
  if (f.has_ring) {
    printf("rings  ");
    for (i = 0; i < f.len; ++i) printf("%s%d", i ? "+" : "", (int)f.ring[i]);
    printf("\n");
  }
  printf("fold   %se%u   %s\n", fr.sign < 0 ? "-" : "+", (unsigned)fr.unit,
         fr.unit < 4 ? (const char *[]){"one", "i", "j", "k"}[fr.unit]
                     : "(beyond the quaternion triple)");
  /* The ring is additive while the unit is multiplicative, so it separates
   * words that land on the same unit. */
  printf("ring   %d\n", (int)fr.ring);
  return 0;
}
