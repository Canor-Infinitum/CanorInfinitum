/* canor_demo_selmer.c - selmer - Selmer-Lie bracketed differentials as an atlas of TST trees
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* selmer - Selmer-Lie bracketed differentials as an atlas of TST trees       */
/* ========================================================================== */
/* Each differential d_u of a fully-fledged Strategy BECOMES AN canor_tst NODE:
 * built with canor_tst_add, linked with canor_tst_link, folded with
 * canor_tst_absolute_origo, classified with canor_origo_classify. Nothing is drawn
 * that was not first constructed and folded.
 *
 * WHY FIVE TREES AND NOT ONE. The spread partitions the fifteen units into
 * five triples with no common root. Inventing a root would assert a
 * containment the algebra does not have, so the atlas is five trees glued by
 * brackets - the same five charts HP^4 draws. WORLD_MODEL Sec.7 requires every
 * recursion to identify its refinement dimension and forbids mixing them; the
 * dimension here is STRATEGY SPECIALIZATION, one of the listed kinds, and it
 * is the only one used.
 *
 * PER-CHANNEL CAYLEY-DICKSON EMBEDDING. Unit u first exists at the doubling
 * reaching index u: 1..3 quaternionic, 4..7 octonionic, 8..15 sedenionic. Not
 * a label - it fixes which laws that channel's differential may rely on. H is
 * associative and satisfies Jacobi; O is alternative only; S has zero
 * divisors. Two channels reporting the same number are NOT making the same
 * claim if they sit at different levels, so the level is on every node.
 *
 * THE ZERO-SET. [e_a,e_b] is 0 or +-2.e_(a XOR b) and never anything else,
 * because a product of basis elements is always +-e_(a XOR b). The vanishing
 * pairs are drawn as absent gluing, not as faint gluing: a commuting pair is
 * no link, and shading it would misstate the algebra.
 *
 * --- WHAT THE TWO MODELS REQUIRE, AND WHERE THIS OBEYS THEM ----------------
 *
 * Reading the specifications properly changed this tool in four ways, and each
 * change is a restriction rather than an addition.
 *
 *   PRE/PERI/POST IS ONE CYCLE, NOT THREE READINGS. EVOLUTIONARY_MODEL Sec.11
 *   assigns distinct work to each stage: PRE estimates Stress and projects
 *   Flow, PERI computes the PFISD components, POST observes the resulting
 *   Stress and calculates the residual. Measuring one quantity three times
 *   under permuted inputs and labelling the results PRE/PERI/POST would have
 *   been three parallel readings wearing the names of a cycle. So the stages
 *   below do different work, in order.
 *
 *   STRESS IS A VECTOR. Sec.4: "Stress SHALL NOT be treated as one universal
 *   scalar where channel separation is materially relevant" - and channel
 *   separation is the entire purpose of this tool. Stress is therefore
 *   per-channel and typed (verification stress, from the Sec.4 list), and no
 *   scalar Stress is ever formed. WORLD_MODEL Sec.9.4 says the same thing from
 *   the other side: an aggregate SHALL NOT erase node-level disagreement.
 *
 *   THE GOVERNOR'S OUTPUT IS REFUSED. Sec.3 defines u = Saturate_C(u_raw)
 *   where C is the authority-, rights-, resource- and safety-constrained
 *   action set, and Sec.5.4 adds that the Strategy term SHALL NOT create its
 *   own authority. This tool has no declared C. It therefore reports the
 *   governor's INPUTS and declines to emit u. P and I are likewise not
 *   computed: both need the control error e = s - s_ref, and Sec.4.2 forbids
 *   assuming s_ref = 0 for adaptive systems - some require nonzero excitation.
 *   An undeclared target is not a zero target.
 *
 *   RGBA KEEPS ITS DECLARED MEANING. WORLD_MODEL Sec.4 fixes R as observed
 *   reality, G as informational gap, B as foundational basis, A as
 *   admissibility. An earlier draft coloured nodes by phase, which would have
 *   overwritten those semantics with unrelated ones in the one place a reader
 *   would trust them. The mapping below is the declared one.
 *
 * L_m ENTERS AT THE ONLY SLOT THAT FITS. Sec.5.5 splits Derivation into a
 * TemporalDerivation and a SemanticDerivationResidual r_Sigma, "a declared
 * Strategy-satisfaction, derivation, or model-residual term". That is exactly
 * what L_m is, so D runs with lambda_t = 0 (there is no time series here) and
 * lambda_Sigma = 1. This is the sanctioned home for the residual rather than a
 * place found for it.
 *
 * BOUNDED SMOOTHING. The sighted-channel profile snaps to the nearest Golay
 * codeword. The [23,12,7] code is perfect, so the snap moves at most three
 * channels ALWAYS and two profiles snapping together are within six. That is
 * what makes this a manifold with a stated resolution rather than a scatter:
 * the radius is proved, not tuned.
 *
 * WHAT IS MEASURED IS THE INSTRUMENT. Capacity is how much Left/Right signal a
 * differential family COULD carry, not how much bias was found. A blind
 * channel reports any subject as unbiased; WORLD_MODEL Sec.9.2 requires a
 * missing facet to read `unknown` or `not applicable` and "never silently
 * zero", so blind channels are named and excluded. Bias stays relational: this
 * compares Strategies through a shared profile and fixes no absolute zero,
 * because the ensemble work showed a single Strategy has none to fix.
 */

