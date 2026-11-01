#ifndef CANOR_DIM_H
#define CANOR_DIM_H

/* Unital pairs: every symbolic quantity carries a (value, dimension).
 *
 * WHY THIS EXISTS, CONCRETELY. Two real errors in this codebase were dimension
 * errors that no physical unit system would have caught, because both operands
 * were "dimensionless numbers":
 *
 *   1. The Kapprox ladder subtracted a log-of-log from a log2 and the ladder
 *      appeared to invert. Both sides were bare int64 milli-units.
 *   2. L_m did the same thing again - order_milli - fact_milli - months later,
 *      in a different file. Same shape, same silence.
 *
 * So the dimension carried here is not just T/N/B. It is:
 *
 *   LOG LEVEL   0 = the quantity itself, 1 = its log, 2 = log of its log.
 *               log2(x) and log2(log2(x)) are both dimensionless and are NOT
 *               interchangeable, and that distinction is the one that was
 *               missed twice.
 *
 *   SCALE       the power of ten the value is expressed in. milli is -3.
 *               Mixing milli-units with unit-units is the other silent error
 *               this system is prone to, since both are int64.
 *
 * plus the ordinary base quantities, which the latency gradient needs:
 *
 *   TIME        milliseconds
 *   COUNT       cardinality: steps, samples, points
 *   BIT         information
 *
 * THE UNIVERSAL LATENCY GRADIENT is then TIME^1 . COUNT^-1 - milliseconds per
 * step - and a checker that knows this cannot silently add it to a raw latency
 * or to a jitter coefficient (which is COUNT^0 TIME^0, a pure ratio).
 *
 * ADDITION IS THE CHECKED OPERATION. Multiplication combines dimensions and is
 * always meaningful; addition requires them to be IDENTICAL, and that is where
 * both real errors lived. A mismatch produces the poison dimension, which
 * propagates exactly like the rational poison already does - so a caller that
 * ignores it gets a value that fails every subsequent check rather than a
 * plausible number.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int8_t time;      /* seconds; ms is the same dimension at scale -3 */
  int8_t length;    /* metres */
  int8_t count;     /* steps, samples, points */
  int8_t bit;       /* information */
  int8_t log;       /* 0 linear, 1 log, 2 log-log */
  int8_t scale;     /* power of ten: -3 is milli */
  bool poison;      /* set by any illegal combination; propagates */
} canor_dim;

canor_dim canor_dim_none(void);                       /* pure number */
canor_dim canor_dim_make(int8_t t, int8_t n, int8_t b, int8_t log, int8_t scale);
canor_dim canor_dim_make_l(int8_t t, int8_t l, int8_t n, int8_t b,
                     int8_t log, int8_t scale);
bool canor_dim_equal(canor_dim a, canor_dim b);
bool canor_dim_ok(canor_dim d);

/* Named dimensions this system actually uses. */
canor_dim canor_dim_ms(void);            /* latency */
canor_dim canor_dim_ms_per_step(void);   /* THE universal latency gradient */
canor_dim canor_dim_ratio(void);         /* jitter: a pure ratio, T^0 N^0 */
canor_dim canor_dim_bits(void);
canor_dim canor_dim_log2(void);          /* log2 of a count, in units */
canor_dim canor_dim_log2_milli(void);    /* log2, expressed in milli-units */
canor_dim canor_dim_order_milli(void);   /* log2 OF log2, in milli-units */
canor_dim canor_dim_metre(void);
canor_dim canor_dim_second(void);
canor_dim canor_dim_velocity(void);      /* m.s^-1 */

/* --- physical constants, as a convention ------------------------------------
 * c IS EXACT, AND NOT BY APPROXIMATION. Since 1983 the metre is DEFINED as the
 * distance light travels in 1/299792458 of a second, so c = 299792458 m/s is a
 * definition rather than a measurement - there is no uncertainty to carry and
 * no digits to round. That makes it the one physical constant that belongs in
 * an exact-rational core without apology.
 *
 * It is stored with its dimension attached, so the checker refuses the obvious
 * mistakes: adding c to a latency, or to a jitter ratio, or treating a
 * light-distance as a time.
 *
 * SCALE MATTERS HERE. This system measures latency in MILLISECONDS, and c is
 * quoted per SECOND. Those are the same dimension at different scales, and
 * canor_dim already refuses to add them - which is exactly the protection wanted,
 * since a factor of 1000 in a light-distance is the kind of error that looks
 * like a plausible answer.
 *
 * FUTURE SCOPE, stated so the boundary is visible. A Bekenstein-style
 * derivation - bounding information by energy and radius - would need this
 * core to carry energy and action as well, and the bound
 *
 *     I  <=  2.pi.R.E / (hbar . c . ln2)
 *
 * is dimensionally BIT = LENGTH . ENERGY / (ACTION . VELOCITY). The `bit`
 * dimension already here was added for the Golay work and turns out to be
 * exactly what such a derivation would need, so the dimensional groundwork
 * exists. What does NOT exist is any measurement path: deriving c from
 * timing and telemetry needs a known baseline distance between two clocks and
 * a synchronisation whose error is smaller than the light travel time, and
 * nothing in this system currently measures distance at all. The constant
 * below is a CONVENTION, and no telemetry here has yet earned the right to
 * check it. */


/* Addition and subtraction REQUIRE identical dimensions. This is the check the
 * whole header exists for; a mismatch yields poison rather than a sum. */
canor_dim canor_dim_add(canor_dim a, canor_dim b);
canor_dim canor_dim_mul(canor_dim a, canor_dim b);
canor_dim canor_dim_div(canor_dim a, canor_dim b);
/* Taking a log raises the log level by one and clears the base quantities:
 * log of a dimensional quantity is only defined against a reference, and
 * silently keeping T^1 under a logarithm is how log-level errors start. */
canor_dim canor_dim_take_log(canor_dim a);
/* Rescaling by a power of ten, e.g. unit -> milli is canor_dim_rescale(d, -3). */
canor_dim canor_dim_rescale(canor_dim a, int8_t by);

/* "ms.step^-1", "log2(count)/milli", "POISON", ... */
int canor_dim_format(canor_dim d, char *out, int cap);

/* A value with its dimension. The pair is the unit of currency here: a bare
 * int64 is exactly what allowed both historical errors. */
typedef struct {
  int64_t value;
  canor_dim dim;
} canor_quantity;

canor_quantity canor_q(int64_t v, canor_dim d);
canor_quantity canor_q_add(canor_quantity a, canor_quantity b);
canor_quantity canor_q_sub(canor_quantity a, canor_quantity b);
canor_quantity canor_q_mul(canor_quantity a, canor_quantity b);
bool canor_q_ok(canor_quantity q);

canor_quantity canor_const_c(void);          /* 299792458 m/s, exact by definition */
canor_quantity canor_const_c_per_ms(void);   /* the same constant, per millisecond */

#ifdef __cplusplus
}
#endif
#endif /* CANOR_DIM_H */
