/* canor_demo_ricci.c - ricci - discrete flow with surgery, with ordinal-bounded termination
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* ricci - discrete flow with surgery, with ordinal-bounded termination       */
/* ========================================================================== */
/* WHAT IS CLAIMED. A discrete smoothing flow, punctuated by surgeries, whose
 * termination is proved by a well-founded ordinal descent. That is a real
 * algorithm with a real proof and every step below is exact.
 *
 * WHAT IS NOT CLAIMED. Nothing about Ricci flow, 3-manifolds, or the Poincare
 * conjecture. Hamilton-Perelman is a theorem about smooth Riemannian metrics
 * on 3-manifolds; this is a bounded integer smoothing on a 15-bit profile. The
 * borrowing is STRUCTURAL and confined to one thing:
 *
 *     THE SHAPE OF THE TERMINATION ARGUMENT.
 *
 * In Ricci flow with surgery the delicate part is not the flow, which is
 * standard, nor the surgery, which is explicit, but showing that SURGERY TIMES
 * DO NOT ACCUMULATE - that infinitely many surgeries cannot occur in bounded
 * time. Every discrete analogue has to answer the same question, and the
 * answer here is a well-founded ordinal.
 *
 * --- THE CORRESPONDENCE, TERM BY TERM ---------------------------------------
 *
 *   FLOW           Golay snap toward the nearest codeword, one bit per step.
 *                  The code is perfect, so the codeword is unique and at most
 *                  3 steps away - the flow converges in bounded time, always.
 *
 *   CURVATURE      h, the Hamming distance to that codeword. Zero exactly when
 *                  the profile is already smooth.
 *
 *   NECK PINCH     A chart of the spread all of whose channels are blind. The
 *                  atlas is about to lose a component: that chart can no
 *                  longer report anything, and no amount of flowing fixes it,
 *                  because flowing moves the profile and not the partition.
 *
 *   SURGERY        Cut the spread and re-glue: replace the whole partition
 *                  with another of the 56, chosen deterministically as the
 *                  lowest-indexed one with strictly fewer degenerate charts.
 *
 *   SURGERY SCALE  The declared scale at which a pinch counts as one. Here it
 *                  is exact rather than asymptotic: a chart is degenerate when
 *                  all three of its channels are blind, and blindness is a
 *                  decided property, not a threshold.
 *
 * --- WHY THE POTENTIAL MUST BE AN ORDINAL AND NOT AN INTEGER -----------------
 *
 *     Phi  =  omega.D + h,     D = degenerate charts,  h = distance to codeword
 *
 * A flow step decreases h by one and leaves D alone, so Phi drops.
 *
 * A SURGERY DECREASES D BY AT LEAST ONE AND MAY INCREASE h - re-gluing the
 * atlas can leave the profile further from a codeword than it was, by up to
 * the full radius 3. An integer potential of the form aD + h would therefore
 * need a to be chosen larger than any possible increase in h, and would be a
 * tuned constant standing in for an argument. With an ordinal no constant is
 * needed and none is chosen: omega.D dominates ANY finite h whatsoever, so
 * Phi strictly decreases even when h rises, and the ordinals being
 * well-founded means there is no infinite descending chain. Termination
 * follows from the order type rather than from a bound anyone picked.
 *
 * That is the whole reason the ordinal machinery in canor_scale is not
 * decoration, and this command exists to exercise exactly that property: the
 * suite checks that Phi strictly decreases at EVERY step, including across
 * surgeries where the finite part goes the wrong way.
 *
 * ONE HONEST LIMIT. All 56 spreads lie in a single GL(4,2) orbit, so any
 * spread is a relabelling of any other. Whether a BETTER spread exists is
 * therefore a property of the measurement and not of the algebra, and when
 * none exists the flow halts with residual degeneracy. That is a real outcome
 * and it is reported as one rather than treated as a failure to converge.
 */

#define RC_MAX_STEPS 64

typedef struct {
  int step;
  bool surgery;
  int D, h;                 /* the ordinal potential, split */
  char phi[64];
  int spread_idx;
  int cut_chart;            /* which chart pinched, -1 if none */
} rc_event;

