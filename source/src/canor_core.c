/* Canor-Core type tower. See include/fc.h.
 *
 * No floating point. BASIS evaluates to a fraction, so the arithmetic here is
 * exact rational arithmetic on int64 pairs; a double would discard exactly the
 * property the design rests on.
 */

#include "canor_core.h"

#include <stdio.h>
#include <string.h>

const char *canor_strerror(canor_status s) {
  switch (s) {
    case CANOR_OK: return "ok";
    case CANOR_ERR_ARGS: return "invalid arguments";
    case CANOR_ERR_RANGE: return "value out of range";
    case CANOR_ERR_FULL: return "container full";
    case CANOR_ERR_NOT_CLOSED: return "imaginary dimension does not close under Cayley-Dickson";
    case CANOR_ERR_UNDEFINED: return "undefined result";
    case CANOR_ERR_NOT_FOUND: return "element not found";
  }
  return "unknown";
}

/* --- exact rationals ------------------------------------------------------ */

/* OVERFLOW. These routines used to multiply int64 numerators directly. Signed
 * overflow is undefined behaviour, which section 39 rules out, and it is not a
 * theoretical concern here: a rational whose denominator squares each step
 * reaches the end of int64 in a handful of iterations, and the symptom is a
 * plausible wrong number rather than a crash.
 *
 * The fix is three things, in this order of preference:
 *
 *   1. DON'T OVERFLOW. Cancel across the operands before multiplying, not
 *      after. (a.num/b.den) and (b.num/a.den) share factors far more often
 *      than not, and cancelling first keeps the intermediate products inside
 *      the range for every input this core actually sees.
 *   2. WHEN IT STILL WOULD, SAY SO. A checked multiply, and a poison value on
 *      failure - never a truncated or wrapped result.
 *   3. FOR COMPARISON, DON'T MULTIPLY AT ALL. canor_rat_cmp uses the continued
 *      fraction expansion, which needs only division and subtraction and
 *      therefore cannot overflow for any pair of valid rationals. Comparison
 *      has no error channel, so making it infallible is worth more than making
 *      it fast.
 *
 * THE POISON. den == 0 now means "not a value", and every operation propagates
 * it. The previous code returned 0/1 for a zero denominator, which is a
 * perfectly good number and therefore the worst possible answer: it turns an
 * arithmetic failure into a silent zero. A caller that ignores canor_rat_valid
 * now gets a value that fails every subsequent check instead of one that
 * quietly poisons the result. */

static int64_t gcd64(int64_t a, int64_t b) {
  /* INT64_MIN has no positive magnitude, so it cannot be negated into range.
   * Callers reject it before reaching here; this is belt and braces. */
  if (a == INT64_MIN || b == INT64_MIN) return 1;
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) { int64_t t = a % b; a = b; b = t; }
  return a ? a : 1;
}

static bool canor_mul64(int64_t a, int64_t b, int64_t *out) {
  if (a == 0 || b == 0) { *out = 0; return true; }
  if (a == INT64_MIN || b == INT64_MIN) return false;
  {
    int64_t aa = a < 0 ? -a : a, ab = b < 0 ? -b : b;
    if (aa > INT64_MAX / ab) return false;
  }
  *out = a * b;
  return true;
}

static bool canor_add64(int64_t a, int64_t b, int64_t *out) {
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return false;
  *out = a + b;
  return true;
}

static canor_rational canor_rat_poison(void) {
  canor_rational r;
  r.num = 0;
  r.den = 0;
  return r;
}

bool canor_rat_valid(canor_rational a) { return a.den > 0; }

canor_rational canor_rat(int64_t num, int64_t den) {
  canor_rational r;
  int64_t g;
  if (den == 0) return canor_rat_poison();
  /* Neither can be negated into range, and reduction needs the magnitude. */
  if (num == INT64_MIN || den == INT64_MIN) return canor_rat_poison();
  if (den < 0) { num = -num; den = -den; }
  g = gcd64(num, den);
  r.num = num / g;
  r.den = den / g;
  return r;
}

canor_rational canor_rat_add(canor_rational a, canor_rational b) {
  int64_t g, ad, bd, den, l, rr, num;
  if (!canor_rat_valid(a) || !canor_rat_valid(b)) return canor_rat_poison();
  /* Least common denominator, not the product: a.den * b.den overflows on
   * pairs whose LCM comfortably fits, and the two are equal only when the
   * denominators are already coprime. */
  g = gcd64(a.den, b.den);
  ad = a.den / g;
  bd = b.den / g;
  if (!canor_mul64(a.den, bd, &den)) return canor_rat_poison();
  if (!canor_mul64(a.num, bd, &l)) return canor_rat_poison();
  if (!canor_mul64(b.num, ad, &rr)) return canor_rat_poison();
  if (!canor_add64(l, rr, &num)) return canor_rat_poison();
  return canor_rat(num, den);
}

canor_rational canor_rat_mul(canor_rational a, canor_rational b) {
  int64_t g1, g2, num, den;
  if (!canor_rat_valid(a) || !canor_rat_valid(b)) return canor_rat_poison();
  /* Cross-cancel first. (2/3)*(3/2) must not need six-digit intermediates to
   * reach 1, and more to the point an iterated product stays in range far
   * longer when the common factors never enter the multiplication. */
  g1 = gcd64(a.num, b.den);
  g2 = gcd64(b.num, a.den);
  if (a.num == INT64_MIN || b.num == INT64_MIN) return canor_rat_poison();
  if (!canor_mul64(a.num / g1, b.num / g2, &num)) return canor_rat_poison();
  if (!canor_mul64(a.den / g2, b.den / g1, &den)) return canor_rat_poison();
  return canor_rat(num, den);
}

bool canor_rat_eq(canor_rational a, canor_rational b) {
  /* Both are reduced with den > 0, so structural equality is value equality.
   * A poison value is equal to nothing, including another poison value:
   * "these two computations both failed" is not evidence they agree. */
  if (!canor_rat_valid(a) || !canor_rat_valid(b)) return false;
  return a.num == b.num && a.den == b.den;
}

/* Floor division, which C's / does not do for negative operands. */
static int64_t canor_fdiv(int64_t n, int64_t d) {      /* d > 0 */
  int64_t q = n / d;
  if (n % d != 0 && n < 0) --q;
  return q;
}

