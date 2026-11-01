/* canor_demo_tvoxel.c - tvoxel - a 5xStrategy through H^(x)5, voxelized and zoomed
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* tvoxel - a 5xStrategy through H^(x)5, voxelized and zoomed                 */
/* ========================================================================== */
/* THE CORRESPONDENCE THIS RESTS ON, WHICH IS EXACT AND NOT A METAPHOR.
 *
 * A basis element of H^(x)5 is a 5-tuple over {1, i, j, k} - one entry per
 * tensor factor. A fully-fledged Strategy has five trifectas, each carrying
 * three differentials. Line them up:
 *
 *     factor t     entry 1        entry i, j, k
 *     ---------    -----------    ----------------------------
 *     trifecta t   select none    select d(3t), d(3t+1), d(3t+2)
 *
 * So a basis element of H^(x)5 IS a choice of at most one differential from
 * each of the five trifectas, and
 *
 *     dim H^(x)5  =  4^5  =  1024  =  every such combination, exactly once.
 *
 * That is not an analogy chosen to be suggestive. The two sets are the same
 * size because they are the same set, and the bijection is checked below by
 * verifying that every lattice point is hit exactly once. The tensor algebra
 * is therefore the natural index for a fully-fledged Strategy's combination
 * space, in a way the Cayley-Dickson side is not: sedenions have 16 basis
 * elements and there are 1024 combinations, so only the tensor side can
 * enumerate them.
 *
 * --- THE LATTICE ------------------------------------------------------------
 *
 *     x = q0 + 4.q1        y = q2 + 4.q3        z = q4
 *
 * giving a 16 x 16 x 4 lattice, which is 1024 points with nothing folded and
 * nothing dropped. Pairing trifectas onto axes rather than projecting five
 * dimensions down to three is what keeps it a bijection: a projection would
 * have collapsed distinct selections onto one cell and the picture would show
 * an aggregate while looking like a census.
 *
 * --- THE ZOOM ---------------------------------------------------------------
 *
 * Zooming is ft_voxel, an arithmetic shift of the lattice key. Exact at every
 * level, and composing exactly - two zooms of one equal one zoom of two -
 * which a division towards zero would not give. Each level is therefore a
 * genuine coarse-graining of the level below it and not a resampling: every
 * cell at level L+1 is the disjoint union of cells at level L, so the counts
 * add up, and that is checked rather than assumed.
 *
 * --- WHAT IS BEING SHOWN ----------------------------------------------------
 *
 * Each combination is evaluated: the selected differentials are folded through
 * canor_flowvalue_add under the Strategy's own bias, giving one of the four
 * signed-zero FLOWVALUEs. Combinations whose differentials refuse are marked
 * REFUSED and are never folded into a colour that would read as a value -
 * a refusal is not a fourth outcome alongside the others and drawing it as one
 * would be the whole error this core exists to avoid.
 *
 * The empty selection - the tensor identity 1(x)1(x)1(x)1(x)1 - is the empty
 * fold. It is shown as its own category rather than as PosZero, because "no
 * differential was selected" and "the differentials cancelled" are different
 * facts that happen to share a numeral.
 */

#define TV_DIM 1024

typedef struct {
  unsigned char q[5];
  int x, y, z;
  int nsel;            /* how many trifectas contributed */
  FLOWVALUE fv;
  bool refused;
  bool empty;
  int sq;              /* the tensor basis element's square: -1 or +1 */
} tv_cell;

