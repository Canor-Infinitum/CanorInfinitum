/* Quantum values: tagged {lo, val, hi}. See canor_qval.h. */

#include "canor_qval.h"

#include <stdio.h>
#include <string.h>

const char *canor_qv_kind_name(canor_qv_kind k) {
  return k == CANOR_QV_STATISTICAL ? "STATISTICAL" : "EXACT";
}

canor_qv_kind canor_qv_join(canor_qv_kind a, canor_qv_kind b) {
  /* Contagious upward and never promoted: a combination of a measured quantity
   * with a proved one is measured. The asymmetry is the whole point - there is
   * no operation that turns confidence into certainty. */
  return (a == CANOR_QV_STATISTICAL || b == CANOR_QV_STATISTICAL)
         ? CANOR_QV_STATISTICAL : CANOR_QV_EXACT;
}

static bool iv_set(canor_qval *o, canor_rational lo, canor_surreal val,
                   canor_rational hi, canor_qv_kind k, int32_t conf) {
  canor_surreal slo, shi;
  if (!o) return false;
  memset(o, 0, sizeof *o);
  o->valid = false;
  if (!canor_rat_valid(lo) || !canor_rat_valid(hi)) return false;
  if (!canor_surreal_valid(&val)) return false;
  if (canor_rat_cmp(lo, hi) > 0) return false;       /* an inverted bound */
  if (!canor_surreal_real(&slo, lo) || !canor_surreal_real(&shi, hi)) return false;
  /* THE INVARIANT, CHECKED. A val outside its bound is refused, not clamped.
   * Note this correctly rejects a val carrying epsilon against a bound tight
   * enough to exclude it: that says the declared bound cannot hold the
   * sub-resolution mass it was handed, which is a defect in the bound. */
  if (canor_surreal_cmp(&val, &slo) < 0) return false;
  if (canor_surreal_cmp(&val, &shi) > 0) return false;
  o->lo = lo;
  o->hi = hi;
  o->val = val;
  o->kind = k;
  o->confidence = (k == CANOR_QV_STATISTICAL) ? conf : 1000;
  o->valid = true;
  return true;
}

bool canor_qv_exact(canor_qval *o, canor_rational lo, canor_surreal val,
                 canor_rational hi) {
  return iv_set(o, lo, val, hi, CANOR_QV_EXACT, 1000);
}

bool canor_qv_stat(canor_qval *o, canor_rational lo, canor_surreal val, canor_rational hi,
                int32_t conf) {
  if (conf < 0 || conf > 1000) return false;
  return iv_set(o, lo, val, hi, CANOR_QV_STATISTICAL, conf);
}

bool canor_qv_point(canor_qval *o, canor_rational v) {
  canor_surreal s;
  if (!canor_surreal_real(&s, v)) return false;
  return iv_set(o, v, s, v, CANOR_QV_EXACT, 1000);
}

bool canor_qv_valid(const canor_qval *x) { return x && x->valid; }

bool canor_qv_contains_val(const canor_qval *x) {
  canor_surreal slo, shi;
  if (!canor_qv_valid(x)) return false;
  if (!canor_surreal_real(&slo, x->lo) || !canor_surreal_real(&shi, x->hi))
    return false;
  return canor_surreal_cmp(&x->val, &slo) >= 0 &&
         canor_surreal_cmp(&x->val, &shi) <= 0;
}

canor_rational canor_qv_width(const canor_qval *x) {
  if (!canor_qv_valid(x)) return canor_rat(0, 0);
  return canor_rat_add(x->hi, canor_rat(-x->lo.num, x->lo.den));
}

/* The narrower confidence of two statistical operands: a composite is no more
 * confident than its weakest input, and reporting otherwise would manufacture
 * certainty from a join. */
static int32_t iv_conf(const canor_qval *a, const canor_qval *b) {
  int32_t ca = a->kind == CANOR_QV_STATISTICAL ? a->confidence : 1000;
  int32_t cb = b->kind == CANOR_QV_STATISTICAL ? b->confidence : 1000;
  return ca < cb ? ca : cb;
}

bool canor_qv_add(const canor_qval *a, const canor_qval *b, canor_qval *o) {
  canor_surreal v;
  if (!canor_qv_valid(a) || !canor_qv_valid(b) || !o) return false;
  if (!canor_surreal_add(&a->val, &b->val, &v)) return false;
  return iv_set(o, canor_rat_add(a->lo, b->lo), v, canor_rat_add(a->hi, b->hi),
                canor_qv_join(a->kind, b->kind), iv_conf(a, b));
}

bool canor_qv_sub(const canor_qval *a, const canor_qval *b, canor_qval *o) {
  canor_surreal v;
  if (!canor_qv_valid(a) || !canor_qv_valid(b) || !o) return false;
  if (!canor_surreal_sub(&a->val, &b->val, &v)) return false;
  /* [a,b] - [c,d] = [a-d, b-c]. The crossed subtraction is not a typo: the
   * smallest difference takes the largest subtrahend. */
  return iv_set(o, canor_rat_add(a->lo, canor_rat(-b->hi.num, b->hi.den)), v,
                canor_rat_add(a->hi, canor_rat(-b->lo.num, b->lo.den)),
                canor_qv_join(a->kind, b->kind), iv_conf(a, b));
}

bool canor_qv_mul(const canor_qval *a, const canor_qval *b, canor_qval *o) {
  canor_rational c[4], lo, hi;
  canor_surreal v;
  int i;
  if (!canor_qv_valid(a) || !canor_qv_valid(b) || !o) return false;
  /* All four corner products, because sign changes move which corner is
   * extremal - taking lo*lo and hi*hi would be wrong across zero. */
  c[0] = canor_rat_mul(a->lo, b->lo);
  c[1] = canor_rat_mul(a->lo, b->hi);
  c[2] = canor_rat_mul(a->hi, b->lo);
  c[3] = canor_rat_mul(a->hi, b->hi);
  for (i = 0; i < 4; ++i) if (!canor_rat_valid(c[i])) return false;
  lo = hi = c[0];
  for (i = 1; i < 4; ++i) {
    if (canor_rat_cmp(c[i], lo) < 0) lo = c[i];
    if (canor_rat_cmp(c[i], hi) > 0) hi = c[i];
  }
  if (!canor_surreal_mul(&a->val, &b->val, &v)) return false;
  return iv_set(o, lo, v, hi, canor_qv_join(a->kind, b->kind), iv_conf(a, b));
}

void canor_qv_format(const canor_qval *x, char *buf, int cap) {
  char v[96];
  if (!buf || cap < 16) return;
  buf[0] = 0;
  if (!canor_qv_valid(x)) { snprintf(buf, (size_t)cap, "<invalid>"); return; }
  canor_surreal_format(&x->val, v, sizeof v);
  if (x->kind == CANOR_QV_STATISTICAL)
    snprintf(buf, (size_t)cap, "{%lld/%lld, %s, %lld/%lld} STAT c=%d",
             (long long)x->lo.num, (long long)x->lo.den, v,
             (long long)x->hi.num, (long long)x->hi.den, x->confidence);
  else
    snprintf(buf, (size_t)cap, "{%lld/%lld, %s, %lld/%lld} EXACT",
             (long long)x->lo.num, (long long)x->lo.den, v,
             (long long)x->hi.num, (long long)x->hi.den);
}
