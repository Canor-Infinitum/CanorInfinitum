/* Cross-frame aggregation. See canor_agg.h. */

#include "canor_agg.h"

#include <stdio.h>
#include <string.h>

const char *canor_equiv_name(canor_equiv e) {
  switch (e) {
    case CANOR_EQ_CARDINALITY: return "cardinality-equivalent";
    case CANOR_EQ_SNAP:        return "snap-equivalent";
    case CANOR_EQ_IDENTITY:    return "identical";
    default:                return "unrelated";
  }
}

bool canor_agg_declare(canor_agg *a, const char *frame, canor_rational resolution,
                    int radius) {
  if (!a) return false;
  memset(a, 0, sizeof *a);
  /* A frame and a positive resolution are what make the aggregate checkable.
   * Defaulting either would produce an aggregate that cannot be wrong. */
  if (!frame || !*frame) return false;
  if (!canor_rat_valid(resolution) || resolution.num <= 0) return false;
  if (radius < 0) return false;
  snprintf(a->frame, sizeof a->frame, "%s", frame);
  a->resolution = resolution;
  a->radius = radius;
  canor_surreal_zero(&a->counted);
  canor_surreal_zero(&a->sub);
  a->synthetic = canor_rat(0, 1);
  a->declared = true;
  return true;
}

bool canor_agg_observe(canor_agg *a, canor_rational v) {
  canor_surreal s, t;
  if (!a || !a->declared) return false;
  if (!canor_rat_valid(v)) return false;
  if (canor_rat_cmp(v, a->resolution) >= 0) {
    if (!canor_surreal_real(&s, v)) return false;
    if (!canor_surreal_add(&a->counted, &s, &t)) return false;
    a->counted = t;
    ++a->n_counted;
    return true;
  }
  /* SUB-RESOLUTION. One epsilon per observation, regardless of the magnitude:
   * the frame cannot distinguish magnitudes below its own resolution, and
   * pretending otherwise would smuggle in a precision it does not have. What
   * is recorded is THAT something was seen, which is exactly the claim. */
  if (!canor_surreal_eps(&s)) return false;
  if (!canor_surreal_add(&a->sub, &s, &t)) return false;
  a->sub = t;
  ++a->n_sub;
  return true;
}

bool canor_agg_singular(const canor_agg *a, const char **why) {
  if (why) *why = "";
  if (!a || !a->declared) { if (why) *why = "undeclared aggregate"; return true; }
  if (!canor_surreal_valid(&a->sub) || !canor_surreal_valid(&a->counted)) {
    if (why) *why = "the accumulator overflowed its normal form";
    return true;
  }
  if (a->sub.n == 0) return false;             /* nothing sub-resolution yet */
  /* The band test. No threshold: while the sub-resolution mass is
   * infinitesimal the aggregate is absorbing it faithfully; the moment it is
   * not, that mass has become countable at a scale where it was never
   * observed. */
  if (!canor_surreal_is_infinitesimal(&a->sub)) {
    if (why) *why = "sub-resolution mass has left the infinitesimal band";
    return true;
  }
  return false;
}

canor_equiv canor_agg_same(const canor_agg *a, const canor_agg *b, canor_equiv want) {
  bool la = false, lb = false;
  canor_rational ta, tb;
  if (!a || !b || !a->declared || !b->declared) return CANOR_EQ_NONE;

  /* IDENTITY requires the same frame. Two aggregates in different frames are
   * not the same object however their numbers compare - that is the whole of
   * "different provenance, different frame". */
  if (want == CANOR_EQ_IDENTITY && strcmp(a->frame, b->frame) == 0 &&
      a->n_counted == b->n_counted && a->n_sub == b->n_sub &&
      canor_surreal_cmp(&a->counted, &b->counted) == 0 &&
      canor_surreal_cmp(&a->sub, &b->sub) == 0)
    return CANOR_EQ_IDENTITY;

  ta = canor_agg_total(a, &la);
  tb = canor_agg_total(b, &lb);

  /* SNAP: equal after bounded smoothing, within the smaller declared radius.
   * Reported as SNAP and never promoted, because equality-after-smoothing is
   * a statement with a radius attached and identity is not. */
  if (want >= CANOR_EQ_SNAP && a->radius > 0 && b->radius > 0) {
    long d = a->n_counted - b->n_counted;
    if (d < 0) d = -d;
    if (d <= (a->radius < b->radius ? a->radius : b->radius))
      return CANOR_EQ_SNAP;
  }

  /* CARDINALITY: the same count. Deliberately the weakest answer this
   * function can return for matching numbers - asking for IDENTITY and
   * getting CARDINALITY is the API refusing to overstate, not a failure. */
  if (a->n_counted == b->n_counted && canor_rat_cmp(ta, tb) == 0)
    return CANOR_EQ_CARDINALITY;

  return CANOR_EQ_NONE;
}

canor_rational canor_agg_total(const canor_agg *a, bool *lossy) {
  if (lossy) *lossy = false;
  if (!a || !a->declared) return canor_rat(0, 1);
  /* Reading the real part DISCARDS the sub-resolution mass. That is legitimate
   * and it is lossy, so the caller is told - which is the declared
   * lossy-transformation record rather than a silent projection. */
  if (lossy && (a->sub.n > 0 || a->n_surgery > 0)) *lossy = true;
  return canor_surreal_real_part(&a->counted);
}

bool canor_agg_surgery(canor_agg *a, canor_agg_cap *rec) {
  canor_surreal cap, t;
  canor_rational amount;
  const char *why = "";
  if (rec) memset(rec, 0, sizeof *rec);
  if (!a || !a->declared) return false;
  if (!canor_agg_singular(a, &why)) return false;   /* nothing to do, not a fault */

  /* THE CAP IS A STANDARD PIECE, sized at the frame's own resolution - the
   * smallest thing this frame can legitimately count. Sizing it from the
   * excised mass would launder an unmeasurable quantity into a measured one,
   * which is the exact move the whole model exists to prevent. */
  amount = a->resolution;
  if (!canor_surreal_real(&cap, amount)) return false;
  if (!canor_surreal_add(&a->counted, &cap, &t)) return false;
  a->counted = t;

  if (rec) {
    rec->n_excised = a->n_sub;
    rec->capped = amount;
    rec->synthetic = true;
    snprintf(rec->frame, sizeof rec->frame, "%s", a->frame);
  }

  /* EXCISE: the sub-resolution accumulator is cleared, which is what makes the
   * omega term of the potential fall and the process terminate. */
  canor_surreal_zero(&a->sub);
  a->n_sub = 0;
  ++a->n_counted;
  ++a->n_surgery;
  a->synthetic = canor_rat_add(a->synthetic, amount);
  return true;
}

canor_rational canor_agg_synthetic(const canor_agg *a) {
  if (!a || !a->declared) return canor_rat(0, 1);
  return a->synthetic;
}

long canor_agg_surgeries(const canor_agg *a) { return a ? a->n_surgery : 0; }