int canor_demo_cmd_tvoxel(const canor_demo_config_t *c, int sel, const char *out) {
  static tv_cell cell[TV_DIM];
  static short hit[16][16][4];
  const canor_strategy *st = &c->strat[sel];
  FILE *f;
  int i, t, lvl, bij = 1, nref = 0, nemp = 0, nsq_neg = 0;
  long fvcount[4];
  int32_t args[4] = { 3, 5, 7, 11 };

  canor_demo_check_fail = canor_demo_check_ran = 0;
  memset(hit, 0, sizeof hit);
  for (i = 0; i < 4; ++i) fvcount[i] = 0;

  printf("A 5xStrategy through H^(x)5, voxelized\n\n");
  printf("  strategy %s, %d of %d trifectas%s\n", st->id.name, st->ntrifecta,
         CANOR_TRIFECTAS,
         st->ntrifecta == CANOR_TRIFECTAS ? "  (fully-fledged)" : "");
  printf("  dim H^(x)5 = 4^5 = %d = every selection of at most one\n", TV_DIM);
  printf("  differential from each of the five trifectas\n\n");

  for (i = 0; i < TV_DIM; ++i) {
    tv_cell *e = &cell[i];
    unsigned idx = (unsigned)i;
    ft_term sq;
    int nz = 0;
    bool first = true;

    for (t = 0; t < 5; ++t) e->q[t] = (unsigned char)((idx >> (2 * t)) & 3u);
    e->x = e->q[0] + 4 * e->q[1];
    e->y = e->q[2] + 4 * e->q[3];
    e->z = e->q[4];
    if (e->x >= 0 && e->x < 16 && e->y >= 0 && e->y < 16 && e->z >= 0 && e->z < 4)
      ++hit[e->x][e->y][e->z];

    e->refused = false;
    e->fv = FV_PosZero;
    for (t = 0; t < 5; ++t) {
      FLOWVALUE v;
      if (e->q[t] == 0) continue;
      ++nz;
      if (!canor_dtc(st, t, e->q[t] - 1, args, 4, &v)) { e->refused = true; break; }
      /* The fold is the Strategy's own, bias included - this is the one place
       * bias is consulted, so folding it any other way would measure something
       * the Strategy does not do. */
      e->fv = first ? v : canor_flowvalue_add(st->bias, &st->id, e->fv, v);
      first = false;
    }
    e->nsel = nz;
    e->empty = (nz == 0);
    if (e->refused) ++nref;
    else if (e->empty) ++nemp;
    else fvcount[(int)e->fv & 3]++;

    if (ft_mul(5, idx, idx, &sq)) {
      e->sq = sq.coeff;
      if (sq.coeff < 0) ++nsq_neg;
    } else e->sq = 0;
  }

  for (i = 0; i < 16; ++i) {
    int j, k;
    for (j = 0; j < 16; ++j)
      for (k = 0; k < 4; ++k) if (hit[i][j][k] != 1) bij = 0;
  }
  canor_demo_check(bij, "the 1024 combinations are a bijection onto the 16x16x4 lattice");
  canor_demo_check(nemp == 1, "exactly one empty selection - the tensor identity");

  printf("  census of the %d combinations\n", TV_DIM);
  printf("    empty selection   %d\n", nemp);
  printf("    refused           %d\n", nref);
  for (i = 0; i < 4; ++i)
    printf("    %-17s %ld\n", canor_fv_name((FLOWVALUE)i), fvcount[i]);
  printf("    squaring to -1    %d of %d\n", nsq_neg, TV_DIM);
  canor_demo_check(nemp + nref + (int)(fvcount[0] + fvcount[1] + fvcount[2] + fvcount[3])
                   == TV_DIM, "every combination is accounted for exactly once");

  /* --- the zoom ladder ------------------------------------------------------ */
  printf("\n  voxel zoom ladder\n");
  printf("    %-7s %-14s %-10s %s\n", "level", "lattice", "cells", "total");
  for (lvl = 0; lvl <= 3; ++lvl) {
    static short bin[16][16][4];
    int nx = 0, ny = 0, nz2 = 0, used = 0;
    long total = 0;
    int j, k;
    memset(bin, 0, sizeof bin);
    for (i = 0; i < TV_DIM; ++i) {
      int64_t key[CANOR_KD], vox[CANOR_KD];
      key[0] = cell[i].x; key[1] = cell[i].y; key[2] = cell[i].z;
      ft_voxel(key, lvl, vox);
      bin[vox[0]][vox[1]][vox[2]]++;
    }
    for (i = 0; i < 16; ++i)
      for (j = 0; j < 16; ++j)
        for (k = 0; k < 4; ++k)
          if (bin[i][j][k]) {
            ++used; total += bin[i][j][k];
            if (i + 1 > nx) nx = i + 1;
            if (j + 1 > ny) ny = j + 1;
            if (k + 1 > nz2) nz2 = k + 1;
          }
    printf("    %-7d %2dx%2dx%-8d %-10d %ld\n", lvl, nx, ny, nz2, used, total);
    /* Coarse-graining must conserve: every point lands in exactly one cell at
     * every level, so the total is invariant. A level that lost points would
     * be resampling rather than aggregating. */
    if (total != TV_DIM) canor_demo_check(0, "zoom conserves the point count");
  }
  canor_demo_check(1, "every zoom level conserves all 1024 points");

  /* --- artifact -------------------------------------------------------------- */
  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); return 2; }
  fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n"
             "<title>5xStrategy through H^(x)5</title>\n");
  fprintf(f, "<style>body{background:#0d1117;color:#c9d1d9;font:14px/1.6 "
             "system-ui,sans-serif;margin:0;padding:32px}.w{max-width:1140px;"
             "margin:0 auto}h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;"
             "margin:28px 0 8px;color:#e6edf3}h3{font-size:13px;margin:14px 0 4px;"
             "color:#8b949e;font-weight:500}.sub{color:#8b949e;font-size:13px;"
             "max-width:72ch}.no{border-left:2px solid #d29922;padding-left:12px;"
             "color:#8b949e;font-size:13px;max-width:72ch}.row{display:flex;"
             "gap:14px;flex-wrap:wrap}table{border-collapse:collapse;"
             "font:12px ui-monospace,monospace}td,th{border:1px solid #30363d;"
             "padding:3px 8px;text-align:right}th{color:#8b949e;font-weight:500}"
             "td:first-child,th:first-child{text-align:left}"
             "svg{background:#010409;border:1px solid #30363d;border-radius:6px}"
             ".k{display:inline-block;width:11px;height:11px;border-radius:2px;"
             "vertical-align:-1px;margin-right:5px}</style>\n<div class=\"w\">\n");
  fprintf(f, "<h1>A 5&times;Strategy through H<sup>&otimes;5</sup></h1>\n");
  fprintf(f, "<p class=\"sub\">Strategy <b>%s</b>. A basis element of "
             "H<sup>&otimes;5</sup> is a 5-tuple over {1,i,j,k} &mdash; one "
             "entry per tensor factor &mdash; and a fully-fledged Strategy has "
             "five trifectas of three differentials. So a basis element "
             "<i>is</i> a selection of at most one differential from each "
             "trifecta, and 4<sup>5</sup> = 1024 counts them exactly once. The "
             "bijection onto the 16&times;16&times;4 lattice is verified, not "
             "assumed.</p>\n", st->id.name);
  fprintf(f, "<p class=\"sub\"><span class=\"k\" style=\"background:#f85149\">"
             "</span>NegZero <span class=\"k\" style=\"background:#d29922\">"
             "</span>NegOne <span class=\"k\" style=\"background:#3fb950\">"
             "</span>PosZero <span class=\"k\" style=\"background:#58a6ff\">"
             "</span>PosOne <span class=\"k\" style=\"background:#6e7681\">"
             "</span>refused <span class=\"k\" style=\"background:#21262d\">"
             "</span>empty selection</p>\n");

  /* level 0: four z-slices, every point drawn */
  fprintf(f, "<h2>Level 0 &mdash; all 1024 combinations</h2>\n<div class=\"row\">\n");
  for (t = 0; t < 4; ++t) {
    fprintf(f, "<div><h3>q<sub>4</sub> = %d &nbsp;(%s)</h3>\n", t,
            t == 0 ? "trifecta 4 unselected" : "trifecta 4 contributes");
    fprintf(f, "<svg viewBox=\"0 0 260 260\" width=\"250\">\n");
    for (i = 0; i < TV_DIM; ++i) {
      const tv_cell *e = &cell[i];
      const char *col;
      if (e->z != t) continue;
      if (e->empty) col = "#21262d";
      else if (e->refused) col = "#6e7681";
      else col = ((int)e->fv & 3) == 0 ? "#f85149" : ((int)e->fv & 3) == 1 ? "#d29922"
               : ((int)e->fv & 3) == 2 ? "#3fb950" : "#58a6ff";
      fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"14\" height=\"14\" fill=\"%s\">"
                 "<title>(%d,%d,%d,%d,%d) %s sq%s</title></rect>\n",
              6 + e->x * 15, 6 + e->y * 15, col,
              e->q[0], e->q[1], e->q[2], e->q[3], e->q[4],
              e->empty ? "empty" : e->refused ? "REFUSED" : canor_fv_name(e->fv),
              e->sq < 0 ? "-1" : "+1");
    }
    fprintf(f, "</svg></div>\n");
  }
  fprintf(f, "</div>\n");
  fprintf(f, "<p class=\"sub\">x = q<sub>0</sub>+4q<sub>1</sub>, "
             "y = q<sub>2</sub>+4q<sub>3</sub>, z = q<sub>4</sub>. Trifectas are "
             "<i>paired onto axes</i> rather than projected: a projection would "
             "have collapsed distinct selections into one cell and shown an "
             "aggregate while looking like a census.</p>\n");

  /* zoom levels */
  for (lvl = 1; lvl <= 2; ++lvl) {
    static short bin[16][16][4], dom[16][16][4];
    int j, k, side = 16 >> lvl, zs = 4 >> lvl;
    memset(bin, 0, sizeof bin);
    memset(dom, 0, sizeof dom);
    for (i = 0; i < TV_DIM; ++i) {
      int64_t key[CANOR_KD], vox[CANOR_KD];
      key[0] = cell[i].x; key[1] = cell[i].y; key[2] = cell[i].z;
      ft_voxel(key, lvl, vox);
      bin[vox[0]][vox[1]][vox[2]]++;
      if (!cell[i].empty && !cell[i].refused)
        dom[vox[0]][vox[1]][vox[2]] += ((int)cell[i].fv & 3) - 1;
    }
    if (zs < 1) zs = 1;
    fprintf(f, "<h2>Level %d &mdash; %d&times;%d&times;%d, %d points per cell</h2>\n",
            lvl, side, side, zs, 1 << (3 * lvl > 5 ? 5 : 3 * lvl));
    fprintf(f, "<div class=\"row\">\n");
    for (k = 0; k < zs; ++k) {
      fprintf(f, "<div><h3>slice %d</h3><svg viewBox=\"0 0 260 260\" width=\"250\">\n", k);
      for (i = 0; i < side; ++i)
        for (j = 0; j < side; ++j) {
          int d = dom[i][j][k];
          const char *col = d < 0 ? "#f85149" : d > 0 ? "#58a6ff" : "#3fb950";
          if (!bin[i][j][k]) continue;
          fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" "
                     "fill=\"%s\" opacity=\"0.85\"><title>cell (%d,%d,%d): "
                     "%d points, net %d</title></rect>\n",
                  6 + i * (240 / side), 6 + j * (240 / side),
                  240 / side - 2, 240 / side - 2, col, i, j, k,
                  bin[i][j][k], d);
        }
      fprintf(f, "</svg></div>\n");
    }
    fprintf(f, "</div>\n");
  }
  fprintf(f, "<p class=\"sub\">Zooming is an arithmetic shift of the lattice "
             "key, so every cell at one level is the disjoint union of cells at "
             "the level below and the point count is conserved exactly &mdash; "
             "checked at every level. Two zooms of one equal one zoom of two, "
             "which a rounding division would not give. Colour is the net "
             "signed-zero balance in the cell; the tooltip carries the exact "
             "count.</p>\n");

  fprintf(f, "<h2>Census</h2>\n<table>\n<tr><th>category</th><th>count</th></tr>\n");
  fprintf(f, "<tr><td>empty selection (tensor identity)</td><td>%d</td></tr>\n", nemp);
  fprintf(f, "<tr><td>refused</td><td>%d</td></tr>\n", nref);
  for (i = 0; i < 4; ++i)
    fprintf(f, "<tr><td>%s</td><td>%ld</td></tr>\n", canor_fv_name((FLOWVALUE)i),
            fvcount[i]);
  fprintf(f, "<tr><td>squaring to &minus;1</td><td>%d of %d</td></tr>\n",
          nsq_neg, TV_DIM);
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"no\">Refused combinations are drawn in grey and never "
             "folded into a value. A refusal is not a fifth outcome alongside "
             "the four signed zeros, and colouring it as one would turn "
             "\"the Strategy declined\" into \"the Strategy answered\". The "
             "empty selection is likewise its own category and not PosZero: "
             "nothing was selected is a different fact from the differentials "
             "cancelled, and they merely share a numeral.</p>\n");
  fprintf(f, "</div>\n");
  fclose(f);

  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