#define SL_CHANNELS 15

typedef struct {
  /* state x: separation capacity, per-mille of DEFINED items */
  int cap[SL_CHANNELS];
  /* PRE: Stress s (per-mille), typed verification stress; Flow f = 1 - s */
  int stress[SL_CHANNELS], flow[SL_CHANNELS];
  long seen[SL_CHANNELS], def[SL_CHANNELS];
  /* PERI: the components that are computable without a declared s_ref or C */
  int F_term[SL_CHANNELS];        /* K_f . f, K_f = +1 declared */
  int sigma_bar[SL_CHANNELS];     /* 2.sigma - 1, in per-mille, signed */
  /* POST: the semantic derivation residual r_Sigma = L_m */
  canor_resid_class rcls[SL_CHANNELS];
  int64_t rlog2[SL_CHANNELS];
  bool applicable[SL_CHANNELS];   /* false => `not applicable`, never zero */
  /* THE ZERO-SET THAT ACTUALLY CARRIES STRUCTURE. The bracket zero-set over
   * the imaginary channels is EMPTY - distinct imaginary units anticommute at
   * every Cayley-Dickson level, so [e_u,e_v] never vanishes for u != v and the
   * only vanishing brackets are the diagonal. That is a fact about the algebra
   * and not a gap in the measurement, so it is reported rather than worked
   * around. What does vanish selectively is the JACOBIATOR: at dimension 16
   * Jacobi holds on some triples and fails on others, and the split is exactly
   * the named zero-set a smoothing manifold can be extracted over. Counted per
   * channel so it joins the profile instead of sitting beside it. */
  long jac_zero[SL_CHANNELS], jac_tot[SL_CHANNELS];
  long asc_zero[SL_CHANNELS];
  int cd[SL_CHANNELS], chart[SL_CHANNELS];
  uint32_t profile;
  canor_tst tree[CANOR_TRIFECTAS];
  int node[SL_CHANNELS];
  canor_origo abs[SL_CHANNELS];
  canor_interval_class cls[SL_CHANNELS];
  long dropped;                   /* Sec.4.1 lossy-transformation record */
} sl_atlas;

/* The doubling at which unit u first exists. Computed, not tabled, so it stays
 * correct if EX_MAX_DIM ever grows. */
static int sl_cd_dim(int u) {
  int d = 2;
  while (d <= EX_MAX_DIM && u >= d) d *= 2;
  return d;
}

