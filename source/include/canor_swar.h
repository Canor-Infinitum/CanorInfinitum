#ifndef CANOR_SWAR_H
#define CANOR_SWAR_H

/* SWAR-SIMD macro library: SIMD-within-a-register lanes for Physical
 * Programming over exact integers and closed LO/HI integer intervals.
 *
 * Canor WD v1 §9 Aether mandates "Integer Programming with Interval
 * Arithmetic ... throughout all of Canor", so every lane here is an exact
 * integer and every interval is a closed integer [lo, hi]. Results are
 * bit-identical on every host (AGENTS.md invariant 1).
 *
 * TWO FAMILIES, both resolved at compile time:
 *
 *   NARROW  W in {2, 4, 8, 16, 32, 64}. 64/W unsigned lanes packed in one
 *           uint64_t word. Lane add/sub wrap modulo 2^W and NEVER carry into
 *           the neighbouring lane; CANOR_SWAR_CARRY / CANOR_SWAR_BORROW expose
 *           the per-lane overflow so a caller diagnoses instead of silently
 *           wrapping (invariant 3). The CANOR_SWAR_* macros take W as a
 *           constant and evaluate their word arguments more than once, so the
 *           arguments must be side-effect free - which Elite assembly already
 *           requires (invariant 4).
 *
 *   WIDE    W in {128, 256, 512, 1024, 2048, 4096}. One lane is W/64
 *           little-endian uint64_t limbs, two's complement when read signed.
 *           These are the BigNum limbs for factoradic ranks and hypervectors:
 *           CANOR_HV_BITS = 1024 is canor_swar_u1024_t. Types and wrappers are
 *           stamped out per width by CANOR_SWAR_DEFINE_WIDE; the arithmetic
 *           itself lives once, in canor_swar_limbs_*.
 *
 * FACTORADIC. A wide value converts to and from the factorial number system,
 * x = sum d_k * k! with 0 <= d_k <= k. That is the Lehmer address MEMORY.md
 * pins ("Factoradic topology is never materialized"), carried at BigNum width.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- constants ------------------------------------------------------------- */

#define CANOR_SWAR_WORD_BITS 64          /* one machine word of lanes */
#define CANOR_SWAR_NARROW_MIN 2          /* 2-bit lanes: FlowValue width (SCHEMA) */
#define CANOR_SWAR_NARROW_MAX 64
#define CANOR_SWAR_WIDE_MIN 128
#define CANOR_SWAR_WIDE_MAX 4096
#define CANOR_SWAR_LIMBS(W) ((W) / CANOR_SWAR_WORD_BITS)
#define CANOR_SWAR_LIMBS_MAX CANOR_SWAR_LIMBS(CANOR_SWAR_WIDE_MAX)
/* 4096-bit values need at most 1000 factoradic digits: 1000! exceeds 2^8000. */
#ifndef CANOR_SWAR_FACTORADIC_MAX
#define CANOR_SWAR_FACTORADIC_MAX 1000
#endif

#define CANOR_SWAR_IS_NARROW(W) \
  ((W) == 2 || (W) == 4 || (W) == 8 || (W) == 16 || (W) == 32 || (W) == 64)
#define CANOR_SWAR_IS_WIDE(W) \
  ((W) == 128 || (W) == 256 || (W) == 512 || (W) == 1024 || (W) == 2048 || (W) == 4096)

/* --- narrow lanes: masks --------------------------------------------------- */

#define CANOR_SWAR_LANES(W) (CANOR_SWAR_WORD_BITS / (W))
/* All W bits of one lane. The (W) & 63 keeps the shift defined at W = 64,
 * where the other branch is the one taken. */
#define CANOR_SWAR_LANE_MASK(W) \
  ((W) >= CANOR_SWAR_WORD_BITS ? UINT64_MAX : ((UINT64_C(1) << ((W) & 63)) - 1u))
/* Lowest bit of every lane: (2^64 - 1) / (2^W - 1) = sum 2^(kW). Exact for
 * every narrow W, including W = 64 where it is 1. */
#define CANOR_SWAR_LO(W) (UINT64_MAX / CANOR_SWAR_LANE_MASK(W))
/* Highest bit of every lane. */
#define CANOR_SWAR_HI(W) (CANOR_SWAR_LO(W) << ((W) - 1))

/* --- narrow lanes: access -------------------------------------------------- */

