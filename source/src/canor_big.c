/* Arbitrary-precision integers and rationals. See canor_big.h for the rationale;
 * the short version is that exact iteration outgrows int64 after three steps
 * and the alternative to unbounded integers is a wrong answer.
 *
 * IMPLEMENTATION NOTES
 *
 * Schoolbook multiplication. Karatsuba would win above a few hundred limbs and
 * this does reach that size on a deep zoom, but schoolbook is the version that
 * is obviously correct, and a subtle multiply would poison every result the
 * rest of the system computes. If profiling ever says otherwise the interface
 * does not change.
 *
 * Binary GCD rather than Euclidean, so the whole file needs division in
 * exactly one place - decimal formatting - and the arithmetic path has none.
 * That matters more than it looks: division is where a truncation would hide.
 *
 * Aliasing. Every binary operation accumulates into a scratch value and only
 * then moves it into r, so r may safely alias either operand. The move is a
 * pointer swap, not a copy.
 */

#include "canor_big.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- budget ---------------------------------------------------------------- */

static size_t g_limit = (size_t)1 << 20;   /* 4 MB of limbs */
static size_t g_live, g_peak;

void   fb_budget_set(size_t limbs) { g_limit = limbs; }
size_t fb_budget_get(void) { return g_limit; }
size_t fb_budget_used(void) { return g_peak; }
void   fb_budget_reset_peak(void) { g_peak = g_live; }

/* --- storage --------------------------------------------------------------- */

void fb_init(fb_int *a) { a->d = NULL; a->n = 0; a->cap = 0; a->sign = 0; }

void fb_free(fb_int *a) {
  if (a->d) {
    g_live -= (size_t)a->cap;
    free(a->d);
  }
  fb_init(a);
}

static bool fb_reserve(fb_int *a, int cap) {
  uint32_t *nd;
  size_t delta;
  if (cap <= a->cap) return true;
  if (cap < 4) cap = 4;
  if (cap < a->cap * 2) cap = a->cap * 2;
  delta = (size_t)(cap - a->cap);
  if (g_limit && g_live + delta > g_limit) return false;
  nd = (uint32_t *)realloc(a->d, (size_t)cap * sizeof *nd);
  if (!nd) return false;
  a->d = nd;
  g_live += delta;
  if (g_live > g_peak) g_peak = g_live;
  a->cap = cap;
  return true;
}

/* Drops leading zero limbs and forces the canonical encoding of zero. Every
 * routine that writes limbs ends here, so `sign == 0 iff n == 0` is an
 * invariant rather than a hope. */
static void fb_trim(fb_int *a) {
  while (a->n > 0 && a->d[a->n - 1] == 0) --a->n;
  if (a->n == 0) a->sign = 0;
  else if (a->sign == 0) a->sign = 1;
}

bool fb_is_zero(const fb_int *a) { return a->n == 0; }
void fb_neg(fb_int *a) { if (a->n) a->sign = -a->sign; }

bool fb_set_i64(fb_int *a, int64_t v) {
  uint64_t m;
  int sg = 0;
  if (v > 0) { sg = 1; m = (uint64_t)v; }
  else if (v < 0) { sg = -1; m = (uint64_t)(-(v + 1)) + 1u; }   /* INT64_MIN safe */
  else { a->n = 0; a->sign = 0; return true; }
  if (!fb_reserve(a, 2)) return false;
  a->d[0] = (uint32_t)(m & 0xffffffffu);
  a->d[1] = (uint32_t)(m >> 32);
  a->n = 2;
  a->sign = sg;
  fb_trim(a);
  return true;
}

bool fb_copy(fb_int *dst, const fb_int *src) {
  if (dst == src) return true;
  if (!fb_reserve(dst, src->n ? src->n : 1)) return false;
  memcpy(dst->d, src->d, (size_t)src->n * sizeof *src->d);
  dst->n = src->n;
  dst->sign = src->sign;
  return true;
}

int fb_bits(const fb_int *a) {
  uint32_t t;
  int b;
  if (a->n == 0) return 0;
  t = a->d[a->n - 1];
  b = (a->n - 1) * 32;
  while (t) { ++b; t >>= 1; }
  return b;
}

