/* Scale-relative operators, ordinal sampling, PG(3,2) duality. See canor_scale.h. */

#include "canor_scale.h"

#include <stdio.h>
#include <string.h>

/* --- ordinals --------------------------------------------------------------- */

void fo_zero(fo_ord *o) { if (o) o->n = 0; }

bool fo_make(fo_ord *o, int e, int64_t c, int64_t n) {
  if (!o || e < 0 || n < 0) return false;
  o->n = 0;
  if (e > 0 && c > 0) {
    o->t[o->n].exp = e;
    o->t[o->n].coef = c;
    ++o->n;
  } else if (e > 0 && c != 0) {
    return false;                 /* a negative coefficient is not an ordinal */
  }
  if (n > 0) {
    o->t[o->n].exp = 0;
    o->t[o->n].coef = n;
    ++o->n;
  }
  return true;
}

int fo_cmp(const fo_ord *a, const fo_ord *b) {
  int i;
  if (!a || !b) return 0;
  /* Lexicographic on (exponent, coefficient) from the leading term. This IS
   * the ordinal order for CNF: a larger leading exponent dominates whatever
   * follows, however large, which is the whole content of omega > n. */
  for (i = 0; i < a->n && i < b->n; ++i) {
    if (a->t[i].exp != b->t[i].exp) return a->t[i].exp > b->t[i].exp ? 1 : -1;
    if (a->t[i].coef != b->t[i].coef) return a->t[i].coef > b->t[i].coef ? 1 : -1;
  }
  if (a->n != b->n) return a->n > b->n ? 1 : -1;
  return 0;
}

bool fo_is_limit(const fo_ord *o) {
  if (!o || o->n == 0) return false;                 /* zero is not a limit */
  return o->t[o->n - 1].exp != 0;                    /* no finite term */
}

void fo_format(const fo_ord *o, char *buf, int cap) {
  int i, k = 0;
  if (!buf || cap < 2) return;
  buf[0] = 0;
  if (!o || o->n == 0) { snprintf(buf, (size_t)cap, "0"); return; }
  for (i = 0; i < o->n; ++i) {
    const char *sep = (i == 0) ? "" : "+";
    int wrote;
    if (o->t[i].exp == 0)
      wrote = snprintf(buf + k, (size_t)(cap - k), "%s%lld", sep,
                       (long long)o->t[i].coef);
    else if (o->t[i].exp == 1 && o->t[i].coef == 1)
      wrote = snprintf(buf + k, (size_t)(cap - k), "%sw", sep);
    else if (o->t[i].exp == 1)
      wrote = snprintf(buf + k, (size_t)(cap - k), "%sw*%lld", sep,
                       (long long)o->t[i].coef);
    else
      wrote = snprintf(buf + k, (size_t)(cap - k), "%sw^%d*%lld", sep,
                       o->t[i].exp, (long long)o->t[i].coef);
    if (wrote < 0 || wrote >= cap - k) return;
    k += wrote;
  }
}

int64_t fo_finite_part(const fo_ord *o) {
  if (!o || o->n == 0) return 0;
  return o->t[o->n - 1].exp == 0 ? o->t[o->n - 1].coef : 0;
}

int64_t fo_omega_part(const fo_ord *o) {
  int i;
  if (!o) return 0;
  for (i = 0; i < o->n; ++i) if (o->t[i].exp == 1) return o->t[i].coef;
  return 0;
}

/* --- scale-relative sampling ------------------------------------------------ */

