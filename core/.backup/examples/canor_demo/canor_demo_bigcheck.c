/* canor_demo_bigcheck.c - bigcheck
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* bigcheck                                                                   */
/* ========================================================================== */
/* Arbitrary-precision arithmetic that is not checked is arbitrary-precision
 * arithmetic that is wrong, and every number the mandel command prints rests
 * on it. The cases chosen are the ones where a plausible implementation
 * breaks: carry propagation past the inner multiply loop, INT64_MIN, aliasing
 * r with an operand, subtraction that crosses zero, and gcd where one side
 * divides the other. */

static int bchecks, bfails;
static void bcheck(int cond, const char *what) {
  ++bchecks;
  if (!cond) { ++bfails; printf("  FAIL  %s\n", what); }
}
static bool beq(const canor_big_t *a, const char *dec) {
  char buf[512];
  canor_big_format(a, buf, sizeof buf);
  return !strcmp(buf, dec);
}
static bool bqeq(const canor_big_rat_t *q, const char *dec) {
  char buf[512];
  canor_big_rat_format(q, buf, sizeof buf);
  return !strcmp(buf, dec);
}

int canor_demo_cmd_bigcheck(void) {
  canor_big_t a, b, r;
  canor_big_rat_t p, q, s;
  int i;

  canor_big_init(&a); canor_big_init(&b); canor_big_init(&r);
  canor_big_rat_init(&p); canor_big_rat_init(&q); canor_big_rat_init(&s);

  canor_big_set_i64(&a, 0);
  bcheck(beq(&a, "0") && canor_big_is_zero(&a) && a.sign == 0, "zero is canonical");
  canor_big_set_i64(&a, INT64_MIN);
  bcheck(beq(&a, "-9223372036854775808"), "INT64_MIN round-trips");
  canor_big_set_i64(&a, INT64_MAX);
  bcheck(beq(&a, "9223372036854775807"), "INT64_MAX round-trips");

  /* Carry all the way across: (2^64 - 1)^2 = 2^128 - 2^65 + 1. */
  (void)canor_big_parse(&a, "18446744073709551615");
  (void)canor_big_mul(&r, &a, &a);
  bcheck(beq(&r, "340282366920938463426481119284349108225"),
         "(2^64-1)^2, carry propagates past the inner loop");

  /* Aliasing: r is also the operand. */
  (void)canor_big_parse(&a, "123456789012345678901234567890");
  r = a;
  (void)canor_big_mul(&r, &r, &r);
  bcheck(beq(&r, "15241578753238836750495351562536198787501905199875019052100"),
         "multiply aliasing the destination");

  /* Subtraction crossing zero, both directions. */
  (void)canor_big_parse(&a, "1000000000000000000000");
  (void)canor_big_parse(&b, "1000000000000000000001");
  (void)canor_big_sub(&r, &a, &b);
  bcheck(beq(&r, "-1"), "subtraction crossing zero");
  (void)canor_big_sub(&r, &b, &a);
  bcheck(beq(&r, "1"), "subtraction the other way");
  (void)canor_big_sub(&r, &a, &a);
  bcheck(beq(&r, "0") && r.sign == 0, "x - x is canonical zero");

  /* Shifts must agree with multiplication by a power of two. */
  (void)canor_big_parse(&a, "987654321098765432109876543210");
  (void)canor_big_shl(&r, &a, 97);
  canor_big_set_i64(&b, 1);
  (void)canor_big_shl(&b, &b, 97);
  (void)canor_big_mul(&a, &a, &b);
  bcheck(canor_big_cmp(&r, &a) == 0, "shl 97 equals multiply by 2^97");
  canor_big_shr(&r, &r, 97);
  (void)canor_big_parse(&b, "987654321098765432109876543210");
  bcheck(canor_big_cmp(&r, &b) == 0, "shr undoes shl");

  /* Division: a == q*b + rem, with rem carrying a's sign. */
  (void)canor_big_parse(&a, "-123456789012345678901234567890");
  (void)canor_big_parse(&b, "9876543210987");
  {
    canor_big_t quo, rem, back;
    canor_big_init(&quo); canor_big_init(&rem); canor_big_init(&back);
    (void)canor_big_divmod(&quo, &rem, &a, &b);
    (void)canor_big_mul(&back, &quo, &b);
    (void)canor_big_add(&back, &back, &rem);
    bcheck(canor_big_cmp(&back, &a) == 0, "divmod reconstructs the dividend");
    bcheck(rem.sign <= 0, "remainder carries the dividend's sign");
    bcheck(canor_big_cmp_abs(&rem, &b) < 0, "remainder is smaller than the divisor");
  }

  /* gcd, including the case where one side divides the other. */
  (void)canor_big_parse(&a, "123456789012345678901234567890");
  (void)canor_big_parse(&b, "9876543210");
  canor_big_gcd(&r, &a, &b);
  bcheck(beq(&r, "90"), "gcd of two large integers");
  canor_big_gcd(&r, &a, &a);
  bcheck(canor_big_cmp(&r, &a) == 0, "gcd(x, x) = x");
  canor_big_set_i64(&b, 0);
  canor_big_gcd(&r, &a, &b);
  bcheck(canor_big_cmp(&r, &a) == 0, "gcd(x, 0) = x");

  /* Rationals reduce, and stay reduced. */
  (void)canor_big_rat_set_i64(&p, 6, 8);
  bcheck(bqeq(&p, "3/4"), "6/8 reduces to 3/4");
  (void)canor_big_rat_set_i64(&p, 4, -8);
  bcheck(bqeq(&p, "-1/2"), "a negative denominator moves to the numerator");
  (void)canor_big_rat_set_i64(&p, 0, -7);
  bcheck(bqeq(&p, "0"), "zero normalizes to 0/1");
  (void)canor_big_rat_set_i64(&p, 1, 3);
  (void)canor_big_rat_set_i64(&q, 1, 6);
  (void)canor_big_rat_add(&s, &p, &q);
  bcheck(bqeq(&s, "1/2"), "1/3 + 1/6 = 1/2");
  (void)canor_big_rat_sub(&s, &p, &q);
  bcheck(bqeq(&s, "1/6"), "1/3 - 1/6 = 1/6");
  (void)canor_big_rat_mul(&s, &p, &q);
  bcheck(bqeq(&s, "1/18"), "1/3 * 1/6 = 1/18");

  /* Parsing: exact decimals and fractions. */
  bcheck((canor_big_rat_parse(&p, "-0.3125") == CANOR_OK) && bqeq(&p, "-5/16"), "-0.3125 = -5/16");
  bcheck((canor_big_rat_parse(&p, "3/4") == CANOR_OK) && bqeq(&p, "3/4"), "3/4 parses");
  bcheck((canor_big_rat_parse(&p, "-2") == CANOR_OK) && bqeq(&p, "-2"), "a bare integer parses");
  bcheck(canor_big_rat_parse(&p, "3/0") != CANOR_OK, "a zero denominator is refused");
  bcheck(canor_big_parse(&a, "12x") != CANOR_OK, "trailing junk is refused");

  /* Comparison never divides, so it must be exact at any size. */
  (void)canor_big_rat_parse(&p, "1/3");
  (void)canor_big_rat_parse(&q, "100000000000000000000/299999999999999999999");
  bcheck(canor_big_rat_cmp(&p, &q, NULL) < 0, "1/3 < 1e20/(3e20-1), by cross-multiplication");
  (void)canor_big_rat_set_i64(&p, 9, 2);
  bcheck(canor_big_rat_cmp_i64(&p, 4, NULL) > 0 && canor_big_rat_cmp_i64(&p, 5, NULL) < 0, "9/2 sits between 4 and 5");

  /* The fixed-point projection, and its refusal. */
  {
    int64_t v = 0;
    (void)canor_big_rat_set_i64(&p, -1, 4);
    bcheck((canor_big_rat_to_fixed(&p.num, &p.den, 32, &v) == CANOR_OK) && v == -(1LL << 30),
           "-1/4 in Q32");
    (void)canor_big_rat_parse(&p, "100000000000/1");
    bcheck(canor_big_rat_to_fixed(&p.num, &p.den, 32, &v) != CANOR_OK,
           "a value too wide for Q32 is refused, not saturated");
  }

  /* The fixed capacity refuses rather than truncates, and the value survives
   * intact: 64 squarings of a 9-digit value would need far more than
   * CANOR_BIG_LIMBS_MAX limbs. */
  {
    canor_big_t big;
    bool grew = false;
    canor_big_init(&big);
    (void)canor_big_parse(&big, "123456789");
    for (i = 0; i < 64; ++i)
      if (canor_big_mul(&big, &big, &big) != CANOR_OK) break;
    grew = (i < 64);
    bcheck(grew, "the capacity eventually refuses a squaring");
    bcheck(!canor_big_is_zero(&big), "a refused operation leaves the operand intact");
  }

  /* An exact orbit: c = -1 is period 2 (0, -1, 0, -1, ...), so it is bounded
   * forever and its values never grow. If the rational path had any drift at
   * all, this is where it would show. */
  {
    canor_big_rat_t cr, ci;
    int steps = 0, bits = 0, res;
    canor_big_rat_init(&cr); canor_big_rat_init(&ci);
    (void)canor_big_rat_set_i64(&cr, -1, 1);
    (void)canor_big_rat_set_i64(&ci, 0, 1);
    res = canor_demo_escape_exact(&cr, &ci, 200, &steps, &bits);
    bcheck(res == 0 && steps == 200, "c = -1 stays bounded for 200 exact steps");
    bcheck(bits <= 2, "and its orbit never grows past a couple of bits");
    (void)canor_big_rat_set_i64(&cr, 1, 1);
    res = canor_demo_escape_exact(&cr, &ci, 200, &steps, &bits);
    bcheck(res == 1, "c = 1 escapes");
  }

  printf("\n%d checks, %d failures\n", bchecks, bfails);
  return bfails ? 1 : 0;
}
