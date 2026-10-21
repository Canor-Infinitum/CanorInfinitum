/* canor_swar.c - generic limb arithmetic behind the SWAR-SIMD macro library.
 *
 * Canor WD v1 §9 Aether: integer programming with interval arithmetic. Every
 * routine is exact over uint64_t limbs and uses no wider native type, so it
 * builds unchanged for 32-bit Zephyr MCUs, WASM and 64-bit hosts. */

#include "canor_swar.h"

#include <string.h>

#define SWAR_HALF_BITS 32
#define SWAR_HALF_MASK UINT64_C(0xFFFFFFFF)
#define SWAR_SIGN_BIT (UINT64_C(1) << 63)

/* --- narrow lanes: runtime width ------------------------------------------ */

#define SWAR_APPLY_CASE(W)                                              \
  case W:                                                               \
    switch (op) {                                                       \
      case CANOR_SWAR_OP_ADD: *out = CANOR_SWAR_ADD(W, x, y); break;     \
      case CANOR_SWAR_OP_SUB: *out = CANOR_SWAR_SUB(W, x, y); break;     \
      case CANOR_SWAR_OP_CARRY: *out = CANOR_SWAR_CARRY(W, x, y); break; \
      case CANOR_SWAR_OP_BORROW: *out = CANOR_SWAR_BORROW(W, x, y); break; \
      case CANOR_SWAR_OP_MIN: *out = CANOR_SWAR_MIN(W, x, y); break;     \
      case CANOR_SWAR_OP_MAX: *out = CANOR_SWAR_MAX(W, x, y); break;     \
      case CANOR_SWAR_OP_MINS: *out = CANOR_SWAR_MINS(W, x, y); break;   \
      case CANOR_SWAR_OP_MAXS: *out = CANOR_SWAR_MAXS(W, x, y); break;   \
      default: return CANOR_ERR_ARGS;                                   \
    }                                                                   \
    return CANOR_OK;

canor_status canor_swar_lanes_apply(unsigned w, canor_swar_op op, uint64_t x, uint64_t y,
                                    uint64_t *out) {
  if (!out) return CANOR_ERR_ARGS;
  switch (w) {
    SWAR_APPLY_CASE(2)
    SWAR_APPLY_CASE(4)
    SWAR_APPLY_CASE(8)
    SWAR_APPLY_CASE(16)
    SWAR_APPLY_CASE(32)
    SWAR_APPLY_CASE(64)
    default: return CANOR_ERR_ARGS;
  }
}

/* --- wide lanes ------------------------------------------------------------ */

uint64_t canor_swar_mul64(uint64_t a, uint64_t b, uint64_t *hi) {
  uint64_t a0 = a & SWAR_HALF_MASK, a1 = a >> SWAR_HALF_BITS;
  uint64_t b0 = b & SWAR_HALF_MASK, b1 = b >> SWAR_HALF_BITS;
  uint64_t p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
  /* Middle column: three terms each below 2^32 * 2^32, summed with carry. */
  uint64_t mid = (p00 >> SWAR_HALF_BITS) + (p01 & SWAR_HALF_MASK) + (p10 & SWAR_HALF_MASK);
  if (hi)
    *hi = p11 + (p01 >> SWAR_HALF_BITS) + (p10 >> SWAR_HALF_BITS) + (mid >> SWAR_HALF_BITS);
  return (mid << SWAR_HALF_BITS) | (p00 & SWAR_HALF_MASK);
}

void canor_swar_limbs_from_u64(uint64_t *o, uint64_t v, size_t n) {
  if (!o || n == 0) return;
  o[0] = v;
  for (size_t i = 1; i < n; ++i) o[i] = 0;
}

void canor_swar_limbs_from_i64(uint64_t *o, int64_t v, size_t n) {
  if (!o || n == 0) return;
  uint64_t ext = v < 0 ? UINT64_MAX : 0;
  o[0] = (uint64_t)v;
  for (size_t i = 1; i < n; ++i) o[i] = ext;
}

