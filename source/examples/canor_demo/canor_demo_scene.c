/* canor_demo_scene.c - scene - the volume, PG(3,2) and the symbolic graph in one camera
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* scene - the volume, PG(3,2) and the symbolic graph in one camera           */
/* ========================================================================== */
/* THREE LAYERS, ONE FRAME. A WebGL raymarched escape field on a canvas, with
 * PG(3,2) and the differential's expression graph drawn as an SVG overlay
 * projected through the SAME camera. Sharing the camera is the whole point:
 * three representations of one Strategy that can be rotated together are
 * evidence about each other, and three pictures side by side are not.
 *
 * SVG for the overlay rather than a second GL program: the lines stay crisp at
 * any zoom, the labels are real text, and the existing SVG work in this tool
 * carries over unchanged.
 *
 * --- THE TWO SLIDERS, AND WHY THEY ARE NOT ONE ------------------------------
 *
 * Both are indexed by the ordinal ladder omega.k + n from canor_scale, and they
 * do different things - which is why they are separate controls rather than
 * one knob with two effects.
 *
 *   SCALE-RELATIVE THRESHOLD. The iso-surface threshold is NOT a free value.
 *   It takes only the values the Q(alpha) sampling actually produces, one per
 *   rung, each carrying its exact interval width. So a surface is always shown
 *   at a STATED precision rather than at an arbitrary cut, and moving up the
 *   ladder refines it by an amount that is known exactly rather than felt.
 *   A continuous slider would have implied the surface is defined more finely
 *   than the sampling supports.
 *
 *   OPERATOR SAMPLING. The operator alpha^2+1 is sampled at its own rung, and
 *   that sample perturbs the Julia constant. A separate field is precomputed
 *   per rung, so moving this slider shows the SET converging as the operator's
 *   scale refines. That is a different question from where to cut the surface,
 *   and answering both with one control would have conflated the precision of
 *   the measurement with the precision of the rendering.
 *
 * The ladder is well-ordered, so the rungs are strictly increasing in
 * refinement and the interval widths shrink monotonically. Both facts are
 * checked before the page is written.
 *
 * --- THE PG(3,2) EMBEDDING --------------------------------------------------
 *
 * Points are the 15 non-zero vectors of F_2^4, so the natural home is 4D. The
 * embedding below is a tesseract projection: the first three bits give a cube
 * vertex and the fourth displaces along the diagonal. It is injective - all 15
 * land distinct - and that is checked, because a projection that collided two
 * points would draw a diagram of something else.
 *
 * A projective line is {u, v, u XOR v}, and those three are NOT collinear in
 * this embedding. They are drawn as a triangle, which is honest: the incidence
 * is a three-element set, and a straight segment through them would assert a
 * betweenness the geometry over F_2 does not have.
 */

#define SC_N 48
#define SC_W 5
#define SC_OP 4