int fb_ctz(const fb_int *a) {
  int i, b = 0;
  uint32_t t;
  if (a->n == 0) return 0;
  for (i = 0; i < a->n && a->d[i] == 0; ++i) b += 32;
  t = a->d[i];
  while (!(t & 1u)) { ++b; t >>= 1; }
  return b;
}

/* --- comparison ------------------------------------------------------------ */

int fb_cmp_abs(const fb_int *a, const fb_int *b) {
  int i;
  if (a->n != b->n) return a->n < b->n ? -1 : 1;
  for (i = a->n - 1; i >= 0; --i)
    if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
  return 0;
}

int fb_cmp(const fb_int *a, const fb_int *b) {
  if (a->sign != b->sign) return a->sign < b->sign ? -1 : 1;
  if (a->sign == 0) return 0;
  return a->sign > 0 ? fb_cmp_abs(a, b) : -fb_cmp_abs(a, b);
}

/* --- magnitude primitives -------------------------------------------------- */

/* r = |a| + |b|, sign untouched. */
static bool mag_add(fb_int *r, const fb_int *a, const fb_int *b) {
  int i, n = (a->n > b->n ? a->n : b->n);
  uint64_t carry = 0;
  if (!fb_reserve(r, n + 1)) return false;
  for (i = 0; i < n; ++i) {
    uint64_t s = carry;
    if (i < a->n) s += a->d[i];
    if (i < b->n) s += b->d[i];
    r->d[i] = (uint32_t)(s & 0xffffffffu);
    carry = s >> 32;
  }
  r->d[n] = (uint32_t)carry;
  r->n = n + 1;
  fb_trim(r);
  return true;
}

/* r = |a| - |b|, requires |a| >= |b|. Sign untouched. */
static bool mag_sub(fb_int *r, const fb_int *a, const fb_int *b) {
  int i;
  int64_t borrow = 0;
  if (!fb_reserve(r, a->n ? a->n : 1)) return false;
  for (i = 0; i < a->n; ++i) {
    int64_t s = (int64_t)a->d[i] - borrow - (i < b->n ? (int64_t)b->d[i] : 0);
    if (s < 0) { s += ((int64_t)1 << 32); borrow = 1; } else borrow = 0;
    r->d[i] = (uint32_t)s;
  }
  r->n = a->n;
  fb_trim(r);
  return true;
}

/* Scratch pool. add/sub/mul each need a destination that is not an operand;
 * reusing statics would break re-entrancy for no gain, so each takes a local
 * and frees it. The allocations are amortised by realloc growth in practice. */

bool fb_add(fb_int *r, const fb_int *a, const fb_int *b) {
  fb_int t;
  bool ok;
  int sg;
  if (a->sign == 0) return fb_copy(r, b);
  if (b->sign == 0) return fb_copy(r, a);
  fb_init(&t);
  if (a->sign == b->sign) {
    sg = a->sign;
    ok = mag_add(&t, a, b);
  } else {
    int c = fb_cmp_abs(a, b);
    if (c == 0) { fb_free(&t); r->n = 0; r->sign = 0; return true; }
    if (c > 0) { sg = a->sign; ok = mag_sub(&t, a, b); }
    else       { sg = b->sign; ok = mag_sub(&t, b, a); }
  }
  if (ok) { t.sign = t.n ? sg : 0; fb_free(r); *r = t; }
  else fb_free(&t);
  return ok;
}

bool fb_sub(fb_int *r, const fb_int *a, const fb_int *b) {
  fb_int nb;
  bool ok;
  if (b->sign == 0) return fb_copy(r, a);
  fb_init(&nb);
  if (!fb_copy(&nb, b)) { fb_free(&nb); return false; }
  fb_neg(&nb);
  ok = fb_add(r, a, &nb);
  fb_free(&nb);
  return ok;
}

