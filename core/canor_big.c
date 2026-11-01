/* Bounded big integers and rationals. See canor_big.h for the rationale.
 *
 * IMPLEMENTATION NOTES
 *
 * Fixed limbs (AGENTS.md invariant 2). Every result is built in a local
 * canor_big_t and assigned only when it fits, so a refused operation leaves
 * its destination untouched and a destination may alias an operand.
 *
 * Schoolbook multiplication: obviously correct, and a subtle multiply would
 * poison every result the rest of the system computes.
 *
 * Binary GCD rather than Euclidean, so the arithmetic path has no division;
 * division appears in divmod (shift-and-subtract) and in decimal formatting.
 */

#include "canor_big.h"

#include <string.h>

#define LIMB_MASK 0xffffffffu
#define LIMB_BASE ((uint64_t)1 << CANOR_BIG_LIMB_BITS)
#define DEC_BASE 10

/* --- storage --------------------------------------------------------------- */

void canor_big_init(canor_big_t *a) { a->n = 0; a->sign = 0; }

/* Drops leading zero limbs and forces the canonical encoding of zero. */
static void big_trim(canor_big_t *a) {
  while (a->n > 0 && a->d[a->n - 1] == 0) --a->n;
  if (a->n == 0) a->sign = 0;
  else if (a->sign == 0) a->sign = 1;
}

static void big_copy(canor_big_t *dst, const canor_big_t *src) {
  if (dst == src) return;
  memcpy(dst->d, src->d, (size_t)src->n * sizeof *src->d);
  dst->n = src->n;
  dst->sign = src->sign;
}

bool canor_big_is_zero(const canor_big_t *a) { return a->n == 0; }
bool canor_big_is_one(const canor_big_t *a) { return a->n == 1 && a->d[0] == 1 && a->sign > 0; }
void canor_big_neg(canor_big_t *a) { if (a->n) a->sign = -a->sign; }
void canor_big_abs(canor_big_t *a) { if (a->n) a->sign = 1; }

void canor_big_set_i64(canor_big_t *a, int64_t v) {
  uint64_t m;
  int sg;
  if (v == 0) { a->n = 0; a->sign = 0; return; }
  if (v > 0) { sg = 1; m = (uint64_t)v; }
  else { sg = -1; m = (uint64_t)(-(v + 1)) + 1u; }   /* INT64_MIN safe */
  a->d[0] = (uint32_t)(m & LIMB_MASK);
  a->d[1] = (uint32_t)(m >> CANOR_BIG_LIMB_BITS);
  a->n = 2;
  a->sign = sg;
  big_trim(a);
}

int canor_big_bits(const canor_big_t *a) {
  uint32_t t;
  int b;
  if (a->n == 0) return 0;
  t = a->d[a->n - 1];
  b = (a->n - 1) * CANOR_BIG_LIMB_BITS;
  while (t) { ++b; t >>= 1; }
  return b;
}

int canor_big_ctz(const canor_big_t *a) {
  int i, b = 0;
  uint32_t t;
  if (a->n == 0) return 0;
  for (i = 0; i < a->n && a->d[i] == 0; ++i) b += CANOR_BIG_LIMB_BITS;
  t = a->d[i];
  while (!(t & 1u)) { ++b; t >>= 1; }
  return b;
}

static int big_bit(const canor_big_t *a, int bit) {
  int limb = bit / CANOR_BIG_LIMB_BITS;
  if (limb >= a->n) return 0;
  return (int)((a->d[limb] >> (bit % CANOR_BIG_LIMB_BITS)) & 1u);
}

/* --- comparison ------------------------------------------------------------ */

int canor_big_cmp_abs(const canor_big_t *a, const canor_big_t *b) {
  int i;
  if (a->n != b->n) return a->n < b->n ? -1 : 1;
  for (i = a->n - 1; i >= 0; --i)
    if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
  return 0;
}

int canor_big_cmp(const canor_big_t *a, const canor_big_t *b) {
  if (a->sign != b->sign) return a->sign < b->sign ? -1 : 1;
  if (a->sign == 0) return 0;
  return a->sign > 0 ? canor_big_cmp_abs(a, b) : -canor_big_cmp_abs(a, b);
}

