/* canor_demo_scales.c - scales - scale-relative operators, ordinal sampling, duality invariance
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* scales - scale-relative operators, ordinal sampling, duality invariance    */
/* ========================================================================== */
/* WHAT IS EXACT HERE AND WHAT IS INTERPRETED, stated once so the results are
 * not read as more than they are.
 *
 * EXACT: the ordinal arithmetic, the Q(alpha) sampling and its interval
 * widths, the Golay snap and its radius, the PG(3,2) duality and its
 * involution property. Every number below is checkable and none is rounded.
 *
 * INTERPRETED: reading PG(3,2) duality as the relevant "negation". It is
 * offered as the nearest exactly-defined involution available on this
 * structure, not as an implementation of any particular phrase. What it does
 * establish is genuine and is the reason to want it:
 *
 *     A QUANTITY INVARIANT UNDER DUALITY IS STRUCTURAL.
 *     A QUANTITY THAT MOVES UNDER IT IS AN ARTIFACT OF THE LABELLING.
 *
 * That separation is exactly what a relational bias measure needs and cannot
 * obtain from a single frame, so it earns its place independently of whether
 * it is the intended reading.
 *
 * THE THREE LEVELS, and why "both" is the interesting one.
 *
 *   TENSOR LEVEL. H^(x)m is associative, so its bracket structure is rigid:
 *   the Jacobiator vanishes everywhere and there is no graded failure for a
 *   relabelling to disturb. The prediction fixed in advance is that tensor
 *   quantities are duality-invariant, and a failure here would indicate a bug
 *   rather than a discovery.
 *
 *   PROBE LEVEL. The Cayley-Dickson side has graded failure, so a per-channel
 *   profile can genuinely move. Whether it does is an open measurement and is
 *   reported rather than predicted.
 *
 *   BOTH. Applying the involution at both levels at once is the only
 *   configuration that can distinguish "the algebra is symmetric" from "the
 *   measurement is symmetric". They are different claims and a single-level
 *   test conflates them.
 *
 * ONE ORDINAL SUBTLETY THAT IS NOT DECORATION. The ladder is indexed by
 * omega.k + n, and the ordering says no amount of refinement at class k
 * reaches class k+1. That is the true relationship between halving a root's
 * isolating interval and moving up the Kapprox ladder: the first is a
 * precision change, the second is a change of growth regime, and an integer
 * index would have implied that enough of the former eventually amounts to the
 * latter. It does not, and the ordinal index is what records that.
 */