bool fb_mul(fb_int *r, const fb_int *a, const fb_int *b) {
  fb_int t;
  int i, j;
  if (a->sign == 0 || b->sign == 0) { r->n = 0; r->sign = 0; return true; }
  fb_init(&t);
  if (!fb_reserve(&t, a->n + b->n)) { fb_free(&t); return false; }
  memset(t.d, 0, (size_t)(a->n + b->n) * sizeof *t.d);
  t.n = a->n + b->n;
  for (i = 0; i < a->n; ++i) {
    uint64_t carry = 0, av = a->d[i];
    if (av == 0) continue;
    for (j = 0; j < b->n; ++j) {
      uint64_t cur = (uint64_t)t.d[i + j] + av * b->d[j] + carry;
      t.d[i + j] = (uint32_t)(cur & 0xffffffffu);
      carry = cur >> 32;
    }
    /* Propagate past the inner loop; one limb is not always enough. */
    for (j = i + b->n; carry; ++j) {
      uint64_t cur = (uint64_t)t.d[j] + carry;
      t.d[j] = (uint32_t)(cur & 0xffffffffu);
      carry = cur >> 32;
    }
  }
  t.sign = a->sign * b->sign;
  fb_trim(&t);
  fb_free(r);
  *r = t;
  return true;
}

bool fb_shl(fb_int *r, const fb_int *a, unsigned k) {
  fb_int t;
  int limbs = (int)(k / 32), bits = (int)(k % 32), i;
  if (a->n == 0) { r->n = 0; r->sign = 0; return true; }
  fb_init(&t);
  if (!fb_reserve(&t, a->n + limbs + 1)) { fb_free(&t); return false; }
  memset(t.d, 0, (size_t)(a->n + limbs + 1) * sizeof *t.d);
  for (i = 0; i < a->n; ++i) {
    uint64_t v = (uint64_t)a->d[i] << bits;
    t.d[i + limbs] |= (uint32_t)(v & 0xffffffffu);
    t.d[i + limbs + 1] |= (uint32_t)(v >> 32);
  }
  t.n = a->n + limbs + 1;
  t.sign = a->sign;
  fb_trim(&t);
  fb_free(r);
  *r = t;
  return true;
}

bool fb_shr(fb_int *r, const fb_int *a, unsigned k) {
  fb_int t;
  int limbs = (int)(k / 32), bits = (int)(k % 32), i, n = a->n - limbs;
  if (n <= 0) { fb_free(r); fb_init(r); return true; }
  fb_init(&t);
  if (!fb_reserve(&t, n)) { fb_free(&t); return false; }
  for (i = 0; i < n; ++i) {
    uint64_t v = a->d[i + limbs] >> bits;
    if (bits && i + limbs + 1 < a->n)
      v |= (uint64_t)a->d[i + limbs + 1] << (32 - bits);
    t.d[i] = (uint32_t)v;
  }
  t.n = n;
  t.sign = a->sign;
  fb_trim(&t);
  fb_free(r);
  *r = t;
  return true;
}

/* Binary GCD of |a| and |b|. No division anywhere. */
bool fb_gcd(fb_int *r, const fb_int *a, const fb_int *b) {
  fb_int x, y, t;
  int shift, ok = 1;
  if (a->n == 0) { return fb_copy(r, b) && (r->sign = r->n ? 1 : 0, true); }
  if (b->n == 0) { return fb_copy(r, a) && (r->sign = r->n ? 1 : 0, true); }
  fb_init(&x); fb_init(&y); fb_init(&t);
  if (!fb_copy(&x, a) || !fb_copy(&y, b)) ok = 0;
  x.sign = x.n ? 1 : 0;
  y.sign = y.n ? 1 : 0;
  if (ok) {
    int sx = fb_ctz(&x), sy = fb_ctz(&y);
    shift = sx < sy ? sx : sy;
    if (!fb_shr(&x, &x, (unsigned)sx)) ok = 0;
    if (ok && !fb_shr(&y, &y, (unsigned)sy)) ok = 0;
    while (ok && y.n) {
      if (fb_cmp_abs(&x, &y) > 0) { t = x; x = y; y = t; }   /* keep x <= y */
      if (!mag_sub(&y, &y, &x)) { ok = 0; break; }
      if (y.n && !fb_shr(&y, &y, (unsigned)fb_ctz(&y))) { ok = 0; break; }
    }
    if (ok && !fb_shl(&x, &x, (unsigned)shift)) ok = 0;
  }
  if (ok) { x.sign = x.n ? 1 : 0; fb_free(r); *r = x; fb_init(&x); }
  fb_free(&x); fb_free(&y);
  return ok != 0;
}

