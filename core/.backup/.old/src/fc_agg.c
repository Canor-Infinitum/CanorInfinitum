/* Cross-frame aggregation. See fc_agg.h. */

#include "fc_agg.h"

#include <stdio.h>
#include <string.h>

const char *fc_equiv_name(fc_equiv e) {
  switch (e) {
    case FC_EQ_CARDINALITY: return "cardinality-equivalent";
    case FC_EQ_SNAP:        return "snap-equivalent";
    case FC_EQ_IDENTITY:    return "identical";
    default:                return "unrelated";
  }
}

bool fc_agg_declare(fc_agg *a, const char *frame, fc_rational resolution,
                    int radius) {
  if (!a) return false;
  memset(a, 0, sizeof *a);
  /* A frame and a positive resolution are what make the aggregate checkable.
   * Defaulting either would produce an aggregate that cannot be wrong. */
  if (!frame || !*frame) return false;
  if (!fc_rat_valid(resolution) || resolution.num <= 0) return false;
  if (radius < 0) return false;
  snprintf(a->frame, sizeof a->frame, "%s", frame);
  a->resolution = resolution;
  a->radius = radius;
  fc_surreal_zero(&a->counted);
  fc_surreal_zero(&a->sub);
  a->synthetic = fc_rat(0, 1);
  a->declared = true;
  return true;
}

bool fc_agg_observe(fc_agg *a, fc_rational v) {
  fc_surreal s, t;
  if (!a || !a->declared) return false;
  if (!fc_rat_valid(v)) return false;
  if (fc_rat_cmp(v, a->resolution) >= 0) {
    if (!fc_surreal_real(&s, v)) return false;
    if (!fc_surreal_add(&a->counted, &s, &t)) return false;
    a->counted = t;
    ++a->n_counted;
    return true;
  }
  /* SUB-RESOLUTION. One epsilon per observation, regardless of the magnitude:
   * the frame cannot distinguish magnitudes below its own resolution, and
   * pretending otherwise would smuggle in a precision it does not have. What
   * is recorded is THAT something was seen, which is exactly the claim. */
  if (!fc_surreal_eps(&s)) return false;
  if (!fc_surreal_add(&a->sub, &s, &t)) return false;
  a->sub = t;
  ++a->n_sub;
  return true;
}

bool fc_agg_singular(const fc_agg *a, const char **why) {
  if (why) *why = "";
  if (!a || !a->declared) { if (why) *why = "undeclared aggregate"; return true; }
  if (!fc_surreal_valid(&a->sub) || !fc_surreal_valid(&a->counted)) {
    if (why) *why = "the accumulator overflowed its normal form";
    return true;
  }
  if (a->sub.n == 0) return false;             /* nothing sub-resolution yet */
  /* The band test. No threshold: while the sub-resolution mass is
   * infinitesimal the aggregate is absorbing it faithfully; the moment it is
   * not, that mass has become countable at a scale where it was never
   * observed. */
  if (!fc_surreal_is_infinitesimal(&a->sub)) {
    if (why) *why = "sub-resolution mass has left the infinitesimal band";
    return true;
  }
  return false;
}

fc_equiv fc_agg_same(const fc_agg *a, const fc_agg *b, fc_equiv want) {
  bool la = false, lb = false;
  fc_rational ta, tb;
  if (!a || !b || !a->declared || !b->declared) return FC_EQ_NONE;

  /* IDENTITY requires the same frame. Two aggregates in different frames are
   * not the same object however their numbers compare - that is the whole of
   * "different provenance, different frame". */
  if (want == FC_EQ_IDENTITY && strcmp(a->frame, b->frame) == 0 &&
      a->n_counted == b->n_counted && a->n_sub == b->n_sub &&
      fc_surreal_cmp(&a->counted, &b->counted) == 0 &&
      fc_surreal_cmp(&a->sub, &b->sub) == 0)
    return FC_EQ_IDENTITY;

  ta = fc_agg_total(a, &la);
  tb = fc_agg_total(b, &lb);

  /* SNAP: equal after bounded smoothing, within the smaller declared radius.
   * Reported as SNAP and never promoted, because equality-after-smoothing is
   * a statement with a radius attached and identity is not. */
  if (want >= FC_EQ_SNAP && a->radius > 0 && b->radius > 0) {
    long d = a->n_counted - b->n_counted;
    if (d < 0) d = -d;
    if (d <= (a->radius < b->radius ? a->radius : b->radius))
      return FC_EQ_SNAP;
  }

  /* CARDINALITY: the same count. Deliberately the weakest answer this
   * function can return for matching numbers - asking for IDENTITY and
   * getting CARDINALITY is the API refusing to overstate, not a failure. */
  if (a->n_counted == b->n_counted && fc_rat_cmp(ta, tb) == 0)
    return FC_EQ_CARDINALITY;

  return FC_EQ_NONE;
}

fc_rational fc_agg_total(const fc_agg *a, bool *lossy) {
  if (lossy) *lossy = false;
  if (!a || !a->declared) return fc_rat(0, 1);
  /* Reading the real part DISCARDS the sub-resolution mass. That is legitimate
   * and it is lossy, so the caller is told - which is the declared
   * lossy-transformation record rather than a silent projection. */
  if (lossy && (a->sub.n > 0 || a->n_surgery > 0)) *lossy = true;
  return fc_surreal_real_part(&a->counted);
}

bool fc_agg_surgery(fc_agg *a, fc_agg_cap *rec) {
  fc_surreal cap, t;
  fc_rational amount;
  const char *why = "";
  if (rec) memset(rec, 0, sizeof *rec);
  if (!a || !a->declared) return false;
  if (!fc_agg_singular(a, &why)) return false;   /* nothing to do, not a fault */

  /* THE CAP IS A STANDARD PIECE, sized at the frame's own resolution - the
   * smallest thing this frame can legitimately count. Sizing it from the
   * excised mass would launder an unmeasurable quantity into a measured one,
   * which is the exact move the whole model exists to prevent. */
  amount = a->resolution;
  if (!fc_surreal_real(&cap, amount)) return false;
  if (!fc_surreal_add(&a->counted, &cap, &t)) return false;
  a->counted = t;

  if (rec) {
    rec->n_excised = a->n_sub;
    rec->capped = amount;
    rec->synthetic = true;
    snprintf(rec->frame, sizeof rec->frame, "%s", a->frame);
  }

  /* EXCISE: the sub-resolution accumulator is cleared, which is what makes the
   * omega term of the potential fall and the process terminate. */
  fc_surreal_zero(&a->sub);
  a->n_sub = 0;
  ++a->n_counted;
  ++a->n_surgery;
  a->synthetic = fc_rat_add(a->synthetic, amount);
  return true;
}

fc_rational fc_agg_synthetic(const fc_agg *a) {
  if (!a || !a->declared) return fc_rat(0, 1);
  return a->synthetic;
}

long fc_agg_surgeries(const fc_agg *a) { return a ? a->n_surgery : 0; }
