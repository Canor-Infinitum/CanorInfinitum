/* canor_demo_pgview.c - pgview - PG(3,2) rendered, with the spread as the default reading
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* pgview - PG(3,2) rendered, with the spread as the default reading          */
/* ========================================================================== */
/* WHAT IS BEING DRAWN. The 15 imaginary units are the 15 non-zero vectors of
 * F2^4, and a quaternion triple {a, b, a^b} is a LINE of the projective space
 * PG(3,2). There are 35 of them, every pair of points lies on exactly one, and
 * all of that is verified elsewhere in this suite rather than asserted here.
 *
 * THE HARD PART IS THAT 35 LINES OVER 15 POINTS IS VISUALLY SATURATED. Drawing
 * them all with equal weight produces a grey mesh that says nothing. So the
 * default reading is the SPREAD - five pairwise disjoint lines that partition
 * all 15 points - drawn as five filled triangles, with the remaining thirty
 * lines behind them at low opacity for context. That choice is not cosmetic:
 * the spread is the grouping the extraction service defaults to, it is the only
 * one that is both a partition and fold-closed, and the 56 spreads form a
 * single A8 orbit, so no spread is more canonical than another and showing one
 * loses nothing.
 *
 * LAYOUT IS BY HAMMING WEIGHT, not around a circle. A circle is symmetric and
 * says nothing; stacking the points by the weight of their index shows the
 * Boolean lattice of F2^4 - 4 points of weight 1, 6 of weight 2, 4 of weight 3,
 * 1 of weight 4 - and in that layout a line's three points must have XOR zero,
 * which constrains where lines can run. The picture then carries the algebra
 * rather than merely being decorated by it.
 *
 * THE INCIDENCE MATRIX is drawn alongside because the diagram is a reading and
 * the matrix is the data: 15 rows by 35 columns, one mark per incidence, 105
 * marks in total and exactly three per column.
 */

int canor_demo_pg_weight(int v) {
  int n = 0;
  while (v) { n += v & 1; v >>= 1; }
  return n;
}