#define CANOR_SWAR_GET(W, x, i) \
  (((x) >> ((unsigned)(i) * (W))) & CANOR_SWAR_LANE_MASK(W))
#define CANOR_SWAR_SET(W, x, i, v)                                         \
  (((x) & ~(CANOR_SWAR_LANE_MASK(W) << ((unsigned)(i) * (W)))) |           \
   (((uint64_t)(v) & CANOR_SWAR_LANE_MASK(W)) << ((unsigned)(i) * (W))))
#define CANOR_SWAR_BROADCAST(W, v) \
  (((uint64_t)(v) & CANOR_SWAR_LANE_MASK(W)) * CANOR_SWAR_LO(W))

/* --- narrow lanes: arithmetic ---------------------------------------------- */

/* Lane-wise add/sub modulo 2^W. The top bit of each lane is computed apart
 * from the rest, which is what stops a carry crossing into the next lane. */
#define CANOR_SWAR_ADD(W, x, y)                                            \
  ((((x) & ~CANOR_SWAR_HI(W)) + ((y) & ~CANOR_SWAR_HI(W))) ^               \
   (((x) ^ (y)) & CANOR_SWAR_HI(W)))
#define CANOR_SWAR_SUB(W, x, y)                                            \
  ((((x) | CANOR_SWAR_HI(W)) - ((y) & ~CANOR_SWAR_HI(W))) ^                \
   (((x) ^ ~(y)) & CANOR_SWAR_HI(W)))

/* Per-lane carry-out (unsigned overflow of ADD) and borrow-out (unsigned
 * x < y in SUB), reported in the top bit of each lane. Both are the majority
 * function of the two top bits and the carry/borrow into the top bit. */
#define CANOR_SWAR_CARRY(W, x, y)                                          \
  ((((x) & (y)) | (((x) | (y)) & ~CANOR_SWAR_ADD(W, x, y))) & CANOR_SWAR_HI(W))
#define CANOR_SWAR_BORROW(W, x, y)                                         \
  (((~(x) & (y)) | ((~(x) | (y)) & CANOR_SWAR_SUB(W, x, y))) & CANOR_SWAR_HI(W))

/* Spreads each lane's top bit across the whole lane. A lane holds at most
 * 1 * (2^W - 1) after the multiply, so no product spills into its neighbour. */
#define CANOR_SWAR_SPREAD(W, t) \
  ((((t) & CANOR_SWAR_HI(W)) >> ((W) - 1)) * CANOR_SWAR_LANE_MASK(W))

/* Full-lane masks: unsigned x < y, and signed x < y (top bit flipped, which
 * maps two's complement order onto unsigned order). */
#define CANOR_SWAR_LT_MASK(W, x, y) CANOR_SWAR_SPREAD(W, CANOR_SWAR_BORROW(W, x, y))
#define CANOR_SWAR_LTS_MASK(W, x, y) \
  CANOR_SWAR_LT_MASK(W, (x) ^ CANOR_SWAR_HI(W), (y) ^ CANOR_SWAR_HI(W))

#define CANOR_SWAR_MIN(W, x, y) ((y) ^ (((x) ^ (y)) & CANOR_SWAR_LT_MASK(W, x, y)))
#define CANOR_SWAR_MAX(W, x, y) ((x) ^ (((x) ^ (y)) & CANOR_SWAR_LT_MASK(W, x, y)))
#define CANOR_SWAR_MINS(W, x, y) ((y) ^ (((x) ^ (y)) & CANOR_SWAR_LTS_MASK(W, x, y)))
#define CANOR_SWAR_MAXS(W, x, y) ((x) ^ (((x) ^ (y)) & CANOR_SWAR_LTS_MASK(W, x, y)))

/* --- narrow lanes: closed unsigned intervals -------------------------------
 * 64/W intervals [lo_i, hi_i] side by side: lane i of `lo` with lane i of
 * `hi`. Valid when lo_i <= hi_i in every lane. */
typedef struct { uint64_t lo, hi; } canor_swar_lanes_t;

/* Stamps out canor_swar_ivW_valid / _add / _sub for one narrow width.
 *   add  [a,b] + [c,d] = [a+c, b+d]. A carry out of any hi lane is RANGE:
 *        the enclosure no longer fits in W bits, and wrapping would invert it.
 *   sub  [a,b] - [c,d] = [a-d, b-c]. A borrow in any lo lane is RANGE: the
 *        lanes are unsigned, so a negative bound is not representable.
 * `out` is written only on CANOR_OK. */