/* --- division --------------------------------------------------------------
 * Shift-and-subtract, one bit at a time. Slow and obviously correct; the only
 * places it runs are rational reduction and the fixed-point projection, and
 * neither is in an inner loop. */
bool fb_divmod(fb_int *q, fb_int *rem, const fb_int *a, const fb_int *b) {
  fb_int quo, r, bb, one;
  bool ok = true;
  int bit;
  if (b->n == 0) return false;
  fb_init(&quo); fb_init(&r); fb_init(&bb); fb_init(&one);
  if (!fb_copy(&bb, b) || !fb_set_i64(&one, 1)) ok = false;
  bb.sign = bb.n ? 1 : 0;
  for (bit = fb_bits(a) - 1; bit >= 0 && ok; --bit) {
    if (!fb_shl(&r, &r, 1)) { ok = false; break; }
    if (a->d[bit / 32] & (1u << (bit % 32)))
      if (!fb_add(&r, &r, &one)) { ok = false; break; }
    if (!fb_shl(&quo, &quo, 1)) { ok = false; break; }
    if (fb_cmp_abs(&r, &bb) >= 0) {
      if (!mag_sub(&r, &r, &bb)) { ok = false; break; }
      if (!fb_add(&quo, &quo, &one)) { ok = false; break; }
    }
  }
  if (ok) {
    quo.sign = quo.n ? a->sign * b->sign : 0;
    r.sign = r.n ? a->sign : 0;
    if (q) { fb_free(q); *q = quo; fb_init(&quo); }
    if (rem) { fb_free(rem); *rem = r; fb_init(&r); }
  }
  fb_free(&quo); fb_free(&r); fb_free(&bb); fb_free(&one);
  return ok;
}

bool fb_rat_to_fixed(const fb_int *num, const fb_int *den, unsigned shift,
                     int64_t *out) {
  fb_int t, q;
  bool ok = true;
  int64_t v = 0;
  fb_init(&t); fb_init(&q);
  if (!fb_shl(&t, num, shift)) ok = false;
  if (ok && !fb_divmod(&q, NULL, &t, den)) ok = false;
  if (ok) {
    if (fb_bits(&q) > 63) ok = false;               /* would not fit */
    else {
      uint64_t m = 0;
      if (q.n > 0) m = q.d[0];
      if (q.n > 1) m |= (uint64_t)q.d[1] << 32;
      v = q.sign < 0 ? -(int64_t)m : (int64_t)m;
    }
  }
  fb_free(&t); fb_free(&q);
  if (ok) *out = v;
  return ok;
}

/* --- decimal --------------------------------------------------------------- */

/* Divides |a| by a 32-bit value in place, returning the remainder. The one
 * division in the file, and it is confined to formatting. */
static uint32_t divmod_small(fb_int *a, uint32_t m) {
  uint64_t rem = 0;
  int i;
  for (i = a->n - 1; i >= 0; --i) {
    uint64_t cur = (rem << 32) | a->d[i];
    a->d[i] = (uint32_t)(cur / m);
    rem = cur % m;
  }
  fb_trim(a);
  return (uint32_t)rem;
}

