/* canor_demo_quality.c - quality - the deterministic structural metric
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* quality - the deterministic structural metric                              */
/* ========================================================================== */
/* "All term/factor/operator/information across the map - induced or explicit."
 *
 * Four components, and the split between the first three and the fourth is the
 * point. EXPLICIT structure is what the Strategy is written to be: terms,
 * factors, distinct operators, distinct arguments. INDUCED information is what
 * the map turns out to do: how many of the 64 possible (di, dj, dk) codes it
 * actually produces over a probe set.
 *
 * They are not the same and a Strategy can score high on one and low on the
 * other. A family with eight terms that always evaluates to the same code is
 * elaborate and uninformative; that is precisely the case a term count alone
 * would flatter, and it is why the induced term has to be measured rather than
 * inferred from the syntax.
 *
 * NO ENTROPY, NO FLOATING POINT. Distinct-code count is an exact integer out of
 * 64. Shannon entropy would need p.log(p) and would put a float in the middle
 * of a metric that feeds a reported figure. The count is coarser and it is
 * reproducible bit for bit, which for a quality score that will be compared
 * across runs is the more useful property.
 *
 * The 3-tuple is (Quality, Capacity, (Quality x Capacity) x 0.5) as specified.
 * The third term is deliberately not a mean of the other two: it is half their
 * product, so a Strategy scoring zero on either collapses the composite. That
 * is the correct behaviour for an instrument rating - an elaborate Strategy
 * that cannot express bias is not half-useful, it is useless, and a metric
 * that averaged would report it as middling.
 */

static void qual_family(const canor_family *f, int *terms, int *factors,
                        unsigned *opmask, unsigned *argmask) {
  int t, k;
  *terms += f->nterms;
  for (t = 0; t < f->nterms; ++t) {
    *factors += f->term[t].nf;
    for (k = 0; k < f->term[t].nf; ++k) {
      *opmask |= 1u << (unsigned)f->term[t].f[k].op;
      *argmask |= 1u << (unsigned)(f->term[t].f[k].arg & 31);
    }
  }
}

static int popcount32(unsigned v) {
  int n = 0;
  while (v) { n += (int)(v & 1u); v >>= 1; }
  return n;
}

static int qual_gcd(int a, int b) { while (b) { int t = a % b; a = b; b = t; } return a ? a : 1; }

void canor_demo_qual_measure_ore(const canor_demo_config_t *c, int sel, canor_demo_qual_t *q) {
  int seen[8], nseen = 0, k, i;
  q->periods_distinct = 1;
  q->period_lcm = 1;
  if (!c->strat_use_ore[sel]) return;
  for (k = 0; k < 3; ++k) {
    int p = canor_ore_period(&c->strat_ore[sel][k]);
    int dup = 0;
    if (p <= 0) continue;                 /* aperiodic contributes no period */
    for (i = 0; i < nseen; ++i) if (seen[i] == p) dup = 1;
    if (!dup) { seen[nseen++] = p; }
    q->period_lcm = q->period_lcm / qual_gcd(q->period_lcm, p) * p;
  }
  q->periods_distinct = nseen ? nseen : 1;
}

/* periods_distinct and period_lcm arrive as ARGUMENTS rather than being set on
 * `q` by the caller: this function begins by zeroing `q`, so anything written
 * beforehand is silently lost - which it was, and the score then divided by a
 * period count of zero without complaining. */
void canor_demo_qual_measure(const canor_demo_config_t *c, const canor_strategy *s,
                         const canor_demo_cloud_point_t *pts, long n,
                         int periods_distinct, int period_lcm, canor_demo_qual_t *q) {
  unsigned opmask = 0, argmask = 0;
  uint64_t seen_lo = 0, seen_hi = 0;
  long i;
  memset(q, 0, sizeof *q);
  q->periods_distinct = periods_distinct > 0 ? periods_distinct : 1;
  q->period_lcm = period_lcm > 0 ? period_lcm : 1;
  qual_family(&s->d[0][0], &q->terms, &q->factors, &opmask, &argmask);
  qual_family(&s->d[0][1], &q->terms, &q->factors, &opmask, &argmask);
  qual_family(&s->d[0][2], &q->terms, &q->factors, &opmask, &argmask);
  q->ops_distinct = popcount32(opmask);
  q->args_distinct = popcount32(argmask);

  for (i = 0; i < n; ++i) {
    char word[CANOR_FLOW_WORD_MAX];
    int cnt[3], len, code;
    int32_t args[4];
    FLOWVALUE v[3];
    len = canor_demo_cloud_word(c, &pts[i], word, CANOR_FLOW_WORD_MAX, cnt);
    if (len == 0) continue;
    args[0] = pts[i].x; args[1] = pts[i].y; args[2] = pts[i].z; args[3] = pts[i].a;
    if (!canor_di(s, args, 4, &v[0]) || !canor_dj(s, args, 4, &v[1]) ||
        !canor_dk(s, args, 4, &v[2])) continue;
    ++q->evaluated;
    code = (int)v[0] * 16 + (int)v[1] * 4 + (int)v[2];
    if (code < 64) {
      if (code < 32) seen_lo |= (uint64_t)1 << code;
      else seen_hi |= (uint64_t)1 << (code - 32);
    }
  }
  {
    int b;
    for (b = 0; b < 32; ++b) {
      if (seen_lo & ((uint64_t)1 << b)) ++q->codes_distinct;
      if (seen_hi & ((uint64_t)1 << b)) ++q->codes_distinct;
    }
  }
  /* Explicit is normalised against the declared maxima rather than against
   * the observed ensemble, so the score does not move when the ensemble does. */
  q->explicit_score =
      ((double)q->terms / (3.0 * CANOR_FAMILY_TERMS_MAX) +
       (double)q->factors / (3.0 * CANOR_FAMILY_TERMS_MAX * CANOR_FAMILY_FACTORS_MAX) +
       (double)q->ops_distinct / 3.0 +
       (double)q->args_distinct / (double)CANOR_FAMILY_ARGS_MAX +
       (double)(q->periods_distinct ? q->periods_distinct : 1) / 3.0) / 5.0;
  q->induced_score = (double)q->codes_distinct / 64.0;
  /* Geometric mean: a Strategy needs BOTH written structure and expressed
   * variety. An arithmetic mean would let one cover for the other, which is
   * the failure mode this metric exists to catch. */
  q->quality = 100.0 * sqrt(q->explicit_score * q->induced_score);
}