int canor_rat_cmp(canor_rational a, canor_rational b) {
  int64_t an, ad, bn, bd;
  int flip = 1;
  if (!canor_rat_valid(a) || !canor_rat_valid(b)) return 0;
  an = a.num; ad = a.den; bn = b.num; bd = b.den;
  /* Continued fraction comparison. At each step the integer parts either
   * differ - which decides it - or agree, and the comparison passes to the
   * reciprocals of the remainders, which reverses the order. No multiplication
   * occurs, so no input can overflow this. It terminates because each step is
   * a Euclidean step on (den, remainder), which strictly decreases. */
  for (;;) {
    int64_t aq = canor_fdiv(an, ad), bq = canor_fdiv(bn, bd), ar, br;
    if (aq != bq) return aq < bq ? -flip : flip;
    ar = an - aq * ad;                 /* |aq*ad| <= |an|, so this is in range */
    br = bn - bq * bd;
    if (ar == 0 && br == 0) return 0;
    if (ar == 0) return -flip;         /* a is exactly aq; b is more */
    if (br == 0) return flip;
    an = ad; ad = ar;
    bn = bd; bd = br;
    flip = -flip;
  }
}

/* --- BASIS ---------------------------------------------------------------- */

canor_status canor_basis_make(int64_t v, canor_basis *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (v == 0) return CANOR_ERR_RANGE;      /* the document says != 0 */
  out->v = v;
  return CANOR_OK;
}

canor_status canor_basis_parse(const char *text, canor_basis *out) {
  /* `^[+-][1-9][0-9]*$`, enforced literally: the sign is REQUIRED, the first
   * digit may not be zero, and there is nothing else. Written out rather than
   * handed to strtol, which would accept " 12", "0x1f" and a bare "5". */
  if (!text || !out) return CANOR_ERR_ARGS;
  const char *p = text;
  int neg = 0;
  if (*p == '+') neg = 0;
  else if (*p == '-') neg = 1;
  else return CANOR_ERR_ARGS;
  ++p;
  if (*p < '1' || *p > '9') return CANOR_ERR_ARGS;
  int64_t v = 0;
  for (; *p; ++p) {
    if (*p < '0' || *p > '9') return CANOR_ERR_ARGS;
    if (v > (INT64_MAX - (*p - '0')) / 10) return CANOR_ERR_RANGE;
    v = v * 10 + (*p - '0');
  }
  out->v = neg ? -v : v;
  return CANOR_OK;
}

canor_rational canor_basis_eval(canor_basis b) {
  /* "for BASIS < 0: the absolute fractional 1/|BASIS|, and for BASIS > 0: just
   * the value itself". This is the reason eval returns a rational. */
  if (b.v < 0) return canor_rat(1, -b.v);
  return canor_rat(b.v, 1);
}

/* --- NUM / NUMBER / INT --------------------------------------------------- */

canor_status canor_num_make(canor_sign sign, uint64_t value, int64_t basis, int64_t exponent,
                      canor_num *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (sign != CANOR_NEG_ZERO && sign != CANOR_NEG_ONE &&
      sign != CANOR_POS_ONE && sign != CANOR_POS_ZERO) return CANOR_ERR_RANGE;
  canor_basis b, e;
  if (canor_basis_make(basis, &b) != CANOR_OK) return CANOR_ERR_RANGE;
  if (canor_basis_make(exponent, &e) != CANOR_OK) return CANOR_ERR_RANGE;
  out->sign = sign;
  out->value = value;
  out->basis = b;
  out->exponent = e;
  return CANOR_OK;
}

canor_status canor_num_eval(const canor_num *n, canor_rational *out) {
  if (!n || !out) return CANOR_ERR_ARGS;

  canor_rational ex = canor_basis_eval(n->exponent);
  /* A fractional exponent is a root, and a root of a rational is not generally
   * rational. Returning an approximation would put a float back into the
   * substrate through the side door, so it is refused instead. */
  if (ex.den != 1) return CANOR_ERR_UNDEFINED;
  int64_t p = ex.num;
  if (p < 0 || p > 63) return CANOR_ERR_RANGE;

  canor_rational base = canor_basis_eval(n->basis);
  canor_rational acc = canor_rat(1, 1);
  for (int64_t i = 0; i < p; ++i) acc = canor_rat_mul(acc, base);

  canor_rational v = canor_rat((int64_t)n->value, 1);
  canor_rational mag = canor_rat_mul(v, acc);

  /* The four-valued signage. A zero-signed NUM evaluates to zero whatever its
   * magnitude - it is a POLE, not a scale - and the pole itself is carried in
   * the sign field rather than in the value, which is what lets NegZero and
   * PosZero be numerically equal and representationally distinct. */
  switch (n->sign) {
    case CANOR_POS_ONE:  *out = mag; break;
    case CANOR_NEG_ONE:  *out = canor_rat(-mag.num, mag.den); break;
    case CANOR_POS_ZERO:
    case CANOR_NEG_ZERO: *out = canor_rat(0, 1); break;
  }
  return CANOR_OK;
}

void canor_number_zero(canor_number *out) {
  memset(out, 0, sizeof *out);
  out->n = CANOR_NUMBER_DIGITS;
  for (int i = 0; i < CANOR_NUMBER_DIGITS; ++i) {
    out->d[i].sign = CANOR_POS_ONE;
    out->d[i].value = 0;
    out->d[i].basis.v = 1;
    out->d[i].exponent.v = 1;
  }
}

canor_status canor_number_from_u64(uint64_t v, canor_number *out) {
  if (!out) return CANOR_ERR_ARGS;
  canor_number_zero(out);
  /* Factoradic: digit i has radix (i+1), so digit i lies in [0, i] and digit 0
   * is always zero. Little-endian by radix, which is what makes the carry
   * rule local. */
  for (int i = 1; i < CANOR_NUMBER_DIGITS; ++i) {
    uint64_t radix = (uint64_t)(i + 1);
    out->d[i].value = v % radix;
    v /= radix;
  }
  if (v != 0) return CANOR_ERR_RANGE;    /* did not fit in CANOR_NUMBER_DIGITS */
  return CANOR_OK;
}

canor_status canor_number_to_u64(const canor_number *n, uint64_t *out) {
  if (!n || !out) return CANOR_ERR_ARGS;
  if (!canor_number_valid(n)) return CANOR_ERR_RANGE;
  uint64_t acc = 0, place = 1;
  for (int i = 1; i < CANOR_NUMBER_DIGITS; ++i) {
    place *= (uint64_t)i;              /* place value of digit i is i! */
    acc += n->d[i].value * place;
  }
  *out = acc;
  return CANOR_OK;
}

bool canor_number_valid(const canor_number *n) {
  if (!n) return false;
  if (n->n != CANOR_NUMBER_DIGITS) return false;
  /* The invariant an arithmetic bug breaks first: digit i <= i. */
  for (int i = 0; i < CANOR_NUMBER_DIGITS; ++i)
    if (n->d[i].value > (uint64_t)i) return false;
  return true;
}