#define CANOR_SWAR_DEFINE_NARROW(W)                                              \
  static inline bool canor_swar_iv##W##_valid(canor_swar_lanes_t a) {            \
    return CANOR_SWAR_LT_MASK(W, a.hi, a.lo) == 0;                               \
  }                                                                              \
  static inline canor_status canor_swar_iv##W##_add(canor_swar_lanes_t a,        \
                                                    canor_swar_lanes_t b,        \
                                                    canor_swar_lanes_t *out) {   \
    canor_swar_lanes_t r;                                                        \
    if (!out || !canor_swar_iv##W##_valid(a) || !canor_swar_iv##W##_valid(b))    \
      return CANOR_ERR_ARGS;                                                     \
    if (CANOR_SWAR_CARRY(W, a.hi, b.hi)) return CANOR_ERR_RANGE;                 \
    r.lo = CANOR_SWAR_ADD(W, a.lo, b.lo);                                        \
    r.hi = CANOR_SWAR_ADD(W, a.hi, b.hi);                                        \
    *out = r;                                                                    \
    return CANOR_OK;                                                             \
  }                                                                              \
  static inline canor_status canor_swar_iv##W##_sub(canor_swar_lanes_t a,        \
                                                    canor_swar_lanes_t b,        \
                                                    canor_swar_lanes_t *out) {   \
    canor_swar_lanes_t r;                                                        \
    if (!out || !canor_swar_iv##W##_valid(a) || !canor_swar_iv##W##_valid(b))    \
      return CANOR_ERR_ARGS;                                                     \
    if (CANOR_SWAR_BORROW(W, a.lo, b.hi)) return CANOR_ERR_RANGE;                \
    r.lo = CANOR_SWAR_SUB(W, a.lo, b.hi);                                        \
    r.hi = CANOR_SWAR_SUB(W, a.hi, b.lo);                                        \
    *out = r;                                                                    \
    return CANOR_OK;                                                             \
  }

CANOR_SWAR_DEFINE_NARROW(2)
CANOR_SWAR_DEFINE_NARROW(4)
CANOR_SWAR_DEFINE_NARROW(8)
CANOR_SWAR_DEFINE_NARROW(16)
CANOR_SWAR_DEFINE_NARROW(32)
CANOR_SWAR_DEFINE_NARROW(64)

/* --- narrow lanes: runtime width -------------------------------------------
 * The same operations with W chosen at run time (a profile, not a constant).
 * An unsupported width is CANOR_ERR_ARGS, never a silent fallback. */
typedef enum {
  CANOR_SWAR_OP_ADD = 0,
  CANOR_SWAR_OP_SUB,
  CANOR_SWAR_OP_CARRY,
  CANOR_SWAR_OP_BORROW,
  CANOR_SWAR_OP_MIN,
  CANOR_SWAR_OP_MAX,
  CANOR_SWAR_OP_MINS,
  CANOR_SWAR_OP_MAXS
} canor_swar_op;

canor_status canor_swar_lanes_apply(unsigned w, canor_swar_op op, uint64_t x, uint64_t y,
                                    uint64_t *out);

/* --- wide lanes: generic limb arithmetic -----------------------------------
 * n limbs, little-endian. Outputs may alias inputs. */

/* 64 x 64 -> 128 using 32-bit halves only; returns the low word. */
uint64_t canor_swar_mul64(uint64_t a, uint64_t b, uint64_t *hi);

void canor_swar_limbs_from_u64(uint64_t *o, uint64_t v, size_t n);
void canor_swar_limbs_from_i64(uint64_t *o, int64_t v, size_t n);   /* sign-extends */
canor_status canor_swar_limbs_to_u64(const uint64_t *a, size_t n, uint64_t *out);
canor_status canor_swar_limbs_to_i64(const uint64_t *a, size_t n, int64_t *out);
bool canor_swar_limbs_is_zero(const uint64_t *a, size_t n);
bool canor_swar_limbs_is_neg(const uint64_t *a, size_t n);
int canor_swar_limbs_cmp_u(const uint64_t *a, const uint64_t *b, size_t n);
int canor_swar_limbs_cmp_s(const uint64_t *a, const uint64_t *b, size_t n);
/* Unsigned: return the carry / borrow out of the top limb (0 or 1). */
uint64_t canor_swar_limbs_add(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n);
uint64_t canor_swar_limbs_sub(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n);
/* Signed: CANOR_ERR_RANGE on two's complement overflow, `o` then unchanged. */
canor_status canor_swar_limbs_add_s(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n);
canor_status canor_swar_limbs_sub_s(uint64_t *o, const uint64_t *a, const uint64_t *b, size_t n);
/* Unsigned o = a * m; returns the limb carried out of the top. */
uint64_t canor_swar_limbs_mul_u64(uint64_t *o, const uint64_t *a, uint64_t m, size_t n);
/* Unsigned q = a / d, *rem = a % d. d = 0 is CANOR_ERR_UNDEFINED (diagnosed,
 * never an infinity). `q` may be NULL when only the remainder is wanted. */
canor_status canor_swar_limbs_divmod_u64(uint64_t *q, const uint64_t *a, uint64_t d, size_t n,
                                         uint64_t *rem);
/* Factorial number system. digits[k] is the coefficient of k!, so digits[0]
 * is always 0 and digits[k] <= k. *count receives the digit count (at least
 * 1). CANOR_ERR_FULL if `cap` is too small. */
canor_status canor_swar_limbs_to_factoradic(const uint64_t *a, size_t n, uint16_t *digits,
                                            size_t cap, size_t *count);
/* CANOR_ERR_RANGE if a digit exceeds its radix or the value does not fit. */
canor_status canor_swar_limbs_from_factoradic(uint64_t *o, size_t n, const uint16_t *digits,
                                              size_t count);

/* --- wide lanes: typed widths ----------------------------------------------
 * canor_swar_uW_t is one W-bit lane; canor_swar_ivW_t is a closed signed
 * interval [lo, hi] of two of them. */
#define CANOR_SWAR_DEFINE_WIDE(W)                                                       \
  typedef struct { uint64_t w[CANOR_SWAR_LIMBS(W)]; } canor_swar_u##W##_t;              \
  typedef struct { canor_swar_u##W##_t lo, hi; } canor_swar_iv##W##_t;                  \
  static inline void canor_swar_u##W##_from_u64(canor_swar_u##W##_t *o, uint64_t v) {   \
    canor_swar_limbs_from_u64(o->w, v, CANOR_SWAR_LIMBS(W));                            \
  }                                                                                     \
  static inline void canor_swar_u##W##_from_i64(canor_swar_u##W##_t *o, int64_t v) {    \
    canor_swar_limbs_from_i64(o->w, v, CANOR_SWAR_LIMBS(W));                            \
  }                                                                                     \
  static inline canor_status canor_swar_u##W##_to_u64(const canor_swar_u##W##_t *a,     \
                                                      uint64_t *out) {                  \
    return canor_swar_limbs_to_u64(a->w, CANOR_SWAR_LIMBS(W), out);                     \
  }                                                                                     \
  static inline int canor_swar_u##W##_cmp(const canor_swar_u##W##_t *a,                \
                                          const canor_swar_u##W##_t *b) {               \
    return canor_swar_limbs_cmp_u(a->w, b->w, CANOR_SWAR_LIMBS(W));                     \
  }                                                                                     \
  static inline int canor_swar_u##W##_cmp_s(const canor_swar_u##W##_t *a,              \
                                            const canor_swar_u##W##_t *b) {             \
    return canor_swar_limbs_cmp_s(a->w, b->w, CANOR_SWAR_LIMBS(W));                     \
  }                                                                                     \
  static inline uint64_t canor_swar_u##W##_add(canor_swar_u##W##_t *o,                  \
                                               const canor_swar_u##W##_t *a,            \
                                               const canor_swar_u##W##_t *b) {          \
    return canor_swar_limbs_add(o->w, a->w, b->w, CANOR_SWAR_LIMBS(W));                 \
  }                                                                                     \
  static inline uint64_t canor_swar_u##W##_sub(canor_swar_u##W##_t *o,                  \
                                               const canor_swar_u##W##_t *a,            \
                                               const canor_swar_u##W##_t *b) {          \
    return canor_swar_limbs_sub(o->w, a->w, b->w, CANOR_SWAR_LIMBS(W));                 \
  }                                                                                     \
  static inline uint64_t canor_swar_u##W##_mul_u64(canor_swar_u##W##_t *o,              \
                                                   const canor_swar_u##W##_t *a,        \
                                                   uint64_t m) {                        \
    return canor_swar_limbs_mul_u64(o->w, a->w, m, CANOR_SWAR_LIMBS(W));                \
  }                                                                                     \
  static inline canor_status canor_swar_u##W##_divmod_u64(canor_swar_u##W##_t *q,       \
                                                          const canor_swar_u##W##_t *a, \
                                                          uint64_t d, uint64_t *rem) {  \
    return canor_swar_limbs_divmod_u64(q ? q->w : NULL, a->w, d, CANOR_SWAR_LIMBS(W),   \
                                       rem);                                            \
  }                                                                                     \
  static inline canor_status canor_swar_u##W##_to_factoradic(                           \
      const canor_swar_u##W##_t *a, uint16_t *digits, size_t cap, size_t *count) {      \
    return canor_swar_limbs_to_factoradic(a->w, CANOR_SWAR_LIMBS(W), digits, cap,       \
                                          count);                                       \
  }                                                                                     \
  static inline canor_status canor_swar_u##W##_from_factoradic(                         \
      canor_swar_u##W##_t *o, const uint16_t *digits, size_t count) {                   \
    return canor_swar_limbs_from_factoradic(o->w, CANOR_SWAR_LIMBS(W), digits, count);  \
  }                                                                                     \
  static inline bool canor_swar_iv##W##_valid(const canor_swar_iv##W##_t *a) {          \
    return canor_swar_limbs_cmp_s(a->lo.w, a->hi.w, CANOR_SWAR_LIMBS(W)) <= 0;          \
  }                                                                                     \
  /* [a,b] + [c,d] = [a+c, b+d]; computed into a temporary so `o` may alias. */        \
  static inline canor_status canor_swar_iv##W##_add(canor_swar_iv##W##_t *o,            \
                                                    const canor_swar_iv##W##_t *a,      \
                                                    const canor_swar_iv##W##_t *b) {    \
    canor_swar_iv##W##_t r;                                                             \
    canor_status s;                                                                     \
    if (!o || !canor_swar_iv##W##_valid(a) || !canor_swar_iv##W##_valid(b))             \
      return CANOR_ERR_ARGS;                                                            \
    s = canor_swar_limbs_add_s(r.lo.w, a->lo.w, b->lo.w, CANOR_SWAR_LIMBS(W));          \
    if (s != CANOR_OK) return s;                                                        \
    s = canor_swar_limbs_add_s(r.hi.w, a->hi.w, b->hi.w, CANOR_SWAR_LIMBS(W));          \
    if (s != CANOR_OK) return s;                                                        \
    *o = r;                                                                             \
    return CANOR_OK;                                                                    \
  }                                                                                     \
  /* [a,b] - [c,d] = [a-d, b-c]. */                                                     \
  static inline canor_status canor_swar_iv##W##_sub(canor_swar_iv##W##_t *o,            \
                                                    const canor_swar_iv##W##_t *a,      \
                                                    const canor_swar_iv##W##_t *b) {    \
    canor_swar_iv##W##_t r;                                                             \
    canor_status s;                                                                     \
    if (!o || !canor_swar_iv##W##_valid(a) || !canor_swar_iv##W##_valid(b))             \
      return CANOR_ERR_ARGS;                                                            \
    s = canor_swar_limbs_sub_s(r.lo.w, a->lo.w, b->hi.w, CANOR_SWAR_LIMBS(W));          \
    if (s != CANOR_OK) return s;                                                        \
    s = canor_swar_limbs_sub_s(r.hi.w, a->hi.w, b->lo.w, CANOR_SWAR_LIMBS(W));          \
    if (s != CANOR_OK) return s;                                                        \
    *o = r;                                                                             \
    return CANOR_OK;                                                                    \
  }

CANOR_SWAR_DEFINE_WIDE(128)
CANOR_SWAR_DEFINE_WIDE(256)
CANOR_SWAR_DEFINE_WIDE(512)
CANOR_SWAR_DEFINE_WIDE(1024)
CANOR_SWAR_DEFINE_WIDE(2048)
CANOR_SWAR_DEFINE_WIDE(4096)

#ifdef __cplusplus
}
#endif
#endif /* CANOR_SWAR_H */
