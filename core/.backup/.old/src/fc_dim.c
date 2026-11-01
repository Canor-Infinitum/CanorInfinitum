/* Unital pairs. See fc_dim.h for the two real errors this exists to catch. */

#include "fc_dim.h"

#include <stdio.h>
#include <string.h>

fc_dim fc_dim_none(void) {
  fc_dim d;
  memset(&d, 0, sizeof d);
  return d;
}

fc_dim fc_dim_make(int8_t t, int8_t n, int8_t b, int8_t log, int8_t scale) {
  fc_dim d = fc_dim_none();
  d.time = t; d.count = n; d.bit = b; d.log = log; d.scale = scale;
  return d;
}

fc_dim fc_dim_make_l(int8_t t, int8_t l, int8_t n, int8_t b,
                     int8_t log, int8_t scale) {
  fc_dim d = fc_dim_make(t, n, b, log, scale);
  d.length = l;
  return d;
}

fc_dim fc_dim_metre(void)   { return fc_dim_make_l(0, 1, 0, 0, 0, 0); }
fc_dim fc_dim_second(void)  { return fc_dim_make_l(1, 0, 0, 0, 0, 0); }
fc_dim fc_dim_velocity(void){ return fc_dim_make_l(-1, 1, 0, 0, 0, 0); }

fc_quantity fc_const_c(void) {
  /* Exact by the SI definition of the metre, not measured. */
  return fc_q(299792458LL, fc_dim_velocity());
}

fc_quantity fc_const_c_per_ms(void) {
  /* Same constant, expressed per millisecond. The VALUE is unchanged and the
   * SCALE carries the difference - which is what keeps the two from being
   * added together by accident. */
  fc_dim d = fc_dim_velocity();
  d.scale = 3;                 /* per 1e-3 s => 1e3 times as many metres */
  return fc_q(299792458LL, d);
}

bool fc_dim_ok(fc_dim d) { return !d.poison; }

bool fc_dim_equal(fc_dim a, fc_dim b) {
  if (a.poison || b.poison) return false;
  return a.time == b.time && a.length == b.length && a.count == b.count &&
         a.bit == b.bit && a.log == b.log && a.scale == b.scale;
}

/* Latency is SECONDS at milli scale - the same dimension as any other
 * duration, which is what lets the checker catch a ms/s confusion. */
fc_dim fc_dim_ms(void)          { return fc_dim_make(1, 0, 0, 0, -3); }
fc_dim fc_dim_ms_per_step(void) { return fc_dim_make(1, -1, 0, 0, -3); }
fc_dim fc_dim_ratio(void)       { return fc_dim_none(); }
fc_dim fc_dim_bits(void)        { return fc_dim_make(0, 0, 1, 0, 0); }
fc_dim fc_dim_log2(void)        { return fc_dim_make(0, 0, 0, 1, 0); }
fc_dim fc_dim_log2_milli(void)  { return fc_dim_make(0, 0, 0, 1, -3); }
fc_dim fc_dim_order_milli(void) { return fc_dim_make(0, 0, 0, 2, -3); }

static fc_dim poisoned(void) {
  fc_dim d = fc_dim_none();
  d.poison = true;
  return d;
}

fc_dim fc_dim_add(fc_dim a, fc_dim b) {
  /* THE CHECK. Both historical errors were an addition (or subtraction) of two
   * dimensionless-looking int64s whose LOG LEVELS differed: a log-of-log minus
   * a log. Requiring exact equality here is what turns that from a silent
   * wrong number into a refusal. */
  if (a.poison || b.poison) return poisoned();
  if (!fc_dim_equal(a, b)) return poisoned();
  return a;
}

fc_dim fc_dim_mul(fc_dim a, fc_dim b) {
  fc_dim d;
  if (a.poison || b.poison) return poisoned();
  /* Multiplying two logs is not a log of anything, so the log level would be
   * meaningless. Products are allowed only when at most one side is logged. */
  if (a.log > 0 && b.log > 0) return poisoned();
  d = fc_dim_none();
  d.time  = (int8_t)(a.time + b.time);
  d.length = (int8_t)(a.length + b.length);
  d.count = (int8_t)(a.count + b.count);
  d.bit   = (int8_t)(a.bit + b.bit);
  d.log   = (int8_t)(a.log + b.log);
  d.scale = (int8_t)(a.scale + b.scale);
  return d;
}

fc_dim fc_dim_div(fc_dim a, fc_dim b) {
  fc_dim inv;
  if (a.poison || b.poison) return poisoned();
  if (b.log > 0 && a.log > 0) return poisoned();
  inv = fc_dim_none();
  inv.time  = (int8_t)-b.time;
  inv.length = (int8_t)-b.length;
  inv.count = (int8_t)-b.count;
  inv.bit   = (int8_t)-b.bit;
  inv.log   = (int8_t)-b.log;
  inv.scale = (int8_t)-b.scale;
  return fc_dim_mul(a, inv);
}

fc_dim fc_dim_take_log(fc_dim a) {
  fc_dim d;
  if (a.poison) return poisoned();
  /* The base quantities are cleared: log of a quantity with units is only
   * defined against a reference, and carrying T^1 under a logarithm is exactly
   * how a log-level error gets started. The LEVEL is what survives. */
  d = fc_dim_none();
  d.log = (int8_t)(a.log + 1);
  d.scale = a.scale;
  return d;
}

fc_dim fc_dim_rescale(fc_dim a, int8_t by) {
  if (a.poison) return poisoned();
  a.scale = (int8_t)(a.scale + by);
  return a;
}

int fc_dim_format(fc_dim d, char *out, int cap) {
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

fc_quantity fc_q(int64_t v, fc_dim d) {
  fc_quantity q;
  q.value = v;
  q.dim = d;
  return q;
}

bool fc_q_ok(fc_quantity q) { return fc_dim_ok(q.dim); }

fc_quantity fc_q_add(fc_quantity a, fc_quantity b) {
  fc_quantity r;
  r.dim = fc_dim_add(a.dim, b.dim);
  /* The value is still computed so a caller stepping through in a debugger
   * sees what it would have been, but the dimension is poison and every
   * downstream fc_q_ok fails. Refusing to compute would hide the magnitude of
   * the mistake, which is often the clue to where it came from. */
  r.value = a.value + b.value;
  return r;
}

fc_quantity fc_q_sub(fc_quantity a, fc_quantity b) {
  fc_quantity r;
  r.dim = fc_dim_add(a.dim, b.dim);
  r.value = a.value - b.value;
  return r;
}

fc_quantity fc_q_mul(fc_quantity a, fc_quantity b) {
  fc_quantity r;
  r.dim = fc_dim_mul(a.dim, b.dim);
  r.value = a.value * b.value;
  return r;
}
