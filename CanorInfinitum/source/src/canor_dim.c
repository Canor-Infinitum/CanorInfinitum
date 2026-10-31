/* Unital pairs. See canor_dim.h for the two real errors this exists to catch. */

#include "canor_dim.h"

#include <stdio.h>
#include <string.h>

canor_dim canor_dim_none(void) {
  canor_dim d;
  memset(&d, 0, sizeof d);
  return d;
}

canor_dim canor_dim_make(int8_t t, int8_t n, int8_t b, int8_t log, int8_t scale) {
  canor_dim d = canor_dim_none();
  d.time = t; d.count = n; d.bit = b; d.log = log; d.scale = scale;
  return d;
}

canor_dim canor_dim_make_l(int8_t t, int8_t l, int8_t n, int8_t b,
                     int8_t log, int8_t scale) {
  canor_dim d = canor_dim_make(t, n, b, log, scale);
  d.length = l;
  return d;
}

canor_dim canor_dim_metre(void)   { return canor_dim_make_l(0, 1, 0, 0, 0, 0); }
canor_dim canor_dim_second(void)  { return canor_dim_make_l(1, 0, 0, 0, 0, 0); }
canor_dim canor_dim_velocity(void){ return canor_dim_make_l(-1, 1, 0, 0, 0, 0); }

canor_quantity canor_const_c(void) {
  /* Exact by the SI definition of the metre, not measured. */
  return canor_q(299792458LL, canor_dim_velocity());
}

canor_quantity canor_const_c_per_ms(void) {
  /* Same constant, expressed per millisecond. The VALUE is unchanged and the
   * SCALE carries the difference - which is what keeps the two from being
   * added together by accident. */
  canor_dim d = canor_dim_velocity();
  d.scale = 3;                 /* per 1e-3 s => 1e3 times as many metres */
  return canor_q(299792458LL, d);
}

bool canor_dim_ok(canor_dim d) { return !d.poison; }

bool canor_dim_equal(canor_dim a, canor_dim b) {
  if (a.poison || b.poison) return false;
  return a.time == b.time && a.length == b.length && a.count == b.count &&
         a.bit == b.bit && a.log == b.log && a.scale == b.scale;
}

/* Latency is SECONDS at milli scale - the same dimension as any other
 * duration, which is what lets the checker catch a ms/s confusion. */
canor_dim canor_dim_ms(void)          { return canor_dim_make(1, 0, 0, 0, -3); }
canor_dim canor_dim_ms_per_step(void) { return canor_dim_make(1, -1, 0, 0, -3); }
canor_dim canor_dim_ratio(void)       { return canor_dim_none(); }
canor_dim canor_dim_bits(void)        { return canor_dim_make(0, 0, 1, 0, 0); }
canor_dim canor_dim_log2(void)        { return canor_dim_make(0, 0, 0, 1, 0); }
canor_dim canor_dim_log2_milli(void)  { return canor_dim_make(0, 0, 0, 1, -3); }
canor_dim canor_dim_order_milli(void) { return canor_dim_make(0, 0, 0, 2, -3); }

static canor_dim poisoned(void) {
  canor_dim d = canor_dim_none();
  d.poison = true;
  return d;
}

canor_dim canor_dim_add(canor_dim a, canor_dim b) {
  /* THE CHECK. Both historical errors were an addition (or subtraction) of two
   * dimensionless-looking int64s whose LOG LEVELS differed: a log-of-log minus
   * a log. Requiring exact equality here is what turns that from a silent
   * wrong number into a refusal. */
  if (a.poison || b.poison) return poisoned();
  if (!canor_dim_equal(a, b)) return poisoned();
  return a;
}

canor_dim canor_dim_mul(canor_dim a, canor_dim b) {
  canor_dim d;
  if (a.poison || b.poison) return poisoned();
  /* Multiplying two logs is not a log of anything, so the log level would be
   * meaningless. Products are allowed only when at most one side is logged. */
  if (a.log > 0 && b.log > 0) return poisoned();
  d = canor_dim_none();
  d.time  = (int8_t)(a.time + b.time);
  d.length = (int8_t)(a.length + b.length);
  d.count = (int8_t)(a.count + b.count);
  d.bit   = (int8_t)(a.bit + b.bit);
  d.log   = (int8_t)(a.log + b.log);
  d.scale = (int8_t)(a.scale + b.scale);
  return d;
}

