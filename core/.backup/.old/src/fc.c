/* Follow-Core type tower. See include/fc.h.
 *
 * No floating point. BASIS evaluates to a fraction, so the arithmetic here is
 * exact rational arithmetic on int64 pairs; a double would discard exactly the
 * property the design rests on.
 */

#include "fc.h"

#include <stdio.h>
#include <string.h>

const char *fc_strerror(fc_status s) {
  switch (s) {
    case FC_OK: return "ok";
    case FC_ERR_ARGS: return "invalid arguments";
    case FC_ERR_RANGE: return "value out of range";
    case FC_ERR_FULL: return "container full";
    case FC_ERR_NOT_CLOSED: return "imaginary dimension does not close under Cayley-Dickson";
    case FC_ERR_UNDEFINED: return "undefined result";
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
 *   3. FOR COMPARISON, DON'T MULTIPLY AT ALL. fc_rat_cmp uses the continued
 *      fraction expansion, which needs only division and subtraction and
 *      therefore cannot overflow for any pair of valid rationals. Comparison
 *      has no error channel, so making it infallible is worth more than making
 *      it fast.
 *
 * THE POISON. den == 0 now means "not a value", and every operation propagates
 * it. The previous code returned 0/1 for a zero denominator, which is a
 * perfectly good number and therefore the worst possible answer: it turns an
 * arithmetic failure into a silent zero. A caller that ignores fc_rat_valid
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

static bool fc_mul64(int64_t a, int64_t b, int64_t *out) {
  if (a == 0 || b == 0) { *out = 0; return true; }
  if (a == INT64_MIN || b == INT64_MIN) return false;
  {
    int64_t aa = a < 0 ? -a : a, ab = b < 0 ? -b : b;
    if (aa > INT64_MAX / ab) return false;
  }
  *out = a * b;
  return true;
}

static bool fc_add64(int64_t a, int64_t b, int64_t *out) {
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return false;
  *out = a + b;
  return true;
}

static fc_rational fc_rat_poison(void) {
  fc_rational r;
  r.num = 0;
  r.den = 0;
  return r;
}

bool fc_rat_valid(fc_rational a) { return a.den > 0; }

fc_rational fc_rat(int64_t num, int64_t den) {
  fc_rational r;
  int64_t g;
  if (den == 0) return fc_rat_poison();
  /* Neither can be negated into range, and reduction needs the magnitude. */
  if (num == INT64_MIN || den == INT64_MIN) return fc_rat_poison();
  if (den < 0) { num = -num; den = -den; }
  g = gcd64(num, den);
  r.num = num / g;
  r.den = den / g;
  return r;
}

fc_rational fc_rat_add(fc_rational a, fc_rational b) {
  int64_t g, ad, bd, den, l, rr, num;
  if (!fc_rat_valid(a) || !fc_rat_valid(b)) return fc_rat_poison();
  /* Least common denominator, not the product: a.den * b.den overflows on
   * pairs whose LCM comfortably fits, and the two are equal only when the
   * denominators are already coprime. */
  g = gcd64(a.den, b.den);
  ad = a.den / g;
  bd = b.den / g;
  if (!fc_mul64(a.den, bd, &den)) return fc_rat_poison();
  if (!fc_mul64(a.num, bd, &l)) return fc_rat_poison();
  if (!fc_mul64(b.num, ad, &rr)) return fc_rat_poison();
  if (!fc_add64(l, rr, &num)) return fc_rat_poison();
  return fc_rat(num, den);
}

fc_rational fc_rat_mul(fc_rational a, fc_rational b) {
  int64_t g1, g2, num, den;
  if (!fc_rat_valid(a) || !fc_rat_valid(b)) return fc_rat_poison();
  /* Cross-cancel first. (2/3)*(3/2) must not need six-digit intermediates to
   * reach 1, and more to the point an iterated product stays in range far
   * longer when the common factors never enter the multiplication. */
  g1 = gcd64(a.num, b.den);
  g2 = gcd64(b.num, a.den);
  if (a.num == INT64_MIN || b.num == INT64_MIN) return fc_rat_poison();
  if (!fc_mul64(a.num / g1, b.num / g2, &num)) return fc_rat_poison();
  if (!fc_mul64(a.den / g2, b.den / g1, &den)) return fc_rat_poison();
  return fc_rat(num, den);
}

bool fc_rat_eq(fc_rational a, fc_rational b) {
  /* Both are reduced with den > 0, so structural equality is value equality.
   * A poison value is equal to nothing, including another poison value:
   * "these two computations both failed" is not evidence they agree. */
  if (!fc_rat_valid(a) || !fc_rat_valid(b)) return false;
  return a.num == b.num && a.den == b.den;
}

/* Floor division, which C's / does not do for negative operands. */
static int64_t fc_fdiv(int64_t n, int64_t d) {      /* d > 0 */
  int64_t q = n / d;
  if (n % d != 0 && n < 0) --q;
  return q;
}

int fc_rat_cmp(fc_rational a, fc_rational b) {
  int64_t an, ad, bn, bd;
  int flip = 1;
  if (!fc_rat_valid(a) || !fc_rat_valid(b)) return 0;
  an = a.num; ad = a.den; bn = b.num; bd = b.den;
  /* Continued fraction comparison. At each step the integer parts either
   * differ - which decides it - or agree, and the comparison passes to the
   * reciprocals of the remainders, which reverses the order. No multiplication
   * occurs, so no input can overflow this. It terminates because each step is
   * a Euclidean step on (den, remainder), which strictly decreases. */
  for (;;) {
    int64_t aq = fc_fdiv(an, ad), bq = fc_fdiv(bn, bd), ar, br;
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

fc_status fc_basis_make(int64_t v, fc_basis *out) {
  if (!out) return FC_ERR_ARGS;
  if (v == 0) return FC_ERR_RANGE;      /* the document says != 0 */
  out->v = v;
  return FC_OK;
}

fc_status fc_basis_parse(const char *text, fc_basis *out) {
  /* `^[+-][1-9][0-9]*$`, enforced literally: the sign is REQUIRED, the first
   * digit may not be zero, and there is nothing else. Written out rather than
   * handed to strtol, which would accept " 12", "0x1f" and a bare "5". */
  if (!text || !out) return FC_ERR_ARGS;
  const char *p = text;
  int neg = 0;
  if (*p == '+') neg = 0;
  else if (*p == '-') neg = 1;
  else return FC_ERR_ARGS;
  ++p;
  if (*p < '1' || *p > '9') return FC_ERR_ARGS;
  int64_t v = 0;
  for (; *p; ++p) {
    if (*p < '0' || *p > '9') return FC_ERR_ARGS;
    if (v > (INT64_MAX - (*p - '0')) / 10) return FC_ERR_RANGE;
    v = v * 10 + (*p - '0');
  }
  out->v = neg ? -v : v;
  return FC_OK;
}

fc_rational fc_basis_eval(fc_basis b) {
  /* "for BASIS < 0: the absolute fractional 1/|BASIS|, and for BASIS > 0: just
   * the value itself". This is the reason eval returns a rational. */
  if (b.v < 0) return fc_rat(1, -b.v);
  return fc_rat(b.v, 1);
}

/* --- NUM / NUMBER / INT --------------------------------------------------- */

fc_status fc_num_make(fc_sign sign, uint64_t value, int64_t basis, int64_t exponent,
                      fc_num *out) {
  if (!out) return FC_ERR_ARGS;
  if (sign != FC_NEG_ZERO && sign != FC_NEG_ONE &&
      sign != FC_POS_ONE && sign != FC_POS_ZERO) return FC_ERR_RANGE;
  fc_basis b, e;
  if (fc_basis_make(basis, &b) != FC_OK) return FC_ERR_RANGE;
  if (fc_basis_make(exponent, &e) != FC_OK) return FC_ERR_RANGE;
  out->sign = sign;
  out->value = value;
  out->basis = b;
  out->exponent = e;
  return FC_OK;
}

fc_status fc_num_eval(const fc_num *n, fc_rational *out) {
  if (!n || !out) return FC_ERR_ARGS;

  fc_rational ex = fc_basis_eval(n->exponent);
  /* A fractional exponent is a root, and a root of a rational is not generally
   * rational. Returning an approximation would put a float back into the
   * substrate through the side door, so it is refused instead. */
  if (ex.den != 1) return FC_ERR_UNDEFINED;
  int64_t p = ex.num;
  if (p < 0 || p > 63) return FC_ERR_RANGE;

  fc_rational base = fc_basis_eval(n->basis);
  fc_rational acc = fc_rat(1, 1);
  for (int64_t i = 0; i < p; ++i) acc = fc_rat_mul(acc, base);

  fc_rational v = fc_rat((int64_t)n->value, 1);
  fc_rational mag = fc_rat_mul(v, acc);

  /* The four-valued signage. A zero-signed NUM evaluates to zero whatever its
   * magnitude - it is a POLE, not a scale - and the pole itself is carried in
   * the sign field rather than in the value, which is what lets NegZero and
   * PosZero be numerically equal and representationally distinct. */
  switch (n->sign) {
    case FC_POS_ONE:  *out = mag; break;
    case FC_NEG_ONE:  *out = fc_rat(-mag.num, mag.den); break;
    case FC_POS_ZERO:
    case FC_NEG_ZERO: *out = fc_rat(0, 1); break;
  }
  return FC_OK;
}

void fc_number_zero(fc_number *out) {
  memset(out, 0, sizeof *out);
  out->n = FC_NUMBER_DIGITS;
  for (int i = 0; i < FC_NUMBER_DIGITS; ++i) {
    out->d[i].sign = FC_POS_ONE;
    out->d[i].value = 0;
    out->d[i].basis.v = 1;
    out->d[i].exponent.v = 1;
  }
}

fc_status fc_number_from_u64(uint64_t v, fc_number *out) {
  if (!out) return FC_ERR_ARGS;
  fc_number_zero(out);
  /* Factoradic: digit i has radix (i+1), so digit i lies in [0, i] and digit 0
   * is always zero. Little-endian by radix, which is what makes the carry
   * rule local. */
  for (int i = 1; i < FC_NUMBER_DIGITS; ++i) {
    uint64_t radix = (uint64_t)(i + 1);
    out->d[i].value = v % radix;
    v /= radix;
  }
  if (v != 0) return FC_ERR_RANGE;    /* did not fit in FC_NUMBER_DIGITS */
  return FC_OK;
}

fc_status fc_number_to_u64(const fc_number *n, uint64_t *out) {
  if (!n || !out) return FC_ERR_ARGS;
  if (!fc_number_valid(n)) return FC_ERR_RANGE;
  uint64_t acc = 0, place = 1;
  for (int i = 1; i < FC_NUMBER_DIGITS; ++i) {
    place *= (uint64_t)i;              /* place value of digit i is i! */
    acc += n->d[i].value * place;
  }
  *out = acc;
  return FC_OK;
}

bool fc_number_valid(const fc_number *n) {
  if (!n) return false;
  if (n->n != FC_NUMBER_DIGITS) return false;
  /* The invariant an arithmetic bug breaks first: digit i <= i. */
  for (int i = 0; i < FC_NUMBER_DIGITS; ++i)
    if (n->d[i].value > (uint64_t)i) return false;
  return true;
}

fc_status fc_int_make(const fc_number *lo, const fc_number *hi, fc_int *out) {
  if (!lo || !hi || !out) return FC_ERR_ARGS;
  out->lo = *lo;
  out->hi = *hi;
  return FC_OK;
}

/* --- LEHMER: the n!-level topology index ---------------------------------- */

void fc_lehmer_zero(fc_lehmer *l, int levels) {
  if (!l) return;
  memset(l, 0, sizeof *l);
  if (levels < FC_LEVELS_MIN) levels = FC_LEVELS_MIN;
  if (levels > FC_LEVELS_MAX) levels = FC_LEVELS_MAX;
  l->levels = (uint8_t)levels;
}

bool fc_lehmer_valid(const fc_lehmer *l) {
  if (!l) return false;
  if (l->levels < FC_LEVELS_MIN || l->levels > FC_LEVELS_MAX) return false;
  /* Digit i in [0, i]. Digit 0 is therefore always 0, and a code that breaks
   * this is not an address in the topology - it is a corrupted one. */
  for (int i = 0; i < l->levels; ++i)
    if (l->d[i] > (uint8_t)i) return false;
  return true;
}

int fc_lehmer_cmp(const fc_lehmer *a, const fc_lehmer *b) {
  if (a->levels != b->levels) return a->levels < b->levels ? -1 : 1;
  /* Most significant digit is the LAST one: digit i carries place value i!, so
   * comparison walks downward. */
  for (int i = a->levels - 1; i >= 0; --i)
    if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
  return 0;
}

fc_status fc_lehmer_inc(fc_lehmer *l) {
  if (!fc_lehmer_valid(l)) return FC_ERR_ARGS;
  /* Odometer in the factorial number system: digit i wraps at i+1. */
  for (int i = 1; i < l->levels; ++i) {
    if (l->d[i] < (uint8_t)i) { ++l->d[i]; return FC_OK; }
    l->d[i] = 0;
  }
  /* Past the last address. Reported rather than wrapped to zero: two distinct
   * traversals arriving at the same address is the one thing an index must
   * never do quietly. */
  return FC_ERR_RANGE;
}

fc_status fc_lehmer_from_u64(uint64_t v, int levels, fc_lehmer *out) {
  if (!out) return FC_ERR_ARGS;
  if (levels < FC_LEVELS_MIN || levels > FC_LEVELS_MAX) return FC_ERR_RANGE;
  fc_lehmer_zero(out, levels);
  for (int i = 1; i < levels; ++i) {
    uint64_t radix = (uint64_t)(i + 1);
    out->d[i] = (uint8_t)(v % radix);
    v /= radix;
  }
  if (v != 0) return FC_ERR_RANGE;
  return FC_OK;
}

fc_status fc_lehmer_to_u64(const fc_lehmer *l, uint64_t *out) {
  if (!fc_lehmer_valid(l) || !out) return FC_ERR_ARGS;
  /* 20! is 2.4e18 and fits a uint64; 21! does not. Refused rather than
   * wrapped - a wrap would alias two topology addresses onto one, and at 96
   * levels almost every address is above this bound anyway. The digit array is
   * the canonical form; this is a convenience for the small end. */
  if (l->levels > 21) return FC_ERR_RANGE;
  uint64_t acc = 0, place = 1;
  for (int i = 1; i < l->levels; ++i) {
    place *= (uint64_t)i;               /* place value of digit i is i! */
    acc += (uint64_t)l->d[i] * place;
  }
  *out = acc;
  return FC_OK;
}

fc_status fc_lehmer_to_permutation(const fc_lehmer *l, uint8_t *perm, int max) {
  if (!fc_lehmer_valid(l) || !perm) return FC_ERR_ARGS;
  int n = l->levels;
  if (max < n) return FC_ERR_RANGE;
  /* Standard Lehmer decode: digit i, read from the most significant end,
   * selects and removes the d-th remaining element. This is what makes an
   * index a permutation, and therefore what connects NUMBER to BASE's
   * permutation group. */
  uint8_t pool[FC_LEVELS_MAX];
  for (int i = 0; i < n; ++i) pool[i] = (uint8_t)i;
  int remaining = n;
  for (int i = 0; i < n; ++i) {
    int digit = l->d[n - 1 - i];
    if (digit >= remaining) return FC_ERR_RANGE;
    perm[i] = pool[digit];
    for (int k = digit; k < remaining - 1; ++k) pool[k] = pool[k + 1];
    --remaining;
  }
  return FC_OK;
}

fc_status fc_lehmer_from_permutation(const uint8_t *perm, int levels, fc_lehmer *out) {
  if (!perm || !out) return FC_ERR_ARGS;
  if (levels < FC_LEVELS_MIN || levels > FC_LEVELS_MAX) return FC_ERR_RANGE;
  /* Refuse anything that is not a permutation of 0..levels-1. A repeated or
   * out-of-range element would encode to a code that decodes to something
   * else, which is worse than an error. */
  int seen[FC_LEVELS_MAX];
  memset(seen, 0, sizeof seen);
  for (int i = 0; i < levels; ++i) {
    if (perm[i] >= levels || seen[perm[i]]) return FC_ERR_RANGE;
    seen[perm[i]] = 1;
  }
  fc_lehmer_zero(out, levels);
  uint8_t pool[FC_LEVELS_MAX];
  for (int i = 0; i < levels; ++i) pool[i] = (uint8_t)i;
  int remaining = levels;
  for (int i = 0; i < levels; ++i) {
    int digit = 0;
    while (pool[digit] != perm[i]) ++digit;
    out->d[levels - 1 - i] = (uint8_t)digit;
    for (int k = digit; k < remaining - 1; ++k) pool[k] = pool[k + 1];
    --remaining;
  }
  return FC_OK;
}

/* --- terms ---------------------------------------------------------------- */

static fc_status term_slot(const fc_term *t, int which, fc_payload *out) {
  switch (t->kind) {
    case FC_T_VALUE:
      *out = which == 0 ? t->pre : (which == 1 ? t->peri : t->post);
      return FC_OK;
    case FC_T_REFERENCE: {
      const fc_payload *p = which == 0 ? t->ref_pre : (which == 1 ? t->ref_peri : t->ref_post);
      if (!p) return FC_ERR_ARGS;
      *out = *p;
      return FC_OK;
    }
    case FC_T_FUNCTIONAL: {
      fc_status (*f)(fc_payload *) =
          which == 0 ? t->fn_pre : (which == 1 ? t->fn_peri : t->fn_post);
      if (!f) return FC_ERR_ARGS;
      return f(out);
    }
  }
  return FC_ERR_ARGS;
}

static uint64_t payload_scalar(const fc_payload *p) {
  switch (p->kind) {
    case FC_P_INT: { uint64_t v = 0; fc_number_to_u64(&p->u.as_int.lo, &v); return v; }
    case FC_P_NUMBER: { uint64_t v = 0; fc_number_to_u64(&p->u.as_number, &v); return v; }
    case FC_P_STRING: {
      uint64_t h = 1469598103934665603ull;
      for (const char *c = p->u.as_string; *c; ++c) { h ^= (unsigned char)*c; h *= 1099511628211ull; }
      return h;
    }
  }
  return 0;
}

static fc_status default_eval(const fc_term *t, fc_int *out) {
  /* TOKEN's eval returns an INT, which is a {lo, hi} pair. The triple's
   * outermost slots are the natural bounds: pre is the earliest commitment and
   * post the settled one, so the pair is (pre, post) with peri carried in the
   * ordering check rather than in the result. */
  fc_payload a, c;
  fc_status s = term_slot(t, 0, &a);
  if (s != FC_OK) return s;
  s = term_slot(t, 2, &c);
  if (s != FC_OK) return s;
  fc_number lo, hi;
  if (fc_number_from_u64(payload_scalar(&a) % 479001600ull, &lo) != FC_OK) return FC_ERR_RANGE;
  if (fc_number_from_u64(payload_scalar(&c) % 479001600ull, &hi) != FC_OK) return FC_ERR_RANGE;
  return fc_int_make(&lo, &hi, out);
}

void fc_term_init_value(fc_term *t, const fc_payload *pre, const fc_payload *peri,
                        const fc_payload *post) {
  memset(t, 0, sizeof *t);
  t->kind = FC_T_VALUE;
  if (pre) t->pre = *pre;
  if (peri) t->peri = *peri;
  if (post) t->post = *post;
  t->eval = default_eval;
}

void fc_term_init_reference(fc_term *t, const fc_payload *pre, const fc_payload *peri,
                            const fc_payload *post) {
  memset(t, 0, sizeof *t);
  t->kind = FC_T_REFERENCE;
  t->ref_pre = pre; t->ref_peri = peri; t->ref_post = post;
  t->eval = default_eval;
}

void fc_term_init_functional(fc_term *t,
                             fc_status (*pre)(fc_payload *),
                             fc_status (*peri)(fc_payload *),
                             fc_status (*post)(fc_payload *)) {
  memset(t, 0, sizeof *t);
  t->kind = FC_T_FUNCTIONAL;
  t->fn_pre = pre; t->fn_peri = peri; t->fn_post = post;
  t->eval = default_eval;
}

fc_status fc_term_eval(const fc_term *t, fc_int *out) {
  if (!t || !out) return FC_ERR_ARGS;
  if (!t->eval) return FC_ERR_ARGS;
  return t->eval(t, out);
}

bool fc_term_ordered(const fc_term *t) {
  fc_payload a, b, c;
  if (!t) return false;
  if (term_slot(t, 0, &a) != FC_OK) return false;
  if (term_slot(t, 1, &b) != FC_OK) return false;
  if (term_slot(t, 2, &c) != FC_OK) return false;
  uint64_t x = payload_scalar(&a), y = payload_scalar(&b), z = payload_scalar(&c);
  return x <= y && y <= z;
}

bool fc_observable_is_expr(const fc_observable *o) {
  if (!o) return false;
  const fc_term *ts[3] = { &o->pre, &o->peri, &o->post };
  for (int i = 0; i < 3; ++i) {
    if (ts[i]->kind != FC_T_VALUE) return false;
    if (ts[i]->pre.kind != FC_P_STRING) return false;
    if (ts[i]->peri.kind != FC_P_STRING) return false;
    if (ts[i]->post.kind != FC_P_STRING) return false;
  }
  return true;
}

fc_status fc_operator_eval(const fc_operator *o, fc_observable *out) {
  if (!o || !out) return FC_ERR_ARGS;
  memset(out, 0, sizeof *out);
  /* "Semantics similar to {LHS : OBSERVABLE::pre, OP : OBSERVABLE::peri,
   * RHS : OBSERVABLE::post}" - so the operator itself occupies the peri slot.
   * That is what "strict infix-only" buys: the shape of the result records
   * where the operator sat. */
  out->pre = o->lhs;
  out->post = o->rhs;
  fc_payload op;
  memset(&op, 0, sizeof op);
  op.kind = FC_P_STRING;
  static const char *const names[4] = { "+", "-", "*", "/" };
  snprintf(op.u.as_string, sizeof op.u.as_string, "%s", names[o->op]);
  fc_term_init_value(&out->peri, &op, &op, &op);
  return FC_OK;
}

/* --- DATA / REAL / INTERVAL ----------------------------------------------- */

bool fc_triple_ordered(const fc_triple *t) {
  if (!t) return false;
  return fc_rat_cmp(t->min, t->nom) <= 0 && fc_rat_cmp(t->nom, t->max) <= 0;
}

bool fc_data_ordered(const fc_data *d) {
  if (!d) return false;
  return fc_triple_ordered(&d->low) && fc_triple_ordered(&d->value) &&
         fc_triple_ordered(&d->high);
}

bool fc_interval_ordered(const fc_interval *i) {
  return i && fc_rat_cmp(i->lo, i->hi) <= 0;
}

/* --- Kapprox (FLOWALGEBRA 30) ---------------------------------------------- */

const char *fc_kclass_name(fc_kclass c) {
  switch (c) {
    case FC_K_GENERAL: return "General";
    case FC_K_SPECIFIC: return "Specific";
    case FC_K_DISTINCTIVE: return "Distinctive";
    case FC_K_ARBITRARY: return "Arbitrary";
  }
  return "?";
}

int64_t fc_log2_milli(uint64_t x) {
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

int64_t fc_log2_factorial_milli(uint64_t n) {
  if (n < 2) return 0;
  /* Exact below the crossover, Stirling above. The crossover is DECLARED here
   * rather than tuned silently: past 100000 the summation costs more than the
   * accuracy is worth to any caller of a complexity estimate. */
  if (n <= 100000ull) {
    int64_t acc = 0;
    for (uint64_t i = 2; i <= n; ++i) acc += fc_log2_milli(i);
    return acc;
  }
  /* log2(n!) ~ n*log2(n) - n*log2(e) + 0.5*log2(2*pi*n).
   * log2(e) = 1.4427, taken as 1443 milli so the estimate errs low rather
   * than high - a complexity bound that overstates is the worse mistake. */
  int64_t ln = fc_log2_milli(n);
  int64_t term = (int64_t)n * ln - (int64_t)n * 1443;
  int64_t corr = fc_log2_milli(6283ull * n / 1000ull) / 2;
  return term + corr;
}

int64_t fc_kapprox_milli(fc_kclass c, uint64_t n) {
  switch (c) {
    case FC_K_GENERAL:
      return fc_log2_milli(2 * n + 1);
    case FC_K_SPECIFIC:
      /* log2(2*(n!)+1) ~ 1 + log2(n!); the +1 is far below the resolution. */
      return 1000 + fc_log2_factorial_milli(n);
    case FC_K_DISTINCTIVE:
      return fc_log2_factorial_milli(2 * n + 1);
    case FC_K_ARBITRARY: {
      /* (2(n!)+1)!  The argument m = 2(n!)+1 overflows uint64 past n=20, so
       * m is never materialized. What is returned is the ORDER of the answer -
       * log2(log2(m!)) - because log2(m!) itself is around 1e152 at n=96 and
       * exceeds anything a caller can do arithmetic with. Reported as an order
       * so the class remains comparable without pretending to a precision it
       * does not have. */
      int64_t log2m = 1000 + fc_log2_factorial_milli(n);
      return log2m + fc_log2_milli((uint64_t)(log2m / 1000 + 1));
    }
  }
  return 0;
}

int64_t fc_kapprox_order_milli(fc_kclass c, uint64_t n) {
  /* ONE SCALE FOR ALL FOUR: log2(log2(value)), in milli.
   *
   * The first three classes fit in log2 bits; the fourth does not, because
   * log2((2(n!)+1)!) is itself around 1e152 at n=96. Reporting three on one
   * scale and the fourth on another made the ladder appear to INVERT -
   * Arbitrary came out below Distinctive - which is a units error and not a
   * property of the ladder. The order scale is the one that stays valid for
   * every class at every level, so comparisons use it. */
  if (c == FC_K_ARBITRARY) {
    /* Already an order by construction: log2(m) + log2(log2(m)) where the
     * value is m! and m is never materialized. */
    return fc_kapprox_milli(c, n);
  }
  int64_t bits = fc_kapprox_milli(c, n);
  uint64_t whole = (uint64_t)(bits / 1000);
  if (whole < 1) whole = 1;
  return fc_log2_milli(whole);
}

bool fc_kapprox_separated(uint64_t n) {
  int64_t a = fc_kapprox_order_milli(FC_K_GENERAL, n);
  int64_t b = fc_kapprox_order_milli(FC_K_SPECIFIC, n);
  int64_t c = fc_kapprox_order_milli(FC_K_DISTINCTIVE, n);
  int64_t d = fc_kapprox_order_milli(FC_K_ARBITRARY, n);
  return a < b && b < c && c < d;
}

/* --- Cayley-Dickson ------------------------------------------------------- */

static int g_imag_ready = 0;
static int g_imag_log2 = 0;
static int8_t g_mul_sign[FC_IMAG_COUNT][FC_IMAG_COUNT];
static uint8_t g_mul_index[FC_IMAG_COUNT][FC_IMAG_COUNT];

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

fc_status fc_imag_init(void) {
  /* Closure is a property of the DIMENSION, and only 1, 2, 4, 8 and 16 have
   * it. FC_IMAG_COUNT is 1 + 3*trifectas, so four trifectas gives 13 - which
   * is not a power of two and admits no consistent product. Refused here
   * rather than producing a table that is not a multiplication. */
  int n = 0, d = FC_IMAG_COUNT;
  while (d > 1) {
    if (d & 1) return FC_ERR_NOT_CLOSED;
    d >>= 1; ++n;
  }
  if (n > 4) return FC_ERR_NOT_CLOSED;   /* beyond sedenions the doubling
                                          * continues but the document's
                                          * trifecta framing does not */
  g_imag_log2 = n;
  for (int i = 0; i < FC_IMAG_COUNT; ++i)
    for (int j = 0; j < FC_IMAG_COUNT; ++j) {
      int idx, sgn;
      cd_mul(n, i, j, &idx, &sgn);
      g_mul_index[i][j] = (uint8_t)idx;
      g_mul_sign[i][j] = (int8_t)sgn;
    }
  g_imag_ready = 1;
  return FC_OK;
}

fc_status fc_imag_mul(fc_imag a, fc_imag b, fc_imag *index, int *sign) {
  if (!g_imag_ready) return FC_ERR_NOT_CLOSED;
  if (a >= FC_IMAG_COUNT || b >= FC_IMAG_COUNT || !index || !sign) return FC_ERR_ARGS;
  *index = g_mul_index[a][b];
  *sign = g_mul_sign[a][b];
  return FC_OK;
}

int fc_imag_conj_sign(fc_imag a) { return a == 0 ? 1 : -1; }

const char *fc_imag_name(fc_imag a) {
  /* The document's own names, extended by the fifth trifecta the dimension
   * requires. */
  static const char *const n[16] = {
    "one", "i", "j", "k", "m", "n", "o", "r",
    "s", "t", "e", "f", "g", "p", "q", "u"
  };
  return a < 16 ? n[a] : "?";
}

void fc_base_init(fc_base *b) {
  memset(b, 0, sizeof *b);
  b->head = b->tail = -1;
}

fc_status fc_base_push(fc_base *b, fc_imaginary v) {
  if (!b) return FC_ERR_ARGS;
  if (v.imag >= FC_IMAG_COUNT) return FC_ERR_RANGE;
  if (b->count >= FC_BASE_MAX) return FC_ERR_FULL;
  int i = b->count++;
  b->node[i] = v;
  b->prev[i] = b->tail;
  b->next[i] = -1;
  if (b->tail >= 0) b->next[b->tail] = i;
  b->tail = i;
  if (b->head < 0) b->head = i;
  return FC_OK;
}

fc_status fc_base_eval(const fc_base *b, fc_imaginary *out, int *sign) {
  if (!b || !out || !sign) return FC_ERR_ARGS;
  if (!g_imag_ready) return FC_ERR_NOT_CLOSED;
  if (b->head < 0) { out->imag = 0; out->ring = 0; *sign = 1; return FC_OK; }
  /* Folded in LIST ORDER, left to right. The product is non-commutative from
   * dimension 4 and non-associative from 8, so the order is part of the
   * contract - a different traversal is a different answer, not a faster one. */
  fc_imag acc = b->node[b->head].imag;
  size_t ring = b->node[b->head].ring;
  int s = 1;
  for (int i = b->next[b->head]; i >= 0; i = b->next[i]) {
    fc_imag r;
    int rs;
    fc_status st = fc_imag_mul(acc, b->node[i].imag, &r, &rs);
    if (st != FC_OK) return st;
    acc = r;
    s *= rs;
    ring += b->node[i].ring;
  }
  out->imag = acc;
  out->ring = ring;
  *sign = s;
  return FC_OK;
}

bool fc_base_is_permutation(fc_imag by) {
  if (!g_imag_ready || by >= FC_IMAG_COUNT) return false;
  /* Left multiplication by a fixed unit maps the units onto the units,
   * bijectively - the permutation-group property the document asserts of BASE.
   * Checked by counting images rather than assumed. */
  int seen[FC_IMAG_COUNT];
  memset(seen, 0, sizeof seen);
  for (int i = 0; i < FC_IMAG_COUNT; ++i) {
    fc_imag r;
    int s;
    if (fc_imag_mul(by, (fc_imag)i, &r, &s) != FC_OK) return false;
    if (r >= FC_IMAG_COUNT || seen[r]) return false;
    seen[r] = 1;
  }
  return true;
}

/* --- TENSORIAL / TENSOR --------------------------------------------------- */

void fc_tensorial_init(fc_tensorial *t, size_t group_id) {
  memset(t, 0, sizeof *t);
  t->group_id = group_id;
  t->regular = true;
  t->unknownsymmetric = true;   /* until declared otherwise */
}

fc_status fc_tensorial_push(fc_tensorial *t, const fc_modulation *m) {
  if (!t || !m) return FC_ERR_ARGS;
  if (t->count >= FC_TENSORIAL_MAX) return FC_ERR_FULL;
  t->data[t->count++] = *m;
  return FC_OK;
}

bool fc_tensorial_flags_consistent(const fc_tensorial *t) {
  if (!t) return false;
  if (t->symmetric && t->antisymmetric) return false;
  if (t->unknownsymmetric && (t->symmetric || t->antisymmetric)) return false;
  return true;
}

void fc_tensor_init(fc_tensor *t) { memset(t, 0, sizeof *t); }

fc_status fc_tensor_add_covariant(fc_tensor *t, const fc_tensorial *v) {
  if (!t || !v) return FC_ERR_ARGS;
  if (t->n_cov >= FC_TENSOR_RANK_MAX) return FC_ERR_FULL;
  t->covariant[t->n_cov++] = *v;
  return FC_OK;
}

fc_status fc_tensor_add_contravariant(fc_tensor *t, const fc_tensorial *v) {
  if (!t || !v) return FC_ERR_ARGS;
  if (t->n_con >= FC_TENSOR_RANK_MAX) return FC_ERR_FULL;
  t->contravariant[t->n_con++] = *v;
  return FC_OK;
}

void fc_tensor_rank(const fc_tensor *t, int *contra, int *cov) {
  if (contra) *contra = t ? t->n_con : 0;
  if (cov) *cov = t ? t->n_cov : 0;
}

fc_status fc_tensor_eval(const fc_tensor *t, fc_modulation *out) {
  if (!t || !out) return FC_ERR_ARGS;
  memset(out, 0, sizeof *out);
  fc_base_init(&out->base);
  /* CALCULUS's eval "uses the dataset covariant/contravariant for
   * tensor-calculus, and results in a MODULATION". The contraction implemented
   * here is the one the types actually support: sum the reals, take the widest
   * bounds, and fold the bases in declared order (contravariant then
   * covariant). A fuller contraction needs an index notation the document does
   * not give, so this is stated as what it is rather than named `contract`. */
  fc_rational acc = fc_rat(0, 1);
  fc_rational lo = fc_rat(0, 1), hi = fc_rat(0, 1);
  bool first = true;
  const fc_tensorial *groups[2 * FC_TENSOR_RANK_MAX];
  int n_groups = 0;
  for (int i = 0; i < t->n_con; ++i) groups[n_groups++] = &t->contravariant[i];
  for (int i = 0; i < t->n_cov; ++i) groups[n_groups++] = &t->covariant[i];

  for (int g = 0; g < n_groups; ++g) {
    if (!fc_tensorial_flags_consistent(groups[g])) return FC_ERR_ARGS;
    for (int m = 0; m < groups[g]->count; ++m) {
      const fc_modulation *mod = &groups[g]->data[m];
      acc = fc_rat_add(acc, mod->real.val);
      if (first) { lo = mod->bounds.lo; hi = mod->bounds.hi; first = false; }
      else {
        if (fc_rat_cmp(mod->bounds.lo, lo) < 0) lo = mod->bounds.lo;
        if (fc_rat_cmp(mod->bounds.hi, hi) > 0) hi = mod->bounds.hi;
      }
      for (int k = mod->base.head; k >= 0; k = mod->base.next[k])
        if (fc_base_push(&out->base, mod->base.node[k]) != FC_OK) return FC_ERR_FULL;
    }
  }
  out->real.val = acc;
  out->bounds.lo = lo;
  out->bounds.hi = hi;
  return FC_OK;
}

/* --- UNIT / EXPRESSION ---------------------------------------------------- */

void fc_unit_init(fc_unit *u, fc_termkind head) {
  memset(u, 0, sizeof *u);
  u->head = head;
}

fc_status fc_unit_add_real(fc_unit *u, fc_real r) {
  if (!u) return FC_ERR_ARGS;
  if (u->n_reals >= FC_UNIT_MAX) return FC_ERR_FULL;
  u->reals[u->n_reals++] = r;
  return FC_OK;
}

fc_status fc_unit_add_interval(fc_unit *u, fc_interval i) {
  if (!u) return FC_ERR_ARGS;
  if (!fc_interval_ordered(&i)) return FC_ERR_RANGE;
  if (u->n_intervals >= FC_UNIT_MAX) return FC_ERR_FULL;
  u->intervals[u->n_intervals++] = i;
  return FC_OK;
}

fc_status fc_unit_add_tensor(fc_unit *u, const fc_tensor *t) {
  if (!u || !t) return FC_ERR_ARGS;
  if (u->n_base >= FC_UNIT_MAX) return FC_ERR_FULL;
  u->base[u->n_base++] = *t;
  return FC_OK;
}

bool fc_unit_is_atom(const fc_unit *u)   { return u && u->head == FC_T_VALUE; }
bool fc_unit_is_change(const fc_unit *u) { return u && u->head == FC_T_REFERENCE; }
bool fc_unit_is_force(const fc_unit *u)  { return u && u->head == FC_T_FUNCTIONAL; }

void fc_expression_init(fc_expression *e) { memset(e, 0, sizeof *e); }

fc_status fc_expression_add(fc_expression *e, const fc_unit *u) {
  if (!e || !u) return FC_ERR_ARGS;
  if (e->n_output >= FC_UNIT_MAX) return FC_ERR_FULL;
  e->output[e->n_output++] = *u;
  return FC_OK;
}

/* --- BBST ----------------------------------------------------------------- */

void fc_bbst_init(fc_bbst *t) {
  memset(t, 0, sizeof *t);
  t->root = -1;
}

fc_status fc_bbst_insert(fc_bbst *t, const int64_t key[FC_KD], int expr) {
  if (!t || !key) return FC_ERR_ARGS;
  if (t->count >= FC_BBST_MAX) return FC_ERR_FULL;
  fc_bbst_node *n = &t->node[t->count];
  memset(n, 0, sizeof *n);
  for (int i = 0; i < FC_KD; ++i) n->key[i] = key[i];
  n->expr = expr;
  n->left = n->right = -1;
  ++t->count;
  return FC_OK;
}

int fc_bbst_expr(const fc_bbst *t, int node) {
  if (!t || node < 0 || node >= t->count) return -1;
  return t->node[node].expr;
}

static int kd_cmp(const int64_t *a, const int64_t *b, int axis) {
  if (a[axis] != b[axis]) return a[axis] < b[axis] ? -1 : 1;
  /* Ties broken on the remaining axes in fixed order, so the sort is total and
   * two identical input sets build identical trees. Without this the median
   * split would depend on the incoming order. */
  for (int i = 0; i < FC_KD; ++i)
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  return 0;
}

static int kd_build(fc_bbst *t, int *idx, int lo, int hi, int depth) {
  if (lo >= hi) return -1;
  int axis = depth % FC_KD;
  /* Insertion sort on the slice: n is bounded by FC_BBST_MAX and the constant
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

fc_status fc_bbst_build(fc_bbst *t) {
  if (!t) return FC_ERR_ARGS;
  if (t->count == 0) { t->root = -1; return FC_OK; }
  int idx[FC_BBST_MAX];
  for (int i = 0; i < t->count; ++i) idx[i] = i;
  t->root = kd_build(t, idx, 0, t->count, 0);
  return FC_OK;
}

int fc_bbst_find(const fc_bbst *t, const int64_t key[FC_KD]) {
  if (!t || !key) return -1;
  int cur = t->root, depth = 0;
  while (cur >= 0) {
    int c = kd_cmp(key, t->node[cur].key, depth % FC_KD);
    if (c == 0) return cur;
    cur = c < 0 ? t->node[cur].left : t->node[cur].right;
    ++depth;
  }
  return -1;
}

static int kd_depth(const fc_bbst *t, int n) {
  if (n < 0) return 0;
  int l = kd_depth(t, t->node[n].left);
  int r = kd_depth(t, t->node[n].right);
  return 1 + (l > r ? l : r);
}

int fc_bbst_depth(const fc_bbst *t) { return t ? kd_depth(t, t->root) : 0; }

/* --- TST ------------------------------------------------------------------ */

fc_rational fc_origo_interval2(const fc_origo *o) {
  /* Minkowski, signature (-,+,+,+). Exact, so the classification below is
   * decided rather than approximated - which matters because the null case is
   * an equality and a float would almost never hit it. */
  fc_rational t2 = fc_rat_mul(o->t, o->t);
  fc_rational x2 = fc_rat_mul(o->x, o->x);
  fc_rational y2 = fc_rat_mul(o->y, o->y);
  fc_rational z2 = fc_rat_mul(o->z, o->z);
  fc_rational s = fc_rat_add(x2, fc_rat_add(y2, z2));
  return fc_rat_add(s, fc_rat(-t2.num, t2.den));
}

fc_interval_class fc_origo_classify(const fc_origo *o, bool provenance_ok) {
  if (!o) return FC_INTERVAL_INDETERMINATE;
  /* WORLD_MODEL section 8.4's fourth class comes FIRST: a delta whose clock,
   * spatial or provenance quality is too poor to classify is indeterminate,
   * and the system is required to represent that rather than guess a class
   * from numbers it does not trust. */
  if (!provenance_ok) return FC_INTERVAL_INDETERMINATE;
  fc_rational s2 = fc_origo_interval2(o);
  if (s2.num == 0) return FC_INTERVAL_NULL;
  return s2.num < 0 ? FC_INTERVAL_TIMELIKE : FC_INTERVAL_SPACELIKE;
}

void fc_tst_init(fc_tst *t) {
  memset(t, 0, sizeof *t);
  t->root = -1;
}

int fc_tst_add(fc_tst *t, const fc_origo *origo) {
  if (!t || t->count >= FC_BBST_MAX) return -1;
  int i = t->count++;
  fc_tst_node *n = &t->node[i];
  memset(n, 0, sizeof *n);
  if (origo) n->origo = *origo;
  for (int k = 0; k < 3; ++k) fc_bbst_init(&n->leaf[k]);
  n->left = n->middle = n->right = -1;
  /* No varying OrigoDelta by default: the node's static origo is the whole
   * boundary until a caller attaches expressions. */
  for (int k = 0; k < 4; ++k) n->origo_expr[k] = -1;
  n->used = true;
  if (t->root < 0) t->root = i;
  return i;
}

fc_status fc_tst_link(fc_tst *t, int parent, int which, int child) {
  if (!t || parent < 0 || parent >= t->count) return FC_ERR_ARGS;
  if (child < 0 || child >= t->count) return FC_ERR_ARGS;
  if (which < 0 || which > 2) return FC_ERR_RANGE;
  if (which == 0) t->node[parent].left = child;
  else if (which == 1) t->node[parent].middle = child;
  else t->node[parent].right = child;
  return FC_OK;
}

static bool tst_path(const fc_tst *t, int from, int target, int *path, int *n, int depth) {
  if (from < 0 || depth > FC_BBST_MAX) return false;
  path[(*n)++] = from;
  if (from == target) return true;
  const fc_tst_node *k = &t->node[from];
  int kids[3] = { k->left, k->middle, k->right };
  for (int i = 0; i < 3; ++i) {
    if (kids[i] < 0) continue;
    if (tst_path(t, kids[i], target, path, n, depth + 1)) return true;
  }
  --(*n);
  return false;
}

fc_status fc_tst_absolute_origo(const fc_tst *t, int node, fc_origo *out) {
  if (!t || !out || node < 0 || node >= t->count) return FC_ERR_ARGS;
  int path[FC_BBST_MAX];
  int n = 0;
  if (!tst_path(t, t->root, node, path, &n, 0)) return FC_ERR_ARGS;
  /* ORIGO is a delta "relative to the Gravity Well's/Parental Node's ORIGO",
   * so absolute position is the fold along the path from the root. Addition of
   * deltas, not composition: the document calls it an INTERVAL delta. */
  memset(out, 0, sizeof *out);
  out->t = fc_rat(0, 1); out->x = fc_rat(0, 1);
  out->y = fc_rat(0, 1); out->z = fc_rat(0, 1);
  for (int i = 0; i < n; ++i) {
    const fc_origo *o = &t->node[path[i]].origo;
    out->t = fc_rat_add(out->t, o->t);
    out->x = fc_rat_add(out->x, o->x);
    out->y = fc_rat_add(out->y, o->y);
    out->z = fc_rat_add(out->z, o->z);
  }
  return FC_OK;
}

fc_status fc_tst_iterate(const fc_tst *t, int node, fc_iteration *out) {
  if (!t || !out || node < 0 || node >= t->count) return FC_ERR_ARGS;
  memset(out, 0, sizeof *out);
  const fc_tst_node *n = &t->node[node];
  /* Strictly ordered, and each stage requires the previous one: PRE gives the
   * Lorentzian frame, PERI adds topology to that frame, POST adds temporal
   * structure to that object. A POST leaf with no PERI before it is not a
   * partial result, it is a malformed one - so the flags cascade rather than
   * being set independently. */
  out->frame = n->leaf[FC_LEAF_PRE].count > 0;
  out->volumetric = out->frame && n->leaf[FC_LEAF_PERI].count > 0;
  out->animated = out->volumetric && n->leaf[FC_LEAF_POST].count > 0;
  out->expressions = n->leaf[0].count + n->leaf[1].count + n->leaf[2].count;
  return FC_OK;
}


fc_status fc_tst_set_origo_expr(fc_tst *t, int node, int et, int ex, int ey, int ez) {
  if (!t || node < 0 || node >= t->count || !t->node[node].used) return FC_ERR_RANGE;
  t->node[node].origo_expr[0] = et;
  t->node[node].origo_expr[1] = ex;
  t->node[node].origo_expr[2] = ey;
  t->node[node].origo_expr[3] = ez;
  return FC_OK;
}

bool fc_tst_has_origo_expr(const fc_tst *t, int node) {
  int k;
  if (!t || node < 0 || node >= t->count || !t->node[node].used) return false;
  for (k = 0; k < 4; ++k) if (t->node[node].origo_expr[k] >= 0) return true;
  return false;
}