/* --- magnitude primitives -------------------------------------------------- */

/* r = |a| + |b|, positive. */
static canor_status mag_add(canor_big_t *r, const canor_big_t *a, const canor_big_t *b) {
  int i, n = (a->n > b->n ? a->n : b->n);
  uint64_t carry = 0;
  canor_big_t t;
  for (i = 0; i < n; ++i) {
    uint64_t s = carry;
    if (i < a->n) s += a->d[i];
    if (i < b->n) s += b->d[i];
    t.d[i] = (uint32_t)(s & LIMB_MASK);
    carry = s >> CANOR_BIG_LIMB_BITS;
  }
  t.n = n;
  if (carry) {
    if (n >= CANOR_BIG_LIMBS_MAX) return CANOR_ERR_FULL;
    t.d[n] = (uint32_t)carry;
    t.n = n + 1;
  }
  t.sign = 1;
  big_trim(&t);
  big_copy(r, &t);
  return CANOR_OK;
}

/* r = |a| - |b|, requires |a| >= |b|. Positive (or zero). In place is safe:
 * limb i reads a and b at i only before writing r at i. */
static void mag_sub(canor_big_t *r, const canor_big_t *a, const canor_big_t *b) {
  int i, n = a->n;
  int64_t borrow = 0;
  for (i = 0; i < n; ++i) {
    int64_t s = (int64_t)a->d[i] - borrow - (i < b->n ? (int64_t)b->d[i] : 0);
    if (s < 0) { s += (int64_t)LIMB_BASE; borrow = 1; } else borrow = 0;
    r->d[i] = (uint32_t)s;
  }
  r->n = n;
  r->sign = 1;
  big_trim(r);
}

canor_status canor_big_add(canor_big_t *r, const canor_big_t *a, const canor_big_t *b) {
  int sg, c;
  canor_status st;
  if (a->sign == 0) { big_copy(r, b); return CANOR_OK; }
  if (b->sign == 0) { big_copy(r, a); return CANOR_OK; }
  if (a->sign == b->sign) {
    sg = a->sign;
    st = mag_add(r, a, b);
    if (st != CANOR_OK) return st;
    if (r->n) r->sign = sg;
    return CANOR_OK;
  }
  c = canor_big_cmp_abs(a, b);
  if (c == 0) { canor_big_init(r); return CANOR_OK; }
  if (c > 0) { sg = a->sign; mag_sub(r, a, b); }
  else       { sg = b->sign; mag_sub(r, b, a); }
  if (r->n) r->sign = sg;
  return CANOR_OK;
}

canor_status canor_big_sub(canor_big_t *r, const canor_big_t *a, const canor_big_t *b) {
  canor_big_t nb;
  big_copy(&nb, b);
  canor_big_neg(&nb);
  return canor_big_add(r, a, &nb);
}

canor_status canor_big_mul(canor_big_t *r, const canor_big_t *a, const canor_big_t *b) {
  /* Double-width scratch, so the full product is formed and the capacity
   * check is on the exact result rather than on an estimate. */
  uint32_t w[2 * CANOR_BIG_LIMBS_MAX];
  int i, j, n;
  if (a->sign == 0 || b->sign == 0) { canor_big_init(r); return CANOR_OK; }
  /* The product has at least bits(a) + bits(b) - 1 bits: refuse early. */
  if (canor_big_bits(a) + canor_big_bits(b) - 1 > CANOR_BIG_BITS_MAX) return CANOR_ERR_FULL;
  n = a->n + b->n;
  memset(w, 0, (size_t)n * sizeof *w);
  for (i = 0; i < a->n; ++i) {
    uint64_t carry = 0, av = a->d[i];
    if (av == 0) continue;
    for (j = 0; j < b->n; ++j) {
      uint64_t cur = (uint64_t)w[i + j] + av * b->d[j] + carry;
      w[i + j] = (uint32_t)(cur & LIMB_MASK);
      carry = cur >> CANOR_BIG_LIMB_BITS;
    }
    for (j = i + b->n; carry; ++j) {
      uint64_t cur = (uint64_t)w[j] + carry;
      w[j] = (uint32_t)(cur & LIMB_MASK);
      carry = cur >> CANOR_BIG_LIMB_BITS;
    }
  }
  while (n > 0 && w[n - 1] == 0) --n;
  if (n > CANOR_BIG_LIMBS_MAX) return CANOR_ERR_FULL;
  memcpy(r->d, w, (size_t)n * sizeof *w);
  r->n = n;
  r->sign = a->sign * b->sign;
  return CANOR_OK;
}