canor_status canor_int_make(const canor_number *lo, const canor_number *hi, canor_int *out) {
  if (!lo || !hi || !out) return CANOR_ERR_ARGS;
  out->lo = *lo;
  out->hi = *hi;
  return CANOR_OK;
}

/* --- LEHMER: the n!-level topology index ---------------------------------- */

void canor_lehmer_zero(canor_lehmer *l, int levels) {
  if (!l) return;
  memset(l, 0, sizeof *l);
  if (levels < CANOR_LEVELS_MIN) levels = CANOR_LEVELS_MIN;
  if (levels > CANOR_LEVELS_MAX) levels = CANOR_LEVELS_MAX;
  l->levels = (uint8_t)levels;
}

bool canor_lehmer_valid(const canor_lehmer *l) {
  if (!l) return false;
  if (l->levels < CANOR_LEVELS_MIN || l->levels > CANOR_LEVELS_MAX) return false;
  /* Digit i in [0, i]. Digit 0 is therefore always 0, and a code that breaks
   * this is not an address in the topology - it is a corrupted one. */
  for (int i = 0; i < l->levels; ++i)
    if (l->d[i] > (uint8_t)i) return false;
  return true;
}

int canor_lehmer_cmp(const canor_lehmer *a, const canor_lehmer *b) {
  if (a->levels != b->levels) return a->levels < b->levels ? -1 : 1;
  /* Most significant digit is the LAST one: digit i carries place value i!, so
   * comparison walks downward. */
  for (int i = a->levels - 1; i >= 0; --i)
    if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
  return 0;
}

canor_status canor_lehmer_inc(canor_lehmer *l) {
  if (!canor_lehmer_valid(l)) return CANOR_ERR_ARGS;
  /* Odometer in the factorial number system: digit i wraps at i+1. */
  for (int i = 1; i < l->levels; ++i) {
    if (l->d[i] < (uint8_t)i) { ++l->d[i]; return CANOR_OK; }
    l->d[i] = 0;
  }
  /* Past the last address. Reported rather than wrapped to zero: two distinct
   * traversals arriving at the same address is the one thing an index must
   * never do quietly. */
  return CANOR_ERR_RANGE;
}

canor_status canor_lehmer_from_u64(uint64_t v, int levels, canor_lehmer *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (levels < CANOR_LEVELS_MIN || levels > CANOR_LEVELS_MAX) return CANOR_ERR_RANGE;
  canor_lehmer_zero(out, levels);
  for (int i = 1; i < levels; ++i) {
    uint64_t radix = (uint64_t)(i + 1);
    out->d[i] = (uint8_t)(v % radix);
    v /= radix;
  }
  if (v != 0) return CANOR_ERR_RANGE;
  return CANOR_OK;
}

canor_status canor_lehmer_to_u64(const canor_lehmer *l, uint64_t *out) {
  if (!canor_lehmer_valid(l) || !out) return CANOR_ERR_ARGS;
  /* 20! is 2.4e18 and fits a uint64; 21! does not. Refused rather than
   * wrapped - a wrap would alias two topology addresses onto one, and at 96
   * levels almost every address is above this bound anyway. The digit array is
   * the canonical form; this is a convenience for the small end. */
  if (l->levels > 21) return CANOR_ERR_RANGE;
  uint64_t acc = 0, place = 1;
  for (int i = 1; i < l->levels; ++i) {
    place *= (uint64_t)i;               /* place value of digit i is i! */
    acc += (uint64_t)l->d[i] * place;
  }
  *out = acc;
  return CANOR_OK;
}

canor_status canor_lehmer_to_permutation(const canor_lehmer *l, uint8_t *perm, int max) {
  if (!canor_lehmer_valid(l) || !perm) return CANOR_ERR_ARGS;
  int n = l->levels;
  if (max < n) return CANOR_ERR_RANGE;
  /* Standard Lehmer decode: digit i, read from the most significant end,
   * selects and removes the d-th remaining element. This is what makes an
   * index a permutation, and therefore what connects NUMBER to BASE's
   * permutation group. */
  uint8_t pool[CANOR_LEVELS_MAX];
  for (int i = 0; i < n; ++i) pool[i] = (uint8_t)i;
  int remaining = n;
  for (int i = 0; i < n; ++i) {
    int digit = l->d[n - 1 - i];
    if (digit >= remaining) return CANOR_ERR_RANGE;
    perm[i] = pool[digit];
    for (int k = digit; k < remaining - 1; ++k) pool[k] = pool[k + 1];
    --remaining;
  }
  return CANOR_OK;
}

canor_status canor_lehmer_from_permutation(const uint8_t *perm, int levels, canor_lehmer *out) {
  if (!perm || !out) return CANOR_ERR_ARGS;
  if (levels < CANOR_LEVELS_MIN || levels > CANOR_LEVELS_MAX) return CANOR_ERR_RANGE;
  /* Refuse anything that is not a permutation of 0..levels-1. A repeated or
   * out-of-range element would encode to a code that decodes to something
   * else, which is worse than an error. */
  int seen[CANOR_LEVELS_MAX];
  memset(seen, 0, sizeof seen);
  for (int i = 0; i < levels; ++i) {
    if (perm[i] >= levels || seen[perm[i]]) return CANOR_ERR_RANGE;
    seen[perm[i]] = 1;
  }
  canor_lehmer_zero(out, levels);
  uint8_t pool[CANOR_LEVELS_MAX];
  for (int i = 0; i < levels; ++i) pool[i] = (uint8_t)i;
  int remaining = levels;
  for (int i = 0; i < levels; ++i) {
    int digit = 0;
    while (pool[digit] != perm[i]) ++digit;
    out->d[levels - 1 - i] = (uint8_t)digit;
    for (int k = digit; k < remaining - 1; ++k) pool[k] = pool[k + 1];
    --remaining;
  }
  return CANOR_OK;
}

/* --- terms ---------------------------------------------------------------- */

static canor_status term_slot(const canor_term *t, int which, canor_payload *out) {
  switch (t->kind) {
    case CANOR_T_VALUE:
      *out = which == 0 ? t->pre : (which == 1 ? t->peri : t->post);
      return CANOR_OK;
    case CANOR_T_REFERENCE: {
      const canor_payload *p = which == 0 ? t->ref_pre : (which == 1 ? t->ref_peri : t->ref_post);
      if (!p) return CANOR_ERR_ARGS;
      *out = *p;
      return CANOR_OK;
    }
    case CANOR_T_FUNCTIONAL: {
      canor_status (*f)(canor_payload *) =
          which == 0 ? t->fn_pre : (which == 1 ? t->fn_peri : t->fn_post);
      if (!f) return CANOR_ERR_ARGS;
      return f(out);
    }
  }
  return CANOR_ERR_ARGS;
}

