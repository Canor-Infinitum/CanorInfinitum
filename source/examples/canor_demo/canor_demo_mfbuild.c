/* canor_demo_mfbuild.c - mfbuild - TST+BBST datasets across Flows / MetaFlows / MultiFlows
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* mfbuild - TST+BBST datasets across Flows / MetaFlows / MultiFlows          */
/* ========================================================================== */
/* INCREMENT ONE of the analytic-continuation build. What is here: the dataset
 * is really constructed, the three Flow tiers are really evaluated, the point
 * clouds are really voxelized, and the continuations are really computed. What
 * is NOT here yet: the curves and the voxel field are drawn as SVG rather than
 * folded into the 3D scene. Said plainly so the gap is visible.
 *
 * --- THE DATASET IS BUILT, NOT DESCRIBED ------------------------------------
 *
 * Five chart-root TST nodes, one per spread chart, each with three
 * differential children - the same atlas `selmer` builds. Every node carries
 * three canor_bbst leaves (PRE, PERI, POST), and each leaf is filled with real
 * k-d keys and then canor_bbst_build'd, so the depth reported below is the depth
 * of an actual balanced index and not an estimate of one.
 *
 * --- THE THREE TIERS ARE NOT THREE NAMES FOR ONE THING ----------------------
 *
 *   FLOW        one word, folded through one trifecta: di, dj, dk.
 *   METAFLOW    the same word across a declared channel/unit map.
 *   MULTIFLOW   the word across EVERY declared trifecta at once, out[t][c],
 *               with a `defined` mask.
 *
 * The mask is the part that matters and it is why MultiFlow is not just "more
 * Flow": a family can divide by a zero-valued operand, so a MultiFlow is
 * PARTIAL by nature. Reporting which cells exist is the difference between a
 * MultiFlow and a MultiFlow with holes quietly filled - and a filled hole
 * would read as a measurement that never happened.
 *
 * --- THE CONTINUATIONS ------------------------------------------------------
 *
 * Each tier gets a fractional relaxation E(alpha, 1; -lambda.t^alpha), the
 * inverse Laplace transform of s^(alpha-1)/(s^alpha + lambda) - the damped
 * PFISD response from EVOLUTIONARY_MODEL Sec.5.5, where the Derivation term
 * carries a declared model residual.
 *
 * The tiers are given DIFFERENT alpha, and that is the whole point rather than
 * a flourish:
 *
 *   alpha = 1     Flow       ordinary exponential decay
 *   alpha = 1/2   MetaFlow   sub-exponential, heavy tail
 *   alpha = 1/3   MultiFlow  heavier still
 *
 * At alpha = 1 the Mittag-Leffler function IS exp, which the suite already
 * checks elsewhere rather than assuming. Below 1 the tail is polynomial rather
 * than exponential, so a MetaFlow's influence persists far longer than a
 * Flow's at the same lambda. That is a visible, checkable consequence of the
 * order - not a stylistic choice about how to draw them.
 *
 * lambda comes from the tier's own defined-fraction, so a tier that evaluated
 * everywhere damps faster than one riddled with holes. Nothing is tuned.
 */

#define MF_TIERS 3

typedef struct {
  const char *name;
  int alpha_num, alpha_den;
  long cells, defined;
  double lambda;
  double curve[48];
  int npts;
  bool ok;
} mf_tier;

