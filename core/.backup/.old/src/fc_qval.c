/* Quantum values: tagged {lo, val, hi}. See fc_qval.h. */

#include "fc_qval.h"

#include <stdio.h>
#include <string.h>

const char *fc_qv_kind_name(fc_qv_kind k) {
  return k == FC_QV_STATISTICAL ? "STATISTICAL" : "EXACT";
}

fc_qv_kind fc_qv_join(fc_qv_kind a, fc_qv_kind b) {
  /* Contagious upward and never promoted: a combination of a measured quantity
   * with a proved one is measured. The asymmetry is the whole point - there is
   * no operation that turns confidence into certainty. */
  return (a == FC_QV_STATISTICAL || b == FC_QV_STATISTICAL)
         ? FC_QV_STATISTICAL : FC_QV_EXACT;
}

static bool iv_set(fc_qval *o, fc_rational lo, fc_surreal val,
                   fc_rational hi, fc_qv_kind k, int32_t conf) {
  fc_surreal slo, shi;
  if (!o) return false;
  memset(o, 0, sizeof *o);
  o->valid = false;
  if (!fc_rat_valid(lo) || !fc_rat_valid(hi)) return false;
  if (!fc_surreal_valid(&val)) return false;
  if (fc_rat_cmp(lo, hi) > 0) return false;       /* an inverted bound */
  if (!fc_surreal_real(&slo, lo) || !fc_surreal_real(&shi, hi)) return false;
  /* THE INVARIANT, CHECKED. A val outside its bound is refused, not clamped.
   * Note this correctly rejects a val carrying epsilon against a bound tight
   * enough to exclude it: that says the declared bound cannot hold the
   * sub-resolution mass it was handed, which is a defect in the bound. */
  if (fc_surreal_cmp(&val, &slo) < 0) return false;
  if (fc_surreal_cmp(&val, &shi) > 0) return false;
  o->lo = lo;
  o->hi = hi;
  o->val = val;
  o->kind = k;
  o->confidence = (k == FC_QV_STATISTICAL) ? conf : 1000;
  o->valid = true;
  return true;
}

bool fc_qv_exact(fc_qval *o, fc_rational lo, fc_surreal val,
                 fc_rational hi) {
  return iv_set(o, lo, val, hi, FC_QV_EXACT, 1000);
}

bool fc_qv_stat(fc_qval *o, fc_rational lo, fc_surreal val, fc_rational hi,
                int32_t conf) {
  if (conf < 0 || conf > 1000) return false;
  return iv_set(o, lo, val, hi, FC_QV_STATISTICAL, conf);
}

bool fc_qv_point(fc_qval *o, fc_rational v) {
  fc_surreal s;
  if (!fc_surreal_real(&s, v)) return false;
  return iv_set(o, v, s, v, FC_QV_EXACT, 1000);
}

bool fc_qv_valid(const fc_qval *x) { return x && x->valid; }

bool fc_qv_contains_val(const fc_qval *x) {
  fc_surreal slo, shi;
  if (!fc_qv_valid(x)) return false;
  if (!fc_surreal_real(&slo, x->lo) || !fc_surreal_real(&shi, x->hi))
    return false;
  return fc_surreal_cmp(&x->val, &slo) >= 0 &&
         fc_surreal_cmp(&x->val, &shi) <= 0;
}

fc_rational fc_qv_width(const fc_qval *x) {
  if (!fc_qv_valid(x)) return fc_rat(0, 0);
  return fc_rat_add(x->hi, fc_rat(-x->lo.num, x->lo.den));
}

/* The narrower confidence of two statistical operands: a composite is no more
 * confident than its weakest input, and reporting otherwise would manufacture
 * certainty from a join. */
static int32_t iv_conf(const fc_qval *a, const fc_qval *b) {
  int32_t ca = a->kind == FC_QV_STATISTICAL ? a->confidence : 1000;
  int32_t cb = b->kind == FC_QV_STATISTICAL ? b->confidence : 1000;
  return ca < cb ? ca : cb;
}

bool fc_qv_add(const fc_qval *a, const fc_qval *b, fc_qval *o) {
  fc_surreal v;
  if (!fc_qv_valid(a) || !fc_qv_valid(b) || !o) return false;
  if (!fc_surreal_add(&a->val, &b->val, &v)) return false;
  return iv_set(o, fc_rat_add(a->lo, b->lo), v, fc_rat_add(a->hi, b->hi),
                fc_qv_join(a->kind, b->kind), iv_conf(a, b));
}

bool fc_qv_sub(const fc_qval *a, const fc_qval *b, fc_qval *o) {
  fc_surreal v;
  if (!fc_qv_valid(a) || !fc_qv_valid(b) || !o) return false;
  if (!fc_surreal_sub(&a->val, &b->val, &v)) return false;
  /* [a,b] - [c,d] = [a-d, b-c]. The crossed subtraction is not a typo: the
   * smallest difference takes the largest subtrahend. */
  return iv_set(o, fc_rat_add(a->lo, fc_rat(-b->hi.num, b->hi.den)), v,
                fc_rat_add(a->hi, fc_rat(-b->lo.num, b->lo.den)),
                fc_qv_join(a->kind, b->kind), iv_conf(a, b));
}

bool fc_qv_mul(const fc_qval *a, const fc_qval *b, fc_qval *o) {
  fc_rational c[4], lo, hi;
  fc_surreal v;
  int i;
  if (!fc_qv_valid(a) || !fc_qv_valid(b) || !o) return false;
  /* All four corner products, because sign changes move which corner is
   * extremal - taking lo*lo and hi*hi would be wrong across zero. */
  c[0] = fc_rat_mul(a->lo, b->lo);
  c[1] = fc_rat_mul(a->lo, b->hi);
  c[2] = fc_rat_mul(a->hi, b->lo);
  c[3] = fc_rat_mul(a->hi, b->hi);
  for (i = 0; i < 4; ++i) if (!fc_rat_valid(c[i])) return false;
  lo = hi = c[0];
  for (i = 1; i < 4; ++i) {
    if (fc_rat_cmp(c[i], lo) < 0) lo = c[i];
    if (fc_rat_cmp(c[i], hi) > 0) hi = c[i];
  }
  if (!fc_surreal_mul(&a->val, &b->val, &v)) return false;
  return iv_set(o, lo, v, hi, fc_qv_join(a->kind, b->kind), iv_conf(a, b));
}

void fc_qv_format(const fc_qval *x, char *buf, int cap) {
  char v[96];
  if (!buf || cap < 16) return;
  buf[0] = 0;
  if (!fc_qv_valid(x)) { snprintf(buf, (size_t)cap, "<invalid>"); return; }
  fc_surreal_format(&x->val, v, sizeof v);
  if (x->kind == FC_QV_STATISTICAL)
    snprintf(buf, (size_t)cap, "{%lld/%lld, %s, %lld/%lld} STAT c=%d",
             (long long)x->lo.num, (long long)x->lo.den, v,
             (long long)x->hi.num, (long long)x->hi.den, x->confidence);
  else
    snprintf(buf, (size_t)cap, "{%lld/%lld, %s, %lld/%lld} EXACT",
             (long long)x->lo.num, (long long)x->lo.den, v,
             (long long)x->hi.num, (long long)x->hi.den);
}
