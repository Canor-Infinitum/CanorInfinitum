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
static bool beq(const fb_int *a, const char *dec) {
  char buf[512];
  fb_format(a, buf, sizeof buf);
  return !strcmp(buf, dec);
}
static bool bqeq(const fb_rat *q, const char *dec) {
  char buf[512];
  fb_rat_format(q, buf, sizeof buf);
  return !strcmp(buf, dec);
}

int canor_demo_cmd_bigcheck(void) {
  fb_int a, b, r;
  fb_rat p, q, s;
  int i;

  fb_init(&a); fb_init(&b); fb_init(&r);
  fb_rat_init(&p); fb_rat_init(&q); fb_rat_init(&s);

  fb_set_i64(&a, 0);
  bcheck(beq(&a, "0") && fb_is_zero(&a) && a.sign == 0, "zero is canonical");
  fb_set_i64(&a, INT64_MIN);
  bcheck(beq(&a, "-9223372036854775808"), "INT64_MIN round-trips");
  fb_set_i64(&a, INT64_MAX);
  bcheck(beq(&a, "9223372036854775807"), "INT64_MAX round-trips");

  /* Carry all the way across: (2^64 - 1)^2 = 2^128 - 2^65 + 1. */
  fb_parse(&a, "18446744073709551615");
  fb_mul(&r, &a, &a);
  bcheck(beq(&r, "340282366920938463426481119284349108225"),
         "(2^64-1)^2, carry propagates past the inner loop");

  /* Aliasing: r is also the operand. */
  fb_parse(&a, "123456789012345678901234567890");
  fb_copy(&r, &a);
  fb_mul(&r, &r, &r);
  bcheck(beq(&r, "15241578753238836750495351562536198787501905199875019052100"),
         "multiply aliasing the destination");

  /* Subtraction crossing zero, both directions. */
  fb_parse(&a, "1000000000000000000000");
  fb_parse(&b, "1000000000000000000001");
  fb_sub(&r, &a, &b);
  bcheck(beq(&r, "-1"), "subtraction crossing zero");
  fb_sub(&r, &b, &a);
  bcheck(beq(&r, "1"), "subtraction the other way");
  fb_sub(&r, &a, &a);
  bcheck(beq(&r, "0") && r.sign == 0, "x - x is canonical zero");

  /* Shifts must agree with multiplication by a power of two. */
  fb_parse(&a, "987654321098765432109876543210");
  fb_shl(&r, &a, 97);
  fb_set_i64(&b, 1);
  fb_shl(&b, &b, 97);
  fb_mul(&a, &a, &b);
  bcheck(fb_cmp(&r, &a) == 0, "shl 97 equals multiply by 2^97");
  fb_shr(&r, &r, 97);
  fb_parse(&b, "987654321098765432109876543210");
  bcheck(fb_cmp(&r, &b) == 0, "shr undoes shl");

  /* Division: a == q*b + rem, with rem carrying a's sign. */
  fb_parse(&a, "-123456789012345678901234567890");
  fb_parse(&b, "9876543210987");
  {
    fb_int quo, rem, back;
    fb_init(&quo); fb_init(&rem); fb_init(&back);
    fb_divmod(&quo, &rem, &a, &b);
    fb_mul(&back, &quo, &b);
    fb_add(&back, &back, &rem);
    bcheck(fb_cmp(&back, &a) == 0, "divmod reconstructs the dividend");
    bcheck(rem.sign <= 0, "remainder carries the dividend's sign");
    bcheck(fb_cmp_abs(&rem, &b) < 0, "remainder is smaller than the divisor");
    fb_free(&quo); fb_free(&rem); fb_free(&back);
  }

  /* gcd, including the case where one side divides the other. */
  fb_parse(&a, "123456789012345678901234567890");
  fb_parse(&b, "9876543210");
  fb_gcd(&r, &a, &b);
  bcheck(beq(&r, "90"), "gcd of two large integers");
  fb_gcd(&r, &a, &a);
  bcheck(fb_cmp(&r, &a) == 0, "gcd(x, x) = x");
  fb_set_i64(&b, 0);
  fb_gcd(&r, &a, &b);
  bcheck(fb_cmp(&r, &a) == 0, "gcd(x, 0) = x");

  /* Rationals reduce, and stay reduced. */
  fb_rat_set_i64(&p, 6, 8);
  bcheck(bqeq(&p, "3/4"), "6/8 reduces to 3/4");
  fb_rat_set_i64(&p, 4, -8);
  bcheck(bqeq(&p, "-1/2"), "a negative denominator moves to the numerator");
  fb_rat_set_i64(&p, 0, -7);
  bcheck(bqeq(&p, "0"), "zero normalizes to 0/1");
  fb_rat_set_i64(&p, 1, 3);
  fb_rat_set_i64(&q, 1, 6);
  fb_rat_add(&s, &p, &q);
  bcheck(bqeq(&s, "1/2"), "1/3 + 1/6 = 1/2");
  fb_rat_sub(&s, &p, &q);
  bcheck(bqeq(&s, "1/6"), "1/3 - 1/6 = 1/6");
  fb_rat_mul(&s, &p, &q);
  bcheck(bqeq(&s, "1/18"), "1/3 * 1/6 = 1/18");

  /* Parsing: exact decimals and fractions. */
  bcheck(fb_rat_parse(&p, "-0.3125") && bqeq(&p, "-5/16"), "-0.3125 = -5/16");
  bcheck(fb_rat_parse(&p, "3/4") && bqeq(&p, "3/4"), "3/4 parses");
  bcheck(fb_rat_parse(&p, "-2") && bqeq(&p, "-2"), "a bare integer parses");
  bcheck(!fb_rat_parse(&p, "3/0"), "a zero denominator is refused");
  bcheck(!fb_parse(&a, "12x"), "trailing junk is refused");

  /* Comparison never divides, so it must be exact at any size. */
  fb_rat_parse(&p, "1/3");
  fb_rat_parse(&q, "100000000000000000000/299999999999999999999");
  bcheck(fb_rat_cmp(&p, &q) < 0, "1/3 < 1e20/(3e20-1), by cross-multiplication");
  fb_rat_set_i64(&p, 9, 2);
  bcheck(fb_rat_cmp_i64(&p, 4) > 0 && fb_rat_cmp_i64(&p, 5) < 0, "9/2 sits between 4 and 5");

  /* The fixed-point projection, and its refusal. */
  {
    int64_t v = 0;
    fb_rat_set_i64(&p, -1, 4);
    bcheck(fb_rat_to_fixed(&p.num, &p.den, 32, &v) && v == -(1LL << 30),
           "-1/4 in Q32");
    fb_rat_parse(&p, "100000000000/1");
    bcheck(!fb_rat_to_fixed(&p.num, &p.den, 32, &v),
           "a value too wide for Q32 is refused, not saturated");
  }

  /* The budget refuses rather than truncates, and the value survives intact. */
  {
    size_t saved = fb_budget_get();
    fb_int big;
    bool grew = false;
    fb_init(&big);
    fb_parse(&big, "123456789");
    fb_budget_set(fb_budget_used() + 2);
    for (i = 0; i < 64; ++i)
      if (!fb_mul(&big, &big, &big)) break;
    grew = (i < 64);
    bcheck(grew, "the budget eventually refuses a squaring");
    bcheck(!fb_is_zero(&big), "a refused operation leaves the operand intact");
    fb_budget_set(saved);
    fb_free(&big);
  }

  /* An exact orbit: c = -1 is period 2 (0, -1, 0, -1, ...), so it is bounded
   * forever and its values never grow. If the rational path had any drift at
   * all, this is where it would show. */
  {
    fb_rat cr, ci;
    int steps = 0, bits = 0, res;
    fb_rat_init(&cr); fb_rat_init(&ci);
    fb_rat_set_i64(&cr, -1, 1);
    fb_rat_set_i64(&ci, 0, 1);
    res = canor_demo_escape_exact(&cr, &ci, 200, &steps, &bits);
    bcheck(res == 0 && steps == 200, "c = -1 stays bounded for 200 exact steps");
    bcheck(bits <= 2, "and its orbit never grows past a couple of bits");
    fb_rat_set_i64(&cr, 1, 1);
    res = canor_demo_escape_exact(&cr, &ci, 200, &steps, &bits);
    bcheck(res == 1, "c = 1 escapes");
    fb_rat_free(&cr); fb_rat_free(&ci);
  }

  fb_free(&a); fb_free(&b); fb_free(&r);
  fb_rat_free(&p); fb_rat_free(&q); fb_rat_free(&s);

  printf("\n%d checks, %d failures\n", bchecks, bfails);
  return bfails ? 1 : 0;
}
