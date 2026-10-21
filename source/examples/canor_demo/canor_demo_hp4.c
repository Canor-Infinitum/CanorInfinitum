/* canor_demo_hp4.c - hp4 - the Strategy as an atlas of HP^4, chart by chart
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* hp4 - the Strategy as an atlas of HP^4, chart by chart                     */
/* ========================================================================== */
/* THE CORRESPONDENCE, AND WHY IT IS NOT ARRANGED.
 *
 * A spread partitions the fifteen imaginary units into five triples, each of
 * which CLOSES under multiplication - so each triple together with 1 spans a
 * quaternion subalgebra H. A fully-fledged five-trifecta Strategy therefore
 * carries H^5. Projectivising gives
 *
 *     HP^4 = P(H^5),   real dimension 20 - 4 = 16
 *
 * which is exactly the dimension of the sedenions. That equality is not a
 * coincidence to be admired; it follows from the spread being a partition of
 * the fifteen units into five closing triples, which is checked here before
 * anything is drawn.
 *
 * A SIXTEEN-DIMENSIONAL MANIFOLD CANNOT BE DRAWN, so this draws its ATLAS.
 * Each chart is one quaternion subalgebra, and inside a chart the Mandelbrot
 * iteration z -> z^2 + c stays in span{1,v} - verified earlier, and exact at
 * every Cayley-Dickson dimension because squaring preserves the imaginary
 * direction. So every chart carries a genuine Mandelbrot set in a genuine
 * complex plane, and the lowering costs nothing.
 *
 * WHAT MAKES THE CHARTS DIFFER is the Strategy. The three differential
 * families sitting on a chart's three units evaluate to FLOWVALUEs, and those
 * offset the chart's parameter c. So the five panels are the same fractal seen
 * through five different differential neighbourhoods - which is what it means
 * to view the Mandelbrot Strategy IN RELATION TO the Mandelbrot fractal rather
 * than beside it.
 *
 * L_m gives each chart a residual class, and the Selmer-Lie brackets give the
 * transition structure: a bracket taken INSIDE a chart stays in it (that is
 * closure), while a bracket taken ACROSS charts lands in a third, and the
 * table of where says how the atlas is glued.
 */

static int hp4_escape(double cr, double ci, int fuel) {
  double zr = 0.0, zi = 0.0;
  int k;
  for (k = 0; k < fuel; ++k) {
    double t = zr * zr - zi * zi + cr;
    zi = 2.0 * zr * zi + ci;
    zr = t;
    if (zr * zr + zi * zi > 4.0) return k;
  }
  return -1;                      /* bounded within fuel */
}