canor_status canor_swar_limbs_to_u64(const uint64_t *a, size_t n, uint64_t *out) {
  if (!a || !out || n == 0) return CANOR_ERR_ARGS;
  for (size_t i = 1; i < n; ++i)
    if (a[i]) return CANOR_ERR_RANGE;
  *out = a[0];
  return CANOR_OK;
}

canor_status canor_swar_limbs_to_i64(const uint64_t *a, size_t n, int64_t *out) {
  if (!a || !out || n == 0) return CANOR_ERR_ARGS;
  /* Fits iff every upper limb repeats the sign of limb 0. */
  uint64_t ext = (a[0] & SWAR_SIGN_BIT) ? UINT64_MAX : 0;
  for (size_t i = 1; i < n; ++i)
    if (a[i] != ext) return CANOR_ERR_RANGE;
  *out = (int64_t)a[0];
  return CANOR_OK;
}

bool canor_swar_limbs_is_zero(const uint64_t *a, size_t n) {
  if (!a) return false;
  for (size_t i = 0; i < n; ++i)
    if (a[i]) return false;
  return true;
}

bool canor_swar_limbs_is_neg(const uint64_t *a, size_t n) {
  return a && n && (a[n - 1] & SWAR_SIGN_BIT);
}

int canor_swar_limbs_cmp_u(const uint64_t *a, const uint64_t *b, size_t n) {
  for (size_t i = n; i-- > 0;) {
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  }
  return 0;
}

int canor_swar_limbs_cmp_s(const uint64_t *a, const uint64_t *b, size_t n) {
  if (n == 0) return 0;
  bool na = canor_swar_limbs_is_neg(a, n), nb = canor_swar_limbs_is_neg(b, n);
  if (na != nb) return na ? -1 : 1;
  /* Same sign: two's complement order equals unsigned order. */
  return canor_swar_limbs_cmp_u(a, b, n);
}

uint64_t canor_swar_limbs_add(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n) {
  uint64_t c = 0;
  for (size_t i = 0; i < n; ++i) {
    uint64_t s = a[i] + b[i];
    uint64_t c1 = s < a[i];
    uint64_t t = s + c;
    uint64_t c2 = t < s;
    o[i] = t;
    c = c1 | c2;
  }
  return c;
}

uint64_t canor_swar_limbs_sub(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n) {
  uint64_t br = 0;
  for (size_t i = 0; i < n; ++i) {
    uint64_t d = a[i] - b[i];
    uint64_t b1 = a[i] < b[i];
    uint64_t t = d - br;
    uint64_t b2 = d < br;
    o[i] = t;
    br = b1 | b2;
  }
  return br;
}

canor_status canor_swar_limbs_add_s(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n) {
  uint64_t r[CANOR_SWAR_LIMBS_MAX];
  if (!o || !a || !b || n == 0 || n > CANOR_SWAR_LIMBS_MAX) return CANOR_ERR_ARGS;
  bool na = canor_swar_limbs_is_neg(a, n), nb = canor_swar_limbs_is_neg(b, n);
  (void)canor_swar_limbs_add(r, a, b, n);
  /* Overflow iff both operands share a sign the result does not. */
  if (na == nb && canor_swar_limbs_is_neg(r, n) != na) return CANOR_ERR_RANGE;
  memcpy(o, r, n * sizeof *r);
  return CANOR_OK;
}

canor_status canor_swar_limbs_sub_s(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n) {
  uint64_t r[CANOR_SWAR_LIMBS_MAX];
  if (!o || !a || !b || n == 0 || n > CANOR_SWAR_LIMBS_MAX) return CANOR_ERR_ARGS;
  bool na = canor_swar_limbs_is_neg(a, n), nb = canor_swar_limbs_is_neg(b, n);
  (void)canor_swar_limbs_sub(r, a, b, n);
  /* Overflow iff the operands differ in sign and the result takes b's. */
  if (na != nb && canor_swar_limbs_is_neg(r, n) != na) return CANOR_ERR_RANGE;
  memcpy(o, r, n * sizeof *r);
  return CANOR_OK;
}