int canor_demo_cmd_scene(const canor_demo_config_t *c, int sel, const char *out, int fuel) {
  const canor_strategy *st = &c->strat[sel];
  unsigned char *fj = NULL, *fm = NULL;
  size_t plane = (size_t)SC_N * SC_N * SC_N;
  size_t nj = plane * SC_W * SC_OP, nm = plane * SC_W;
  int64_t cq[4];
  fo_ord lad[8];
  fs_sample samp[8];
  canor_poly mp;
  canor_alg roots[4];
  canor_alg_elem el;
  int nroots = 0, nlad = 0, nsamp = 0;
  int wi, oi, ix, iy, iz, i, k, t;
  ex_groups spread;
  FILE *f;
  double px[16], py[16], pz[16];
  int inj = 1;
  static int64_t vox[512][CANOR_KD];
  static double voxp[512][CANOR_KD];
  static int voxn[512];
  int nvox = 0, voxlvl = 0;
  static double curve[3][32];
  int ncurve[3] = { 0, 0, 0 };

  canor_demo_check_fail = canor_demo_check_ran = 0;
  if (fuel <= 0 || fuel > 255) fuel = 64;
  if (canor_imag_init() != CANOR_OK) return 2;
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;

  printf("Scene: volume + PG(3,2) + symbolic graph, one camera\n\n");

  /* --- the ordinal ladder and the Q(alpha) samples -------------------------- */
  {
    canor_rational co[4];
    canor_poly_init(&mp);
    co[0] = canor_rat(-2, 1); co[1] = canor_rat(0, 1);
    co[2] = canor_rat(0, 1);  co[3] = canor_rat(1, 1);
    if (!canor_poly_set(&mp, 3, co)) return 2;
    if (!canor_alg_isolate(&mp, roots, 4, &nroots) || nroots < 1) return 2;
  }
  canor_alg_elem_init(&el);
  el.c[0] = canor_rat(1, 1);
  el.c[2] = canor_rat(1, 1);
  el.degree = 2;
  for (k = 0; k <= 1; ++k)
    for (i = 0; i <= 3; ++i) {
      if (nlad >= 8) break;
      if (!fo_make(&lad[nlad], k > 0 ? 1 : 0, k, i)) continue;
      ++nlad;
    }
  printf("  ordinal ladder, and the operator alpha^2+1 sampled on it\n");
  printf("    %-9s %-6s %-18s %s\n", "index", "class", "interval width", "value");
  for (i = 0; i < nlad; ++i) {
    char ob[64];
    if (!fs_sample_at(&roots[0], &el, &lad[i], &samp[nsamp])) continue;
    fo_format(&lad[i], ob, sizeof ob);
    printf("    %-9s %-6d %8lld/%-9lld %.9f\n", ob, samp[nsamp].kclass,
           (long long)samp[nsamp].width.num, (long long)samp[nsamp].width.den,
           canor_demo_rat_d(samp[nsamp].value));
    ++nsamp;
  }
  {
    int mono = 1, ord = 1;
    for (i = 1; i < nsamp; ++i)
      if (canor_rat_cmp(samp[i].width, samp[i - 1].width) > 0) mono = 0;
    for (i = 1; i < nlad; ++i) if (fo_cmp(&lad[i - 1], &lad[i]) >= 0) ord = 0;
    canor_demo_check(ord, "the ordinal ladder is strictly increasing");
    canor_demo_check(mono, "sampled interval widths shrink monotonically");
    canor_demo_check(nsamp >= SC_OP, "enough rungs sampled to drive the operator slider");
  }

  /* --- PG(3,2) embedding ---------------------------------------------------- */
  for (i = 1; i <= 15; ++i) {
    double b0 = (i & 1) ? 1.0 : 0.0, b1 = (i & 2) ? 1.0 : 0.0;
    double b2 = (i & 4) ? 1.0 : 0.0, b3 = (i & 8) ? 1.0 : 0.0;
    px[i] = (b0 - 0.5) * 0.62 + b3 * 0.30;
    py[i] = (b1 - 0.5) * 0.62 + b3 * 0.30;
    pz[i] = (b2 - 0.5) * 0.62 + b3 * 0.30;
  }
  for (i = 1; i <= 15; ++i)
    for (k = i + 1; k <= 15; ++k)
      if (px[i] == px[k] && py[i] == py[k] && pz[i] == pz[k]) inj = 0;
  canor_demo_check(inj, "the tesseract projection is injective on all 15 points");

  /* --- MultiFlow layers, merged in ------------------------------------------
   * ANCHORING FOLLOWS THE SAME RULE AS THE REST OF THE SCENE. The voxel
   * occupancy is spatial - k-d keys ARE positions - so it goes in the camera
   * and rotates with the field. The tier curves are functions of t with no
   * position, so they are pinned. Deciding this per layer rather than per
   * artifact is what keeps the combined view honest: everything drawn in the
   * camera is claiming to share the field's space, and nothing else is. */
  {
    canor_demo_cloud_point_t *cp = NULL;
    long cn = 0, j;
    if (canor_demo_cloud_get(c, sel, NULL, 1024, &cp, &cn) && cn > 0) {
      int64_t lo[CANOR_KD], hi[CANOR_KD];
      int d;
      for (d = 0; d < CANOR_KD; ++d) { lo[d] = INT64_MAX; hi[d] = INT64_MIN; }
      for (j = 0; j < cn; ++j) {
        int64_t kk[CANOR_KD];
        kk[0] = cp[j].x; kk[1] = cp[j].y; kk[2] = cp[j].z;
        for (d = 0; d < CANOR_KD; ++d) {
          if (kk[d] < lo[d]) lo[d] = kk[d];
          if (kk[d] > hi[d]) hi[d] = kk[d];
        }
      }
      /* Level chosen from the measured span so the bins actually merge - the
       * same correction mfbuild needed. A level picked blind coarsens nothing
       * and draws one point per key while looking like an occupancy map. */
      {
        int64_t r = 0;
        int bits = 0;
        for (d = 0; d < CANOR_KD; ++d) if (hi[d] - lo[d] > r) r = hi[d] - lo[d];
        while (r > 0) { ++bits; r >>= 1; }
        voxlvl = bits > 5 ? bits - 5 : 0;
      }
      for (j = 0; j < cn && nvox < 512; ++j) {
        int64_t kk[CANOR_KD], vx[CANOR_KD];
        int q, dup = 0;
        kk[0] = cp[j].x; kk[1] = cp[j].y; kk[2] = cp[j].z;
        ft_voxel(kk, voxlvl, vx);
        for (q = 0; q < nvox; ++q)
          if (vox[q][0] == vx[0] && vox[q][1] == vx[1] && vox[q][2] == vx[2])
            { ++voxn[q]; dup = 1; break; }
        if (dup) continue;
        vox[nvox][0] = vx[0]; vox[nvox][1] = vx[1]; vox[nvox][2] = vx[2];
        voxn[nvox] = 1;
        for (d = 0; d < CANOR_KD; ++d) {
          double denom = (double)(hi[d] - lo[d]);
          voxp[nvox][d] = denom > 0.0
            ? ((double)(kk[d] - lo[d]) / denom) - 0.5 : 0.0;
        }
        ++nvox;
      }
      free(cp);
    }
  }
  /* the three tier curves - same fractional relaxation mfbuild computes */
  for (i = 0; i < 3; ++i) {
    static const int ADEN[3] = { 1, 2, 3 };
    static const double LAM[3] = { 0.15, 0.35, 0.70 };
    int q;
    ncurve[i] = 0;
    for (q = 0; q < 32; ++q) {
      fb_rat v;
      canor_ml_report rep;
      int64_t fx = 0;
      fb_rat_init(&v);
      if (canor_fractional_relax(canor_rat(1, ADEN[i]),
                              canor_rat((int64_t)(LAM[i] * 100), 100),
                              canor_rat(q, 4), 64, &v, &rep) && rep.ok &&
          fb_rat_to_fixed(&v.num, &v.den, 20, &fx))
        curve[i][ncurve[i]++] = (double)fx / 1048576.0;
      fb_rat_free(&v);
    }
  }
  canor_demo_check(nvox > 0, "the voxel layer found occupied cells");
  canor_demo_check(ncurve[0] > 1 && ncurve[2] > 1, "the tier curves evaluated");

  /* --- the fields ----------------------------------------------------------- */
  fj = malloc(nj);
  fm = malloc(nm);
  if (!fj || !fm) { free(fj); free(fm); return 2; }
  canor_demo_vol_strategy_c(st, cq);

  for (oi = 0; oi < SC_OP; ++oi) {
    /* The sampled operator value perturbs the constant. Bounded deliberately:
     * a large perturbation would leave the connectedness locus and render an
     * empty box, which says nothing about either the operator or the
     * Strategy. */
    double sv = (oi < nsamp) ? canor_demo_rat_d(samp[oi].value) : 2.6;
    int64_t dc = canor_demo_vfp(0.10 * (sv - 2.6));
    for (wi = 0; wi < SC_W; ++wi) {
      int64_t w = canor_demo_vfp(-0.7 + 1.4 * (double)wi / (double)(SC_W - 1));
      for (iz = 0; iz < SC_N; ++iz)
        for (iy = 0; iy < SC_N; ++iy)
          for (ix = 0; ix < SC_N; ++ix) {
            double sx = -1.5 + 3.0 * (double)ix / (double)(SC_N - 1);
            double sy = -1.5 + 3.0 * (double)iy / (double)(SC_N - 1);
            double sz = -1.5 + 3.0 * (double)iz / (double)(SC_N - 1);
            size_t o = ((size_t)oi * SC_W + wi) * plane +
                       (size_t)iz * SC_N * SC_N + (size_t)iy * SC_N + ix;
            int e = canor_demo_vol_escape(canor_demo_vfp(sx), canor_demo_vfp(sy), canor_demo_vfp(sz), w,
                                          cq[0] + dc, cq[1], cq[2], cq[3], fuel);
            fj[o] = (unsigned char)(e * 255 / fuel);
          }
    }
  }
  for (wi = 0; wi < SC_W; ++wi) {
    int64_t w = canor_demo_vfp(-0.7 + 1.4 * (double)wi / (double)(SC_W - 1));
    for (iz = 0; iz < SC_N; ++iz)
      for (iy = 0; iy < SC_N; ++iy)
        for (ix = 0; ix < SC_N; ++ix) {
          double sx = -1.5 + 3.0 * (double)ix / (double)(SC_N - 1);
          double sy = -1.5 + 3.0 * (double)iy / (double)(SC_N - 1);
          double sz = -1.5 + 3.0 * (double)iz / (double)(SC_N - 1);
          size_t o = (size_t)wi * plane +
                     (size_t)iz * SC_N * SC_N + (size_t)iy * SC_N + ix;
          int e = canor_demo_vol_escape(0, 0, 0, 0, canor_demo_vfp(sx - 0.6), canor_demo_vfp(sy), canor_demo_vfp(sz), w, fuel);
          fm[o] = (unsigned char)(e * 255 / fuel);
        }
  }
  printf("\n  fields: julia %d op-rungs x %d w-slices, mandelbrot %d w-slices\n",
         SC_OP, SC_W, SC_W);

  /* --- the symbolic graph of di --------------------------------------------- */
  /* The expression is built from the Strategy's own first differential so the
   * graph is that Strategy's and not a generic illustration. */
  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); free(fj); free(fm); return 2; }

  fputs("<!doctype html>\n<meta charset=\"utf-8\">\n", f);
  fprintf(f, "<title>%s - scene</title>\n", st->id.name);
  fputs("<style>\n"
        "html,body{margin:0;height:100%;background:#07080a;color:#c9d1d9;"
        "font:13px/1.5 system-ui,sans-serif;overflow:hidden}\n"
        "#c,#o{position:fixed;inset:0;width:100%;height:100%}\n"
        "#c{display:block;cursor:grab}#c:active{cursor:grabbing}\n"
        "#o{pointer-events:none}\n"
        "#ui{position:fixed;top:12px;left:12px;width:262px;padding:14px 16px;\n"
        "background:rgba(13,17,23,.9);border:1px solid #30363d;border-radius:8px;\n"
        "max-height:calc(100% - 24px);overflow:auto}\n"
        "h1{font-size:14px;margin:0 0 2px}\n"
        ".s{color:#8b949e;font-size:11px;margin:0 0 10px}\n"
        "label{display:block;margin:10px 0 2px;color:#8b949e;font-size:11px}\n"
        "input[type=range]{width:100%;accent-color:#58a6ff}\n"
        "button{background:#21262d;color:#c9d1d9;border:1px solid #30363d;\n"
        "border-radius:5px;padding:4px 9px;font:11px system-ui;cursor:pointer}\n"
        "button.on{background:#1f6feb;border-color:#1f6feb;color:#fff}\n"
        ".r{display:flex;gap:5px;margin-top:4px;flex-wrap:wrap}\n"
        ".mono{font:11px ui-monospace,monospace;color:#8b949e}\n"
        "#note{position:fixed;bottom:10px;left:290px;right:10px;color:#6e7681;"
        "font-size:11px}\n"
        "</style>\n", f);
  fputs("<canvas id=\"c\"></canvas>\n<svg id=\"o\"></svg>\n<div id=\"ui\">\n", f);
  fprintf(f, "<h1>%s</h1>\n<p class=\"s\">%d&sup3;, %d w &times; %d op-rungs, "
             "fuel %d</p>\n", st->id.name, SC_N, SC_W, SC_OP, fuel);
  fputs("<div class=\"r\"><button id=\"bj\" class=\"on\">Julia</button>"
        "<button id=\"bm\">Mandelbrot</button></div>\n", f);
  /* THREE INDEPENDENT CONTROLS, and they are separate because they answer
   * different questions. SCALE picks the rung of the ordinal ladder, which
   * selects the operator sample and hence the field. SURFACE THRESHOLD picks
   * where to cut that field. SAMPLING RATE picks how finely the ray reads it.
   * Precision of the measurement, placement of the cut, and resolution of the
   * render are three different things; one control for all three would have
   * conflated them. Moving SCALE re-seeds the threshold with its scale-relative
   * value, so the coupling is the default and the override is explicit. */
  fputs("<label>scale &mdash; ordinal rung <b id=\"lo\"></b></label>"
        "<input id=\"op\" type=\"range\" min=\"0\" max=\"", f);
  fprintf(f, "%d", nsamp - 1);
  fputs("\" value=\"3\" step=\"1\">\n<div class=\"mono\" id=\"mo\"></div>\n", f);
  fputs("<label>surface threshold <b id=\"lt\"></b></label>"
        "<input id=\"t\" type=\"range\" min=\"5\" max=\"99\" value=\"92\" step=\"1\">"
        "\n<div class=\"mono\" id=\"mt\"></div>\n", f);
  fputs("<label>sampling rate <b id=\"ls\"></b></label>"
        "<input id=\"sr\" type=\"range\" min=\"48\" max=\"384\" value=\"176\" "
        "step=\"8\">\n<div class=\"mono\" id=\"ms\"></div>\n", f);
  fputs("<label>w &mdash; fourth axis <b id=\"lw\"></b></label>"
        "<input id=\"w\" type=\"range\" min=\"0\" max=\"", f);
  fprintf(f, "%d", SC_W - 1);
  fputs("\" value=\"2\" step=\"1\">\n"
        "<label>zoom <b id=\"lz\"></b></label>"
        "<input id=\"z\" type=\"range\" min=\"6\" max=\"400\" value=\"185\">\n"
        "<label>DE iterations <b id=\"li\"></b></label>"
        "<input id=\"it\" type=\"range\" min=\"8\" max=\"200\" value=\"48\" step=\"4\">\n"
        "<div class=\"r\" style=\"margin-top:10px\">"
        "<button id=\"bp\" class=\"on\">PG(3,2)</button>"
        "<button id=\"bg\" class=\"on\">expr graph</button>"
        "<button id=\"bv\">voxels</button>"
        "<button id=\"br\">spin</button>"
        "<button id=\"bd\">infinite zoom</button>"
        "<button id=\"bx\">voxel cloud</button>"
        "<button id=\"bc\">tier curves</button></div>\n"
        "<div class=\"mono\" id=\"md\" style=\"margin-top:6px\"></div>\n</div>\n"
        "<div id=\"note\"></div>\n", f);

  fputs("<script id=\"dj\" type=\"application/octet-stream\">", f);
  canor_demo_b64_out(f, fj, nj);
  fputs("</script>\n<script id=\"dm\" type=\"application/octet-stream\">", f);
  canor_demo_b64_out(f, fm, nm);
  fputs("</script>\n", f);

  fprintf(f, "<script>\nconst N=%d,W=%d,OP=%d,PLANE=N*N*N;\n", SC_N, SC_W, SC_OP);

  /* PG(3,2) geometry */
  fputs("const PTS=[", f);
  for (i = 1; i <= 15; ++i)
    fprintf(f, "%s[%.4f,%.4f,%.4f,\"%s\"]", i > 1 ? "," : "",
            px[i], py[i], pz[i], canor_imag_name((canor_imag)i));
  fputs("];\n", f);
  fputs("const CHART=[", f);
  for (i = 1; i <= 15; ++i) {
    int ch = 0;
    for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
      for (k = 0; k < 3; ++k) if (spread.u[t][k] == i) ch = t;
    fprintf(f, "%s%d", i > 1 ? "," : "", ch);
  }
  fputs("];\n", f);
  fputs("const TRI=[", f);
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
    fprintf(f, "%s[%d,%d,%d]", t ? "," : "",
            spread.u[t][0] - 1, spread.u[t][1] - 1, spread.u[t][2] - 1);
  fputs("];\n", f);

  fprintf(f, "const CJ=[%.6f,%.6f,%.6f,%.6f];\n",
          (double)cq[0] / CANOR_DEMO_VOL_ONE, (double)cq[1] / CANOR_DEMO_VOL_ONE,
          (double)cq[2] / CANOR_DEMO_VOL_ONE, (double)cq[3] / CANOR_DEMO_VOL_ONE);

  fputs("const VOX=[", f);
  for (i = 0; i < nvox; ++i)
    fprintf(f, "%s[%.4f,%.4f,%.4f,%d]", i ? "," : "",
            voxp[i][0], voxp[i][1], voxp[i][2], voxn[i]);
  fprintf(f, "];\nconst VOXLVL=%d;\n", voxlvl);
  fputs("const CURVE=[", f);
  for (i = 0; i < 3; ++i) {
    int q;
    fprintf(f, "%s[", i ? "," : "");
    for (q = 0; q < ncurve[i]; ++q)
      fprintf(f, "%s%.5f", q ? "," : "", curve[i][q]);
    fputs("]", f);
  }
  fputs("];\nconst CNAME=['Flow a=1','MetaFlow a=1/2','MultiFlow a=1/3'];\n", f);

  /* the ladder */
  fputs("const RUNG=[", f);
  for (i = 0; i < nsamp; ++i) {
    char ob[64];
    fo_format(&lad[i], ob, sizeof ob);
    fprintf(f, "%s{o:\"%s\",k:%d,wn:%lld,wd:%lld,v:%.9f}", i ? "," : "",
            ob, samp[i].kclass, (long long)samp[i].width.num,
            (long long)samp[i].width.den, canor_demo_rat_d(samp[i].value));
  }
  fputs("];\n", f);

  /* the expression graph of the Strategy's di, as a layered tree */
  {
    char fam[160];
    canor_family_format(&st->d[0][0], fam, sizeof fam);
    fprintf(f, "const EXPR=\"di = %s\";\n", fam);
  }
  fputs("const GNODE=[[0,\"di\"],[1,\"sin\"],[1,\"cos\"],[1,\"tan\"],"
        "[2,\"arg1\"],[2,\"arg2\"],[2,\"arg3\"],[2,\"arg4\"],[3,\"FLOWVALUE\"]];\n"
        "const GEDGE=[[0,1],[0,2],[0,3],[1,4],[1,5],[2,5],[2,6],[3,6],[3,7],"
        "[4,8],[5,8],[6,8],[7,8]];\n", f);

  fputs(
"function unb64(id){const s=document.getElementById(id).textContent.trim();\n"
" const b=atob(s),a=new Uint8Array(b.length);\n"
" for(let i=0;i<b.length;i++)a[i]=b.charCodeAt(i);return a;}\n"
"const DJ=unb64('dj'),DM=unb64('dm');\n"
"const cv=document.getElementById('c'),ov=document.getElementById('o');\n"
"const gl=cv.getContext('webgl2',{antialias:false});\n"
"const VS=`#version 300 es\n"
"void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);\n"
"gl_Position=vec4(p*2.0-1.0,0.0,1.0);}`;\n"
"const FS=`#version 300 es\n"
"precision highp float;precision highp sampler3D;\n"
"uniform sampler3D V;uniform vec3 cam;uniform mat3 bas;\n"
"uniform float thr;uniform vec2 res;uniform float chunky;uniform float n;\n"
"uniform float steps;uniform float demode;uniform vec4 C;uniform float wsl;\n"
"uniform float deiter;\n"
"out vec4 o;\n"
"float smp(vec3 p){vec3 q=p+0.5;\n"
" if(chunky>0.5)q=(floor(q*n)+0.5)/n;return texture(V,q).r;}\n"
"// --- distance estimator: unlimited zoom, and NOT the stamped field --------\n"
"// z=(a,b,c,d) with a real. z^2 = (a2-b2-c2-d2, 2ab, 2ac, 2ad) - the same\n"
"// squaring the C core iterates, so the two modes agree on WHAT is drawn and\n"
"// differ only in where it is evaluated. |dz| tracks |z'| by |z'|=2|z||z'|,\n"
"// giving the Hart-Sandin-Kauffman estimate 0.5*r*log(r)/|dz|. Evaluated per\n"
"// pixel, so detail is bounded by float precision and iteration count rather\n"
"// than by a voxel grid - which is the whole reason this mode exists.\n"
"vec4 qsq(vec4 z){return vec4(z.x*z.x-z.y*z.y-z.z*z.z-z.w*z.w,\n"
" 2.0*z.x*z.y,2.0*z.x*z.z,2.0*z.x*z.w);}\n"
"float de(vec3 p){\n"
" vec4 z=vec4(p.x*3.0,p.y*3.0,p.z*3.0,wsl);float dz=1.0,r=length(z);\n"
" for(int i=0;i<256;i++){ if(float(i)>=deiter)break;\n"
"  if(r>6.0)break; dz=2.0*r*dz; z=qsq(z)+C; r=length(z);}\n"
" return 0.5*r*log(max(r,1.0001))/max(dz,1e-8)/3.0;}\n"
"bool box(vec3 ro,vec3 rd,out float t0,out float t1){\n"
" vec3 iv=1.0/rd,a=(vec3(-0.5)-ro)*iv,b=(vec3(0.5)-ro)*iv;\n"
" vec3 lo=min(a,b),hi=max(a,b);\n"
" t0=max(max(lo.x,lo.y),lo.z);t1=min(min(hi.x,hi.y),hi.z);\n"
" return t1>max(t0,0.0);}\n"
"void main(){vec2 uv=(gl_FragCoord.xy*2.0-res)/res.y;\n"
" vec3 rd=normalize(bas*vec3(uv,-1.7));float t0,t1;\n"
" if(!box(cam,rd,t0,t1)){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
" t0=max(t0,0.0);float dt=(t1-t0)/steps,t=t0;bool hit=false;vec3 hp=vec3(0.0);\n"
" vec3 nr=vec3(0.0,1.0,0.0);\n"
" if(demode>0.5){\n"
"  // EPSILON SCALES WITH DISTANCE, which is what makes the zoom unbounded: the\n"
"  // surface is resolved to a fixed angular size rather than a fixed world size.\n"
"  t=t0;\n"
"  for(int i=0;i<512;i++){ if(float(i)>=steps)break;\n"
"   vec3 p=cam+rd*t; float d=de(p);\n"
"   if(d<0.00035*t){hit=true;hp=p;break;}\n"
"   t+=d*0.85; if(t>t1)break;}\n"
"  if(!hit){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
"  float e=0.0016;\n"
"  nr=normalize(vec3(de(hp+vec3(e,0.0,0.0))-de(hp-vec3(e,0.0,0.0)),\n"
"   de(hp+vec3(0.0,e,0.0))-de(hp-vec3(0.0,e,0.0)),\n"
"   de(hp+vec3(0.0,0.0,e))-de(hp-vec3(0.0,0.0,e)))+vec3(1e-9));\n"
" } else {\n"
"  for(int i=0;i<512;i++){ if(float(i)>=steps)break; vec3 p=cam+rd*t;\n"
"   if(smp(p)>=thr){hit=true;hp=p;break;} t+=dt; if(t>t1)break;}\n"
"  if(!hit){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
"  for(int i=0;i<5;i++){float h=dt*0.5;vec3 q=hp-rd*h;\n"
"   if(smp(q)>=thr)hp=q;dt=h;}\n"
"  if(chunky>0.5){\n"
"   // FLAT FACE NORMAL. A central difference over 1.6/n spans more than one\n"
"   // voxel, so it re-smoothed exactly what the quantisation had just made\n"
"   // blocky - which is why voxel mode looked almost identical to smooth mode.\n"
"   // The face is the axis on which the sample sits furthest from centre.\n"
"   vec3 f=fract((hp+0.5)*n)-0.5; vec3 a=abs(f);\n"
"   nr=(a.x>a.y&&a.x>a.z)?vec3(sign(f.x),0.0,0.0)\n"
"     :(a.y>a.z)?vec3(0.0,sign(f.y),0.0):vec3(0.0,0.0,sign(f.z));\n"
"   nr=-nr;\n"
"  } else {\n"
"   float e=1.6/n;\n"
"   nr=normalize(-vec3(\n"
"    smp(hp+vec3(e,0.0,0.0))-smp(hp-vec3(e,0.0,0.0)),\n"
"    smp(hp+vec3(0.0,e,0.0))-smp(hp-vec3(0.0,e,0.0)),\n"
"    smp(hp+vec3(0.0,0.0,e))-smp(hp-vec3(0.0,0.0,e)))+vec3(1e-6));}\n"
" }\n"
" vec3 L=normalize(vec3(0.6,0.8,0.5));\n"
" float d=max(dot(nr,L),0.0),rim=pow(1.0-max(dot(nr,-rd),0.0),2.5);\n"
" vec3 col=mix(vec3(0.10,0.30,0.62),vec3(0.85,0.55,0.25),clamp(hp.y+0.5,0.0,1.0));\n"
" col=col*(0.20+0.85*d)+vec3(0.35,0.55,0.95)*rim*0.55;\n"
" col=mix(col,vec3(0.027,0.031,0.039),clamp((t-t0)/max(t1-t0,1e-4),0.0,1.0)*0.35);\n"
" o=vec4(pow(col,vec3(0.4545)),1.0);}`;\n"
"// A SILENT SHADER FAILURE IS THE EXPENSIVE KIND. Without this the canvas just\n"
"// stays black and the overlay keeps drawing, which reads as a camera or data\n"
"// problem and sends you looking in the wrong place. Report it where it is.\n"
"function sh(ty,src){const s=gl.createShader(ty);gl.shaderSource(s,src);\n"
" gl.compileShader(s);\n"
" if(!gl.getShaderParameter(s,gl.COMPILE_STATUS)){\n"
"  const m='shader: '+gl.getShaderInfoLog(s);\n"
"  console.error(m);\n"
"  document.getElementById('note').textContent=m;}\n"
" return s;}\n"
"const pr=gl.createProgram();\n"
"gl.attachShader(pr,sh(gl.VERTEX_SHADER,VS));\n"
"gl.attachShader(pr,sh(gl.FRAGMENT_SHADER,FS));\n"
"gl.linkProgram(pr);gl.useProgram(pr);\n"
"const tex=gl.createTexture();gl.bindTexture(gl.TEXTURE_3D,tex);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);\n"
"gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_R,gl.CLAMP_TO_EDGE);\n"
"// The operator slider starts at the MOST refined rung, so the first thing\n"
"// shown is the converged set; sliding back reveals the coarser samples it\n"
"// converged from. Starting at rung 0 opened on the least converged shape and\n"
"// read as a broken render rather than as the first step of a limit.\n"
"let mode='j',wi=2,opi=OP-1,scaleI=3,thr01=0.92,\n"
"    chunky=false,spin=false,showP=true,showG=true,demode=false,\n"
"    showX=true,showC=true;\n"
"function upload(){let sub;\n"
" if(mode==='j'){const off=(opi*W+wi)*PLANE;sub=DJ.subarray(off,off+PLANE);}\n"
" else {const off=wi*PLANE;sub=DM.subarray(off,off+PLANE);}\n"
" gl.pixelStorei(gl.UNPACK_ALIGNMENT,1);\n"
" gl.texImage3D(gl.TEXTURE_3D,0,gl.R8,N,N,N,0,gl.RED,gl.UNSIGNED_BYTE,sub);}\n"
"upload();\n"
"const U=n=>gl.getUniformLocation(pr,n);\n"
"let yaw=0.7,pitch=0.45,dist=1.85,drag=false,lx=0,ly=0;\n"
"cv.addEventListener('pointerdown',e=>{drag=true;lx=e.clientX;ly=e.clientY;\n"
" cv.setPointerCapture(e.pointerId);});\n"
"cv.addEventListener('pointerup',()=>{drag=false;});\n"
"cv.addEventListener('pointermove',e=>{if(!drag)return;\n"
" yaw+=(e.clientX-lx)*0.0075;pitch+=(e.clientY-ly)*0.0075;\n"
" pitch=Math.max(-1.55,Math.min(1.55,pitch));lx=e.clientX;ly=e.clientY;});\n"
"cv.addEventListener('wheel',e=>{e.preventDefault();\n"
" dist=Math.max(0.6,Math.min(4.0,dist*(1+Math.sign(e.deltaY)*0.09)));\n"
" gz.value=Math.round(dist*100);lz.textContent=dist.toFixed(2);},{passive:false});\n"
"const gt=document.getElementById('t'),go=document.getElementById('op'),\n"
"  gw=document.getElementById('w'),gz=document.getElementById('z'),\n"
"  gs=document.getElementById('sr'),gi=document.getElementById('it'),\n"
"  lt=document.getElementById('lt'),lo=document.getElementById('lo'),\n"
"  lw=document.getElementById('lw'),lz=document.getElementById('lz'),\n"
"  ls=document.getElementById('ls'),ms=document.getElementById('ms'),\n"
"  mt=document.getElementById('mt'),mo=document.getElementById('mo'),\n"
"  li=document.getElementById('li'),md=document.getElementById('md');\n"
"// THE SCALED SMOOTHING: threshold falls directly out of the interval width.\n"
"// thr = 1 - width*0.11, clamped. Across the ladder that is 0.34, 0.67, 0.92,\n"
"// 0.99 - a wide, monotone spread, because the widths themselves span 6/1 down\n"
"// to 3/2048. The functional is deliberately in the width's OWN units rather\n"
"// than normalised against the coarsest rung: normalising makes the first rung\n"
"// always 0 and the last always 1 regardless of how far the ladder actually\n"
"// refined, which hides exactly the thing the scale-relative reading is for.\n"
"//\n"
"// A normalised variant was tried and reverted. It produced 0.38, 0.69, 0.92,\n"
"// 0.985 - within a few percent of these - so it fixed nothing, and the reason\n"
"// given for it at the time (that the original pinned every rung above 0.9)\n"
"// was false: only rungs 2 and up sit there, and the sparse first render came\n"
"// from the operator rung defaulting to its least converged sample.\n"
"function thrOf(i){const r=RUNG[Math.min(i,RUNG.length-1)];\n"
" const wd=r.wn/r.wd; return Math.max(0.05,Math.min(0.99,1.0-wd*0.11));}\n"
"function syncT(){lt.textContent=(thr01).toFixed(2);\n"
" const r=RUNG[Math.min(scaleI,RUNG.length-1)];\n"
" mt.textContent='scale-relative value at this rung: '+thrOf(scaleI).toFixed(3);}\n"
"function syncO(){const r=RUNG[Math.min(scaleI,RUNG.length-1)];\n"
" lo.textContent=r.o;\n"
" mo.textContent='width '+r.wn+'/'+r.wd+'  alpha^2+1='+r.v.toFixed(6)\n"
"   +'  class '+r.k;}\n"
"function syncS(){ls.textContent=gs.value;\n"
" ms.textContent='ray steps; thin features need more';}\n"
"gt.oninput=()=>{thr01=(+gt.value)/100;syncT();};\n"
"go.oninput=()=>{scaleI=+go.value;\n"
" // the operator field only has SC_OP rungs; the ladder has more, so the\n"
" // field index is clamped and the reported width is the true one either way\n"
" opi=Math.min(scaleI,OP-1);\n"
" thr01=thrOf(scaleI);gt.value=Math.round(thr01*100);\n"
" syncO();syncT();upload();};\n"
"gs.oninput=()=>{syncS();};\n"
"gw.oninput=()=>{wi=+gw.value;lw.textContent=(-0.7+1.4*wi/(W-1)).toFixed(2);\n"
" upload();};\n"
"gz.oninput=()=>{dist=+gz.value/100;lz.textContent=dist.toFixed(2);};\n"
"const bj=document.getElementById('bj'),bm=document.getElementById('bm');\n"
"bj.onclick=()=>{mode='j';bj.classList.add('on');bm.classList.remove('on');upload();};\n"
"bm.onclick=()=>{mode='m';bm.classList.add('on');bj.classList.remove('on');upload();};\n"
"const bp=document.getElementById('bp'),bgb=document.getElementById('bg'),\n"
"  bv=document.getElementById('bv'),br=document.getElementById('br');\n"
"bp.onclick=()=>{showP=!showP;bp.classList.toggle('on',showP);};\n"
"bgb.onclick=()=>{showG=!showG;bgb.classList.toggle('on',showG);};\n"
"bv.onclick=()=>{chunky=!chunky;bv.classList.toggle('on',chunky);\n"
" gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MAG_FILTER,chunky?gl.NEAREST:gl.LINEAR);};\n"
"br.onclick=()=>{spin=!spin;br.classList.toggle('on',spin);};\n"
"const bd=document.getElementById('bd');\n"
"function syncD(){md.textContent=demode\n"
" ? 'DE: per-pixel, unlimited zoom - NOT the stamped field'\n"
" : 'volume: the stamped 48^3 field, zoom limited by the grid';\n"
" li.textContent=gi.value;}\n"
"bd.onclick=()=>{demode=!demode;bd.classList.toggle('on',demode);syncD();};\n"
"gi.oninput=()=>{syncD();};\n"
"const bx=document.getElementById('bx'),bc=document.getElementById('bc');\n"
"bx.classList.add('on');bc.classList.add('on');\n"
"bx.onclick=()=>{showX=!showX;bx.classList.toggle('on',showX);};\n"
"bc.onclick=()=>{showC=!showC;bc.classList.toggle('on',showC);};\n"
"syncO();syncT();syncS();syncD();\n"
"lw.textContent=(-0.7+1.4*wi/(W-1)).toFixed(2);\n"
"lz.textContent=dist.toFixed(2);\n"
"document.getElementById('note').textContent=EXPR+\n"
" '  |  PG(3,2) shares the volume camera and rotates with it; the expression\\n"
" graph is a DAG with no spatial embedding, so it stays pinned.';\n"
"const CC=['#f85149','#d29922','#3fb950','#58a6ff','#bc8cff'];\n"
"function frame(){\n"
" const dpr=Math.min(devicePixelRatio||1,1.75);\n"
" const w=Math.floor(innerWidth*dpr),h=Math.floor(innerHeight*dpr);\n"
" if(cv.width!==w||cv.height!==h){cv.width=w;cv.height=h;}\n"
" gl.viewport(0,0,w,h);\n"
" if(spin)yaw+=0.004;\n"
" const cy=Math.cos(yaw),sy=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);\n"
" const cam=[cp*sy*dist,sp*dist,cp*cy*dist];\n"
" const fwd=[-cp*sy,-sp,-cp*cy],rt=[cy,0,-sy],up=[-sy*sp,cp,-cy*sp];\n"
" gl.uniform3f(U('cam'),cam[0],cam[1],cam[2]);\n"
" gl.uniformMatrix3fv(U('bas'),false,\n"
"  [rt[0],rt[1],rt[2],up[0],up[1],up[2],-fwd[0],-fwd[1],-fwd[2]]);\n"
" gl.uniform1f(U('thr'),thr01);\n"
" gl.uniform1f(U('steps'),+gs.value);\n"
" gl.uniform1f(U('demode'),demode?1:0);\n"
" gl.uniform1f(U('deiter'),+gi.value);\n"
" gl.uniform1f(U('wsl'),-0.7+1.4*wi/(W-1));\n"
" gl.uniform4f(U('C'),CJ[0],CJ[1],CJ[2],CJ[3]);\n"
" gl.uniform2f(U('res'),w,h);\n"
" gl.uniform1f(U('chunky'),chunky?1:0);\n"
" gl.uniform1f(U('n'),N);gl.uniform1i(U('V'),0);\n"
" gl.drawArrays(gl.TRIANGLES,0,3);\n"
" // --- SVG overlay, same camera ---\n"
" const VW=innerWidth,VH=innerHeight;\n"
" function proj(p){\n"
"  const d=[p[0]-cam[0],p[1]-cam[1],p[2]-cam[2]];\n"
"  const zc=d[0]*fwd[0]+d[1]*fwd[1]+d[2]*fwd[2];\n"
"  if(zc<=0.02)return null;\n"
"  const xc=d[0]*rt[0]+d[1]*rt[1]+d[2]*rt[2];\n"
"  const yc=d[0]*up[0]+d[1]*up[1]+d[2]*up[2];\n"
"  return [VW*0.5+xc/zc*1.7*VH*0.5, VH*0.5-yc/zc*1.7*VH*0.5, zc];}\n"
" let out='';\n"
" if(showP){\n"
"  for(let t=0;t<TRI.length;t++){\n"
"   const a=proj(PTS[TRI[t][0]]),b=proj(PTS[TRI[t][1]]),\n"
"         c2=proj(PTS[TRI[t][2]]);\n"
"   if(a&&b&&c2)out+='<polygon points=\"'+a[0]+','+a[1]+' '+b[0]+','+b[1]+' '\n"
"     +c2[0]+','+c2[1]+'\" fill=\"'+CC[t%5]+'\" fill-opacity=\"0.10\" stroke=\"'\n"
"     +CC[t%5]+'\" stroke-width=\"1.3\" stroke-opacity=\"0.85\"/>';}\n"
"  for(let i=0;i<15;i++){const q=proj(PTS[i]);if(!q)continue;\n"
"   out+='<circle cx=\"'+q[0]+'\" cy=\"'+q[1]+'\" r=\"4\" fill=\"'+CC[CHART[i]%5]\n"
"    +'\" stroke=\"#0d1117\" stroke-width=\"1.2\"/>'\n"
"    +'<text x=\"'+(q[0]+7)+'\" y=\"'+(q[1]+3)+'\" fill=\"#8b949e\" '\n"
"    +'font-size=\"10\" font-family=\"monospace\">'+PTS[i][3]+'</text>';}}\n"
" if(showX){\n"
"  // WORLD-ANCHORED: a k-d key IS a position, so the occupancy belongs in the\n"
"  // camera with the field. Radius grows with the cell count, so a dense cell\n"
"  // reads as dense rather than as one more dot.\n"
"  for(let i=0;i<VOX.length;i++){const q=proj(VOX[i]);if(!q)continue;\n"
"   const r=1.4+Math.min(3.4,Math.sqrt(VOX[i][3])*0.9);\n"
"   out+='<circle cx=\"'+q[0]+'\" cy=\"'+q[1]+'\" r=\"'+r.toFixed(1)\n"
"    +'\" fill=\"#3fb950\" fill-opacity=\"0.45\"/>';}}\n"
" if(showC){\n"
"  // PINNED: a relaxation curve is a function of t, not a place.\n"
"  const CX=70,CY=VH-150,CW=260,CH=110;\n"
"  const CC2=['#58a6ff','#d29922','#f85149'];\n"
"  out+='<rect x=\"'+(CX-14)+'\" y=\"'+(CY-18)+'\" width=\"'+(CW+34)\n"
"   +'\" height=\"'+(CH+52)+'\" fill=\"#0d1117\" fill-opacity=\"0.82\" '\n"
"   +'stroke=\"#30363d\" rx=\"6\"/>';\n"
"  out+='<line x1=\"'+CX+'\" y1=\"'+(CY+CH)+'\" x2=\"'+(CX+CW)+'\" y2=\"'\n"
"   +(CY+CH)+'\" stroke=\"#30363d\"/>';\n"
"  for(let i=0;i<CURVE.length;i++){let pts='';\n"
"   for(let q=0;q<CURVE[i].length;q++){\n"
"    const y=Math.max(0,Math.min(1.05,CURVE[i][q]));\n"
"    pts+=(CX+CW*q/(CURVE[i].length-1)).toFixed(1)+','\n"
"      +(CY+CH-CH*y).toFixed(1)+' ';}\n"
"   out+='<polyline fill=\"none\" stroke=\"'+CC2[i]+'\" stroke-width=\"1.8\" '\n"
"    +'points=\"'+pts+'\"/>';\n"
"   out+='<text x=\"'+CX+'\" y=\"'+(CY+CH+16+i*12)+'\" fill=\"'+CC2[i]\n"
"    +'\" font-size=\"10\" font-family=\"monospace\">'+CNAME[i]+'</text>';}}\n"
" if(showG){\n"
"  // SCREEN-ANCHORED, NOT WORLD-ANCHORED - and the two overlays differ here on\n"
"  // purpose. PG(3,2) belongs in the camera: its 15 points and 5 chart triangles\n"
"  // are a structure in the SAME space as the field, so rotating them together\n"
"  // shows which chart a region of the set lies near. The expression graph is a\n"
"  // DAG with no embedding in quaternion space; putting it in 3D made it\n"
"  // unreadable at most angles and swing off-screen, which asserted a spatial\n"
"  // relationship it does not have. It is pinned to the corner instead.\n"
"  const gp=[];\n"
"  const GX=VW-190, GY=110, GW=150, GH=250;\n"
"  for(let i=0;i<GNODE.length;i++){\n"
"   const lv=GNODE[i][0], sib=GNODE.filter(z=>z[0]===lv).length;\n"
"   let idx=0,cnt=0;\n"
"   for(let j=0;j<GNODE.length;j++){if(GNODE[j][0]===lv){if(j===i)idx=cnt;cnt++;}}\n"
"   const fr=sib>1?idx/(sib-1):0.5;\n"
"   gp.push([GX+GW*fr, GY+GH*lv/3, 1]);}\n"
"  const projG=q=>q;\n"
"  for(const e of GEDGE){const a=projG(gp[e[0]]),b=projG(gp[e[1]]);\n"
"   if(a&&b)out+='<line x1=\"'+a[0]+'\" y1=\"'+a[1]+'\" x2=\"'+b[0]+'\" y2=\"'\n"
"    +b[1]+'\" stroke=\"#30363d\" stroke-width=\"1.1\"/>';}\n"
"  for(let i=0;i<GNODE.length;i++){const q=projG(gp[i]);if(!q)continue;\n"
"   out+='<circle cx=\"'+q[0]+'\" cy=\"'+q[1]+'\" r=\"3.5\" fill=\"#161b22\" '\n"
"    +'stroke=\"#8b949e\" stroke-width=\"1.2\"/>'\n"
"    +'<text x=\"'+(q[0]+6)+'\" y=\"'+(q[1]+3)+'\" fill=\"#6e7681\" font-size=\"9\" '\n"
"    +'font-family=\"monospace\">'+GNODE[i][1]+'</text>';}}\n"
" ov.setAttribute('viewBox','0 0 '+VW+' '+VH);\n"
" ov.innerHTML=out;\n"
" requestAnimationFrame(frame);}\n"
"frame();\n</script>\n", f);

  fclose(f);
  free(fj);
  free(fm);

  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