static uint64_t payload_scalar(const canor_payload *p) {
  switch (p->kind) {
    case CANOR_P_INT: { uint64_t v = 0; canor_number_to_u64(&p->u.as_int.lo, &v); return v; }
    case CANOR_P_NUMBER: { uint64_t v = 0; canor_number_to_u64(&p->u.as_number, &v); return v; }
    case CANOR_P_STRING: {
      uint64_t h = 1469598103934665603ull;
      for (const char *c = p->u.as_string; *c; ++c) { h ^= (unsigned char)*c; h *= 1099511628211ull; }
      return h;
    }
  }
  return 0;
}

static canor_status default_eval(const canor_term *t, canor_int *out) {
  /* TOKEN's eval returns an INT, which is a {lo, hi} pair. The triple's
   * outermost slots are the natural bounds: pre is the earliest commitment and
   * post the settled one, so the pair is (pre, post) with peri carried in the
   * ordering check rather than in the result. */
  canor_payload a, c;
  canor_status s = term_slot(t, 0, &a);
  if (s != CANOR_OK) return s;
  s = term_slot(t, 2, &c);
  if (s != CANOR_OK) return s;
  canor_number lo, hi;
  if (canor_number_from_u64(payload_scalar(&a) % 479001600ull, &lo) != CANOR_OK) return CANOR_ERR_RANGE;
  if (canor_number_from_u64(payload_scalar(&c) % 479001600ull, &hi) != CANOR_OK) return CANOR_ERR_RANGE;
  return canor_int_make(&lo, &hi, out);
}

void canor_term_init_value(canor_term *t, const canor_payload *pre, const canor_payload *peri,
                        const canor_payload *post) {
  memset(t, 0, sizeof *t);
  t->kind = CANOR_T_VALUE;
  if (pre) t->pre = *pre;
  if (peri) t->peri = *peri;
  if (post) t->post = *post;
  t->eval = default_eval;
}

void canor_term_init_reference(canor_term *t, const canor_payload *pre, const canor_payload *peri,
                            const canor_payload *post) {
  memset(t, 0, sizeof *t);
  t->kind = CANOR_T_REFERENCE;
  t->ref_pre = pre; t->ref_peri = peri; t->ref_post = post;
  t->eval = default_eval;
}

void canor_term_init_functional(canor_term *t,
                             canor_status (*pre)(canor_payload *),
                             canor_status (*peri)(canor_payload *),
                             canor_status (*post)(canor_payload *)) {
  memset(t, 0, sizeof *t);
  t->kind = CANOR_T_FUNCTIONAL;
  t->fn_pre = pre; t->fn_peri = peri; t->fn_post = post;
  t->eval = default_eval;
}

canor_status canor_term_eval(const canor_term *t, canor_int *out) {
  if (!t || !out) return CANOR_ERR_ARGS;
  if (!t->eval) return CANOR_ERR_ARGS;
  return t->eval(t, out);
}

bool canor_term_ordered(const canor_term *t) {
  canor_payload a, b, c;
  if (!t) return false;
  if (term_slot(t, 0, &a) != CANOR_OK) return false;
  if (term_slot(t, 1, &b) != CANOR_OK) return false;
  if (term_slot(t, 2, &c) != CANOR_OK) return false;
  uint64_t x = payload_scalar(&a), y = payload_scalar(&b), z = payload_scalar(&c);
  return x <= y && y <= z;
}

bool canor_observable_is_expr(const canor_observable *o) {
  if (!o) return false;
  const canor_term *ts[3] = { &o->pre, &o->peri, &o->post };
  for (int i = 0; i < 3; ++i) {
    if (ts[i]->kind != CANOR_T_VALUE) return false;
    if (ts[i]->pre.kind != CANOR_P_STRING) return false;
    if (ts[i]->peri.kind != CANOR_P_STRING) return false;
    if (ts[i]->post.kind != CANOR_P_STRING) return false;
  }
  return true;
}

canor_status canor_operator_eval(const canor_operator *o, canor_observable *out) {
  if (!o || !out) return CANOR_ERR_ARGS;
  memset(out, 0, sizeof *out);
  /* "Semantics similar to {LHS : OBSERVABLE::pre, OP : OBSERVABLE::peri,
   * RHS : OBSERVABLE::post}" - so the operator itself occupies the peri slot.
   * That is what "strict infix-only" buys: the shape of the result records
   * where the operator sat. */
  out->pre = o->lhs;
  out->post = o->rhs;
  canor_payload op;
  memset(&op, 0, sizeof op);
  op.kind = CANOR_P_STRING;
  static const char *const names[4] = { "+", "-", "*", "/" };
  snprintf(op.u.as_string, sizeof op.u.as_string, "%s", names[o->op]);
  canor_term_init_value(&out->peri, &op, &op, &op);
  return CANOR_OK;
}

/* --- DATA / REAL / INTERVAL ----------------------------------------------- */

bool canor_triple_ordered(const canor_triple *t) {
  if (!t) return false;
  return canor_rat_cmp(t->min, t->nom) <= 0 && canor_rat_cmp(t->nom, t->max) <= 0;
}

bool canor_data_ordered(const canor_data *d) {
  if (!d) return false;
  return canor_triple_ordered(&d->low) && canor_triple_ordered(&d->value) &&
         canor_triple_ordered(&d->high);
}

bool canor_interval_ordered(const canor_interval *i) {
  return i && canor_rat_cmp(i->lo, i->hi) <= 0;
}

/* --- Kapprox (FLOWALGEBRA 30) ---------------------------------------------- */

const char *canor_kclass_name(canor_kclass c) {
  switch (c) {
    case CANOR_K_GENERAL: return "General";
    case CANOR_K_SPECIFIC: return "Specific";
    case CANOR_K_DISTINCTIVE: return "Distinctive";
    case CANOR_K_ARBITRARY: return "Arbitrary";
  }
  return "?";
}