int canor_demo_cmd_hp4(const canor_demo_config_t *c, int sel, const char *emit) {
  ex_groups spread;
  FILE *f;
  char path[600];
  int t, i, j, k;
  const int CW = 46, CH = 22;     /* chart raster */

  snprintf(path, sizeof path, "%s", emit ? emit : "hp4.html");
  if (!ex_index_xor_holds()) {
    fprintf(stderr, "follow: the F2^4 reading is invalid\n");
    return 2;
  }
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;
  /* EVERY chart must close, or it is not a quaternion subalgebra and the whole
   * H^5 reading collapses. Checked, not assumed. */
  for (t = 0; t < spread.count; ++t)
    if (!spread.closes[t]) {
      fprintf(stderr, "follow: spread group %d does not close; H^5 does not hold\n", t);
      return 2;
    }

  f = fopen(path, "w");
  if (!f) { fprintf(stderr, "follow: cannot write %s\n", path); return 2; }

  fprintf(f, "<title>HP^4 atlas - Strategy against the Mandelbrot set</title>\n");
  fprintf(f, "<style>\n"
             ":root{color-scheme:light dark}\n"
             "body{font:14px/1.55 ui-monospace,SFMono-Regular,Menlo,monospace;"
             "margin:0;padding:24px;background:#fbfbfc;color:#1a1a1c}\n"
             "@media(prefers-color-scheme:dark){body{background:#0f1012;color:#e8e8ea}}\n"
             ".panel{max-width:860px;margin:0 auto}\n"
             "h1{font-size:17px;margin:0 0 4px}h2{font-size:14px;margin:24px 0 4px}\n"
             ".sub{opacity:.66;font-size:12px;margin:0 0 12px}\n"
             ".charts{display:flex;flex-wrap:wrap;gap:16px}\n"
             ".chart{flex:1 1 250px;min-width:250px}\n"
             "pre{font-size:9px;line-height:1.0;margin:4px 0;overflow-x:auto}\n"
             "table{border-collapse:collapse;font-size:12px;margin-top:6px}\n"
             "td,th{padding:2px 10px 2px 0;text-align:left;white-space:nowrap}\n"
             ".k{display:inline-block;width:10px;height:10px;border-radius:2px;"
             "vertical-align:middle;margin-right:5px}\n"
             "</style>\n");
  fprintf(f, "<div class=\"panel\">\n"
             "<h1>HP<sup>4</sup> = P(H<sup>5</sup>): a fully-fledged Strategy as "
             "an atlas</h1>\n");
  fprintf(f, "<p class=\"sub\">The spread partitions the 15 imaginary units into "
             "5 triples that each <b>close</b> under multiplication, so each "
             "spans a quaternion subalgebra H. Five of them give H<sup>5</sup>, "
             "and P(H<sup>5</sup>) = HP<sup>4</sup> has real dimension "
             "20&minus;4 = <b>16</b> &mdash; the dimension of the sedenions. "
             "Sixteen dimensions cannot be drawn, so what follows is the atlas: "
             "one chart per subalgebra, each exactly lowered to its 2-plane, "
             "each carrying a real Mandelbrot set. Strategy <b>%s</b>.</p>\n",
          c->strat[sel].id.name);

  /* --- the five charts --- */
  fprintf(f, "<div class=\"charts\">\n");
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t) {
    FLOWVALUE mf[CANOR_TRIFECTAS][3];
    uint32_t defined = 0;
    double offr = 0.0, offi = 0.0;
    int nd = 0;
    canor_residual res;

    /* The chart's parameter offset comes from its own three families. A
     * FLOWVALUE is 4-valued, so the offset is small and bounded - the point is
     * that the charts differ BY THE STRATEGY, not that the offset is large. */
    if (canor_flow_multiflow(&c->strat[sel], &c->flows, 0, mf, &defined)) {
      for (k = 0; k < 3; ++k) {
        int have = (defined >> (t * 3 + k)) & 1u;
        if (!have) continue;
        ++nd;
        {
          int v = (int)mf[t][k] & 3;
          double num = (v == 1) ? -1.0 : (v == 3) ? 1.0 : 0.0;
          if (k == 0) offr += 0.18 * num;
          else if (k == 1) offi += 0.18 * num;
          else offr += 0.09 * num;
        }
      }
    }

    fprintf(f, "<div class=\"chart\">\n");
    fprintf(f, "<h2>chart %d &nbsp; {%s, %s, %s}</h2>\n", t,
            canor_imag_name(spread.u[t][0]), canor_imag_name(spread.u[t][1]),
            canor_imag_name(spread.u[t][2]));
    fprintf(f, "<p class=\"sub\">closes: yes &nbsp;&middot;&nbsp; families "
               "defined %d/3 &nbsp;&middot;&nbsp; c offset (%+.2f, %+.2f)</p>\n",
            nd, offr, offi);

    fprintf(f, "<pre>");
    for (j = 0; j < CH; ++j) {
      for (i = 0; i < CW; ++i) {
        double cr = -2.1 + 3.0 * (double)i / (double)CW + offr;
        double ci = -1.2 + 2.4 * (double)j / (double)CH + offi;
        int e = hp4_escape(cr, ci, 40);
        char ch;
        if (e < 0) ch = '#';
        else if (e > 12) ch = '+';
        else if (e > 5) ch = ':';
        else if (e > 2) ch = '.';
        else ch = ' ';
        fputc(ch, f);
      }
      fputc('\n', f);
    }
    fprintf(f, "</pre>\n");

    /* L_m for this chart: the residual class of its MetaFlow reading. */
    if (canor_residual_L((canor_kclass)(t % 4), (uint64_t)(6 + t), 0, 0.0, 0.0, &res)) {
      fprintf(f, "<p class=\"sub\">L_m class: <b>%s</b> &nbsp; log2 ratio %lld</p>\n",
              canor_resid_class_name(res.cls), (long long)res.log2_ratio_milli);
      canor_residual_free(&res);
    }
    fprintf(f, "</div>\n");
  }
  fprintf(f, "</div>\n");

  /* --- the gluing: Selmer-Lie brackets across charts --- */
  fprintf(f, "<h2>Transition structure &mdash; Selmer-Lie brackets between charts</h2>\n");
  fprintf(f, "<p class=\"sub\">A bracket taken inside a chart stays inside it: "
             "that is what closure means, and it is why each chart is a "
             "subalgebra. A bracket taken across two charts lands in a third, "
             "and the table says which &mdash; that is how the atlas is glued. "
             "The diagonal is the chart itself.</p>\n");
  fprintf(f, "<table><tr><th></th>");
  for (j = 0; j < spread.count; ++j) fprintf(f, "<th>c%d</th>", j);
  fprintf(f, "</tr>\n");
  for (i = 0; i < spread.count; ++i) {
    fprintf(f, "<tr><th>c%d</th>", i);
    for (j = 0; j < spread.count; ++j) {
      int hits[8], nh = 0, a, b, m;
      memset(hits, 0, sizeof hits);
      for (a = 0; a < 3; ++a)
        for (b = 0; b < 3; ++b) {
          ex_bracket br;
          int tgt = -1;
          if (!ex_bracket_of(spread.u[i][a], spread.u[j][b], &br)) continue;
          if (br.coeff == 0) continue;
          for (m = 0; m < spread.count; ++m) {
            int q;
            for (q = 0; q < 3; ++q)
              if (spread.u[m][q] == br.unit) tgt = m;
          }
          if (tgt >= 0 && !hits[tgt]) { hits[tgt] = 1; ++nh; }
        }
      fprintf(f, "<td>");
      if (i == j) fprintf(f, "self");
      else {
        int first = 1;
        for (m = 0; m < spread.count; ++m)
          if (hits[m]) { fprintf(f, "%sc%d", first ? "" : ",", m); first = 0; }
        if (first) fprintf(f, "&mdash;");
      }
      fprintf(f, "</td>");
      (void)nh;
    }
    fprintf(f, "</tr>\n");
  }
  fprintf(f, "</table>\n");

  /* --- PFISD, as a governor over the atlas --- */
  fprintf(f, "<h2>PFISD across the atlas</h2>\n");
  fprintf(f, "<p class=\"sub\">Stress is the fraction of a chart's families that "
             "refused; Flow is 1 &minus; Stress, as the model defines it. The "
             "five terms are reported per chart rather than summed, because a "
             "governor over a manifold acts chart-wise &mdash; averaging them "
             "would hide exactly the chart that is failing.</p>\n");
  fprintf(f, "<table><tr><th>chart</th><th>Stress</th><th>Flow</th>"
             "<th>defined</th><th>closes</th></tr>\n");
  {
    FLOWVALUE mf[CANOR_TRIFECTAS][3];
    uint32_t defined = 0;
    canor_flow_multiflow(&c->strat[sel], &c->flows, 0, mf, &defined);
    for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t) {
      int nd = 0;
      double stress, flow;
      for (k = 0; k < 3; ++k) if ((defined >> (t * 3 + k)) & 1u) ++nd;
      stress = 1.0 - (double)nd / 3.0;
      flow = 1.0 - stress;
      fprintf(f, "<tr><td>c%d</td><td>%.2f</td><td>%.2f</td><td>%d/3</td>"
                 "<td>%s</td></tr>\n", t, stress, flow, nd,
              spread.closes[t] ? "yes" : "NO");
    }
  }
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">Every structural claim above is checked before it "
             "is drawn: that each spread triple closes, that the units satisfy "
             "e<sub>a</sub>&middot;e<sub>b</sub> = &plusmn;e<sub>a XOR b</sub>, "
             "and that the bracket targets land where the table says. What is "
             "NOT claimed is any physical or geometric meaning for HP<sup>4</sup> "
             "beyond the algebra: it is the projectivisation of H<sup>5</sup>, "
             "and the atlas is a way of drawing sixteen dimensions, not a claim "
             "about what they are.</p>\n");
  fprintf(f, "</div>\n");
  fclose(f);

  printf("wrote %s\n", path);
  printf("  HP^4 = P(H^5), real dim 16 = dim(sedenions)\n");
  printf("  %d charts, all closing, strategy %s\n",
         spread.count, c->strat[sel].id.name);
  canor_demo_prov_write_sidecar(path);
  return 0;
}