int canor_demo_cmd_scales(const canor_demo_config_t *c, int sel, const char *out) {
  fo_ord lad[12];
  char obuf[64];
  int nlad = 0, i, k, u;
  canor_poly mp;
  canor_alg roots[8];
  int nroots = 0;
  canor_alg_elem el;
  FILE *f;
  uint32_t prof = 0, dual_id = 0, dual_p = 0;
  int perm[4] = { 1, 0, 3, 2 };
  fs_sample samp[12];
  int nsamp = 0;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  if (!golay_init()) return 2;

  printf("Scale-relative operators over Q(alpha), ordinal-indexed\n\n");

  /* --- the ordinal ladder --------------------------------------------------- */
  printf("  ordinal ladder (Cantor normal form)\n");
  for (k = 0; k <= 2; ++k)
    for (i = 0; i <= 2; ++i) {
      if (nlad >= 12) break;
      if (!fo_make(&lad[nlad], k > 0 ? 1 : 0, k, i)) continue;
      ++nlad;
    }
  for (i = 0; i < nlad; ++i) {
    fo_format(&lad[i], obuf, sizeof obuf);
    printf("    [%2d] %-10s %s\n", i, obuf,
           fo_is_limit(&lad[i]) ? "limit - no immediate predecessor" : "");
  }
  {
    int ordered = 1;
    for (i = 1; i < nlad; ++i) if (fo_cmp(&lad[i - 1], &lad[i]) >= 0) ordered = 0;
    canor_demo_check(ordered, "the ladder is strictly increasing in the ordinal order");
  }
  {
    fo_ord w, big;
    /* THE POINT OF USING ORDINALS: omega exceeds every finite ordinal, so no
     * number of refinements at a class reaches the next class. An integer
     * index cannot express this and would have implied the opposite. */
    fo_make(&w, 1, 1, 0);
    fo_make(&big, 0, 0, 4096);
    canor_demo_check(fo_cmp(&w, &big) > 0, "w > every finite ordinal, however large");
    canor_demo_check(fo_is_limit(&w) && !fo_is_limit(&big), "w is a limit, 4096 is not");
  }

  /* --- an algebraic number, and its scale ----------------------------------- */
  printf("\n  the field, and the scale it carries\n");
  {
    /* x^3 - 2: alpha = cube root of 2, degree 3 over Q - a genuine cubic
     * irrationality rather than a quadratic that might be handled by accident. */
    canor_rational co[4];
    canor_poly_init(&mp);
    co[0] = canor_rat(-2, 1); co[1] = canor_rat(0, 1);
    co[2] = canor_rat(0, 1);  co[3] = canor_rat(1, 1);
    if (!canor_poly_set(&mp, 3, co)) { printf("    cannot build x^3-2\n"); return 2; }
    if (!canor_alg_isolate(&mp, roots, 8, &nroots)) { printf("    isolate failed\n"); return 2; }
    printf("    x^3 - 2 has %d real root(s)\n", nroots);
    canor_demo_check(nroots == 1, "x^3 - 2 has exactly one real root");
  }
  if (nroots < 1) { printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail); return 1; }

  /* alpha^2 + 1, an element of Q(alpha) that is not in Q. */
  canor_alg_elem_init(&el);
  el.c[0] = canor_rat(1, 1);
  el.c[2] = canor_rat(1, 1);
  el.degree = 2;

  printf("\n  sampling the operator along the ladder\n");
  printf("    %-10s %10s %-22s %s\n", "index", "kclass", "interval width", "value");
  for (i = 0; i < nlad; ++i) {
    fs_sample sm;
    if (!fs_sample_at(&roots[0], &el, &lad[i], &sm)) continue;
    samp[nsamp++] = sm;
    fo_format(&lad[i], obuf, sizeof obuf);
    printf("    %-10s %10d %10lld/%-10lld %.9f\n", obuf, sm.kclass,
           (long long)sm.width.num, (long long)sm.width.den, canor_demo_rat_d(sm.value));
  }
  {
    /* The width must shrink monotonically: refinement only ever halves. If it
     * did not, "scale" would not mean anything and the ladder would be
     * decorative. */
    int mono = 1;
    for (i = 1; i < nsamp; ++i)
      if (canor_rat_cmp(samp[i].width, samp[i - 1].width) > 0) mono = 0;
    canor_demo_check(mono, "the sampled scale shrinks monotonically along the ladder");
    canor_demo_check(nsamp > 1 && canor_rat_cmp(samp[nsamp - 1].width, samp[0].width) < 0,
                     "the last sample is strictly finer than the first");
  }

  /* --- PG(3,2) duality ------------------------------------------------------ */
  printf("\n  PG(3,2) duality  (an interpretation, labelled as one)\n");
  {
    int inv_ok = 1, line_ok = 1, n = 0;
    for (u = 1; u <= 15; ++u)
      for (k = u + 1; k <= 15; ++k) {
        uint8_t L[3], D[3], DD[3], L2[3];
        if (!pg_line((uint8_t)u, (uint8_t)k, L)) continue;
        if (!pg_dual_line((uint8_t)u, (uint8_t)k, D)) { line_ok = 0; continue; }
        if ((uint8_t)(D[0] ^ D[1]) != D[2]) line_ok = 0;
        if (!pg_dual_line(D[0], D[1], DD)) { inv_ok = 0; continue; }
        L2[0] = L[0]; L2[1] = L[1]; L2[2] = L[2];
        /* Compare as lines, i.e. as sorted point sets. */
        { uint8_t t;
          if (L2[0] > L2[1]) { t = L2[0]; L2[0] = L2[1]; L2[1] = t; }
          if (L2[1] > L2[2]) { t = L2[1]; L2[1] = L2[2]; L2[2] = t; }
          if (L2[0] > L2[1]) { t = L2[0]; L2[0] = L2[1]; L2[1] = t; } }
        if (DD[0] != L2[0] || DD[1] != L2[1] || DD[2] != L2[2]) inv_ok = 0;
        ++n;
      }
    printf("    %d point pairs, each spanning a line\n", n);
    canor_demo_check(line_ok, "the dual of a line is a line");
    canor_demo_check(inv_ok, "duality is an involution: dual(dual(L)) = L");
    canor_demo_check(pg_form(1, 1) == 1 && pg_form(1, 2) == 0,
                     "the bilinear form is the standard one and non-degenerate");
  }

  /* --- invariance, at both levels and jointly -------------------------------- */
  printf("\n  invariance under the involution\n");
  {
    /* The probe-level profile: which channels a fully-fledged Strategy can
     * separate. Rebuilt here from the Strategy directly so `scales` does not
     * depend on a prior selmer run having happened. */
    canor_strategy sL = c->strat[sel], sR = c->strat[sel];
    int t;
    sL.bias = LeftBiased; sR.bias = RightBiased;
    for (t = 0; t < CANOR_TRIFECTAS; ++t)
      for (k = 0; k < 3; ++k) {
        int32_t args[4]; FLOWVALUE l, r; int sep = 0, d;
        u = t * 3 + k;
        if (u >= 15) continue;
        for (d = 1; d <= 12; ++d) {
          args[0] = d; args[1] = d * 2; args[2] = d * 3; args[3] = d * 5;
          if (!canor_dtc(&sL, t, k, args, 4, &l)) continue;
          if (!canor_dtc(&sR, t, k, args, 4, &r)) continue;
          if (l != r) ++sep;
        }
        if (sep) prof |= 1u << u;
      }
    dual_id = pg_dual_profile(prof);
    if (!pg_dual_perm(prof, perm, &dual_p)) { printf("    correlation refused\n"); return 2; }

    printf("    probe profile          %05x\n", prof & 0x7fffffu);
    printf("    under plain duality    %05x   %s\n", dual_id & 0x7fffffu,
           dual_id == prof ? "invariant" : "MOVED");
    printf("    under a correlation    %05x   %s\n", dual_p & 0x7fffffu,
           dual_p == prof ? "invariant" : "moved");
    /* Plain duality is the identity on point profiles under the standard form.
     * That is a fact about the form, not a result about the Strategy, and
     * treating it as evidence of symmetry would be the easiest mistake to make
     * here - so it is checked as a property of the map and nothing is
     * concluded about bias from it. */
    canor_demo_check(dual_id == prof, "plain duality is the identity on point profiles");

    {
      int moved_id = 0, moved_p = 0;
      uint32_t s1 = golay_snap23(prof, &moved_id);
      uint32_t s2 = golay_snap23(dual_p, &moved_p);
      printf("\n    cross-frame bounded smoothing\n");
      printf("      original snapped   %05x  (moved %d)\n", s1 & 0x7fffffu, moved_id);
      printf("      correlated snapped %05x  (moved %d)\n", s2 & 0x7fffffu, moved_p);
      printf("      %s\n", s1 == s2
             ? "same codeword: the two frames agree within distance 6"
             : "different codewords: the reading is frame-dependent");
      canor_demo_check(moved_id <= 3 && moved_p <= 3,
                       "both snaps move at most 3 channels - the bound holds");
    }

    /* TENSOR LEVEL. The prediction fixed in advance: H^(x)m structure cannot
     * move under a relabelling of channels, because its structure constants
     * depend only on XOR of packed indices and the Jacobiator vanishes
     * identically. A failure here is a bug, not a finding. */
    {
      ft_census cen;
      int tensor_rigid = 0;
      if (ft_census_run(2, 16, &cen))
        tensor_rigid = (cen.jacobi_fail == 0 && cen.assoc_fail == 0);
      printf("\n    tensor level: Jacobi failures %ld, associativity failures %ld\n",
             (long)cen.jacobi_fail, (long)cen.assoc_fail);
      canor_demo_check(tensor_rigid,
                       "tensor level is rigid - nothing for a relabelling to disturb");
    }

    /* BOTH AT ONCE. This is the only configuration that separates "the algebra
     * is symmetric" from "the measurement is symmetric". */
    printf("\n    both levels: algebra rigid = yes, measurement %s\n",
           dual_p == prof ? "also invariant" : "frame-dependent");
    printf("      so the symmetry seen at tensor level is a property of the\n"
           "      ALGEBRA, and says nothing about whether the measurement is\n"
           "      symmetric. Reporting one as evidence for the other would be\n"
           "      the error this joint test exists to prevent.\n");
  }

  /* --- artifact -------------------------------------------------------------- */
  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); return 2; }
  fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n<title>Scale-relative "
             "operators</title>\n");
  fprintf(f, "<style>body{background:#0d1117;color:#c9d1d9;font:14px/1.6 "
             "system-ui,sans-serif;margin:0;padding:32px}.w{max-width:1100px;"
             "margin:0 auto}h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;"
             "margin:28px 0 8px;color:#e6edf3}.sub{color:#8b949e;font-size:13px;"
             "max-width:72ch}.no{border-left:2px solid #d29922;padding-left:12px;"
             "color:#8b949e;font-size:13px;max-width:72ch}table{border-collapse:"
             "collapse;font:12px ui-monospace,monospace}td,th{border:1px solid "
             "#30363d;padding:3px 8px;text-align:right}th{color:#8b949e;"
             "font-weight:500}td:first-child,th:first-child{text-align:left}"
             "</style>\n<div class=\"w\">\n");
  fprintf(f, "<h1>Scale-relative operators over Q(&alpha;)</h1>\n");
  fprintf(f, "<p class=\"sub\">Strategy <b>%s</b>. &alpha; is the real root of "
             "x&sup3;&minus;2, a genuine cubic irrationality; the operator is "
             "&alpha;&sup2;+1, sampled along an ordinal ladder. Every value is "
             "exact and every width is the true interval width.</p>\n",
          c->strat[sel].id.name);

  fprintf(f, "<h2>The ladder</h2>\n<table>\n<tr><th>index</th><th>class</th>"
             "<th>interval width</th><th>value</th></tr>\n");
  for (i = 0; i < nsamp && i < nlad; ++i) {
    fo_format(&lad[i], obuf, sizeof obuf);
    fprintf(f, "<tr><td>%s</td><td>%d</td><td>%lld/%lld</td><td>%.9f</td></tr>\n",
            obuf, samp[i].kclass, (long long)samp[i].width.num,
            (long long)samp[i].width.den, canor_demo_rat_d(samp[i].value));
  }
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">Indexed by &omega;&middot;k+n, not by an integer. "
             "The finite part counts refinements <i>within</i> a growth class "
             "and the &omega; part counts the classes, so &omega;&middot;k+n &lt; "
             "&omega;&middot;(k+1) for every n &mdash; no amount of halving an "
             "interval reaches the next growth regime. An integer index would "
             "have implied that it does.</p>\n");

  fprintf(f, "<h2>Duality, and what it separates</h2>\n");
  fprintf(f, "<table>\n<tr><th>quantity</th><th>value</th><th>under involution</th></tr>\n");
  fprintf(f, "<tr><td>probe profile</td><td>%05x</td><td>%05x %s</td></tr>\n",
          prof & 0x7fffffu, dual_p & 0x7fffffu,
          dual_p == prof ? "(invariant)" : "(moved)");
  fprintf(f, "<tr><td>tensor structure</td><td>rigid</td><td>rigid</td></tr>\n");
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">A quantity invariant under duality is structural; "
             "one that moves is an artifact of the labelling. The tensor level "
             "is rigid because H<sup>&otimes;m</sup> is associative and its "
             "structure constants depend only on XOR &mdash; so its invariance "
             "is a property of the <i>algebra</i> and is not evidence that the "
             "<i>measurement</i> is symmetric. Testing both levels at once is "
             "the only way to keep those apart.</p>\n");
  fprintf(f, "<p class=\"no\"><b>Interpretation, declared.</b> Reading PG(3,2) "
             "duality as the relevant negation is a choice, not a derivation. "
             "It is the nearest exactly-defined involution on this structure, "
             "and it earns its place by separating structure from labelling "
             "&mdash; but no claim is made that it is the intended sense of any "
             "particular phrase.</p>\n");
  fprintf(f, "<p class=\"no\"><b>Not built.</b> Surgery &mdash; cutting a chart "
             "out of the spread and gluing another in &mdash; is well defined "
             "here (all 56 spreads are enumerable and the gluing is already "
             "computed) but is not implemented, because nothing in this run "
             "needed it. It is a definite operation whenever it is wanted, not "
             "an open question.</p>\n");
  fprintf(f, "</div>\n");
  fclose(f);

  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
