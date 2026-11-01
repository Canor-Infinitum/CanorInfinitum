/* canor_demo_cloud.c - point cloud - RGBA-XYZ
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* point cloud - RGBA-XYZ                                                     */
/* ========================================================================== */
/* A point carries a colour and a position, and both are already the right
 * shape for this algebra:
 *
 *   RGB   the three channels are the MetaFlow axis and, through the configured
 *         unit map, the letters of an IMAG word - so a colour has a Flow.
 *   A     alpha is how much of the point there is. It sets the WORD LENGTH,
 *         which is the natural reading: a faint point is a short word and
 *         carries less structure. Alpha zero is a point that is not there, and
 *         it lands in the refusal slot rather than being dropped silently.
 *   XYZ   position supplies the differential arguments x1..x4 (with A as the
 *         fourth), so the Flow says WHAT the point is and the position says
 *         how it is changing.
 *
 * WHY A LARGE SET. Bias is only consulted in one place - canor_flowvalue_add,
 * when a family sums two terms. A single point tells you nothing about which
 * BiasMode models a source, because most sums do not cancel and every mode
 * agrees when they do not. Only the cancelling ones carry information, they
 * are a small fraction, and you need a lot of points before that fraction is
 * a population rather than an anecdote. That is the whole argument for a
 * dataset rather than an example.
 *
 * The cloud is DETERMINISTIC - synthesised from the Strategy's own stream, or
 * read from a file - and `--emit` writes it back out. That matters for
 * verifiable coordination: the dataset is a frozen artifact with a stated
 * provenance, so a conclusion drawn from it can be re-derived by anyone with
 * the same file rather than trusted. */

/* Largest-remainder apportionment of `len` letters across the three channels,
 * emitted round-robin rather than in blocks. Blocks would make the fold depend
 * only on the counts, which throws away the one thing the fold is for: the
 * product is non-commutative, so the ORDER is content. */
int canor_demo_cloud_word(const canor_demo_config_t *c, const canor_demo_cloud_point_t *p, char *word,
                      int cap, int cnt[3]) {
  int tot = p->r + p->g + p->b, len, n = 0, i;
  int quota[3], rem[3], used = 0;
  if (cap > CANOR_FLOW_WORD_MAX) cap = CANOR_FLOW_WORD_MAX;
  len = 1 + (p->a * (cap - 2)) / 255;         /* alpha sets the length */
  cnt[0] = cnt[1] = cnt[2] = 0;
  if (tot <= 0 || p->a <= 0) { word[0] = 0; return 0; }
  {
    const int ch[3] = { p->r, p->g, p->b };
    for (i = 0; i < 3; ++i) {
      quota[i] = (ch[i] * len) / tot;
      rem[i] = (ch[i] * len) % tot;
      used += quota[i];
    }
    while (used < len) {                       /* hand out the remainder */
      int best = 0;
      for (i = 1; i < 3; ++i) if (rem[i] > rem[best]) best = i;
      ++quota[best]; rem[best] = -1; ++used;
    }
  }
  while (n < len) {                  /* round-robin interleave */
    int placed = 0;
    for (i = 0; i < 3 && n < len; ++i) {
      int m;
      if (quota[i] <= 0) continue;
      for (m = 0; m < CANOR_DEMO_AXIS_N; ++m) if (c->meta[m].channel == i) break;
      if (m == CANOR_DEMO_AXIS_N || !c->meta[m].unit) continue;
      word[n++] = c->meta[m].unit;
      ++cnt[i];
      --quota[i];
      placed = 1;
    }
    if (!placed) break;
  }
  word[n] = 0;
  return n;
}

/* Deterministic synthesis from the Strategy stream. Not random: the same
 * Strategy and the same index give the same point on any machine. */
static void cloud_synth(const canor_strategy *s, uint64_t i, canor_demo_cloud_point_t *p) {
  uint64_t u = canor_rand(&s->id, 0xC10DDull, i);
  uint64_t v = canor_rand(&s->id, 0xC10DEull, i);
  p->r = (int)(u & 0xff);
  p->g = (int)((u >> 8) & 0xff);
  p->b = (int)((u >> 16) & 0xff);
  p->a = (int)((u >> 24) & 0xff);
  p->x = (int32_t)(v & 0xffff) - 32768;
  p->y = (int32_t)((v >> 16) & 0xffff) - 32768;
  p->z = (int32_t)((v >> 32) & 0xffff) - 32768;
}