canor_status canor_big_mul_i64(canor_big_t *r, const canor_big_t *a, int64_t m) {
  canor_big_t mm;
  canor_big_set_i64(&mm, m);
  return canor_big_mul(r, a, &mm);
}

canor_status canor_big_shl(canor_big_t *r, const canor_big_t *a, unsigned k) {
  canor_big_t t;
  int limbs = (int)(k / CANOR_BIG_LIMB_BITS), bits = (int)(k % CANOR_BIG_LIMB_BITS), i, n;
  if (a->n == 0) { canor_big_init(r); return CANOR_OK; }
  if (canor_big_bits(a) + (int64_t)k > CANOR_BIG_BITS_MAX) return CANOR_ERR_FULL;
  n = a->n + limbs + 1;
  if (n > CANOR_BIG_LIMBS_MAX) n = CANOR_BIG_LIMBS_MAX;
  memset(t.d, 0, (size_t)n * sizeof *t.d);
  for (i = 0; i < a->n; ++i) {
    uint64_t v = (uint64_t)a->d[i] << bits;
    t.d[i + limbs] |= (uint32_t)(v & LIMB_MASK);
    if (i + limbs + 1 < n) t.d[i + limbs + 1] |= (uint32_t)(v >> CANOR_BIG_LIMB_BITS);
  }
  t.n = n;
  t.sign = a->sign;
  big_trim(&t);
  big_copy(r, &t);
  return CANOR_OK;
}

void canor_big_shr(canor_big_t *r, const canor_big_t *a, unsigned k) {
  int limbs = (int)(k / CANOR_BIG_LIMB_BITS), bits = (int)(k % CANOR_BIG_LIMB_BITS), i;
  int n = a->n - limbs, sg = a->sign;
  if (n <= 0) { canor_big_init(r); return; }
  /* Ascending, so reading limb i + limbs (+1) never sees a written limb even
   * when r aliases a. */
  for (i = 0; i < n; ++i) {
    uint64_t v = a->d[i + limbs] >> bits;
    if (bits && i + limbs + 1 < a->n)
      v |= (uint64_t)a->d[i + limbs + 1] << (CANOR_BIG_LIMB_BITS - bits);
    r->d[i] = (uint32_t)(v & LIMB_MASK);
  }
  r->n = n;
  r->sign = sg;
  big_trim(r);
}

/* Binary GCD of |a| and |b|. No division anywhere. */
void canor_big_gcd(canor_big_t *r, const canor_big_t *a, const canor_big_t *b) {
  canor_big_t x, y;
  int shift, sx, sy;
  if (a->n == 0) { big_copy(r, b); canor_big_abs(r); return; }
  if (b->n == 0) { big_copy(r, a); canor_big_abs(r); return; }
  big_copy(&x, a); canor_big_abs(&x);
  big_copy(&y, b); canor_big_abs(&y);
  sx = canor_big_ctz(&x);
  sy = canor_big_ctz(&y);
  shift = sx < sy ? sx : sy;
  canor_big_shr(&x, &x, (unsigned)sx);
  canor_big_shr(&y, &y, (unsigned)sy);
  while (y.n) {
    if (canor_big_cmp_abs(&x, &y) > 0) { canor_big_t t = x; x = y; y = t; }   /* keep x <= y */
    mag_sub(&y, &y, &x);
    if (y.n) canor_big_shr(&y, &y, (unsigned)canor_big_ctz(&y));
  }
  /* The gcd divides a and b, so shifting it back can never exceed the
   * capacity they already fit in. */
  (void)canor_big_shl(r, &x, (unsigned)shift);
}

