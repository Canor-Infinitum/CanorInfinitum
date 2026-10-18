#ifndef CANOR_B2I_H
#define CANOR_B2I_H

/* Base 2i - the quater-imaginary system - and a digit filter over it.
 *
 * Digits are {0,1,2,3} and the base is 2i. Knuth's result is that every
 * GAUSSIAN INTEGER has a representation, and it is unique. That is what earns
 * this a place here: it is an exact positional system for complex integers,
 * with no rounding anywhere, so a base-2i digit string is a lossless name for
 * a lattice point rather than a rendering of one.
 *
 * WHY THE PARTS SEPARATE. Powers of the base cycle through the axes:
 *
 *     (2i)^0 = 1      (2i)^1 = 2i     (2i)^2 = -4
 *     (2i)^3 = -8i    (2i)^4 = 16     (2i)^5 = 32i
 *
 * so EVEN positions are real and ODD positions are imaginary, and each axis
 * runs in base -4. That is the whole construction: a real part written in base
 * -4 lands on the even positions, and half the imaginary part written in base
 * -4 lands on the odd ones. Negative bases are what remove the need for a sign
 * - every Gaussian integer, in all four quadrants, is a string of digits
 * 0..3 with no minus anywhere.
 *
 * THE ONE FRACTIONAL CASE, AND WHY IT IS NOT AN EDGE CASE TO PAPER OVER.
 * The odd positions contribute 2i.(...), so they can only reach EVEN imaginary
 * parts. An odd imaginary part needs the position -1, where (2i)^-1 = -i/2 and
 * the digit 2 contributes exactly -i. So a + bi with b odd is written as the
 * representation of a + (b+1)i followed by ".2". This is exact, not an
 * approximation: 1+i is 11.2 and that identity is checked rather than trusted.
 *
 * A finite expansion always exists and the fraction is never longer than one
 * digit, so nothing here can run away.
 *
 * --- THE FILTER -------------------------------------------------------------
 *
 * A filter selects lattice points by the SHAPE of their digit string rather
 * than by their magnitude: which digits may appear, how long the string is,
 * whether it reads the same both ways, whether it contains a given run. Those
 * are properties of the representation, and because the representation is
 * unique they are well-defined properties of the NUMBER - which is the only
 * reason a digit filter is meaningful at all. In a redundant system the same
 * point would pass under one spelling and fail under another.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define B2I_MAX 40

typedef struct {
  uint8_t d[B2I_MAX];   /* d[0] is the units position, ascending */
  int n;                /* integer digits */
  int frac;             /* 0, or 1 when a ".2" tail is present */
  bool valid;
} canor_b2i;

/* Exact. Returns false only if the value needs more than B2I_MAX digits. */
bool canor_b2i_encode(int64_t re, int64_t im, canor_b2i *out);

/* The inverse. Exact, and the suite round-trips it rather than assuming it. */
bool canor_b2i_decode(const canor_b2i *v, int64_t *re, int64_t *im);

/* "11.2" style. Buffer needs B2I_MAX + 4 bytes. */
void canor_b2i_format(const canor_b2i *v, char *buf, int cap);

/* --- the filter ------------------------------------------------------------ */

typedef struct {
  uint8_t allow;        /* bit d set => digit d permitted; 15 = all */
  int min_len, max_len; /* integer-digit count, inclusive; max_len 0 = no cap */
  bool no_frac;         /* reject the ".2" tail, i.e. odd imaginary parts */
  bool palindrome;      /* the integer digits read the same both ways */
  int run_digit;        /* -1 for none */
  int run_len;          /* require a run of `run_digit` at least this long */
} canor_b2i_filter;

void canor_b2i_filter_init(canor_b2i_filter *f);   /* accepts everything */
bool canor_b2i_pass(const canor_b2i *v, const canor_b2i_filter *f);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_B2I_H */