int fb_format(const fb_int *a, char *out, int cap) {
  char *chunks = NULL;
  int nch = 0, cch = 0, len = 0, i, pos = 0;
  fb_int t;
  if (a->n == 0) { if (cap > 1) { out[0] = '0'; out[1] = 0; } return 1; }
  fb_init(&t);
  if (!fb_copy(&t, a)) { if (cap) out[0] = 0; return 0; }
  t.sign = 1;
  while (t.n) {
    uint32_t r = divmod_small(&t, 1000000000u);
    if (nch == cch) {
      char *nc;
      cch = cch ? cch * 2 : 8;
      nc = (char *)realloc(chunks, (size_t)cch * 10);
      if (!nc) { free(chunks); fb_free(&t); if (cap) out[0] = 0; return 0; }
      chunks = nc;
    }
    snprintf(chunks + (size_t)nch * 10, 10, "%09u", r);
    ++nch;
  }
  fb_free(&t);

  /* Most significant chunk without its zero padding, the rest padded. */
  len = (a->sign < 0) ? 1 : 0;
  if (a->sign < 0 && pos < cap - 1) out[pos++] = '-';
  {
    const char *top = chunks + (size_t)(nch - 1) * 10;
    int skip = 0;
    while (skip < 8 && top[skip] == '0') ++skip;
    len += 9 - skip;
    for (i = skip; i < 9 && pos < cap - 1; ++i) out[pos++] = top[i];
  }
  for (i = nch - 2; i >= 0; --i) {
    const char *c = chunks + (size_t)i * 10;
    int k;
    len += 9;
    for (k = 0; k < 9 && pos < cap - 1; ++k) out[pos++] = c[k];
  }
  if (cap > 0) out[pos] = 0;
  free(chunks);
  return len;
}

bool fb_parse(fb_int *a, const char *s) {
  int sg = 1;
  bool any = false;
  fb_int ten, dig;
  bool ok = true;
  while (*s == ' ') ++s;
  if (*s == '-') { sg = -1; ++s; }
  else if (*s == '+') ++s;
  fb_init(&ten); fb_init(&dig);
  if (!fb_set_i64(&ten, 10)) { fb_free(&ten); fb_free(&dig); return false; }
  a->n = 0; a->sign = 0;
  for (; *s >= '0' && *s <= '9'; ++s) {
    any = true;
    if (!fb_mul(a, a, &ten)) { ok = false; break; }
    if (!fb_set_i64(&dig, *s - '0')) { ok = false; break; }
    if (!fb_add(a, a, &dig)) { ok = false; break; }
  }
  fb_free(&ten); fb_free(&dig);
  if (!ok || !any || *s) return false;
  if (a->n) a->sign = sg;
  return true;
}

/* --- rationals -------------------------------------------------------------- */

void fb_rat_init(fb_rat *q) { fb_init(&q->num); fb_init(&q->den); }
void fb_rat_free(fb_rat *q) { fb_free(&q->num); fb_free(&q->den); }

bool fb_rat_set_i64(fb_rat *q, int64_t n, int64_t d) {
  if (d == 0) return false;
  if (!fb_set_i64(&q->num, n) || !fb_set_i64(&q->den, d)) return false;
  return fb_rat_normalize(q);
}

bool fb_rat_copy(fb_rat *dst, const fb_rat *src) {
  return fb_copy(&dst->num, &src->num) && fb_copy(&dst->den, &src->den);
}

bool fb_rat_normalize(fb_rat *q) {
  fb_int g, t;
  bool ok = true;
  if (q->den.sign == 0) return false;
  if (q->den.sign < 0) { fb_neg(&q->num); fb_neg(&q->den); }
  if (q->num.sign == 0) return fb_set_i64(&q->den, 1);
  fb_init(&g); fb_init(&t);
  if (!fb_gcd(&g, &q->num, &q->den)) ok = false;
  /* gcd is 1 far more often than not once values are reduced each step, and
   * the division below is the expensive one, so skip it when it is identity. */
  if (ok && !(g.n == 1 && g.d[0] == 1)) {
    /* Both divisions are exact by construction - g divides each - so the
     * remainder fb_divmod computes is discarded rather than checked away. */
    if (!fb_divmod(&q->num, NULL, &q->num, &g)) ok = false;
    if (ok && !fb_divmod(&q->den, NULL, &q->den, &g)) ok = false;
  }
  fb_free(&g); fb_free(&t);
  return ok;
}

/* a/b (+|-) c/d = (ad +|- cb) / bd, then reduce. */
static bool rat_addsub(fb_rat *r, const fb_rat *a, const fb_rat *b, int sub) {
  fb_rat t;
  fb_int x, y;
  bool ok = true;
  fb_rat_init(&t); fb_init(&x); fb_init(&y);
  if (!fb_mul(&x, &a->num, &b->den)) ok = false;
  if (ok && !fb_mul(&y, &b->num, &a->den)) ok = false;
  if (ok && sub) fb_neg(&y);
  if (ok && !fb_add(&t.num, &x, &y)) ok = false;
  if (ok && !fb_mul(&t.den, &a->den, &b->den)) ok = false;
  if (ok && !fb_rat_normalize(&t)) ok = false;
  if (ok) { fb_rat_free(r); *r = t; fb_rat_init(&t); }
  fb_rat_free(&t); fb_free(&x); fb_free(&y);
  return ok;
}