canor_status canor_big_pow_u32(canor_big_t *r, const canor_big_t *a, uint32_t e) {
  canor_big_t acc, base;
  canor_status st;
  canor_big_set_i64(&acc, 1);
  big_copy(&base, a);
  while (e) {
    if (e & 1u) { st = canor_big_mul(&acc, &acc, &base); if (st) return st; }
    e >>= 1;
    if (e) { st = canor_big_mul(&base, &base, &base); if (st) return st; }
  }
  big_copy(r, &acc);
  return CANOR_OK;
}

/* --- division -------------------------------------------------------------- */

canor_status canor_big_divmod(canor_big_t *q, canor_big_t *rem,
                              const canor_big_t *a, const canor_big_t *b) {
  canor_big_t quo, r, bb;
  int bit, top;
  if (b->n == 0) return CANOR_ERR_UNDEFINED;
  canor_big_init(&quo);
  canor_big_init(&r);
  big_copy(&bb, b);
  canor_big_abs(&bb);
  top = canor_big_bits(a);
  /* Shift-and-subtract, one bit at a time. The quotient and remainder are
   * both bounded by |a|, so neither can exceed the capacity. */
  if (top > 0) {
    quo.n = (top + CANOR_BIG_LIMB_BITS - 1) / CANOR_BIG_LIMB_BITS;
    memset(quo.d, 0, (size_t)quo.n * sizeof *quo.d);
    quo.sign = 1;
  }
  for (bit = top - 1; bit >= 0; --bit) {
    /* r = 2r + bit. r < |b| <= capacity before the shift, so 2r + 1 < 2|b|
     * fits one bit wider; a full-width |b| is the only case that needs it. */
    if (canor_big_shl(&r, &r, 1) != CANOR_OK) return CANOR_ERR_FULL;
    if (big_bit(a, bit)) {
      if (r.n == 0) { r.d[0] = 1; r.n = 1; r.sign = 1; }
      else r.d[0] |= 1u;
    }
    if (canor_big_cmp_abs(&r, &bb) >= 0) {
      mag_sub(&r, &r, &bb);
      quo.d[bit / CANOR_BIG_LIMB_BITS] |= (uint32_t)1u << (bit % CANOR_BIG_LIMB_BITS);
    }
  }
  big_trim(&quo);
  if (quo.n) quo.sign = a->sign * b->sign;
  if (r.n) r.sign = a->sign;
  if (q) big_copy(q, &quo);
  if (rem) big_copy(rem, &r);
  return CANOR_OK;
}

canor_status canor_big_iroot(canor_big_t *r, const canor_big_t *a, uint32_t k) {
  canor_big_t x, p;
  int bit, top;
  if (k == 0 || a->sign < 0) return CANOR_ERR_ARGS;
  if (a->n == 0 || k == 1) { big_copy(r, a); return CANOR_OK; }
  /* Bit by bit from the top: set a bit, keep it when x^k <= a. Exact, and
   * x^k <= a always fits, so a power that overflows means "too big". */
  top = (canor_big_bits(a) + (int)k - 1) / (int)k;
  canor_big_init(&x);
  for (bit = top; bit >= 0; --bit) {
    canor_big_t c;
    canor_status st;
    if (canor_big_shl(&c, &x, 0) != CANOR_OK) return CANOR_ERR_FULL;   /* copy */
    {
      int limb = bit / CANOR_BIG_LIMB_BITS, i;
      for (i = c.n; i <= limb; ++i) c.d[i] = 0;
      if (c.n <= limb) c.n = limb + 1;
      c.d[limb] |= (uint32_t)1u << (bit % CANOR_BIG_LIMB_BITS);
      c.sign = 1;
    }
    st = canor_big_pow_u32(&p, &c, k);
    if (st == CANOR_OK && canor_big_cmp_abs(&p, a) <= 0) x = c;
    else if (st != CANOR_OK && st != CANOR_ERR_FULL) return st;
  }
  big_copy(r, &x);
  return CANOR_OK;
}