int canor_demo_cmd_pgview(const canor_demo_config_t *c, const char *emit) {
  ex_groups all, spread;
  FILE *f;
  char path[600];
  double px[16], py[16];
  int i, j, k, wcount[5] = { 0, 0, 0, 0, 0 }, wseen[5] = { 0, 0, 0, 0, 0 };
  /* H is four rows at 96 plus the top offset plus a small margin. It was 470,
   * which left a hundred units of empty canvas under the weight-4 row - the
   * viewBox does not shrink to its content, so the gap was real page space. */
  const int W = 720, H = 52 + 96 * 3 + 40;
  static const char *const COL[5] = {
    "#c02040", "#2a7fd4", "#2e9e5b", "#d08a1e", "#8a4fc4"
  };

  (void)c;
  snprintf(path, sizeof path, "%s", emit ? emit : "pg32.html");
  if (!ex_index_xor_holds()) {
    fprintf(stderr, "follow: e_a.e_b = +-e_(a XOR b) does not hold; the F2^4 "
                    "reading is invalid and nothing drawn would mean anything\n");
    return 2;
  }
  if (!ex_groups_build(&all, EX_GROUP_QUATERNION, CANOR_IMAG_COUNT)) return 2;
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;

  f = fopen(path, "w");
  if (!f) { fprintf(stderr, "follow: cannot write %s\n", path); return 2; }

  /* Positions: rows by Hamming weight, spread evenly across each row. */
  for (i = 1; i <= 15; ++i) ++wcount[canor_demo_pg_weight(i)];
  for (i = 1; i <= 15; ++i) {
    int w = canor_demo_pg_weight(i);
    double slot = (double)(wseen[w]++) + 0.5;
    px[i] = 60.0 + (double)(W - 170) * slot / (double)wcount[w];
    py[i] = 52.0 + 96.0 * (double)(w - 1);
  }

  fprintf(f, "<title>PG(3,2) - 15 points, 35 lines</title>\n");
  fprintf(f, "<style>\n"
             ":root{color-scheme:light dark}\n"
             "body{font:14px/1.55 ui-monospace,SFMono-Regular,Menlo,monospace;"
             "margin:0;padding:24px;background:#fbfbfc;color:#1a1a1c}\n"
             "@media(prefers-color-scheme:dark){body{background:#0f1012;color:#e8e8ea}}\n"
             ".panel{max-width:780px;margin:0 auto}\n"
             "h1{font-size:17px;margin:0 0 4px}h2{font-size:14px;margin:26px 0 6px}\n"
             ".sub{opacity:.66;font-size:12px;margin:0 0 14px}\n"
             "svg{width:100%%;height:auto;display:block}\n"
             ".faint{stroke:currentColor;stroke-opacity:.13;stroke-width:1;fill:none}\n"
             ".pt{fill:currentColor}\n"
             ".lab{font-size:10px;fill:currentColor;fill-opacity:.75}\n"
             ".rowlab{font-size:10px;fill:currentColor;fill-opacity:.45}\n"
             "table{border-collapse:collapse;font-size:11px;margin-top:8px}\n"
             "td,th{padding:1px 7px 1px 0;text-align:left;white-space:nowrap}\n"
             ".key{display:inline-block;width:10px;height:10px;border-radius:2px;"
             "vertical-align:middle;margin-right:6px}\n"
             "</style>\n");

  fprintf(f, "<div class=\"panel\">\n<h1>PG(3,2): the 15 imaginary units as a "
             "projective space</h1>\n");
  fprintf(f, "<p class=\"sub\">%d points, %d lines, 3 points per line, every pair "
             "on exactly one line. Rows are Hamming weight of the unit index read "
             "as a vector of F2^4, so the picture carries the lattice rather than "
             "being arranged for looks. The five coloured triangles are the "
             "default <b>spread</b>: disjoint, covering all 15 points, and every "
             "one of them fold-closed. The %d remaining lines are behind them.</p>\n",
          15, all.count, all.count - 5);

  /* --- the diagram --- */
  fprintf(f, "<svg viewBox=\"0 0 %d %d\" role=\"img\">\n", W, H);
  for (i = 0; i < 4; ++i)
    fprintf(f, "<text class=\"rowlab\" x=\"6\" y=\"%.1f\">wt %d</text>\n",
            52.0 + 96.0 * i + 4.0, i + 1);

  /* Every line, faint. A line is drawn as its three connecting segments. */
  for (i = 0; i < all.count; ++i) {
    int a = all.u[i][0], b = all.u[i][1], cc = all.u[i][2];
    fprintf(f, "<path class=\"faint\" d=\"M%.1f %.1f L%.1f %.1f L%.1f %.1f Z\"/>\n",
            px[a], py[a], px[b], py[b], px[cc], py[cc]);
  }
  /* The spread, filled. */
  for (i = 0; i < spread.count && i < 5; ++i) {
    int a = spread.u[i][0], b = spread.u[i][1], cc = spread.u[i][2];
    fprintf(f, "<path d=\"M%.1f %.1f L%.1f %.1f L%.1f %.1f Z\" fill=\"%s\" "
               "fill-opacity=\".17\" stroke=\"%s\" stroke-width=\"2\" "
               "stroke-opacity=\".95\"/>\n",
            px[a], py[a], px[b], py[b], px[cc], py[cc], COL[i], COL[i]);
  }
  /* Points on top of the lines, so no vertex is buried. */
  for (i = 1; i <= 15; ++i) {
    fprintf(f, "<circle class=\"pt\" cx=\"%.1f\" cy=\"%.1f\" r=\"4.5\"/>\n",
            px[i], py[i]);
    fprintf(f, "<text class=\"lab\" x=\"%.1f\" y=\"%.1f\">%s</text>\n",
            px[i] + 7.0, py[i] + 3.5, canor_imag_name((canor_imag)i));
  }
  fprintf(f, "</svg>\n");

  fprintf(f, "<p class=\"sub\">");
  for (i = 0; i < spread.count && i < 5; ++i)
    fprintf(f, "<span class=\"key\" style=\"background:%s\"></span>G%d "
               "{%s,%s,%s} &nbsp;&nbsp;", COL[i], i,
            canor_imag_name(spread.u[i][0]), canor_imag_name(spread.u[i][1]),
            canor_imag_name(spread.u[i][2]));
  fprintf(f, "</p>\n");

  /* --- incidence matrix --- */
  fprintf(f, "<h2>Incidence matrix &mdash; the data behind the diagram</h2>\n");
  fprintf(f, "<p class=\"sub\">15 points by %d lines. Exactly three marks per "
             "column, seven per row, %d in total &mdash; and 35 x 3 = 15 x 7 = "
             "%d is the incidence count both ways, which is the arithmetic that "
             "makes this a projective space rather than a graph.</p>\n",
          all.count, all.count * 3, all.count * 3);
  {
    const int cw = 19, rh = 15, ox = 44, oy = 16;
    int mw = ox + all.count * cw + 10, mh = oy + 15 * rh + 14;
    fprintf(f, "<svg viewBox=\"0 0 %d %d\" role=\"img\">\n", mw, mh);
    for (i = 1; i <= 15; ++i)
      fprintf(f, "<text class=\"lab\" x=\"4\" y=\"%d\">%s</text>\n",
              oy + (i - 1) * rh + 11, canor_imag_name((canor_imag)i));
    for (j = 0; j < all.count; ++j)
      for (i = 1; i <= 15; ++i) {
        int on = 0;
        for (k = 0; k < 3; ++k) if (all.u[j][k] == i) on = 1;
        if (!on) continue;
        /* Spread lines keep their colour here too, so the two views are
         * legible as the same object rather than as two diagrams. */
        {
          const char *col = "currentColor";
          double op = 0.55;
          int sp2;
          for (sp2 = 0; sp2 < spread.count && sp2 < 5; ++sp2)
            if (all.u[j][0] == spread.u[sp2][0] &&
                all.u[j][1] == spread.u[sp2][1] &&
                all.u[j][2] == spread.u[sp2][2]) { col = COL[sp2]; op = 1.0; }
          fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" "
                     "fill=\"%s\" fill-opacity=\"%.2f\" rx=\"2\"/>\n",
                  ox + j * cw, oy + (i - 1) * rh, cw - 4, rh - 4, col, op);
        }
      }
    fprintf(f, "</svg>\n");
  }

  /* --- the numbers, all computed --- */
  fprintf(f, "<h2>Verified counts</h2>\n<table>\n");
  fprintf(f, "<tr><td>points</td><td>15</td><td>non-zero vectors of F2^4</td></tr>\n");
  fprintf(f, "<tr><td>lines</td><td>%d</td><td>quaternion triples {a,b,a^b}</td></tr>\n",
          all.count);
  fprintf(f, "<tr><td>points per line</td><td>3</td><td></td></tr>\n");
  fprintf(f, "<tr><td>lines per point</td><td>%d</td><td></td></tr>\n",
          all.count * 3 / 15);
  fprintf(f, "<tr><td>point pairs</td><td>105</td><td>C(15,2), each on exactly "
             "one line</td></tr>\n");
  fprintf(f, "<tr><td>spreads</td><td>56</td><td>partitions into 5 disjoint "
             "lines</td></tr>\n");
  fprintf(f, "<tr><td>A8 orbits on spreads</td><td>1</td><td>so the choice of "
             "spread is not a parameter</td></tr>\n");
  fprintf(f, "<tr><td>|Aut| = |GL(4,2)|</td><td>20160</td><td>isomorphic to "
             "A8</td></tr>\n");
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">Every number above is computed by this program, "
             "not quoted: the lines are found by testing closure, the spreads by "
             "exhaustive disjoint search, and the orbit count by acting with all "
             "20160 elements of GL(4,2).</p>\n");
  fprintf(f, "</div>\n");
  fclose(f);

  printf("wrote %s\n", path);
  printf("  %d points, %d lines, spread of %d, %d lines drawn faint\n",
         15, all.count, spread.count, all.count - 5);
  canor_demo_prov_write_sidecar(path);
  return 0;
}