static bool cloud_load(const char *path, canor_demo_cloud_point_t **out, long *count) {
  FILE *f = fopen(path, "r");
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0, cap = 0;
  if (!f) return false;
  /* Line-oriented, skipping '#' and blanks, so a file this tool EMITTED can be
   * read back. A writer and a reader that disagree about the header make the
   * artifact unverifiable, which defeats the purpose of emitting it. */
  for (;;) {
    char line[256];
    canor_demo_cloud_point_t p;
    const char *q;
    int got;
    if (!fgets(line, sizeof line, f)) break;
    for (q = line; *q == ' ' || *q == '\t'; ++q) { }
    if (*q == '#' || *q == '\n' || *q == '\r' || *q == 0) continue;
    got = sscanf(q, "%d %d %d %d %d %d %d", &p.r, &p.g, &p.b, &p.a,
                 &p.x, &p.y, &p.z);
    if (got != 7) continue;
    if (n == cap) {
      canor_demo_cloud_point_t *np;
      cap = cap ? cap * 2 : 1024;
      np = (canor_demo_cloud_point_t *)realloc(pts, (size_t)cap * sizeof *np);
      if (!np) { free(pts); fclose(f); return false; }
      pts = np;
    }
    pts[n++] = p;
  }
  fclose(f);
  *out = pts;
  *count = n;
  return n > 0;
}

/* Fills the caller's buffer with the cloud, from a file or synthesised. */
/* --- v1-beta: word -> {R,G,B} string, read as a cloud --------------------
 *
 * The file is 370,100 words each mapped to a string over exactly R, G and B.
 * That IS an RGBA-XYZ point already, without reinterpretation: the three
 * channel COUNTS are the RGB triple this tool uses everywhere, and the code
 * length is the alpha, which is what canor_demo_cloud_word's length rule consumes.
 *
 * XYZ IS DERIVED FROM THE CODE, NOT INVENTED. The word's position in the file
 * would be alphabetical order, which is a fact about English spelling and not
 * about the encoding. Instead each axis accumulates a running signed walk over
 * the code - R steps x, G steps y, B steps z, with the sign alternating by
 * position - so two words with the same composition but a different ORDER land
 * in different places. That matters because the fold is non-commutative: order
 * is content, and a layout that ignored it would collapse points the algebra
 * keeps apart.
 *
 * WHAT IS DELIBERATELY NOT DONE. No trie, no prefix nesting. Prefix-stability
 * measured 1.2% on this file - code(prefix) almost never prefixes code(word) -
 * so the encoding is a hash, not a tree, and arranging it as one would assert
 * a structure the data does not have.
 *
 * The parse is deliberately minimal: it scans for "key":"value" pairs whose
 * value is non-empty and drawn only from RGB, and skips anything else. A
 * malformed or foreign line is ignored rather than guessed at. */
static bool cloud_load_beta(const char *path, canor_demo_cloud_point_t **out, long *count,
                            long want) {
  FILE *f = fopen(path, "r");
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0, cap = 0;
  int ch;
  char val[128];
  if (!f) return false;
  for (;;) {
    int inq = 0, vl = 0, iscode = 1;
    /* find the next quoted string that is a pure RGB code */
    while ((ch = fgetc(f)) != EOF && ch != '"') { }
    if (ch == EOF) break;
    while ((ch = fgetc(f)) != EOF && ch != '"') {
      if (vl < (int)sizeof val - 1) val[vl++] = (char)ch;
      if (ch != 'R' && ch != 'G' && ch != 'B') iscode = 0;
    }
    val[vl] = 0;
    (void)inq;
    if (!iscode || vl == 0) continue;
    if (n == cap) {
      canor_demo_cloud_point_t *np;
      long ncap = cap ? cap * 2 : 4096;
      np = (canor_demo_cloud_point_t *)realloc(pts, (size_t)ncap * sizeof *np);
      if (!np) { free(pts); fclose(f); return false; }
      pts = np; cap = ncap;
    }
    {
      canor_demo_cloud_point_t *p = &pts[n];
      long i;
      int32_t wx = 0, wy = 0, wz = 0;
      p->r = p->g = p->b = 0;
      for (i = 0; i < vl; ++i) {
        int sgn = (i & 1) ? -1 : 1;
        switch (val[i]) {
          case 'R': ++p->r; wx += sgn * (int32_t)(i + 1); break;
          case 'G': ++p->g; wy += sgn * (int32_t)(i + 1); break;
          default:  ++p->b; wz += sgn * (int32_t)(i + 1); break;
        }
      }
      /* Alpha is the code length, capped to the channel's range, since
       * canor_demo_cloud_word reads alpha as the word length it should generate. */
      p->a = vl > 255 ? 255 : vl;
      p->x = wx * 2048;
      p->y = wy * 2048;
      p->z = wz * 2048;
      ++n;
    }
    if (want > 0 && n >= want) break;
  }
  fclose(f);
  if (n == 0) { free(pts); return false; }
  *out = pts;
  *count = n;
  return true;
}