int64_t canor_log2_milli(uint64_t x) {
  if (x == 0) return 0;
  int64_t hi = 0;
  uint64_t v = x;
  while (v >>= 1) ++hi;
  int64_t result = hi * 1000;
  if (x == (1ull << hi)) return result;      /* exact power of two */

  /* Fractional part by repeated squaring in Q32 - the standard fixed-point
   * log2. Integer throughout, so two hosts agree bit for bit, which is the
   * whole reason this is not a call to log2(). */
  uint64_t q = (hi <= 32) ? (x << (32 - hi)) : (x >> (hi - 32));
  int64_t frac = 0, weight = 500;
  for (int i = 0; i < 12 && weight > 0; ++i) {
    /* Square a Q32 value into Q32 without a 128-bit type. */
    uint64_t lo = q & 0xFFFFFFFFull, up = q >> 32;
    uint64_t sq = up * up * (1ull << 32) + 2ull * up * lo + (lo * lo >> 32);
    q = sq;
    if (q >= (2ull << 32)) { q >>= 1; frac += weight; }
    weight /= 2;
  }
  return result + frac;
}

int64_t canor_log2_factorial_milli(uint64_t n) {
  if (n < 2) return 0;
  /* Exact below the crossover, Stirling above. The crossover is DECLARED here
   * rather than tuned silently: past 100000 the summation costs more than the
   * accuracy is worth to any caller of a complexity estimate. */
  if (n <= 100000ull) {
    int64_t acc = 0;
    for (uint64_t i = 2; i <= n; ++i) acc += canor_log2_milli(i);
    return acc;
  }
  /* log2(n!) ~ n*log2(n) - n*log2(e) + 0.5*log2(2*pi*n).
   * log2(e) = 1.4427, taken as 1443 milli so the estimate errs low rather
   * than high - a complexity bound that overstates is the worse mistake. */
  int64_t ln = canor_log2_milli(n);
  int64_t term = (int64_t)n * ln - (int64_t)n * 1443;
  int64_t corr = canor_log2_milli(6283ull * n / 1000ull) / 2;
  return term + corr;
}

int64_t canor_kapprox_milli(canor_kclass c, uint64_t n) {
  switch (c) {
    case CANOR_K_GENERAL:
      return canor_log2_milli(2 * n + 1);
    case CANOR_K_SPECIFIC:
      /* log2(2*(n!)+1) ~ 1 + log2(n!); the +1 is far below the resolution. */
      return 1000 + canor_log2_factorial_milli(n);
    case CANOR_K_DISTINCTIVE:
      return canor_log2_factorial_milli(2 * n + 1);
    case CANOR_K_ARBITRARY: {
      /* (2(n!)+1)!  The argument m = 2(n!)+1 overflows uint64 past n=20, so
       * m is never materialized. What is returned is the ORDER of the answer -
       * log2(log2(m!)) - because log2(m!) itself is around 1e152 at n=96 and
       * exceeds anything a caller can do arithmetic with. Reported as an order
       * so the class remains comparable without pretending to a precision it
       * does not have. */
      int64_t log2m = 1000 + canor_log2_factorial_milli(n);
      return log2m + canor_log2_milli((uint64_t)(log2m / 1000 + 1));
    }
  }
  return 0;
}

int64_t canor_kapprox_order_milli(canor_kclass c, uint64_t n) {
  /* ONE SCALE FOR ALL FOUR: log2(log2(value)), in milli.
   *
   * The first three classes fit in log2 bits; the fourth does not, because
   * log2((2(n!)+1)!) is itself around 1e152 at n=96. Reporting three on one
   * scale and the fourth on another made the ladder appear to INVERT -
   * Arbitrary came out below Distinctive - which is a units error and not a
   * property of the ladder. The order scale is the one that stays valid for
   * every class at every level, so comparisons use it. */
  if (c == CANOR_K_ARBITRARY) {
    /* Already an order by construction: log2(m) + log2(log2(m)) where the
     * value is m! and m is never materialized. */
    return canor_kapprox_milli(c, n);
  }
  int64_t bits = canor_kapprox_milli(c, n);
  uint64_t whole = (uint64_t)(bits / 1000);
  if (whole < 1) whole = 1;
  return canor_log2_milli(whole);
}

bool canor_kapprox_separated(uint64_t n) {
  int64_t a = canor_kapprox_order_milli(CANOR_K_GENERAL, n);
  int64_t b = canor_kapprox_order_milli(CANOR_K_SPECIFIC, n);
  int64_t c = canor_kapprox_order_milli(CANOR_K_DISTINCTIVE, n);
  int64_t d = canor_kapprox_order_milli(CANOR_K_ARBITRARY, n);
  return a < b && b < c && c < d;
}

/* --- Cayley-Dickson ------------------------------------------------------- */

static int g_imag_ready = 0;
static int8_t g_mul_sign[CANOR_IMAG_COUNT][CANOR_IMAG_COUNT];
static uint8_t g_mul_index[CANOR_IMAG_COUNT][CANOR_IMAG_COUNT];

/* e_i * e_j in the algebra of dimension 2^n, by Cayley-Dickson doubling
 * (a,b)(c,d) = (ac - conj(d) b, d a + b conj(c)).
 *
 * Computed rather than tabulated. A hand-written 16x16 signed table is 256
 * entries that no reviewer can check by eye; this is the construction itself,
 * and the suite then verifies what follows from it - i*j=k with j*i=-k at
 * dimension 4, non-associativity at 8, zero divisors at 16. */
static void cd_mul(int n, int i, int j, int *idx, int *sgn) {
  if (n == 0) { *idx = 0; *sgn = 1; return; }
  int h = 1 << (n - 1);
  int k, s;
  if (i < h && j < h) {
    cd_mul(n - 1, i, j, idx, sgn);
    return;
  }
  if (i < h && j >= h) {                 /* (a,0)(0,d) = (0, d a) */
    cd_mul(n - 1, j - h, i, &k, &s);
    *idx = k + h; *sgn = s;
    return;
  }
  if (i >= h && j < h) {                 /* (0,b)(c,0) = (0, b conj(c)) */
    int cs = (j == 0) ? 1 : -1;
    cd_mul(n - 1, i - h, j, &k, &s);
    *idx = k + h; *sgn = s * cs;
    return;
  }
  {                                      /* (0,b)(0,d) = (-conj(d) b, 0) */
    int ds = (j - h == 0) ? 1 : -1;
    cd_mul(n - 1, j - h, i - h, &k, &s);
    *idx = k; *sgn = -(s * ds);
  }
}

canor_status canor_imag_init(void) {
  /* Closure is a property of the DIMENSION, and only 1, 2, 4, 8 and 16 have
   * it. CANOR_IMAG_COUNT is 1 + 3*trifectas, so four trifectas gives 13 - which
   * is not a power of two and admits no consistent product. Refused here
   * rather than producing a table that is not a multiplication. */
  int n = 0, d = CANOR_IMAG_COUNT;
  while (d > 1) {
    if (d & 1) return CANOR_ERR_NOT_CLOSED;
    d >>= 1; ++n;
  }
  if (n > 4) return CANOR_ERR_NOT_CLOSED;   /* beyond sedenions the doubling
                                          * continues but the document's
                                          * trifecta framing does not */
  for (int i = 0; i < CANOR_IMAG_COUNT; ++i)
    for (int j = 0; j < CANOR_IMAG_COUNT; ++j) {
      int idx, sgn;
      cd_mul(n, i, j, &idx, &sgn);
      g_mul_index[i][j] = (uint8_t)idx;
      g_mul_sign[i][j] = (int8_t)sgn;
    }
  g_imag_ready = 1;
  return CANOR_OK;
}

