/* Is periodicity really governed by `a` alone, with any q != 1 aperiodic?
 *
 * The recurrence C(n+1) = a.C(n) - q.C(n-1) has characteristic polynomial
 * x^2 - a.x + q, whose roots multiply to q. Both roots lie on the unit circle
 * - the condition for a bounded, potentially periodic orbit - when |q| = 1 and
 * |a| <= 2. So q = -1 should admit periodic operators just as q = +1 does.
 *
 * Rather than argue, run the recurrence and look for the STATE (C(n), C(n+1))
 * repeating. The values are exact rationals, so a repeated state means genuine
 * periodicity, not a numerical coincidence.
 */
#include "fc.h"
#include <stdio.h>

static int detect_period(long a_num, long a_den, long q_num, long q_den,
                         int maxn, int *bounded, long *maxbits) {
  fc_rational a = fc_rat(a_num, a_den), q = fc_rat(q_num, q_den);
  fc_rational prev = fc_rat(1, 1);                  /* C(0) */
  fc_rational cur = fc_rat_mul(a, fc_rat(1, 2));    /* C(1) = a/2 */
  fc_rational p0 = prev, p1 = cur;
  int n;
  *bounded = 1;
  *maxbits = 0;
  for (n = 1; n <= maxn; ++n) {
    fc_rational t1 = fc_rat_mul(a, cur);
    fc_rational t2 = fc_rat_mul(q, prev);
    fc_rational nx;
    long bits = 0, v;
    if (!fc_rat_valid(t1) || !fc_rat_valid(t2)) { *bounded = 0; return 0; }
    t2.num = -t2.num;
    nx = fc_rat_add(t1, t2);
    if (!fc_rat_valid(nx)) { *bounded = 0; return 0; }
    prev = cur;
    cur = nx;
    v = cur.den > (cur.num < 0 ? -cur.num : cur.num)
        ? cur.den : (cur.num < 0 ? -cur.num : cur.num);
    while (v) { ++bits; v >>= 1; }
    if (bits > *maxbits) *maxbits = bits;
    if (fc_rat_eq(prev, p0) && fc_rat_eq(cur, p1)) return n;  /* state repeated */
  }
  return 0;
}

int main(void) {
  static const long as[] = { 2, 1, 0, -1, -2 };
  static const long qs[] = { 1, -1, 2 };
  int ai, qi;
  printf("period detected by state repetition, not by a lookup table\n\n");
  printf("  %4s %4s %10s %10s %10s\n", "a", "q", "period", "bounded", "maxbits");
  for (qi = 0; qi < 3; ++qi)
    for (ai = 0; ai < 5; ++ai) {
      int bounded = 0;
      long mb = 0;
      int p = detect_period(as[ai], 1, qs[qi], 1, 400, &bounded, &mb);
      printf("  %4ld %4ld %10s %10s %10ld\n", as[ai], qs[qi],
             p ? "" : "none", bounded ? "yes" : "NO", mb);
      if (p) printf("\033[1A\033[14C%6d\033[B\r", p);
    }
  printf("\n  a=1/2 (Niven: irrational angle) as a control:\n");
  {
    int bounded = 0;
    long mb = 0;
    int p = detect_period(1, 2, 1, 1, 400, &bounded, &mb);
    printf("    period %s, bounded %s, maxbits %ld\n",
           p ? "found" : "none", bounded ? "yes" : "NO", mb);
  }
  return 0;
}
