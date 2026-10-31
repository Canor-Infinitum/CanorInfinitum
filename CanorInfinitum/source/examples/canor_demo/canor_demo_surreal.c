/* canor_demo_surreal.c - surreal - epsilon as a value, and the band crossing
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* surreal - epsilon as a value, and the band crossing                        */
/* ========================================================================== */
/* The telemetry model asks epsilon to be "an observation below the resolution
 * of its scale: real, recorded, not yet countable at that level". Everything
 * below tests whether the representation actually delivers that, rather than
 * whether it merely has a field named epsilon.
 *
 * The load-bearing check is the LAST one. n.epsilon stays infinitesimal for
 * every finite n, but omega.epsilon = 1: sub-resolution facts accumulate
 * harmlessly at any finite scale and become COUNTABLE at transfinite scale.
 * That is "transfinite smoothing -> singularity" as arithmetic rather than
 * analogy, and it is the thing a rational-only core cannot express at all -
 * it rounds epsilon to zero long before the crossing.
 */

int canor_demo_cmd_surreal(const canor_demo_config_t *c, const char *out) {
  canor_surreal zero, one, eps, om, a, b, r;
  char s1[96], s2[96];
  int i;
  FILE *f;
  (void)c;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  printf("Surreal values: Conway normal form, dyadic fast path\n\n");

  canor_surreal_zero(&zero);
  canor_surreal_real(&one, canor_rat(1, 1));
  canor_surreal_eps(&eps);
  canor_surreal_omega(&om);

  canor_surreal_format(&eps, s1, sizeof s1);
  canor_surreal_format(&om, s2, sizeof s2);
  printf("  epsilon = %s      omega = %s\n\n", s1, s2);

  /* --- epsilon is not zero, and not a small rational --------------------- */
  printf("  epsilon is empty-set adjacent, not empty\n");
  canor_demo_check(canor_surreal_cmp(&eps, &zero) > 0, "epsilon > 0");
  canor_demo_check(canor_surreal_sign(&eps) == 1, "epsilon has positive sign");
  canor_demo_check(!canor_surreal_is_real(&eps), "epsilon is not a real");
  canor_demo_check(canor_surreal_is_infinitesimal(&eps), "epsilon is infinitesimal");
  canor_demo_check(!canor_surreal_is_infinitesimal(&zero),
                   "zero is NOT infinitesimal - absence and sub-resolution differ");
  /* Smaller than every representable positive rational. Checked against a
   * genuinely small one rather than against 1, since the claim is about ALL
   * of them and a weak witness would pass vacuously. */
  {
    canor_surreal tiny;
    canor_surreal_real(&tiny, canor_rat(1, 1000000000));
    canor_demo_check(canor_surreal_cmp(&eps, &tiny) < 0,
                     "epsilon < 1e-9 - below every representable rational resolution");
  }

  /* --- a sub-resolution fact does not round away ------------------------- */
  printf("\n  a recorded sub-resolution fact survives arithmetic\n");
  canor_surreal_add(&one, &eps, &a);
  canor_surreal_format(&a, s1, sizeof s1);
  printf("    1 + epsilon = %s\n", s1);
  canor_demo_check(canor_surreal_cmp(&a, &one) > 0, "1 + epsilon > 1 - it did not round away");
  canor_surreal_sub(&a, &one, &b);
  canor_demo_check(canor_surreal_cmp(&b, &eps) == 0, "(1 + epsilon) - 1 = epsilon exactly");
  /* THE PROJECTION IS LOSSY, and that is the point: a consumer reading only
   * the real part sees 1 and owes the declared record. */
  canor_demo_check(canor_rat_cmp(canor_surreal_real_part(&a), canor_rat(1, 1)) == 0,
                   "the real part of 1 + epsilon is 1 - the projection loses it");

  /* --- finite accumulation stays below resolution ------------------------ */
  printf("\n  finite accumulation stays sub-resolution\n");
  r = eps;
  for (i = 1; i < 1000; ++i) {
    canor_surreal t;
    if (!canor_surreal_add(&r, &eps, &t)) break;
    r = t;
  }
  canor_surreal_format(&r, s1, sizeof s1);
  printf("    1000 . epsilon = %s\n", s1);
  canor_demo_check(canor_surreal_is_infinitesimal(&r),
                   "1000 epsilons are still infinitesimal - no finite count reaches scale");
  {
    canor_surreal tiny;
    canor_surreal_real(&tiny, canor_rat(1, 1000000000));
    canor_demo_check(canor_surreal_cmp(&r, &tiny) < 0,
                     "and still below 1e-9 - finite accumulation cannot cross");
  }

  /* --- THE CROSSING ------------------------------------------------------- */
  printf("\n  transfinite accumulation crosses into the countable band\n");
  canor_surreal_mul(&om, &eps, &a);
  canor_surreal_format(&a, s1, sizeof s1);
  printf("    omega . epsilon = %s\n", s1);
  canor_demo_check(canor_surreal_cmp(&a, &one) == 0,
                   "omega . epsilon = 1 - the singularity, stated exactly");
  canor_demo_check(canor_surreal_is_real(&a) && !canor_surreal_is_infinitesimal(&a),
                   "the product left the infinitesimal band");
  canor_demo_check(canor_surreal_is_infinite(&om) && canor_surreal_cmp(&om, &one) > 0,
                   "omega exceeds every finite value");

  /* --- the dyadic fast path is cost, never meaning ------------------------ */
  printf("\n  dyadic fast path is an optimisation, not a semantics\n");
  {
    canor_surreal h, th, sum;
    canor_surreal_real(&h, canor_rat(1, 2));      /* dyadic */
    canor_surreal_real(&th, canor_rat(1, 3));     /* not dyadic */
    canor_demo_check(canor_surreal_is_real(&h) && canor_surreal_is_real(&th),
                     "1/2 and 1/3 are both real - the fast path changes no classification");
    canor_surreal_add(&h, &th, &sum);
    canor_demo_check(canor_rat_cmp(canor_surreal_real_part(&sum), canor_rat(5, 6)) == 0,
                     "1/2 + 1/3 = 5/6 exactly, across the fast-path boundary");
  }

  /* --- refusal rather than truncation ------------------------------------- */
  {
    canor_surreal wide;
    int ok = 1;
    canor_surreal_zero(&wide);
    /* Distinct exponents, so nothing merges and the form genuinely widens. */
    for (i = 0; i < FS_MAX_TERMS + 3; ++i) {
      canor_surreal one_term, acc;
      canor_surreal_term(&one_term, canor_rat(i, 1), canor_rat(1, 1));
      if (!canor_surreal_add(&wide, &one_term, &acc)) { ok = 0; wide = acc; break; }
      wide = acc;
    }
    canor_demo_check(!ok && !canor_surreal_valid(&wide),
                     "an over-wide normal form REFUSES rather than truncating");
  }

  f = fopen(out, "w");
  if (f) {
    fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n<title>surreal</title>\n"
               "<style>body{background:#0d1117;color:#c9d1d9;font:14px/1.6 "
               "system-ui,sans-serif;margin:0;padding:32px}.w{max-width:900px;"
               "margin:0 auto}h1{font-size:20px;margin:0 0 4px}.sub{color:#8b949e;"
               "font-size:13px;max-width:72ch}table{border-collapse:collapse;"
               "font:12px ui-monospace,monospace;margin:10px 0}td,th{border:1px "
               "solid #30363d;padding:3px 8px;text-align:left}th{color:#8b949e;"
               "font-weight:500}</style>\n<div class=\"w\">\n");
    fprintf(f, "<h1>Epsilon as a value</h1>\n");
    fprintf(f, "<p class=\"sub\">Conway normal form &Sigma;&omega;<sup>e</sup>r "
               "with rational exponents, so the infinite / real / infinitesimal "
               "bands are all representable and ordered. &epsilon; = "
               "&omega;<sup>&minus;1</sup> is strictly between 0 and every "
               "positive rational, which is what lets a sub-resolution "
               "observation be <i>recorded</i> rather than rounded to "
               "absence.</p>\n");
    fprintf(f, "<table><tr><th>claim</th><th>value</th></tr>\n");
    canor_surreal_format(&eps, s1, sizeof s1);
    fprintf(f, "<tr><td>&epsilon;</td><td>%s</td></tr>\n", s1);
    canor_surreal_add(&one, &eps, &a);
    canor_surreal_format(&a, s1, sizeof s1);
    fprintf(f, "<tr><td>1 + &epsilon;</td><td>%s</td></tr>\n", s1);
    canor_surreal_format(&r, s1, sizeof s1);
    fprintf(f, "<tr><td>1000&middot;&epsilon;</td><td>%s (still infinitesimal)"
               "</td></tr>\n", s1);
    canor_surreal_mul(&om, &eps, &a);
    canor_surreal_format(&a, s1, sizeof s1);
    fprintf(f, "<tr><td>&omega;&middot;&epsilon;</td><td>%s <b>&mdash; the "
               "crossing</b></td></tr>\n", s1);
    fprintf(f, "</table>\n");
    fprintf(f, "<p class=\"sub\">No finite accumulation of &epsilon; reaches the "
               "countable band; an &omega;-fold accumulation lands exactly on 1. "
               "That is <b>transfinite smoothing &rarr; singularity</b> as "
               "arithmetic rather than analogy &mdash; and it is precisely what a "
               "rational-only core cannot express, because it rounds &epsilon; to "
               "zero long before the crossing.</p>\n");
    fprintf(f, "</div>\n");
    fclose(f);
    printf("\n  wrote %s\n", out);
    canor_demo_prov_write_sidecar(out);
  }
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