bool fb_rat_add(fb_rat *r, const fb_rat *a, const fb_rat *b) { return rat_addsub(r, a, b, 0); }
bool fb_rat_sub(fb_rat *r, const fb_rat *a, const fb_rat *b) { return rat_addsub(r, a, b, 1); }

bool fb_rat_mul(fb_rat *r, const fb_rat *a, const fb_rat *b) {
  fb_rat t;
  bool ok = true;
  fb_rat_init(&t);
  if (!fb_mul(&t.num, &a->num, &b->num)) ok = false;
  if (ok && !fb_mul(&t.den, &a->den, &b->den)) ok = false;
  if (ok && !fb_rat_normalize(&t)) ok = false;
  if (ok) { fb_rat_free(r); *r = t; fb_rat_init(&t); }
  fb_rat_free(&t);
  return ok;
}

int fb_rat_cmp_i64(const fb_rat *a, int64_t v) {
  fb_int rhs, vv;
  int c = 0;
  fb_init(&rhs); fb_init(&vv);
  if (fb_set_i64(&vv, v) && fb_mul(&rhs, &vv, &a->den))
    c = fb_cmp(&a->num, &rhs);
  fb_free(&rhs); fb_free(&vv);
  return c;
}

int fb_rat_cmp(const fb_rat *a, const fb_rat *b) {
  fb_int l, r;
  int c = 0;
  fb_init(&l); fb_init(&r);
  if (fb_mul(&l, &a->num, &b->den) && fb_mul(&r, &b->num, &a->den))
    c = fb_cmp(&l, &r);
  fb_free(&l); fb_free(&r);
  return c;
}

int fb_rat_bits(const fb_rat *a) {
  int bn = fb_bits(&a->num), bd = fb_bits(&a->den);
  return bn > bd ? bn : bd;
}

int fb_rat_format(const fb_rat *a, char *out, int cap) {
  int n = fb_format(&a->num, out, cap);
  if (a->den.n == 1 && a->den.d[0] == 1) return n;
  if (n < cap - 1) out[n] = '/';
  n += 1;
  if (n < cap) n += fb_format(&a->den, out + n, cap - n);
  else n += fb_format(&a->den, out, 0);
  return n;
}

bool fb_rat_parse(fb_rat *q, const char *s) {
  const char *slash = strchr(s, '/'), *dot = strchr(s, '.');
  char buf[512];
  bool ok;
  if (slash) {
    size_t ln = (size_t)(slash - s);
    if (ln >= sizeof buf) return false;
    memcpy(buf, s, ln);
    buf[ln] = 0;
    if (!fb_parse(&q->num, buf)) return false;
    if (!fb_parse(&q->den, slash + 1)) return false;
    if (q->den.sign == 0) return false;
    return fb_rat_normalize(q);
  }
  if (dot) {
    /* "-0.3125" -> -3125 / 10^4, exact. A decimal that cannot be written
     * exactly cannot be typed here, so nothing is silently rounded. */
    size_t i = 0, k = 0;
    int frac = 0;
    for (const char *p = s; *p; ++p) {
      if (*p == '.') { frac = 1; continue; }
      if (i + 1 >= sizeof buf) return false;
      buf[i++] = *p;
      if (frac) ++k;
    }
    buf[i] = 0;
    if (!fb_parse(&q->num, buf)) return false;
    if (!fb_set_i64(&q->den, 1)) return false;
    {
      fb_int ten;
      fb_init(&ten);
      ok = fb_set_i64(&ten, 10);
      while (ok && k--) ok = fb_mul(&q->den, &q->den, &ten);
      fb_free(&ten);
    }
    return ok && fb_rat_normalize(q);
  }
  if (!fb_parse(&q->num, s)) return false;
  return fb_set_i64(&q->den, 1);
}