uint64_t canor_swar_limbs_mul_u64(uint64_t *o, const uint64_t *a, uint64_t m, size_t n) {
  uint64_t carry = 0;
  for (size_t i = 0; i < n; ++i) {
    uint64_t hi;
    uint64_t lo = canor_swar_mul64(a[i], m, &hi);
    lo += carry;
    hi += lo < carry;
    o[i] = lo;
    carry = hi;
  }
  return carry;
}

canor_status canor_swar_limbs_divmod_u64(uint64_t *q, const uint64_t *a, uint64_t d, size_t n,
                                         uint64_t *rem) {
  if (!a || n == 0) return CANOR_ERR_ARGS;
  if (d == 0) return CANOR_ERR_UNDEFINED;
  /* Restoring long division one bit at a time. The running remainder stays
   * below d, so after a shift it is below 2d <= 2^65: `top` holds the bit that
   * fell off, and when it is set the true value certainly exceeds d. */
  uint64_t r = 0;
  for (size_t i = n; i-- > 0;) {
    uint64_t word = a[i], qw = 0;
    for (int b = 63; b >= 0; --b) {
      uint64_t top = r >> 63;
      r = (r << 1) | ((word >> b) & 1u);
      qw <<= 1;
      if (top || r >= d) {
        r -= d;
        qw |= 1u;
      }
    }
    if (q) q[i] = qw;
  }
  if (rem) *rem = r;
  return CANOR_OK;
}

canor_status canor_swar_limbs_to_factoradic(const uint64_t *a, size_t n, uint16_t *digits,
                                            size_t cap, size_t *count) {
  uint64_t x[CANOR_SWAR_LIMBS_MAX];
  if (!a || !digits || !count || n == 0 || n > CANOR_SWAR_LIMBS_MAX) return CANOR_ERR_ARGS;
  if (cap == 0) return CANOR_ERR_FULL;
  memcpy(x, a, n * sizeof *x);
  /* digit k = (x / k!) mod (k+1): divide by the radices 1, 2, 3, ... in turn. */
  size_t k = 0;
  digits[k++] = 0;
  uint64_t radix = 2;
  while (!canor_swar_limbs_is_zero(x, n)) {
    uint64_t r;
    if (k >= cap) return CANOR_ERR_FULL;
    (void)canor_swar_limbs_divmod_u64(x, x, radix, n, &r);
    digits[k++] = (uint16_t)r;
    ++radix;
  }
  *count = k;
  return CANOR_OK;
}

canor_status canor_swar_limbs_from_factoradic(uint64_t *o, size_t n, const uint16_t *digits,
                                              size_t count) {
  uint64_t x[CANOR_SWAR_LIMBS_MAX], t[CANOR_SWAR_LIMBS_MAX], d[CANOR_SWAR_LIMBS_MAX];
  if (!o || !digits || n == 0 || n > CANOR_SWAR_LIMBS_MAX || count == 0) return CANOR_ERR_ARGS;
  for (size_t k = 0; k < count; ++k)
    if (digits[k] > k) return CANOR_ERR_RANGE;
  /* Horner over the radices: x = (...(d_{c-1} * (c-1) + d_{c-2}) * (c-2) ...) + d_0. */
  canor_swar_limbs_from_u64(x, digits[count - 1], n);
  for (size_t k = count - 1; k >= 1; --k) {
    if (canor_swar_limbs_mul_u64(t, x, (uint64_t)k, n)) return CANOR_ERR_RANGE;
    canor_swar_limbs_from_u64(d, digits[k - 1], n);
    if (canor_swar_limbs_add(x, t, d, n)) return CANOR_ERR_RANGE;
  }
  memcpy(o, x, n * sizeof *x);
  return CANOR_OK;
}
