/* canor_demo_mfview.c - mfview - a fully-fledged Strategy on PG(3,2), lowered to the FLOWVALUE plane
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* mfview - a fully-fledged Strategy on PG(3,2), lowered to the FLOWVALUE plane
 * ========================================================================== */
/* THE CORRESPONDENCE THIS DRAWS. A fully-fledged Strategy carries five
 * trifectas of three differential families - fifteen in all. PG(3,2) has
 * fifteen points. They are the same fifteen: family (t, c) belongs to imaginary
 * unit 3t + c + 1, and the units ARE the non-zero vectors of F2^4 because
 * e_a . e_b = +-e_(a XOR b). So a MultiFlow evaluated over a Flow is precisely
 * a LABELLING OF PG(3,2) BY FLOWVALUES, and that is what the top panel is.
 *
 * That is not an analogy arranged for the picture. The counts coincide because
 * both come from the same place: fifteen imaginary units.
 *
 * THE LOWERING, AND WHAT IT REALLY IS. "Lowering into 2D" has an exact meaning
 * here and it is not a projection chosen for display. For q = a + v,
 *
 *     q^2 = a^2 - |v|^2 + 2a.v
 *
 * so squaring preserves the imaginary DIRECTION - the cross terms cancel
 * because imaginary units anticommute. Every orbit therefore stays inside
 * span{1, v}, a copy of C, at every Cayley-Dickson dimension including the
 * sedenions. The 16-dimensional algebra collapses to a plane for this class of
 * dynamics EXACTLY, losing nothing, which is why no projective machinery is
 * needed to do the lowering: there is nothing above the plane to project.
 *
 * The plane the values land in is the FLOWVALUE square: sign against magnitude,
 * four points, with the two zeros distinguished by pole. The lower panel is
 * that square with the fifteen units placed in it - the sixteen dimensions
 * lowered to four cells, and the census of which cells fill.
 *
 * ON THE NAME. HP^n is quaternionic projective space and has real dimension
 * 4n, so HP^16 would be 64-dimensional; the sedenions are 16-dimensional and
 * are neither associative nor alternative, so no quaternionic projective space
 * sits over them. The structure that IS here is PG(3,2) on the fifteen units,
 * verified, and the exact 2-plane reduction above. Both are drawn; the name is
 * not.
 */

static const char *const FV_NAME[4] = { "NegZero", "NegOne", "PosZero", "PosOne" };
static const char *const FV_COL[4] = {
  "#4a7fb5",   /* NegZero - faint negative */
  "#1b4f86",   /* NegOne  - strong negative */
  "#d08a92",   /* PosZero - faint positive */
  "#b02038"    /* PosOne  - strong positive */
};