bool fs_sample_at(canor_alg *a, const canor_alg_elem *e, const fo_ord *at,
                  fs_sample *out) {
  int64_t fin, om;
  int want;
  if (!a || !e || !at || !out) return false;
  memset(out, 0, sizeof *out);

  fin = fo_finite_part(at);
  om  = fo_omega_part(at);
  /* A limit ordinal has no finite part, so it names the class boundary itself
   * and not any particular refinement inside it. Sampling AT a limit is
   * therefore a different request from sampling just below it, and the two are
   * not conflated: the limit uses the class with no extra halvings. */
  if (fin < 0 || fin > 4096) return false;
  if (om < 0 || om > 3) return false;              /* four Kapprox classes */

  want = (int)fin;
  if (want > 0 && !canor_alg_refine(a, want)) return false;

  out->refinements = want;
  out->kclass = (int)om;
  out->value = canor_alg_elem_approx(a, e);
  (void)canor_alg_approx(a, &out->width);
  out->valid = canor_rat_valid(out->value) && canor_rat_valid(out->width);
  return out->valid;
}

/* --- PG(3,2) ----------------------------------------------------------------- */

int pg_form(uint8_t u, uint8_t v) {
  unsigned x = (unsigned)(u & v) & 0xfu;
  x ^= x >> 2;
  x ^= x >> 1;
  return (int)(x & 1u);
}

bool pg_line(uint8_t u, uint8_t v, uint8_t out[3]) {
  if (!out) return false;
  if (u == 0 || v == 0 || u == v || u > 15 || v > 15) return false;
  out[0] = u; out[1] = v; out[2] = (uint8_t)(u ^ v);
  return true;
}

static void sort3(uint8_t a[3]) {
  uint8_t t;
  if (a[0] > a[1]) { t = a[0]; a[0] = a[1]; a[1] = t; }
  if (a[1] > a[2]) { t = a[1]; a[1] = a[2]; a[2] = t; }
  if (a[0] > a[1]) { t = a[0]; a[0] = a[1]; a[1] = t; }
}

bool pg_dual_line(uint8_t u, uint8_t v, uint8_t out[3]) {
  uint8_t found[8];
  int n = 0, w;
  if (!out) return false;
  if (u == 0 || v == 0 || u == v || u > 15 || v > 15) return false;
  /* The annihilator of a 2-dimensional subspace of F_2^4 is 2-dimensional, so
   * exactly 3 non-zero vectors satisfy both conditions. Finding a different
   * count would mean the form is degenerate or the input was not a line, and
   * either is a failure rather than something to average over. */
  for (w = 1; w <= 15; ++w)
    if (!pg_form((uint8_t)w, u) && !pg_form((uint8_t)w, v)) {
      if (n >= 8) return false;
      found[n++] = (uint8_t)w;
    }
  if (n != 3) return false;
  out[0] = found[0]; out[1] = found[1]; out[2] = found[2];
  sort3(out);
  /* The result must itself be a line: the third point is the sum of the other
   * two. Checked, because a set of three annihilating vectors that did not
   * close would silently break the involution. */
  if ((uint8_t)(out[0] ^ out[1]) != out[2]) return false;
  return true;
}

uint32_t pg_dual_profile(uint32_t profile) {
  /* Under the standard form the hyperplane of point v is indexed by v, so the
   * induced map on point profiles is the identity. Returned unchanged and said
   * so in the header: this is a fact about the form, not a missing step. A
   * non-trivial test needs a correlation - pg_dual_perm supplies one. */
  return profile;
}

bool pg_dual_perm(uint32_t profile, const int perm[4], uint32_t *out) {
  uint32_t r = 0;
  int seen = 0, i, p;
  if (!perm || !out) return false;
  for (i = 0; i < 4; ++i) {
    if (perm[i] < 0 || perm[i] > 3) return false;
    if (seen & (1 << perm[i])) return false;        /* not a permutation */
    seen |= 1 << perm[i];
  }
  for (p = 1; p <= 15; ++p) {
    uint32_t q = 0;
    if (!((profile >> (p - 1)) & 1u)) continue;
    for (i = 0; i < 4; ++i)
      if ((unsigned)p & (1u << i)) q |= 1u << perm[i];
    if (q == 0 || q > 15) return false;
    r |= 1u << (q - 1);
  }
  *out = r;
  return true;
}