bool canor_demo_cloud_get(const canor_demo_config_t *c, int sel, const char *path, long want,
                      canor_demo_cloud_point_t **pts, long *n) {
  if (path) {
    /* A file whose rows are not 7 integers may still be a v1-beta map, so the
     * RGBA-XYZ reader is tried first and the beta reader is the fallback -
     * never the other way round, or an emitted cloud could be misparsed. */
    if (cloud_load(path, pts, n)) return true;
    if (cloud_load_beta(path, pts, n, want)) {
      fprintf(stderr, "follow: %s: read %ld v1-beta RGB codes as a cloud\n",
              path, *n);
      return true;
    }
    if (!cloud_load(path, pts, n)) {
      fprintf(stderr, "follow: %s: no readable RGBA-XYZ rows "
                      "(7 integers per line)\n", path);
      return false;
    }
    return true;
  }
  *pts = (canor_demo_cloud_point_t *)malloc((size_t)want * sizeof **pts);
  if (!*pts) { fprintf(stderr, "follow: out of memory\n"); return false; }
  for (long i = 0; i < want; ++i) cloud_synth(&c->strat[sel], (uint64_t)i, &(*pts)[i]);
  *n = want;
  return true;
}

int canor_demo_cmd_cloud(const canor_demo_config_t *c, int sel, const char *path, long want,
                     const char *emit) {
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0, i, empty = 0, nofold = 0;
  long joint[CANOR_DEMO_AXIS_N][8];
  int fi, mi;
  canor_flow_registry reg;

  if (!canor_demo_cloud_get(c, sel, path, want, &pts, &n)) return 2;
  memset(joint, 0, sizeof joint);

  if (emit) {
    FILE *f = fopen(emit, "w");
    if (!f) { free(pts); fprintf(stderr, "follow: cannot write %s\n", emit); return 2; }
    fprintf(f, "# RGBA-XYZ point cloud\n");
    fprintf(f, "# strategy %s v%u, %ld points\n", c->strat[sel].id.name,
            (unsigned)c->strat[sel].id.version, n);
    fprintf(f, "# R G B A X Y Z\n");
    for (i = 0; i < n; ++i)
      fprintf(f, "%d %d %d %d %d %d %d\n", pts[i].r, pts[i].g, pts[i].b,
              pts[i].a, (int)pts[i].x, (int)pts[i].y, (int)pts[i].z);
    fclose(f);
    printf("wrote %ld points to %s\n", n, emit);
    canor_demo_prov_write_sidecar(emit);
    printf("\n");
  }

  for (i = 0; i < n; ++i) {
    char word[CANOR_FLOW_WORD_MAX];
    int cnt[3], len;
    uint8_t u;
    int sg;
    len = canor_demo_cloud_word(c, &pts[i], word, CANOR_FLOW_WORD_MAX, cnt);
    if (len == 0) { ++empty; continue; }        /* alpha 0 or colourless */
    memset(&reg, 0, sizeof reg);
    if (!canor_flows_add(&reg, "p", word) || !canor_flow_fold(&reg, 0, &u, &sg)) {
      ++nofold;
      continue;
    }
    mi = canor_demo_meta_of(c, cnt);
    fi = (u < (uint8_t)c->flows.count) ? (int)u : c->flows.count;
    joint[mi][fi]++;
  }

  printf("point cloud: %ld points, source %s\n", n,
         path ? path : "synthesised from the Strategy stream");
  printf("  %ld with zero alpha or no colour (the refusal, not dropped)\n", empty);
  if (nofold) printf("  %ld whose word did not fold\n", nofold);
  printf("\njoint distribution, MetaFlow (RGB level) x Flow (IMAG level)\n\n");
  printf("  %-14s", "");
  for (fi = 0; fi < c->flows.count; ++fi) printf(" %10s", c->flows.flow[fi].name);
  printf(" %10s\n", "total");
  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) {
    long row = 0;
    printf("  %-14s", c->meta[mi].name);
    for (fi = 0; fi < c->flows.count; ++fi) { printf(" %10ld", joint[mi][fi]); row += joint[mi][fi]; }
    printf(" %10ld\n", row);
  }
  printf("  %-14s", "total");
  for (fi = 0; fi < c->flows.count; ++fi) {
    long col = 0;
    for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) col += joint[mi][fi];
    printf(" %10ld", col);
  }
  printf("\n");
  free(pts);
  return 0;
}