int canor_demo_cmd_mfview(const canor_demo_config_t *c, int sel, const char *emit) {
  ex_groups spread;
  FILE *f;
  char path[600];
  double px[16], py[16];
  int i, fl, wcount[5] = { 0, 0, 0, 0, 0 }, wseen[5] = { 0, 0, 0, 0, 0 };
  const int W = 720, H = 52 + 96 * 3 + 40;
  long cell[4];

  snprintf(path, sizeof path, "%s", emit ? emit : "multiflow.html");
  if (!ex_index_xor_holds()) {
    fprintf(stderr, "follow: the F2^4 reading is invalid; nothing drawn would mean anything\n");
    return 2;
  }
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;
  f = fopen(path, "w");
  if (!f) { fprintf(stderr, "follow: cannot write %s\n", path); return 2; }

  for (i = 1; i <= 15; ++i) ++wcount[canor_demo_pg_weight(i)];
  for (i = 1; i <= 15; ++i) {
    int w = canor_demo_pg_weight(i);
    double slot = (double)(wseen[w]++) + 0.5;
    px[i] = 60.0 + (double)(W - 170) * slot / (double)wcount[w];
    py[i] = 52.0 + 96.0 * (double)(w - 1);
  }

  fprintf(f, "<title>MultiFlow on PG(3,2)</title>\n");
  fprintf(f, "<style>\n"
             ":root{color-scheme:light dark}\n"
             "body{font:14px/1.55 ui-monospace,SFMono-Regular,Menlo,monospace;"
             "margin:0;padding:24px;background:#fbfbfc;color:#1a1a1c}\n"
             "@media(prefers-color-scheme:dark){body{background:#0f1012;color:#e8e8ea}}\n"
             ".panel{max-width:780px;margin:0 auto}\n"
             "h1{font-size:17px;margin:0 0 4px}h2{font-size:14px;margin:26px 0 6px}\n"
             ".sub{opacity:.66;font-size:12px;margin:0 0 14px}\n"
             "svg{width:100%%;height:auto;display:block}\n"
             ".faint{stroke:currentColor;stroke-opacity:.12;stroke-width:1;fill:none}\n"
             ".lab{font-size:10px;fill:currentColor;fill-opacity:.8}\n"
             ".rowlab{font-size:10px;fill:currentColor;fill-opacity:.45}\n"
             ".hole{fill:none;stroke:currentColor;stroke-opacity:.5;"
             "stroke-width:1.4;stroke-dasharray:2 2}\n"
             ".key{display:inline-block;width:10px;height:10px;border-radius:2px;"
             "vertical-align:middle;margin-right:5px}\n"
             "table{border-collapse:collapse;font-size:12px;margin-top:6px}\n"
             "td,th{padding:2px 12px 2px 0;text-align:left;white-space:nowrap}\n"
             "</style>\n");
  fprintf(f, "<div class=\"panel\">\n"
             "<h1>MultiFlow of a fully-fledged Strategy, on PG(3,2)</h1>\n");
  fprintf(f, "<p class=\"sub\">Strategy <b>%s</b>. Five trifectas of three "
             "differential families is fifteen families; PG(3,2) has fifteen "
             "points; they are the same fifteen, because both are the imaginary "
             "units. Each point is coloured by the FLOWVALUE that family takes "
             "on the Flow; a dashed ring is a family that did not evaluate, "
             "which a MultiFlow may legitimately have.</p>\n",
          c->strat[sel].id.name);

  memset(cell, 0, sizeof cell);

  for (fl = 0; fl < c->flows.count; ++fl) {
    FLOWVALUE mf[CANOR_TRIFECTAS][3];
    uint32_t defined = 0;
    int t, cc;
    if (!canor_flow_multiflow(&c->strat[sel], &c->flows, fl, mf, &defined)) continue;

    fprintf(f, "<h2>Flow &ldquo;%s&rdquo; &nbsp;<span class=\"sub\">%s</span></h2>\n",
            c->flows.flow[fl].name, c->flows.flow[fl].word);
    fprintf(f, "<svg viewBox=\"0 0 %d %d\" role=\"img\">\n", W, H);
    for (i = 0; i < 4; ++i)
      fprintf(f, "<text class=\"rowlab\" x=\"6\" y=\"%.1f\">wt %d</text>\n",
              52.0 + 96.0 * i + 4.0, i + 1);
    /* The spread, faint, so the projective structure stays visible under the
     * colouring rather than being replaced by it. */
    for (i = 0; i < spread.count; ++i) {
      int a = spread.u[i][0], b = spread.u[i][1], d = spread.u[i][2];
      fprintf(f, "<path class=\"faint\" d=\"M%.1f %.1f L%.1f %.1f L%.1f %.1f Z\"/>\n",
              px[a], py[a], px[b], py[b], px[d], py[d]);
    }
    for (t = 0; t < CANOR_TRIFECTAS; ++t)
      for (cc = 0; cc < 3; ++cc) {
        int unit = 3 * t + cc + 1;
        int have = (defined >> (t * 3 + cc)) & 1u;
        if (unit > 15) continue;
        if (have) {
          int v = (int)mf[t][cc] & 3;
          ++cell[v];
          fprintf(f, "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"6\" fill=\"%s\"/>\n",
                  px[unit], py[unit], FV_COL[v]);
        } else {
          fprintf(f, "<circle class=\"hole\" cx=\"%.1f\" cy=\"%.1f\" r=\"6\"/>\n",
                  px[unit], py[unit]);
        }
        fprintf(f, "<text class=\"lab\" x=\"%.1f\" y=\"%.1f\">%s</text>\n",
                px[unit] + 9.0, py[unit] + 3.5, canor_imag_name((canor_imag)unit));
      }
    fprintf(f, "</svg>\n");
    {
      int nd = 0, k2;
      for (k2 = 0; k2 < 15; ++k2) if ((defined >> k2) & 1u) ++nd;
      fprintf(f, "<p class=\"sub\">%d of 15 families evaluated; %d refused "
                 "(division by a zero-valued operand).</p>\n", nd, 15 - nd);
    }
  }

  fprintf(f, "<p class=\"sub\">");
  for (i = 0; i < 4; ++i)
    fprintf(f, "<span class=\"key\" style=\"background:%s\"></span>%s &nbsp;&nbsp;",
            FV_COL[i], FV_NAME[i]);
  fprintf(f, "<span class=\"key\" style=\"border:1.4px dashed currentColor;"
             "background:none\"></span>undefined</p>\n");

  /* --- the lowering --- */
  fprintf(f, "<h2>The lowering: 16 dimensions to the FLOWVALUE plane</h2>\n");
  fprintf(f, "<p class=\"sub\">q&sup2; = a&sup2; &minus; |v|&sup2; + 2a&middot;v "
             "preserves the imaginary DIRECTION, because imaginary units "
             "anticommute and the cross terms cancel. So every orbit stays in "
             "span{1,v}, a copy of C, at every Cayley-Dickson dimension "
             "including the sedenions &mdash; the collapse to a plane is exact "
             "and loses nothing, which is why no projective machinery is needed "
             "to perform it. The plane values land in is the FLOWVALUE square: "
             "sign against magnitude, with the two zeros kept apart by pole.</p>\n");
  {
    const int PW = 420, PH = 260;
    double cx[4], cy[4];
    long total = cell[0] + cell[1] + cell[2] + cell[3];
    cx[1] = 110; cy[1] = 70;    /* NegOne  */
    cx[0] = 110; cy[0] = 190;   /* NegZero */
    cx[2] = 310; cy[2] = 190;   /* PosZero */
    cx[3] = 310; cy[3] = 70;    /* PosOne  */
    fprintf(f, "<svg viewBox=\"0 0 %d %d\" role=\"img\">\n", PW, PH);
    fprintf(f, "<line class=\"faint\" x1=\"210\" y1=\"24\" x2=\"210\" y2=\"236\" "
               "stroke-opacity=\".3\"/>\n");
    fprintf(f, "<line class=\"faint\" x1=\"40\" y1=\"130\" x2=\"380\" y2=\"130\" "
               "stroke-opacity=\".3\"/>\n");
    fprintf(f, "<text class=\"rowlab\" x=\"186\" y=\"18\">magnitude 1</text>\n");
    fprintf(f, "<text class=\"rowlab\" x=\"186\" y=\"250\">magnitude 0</text>\n");
    fprintf(f, "<text class=\"rowlab\" x=\"44\" y=\"134\">negative</text>\n");
    fprintf(f, "<text class=\"rowlab\" x=\"330\" y=\"134\">positive</text>\n");
    for (i = 0; i < 4; ++i) {
      double r = total ? 14.0 + 40.0 * (double)cell[i] / (double)total : 14.0;
      fprintf(f, "<circle cx=\"%.0f\" cy=\"%.0f\" r=\"%.1f\" fill=\"%s\" "
                 "fill-opacity=\".75\"/>\n", cx[i], cy[i], r, FV_COL[i]);
      fprintf(f, "<text class=\"lab\" x=\"%.0f\" y=\"%.0f\" text-anchor=\"middle\">"
                 "%s</text>\n", cx[i], cy[i] - r - 6.0, FV_NAME[i]);
      fprintf(f, "<text class=\"lab\" x=\"%.0f\" y=\"%.0f\" text-anchor=\"middle\">"
                 "%ld</text>\n", cx[i], cy[i] + 4.0, cell[i]);
    }
    fprintf(f, "</svg>\n");
    fprintf(f, "<p class=\"sub\">%ld family evaluations across all Flows. The two "
               "zero cells are the reason FLOWVALUE is four-valued rather than "
               "three: a cancelling sum still has a pole, and under the Ore "
               "operators that pole is derived from which way the sequence "
               "passes through zero rather than declared.</p>\n", total);
  }

  fprintf(f, "</div>\n");
  fclose(f);
  printf("wrote %s\n", path);
  printf("  strategy %s, %d Flows, 15 families each\n",
         c->strat[sel].id.name, c->flows.count);
  printf("  FLOWVALUE census:");
  for (i = 0; i < 4; ++i) printf("  %s %ld", FV_NAME[i], cell[i]);
  printf("\n");
  canor_demo_prov_write_sidecar(path);
  return 0;
}