static bool sl_build(const canor_demo_config_t *c, int sel, const canor_demo_cloud_point_t *pts, long n,
                     sl_atlas *at) {
  canor_strategy sL = c->strat[sel], sR = c->strat[sel];
  ex_groups spread;
  int t, k, u;
  long i;

  memset(at, 0, sizeof *at);
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return false;
  sL.bias = LeftBiased;
  sR.bias = RightBiased;

  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
    for (k = 0; k < 3; ++k) {
      int unit = spread.u[t][k];
      if (unit < 1 || unit > SL_CHANNELS) continue;
      at->chart[unit - 1] = t;
      at->cd[unit - 1] = sl_cd_dim(unit);
    }

  /* --- PRE: observe state, estimate Stress, project Flow ------------------ */
  for (t = 0; t < CANOR_TRIFECTAS; ++t)
    for (k = 0; k < 3; ++k) {
      long sep = 0, def = 0, seen = 0;
      u = t * 3 + k;
      if (u >= SL_CHANNELS) continue;
      for (i = 0; i < n; ++i) {
        char word[CANOR_FLOW_WORD_MAX];
        int cnt[3];
        int32_t args[4];
        FLOWVALUE l, r;
        if (canor_demo_cloud_word(c, &pts[i], word, CANOR_FLOW_WORD_MAX, cnt) == 0) continue;
        args[0] = pts[i].x; args[1] = pts[i].y;
        args[2] = pts[i].z; args[3] = pts[i].a;
        ++seen;
        if (!canor_dtc(&sL, t, k, args, 4, &l)) continue;
        if (!canor_dtc(&sR, t, k, args, 4, &r)) continue;
        ++def;
        if (l != r) ++sep;
      }
      at->seen[u] = seen;
      at->def[u] = def;
      /* STRESS IS THE REFUSAL FRACTION, and it is one typed channel -
       * verification stress from the Sec.4 list - not a blend of several. A
       * differential that refuses an input cannot verify anything about it. */
      at->stress[u] = seen ? (int)(1000 * (seen - def) / seen) : 1000;
      at->flow[u] = 1000 - at->stress[u];
      at->cap[u] = def ? (int)(1000 * sep / def) : 0;
      /* Sec.9.2: a missing facet is `not applicable`, never silently zero. */
      at->applicable[u] = (def > 0) && (at->cap[u] > 0);
      if (at->applicable[u]) at->profile |= 1u << u;
      at->dropped += seen - def;
    }

  /* --- PERI: the PFISD components that a missing s_ref and C still allow --- */
  for (u = 0; u < SL_CHANNELS; ++u) {
    at->F_term[u] = at->flow[u];              /* K_f = +1, sign declared */
    at->sigma_bar[u] = 2 * at->cap[u] - 1000; /* 2.sigma - 1 */
  }

  /* --- POST: resulting observation and the semantic residual r_Sigma ------ */
  for (u = 0; u < SL_CHANNELS; ++u) {
    canor_residual res;
    /* n is the channel index and X is the channel's own Flow magnitude, so the
     * residual is a deterministic function of the measurement and of nothing
     * outside it. theta/phi are report-layer only and do not move the class. */
    if (canor_residual_L(CANOR_K_GENERAL, (uint64_t)(u + 1), (int64_t)at->flow[u],
                      0.0, 0.0, &res)) {
      at->rcls[u] = res.cls;
      at->rlog2[u] = res.log2_ratio_milli;
      canor_residual_free(&res);
    } else {
      at->rcls[u] = CANOR_RESID_UNDEFINED;
    }
  }

  /* --- the Jacobiator and associator zero-sets, per channel --------------- */
  for (u = 1; u <= SL_CHANNELS; ++u) {
    int a, b;
    for (a = 1; a <= SL_CHANNELS; ++a)
      for (b = a + 1; b <= SL_CHANNELS; ++b) {
        ex_vec j, x;
        if (a == u || b == u) continue;
        if (!ex_jacobiator((uint8_t)u, (uint8_t)a, (uint8_t)b, &j)) continue;
        ++at->jac_tot[u - 1];
        if (ex_vec_is_zero(&j)) ++at->jac_zero[u - 1];
        /* Reported separately because the associator is what fails FIRST: the
         * algebra stops being associative at dimension 8 and the Jacobiator
         * inherits that failure. Conflating them would credit the Jacobiator
         * with a distinction it did not make. */
        if (ex_associator((uint8_t)u, (uint8_t)a, (uint8_t)b, &x) &&
            ex_vec_is_zero(&x))
          ++at->asc_zero[u - 1];
      }
  }

  /* --- build the trees, for real ------------------------------------------ */
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t) {
    canor_origo zero;
    int root;
    zero.t = canor_rat(0, 1); zero.x = canor_rat(0, 1);
    zero.y = canor_rat(0, 1); zero.z = canor_rat(0, 1);
    canor_tst_init(&at->tree[t]);
    root = canor_tst_add(&at->tree[t], &zero);
    if (root < 0) return false;
    at->tree[t].root = root;
    for (k = 0; k < 3; ++k) {
      int unit = spread.u[t][k], ch, nd;
      canor_origo d;
      if (unit < 1 || unit > SL_CHANNELS) continue;
      ch = unit - 1;
      /* The ORIGO delta of a differential node IS its measurement: time is the
       * Flow projection, the spatial part is capacity and the CD level. The
       * Minkowski class then falls out of the data rather than being assigned
       * to it. Sec.8.4's world-model reading needs a declared latency
       * boundary, which this tool does not have - so the class is computed and
       * reported, and its operational interpretation is declined. */
      d.t = canor_rat(at->flow[ch], 1000);
      d.x = canor_rat(at->cap[ch], 1000);
      d.y = canor_rat(at->stress[ch], 1000);
      d.z = canor_rat(at->cd[ch], 16);
      nd = canor_tst_add(&at->tree[t], &d);
      if (nd < 0) return false;
      if (canor_tst_link(&at->tree[t], root, k, nd) != CANOR_OK) return false;
      at->node[ch] = nd;
      if (canor_tst_absolute_origo(&at->tree[t], nd, &at->abs[ch]) != CANOR_OK)
        return false;
      at->cls[ch] = canor_origo_classify(&at->abs[ch], true);
    }
  }
  return true;
}