canor_status canor_big_isqrt(canor_big_t *r, const canor_big_t *a) {
  return canor_big_iroot(r, a, 2);
}

canor_status canor_big_to_i64(const canor_big_t *a, int64_t *out) {
  uint64_t m = 0;
  if (canor_big_bits(a) > 63) return CANOR_ERR_RANGE;
  if (a->n > 0) m = a->d[0];
  if (a->n > 1) m |= (uint64_t)a->d[1] << CANOR_BIG_LIMB_BITS;
  *out = a->sign < 0 ? -(int64_t)m : (int64_t)m;
  return CANOR_OK;
}

canor_status canor_big_rat_to_fixed(const canor_big_t *num, const canor_big_t *den,
                                    unsigned shift, int64_t *out) {
  canor_big_t t, q;
  canor_status st;
  int64_t v;
  st = canor_big_shl(&t, num, shift);
  if (st == CANOR_ERR_FULL) {
    /* num * 2^shift does not fit; the quotient still may, when den is wide.
     * Divide first and shift the remainder in afterwards would lose bits, so
     * the honest answer here is RANGE: the caller asked for more precision
     * than the representation can carry. */
    return CANOR_ERR_RANGE;
  }
  if (st) return st;
  st = canor_big_divmod(&q, NULL, &t, den);
  if (st) return st;
  st = canor_big_to_i64(&q, &v);
  if (st) return st;
  *out = v;
  return CANOR_OK;
}

/* --- decimal --------------------------------------------------------------- */

/* Divides |a| by a 32-bit value in place, returning the remainder. */
static uint32_t divmod_small(canor_big_t *a, uint32_t m) {
  uint64_t rem = 0;
  int i;
  for (i = a->n - 1; i >= 0; --i) {
    uint64_t cur = (rem << CANOR_BIG_LIMB_BITS) | a->d[i];
    a->d[i] = (uint32_t)(cur / m);
    rem = cur % m;
  }
  big_trim(a);
  return (uint32_t)rem;
}

static void put_char(char *out, int cap, int *pos, char c) {
  if (*pos < cap - 1) out[*pos] = c;
  ++*pos;
}

int canor_big_format(const canor_big_t *a, char *out, int cap) {
  uint32_t chunks[CANOR_BIG_DEC_CHUNKS_MAX];
  int nch = 0, pos = 0, i;
  canor_big_t t;
  if (a->n == 0) {
    put_char(out, cap, &pos, '0');
    if (cap > 0) out[pos < cap ? pos : cap - 1] = 0;
    return pos;
  }
  big_copy(&t, a);
  canor_big_abs(&t);
  while (t.n && nch < CANOR_BIG_DEC_CHUNKS_MAX) chunks[nch++] = divmod_small(&t, CANOR_BIG_DEC_CHUNK);
  if (a->sign < 0) put_char(out, cap, &pos, '-');
  for (i = nch - 1; i >= 0; --i) {
    char buf[CANOR_BIG_DEC_CHUNK_DIGITS];
    uint32_t c = chunks[i];
    int k, first = 0;
    for (k = CANOR_BIG_DEC_CHUNK_DIGITS - 1; k >= 0; --k) { buf[k] = (char)('0' + c % DEC_BASE); c /= DEC_BASE; }
    if (i == nch - 1) while (first < CANOR_BIG_DEC_CHUNK_DIGITS - 1 && buf[first] == '0') ++first;
    for (k = first; k < CANOR_BIG_DEC_CHUNK_DIGITS; ++k) put_char(out, cap, &pos, buf[k]);
  }
  if (cap > 0) out[pos < cap ? pos : cap - 1] = 0;
  return pos;
}

