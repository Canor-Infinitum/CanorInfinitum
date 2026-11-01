/* Kapprox ladder, and the ijk->RGB projection at min-6.
 *
 * Checks the two declared properties rather than describing them: that the
 * ladder separates only from n=3, and that it is strictly increasing across
 * the whole declared topology range 6..96.
 */

#include "fc.h"

#include <stdio.h>

static int checks = 0, failures = 0;
static void check(int c, const char *w) {
  ++checks;
  if (!c) { ++failures; printf("  FAIL  %s\n", w); }
}

int main(void) {
  printf("Kapprox ladder, log2 in milli-bits, 2n! read as 2*(n!)\n\n");
  printf("   n  %14s %14s %14s %14s  separated\n",
         "General", "Specific", "Distinctive", "Arbitrary*");
  const uint64_t ns[] = { 1, 2, 3, 4, 6, 12, 20, 48, 96 };
  for (size_t i = 0; i < sizeof ns / sizeof ns[0]; ++i) {
    uint64_t n = ns[i];
    printf("  %3llu  %14lld %14lld %14lld %14lld  %s\n",
           (unsigned long long)n,
           (long long)fc_kapprox_order_milli(FC_K_GENERAL, n),
           (long long)fc_kapprox_order_milli(FC_K_SPECIFIC, n),
           (long long)fc_kapprox_order_milli(FC_K_DISTINCTIVE, n),
           (long long)fc_kapprox_order_milli(FC_K_ARBITRARY, n),
           fc_kapprox_separated(n) ? "yes" : "NO");
  }
  printf("\n  * Arbitrary is reported as an ORDER - log2 of the log2 - because\n"
         "    log2((2(n!)+1)!) is about 1e152 at n=96 and is not a quantity a\n"
         "    caller can compute with. The class stays comparable; the\n"
         "    precision it does not have is not claimed.\n\n");

  /* The declared collapse below n=3. */
  check(!fc_kapprox_separated(1), "n=1 does NOT separate (3, 3, 6, 6)");
  check(!fc_kapprox_separated(2), "n=2 does NOT separate (5, 5, 120, 120)");
  check(fc_kapprox_separated(3), "n=3 separates");

  /* Strictly increasing across the whole declared topology range. */
  int all = 1;
  for (uint64_t n = FC_LEVELS_MIN; n <= FC_LEVELS_MAX; ++n)
    if (!fc_kapprox_separated(n)) all = 0;
  check(all, "separated for every level 6..96, the declared topology range");

  /* Monotone in n within each class - a description-length ladder that fell
   * as the space grew would not be one. */
  int mono = 1;
  for (int c = 0; c < 4; ++c)
    for (uint64_t n = FC_LEVELS_MIN; n < FC_LEVELS_MAX; ++n)
      if (fc_kapprox_order_milli((fc_kclass)c, n + 1) <
          fc_kapprox_order_milli((fc_kclass)c, n)) mono = 0;
  check(mono, "each class is monotone in n");

  /* Spot-check the fixed-point log2 against values known exactly. */
  check(fc_log2_milli(1) == 0, "log2(1) = 0");
  check(fc_log2_milli(2) == 1000, "log2(2) = 1");
  check(fc_log2_milli(1024) == 10000, "log2(1024) = 10");
  int64_t l3 = fc_log2_milli(3);
  check(l3 >= 1580 && l3 <= 1590, "log2(3) is 1.585 to three places");
  int64_t l10 = fc_log2_milli(10);
  check(l10 >= 3315 && l10 <= 3325, "log2(10) is 3.322");
  /* log2(6!) = log2(720) = 9.492 */
  int64_t f6 = fc_log2_factorial_milli(6);
  check(f6 >= 9480 && f6 <= 9500, "log2(6!) = 9.49");

  printf("\n%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