/* --- bias modelling ---------------------------------------------------------
 * The measurable quantity is not "which bias is right" - nothing here can know
 * that - but "how much of this dataset could tell the difference". Running the
 * same points under all four modes and counting where they diverge gives that
 * number directly, and it is the number to look at first: if it is zero, any
 * claim about bias drawn from this source is unfalsifiable, however large the
 * source is. */
int canor_demo_cmd_bias(const canor_demo_config_t *c, int sel, const char *path, long want) {
  static const BiasMode modes[4] = { LeftBiased, RightBiased, NeutralBiased, StrategyBiased };
  static const char *const mname[4] = { "left", "right", "neutral", "strategy" };
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0, i, evaluated = 0, sensitive = 0, undef = 0;
  long agree[4][4];
  long dist[4][4];                              /* mode x FLOWVALUE, for di */
  canor_strategy s[4];
  int m, m2, terms;

  if (!canor_demo_cloud_get(c, sel, path, want, &pts, &n)) return 2;
  memset(agree, 0, sizeof agree);
  memset(dist, 0, sizeof dist);
  for (m = 0; m < 4; ++m) { s[m] = c->strat[sel]; s[m].bias = modes[m]; }

  terms = c->strat[sel].d[0][0].nterms + c->strat[sel].d[0][1].nterms + c->strat[sel].d[0][2].nterms;

  for (i = 0; i < n; ++i) {
    char word[CANOR_FLOW_WORD_MAX];
    int cnt[3], len;
    int32_t args[4];
    FLOWVALUE v[4][3];
    bool ok = true;
    len = canor_demo_cloud_word(c, &pts[i], word, CANOR_FLOW_WORD_MAX, cnt);
    if (len == 0) continue;
    args[0] = pts[i].x; args[1] = pts[i].y; args[2] = pts[i].z; args[3] = pts[i].a;
    for (m = 0; m < 4 && ok; ++m)
      if (!canor_di(&s[m], args, 4, &v[m][0]) ||
          !canor_dj(&s[m], args, 4, &v[m][1]) ||
          !canor_dk(&s[m], args, 4, &v[m][2])) ok = false;
    if (!ok) { ++undef; continue; }
    ++evaluated;
    for (m = 0; m < 4; ++m) dist[m][v[m][0]]++;
    {
      int diverged = 0;
      for (m = 0; m < 4; ++m)
        for (m2 = m + 1; m2 < 4; ++m2) {
          int same = (v[m][0] == v[m2][0] && v[m][1] == v[m2][1] && v[m][2] == v[m2][2]);
          if (same) { agree[m][m2]++; agree[m2][m]++; }
          else diverged = 1;
        }
      if (diverged) ++sensitive;
    }
  }

  printf("bias modelling over %ld points, strategy %s\n\n", n, c->strat[sel].id.name);
  printf("  di/dj/dk have %d terms in total.\n", terms);
  if (terms <= 3)
    printf("  EVERY FAMILY IS A SINGLE TERM, so no sum is ever formed and\n"
           "  canor_flowvalue_add is never reached. Bias cannot affect this\n"
           "  Strategy's output at all - not rarely, never. A dataset of any\n"
           "  size will report zero sensitivity below, and that is a fact\n"
           "  about the Strategy, not about the data.\n");
  printf("\n  evaluated          %ld\n", evaluated);
  printf("  undefined          %ld  (division by a zero-valued operand)\n", undef);
  printf("  bias-sensitive     %ld", sensitive);
  if (evaluated) printf("  (%.2f%%)", 100.0 * sensitive / evaluated);
  printf("\n\n");

  if (!sensitive) {
    printf("  No point in this cloud distinguishes any two bias modes, so this\n"
           "  dataset cannot be used to fit one. To model bias, the Strategy\n"
           "  needs multi-term families whose sums can cancel.\n");
  } else {
    printf("  pairwise agreement (points where two modes give identical di/dj/dk)\n\n");
    printf("  %-10s", "");
    for (m = 0; m < 4; ++m) printf(" %9s", mname[m]);
    printf("\n");
    for (m = 0; m < 4; ++m) {
      printf("  %-10s", mname[m]);
      for (m2 = 0; m2 < 4; ++m2) {
        if (m == m2) printf(" %9s", "-");
        else printf(" %9ld", agree[m][m2]);
      }
      printf("\n");
    }
    printf("\n  di distribution by mode\n\n");
    printf("  %-10s %9s %9s %9s %9s\n", "", "NegZero", "NegOne", "PosZero", "PosOne");
    for (m = 0; m < 4; ++m)
      printf("  %-10s %9ld %9ld %9ld %9ld\n", mname[m],
             dist[m][0], dist[m][1], dist[m][2], dist[m][3]);
  }
  free(pts);
  return 0;
}