canor_status canor_imag_mul(canor_imag a, canor_imag b, canor_imag *index, int *sign) {
  if (!g_imag_ready) return CANOR_ERR_NOT_CLOSED;
  if (a >= CANOR_IMAG_COUNT || b >= CANOR_IMAG_COUNT || !index || !sign) return CANOR_ERR_ARGS;
  *index = g_mul_index[a][b];
  *sign = g_mul_sign[a][b];
  return CANOR_OK;
}

int canor_imag_conj_sign(canor_imag a) { return a == 0 ? 1 : -1; }

const char *canor_imag_name(canor_imag a) {
  /* PINNED IMAG basis (SCHEMA.md, MEMORY.md 2026-10-20):
   *   [one=0, (i,j,k)=1..3, (l,m,n)=4..6, (o,p,q)=7..9, (r,s,t)=10..12,
   *    (e,f,g)=13..15]
   * Only the names are pinned here; the product is still Cayley-Dickson
   * doubling over the indices, so renaming cannot change any table entry. */
  static const char *const n[16] = {
    "one", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "e", "f", "g"
  };
  return a < 16 ? n[a] : "?";
}

void canor_base_init(canor_base *b) {
  memset(b, 0, sizeof *b);
  b->head = b->tail = -1;
}

canor_status canor_base_push(canor_base *b, canor_imaginary v) {
  if (!b) return CANOR_ERR_ARGS;
  if (v.imag >= CANOR_IMAG_COUNT) return CANOR_ERR_RANGE;
  if (b->count >= CANOR_BASE_MAX) return CANOR_ERR_FULL;
  int i = b->count++;
  b->node[i] = v;
  b->prev[i] = b->tail;
  b->next[i] = -1;
  if (b->tail >= 0) b->next[b->tail] = i;
  b->tail = i;
  if (b->head < 0) b->head = i;
  return CANOR_OK;
}

canor_status canor_base_eval(const canor_base *b, canor_imaginary *out, int *sign) {
  if (!b || !out || !sign) return CANOR_ERR_ARGS;
  if (!g_imag_ready) return CANOR_ERR_NOT_CLOSED;
  if (b->head < 0) { out->imag = 0; out->ring = 0; *sign = 1; return CANOR_OK; }
  /* Folded in LIST ORDER, left to right. The product is non-commutative from
   * dimension 4 and non-associative from 8, so the order is part of the
   * contract - a different traversal is a different answer, not a faster one. */
  canor_imag acc = b->node[b->head].imag;
  size_t ring = b->node[b->head].ring;
  int s = 1;
  for (int i = b->next[b->head]; i >= 0; i = b->next[i]) {
    canor_imag r;
    int rs;
    canor_status st = canor_imag_mul(acc, b->node[i].imag, &r, &rs);
    if (st != CANOR_OK) return st;
    acc = r;
    s *= rs;
    ring += b->node[i].ring;
  }
  out->imag = acc;
  out->ring = ring;
  *sign = s;
  return CANOR_OK;
}

bool canor_base_is_permutation(canor_imag by) {
  if (!g_imag_ready || by >= CANOR_IMAG_COUNT) return false;
  /* Left multiplication by a fixed unit maps the units onto the units,
   * bijectively - the permutation-group property the document asserts of BASE.
   * Checked by counting images rather than assumed. */
  int seen[CANOR_IMAG_COUNT];
  memset(seen, 0, sizeof seen);
  for (int i = 0; i < CANOR_IMAG_COUNT; ++i) {
    canor_imag r;
    int s;
    if (canor_imag_mul(by, (canor_imag)i, &r, &s) != CANOR_OK) return false;
    if (r >= CANOR_IMAG_COUNT || seen[r]) return false;
    seen[r] = 1;
  }
  return true;
}

/* --- TENSORIAL / TENSOR --------------------------------------------------- */

void canor_tensorial_init(canor_tensorial *t, size_t group_id) {
  memset(t, 0, sizeof *t);
  t->group_id = group_id;
  t->regular = true;
  t->unknownsymmetric = true;   /* until declared otherwise */
}

canor_status canor_tensorial_push(canor_tensorial *t, const canor_modulation *m) {
  if (!t || !m) return CANOR_ERR_ARGS;
  if (t->count >= CANOR_TENSORIAL_MAX) return CANOR_ERR_FULL;
  t->data[t->count++] = *m;
  return CANOR_OK;
}

bool canor_tensorial_flags_consistent(const canor_tensorial *t) {
  if (!t) return false;
  if (t->symmetric && t->antisymmetric) return false;
  if (t->unknownsymmetric && (t->symmetric || t->antisymmetric)) return false;
  return true;
}

void canor_tensor_init(canor_tensor *t) { memset(t, 0, sizeof *t); }

canor_status canor_tensor_add_covariant(canor_tensor *t, const canor_tensorial *v) {
  if (!t || !v) return CANOR_ERR_ARGS;
  if (t->n_cov >= CANOR_TENSOR_RANK_MAX) return CANOR_ERR_FULL;
  t->covariant[t->n_cov++] = *v;
  return CANOR_OK;
}

canor_status canor_tensor_add_contravariant(canor_tensor *t, const canor_tensorial *v) {
  if (!t || !v) return CANOR_ERR_ARGS;
  if (t->n_con >= CANOR_TENSOR_RANK_MAX) return CANOR_ERR_FULL;
  t->contravariant[t->n_con++] = *v;
  return CANOR_OK;
}

void canor_tensor_rank(const canor_tensor *t, int *contra, int *cov) {
  if (contra) *contra = t ? t->n_con : 0;
  if (cov) *cov = t ? t->n_cov : 0;
}

