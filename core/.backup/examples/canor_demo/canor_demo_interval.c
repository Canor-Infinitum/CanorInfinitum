/* canor_demo_interval.c - interval - quantum values as a tagged triple
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* interval - quantum values as a tagged triple                               */
/* ========================================================================== */
/* Every check below is one the representation can FAIL. The two that matter
 * most are the invariant refusal (a val outside its bound is not clamped) and
 * the kind asymmetry (nothing turns confidence into certainty).
 */

int canor_demo_cmd_interval(const canor_demo_config_t *c, const char *out) {
  canor_qval a, b, r;
  canor_surreal v, one, eps;
  char s1[160], s2[160];
  FILE *f;
  (void)c;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  printf("Quantum values: {lo, val, hi} with a kind\n\n");

  canor_surreal_real(&one, canor_rat(1, 1));
  canor_surreal_eps(&eps);

  /* --- the invariant is checked, not assumed ----------------------------- */
  printf("  lo <= val <= hi is enforced at construction\n");
  canor_surreal_real(&v, canor_rat(1, 2));
  canor_demo_check(canor_qv_exact(&a, canor_rat(0, 1), v, canor_rat(1, 1)),
                   "a val inside its bound constructs");
  canor_surreal_real(&v, canor_rat(2, 1));
  canor_demo_check(!canor_qv_exact(&b, canor_rat(0, 1), v, canor_rat(1, 1)),
                   "a val outside its bound is REFUSED, not clamped");
  canor_surreal_real(&v, canor_rat(1, 2));
  canor_demo_check(!canor_qv_exact(&b, canor_rat(1, 1), v, canor_rat(0, 1)),
                   "an inverted bound is refused");

  /* --- epsilon against a tight bound -------------------------------------- */
  /* This is the case worth stating: 1 + epsilon does NOT fit in [0,1], and the
   * refusal is correct. It says the declared bound cannot hold the
   * sub-resolution mass it was handed - a defect in the bound, not a rounding
   * nuisance to be smoothed over. */
  printf("\n  epsilon against a tight bound\n");
  canor_surreal_add(&one, &eps, &v);
  canor_demo_check(!canor_qv_exact(&b, canor_rat(0, 1), v, canor_rat(1, 1)),
                   "1 + epsilon does not fit in [0,1] - the bound is too tight");
  canor_demo_check(canor_qv_exact(&b, canor_rat(0, 1), v, canor_rat(2, 1)),
                   "and it fits in [0,2], where the bound can hold it");
  canor_qv_format(&b, s1, sizeof s1);
  printf("    %s\n", s1);

  /* --- the kind is contagious upward and never promoted ------------------- */
  printf("\n  kind propagation\n");
  canor_demo_check(canor_qv_join(CANOR_QV_EXACT, CANOR_QV_EXACT) == CANOR_QV_EXACT,
                   "EXACT op EXACT = EXACT");
  canor_demo_check(canor_qv_join(CANOR_QV_EXACT, CANOR_QV_STATISTICAL) == CANOR_QV_STATISTICAL,
                   "EXACT op STATISTICAL = STATISTICAL");
  canor_demo_check(canor_qv_join(CANOR_QV_STATISTICAL, CANOR_QV_EXACT) == CANOR_QV_STATISTICAL,
                   "and the other way round - the rule is symmetric in its operands");
  canor_demo_check(canor_qv_join(CANOR_QV_STATISTICAL, CANOR_QV_STATISTICAL) == CANOR_QV_STATISTICAL,
                   "nothing in the lattice promotes to EXACT");

  {
    canor_qval ex, st;
    canor_surreal hv;
    canor_surreal_real(&hv, canor_rat(1, 2));
    canor_qv_exact(&ex, canor_rat(0, 1), hv, canor_rat(1, 1));
    canor_surreal_real(&hv, canor_rat(3, 10));
    canor_qv_stat(&st, canor_rat(1, 5), hv, canor_rat(2, 5), 950);
    canor_demo_check(canor_qv_add(&ex, &st, &r), "a proved value combines with a measured one");
    canor_qv_format(&r, s1, sizeof s1);
    printf("    exact + statistical = %s\n", s1);
    canor_demo_check(r.kind == CANOR_QV_STATISTICAL,
                     "and the result is MEASURED - certainty is not recoverable");
    canor_demo_check(r.confidence == 950,
                     "confidence is the weakest input's, never manufactured by the join");
  }

  /* --- interval arithmetic ------------------------------------------------ */
  printf("\n  interval arithmetic keeps val inside the bound\n");
  {
    canor_qval p, q;
    canor_surreal pv, qv;
    canor_surreal_real(&pv, canor_rat(3, 2));
    canor_surreal_real(&qv, canor_rat(-1, 2));
    canor_demo_check(canor_qv_exact(&p, canor_rat(1, 1), pv, canor_rat(2, 1)), "p = {1, 3/2, 2}");
    canor_demo_check(canor_qv_exact(&q, canor_rat(-1, 1), qv, canor_rat(1, 1)), "q = {-1, -1/2, 1}");
    canor_demo_check(canor_qv_add(&p, &q, &r) && canor_qv_contains_val(&r), "p + q holds its val");
    canor_qv_format(&r, s1, sizeof s1);
    printf("    p + q = %s\n", s1);
    canor_demo_check(canor_qv_sub(&p, &q, &r) && canor_qv_contains_val(&r), "p - q holds its val");
    canor_qv_format(&r, s2, sizeof s2);
    printf("    p - q = %s\n", s2);
    /* The crossed subtraction matters: [1,2] - [-1,1] is [0,3], not [2,1]. */
    canor_demo_check(canor_rat_cmp(r.lo, canor_rat(0, 1)) == 0 &&
                     canor_rat_cmp(r.hi, canor_rat(3, 1)) == 0,
                     "subtraction crosses the bounds - [1,2]-[-1,1] = [0,3]");
    canor_demo_check(canor_qv_mul(&p, &q, &r) && canor_qv_contains_val(&r), "p * q holds its val");
    canor_qv_format(&r, s1, sizeof s1);
    printf("    p * q = %s\n", s1);
    /* All four corners, because sign changes move which is extremal. */
    canor_demo_check(canor_rat_cmp(r.lo, canor_rat(-2, 1)) == 0 &&
                     canor_rat_cmp(r.hi, canor_rat(2, 1)) == 0,
                     "multiplication takes all four corners across zero");
  }

  /* --- width is the resolution actually carried --------------------------- */
  {
    canor_qval pt;
    canor_demo_check(canor_qv_point(&pt, canor_rat(7, 3)), "a point interval constructs");
    canor_demo_check(canor_rat_cmp(canor_qv_width(&pt), canor_rat(0, 1)) == 0,
                     "and has width zero - it claims exactness and carries it");
  }

  f = fopen(out, "w");
  if (f) {
    fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n<title>quantum "
               "values</title>\n<style>body{background:#0d1117;color:#c9d1d9;"
               "font:14px/1.6 system-ui,sans-serif;margin:0;padding:32px}"
               ".w{max-width:900px;margin:0 auto}h1{font-size:20px;margin:0 0 4px}"
               "h2{font-size:15px;margin:24px 0 6px;color:#e6edf3}"
               ".sub{color:#8b949e;font-size:13px;max-width:72ch}"
               "table{border-collapse:collapse;font:12px ui-monospace,monospace}"
               "td,th{border:1px solid #30363d;padding:3px 8px;text-align:left}"
               "th{color:#8b949e;font-weight:500}</style>\n<div class=\"w\">\n");
    fprintf(f, "<h1>Quantum values &mdash; {lo, val, hi}</h1>\n");
    fprintf(f, "<p class=\"sub\">A triple, not a pair. <code>canor_alg</code> "
               "already had to <i>choose</i> a representative from an isolating "
               "interval and returned the midpoint; the triple makes that a "
               "declared commitment instead of an implicit convention. "
               "<code>val</code> is surreal, so sub-resolution mass rides in it "
               "as &epsilon; while the bounds stay countable.</p>\n");
    fprintf(f, "<h2>The kind is part of the claim</h2>\n<table>"
               "<tr><th>kind</th><th>says</th></tr>\n"
               "<tr><td>EXACT</td><td>the value is in here, <b>certainly</b> "
               "&mdash; isolation, algebraic bounds</td></tr>\n"
               "<tr><td>STATISTICAL</td><td>in here, <b>with confidence c</b> "
               "&mdash; sampling, thermodynamic, p-bit</td></tr></table>\n");
    fprintf(f, "<p class=\"sub\">Same shape, different claim. The kind is "
               "contagious upward and never promoted: EXACT&nbsp;op&nbsp;"
               "STATISTICAL is STATISTICAL, and no operation turns confidence "
               "into certainty. Confidence itself takes the weakest input's, so "
               "a join cannot manufacture it. This is the same refusal "
               "discipline as <code>canor_equiv</code> declining to promote "
               "cardinality to identity.</p>\n");
    fprintf(f, "<h2>Why p-bits can enter at all</h2>\n<p class=\"sub\">p-bit "
               "noise is physical and thermal, so a <i>sample</i> can never be "
               "stamped as a value &mdash; only the distribution and the noise "
               "source's provenance can. The interval is what lets a stochastic "
               "result join a deterministic framework without lying: the sample "
               "is not exact, but the interval is a true statement about it, "
               "<i>provided it is labelled</i>.</p>\n");
    fprintf(f, "</div>\n");
    fclose(f);
    printf("\n  wrote %s\n", out);
    canor_demo_prov_write_sidecar(out);
  }
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