const char *canor_demo_sl_cls_name(canor_interval_class k) {
  switch (k) {
    case CANOR_INTERVAL_TIMELIKE:  return "timelike";
    case CANOR_INTERVAL_NULL:      return "null";
    case CANOR_INTERVAL_SPACELIKE: return "spacelike";
    default:                    return "indeterminate";
  }
}

int canor_demo_cmd_selmer(const canor_demo_config_t *c, int sel, const char *path, long want,
                      const char *out) {
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0;
  /* HEAPED, NOT STACKED. Five canor_tst carry CANOR_BBST_MAX nodes each, so this
   * struct is far past any default stack and an automatic would fault before
   * the first measurement. */
  sl_atlas *at;
  ex_groups spread;
  FILE *f;
  int t, k, u, v, moved = 0, nblind = 0, ncommute = 0;
  uint32_t snapped;

  at = calloc(1, sizeof *at);
  if (!at) { fprintf(stderr, "out of memory\n"); return 2; }
  if (!canor_demo_cloud_get(c, sel, path, want, &pts, &n)) { free(at); return 2; }
  if (!golay_init()) { free(pts); free(at); return 2; }
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) { free(pts); free(at); return 2; }
  if (!sl_build(c, sel, pts, n, at)) { free(pts); free(at); return 2; }
  free(pts);

  for (u = 0; u < SL_CHANNELS; ++u) if (!at->applicable[u]) ++nblind;
  snapped = golay_snap23(at->profile, &moved);

  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); return 2; }

  fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n");
  fprintf(f, "<title>Selmer-Lie bracketed differentials</title>\n");
  fprintf(f, "<style>\n"
             "body{background:#0d1117;color:#c9d1d9;font:14px/1.6 system-ui,sans-serif;"
             "margin:0;padding:32px}\n"
             ".w{max-width:1180px;margin:0 auto}\n"
             "h1{font-size:20px;font-weight:600;margin:0 0 4px}\n"
             "h2{font-size:15px;font-weight:600;margin:30px 0 8px;color:#e6edf3}\n"
             ".sub{color:#8b949e;font-size:13px;max-width:72ch}\n"
             ".no{border-left:2px solid #d29922;padding-left:12px;color:#8b949e;"
             "font-size:13px;max-width:72ch}\n"
             "table{border-collapse:collapse;font:12px ui-monospace,monospace;margin:8px 0}\n"
             "td,th{border:1px solid #30363d;padding:3px 8px;text-align:right}\n"
             "th{color:#8b949e;font-weight:500}\n"
             "td:first-child,th:first-child{text-align:left}\n"
             ".na{color:#6e7681}\n"
             "svg{background:#010409;border:1px solid #30363d;border-radius:6px}\n"
             "</style>\n");
  fprintf(f, "<div class=\"w\">\n");
  fprintf(f, "<h1>Selmer&ndash;Lie bracketed differentials</h1>\n");
  fprintf(f, "<p class=\"sub\">Strategy <b>%s</b>, %d of %d trifectas declared, "
             "%ld probe items. Every differential below is an actual "
             "<code>canor_tst</code> node &mdash; added, linked, folded and "
             "classified through the same calls any other consumer of the type "
             "tower makes.</p>\n",
          c->strat[sel].id.name, c->strat[sel].ntrifecta, CANOR_TRIFECTAS, n);

  /* --- the atlas ---------------------------------------------------------- */
  fprintf(f, "<h2>Five TST trees, one per spread chart</h2>\n");
  fprintf(f, "<svg viewBox=\"0 0 800 250\" width=\"100%%\">\n");
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t) {
    double rx = 70.0 + t * 150.0, ry = 38.0;
    fprintf(f, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"13\" fill=\"#161b22\" "
               "stroke=\"%s\" stroke-width=\"2\"/>\n",
            rx, ry, spread.closes[t] ? "#3fb950" : "#f85149");
    fprintf(f, "<text x=\"%.0f\" y=\"%.0f\" fill=\"#c9d1d9\" font-size=\"11\" "
               "font-family=\"monospace\" text-anchor=\"middle\">c%d</text>\n",
            rx, ry + 4, t);
    for (k = 0; k < 3; ++k) {
      int unit = spread.u[t][k], ch;
      double nx, ny;
      if (unit < 1 || unit > SL_CHANNELS) continue;
      ch = unit - 1;
      nx = rx + (k - 1) * 42.0;
      ny = ry + 92.0;
      fprintf(f, "<line x1=\"%.0f\" y1=\"%.0f\" x2=\"%.0f\" y2=\"%.0f\" "
                 "stroke=\"#30363d\" stroke-width=\"1.5\"/>\n",
              rx, ry + 13, nx, ny - 15);
      /* RGBA AS THE WORLD MODEL DECLARES IT (Sec.4), not as display colour:
       * R observed reality, G informational gap, B foundational basis,
       * A admissibility. Sec.4 permits a visualization to map them to display
       * colours; it does not permit redefining them. */
      fprintf(f, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"15\" fill=\"rgb(%d,%d,%d)\" "
                 "fill-opacity=\"%.2f\" stroke=\"%s\" stroke-width=\"2\">"
                 "<title>%s  R reality %d  G gap %d  B basis %s  A %s</title>"
                 "</circle>\n",
              nx, ny,
              at->cap[ch] * 255 / 1000,
              at->stress[ch] * 255 / 1000,
              at->cd[ch] == 4 ? 85 : at->cd[ch] == 8 ? 170 : 255,
              at->applicable[ch] ? 1.0 : 0.25,
              at->applicable[ch] ? "#e6edf3" : "#6e7681",
              canor_imag_name((canor_imag)unit), at->cap[ch], at->stress[ch],
              at->cd[ch] == 4 ? "H" : at->cd[ch] == 8 ? "O" : "S",
              at->applicable[ch] ? "admissible" : "not applicable");
      fprintf(f, "<text x=\"%.0f\" y=\"%.0f\" fill=\"#8b949e\" font-size=\"10\" "
                 "font-family=\"monospace\" text-anchor=\"middle\">%s</text>\n",
              nx, ny + 31, canor_imag_name((canor_imag)unit));
      fprintf(f, "<text x=\"%.0f\" y=\"%.0f\" fill=\"#6e7681\" font-size=\"9\" "
                 "font-family=\"monospace\" text-anchor=\"middle\">%s</text>\n",
              nx, ny + 43,
              at->cd[ch] == 4 ? "H" : at->cd[ch] == 8 ? "O" : "S");
    }
  }
  fprintf(f, "</svg>\n");
  fprintf(f, "<p class=\"sub\">Five trees and not one: the spread has no common "
             "root, and inventing one would assert a containment the algebra "
             "does not have. The refinement dimension is <b>Strategy "
             "specialization</b> and it is the only one used, as WORLD_MODEL "
             "&sect;7 requires. Colour is the declared RGBA profile of &sect;4 "
             "&mdash; R observed reality, G informational gap, B foundational "
             "basis (H/O/S), A admissibility, shown as opacity &mdash; not a "
             "graphical recolouring of it.</p>\n");

  /* --- gluing and zero-set ------------------------------------------------ */
  fprintf(f, "<h2>Selmer&ndash;Lie gluing, and the zero-set</h2>\n");
  fprintf(f, "<svg viewBox=\"0 0 420 420\" width=\"420\">\n");
  for (u = 1; u <= SL_CHANNELS; ++u)
    for (v = 1; v <= SL_CHANNELS; ++v) {
      ex_bracket br;
      double x = 30.0 + (v - 1) * 25.0, y = 30.0 + (u - 1) * 25.0;
      if (!ex_bracket_of((uint8_t)u, (uint8_t)v, &br)) continue;
      if (br.coeff == 0) {
        if (u < v) ++ncommute;
        fprintf(f, "<rect x=\"%.0f\" y=\"%.0f\" width=\"23\" height=\"23\" "
                   "fill=\"#010409\" stroke=\"#21262d\"/>\n", x, y);
      } else {
        static const char *const CC[5] = { "#f85149", "#d29922", "#3fb950",
                                           "#58a6ff", "#bc8cff" };
        int tc = at->chart[br.unit - 1];
        fprintf(f, "<rect x=\"%.0f\" y=\"%.0f\" width=\"23\" height=\"23\" "
                   "fill=\"%s\" opacity=\"0.85\"><title>[%s,%s] = %+d e_%s "
                   "-&gt; c%d</title></rect>\n",
                x, y, CC[tc % 5], canor_imag_name((canor_imag)u),
                canor_imag_name((canor_imag)v), br.coeff, canor_imag_name(br.unit), tc);
      }
    }
  fprintf(f, "</svg>\n");
  fprintf(f, "<p class=\"sub\">Cell (u,v) is [e<sub>u</sub>,e<sub>v</sub>], "
             "coloured by the chart its target lands in. The bracket is 0 or "
             "&plusmn;2&middot;e<sub>u XOR v</sub> and never anything else, so "
             "this grid is integer throughout and carries no rounding. "
             "<b>%d off-diagonal cells vanish, of %d.</b> That is not a gap in "
             "the measurement: distinct imaginary units anticommute at every "
             "Cayley&ndash;Dickson level, so the bracket zero-set over the "
             "channels is empty by construction and the black diagonal is the "
             "whole of it. The zero-set that carries structure is the "
             "Jacobiator's, below.</p>\n",
          ncommute, SL_CHANNELS * (SL_CHANNELS - 1) / 2);

  /* --- the zero-set that is not empty -------------------------------------- */
  fprintf(f, "<h2>The named zero-set: where Jacobi holds</h2>\n");
  fprintf(f, "<svg viewBox=\"0 0 800 150\" width=\"100%%\">\n");
  for (u = 0; u < SL_CHANNELS; ++u) {
    double x = 30.0 + u * 50.0, h;
    long tot = at->jac_tot[u] ? at->jac_tot[u] : 1;
    h = 100.0 * (double)at->jac_zero[u] / (double)tot;
    fprintf(f, "<rect x=\"%.0f\" y=\"%.0f\" width=\"30\" height=\"%.0f\" "
               "fill=\"#58a6ff\" opacity=\"0.8\"><title>%s: Jacobi holds on "
               "%ld of %ld triples</title></rect>\n",
            x, 115.0 - h, h, canor_imag_name((canor_imag)(u + 1)),
            at->jac_zero[u], at->jac_tot[u]);
    fprintf(f, "<text x=\"%.0f\" y=\"132\" fill=\"#8b949e\" font-size=\"10\" "
               "font-family=\"monospace\" text-anchor=\"middle\">%s</text>\n",
            x + 15.0, canor_imag_name((canor_imag)(u + 1)));
  }
  fprintf(f, "</svg>\n");
  fprintf(f, "<p class=\"sub\">For each channel u, the fraction of triples "
             "(u,a,b) whose Jacobiator vanishes. This is the zero-set worth "
             "naming: the commutator of an associative algebra is always a Lie "
             "algebra, but Cayley&ndash;Dickson stops being associative at "
             "dimension 8, so at 16 the Jacobi identity holds on some triples "
             "and fails on others. A quaternionic channel sits inside a closing "
             "subalgebra and holds broadly; a sedenionic one does not. The "
             "bounded smoothing below extracts over this split, which is why it "
             "had to be a real measurement rather than an assumption.</p>\n");

  /* --- the cycle ---------------------------------------------------------- */
  fprintf(f, "<h2>PRE &rarr; PERI &rarr; POST, per channel</h2>\n<table>\n");
  fprintf(f, "<tr><th>d</th><th>chart</th><th>node</th><th>CD</th>"
             "<th>x cap</th><th>s stress</th><th>f flow</th>"
             "<th>F=K<sub>f</sub>f</th><th>&sigma;&#772;</th>"
             "<th>r<sub>&Sigma;</sub>=L<sub>m</sub></th><th>interval</th>"
             "<th>Jacobi 0</th><th>assoc 0</th></tr>\n");
  for (u = 0; u < SL_CHANNELS; ++u) {
    if (!at->applicable[u]) {
      fprintf(f, "<tr class=\"na\"><td>%s</td><td>c%d</td><td>%d</td><td>%d</td>"
                 "<td colspan=\"9\">not applicable &mdash; the family cannot "
                 "express a Left/Right distinction</td></tr>\n",
              canor_imag_name((canor_imag)(u + 1)), at->chart[u], at->node[u], at->cd[u]);
      continue;
    }
    fprintf(f, "<tr><td>%s</td><td>c%d</td><td>%d</td><td>%d</td>"
               "<td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%+d</td>"
               "<td>%s</td><td>%s</td><td>%ld/%ld</td><td>%ld/%ld</td></tr>\n",
            canor_imag_name((canor_imag)(u + 1)), at->chart[u], at->node[u], at->cd[u],
            at->cap[u], at->stress[u], at->flow[u], at->F_term[u], at->sigma_bar[u],
            canor_resid_class_name(at->rcls[u]), canor_demo_sl_cls_name(at->cls[u]),
            at->jac_zero[u], at->jac_tot[u], at->asc_zero[u], at->jac_tot[u]);
  }
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">All quantities per-mille. PRE observes the state "
             "and estimates Stress; PERI computes the components; POST takes the "
             "residual. Stress here is one typed channel &mdash; <b>verification "
             "stress</b>, the refusal fraction &mdash; and is never blended with "
             "others or collapsed to a scalar, because &sect;4 forbids a "
             "universal scalar exactly where channel separation matters, and "
             "channel separation is what this tool is for. The interval class is "
             "<code>canor_origo_classify</code> of the folded ORIGO, so it is "
             "derived from the measurement rather than assigned to it.</p>\n");

  /* --- what is deliberately not computed ---------------------------------- */
  fprintf(f, "<h2>Not computed, and why</h2>\n");
  fprintf(f, "<p class=\"no\"><b>P and I.</b> Both need the control error "
             "e = s &minus; s<sub>ref</sub>. No Strategy here declares a Stress "
             "target, and &sect;4.2 forbids assuming zero for an adaptive system "
             "&mdash; some require nonzero excitation, reserve usage or "
             "productive tension. An undeclared target is not a zero "
             "target.</p>\n");
  fprintf(f, "<p class=\"no\"><b>u = Saturate<sub>C</sub>(u<sub>raw</sub>).</b> "
             "C is the authority-, rights-, resource- and safety-constrained "
             "action set, and this tool has none declared. &sect;5.4 adds that "
             "the Strategy term SHALL NOT create its own authority, so the "
             "governor's inputs are reported and its output is refused.</p>\n");
  fprintf(f, "<p class=\"no\"><b>Temporal derivation.</b> &lambda;<sub>t</sub> = 0: "
             "there is no time series, only a single cycle. D therefore carries "
             "&lambda;<sub>&Sigma;</sub> = 1 and the semantic residual alone "
             "&mdash; which is where &sect;5.5 puts L<sub>m</sub>, as a declared "
             "model-residual term.</p>\n");
  fprintf(f, "<p class=\"no\"><b>Operational reading of the interval class.</b> "
             "WORLD_MODEL &sect;8.4 reads timelike as causally executable within "
             "a declared latency boundary. This tool declares none, so the class "
             "is computed and reported and its operational meaning is left "
             "open.</p>\n");

  /* --- lossy-transformation record (WORLD_MODEL 4.1) ---------------------- */
  fprintf(f, "<h2>Declared lossy transformation</h2>\n<table>\n");
  fprintf(f, "<tr><th>property</th><th>value</th></tr>\n");
  fprintf(f, "<tr><td>input points</td><td>%ld</td></tr>\n", n);
  fprintf(f, "<tr><td>aggregation</td><td>count of separating items per channel</td></tr>\n");
  fprintf(f, "<tr><td>point-level provenance</td><td>not carried into the aggregate</td></tr>\n");
  fprintf(f, "<tr><td>refusals dropped</td><td>%ld</td></tr>\n", at->dropped);
  fprintf(f, "<tr><td>reversible</td><td>no</td></tr>\n");
  fprintf(f, "<tr><td>re-derivable</td><td>yes, from the same cloud and Strategy</td></tr>\n");
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">&sect;4.1 forbids erasing point-level provenance "
             "without a declared lossy-transformation record. Counting per "
             "channel does erase it, so the record is stated here rather than "
             "left implicit.</p>\n");

  /* --- bounded smoothing --------------------------------------------------- */
  fprintf(f, "<h2>Bounded smoothing</h2>\n<table>\n");
  fprintf(f, "<tr><th>raw profile</th><th>snapped</th><th>moved</th><th>bound</th></tr>\n");
  fprintf(f, "<tr><td>%05x</td><td>%05x</td><td>%d</td><td>&le; 3</td></tr>\n",
          at->profile & 0x7fffffu, snapped & 0x7fffffu, moved);
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">The admissible-channel profile snapped to its "
             "nearest Golay codeword. The [23,12,7] code is perfect, so every "
             "word lies within distance 3 of exactly one codeword: the snap moves "
             "at most three channels <i>always</i> &mdash; a bound, not a "
             "tendency &mdash; and two Strategies snapping to the same codeword "
             "are within distance 6. That is what makes this a manifold with a "
             "stated resolution rather than a scatter of points: the smoothing "
             "has a radius, and the radius is proved rather than tuned.</p>\n");

  fprintf(f, "<h2>What this claims</h2>\n");
  fprintf(f, "<p class=\"sub\">Capacity is a property of the <b>instrument</b>: "
             "how much Left/Right signal a differential family could carry, not "
             "how much bias was found. %d of %d channels are <i>not applicable</i> "
             "&mdash; named, per &sect;9.2, which requires a missing facet to read "
             "unknown or not applicable and never silently zero. Averaging them in "
             "would make an unmeasured direction look like a moderate one, and "
             "&sect;9.4 separately forbids an aggregate that erases per-node "
             "disagreement. Bias stays relational: this compares Strategies "
             "through a shared profile and fixes no absolute zero, because a "
             "single Strategy has none to fix. The algebra is exact and checked; "
             "reading a bracket as inter-observer distortion is a framework claim "
             "the arithmetic does not establish.</p>\n", nblind, SL_CHANNELS);
  fprintf(f, "</div>\n");
  fclose(f);

  printf("wrote %s\n", out);
  printf("  %d TST trees, %d differential nodes, folded and classified\n",
         spread.count, SL_CHANNELS);
  printf("  bracket zero-set  %d of %d off-diagonal pairs vanish\n",
         ncommute, SL_CHANNELS * (SL_CHANNELS - 1) / 2);
  printf("                    (empty by construction: distinct imaginary units\n");
  printf("                     anticommute at every Cayley-Dickson level)\n");
  {
    long jz = 0, jt = 0, az = 0;
    for (u = 0; u < SL_CHANNELS; ++u) {
      jz += at->jac_zero[u]; jt += at->jac_tot[u]; az += at->asc_zero[u];
    }
    printf("  Jacobi zero-set   %ld of %ld channel-triples\n", jz, jt);
    printf("  assoc zero-set    %ld of %ld  (associativity fails first)\n", az, jt);
  }
  printf("  not applicable    %d of %d channels\n", nblind, SL_CHANNELS);
  printf("  profile           %05x -> %05x, moved %d (bound 3)\n",
         at->profile & 0x7fffffu, snapped & 0x7fffffu, moved);
  printf("  Stress/Flow       per-channel vectors; no scalar formed (EM 4)\n");
  printf("  P, I, u           not computed: no s_ref, no action set C\n");
  printf("  lossy record      %ld refusals dropped, declared (WM 4.1)\n", at->dropped);
  canor_demo_prov_write_sidecar(out);
  free(at);
  return 0;
}