canor_status canor_tensor_eval(const canor_tensor *t, canor_modulation *out) {
  if (!t || !out) return CANOR_ERR_ARGS;
  memset(out, 0, sizeof *out);
  canor_base_init(&out->base);
  /* CALCULUS's eval "uses the dataset covariant/contravariant for
   * tensor-calculus, and results in a MODULATION". The contraction implemented
   * here is the one the types actually support: sum the reals, take the widest
   * bounds, and fold the bases in declared order (contravariant then
   * covariant). A fuller contraction needs an index notation the document does
   * not give, so this is stated as what it is rather than named `contract`. */
  canor_rational acc = canor_rat(0, 1);
  canor_rational lo = canor_rat(0, 1), hi = canor_rat(0, 1);
  bool first = true;
  const canor_tensorial *groups[2 * CANOR_TENSOR_RANK_MAX];
  int n_groups = 0;
  for (int i = 0; i < t->n_con; ++i) groups[n_groups++] = &t->contravariant[i];
  for (int i = 0; i < t->n_cov; ++i) groups[n_groups++] = &t->covariant[i];

  for (int g = 0; g < n_groups; ++g) {
    if (!canor_tensorial_flags_consistent(groups[g])) return CANOR_ERR_ARGS;
    for (int m = 0; m < groups[g]->count; ++m) {
      const canor_modulation *mod = &groups[g]->data[m];
      acc = canor_rat_add(acc, mod->real.val);
      if (first) { lo = mod->bounds.lo; hi = mod->bounds.hi; first = false; }
      else {
        if (canor_rat_cmp(mod->bounds.lo, lo) < 0) lo = mod->bounds.lo;
        if (canor_rat_cmp(mod->bounds.hi, hi) > 0) hi = mod->bounds.hi;
      }
      for (int k = mod->base.head; k >= 0; k = mod->base.next[k])
        if (canor_base_push(&out->base, mod->base.node[k]) != CANOR_OK) return CANOR_ERR_FULL;
    }
  }
  out->real.val = acc;
  out->bounds.lo = lo;
  out->bounds.hi = hi;
  return CANOR_OK;
}

/* --- UNIT / EXPRESSION ---------------------------------------------------- */

void canor_unit_init(canor_unit *u, canor_termkind head) {
  memset(u, 0, sizeof *u);
  u->head = head;
}

canor_status canor_unit_add_real(canor_unit *u, canor_real r) {
  if (!u) return CANOR_ERR_ARGS;
  if (u->n_reals >= CANOR_UNIT_MAX) return CANOR_ERR_FULL;
  u->reals[u->n_reals++] = r;
  return CANOR_OK;
}

canor_status canor_unit_add_interval(canor_unit *u, canor_interval i) {
  if (!u) return CANOR_ERR_ARGS;
  if (!canor_interval_ordered(&i)) return CANOR_ERR_RANGE;
  if (u->n_intervals >= CANOR_UNIT_MAX) return CANOR_ERR_FULL;
  u->intervals[u->n_intervals++] = i;
  return CANOR_OK;
}

canor_status canor_unit_add_tensor(canor_unit *u, const canor_tensor *t) {
  if (!u || !t) return CANOR_ERR_ARGS;
  if (u->n_base >= CANOR_UNIT_MAX) return CANOR_ERR_FULL;
  u->base[u->n_base++] = *t;
  return CANOR_OK;
}

bool canor_unit_is_atom(const canor_unit *u)   { return u && u->head == CANOR_T_VALUE; }
bool canor_unit_is_change(const canor_unit *u) { return u && u->head == CANOR_T_REFERENCE; }
bool canor_unit_is_force(const canor_unit *u)  { return u && u->head == CANOR_T_FUNCTIONAL; }

void canor_expression_init(canor_expression *e) { memset(e, 0, sizeof *e); }

canor_status canor_expression_add(canor_expression *e, const canor_unit *u) {
  if (!e || !u) return CANOR_ERR_ARGS;
  if (e->n_output >= CANOR_UNIT_MAX) return CANOR_ERR_FULL;
  e->output[e->n_output++] = *u;
  return CANOR_OK;
}

/* --- BBST ----------------------------------------------------------------- */

void canor_bbst_init(canor_bbst *t) {
  memset(t, 0, sizeof *t);
  t->root = -1;
}

canor_status canor_bbst_insert(canor_bbst *t, const int64_t key[CANOR_KD], int expr) {
  if (!t || !key) return CANOR_ERR_ARGS;
  if (t->count >= CANOR_BBST_MAX) return CANOR_ERR_FULL;
  canor_bbst_node *n = &t->node[t->count];
  memset(n, 0, sizeof *n);
  for (int i = 0; i < CANOR_KD; ++i) n->key[i] = key[i];
  n->expr = expr;
  n->left = n->right = -1;
  ++t->count;
  return CANOR_OK;
}

int canor_bbst_expr(const canor_bbst *t, int node) {
  if (!t || node < 0 || node >= t->count) return -1;
  return t->node[node].expr;
}

static int kd_cmp(const int64_t *a, const int64_t *b, int axis) {
  if (a[axis] != b[axis]) return a[axis] < b[axis] ? -1 : 1;
  /* Ties broken on the remaining axes in fixed order, so the sort is total and
   * two identical input sets build identical trees. Without this the median
   * split would depend on the incoming order. */
  for (int i = 0; i < CANOR_KD; ++i)
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  return 0;
}

static int kd_build(canor_bbst *t, int *idx, int lo, int hi, int depth) {
  if (lo >= hi) return -1;
  int axis = depth % CANOR_KD;
  /* Insertion sort on the slice: n is bounded by CANOR_BBST_MAX and the constant
   * factor beats a recursive selection here. Stable given kd_cmp is total. */
  for (int i = lo + 1; i < hi; ++i) {
    int cur = idx[i], j = i - 1;
    while (j >= lo && kd_cmp(t->node[idx[j]].key, t->node[cur].key, axis) > 0) {
      idx[j + 1] = idx[j];
      --j;
    }
    idx[j + 1] = cur;
  }
  int mid = lo + (hi - lo) / 2;
  int me = idx[mid];
  t->node[me].left = kd_build(t, idx, lo, mid, depth + 1);
  t->node[me].right = kd_build(t, idx, mid + 1, hi, depth + 1);
  return me;
}

canor_status canor_bbst_build(canor_bbst *t) {
  if (!t) return CANOR_ERR_ARGS;
  if (t->count == 0) { t->root = -1; return CANOR_OK; }
  int idx[CANOR_BBST_MAX];
  for (int i = 0; i < t->count; ++i) idx[i] = i;
  t->root = kd_build(t, idx, 0, t->count, 0);
  return CANOR_OK;
}

int canor_bbst_find(const canor_bbst *t, const int64_t key[CANOR_KD]) {
  if (!t || !key) return -1;
  int cur = t->root, depth = 0;
  while (cur >= 0) {
    int c = kd_cmp(key, t->node[cur].key, depth % CANOR_KD);
    if (c == 0) return cur;
    cur = c < 0 ? t->node[cur].left : t->node[cur].right;
    ++depth;
  }
  return -1;
}