canor_status canor_big_parse(canor_big_t *a, const char *s) {
  int sg = 1;
  bool any = false;
  canor_big_t t, dig;
  canor_status st;
  if (!a || !s) return CANOR_ERR_ARGS;
  while (*s == ' ') ++s;
  if (*s == '-') { sg = -1; ++s; }
  else if (*s == '+') ++s;
  canor_big_init(&t);
  for (; *s >= '0' && *s <= '9'; ++s) {
    any = true;
    st = canor_big_mul_i64(&t, &t, DEC_BASE);
    if (st) return st;
    canor_big_set_i64(&dig, *s - '0');
    st = canor_big_add(&t, &t, &dig);
    if (st) return st;
  }
  if (!any || *s) return CANOR_ERR_ARGS;
  if (t.n) t.sign = sg;
  big_copy(a, &t);
  return CANOR_OK;
}

/* --- rationals -------------------------------------------------------------- */

void canor_big_rat_init(canor_big_rat_t *q) {
  canor_big_init(&q->num);
  canor_big_set_i64(&q->den, 1);
}

canor_status canor_big_rat_set_i64(canor_big_rat_t *q, int64_t n, int64_t d) {
  canor_big_rat_t t;
  if (d == 0) return CANOR_ERR_UNDEFINED;
  canor_big_set_i64(&t.num, n);
  canor_big_set_i64(&t.den, d);
  (void)canor_big_rat_normalize(&t);   /* cannot fail: den != 0, gcd shrinks */
  *q = t;
  return CANOR_OK;
}

canor_status canor_big_rat_normalize(canor_big_rat_t *q) {
  canor_big_t g;
  if (q->den.sign == 0) return CANOR_ERR_UNDEFINED;
  if (q->den.sign < 0) { canor_big_neg(&q->num); canor_big_neg(&q->den); }
  if (q->num.sign == 0) { canor_big_set_i64(&q->den, 1); return CANOR_OK; }
  canor_big_gcd(&g, &q->num, &q->den);
  /* gcd is 1 far more often than not once values are reduced each step, and
   * the divisions are the expensive part, so skip them when it is identity.
   * Both are exact by construction. */
  if (!canor_big_is_one(&g)) {
    canor_status st = canor_big_divmod(&q->num, NULL, &q->num, &g);
    if (st) return st;
    st = canor_big_divmod(&q->den, NULL, &q->den, &g);
    if (st) return st;
  }
  return CANOR_OK;
}

/* a/b (+|-) c/d = (ad +|- cb) / bd, then reduce. */
static canor_status rat_addsub(canor_big_rat_t *r, const canor_big_rat_t *a,
                               const canor_big_rat_t *b, int sub) {
  canor_big_rat_t t;
  canor_big_t x, y;
  canor_status st;
  if ((st = canor_big_mul(&x, &a->num, &b->den)) != CANOR_OK) return st;
  if ((st = canor_big_mul(&y, &b->num, &a->den)) != CANOR_OK) return st;
  if (sub) canor_big_neg(&y);
  if ((st = canor_big_add(&t.num, &x, &y)) != CANOR_OK) return st;
  if ((st = canor_big_mul(&t.den, &a->den, &b->den)) != CANOR_OK) return st;
  if ((st = canor_big_rat_normalize(&t)) != CANOR_OK) return st;
  *r = t;
  return CANOR_OK;
}

canor_status canor_big_rat_add(canor_big_rat_t *r, const canor_big_rat_t *a, const canor_big_rat_t *b) {
  return rat_addsub(r, a, b, 0);
}
canor_status canor_big_rat_sub(canor_big_rat_t *r, const canor_big_rat_t *a, const canor_big_rat_t *b) {
  return rat_addsub(r, a, b, 1);
}

canor_status canor_big_rat_mul(canor_big_rat_t *r, const canor_big_rat_t *a, const canor_big_rat_t *b) {
  canor_big_rat_t t;
  canor_status st;
  if ((st = canor_big_mul(&t.num, &a->num, &b->num)) != CANOR_OK) return st;
  if ((st = canor_big_mul(&t.den, &a->den, &b->den)) != CANOR_OK) return st;
  if ((st = canor_big_rat_normalize(&t)) != CANOR_OK) return st;
  *r = t;
  return CANOR_OK;
}

