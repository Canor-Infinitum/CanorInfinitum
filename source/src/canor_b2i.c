/* Base 2i (quater-imaginary) and its digit filter. See canor_b2i.h. */

#include "canor_b2i.h"

#include <stdio.h>
#include <string.h>

/* Base -4 with digits 0..3. The negative base is what removes the sign: the
 * remainder is taken NON-NEGATIVE and the quotient carries the alternation, so
 * every integer - positive or negative - is a string of 0..3 with no minus. */
static int base_m4(int64_t n, uint8_t *out, int cap) {
  int k = 0;
  if (n == 0) { if (cap > 0) out[0] = 0; return 1; }
  while (n != 0 && k < cap) {
    int64_t r = n % 4;
    if (r < 0) r += 4;              /* C's % keeps the sign; base -4 must not */
    out[k++] = (uint8_t)r;
    n = (n - r) / -4;
  }
  return (n == 0) ? k : -1;
}

bool canor_b2i_encode(int64_t re, int64_t im, canor_b2i *out) {
  uint8_t er[B2I_MAX], od[B2I_MAX];
  int ne, no, i, top;
  if (!out) return false;
  memset(out, 0, sizeof *out);

  /* ODD IMAGINARY PART. The odd positions carry 2i.(...), so they reach only
   * even imaginary parts. Digit 2 at position -1 is exactly -i, so shifting b
   * up by one and appending ".2" is an identity rather than a rounding. */
  if (im & 1) {
    out->frac = 1;
    im += 1;
  }

  ne = base_m4(re, er, B2I_MAX / 2);
  no = base_m4(im / 2, od, B2I_MAX / 2);
  if (ne < 0 || no < 0) return false;

  top = (ne > no) ? ne : no;
  if (2 * top > B2I_MAX) return false;
  for (i = 0; i < top; ++i) {
    out->d[2 * i]     = (i < ne) ? er[i] : 0;
    out->d[2 * i + 1] = (i < no) ? od[i] : 0;
  }
  out->n = 2 * top;
  while (out->n > 1 && out->d[out->n - 1] == 0) --out->n;
  out->valid = true;
  return true;
}

bool canor_b2i_decode(const canor_b2i *v, int64_t *re, int64_t *im) {
  int64_t r = 0, m = 0, pr = 1, pm = 1;
  int i;
  if (!v || !v->valid || !re || !im) return false;
  /* Even positions run in base -4 on the real axis; odd positions do the same
   * on half the imaginary axis. Accumulated with running powers rather than
   * recomputed, so nothing overflows earlier than it must. */
  for (i = 0; i < v->n; ++i) {
    if ((i & 1) == 0) { r += (int64_t)v->d[i] * pr; pr *= -4; }
    else              { m += (int64_t)v->d[i] * pm; pm *= -4; }
  }
  *re = r;
  *im = 2 * m - (v->frac ? 1 : 0);
  return true;
}

void canor_b2i_format(const canor_b2i *v, char *buf, int cap) {
  int i, k = 0;
  if (!buf || cap < 4) return;
  buf[0] = 0;
  if (!v || !v->valid) { snprintf(buf, (size_t)cap, "?"); return; }
  for (i = v->n - 1; i >= 0 && k < cap - 4; --i)
    buf[k++] = (char)('0' + v->d[i]);
  if (k == 0) buf[k++] = '0';
  if (v->frac) { buf[k++] = '.'; buf[k++] = '2'; }
  buf[k] = 0;
}

/* --- filter ---------------------------------------------------------------- */

void canor_b2i_filter_init(canor_b2i_filter *f) {
  if (!f) return;
  f->allow = 15;
  f->min_len = 0;
  f->max_len = 0;
  f->no_frac = false;
  f->palindrome = false;
  f->run_digit = -1;
  f->run_len = 0;
}

bool canor_b2i_pass(const canor_b2i *v, const canor_b2i_filter *f) {
  int i;
  if (!v || !v->valid || !f) return false;
  if (f->no_frac && v->frac) return false;
  if (v->n < f->min_len) return false;
  if (f->max_len > 0 && v->n > f->max_len) return false;
  for (i = 0; i < v->n; ++i)
    if (!((f->allow >> v->d[i]) & 1)) return false;
  if (f->palindrome)
    for (i = 0; i < v->n / 2; ++i)
      if (v->d[i] != v->d[v->n - 1 - i]) return false;
  if (f->run_digit >= 0 && f->run_len > 0) {
    int best = 0, cur = 0;
    for (i = 0; i < v->n; ++i) {
      if (v->d[i] == (uint8_t)f->run_digit) { if (++cur > best) best = cur; }
      else cur = 0;
    }
    if (best < f->run_len) return false;
  }
  return true;
}