static int kd_depth(const canor_bbst *t, int n) {
  if (n < 0) return 0;
  int l = kd_depth(t, t->node[n].left);
  int r = kd_depth(t, t->node[n].right);
  return 1 + (l > r ? l : r);
}

int canor_bbst_depth(const canor_bbst *t) { return t ? kd_depth(t, t->root) : 0; }

/* --- TST ------------------------------------------------------------------ */

canor_rational canor_origo_interval2(const canor_origo *o) {
  /* Minkowski, signature (-,+,+,+). Exact, so the classification below is
   * decided rather than approximated - which matters because the null case is
   * an equality and a float would almost never hit it. */
  canor_rational t2 = canor_rat_mul(o->t, o->t);
  canor_rational x2 = canor_rat_mul(o->x, o->x);
  canor_rational y2 = canor_rat_mul(o->y, o->y);
  canor_rational z2 = canor_rat_mul(o->z, o->z);
  canor_rational s = canor_rat_add(x2, canor_rat_add(y2, z2));
  return canor_rat_add(s, canor_rat(-t2.num, t2.den));
}

canor_interval_class canor_origo_classify(const canor_origo *o, bool provenance_ok) {
  if (!o) return CANOR_INTERVAL_INDETERMINATE;
  /* WORLD_MODEL section 8.4's fourth class comes FIRST: a delta whose clock,
   * spatial or provenance quality is too poor to classify is indeterminate,
   * and the system is required to represent that rather than guess a class
   * from numbers it does not trust. */
  if (!provenance_ok) return CANOR_INTERVAL_INDETERMINATE;
  canor_rational s2 = canor_origo_interval2(o);
  if (s2.num == 0) return CANOR_INTERVAL_NULL;
  return s2.num < 0 ? CANOR_INTERVAL_TIMELIKE : CANOR_INTERVAL_SPACELIKE;
}

void canor_tst_init(canor_tst *t) {
  memset(t, 0, sizeof *t);
  t->root = -1;
}

int canor_tst_add(canor_tst *t, const canor_origo *origo) {
  if (!t || t->count >= CANOR_BBST_MAX) return -1;
  int i = t->count++;
  canor_tst_node *n = &t->node[i];
  memset(n, 0, sizeof *n);
  if (origo) n->origo = *origo;
  for (int k = 0; k < 3; ++k) canor_bbst_init(&n->leaf[k]);
  n->left = n->middle = n->right = -1;
  /* No varying OrigoDelta by default: the node's static origo is the whole
   * boundary until a caller attaches expressions. */
  for (int k = 0; k < 4; ++k) n->origo_expr[k] = -1;
  n->used = true;
  if (t->root < 0) t->root = i;
  return i;
}

canor_status canor_tst_link(canor_tst *t, int parent, int which, int child) {
  if (!t || parent < 0 || parent >= t->count) return CANOR_ERR_ARGS;
  if (child < 0 || child >= t->count) return CANOR_ERR_ARGS;
  if (which < 0 || which > 2) return CANOR_ERR_RANGE;
  if (which == 0) t->node[parent].left = child;
  else if (which == 1) t->node[parent].middle = child;
  else t->node[parent].right = child;
  return CANOR_OK;
}

static bool tst_path(const canor_tst *t, int from, int target, int *path, int *n, int depth) {
  if (from < 0 || depth > CANOR_BBST_MAX) return false;
  path[(*n)++] = from;
  if (from == target) return true;
  const canor_tst_node *k = &t->node[from];
  int kids[3] = { k->left, k->middle, k->right };
  for (int i = 0; i < 3; ++i) {
    if (kids[i] < 0) continue;
    if (tst_path(t, kids[i], target, path, n, depth + 1)) return true;
  }
  --(*n);
  return false;
}

canor_status canor_tst_absolute_origo(const canor_tst *t, int node, canor_origo *out) {
  if (!t || !out || node < 0 || node >= t->count) return CANOR_ERR_ARGS;
  int path[CANOR_BBST_MAX];
  int n = 0;
  if (!tst_path(t, t->root, node, path, &n, 0)) return CANOR_ERR_ARGS;
  /* ORIGO is a delta "relative to the Gravity Well's/Parental Node's ORIGO",
   * so absolute position is the fold along the path from the root. Addition of
   * deltas, not composition: the document calls it an INTERVAL delta. */
  memset(out, 0, sizeof *out);
  out->t = canor_rat(0, 1); out->x = canor_rat(0, 1);
  out->y = canor_rat(0, 1); out->z = canor_rat(0, 1);
  for (int i = 0; i < n; ++i) {
    const canor_origo *o = &t->node[path[i]].origo;
    out->t = canor_rat_add(out->t, o->t);
    out->x = canor_rat_add(out->x, o->x);
    out->y = canor_rat_add(out->y, o->y);
    out->z = canor_rat_add(out->z, o->z);
  }
  return CANOR_OK;
}

canor_status canor_tst_iterate(const canor_tst *t, int node, canor_iteration *out) {
  if (!t || !out || node < 0 || node >= t->count) return CANOR_ERR_ARGS;
  memset(out, 0, sizeof *out);
  const canor_tst_node *n = &t->node[node];
  /* Strictly ordered, and each stage requires the previous one: PRE gives the
   * Lorentzian frame, PERI adds topology to that frame, POST adds temporal
   * structure to that object. A POST leaf with no PERI before it is not a
   * partial result, it is a malformed one - so the flags cascade rather than
   * being set independently. */
  out->frame = n->leaf[CANOR_LEAF_PRE].count > 0;
  out->volumetric = out->frame && n->leaf[CANOR_LEAF_PERI].count > 0;
  out->animated = out->volumetric && n->leaf[CANOR_LEAF_POST].count > 0;
  out->expressions = n->leaf[0].count + n->leaf[1].count + n->leaf[2].count;
  return CANOR_OK;
}


canor_status canor_tst_set_origo_expr(canor_tst *t, int node, int et, int ex, int ey, int ez) {
  if (!t || node < 0 || node >= t->count || !t->node[node].used) return CANOR_ERR_RANGE;
  t->node[node].origo_expr[0] = et;
  t->node[node].origo_expr[1] = ex;
  t->node[node].origo_expr[2] = ey;
  t->node[node].origo_expr[3] = ez;
  return CANOR_OK;
}

bool canor_tst_has_origo_expr(const canor_tst *t, int node) {
  int k;
  if (!t || node < 0 || node >= t->count || !t->node[node].used) return false;
  for (k = 0; k < 4; ++k) if (t->node[node].origo_expr[k] >= 0) return true;
  return false;
}
