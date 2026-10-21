/* canor_demo_agg.c - agg - declared equivalence, and the singularity detector
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* agg - declared equivalence, and the singularity detector                   */
/* ========================================================================== */
/* The telemetry model made checkable. Every claim below is one an aggregation
 * can FAIL, which is the whole reason for stating the model as a required
 * declaration set rather than as a universal.
 */

int canor_demo_cmd_agg(const canor_demo_config_t *c, const char *out) {
  canor_agg a, b, d;
  const char *why = "";
  int i;
  FILE *f;
  (void)c;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  printf("Cross-frame aggregation: declared equivalence, singularity detector\n\n");

  /* --- an aggregation must be declared to be checkable -------------------- */
  printf("  declaration is required, not defaulted\n");
  canor_demo_check(!canor_agg_declare(&a, NULL, canor_rat(1, 100), 3),
                   "an aggregate with no frame is refused");
  canor_demo_check(!canor_agg_declare(&a, "crs:local", canor_rat(0, 1), 3),
                   "an aggregate with no resolution is refused");
  canor_demo_check(canor_agg_declare(&a, "crs:local", canor_rat(1, 100), 3),
                   "frame + resolution + radius declares cleanly");

  /* --- sub-resolution is recorded, not discarded -------------------------- */
  printf("\n  sub-resolution observations are recorded as epsilon\n");
  canor_agg_declare(&a, "crs:local", canor_rat(1, 100), 3);
  canor_agg_observe(&a, canor_rat(1, 2));            /* above resolution */
  canor_agg_observe(&a, canor_rat(1, 10000));        /* below: epsilon */
  canor_agg_observe(&a, canor_rat(1, 100000));       /* below: epsilon */
  printf("    counted %ld, sub-resolution %ld\n", a.n_counted, a.n_sub);
  canor_demo_check(a.n_counted == 1 && a.n_sub == 2, "each observation landed in a band");
  {
    bool lossy = false;
    canor_rational t = canor_agg_total(&a, &lossy);
    printf("    total %lld/%lld, lossy %s\n", (long long)t.num,
           (long long)t.den, lossy ? "yes" : "no");
    canor_demo_check(canor_rat_cmp(t, canor_rat(1, 2)) == 0, "the total is the counted mass");
    canor_demo_check(lossy, "and reading it reports the loss - the declared record");
  }
  canor_demo_check(!canor_agg_singular(&a, &why), "two sub-resolution facts are not a breakdown");

  /* --- cardinality-equivalence is not equality ---------------------------- */
  printf("\n  cardinality-equivalence is not identity\n");
  canor_agg_declare(&b, "crs:remote", canor_rat(1, 100), 3);
  canor_agg_observe(&b, canor_rat(1, 2));
  canor_agg_observe(&b, canor_rat(1, 9999));
  canor_agg_observe(&b, canor_rat(1, 99999));
  {
    canor_equiv got = canor_agg_same(&a, &b, CANOR_EQ_IDENTITY);
    printf("    asked for IDENTITY, established %s\n", canor_equiv_name(got));
    /* The API refuses to overstate: same counts, different frames. Asking for
     * the stronger relation does not produce it. */
    canor_demo_check(got != CANOR_EQ_IDENTITY,
                     "same counts in different frames are NOT identical");
    canor_demo_check(got == CANOR_EQ_CARDINALITY || got == CANOR_EQ_SNAP,
                     "but a weaker relation IS established and named");
  }
  {
    canor_agg same;
    canor_equiv got;
    canor_agg_declare(&same, "crs:local", canor_rat(1, 100), 3);
    canor_agg_observe(&same, canor_rat(1, 2));
    canor_agg_observe(&same, canor_rat(1, 10000));
    canor_agg_observe(&same, canor_rat(1, 100000));
    got = canor_agg_same(&a, &same, CANOR_EQ_IDENTITY);
    canor_demo_check(got == CANOR_EQ_IDENTITY,
                     "same frame and same content DOES establish identity");
  }

  /* --- the detector fires, and needs no threshold ------------------------- */
  printf("\n  the singularity detector\n");
  canor_agg_declare(&d, "crs:sweep", canor_rat(1, 100), 3);
  for (i = 0; i < 500; ++i) canor_agg_observe(&d, canor_rat(1, 1000000));
  printf("    %ld sub-resolution observations: singular %s\n", d.n_sub,
         canor_agg_singular(&d, &why) ? "yes" : "no");
  canor_demo_check(!canor_agg_singular(&d, &why),
                   "500 sub-resolution facts still absorb - finite accumulation is safe");
  /* Force the crossing the same way the surreal suite does, by multiplying the
   * accumulated mass into the countable band. No threshold is consulted. */
  {
    canor_surreal om, crossed;
    canor_surreal_omega(&om);
    if (canor_surreal_mul(&d.sub, &om, &crossed)) d.sub = crossed;
    printf("    after an omega-fold accumulation: singular %s\n",
           canor_agg_singular(&d, &why) ? "yes" : "no");
    canor_demo_check(canor_agg_singular(&d, &why),
                     "the detector fires when the mass leaves the infinitesimal band");
    printf("      reason: %s\n", why);
  }
  canor_demo_check(why && *why, "and it states why, rather than only that");

  /* --- surgery, generalized to any declared aggregate --------------------- */
  printf("\n  surgery: excise the breakdown, cap it, record the cut\n");
  {
    canor_agg_cap rec;
    canor_rational before = canor_agg_total(&d, NULL);
    bool did = canor_agg_surgery(&d, &rec);
    printf("    performed %s, excised %ld sub-resolution observations,\n"
           "      capped with %lld/%lld in frame %s\n",
           did ? "yes" : "no", rec.n_excised,
           (long long)rec.capped.num, (long long)rec.capped.den, rec.frame);
    canor_demo_check(did, "the surgery ran because the detector had fired");
    canor_demo_check(rec.synthetic, "the cap is recorded as synthetic, never as evidence");
    canor_demo_check(!canor_agg_singular(&d, &why),
                     "and the aggregate is no longer singular - the omega term fell");
    /* The cap is sized at the frame's resolution, NOT at the excised mass.
     * Sizing it from what was cut would launder an unmeasurable quantity into
     * a measured one, which is the move the whole model exists to prevent. */
    canor_demo_check(canor_rat_cmp(rec.capped, d.resolution) == 0,
                     "the cap is a standard piece at the frame's own resolution");
    canor_demo_check(canor_rat_cmp(canor_agg_synthetic(&d), rec.capped) == 0,
                     "the aggregate tracks how much of itself is now a cap");
    canor_demo_check(canor_agg_surgeries(&d) == 1, "and how many surgeries it has taken");
    {
      bool lossy = false;
      canor_rational after = canor_agg_total(&d, &lossy);
      printf("    total %lld/%lld -> %lld/%lld, lossy %s\n",
             (long long)before.num, (long long)before.den,
             (long long)after.num, (long long)after.den,
             lossy ? "yes" : "no");
      canor_demo_check(lossy, "a total read after surgery still reports the loss");
    }
    canor_demo_check(!canor_agg_surgery(&d, &rec),
                     "a second surgery is refused - nothing left to excise");
  }

  f = fopen(out, "w");
  if (f) {
    fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n<title>aggregation"
               "</title>\n<style>body{background:#0d1117;color:#c9d1d9;"
               "font:14px/1.6 system-ui,sans-serif;margin:0;padding:32px}"
               ".w{max-width:900px;margin:0 auto}h1{font-size:20px;margin:0 0 4px}"
               "h2{font-size:15px;margin:24px 0 6px;color:#e6edf3}"
               ".sub{color:#8b949e;font-size:13px;max-width:72ch}"
               "table{border-collapse:collapse;font:12px ui-monospace,monospace}"
               "td,th{border:1px solid #30363d;padding:3px 8px;text-align:left}"
               "th{color:#8b949e;font-weight:500}</style>\n<div class=\"w\">\n");
    fprintf(f, "<h1>Cross-frame aggregation</h1>\n");
    fprintf(f, "<p class=\"sub\">The telemetry model as a <b>required "
               "declaration set</b> rather than a universal: frame, resolution, "
               "smoothing radius, and which equivalence was established. A model "
               "that can fail on a given aggregation is worth more than one true "
               "by construction.</p>\n");
    fprintf(f, "<h2>Equivalence is part of the claim</h2>\n<table>"
               "<tr><th>relation</th><th>what it licenses</th></tr>\n"
               "<tr><td>cardinality-equivalent</td><td>same count; different "
               "objects, frames, provenance</td></tr>\n"
               "<tr><td>snap-equivalent</td><td>equal after bounded smoothing, "
               "within a <i>proved</i> radius</td></tr>\n"
               "<tr><td>identical</td><td>the same object, same frame</td></tr>"
               "</table>\n");
    fprintf(f, "<p class=\"sub\">Asking for identity and receiving "
               "cardinality-equivalence is the API refusing to overstate, not a "
               "failure. The same principle already lived in two places: "
               "<code>canor_rat_eq</code> will not call two poisons equal, and two "
               "Golay profiles on one codeword are within distance 6, not "
               "identical.</p>\n");
    fprintf(f, "<h2>The detector</h2>\n<p class=\"sub\">Sub-resolution mass is "
               "carried as &epsilon;. While it stays infinitesimal the aggregate "
               "absorbs it faithfully; the moment it does not, that mass has "
               "become countable at a scale where it was never observed. The "
               "test is a band boundary, so there is <b>no threshold to "
               "tune</b> &mdash; which is the point.</p>\n");
    fprintf(f, "</div>\n");
    fclose(f);
    printf("\n  wrote %s\n", out);
    canor_demo_prov_write_sidecar(out);
  }
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