canor_status canor_big_rat_inv(canor_big_rat_t *r, const canor_big_rat_t *a) {
  canor_big_rat_t t;
  if (a->num.sign == 0) return CANOR_ERR_UNDEFINED;
  t.num = a->den;
  t.den = a->num;
  if (t.den.sign < 0) { canor_big_neg(&t.num); canor_big_neg(&t.den); }
  *r = t;
  return CANOR_OK;
}

int canor_big_rat_cmp_i64(const canor_big_rat_t *a, int64_t v, canor_status *st) {
  canor_big_t rhs;
  canor_status s = canor_big_mul_i64(&rhs, &a->den, v);
  if (st) *st = s;
  return s == CANOR_OK ? canor_big_cmp(&a->num, &rhs) : 0;
}

int canor_big_rat_cmp(const canor_big_rat_t *a, const canor_big_rat_t *b, canor_status *st) {
  canor_big_t l, r;
  canor_status s = canor_big_mul(&l, &a->num, &b->den);
  if (s == CANOR_OK) s = canor_big_mul(&r, &b->num, &a->den);
  if (st) *st = s;
  return s == CANOR_OK ? canor_big_cmp(&l, &r) : 0;
}

int canor_big_rat_bits(const canor_big_rat_t *a) {
  int bn = canor_big_bits(&a->num), bd = canor_big_bits(&a->den);
  return bn > bd ? bn : bd;
}

int canor_big_rat_format(const canor_big_rat_t *a, char *out, int cap) {
  int n = canor_big_format(&a->num, out, cap);
  if (canor_big_is_one(&a->den)) return n;
  if (n < cap - 1) { out[n] = '/'; out[n + 1] = 0; }
  n += 1;
  if (n < cap) n += canor_big_format(&a->den, out + n, cap - n);
  else n += canor_big_format(&a->den, out, 0);
  return n;
}

canor_status canor_big_rat_parse(canor_big_rat_t *q, const char *s) {
  const char *slash, *dot;
  char buf[CANOR_BIG_TEXT_MAX];
  canor_big_rat_t t;
  canor_status st;
  if (!q || !s) return CANOR_ERR_ARGS;
  slash = strchr(s, '/');
  dot = strchr(s, '.');
  if (slash) {
    size_t ln = (size_t)(slash - s);
    if (ln >= sizeof buf) return CANOR_ERR_FULL;
    memcpy(buf, s, ln);
    buf[ln] = 0;
    if ((st = canor_big_parse(&t.num, buf)) != CANOR_OK) return st;
    if ((st = canor_big_parse(&t.den, slash + 1)) != CANOR_OK) return st;
    if (t.den.sign == 0) return CANOR_ERR_UNDEFINED;
    if ((st = canor_big_rat_normalize(&t)) != CANOR_OK) return st;
    *q = t;
    return CANOR_OK;
  }
  if (dot) {
    /* "-0.3125" -> -3125 / 10^4, exact. A decimal that cannot be written
     * exactly cannot be typed here, so nothing is silently rounded. */
    size_t i = 0;
    uint32_t k = 0;
    int frac = 0;
    const char *p;
    canor_big_t ten;
    for (p = s; *p; ++p) {
      if (*p == '.') { if (frac) return CANOR_ERR_ARGS; frac = 1; continue; }
      if (i + 1 >= sizeof buf) return CANOR_ERR_FULL;
      buf[i++] = *p;
      if (frac) ++k;
    }
    buf[i] = 0;
    if ((st = canor_big_parse(&t.num, buf)) != CANOR_OK) return st;
    canor_big_set_i64(&ten, DEC_BASE);
    if ((st = canor_big_pow_u32(&t.den, &ten, k)) != CANOR_OK) return st;
    if ((st = canor_big_rat_normalize(&t)) != CANOR_OK) return st;
    *q = t;
    return CANOR_OK;
  }
  if ((st = canor_big_parse(&t.num, s)) != CANOR_OK) return st;
  canor_big_set_i64(&t.den, 1);
  *q = t;
  return CANOR_OK;
}