canor_dim canor_dim_div(canor_dim a, canor_dim b) {
  canor_dim inv;
  if (a.poison || b.poison) return poisoned();
  if (b.log > 0 && a.log > 0) return poisoned();
  inv = canor_dim_none();
  inv.time  = (int8_t)-b.time;
  inv.length = (int8_t)-b.length;
  inv.count = (int8_t)-b.count;
  inv.bit   = (int8_t)-b.bit;
  inv.log   = (int8_t)-b.log;
  inv.scale = (int8_t)-b.scale;
  return canor_dim_mul(a, inv);
}

canor_dim canor_dim_take_log(canor_dim a) {
  canor_dim d;
  if (a.poison) return poisoned();
  /* The base quantities are cleared: log of a quantity with units is only
   * defined against a reference, and carrying T^1 under a logarithm is exactly
   * how a log-level error gets started. The LEVEL is what survives. */
  d = canor_dim_none();
  d.log = (int8_t)(a.log + 1);
  d.scale = a.scale;
  return d;
}

canor_dim canor_dim_rescale(canor_dim a, int8_t by) {
  if (a.poison) return poisoned();
  a.scale = (int8_t)(a.scale + by);
  return a;
}

int canor_dim_format(canor_dim d, char *out, int cap) {
  char buf[128];
  int n = 0;
  if (d.poison) return snprintf(out, (size_t)cap, "POISON");
  buf[0] = 0;
  if (d.log == 1) n += snprintf(buf + n, sizeof buf - (size_t)n, "log2 ");
  else if (d.log == 2) n += snprintf(buf + n, sizeof buf - (size_t)n, "log2.log2 ");
  else if (d.log != 0) n += snprintf(buf + n, sizeof buf - (size_t)n, "log^%d ", d.log);
  if (d.time)  n += snprintf(buf + n, sizeof buf - (size_t)n, "s^%d ", d.time);
  if (d.length) n += snprintf(buf + n, sizeof buf - (size_t)n, "m^%d ", d.length);
  if (d.count) n += snprintf(buf + n, sizeof buf - (size_t)n, "step^%d ", d.count);
  if (d.bit)   n += snprintf(buf + n, sizeof buf - (size_t)n, "bit^%d ", d.bit);
  if (d.scale == -3) n += snprintf(buf + n, sizeof buf - (size_t)n, "[milli]");
  else if (d.scale) n += snprintf(buf + n, sizeof buf - (size_t)n, "[1e%d]", d.scale);
  if (!buf[0]) snprintf(buf, sizeof buf, "pure");
  return snprintf(out, (size_t)cap, "%s", buf);
}

canor_quantity canor_q(int64_t v, canor_dim d) {
  canor_quantity q;
  q.value = v;
  q.dim = d;
  return q;
}

bool canor_q_ok(canor_quantity q) { return canor_dim_ok(q.dim); }

canor_quantity canor_q_add(canor_quantity a, canor_quantity b) {
  canor_quantity r;
  r.dim = canor_dim_add(a.dim, b.dim);
  /* The value is still computed so a caller stepping through in a debugger
   * sees what it would have been, but the dimension is poison and every
   * downstream canor_q_ok fails. Refusing to compute would hide the magnitude of
   * the mistake, which is often the clue to where it came from. */
  r.value = a.value + b.value;
  return r;
}

canor_quantity canor_q_sub(canor_quantity a, canor_quantity b) {
  canor_quantity r;
  r.dim = canor_dim_add(a.dim, b.dim);
  r.value = a.value - b.value;
  return r;
}

canor_quantity canor_q_mul(canor_quantity a, canor_quantity b) {
  canor_quantity r;
  r.dim = canor_dim_mul(a.dim, b.dim);
  r.value = a.value * b.value;
  return r;
}