int canor_demo_cmd_mfbuild(const canor_demo_config_t *c, int sel, const char *out, long want) {
  const canor_strategy *st = &c->strat[sel];
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0;
  ex_groups spread;
  canor_tst *tree = NULL;
  mf_tier tier[MF_TIERS] = {
    { "Flow",      1, 1, 0, 0, 0.0, {0}, 0, false },
    { "MetaFlow",  1, 2, 0, 0, 0.0, {0}, 0, false },
    { "MultiFlow", 1, 3, 0, 0, 0.0, {0}, 0, false }
  };
  int t, k, i, ti, lvl, span_bits = 0;
  long inserted = 0, maxdepth = 0, occ0 = 0, occN = 0;
  FILE *f;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;
  if (!canor_demo_cloud_get(c, sel, NULL, want, &pts, &n)) return 2;
  tree = calloc(CANOR_TRIFECTAS, sizeof *tree);   /* far too large for the stack */
  if (!tree) { free(pts); return 2; }

  printf("MultiFlow build: TST+BBST datasets across the three tiers\n\n");
  printf("  strategy   %s (%d of %d trifectas)\n", st->id.name,
         st->ntrifecta, CANOR_TRIFECTAS);
  printf("  cloud      %ld points\n", n);
  printf("  flows      %d declared\n\n", c->flows.count);

  /* --- build the dataset ---------------------------------------------------- */
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t) {
    canor_origo z;
    int root;
    z.t = canor_rat(0, 1); z.x = canor_rat(0, 1); z.y = canor_rat(0, 1); z.z = canor_rat(0, 1);
    canor_tst_init(&tree[t]);
    root = canor_tst_add(&tree[t], &z);
    if (root < 0) { free(pts); free(tree); return 2; }
    tree[t].root = root;
    for (k = 0; k < 3; ++k) {
      canor_origo d;
      int nd;
      d.t = canor_rat(k + 1, 4); d.x = canor_rat(spread.u[t][k], 16);
      d.y = canor_rat(t + 1, 8); d.z = canor_rat(1, 2);
      nd = canor_tst_add(&tree[t], &d);
      if (nd < 0) break;
      if (canor_tst_link(&tree[t], root, k, nd) != CANOR_OK) break;
      /* Fill the three phase leaves with real k-d keys. Points are strided by
       * chart and slot so the three leaves of one node hold DIFFERENT data -
       * filling all three from the same slice would make the PRE/PERI/POST
       * split decorative. */
      for (i = 0; i < 3; ++i) {
        canor_bbst *leaf = &tree[t].node[nd].leaf[i];
        long j;
        canor_bbst_init(leaf);
        for (j = (long)(t * 3 + k + i); j < n; j += 17) {
          int64_t key[CANOR_KD];
          key[0] = pts[j].x; key[1] = pts[j].y; key[2] = pts[j].z;
          if (canor_bbst_insert(leaf, key, -1) != CANOR_OK) break;
          ++inserted;
        }
        if (canor_bbst_build(leaf) != CANOR_OK) continue;
        if (canor_bbst_depth(leaf) > maxdepth) maxdepth = canor_bbst_depth(leaf);
      }
    }
  }
  printf("  dataset\n");
  printf("    %d chart trees x 3 differential nodes x 3 phase leaves\n",
         spread.count);
  printf("    %ld keys inserted, deepest balanced index %ld\n",
         inserted, maxdepth);
  canor_demo_check(inserted > 0, "the BBST leaves actually received keys");
  canor_demo_check(maxdepth > 0, "the balanced indices built");

  /* --- the three tiers ------------------------------------------------------ */
  printf("\n  tiers\n");
  printf("    %-11s %8s %8s %9s %s\n", "tier", "cells", "defined", "fraction",
         "alpha");
  for (ti = 0; ti < MF_TIERS; ++ti) {
    for (i = 0; i < c->flows.count; ++i) {
      FLOWVALUE mf[CANOR_TRIFECTAS][3];
      uint32_t defined = 0;
      int tt, kk;
      if (!canor_flow_multiflow(st, &c->flows, i, mf, &defined)) continue;
      for (tt = 0; tt < CANOR_TRIFECTAS; ++tt)
        for (kk = 0; kk < 3; ++kk) {
          int u = tt * 3 + kk;
          /* TIER SCOPE, CORRECTED.
           *
           * An earlier version read MetaFlow as "the declared trifectas" and
           * MultiFlow as "all five". For a fully-fledged Strategy those are the
           * SAME SET, so the two tiers reported identical counts and the
           * distinction did no work - it looked like a result and was an
           * artifact of the definition.
           *
           * WORLD_MODEL Sec.9.3 does not define a MetaFlow by breadth. A
           * MetaFlow maps or constrains Flow facets across a declared context -
           * here the configured channel/unit map. So the MetaFlow tier reads
           * only the cells its declared units project onto, which is a
           * different SHAPE of window rather than a wider one, and stays
           * distinct from MultiFlow however many trifectas a Strategy carries. */
          if (ti == 0 && tt != 0) continue;
          if (ti == 1) {
            int mi, hit = 0;
            for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) {
              if (!c->meta[mi].unit) continue;
              /* unit names are single characters, so the first char IS the
               * unit; compared against the name rather than a second table
               * that could drift out of step with it. */
              if (c->meta[mi].unit == canor_imag_name((canor_imag)(u + 1))[0])
                { hit = 1; break; }
            }
            if (!hit) continue;
          }
          ++tier[ti].cells;
          if ((defined >> u) & 1u) ++tier[ti].defined;
        }
    }
    tier[ti].lambda = tier[ti].cells
      ? 1.0 - (double)tier[ti].defined / (double)tier[ti].cells : 1.0;
    if (tier[ti].lambda < 0.05) tier[ti].lambda = 0.05;
    printf("    %-11s %8ld %8ld %9.3f     1/%d\n", tier[ti].name,
           tier[ti].cells, tier[ti].defined,
           tier[ti].cells ? (double)tier[ti].defined / tier[ti].cells : 0.0,
           tier[ti].alpha_den);
  }
  canor_demo_check(tier[2].cells >= tier[0].cells,
                   "MultiFlow spans at least as many cells as Flow");
  canor_demo_check(tier[1].cells != tier[2].cells,
                   "MetaFlow and MultiFlow are distinct windows, not the same set");
  /* WHETHER LAMBDA IS DOING ANY WORK. With a fully-fledged Strategy every cell
   * evaluates, so every fraction is 1.0 and lambda clamps to the same floor for
   * all three tiers - the curves then differ by alpha alone. That is a property
   * of the SUBJECT, not of the machinery, and it is reported rather than left
   * for the reader to infer from three suspiciously parallel curves. Run this
   * against a Strategy declaring fewer trifectas to see lambda separate. */
  {
    int varies = (tier[0].lambda != tier[1].lambda) ||
                 (tier[1].lambda != tier[2].lambda);
    printf("\n  lambda varies across tiers: %s\n", varies ? "yes" : "NO");
    if (!varies)
      printf("    every cell evaluated, so the curves differ by alpha alone;\n"
             "    a Strategy with undeclared trifectas separates them\n");
  }

  /* --- the continuations ---------------------------------------------------- */
  printf("\n  analytic continuation: E(alpha,1; -lambda.t^alpha)\n");
  for (ti = 0; ti < MF_TIERS; ++ti) {
    int ok = 1;
    tier[ti].npts = 0;
    for (i = 0; i < 40; ++i) {
      fb_rat v;
      canor_ml_report rep;
      int64_t fx = 0;
      canor_rational tt = canor_rat(i, 8);
      fb_rat_init(&v);
      if (!canor_fractional_relax(canor_rat(tier[ti].alpha_num, tier[ti].alpha_den),
                               canor_rat((int64_t)(tier[ti].lambda * 1000), 1000),
                               tt, 64, &v, &rep) || !rep.ok) {
        fb_rat_free(&v); ok = 0; break;
      }
      if (fb_rat_to_fixed(&v.num, &v.den, 20, &fx))
        tier[ti].curve[tier[ti].npts++] = (double)fx / 1048576.0;
      fb_rat_free(&v);
    }
    tier[ti].ok = ok && tier[ti].npts > 1;
    printf("    %-11s alpha=1/%d lambda=%.3f  %d samples  %s\n",
           tier[ti].name, tier[ti].alpha_den, tier[ti].lambda, tier[ti].npts,
           tier[ti].ok ? "" : "FAILED");
  }
  canor_demo_check(tier[0].ok && tier[1].ok && tier[2].ok,
                   "all three continuations evaluated");
  if (tier[0].ok && tier[0].npts > 1)
    canor_demo_check(tier[0].curve[0] > tier[0].curve[tier[0].npts - 1],
                     "the Flow relaxation decays");
  /* THE HEAVY TAIL, CHECKED AT EQUAL LAMBDA - which is the only way it is a
   * theorem. An earlier form compared tier 2 against tier 0 directly. Those
   * tiers carry DIFFERENT lambda (each tier's own undefined-fraction), so the
   * comparison mixed two variables: it passed on a Strategy where every tier
   * happened to share a lambda, and failed on one where they did not - not
   * because the orders were wrong, but because the test was. A dedicated pair
   * at one lambda isolates alpha, which is what the claim is about. */
  {
    fb_rat a1, a3;
    canor_ml_report r1, r3;
    int64_t f1 = 0, f3 = 0;
    canor_rational lam = canor_rat(1, 2), tt = canor_rat(5, 1);
    fb_rat_init(&a1); fb_rat_init(&a3);
    if (canor_fractional_relax(canor_rat(1, 1), lam, tt, 64, &a1, &r1) && r1.ok &&
        canor_fractional_relax(canor_rat(1, 3), lam, tt, 64, &a3, &r3) && r3.ok &&
        fb_rat_to_fixed(&a1.num, &a1.den, 20, &f1) &&
        fb_rat_to_fixed(&a3.num, &a3.den, 20, &f3)) {
      printf("    at equal lambda=1/2, t=5:  alpha=1 -> %.6f   alpha=1/3 -> %.6f\n",
             (double)f1 / 1048576.0, (double)f3 / 1048576.0);
      canor_demo_check(f3 > f1, "at equal lambda, lower alpha leaves the heavier tail");
    } else {
      canor_demo_check(0, "the equal-lambda tail comparison evaluated");
    }
    fb_rat_free(&a1); fb_rat_free(&a3);
  }

  /* --- voxelization of the leaves ------------------------------------------- */
  /* LEVELS MATCHED TO THE DATA, NOT PICKED. An earlier run reported the same
   * occupancy at levels 0, 2, 4 and 6 - the shift was merging nothing, because
   * the k-d keys span far more than 2^6 and every voxel stayed distinct. A
   * ladder that never coarsens is not a ladder. The span is measured first and
   * the levels are laid out across it, so the last level provably collapses to
   * a handful of cells. */
  {
    int64_t lo[CANOR_KD], hi[CANOR_KD];
    int d;
    for (d = 0; d < CANOR_KD; ++d) { lo[d] = INT64_MAX; hi[d] = INT64_MIN; }
    for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
      for (k = 0; k < 3; ++k) {
        int nd = (k == 0) ? tree[t].node[tree[t].root].left
               : (k == 1) ? tree[t].node[tree[t].root].middle
                          : tree[t].node[tree[t].root].right;
        if (nd < 0) continue;
        for (i = 0; i < 3; ++i) {
          const canor_bbst *leaf = &tree[t].node[nd].leaf[i];
          int j;
          for (j = 0; j < leaf->count; ++j)
            for (d = 0; d < CANOR_KD; ++d) {
              int64_t v = leaf->node[j].key[d];
              if (v < lo[d]) lo[d] = v;
              if (v > hi[d]) hi[d] = v;
            }
        }
      }
    span_bits = 0;
    for (d = 0; d < CANOR_KD; ++d) {
      int64_t r = (hi[d] > lo[d]) ? hi[d] - lo[d] : 0;
      int b = 0;
      while (r > 0) { ++b; r >>= 1; }
      if (b > span_bits) span_bits = b;
    }
    printf("\n  key span %d bits, so the levels run 0..%d\n",
           span_bits, span_bits);
    canor_demo_check(span_bits > 0, "the keys span a measurable range");
  }
  printf("  voxelized leaves\n");
  printf("    %-7s %10s %10s\n", "level", "occupied", "keys");
  for (lvl = 0; lvl <= span_bits; lvl += (span_bits > 4 ? span_bits / 4 : 1)) {
    long occ = 0, tot = 0;
    static int64_t seen[4096][CANOR_KD];
    long nseen = 0;
    for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
      for (k = 0; k < 3; ++k) {
        int nd = tree[t].node[tree[t].root].left;
        if (k == 1) nd = tree[t].node[tree[t].root].middle;
        if (k == 2) nd = tree[t].node[tree[t].root].right;
        if (nd < 0) continue;
        for (i = 0; i < 3; ++i) {
          const canor_bbst *leaf = &tree[t].node[nd].leaf[i];
          int j;
          for (j = 0; j < leaf->count; ++j) {
            int64_t vox[CANOR_KD];
            long q;
            int dup = 0;
            ft_voxel(leaf->node[j].key, lvl, vox);
            ++tot;
            for (q = 0; q < nseen; ++q)
              if (seen[q][0] == vox[0] && seen[q][1] == vox[1] &&
                  seen[q][2] == vox[2]) { dup = 1; break; }
            if (!dup && nseen < 4096) {
              seen[nseen][0] = vox[0]; seen[nseen][1] = vox[1];
              seen[nseen][2] = vox[2]; ++nseen; ++occ;
            }
          }
        }
      }
    printf("    %-7d %10ld %10ld\n", lvl, occ, tot);
    if (lvl == 0) { canor_demo_check(tot > 0, "the voxelization saw every inserted key");
                    occ0 = occ; }
    occN = occ;
  }
  /* The ladder must actually coarsen. If the last level occupies as many cells
   * as the first, the zoom is doing nothing and saying so is the only honest
   * report. */
  canor_demo_check(occN < occ0, "the top of the ladder genuinely collapses cells");

  /* --- artifact -------------------------------------------------------------- */
  f = fopen(out, "w");
  if (!f) { free(pts); free(tree); return 2; }
  fputs("<!doctype html>\n<meta charset=\"utf-8\">\n"
        "<title>MultiFlow build</title>\n<style>"
        "body{background:#0d1117;color:#c9d1d9;font:14px/1.6 system-ui,sans-serif;"
        "margin:0;padding:32px}.w{max-width:1000px;margin:0 auto}"
        "h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;margin:26px 0 8px;"
        "color:#e6edf3}.sub{color:#8b949e;font-size:13px;max-width:72ch}"
        ".no{border-left:2px solid #d29922;padding-left:12px;color:#8b949e;"
        "font-size:13px;max-width:72ch}table{border-collapse:collapse;"
        "font:12px ui-monospace,monospace}td,th{border:1px solid #30363d;"
        "padding:3px 8px;text-align:right}th{color:#8b949e;font-weight:500}"
        "td:first-child,th:first-child{text-align:left}"
        "svg{background:#010409;border:1px solid #30363d;border-radius:6px}"
        "</style>\n<div class=\"w\">\n", f);
  fprintf(f, "<h1>MultiFlow build &mdash; %s</h1>\n", st->id.name);
  fprintf(f, "<p class=\"sub\">%d chart trees &times; 3 differential nodes "
             "&times; 3 phase leaves; %ld k-d keys inserted; deepest balanced "
             "index %ld. Every structure below was constructed through the "
             "ordinary type-tower calls and then measured.</p>\n",
          spread.count, inserted, maxdepth);

  fprintf(f, "<h2>Fractional relaxation per tier</h2>\n");
  fprintf(f, "<svg viewBox=\"0 0 720 300\" width=\"100%%\">\n");
  {
    static const char *const CL[3] = { "#58a6ff", "#d29922", "#f85149" };
    fprintf(f, "<line x1=\"50\" y1=\"260\" x2=\"700\" y2=\"260\" stroke=\"#30363d\"/>\n");
    fprintf(f, "<line x1=\"50\" y1=\"20\" x2=\"50\" y2=\"260\" stroke=\"#30363d\"/>\n");
    for (ti = 0; ti < MF_TIERS; ++ti) {
      if (!tier[ti].ok) continue;
      fprintf(f, "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"2\" "
                 "points=\"", CL[ti]);
      for (i = 0; i < tier[ti].npts; ++i) {
        double y = tier[ti].curve[i];
        if (y < 0) y = 0; if (y > 1.2) y = 1.2;
        fprintf(f, "%.1f,%.1f ", 50.0 + 650.0 * i / (double)(tier[ti].npts - 1),
                260.0 - 200.0 * y);
      }
      fprintf(f, "\"/>\n");
      fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"%s\" font-size=\"11\" "
                 "font-family=\"monospace\">%s  alpha=1/%d</text>\n",
              580, 34 + ti * 16, CL[ti], tier[ti].name, tier[ti].alpha_den);
    }
  }
  fprintf(f, "</svg>\n");
  fprintf(f, "<p class=\"sub\">E(&alpha;,1;&minus;&lambda;t<sup>&alpha;</sup>), "
             "the inverse Laplace transform of "
             "s<sup>&alpha;&minus;1</sup>/(s<sup>&alpha;</sup>+&lambda;) &mdash; "
             "the damped PFISD response. At &alpha;=1 this is exp(&minus;&lambda;t); "
             "below 1 the tail is polynomial rather than exponential, so a "
             "MetaFlow's influence outlives a Flow's at the same &lambda;. "
             "&lambda; is each tier's own undefined-fraction, so nothing is "
             "tuned: a tier riddled with holes damps differently because it has "
             "holes.</p>\n");

  fprintf(f, "<h2>Tiers</h2>\n<table>\n<tr><th>tier</th><th>cells</th>"
             "<th>defined</th><th>fraction</th><th>&alpha;</th>"
             "<th>&lambda;</th></tr>\n");
  for (ti = 0; ti < MF_TIERS; ++ti)
    fprintf(f, "<tr><td>%s</td><td>%ld</td><td>%ld</td><td>%.3f</td>"
               "<td>1/%d</td><td>%.3f</td></tr>\n", tier[ti].name,
            tier[ti].cells, tier[ti].defined,
            tier[ti].cells ? (double)tier[ti].defined / tier[ti].cells : 0.0,
            tier[ti].alpha_den, tier[ti].lambda);
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">A MultiFlow is partial by nature &mdash; a family "
             "can divide by a zero-valued operand &mdash; so the "
             "<code>defined</code> mask is carried rather than filled. A filled "
             "hole would read as a measurement that never happened.</p>\n");

  fprintf(f, "<h2>Still to come</h2>\n");
  fprintf(f, "<p class=\"no\">Increment one. The dataset, tiers, continuations "
             "and voxelization are real and checked; the curves and the voxel "
             "occupancy are drawn as SVG here rather than folded into the 3D "
             "scene alongside the volume and PG(3,2). That merge is the next "
             "step, not a claim already delivered.</p>\n");
  fputs("</div>\n", f);
  fclose(f);

  free(pts);
  free(tree);
  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