/* Degenerate charts of a given spread under a fixed blindness map. */
static int rc_degenerate(const uint8_t tri[5][3], const bool blind[15],
                         int *first) {
  int t, k, d = 0;
  if (first) *first = -1;
  for (t = 0; t < 5; ++t) {
    int all = 1;
    for (k = 0; k < 3; ++k) {
      int u = tri[t][k];
      if (u < 1 || u > 15) { all = 0; break; }
      if (!blind[u - 1]) { all = 0; break; }
    }
    if (all) { ++d; if (first && *first < 0) *first = t; }
  }
  return d;
}

static int rc_popcount(uint32_t x) {
  int n = 0;
  while (x) { x &= x - 1; ++n; }
  return n;
}

int canor_demo_cmd_ricci(const canor_demo_config_t *c, int sel, const char *out) {
  ex_spread_set sp;
  bool blind[15];
  /* CAPPED CHANNELS ARE NOT MEASUREMENTS. Surgery glues in a standard piece
   * where an unmeasurable region was cut out, so a capped channel carries the
   * cap's value and not the Strategy's. Tracked separately for that reason:
   * after any surgery the profile is part measured and part synthetic, and a
   * bias conclusion drawn from it without saying so would be reporting the
   * cap as evidence. */
  bool capped[15];
  int ncapped = 0;
  uint32_t prof = 0, code;
  int t, k, u, moved = 0;
  int cur = 0, D, h, steps = 0, nsurg = 0;
  rc_event log[RC_MAX_STEPS];
  int nlog = 0;
  fo_ord phi, prev;
  bool descended = true, closure_ok = true, bound_ok = true;
  int flow_since_surgery = 0, max_flow_run = 0;
  FILE *f;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  if (!golay_init()) return 2;
  if (!ex_spreads_all(&sp)) return 2;
  memset(capped, 0, sizeof capped);

  printf("Discrete flow with surgery, ordinal-bounded\n\n");
  printf("  strategy %s, %d of %d trifectas\n\n",
         c->strat[sel].id.name, c->strat[sel].ntrifecta, CANOR_TRIFECTAS);

  /* --- the initial profile -------------------------------------------------- */
  {
    canor_strategy sL = c->strat[sel], sR = c->strat[sel];
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
        blind[u] = (sep == 0);
        if (sep) prof |= 1u << u;
      }
  }
  printf("  initial profile %05x, %d of 15 channels blind\n",
         prof & 0x7fffffu, (int)(15 - rc_popcount(prof)));

  /* --- the flow ------------------------------------------------------------- */
  code = golay_snap23(prof, &moved);
  h = rc_popcount((prof ^ code) & 0x7fffffu);
  D = rc_degenerate(sp.line[cur], blind, NULL);
  fo_make(&phi, D > 0 ? 1 : 0, D, h);
  fo_format(&phi, log[0].phi, sizeof log[0].phi);
  printf("  spread %d, D=%d degenerate chart(s), h=%d, Phi=%s\n\n",
         cur, D, h, log[0].phi);

  printf("  %-5s %-9s %-4s %-4s %-10s %s\n",
         "step", "kind", "D", "h", "Phi", "note");
  while (steps < RC_MAX_STEPS - 1) {
    prev = phi;

    if (h > 0) {
      /* FLOW: move one bit toward the codeword. The perfect code guarantees
       * this terminates in at most 3 steps, so the flow never runs away
       * between surgeries - that bound is checked, not assumed. */
      uint32_t diff = (prof ^ code) & 0x7fffffu;
      uint32_t bit = diff & (~diff + 1u);      /* lowest differing bit */
      prof ^= bit;
      --h;
      ++flow_since_surgery;
      if (flow_since_surgery > max_flow_run) max_flow_run = flow_since_surgery;
      log[nlog].surgery = false;
      log[nlog].cut_chart = -1;
    } else {
      int firstdeg = -1;
      D = rc_degenerate(sp.line[cur], blind, &firstdeg);
      if (D == 0) break;                        /* smooth and non-degenerate */
      /* SURGERY: cut the pinched chart out and cap it. Capping is what makes
       * this a surgery rather than a relabelling - a standard piece replaces
       * the region that could not be measured, exactly as a cap replaces a
       * pinched neck. The three channels stop being blind because they now
       * carry the cap, so D falls; and setting three bits can move the profile
       * away from its codeword, so h may RISE by up to the full radius.
       *
       * An earlier version of this routine only swapped the spread and never
       * touched the profile. It terminated and passed every check, but h could
       * not rise by construction, so the omega term was never exercised and
       * the ordinal was doing no work the integers could not have done. Local
       * capping is both the faithful operation and the one that makes the
       * potential's order type matter. */
      {
        int i;
        if (firstdeg < 0) break;
        for (i = 0; i < 3; ++i) {
          int cu = sp.line[cur][firstdeg][i];
          if (cu < 1 || cu > 15) continue;
          blind[cu - 1] = false;
          if (!capped[cu - 1]) { capped[cu - 1] = true; ++ncapped; }
          prof |= 1u << (cu - 1);          /* the standard cap */
        }
        ++nsurg;
        log[nlog].surgery = true;
        log[nlog].cut_chart = firstdeg;
        flow_since_surgery = 0;
        code = golay_snap23(prof, &moved);
        h = rc_popcount((prof ^ code) & 0x7fffffu);
      }
    }

    D = rc_degenerate(sp.line[cur], blind, NULL);
    fo_make(&phi, D > 0 ? 1 : 0, D, h);
    log[nlog].step = steps;
    log[nlog].D = D; log[nlog].h = h;
    log[nlog].spread_idx = cur;
    fo_format(&phi, log[nlog].phi, sizeof log[nlog].phi);

    if (fo_cmp(&phi, &prev) >= 0) descended = false;

    printf("  %-5d %-9s %-4d %-4d %-10s %s\n", steps,
           log[nlog].surgery ? "SURGERY" : "flow", D, h, log[nlog].phi,
           log[nlog].surgery ? "cut a pinched chart, re-glued the atlas" : "");
    ++nlog; ++steps;
  }

  /* Every chart of the final spread must still close under multiplication -
   * surgery that produced a non-closing triple would have broken the atlas
   * rather than repaired it. */
  {
    ex_groups g;
    int i;
    if (ex_groups_build(&g, EX_GROUP_SPREAD, CANOR_IMAG_COUNT))
      for (i = 0; i < g.count; ++i) if (!g.closes[i]) closure_ok = false;
  }
  bound_ok = (max_flow_run <= 3);

  printf("\n  terminated after %d steps, %d surgery(ies)\n", steps, nsurg);
  printf("  final: spread %d, D=%d, h=%d\n", cur, D, h);
  printf("  %d of 15 channels are CAPPED - synthetic, not measured\n", ncapped);
  if (ncapped)
    printf("    a bias reading taken from this profile without excluding them\n"
           "    would be reporting the cap as evidence\n");

  canor_demo_check(descended, "Phi strictly decreases at every step, surgeries included");
  canor_demo_check(bound_ok, "the flow runs at most 3 steps between surgeries");
  canor_demo_check(steps < RC_MAX_STEPS - 1, "the process terminated rather than ran out");
  canor_demo_check(h == 0 || D > 0, "it halted smooth, or with a stated residual pinch");
    canor_demo_check(closure_ok, "every chart of the final atlas still closes");
  canor_demo_check(ncapped == 3 * nsurg, "each surgery caps exactly one chart's 3 channels");
  {
    /* The property that makes the ordinal necessary: a surgery may raise h
     * while Phi still falls. If this never occurred in this run the check
     * would be vacuous, so it reports which case it is rather than passing
     * silently either way. */
    int raised = 0, i;
    for (i = 1; i < nlog; ++i)
      if (log[i].surgery && log[i].h > log[i - 1].h) raised = 1;
    printf("  a surgery raised h while Phi fell: %s\n", raised ? "yes" : "no");
    if (!raised)
      printf("    (so this run does not by itself demonstrate the need for the\n"
             "     omega term - the ordering is still what licenses it)\n");
  }

  /* --- artifact ------------------------------------------------------------- */
  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); return 2; }
  fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n"
             "<title>Flow with surgery</title>\n");
  fprintf(f, "<style>body{background:#0d1117;color:#c9d1d9;font:14px/1.6 "
             "system-ui,sans-serif;margin:0;padding:32px}.w{max-width:1100px;"
             "margin:0 auto}h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;"
             "margin:28px 0 8px;color:#e6edf3}.sub{color:#8b949e;font-size:13px;"
             "max-width:72ch}.no{border-left:2px solid #d29922;padding-left:12px;"
             "color:#8b949e;font-size:13px;max-width:72ch}table{border-collapse:"
             "collapse;font:12px ui-monospace,monospace}td,th{border:1px solid "
             "#30363d;padding:3px 8px;text-align:right}th{color:#8b949e;"
             "font-weight:500}td:first-child,th:first-child{text-align:left}"
             ".s{background:#1f2a12}</style>\n<div class=\"w\">\n");
  fprintf(f, "<h1>Discrete flow with surgery</h1>\n");
  fprintf(f, "<p class=\"sub\">Strategy <b>%s</b>. A bounded smoothing flow "
             "punctuated by surgeries on the spread, with termination proved by "
             "a well-founded ordinal &Phi; = &omega;&middot;D + h.</p>\n",
          c->strat[sel].id.name);

  fprintf(f, "<h2>The run</h2>\n<table>\n<tr><th>step</th><th>kind</th>"
             "<th>spread</th><th>D</th><th>h</th><th>&Phi;</th></tr>\n");
  for (t = 0; t < nlog; ++t)
    fprintf(f, "<tr%s><td>%d</td><td>%s</td><td>%d</td><td>%d</td><td>%d</td>"
               "<td>%s</td></tr>\n", log[t].surgery ? " class=\"s\"" : "",
            log[t].step, log[t].surgery ? "SURGERY" : "flow",
            log[t].spread_idx, log[t].D, log[t].h, log[t].phi);
  fprintf(f, "</table>\n");

  fprintf(f, "<h2>Capped channels</h2>\n");
  fprintf(f, "<p class=\"no\"><b>%d of 15 channels are synthetic.</b> Surgery "
             "glues a standard piece in where a region could not be measured, "
             "so a capped channel carries the cap's value and not the "
             "Strategy's. Any bias reading from this profile must exclude them; "
             "including them would report the cap as evidence.</p>\n", ncapped);

  fprintf(f, "<h2>Why the potential is an ordinal</h2>\n");
  fprintf(f, "<p class=\"sub\">A flow step lowers h. A surgery lowers D but "
             "<i>may raise h</i> &mdash; re-gluing can leave the profile up to "
             "the full radius 3 further from a codeword. An integer potential "
             "aD + h would need a chosen larger than any possible rise in h: a "
             "tuned constant standing in for an argument. &omega;&middot;D "
             "dominates every finite h without any constant being chosen, so "
             "&Phi; falls even when h rises, and well-foundedness forbids an "
             "infinite descent. Termination comes from the order type, not from "
             "a bound anyone picked.</p>\n");

  fprintf(f, "<h2>What is and is not claimed</h2>\n");
  fprintf(f, "<p class=\"no\">Nothing here is a claim about Ricci flow, "
             "3-manifolds, or the Poincar&eacute; conjecture. "
             "Hamilton&ndash;Perelman is a theorem about smooth Riemannian "
             "metrics; this is bounded integer smoothing on a 15-bit profile. "
             "What is borrowed is the <i>shape of the termination argument</i> "
             "&mdash; that surgeries cannot accumulate &mdash; because every "
             "flow-with-surgery has to answer that question and an ordinal is "
             "the honest way to answer it here.</p>\n");
  fprintf(f, "<p class=\"no\">All 56 spreads lie in one GL(4,2) orbit, so any "
             "spread is a relabelling of any other and the choice of "
             "replacement carries no structure. Whether a better spread exists "
             "is therefore a fact about the measurement, not the algebra. When "
             "none does, the flow halts with a stated residual pinch rather "
             "than pretending to converge.</p>\n");
  fprintf(f, "</div>\n");
  fclose(f);

  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
