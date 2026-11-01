/* canor_demo_survey.c - survey - RGBA-WXYZ governance and provenance
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* survey - RGBA-WXYZ governance and provenance                               */
/* ========================================================================== */
/* A survey point is p = (R, G, B, A, W, X, Y, Z), and the channels are not
 * colour:
 *
 *   R  observed / evidentially supported reality
 *   G  informational gap, typed uncertainty
 *   B  foundational basis, assumptions, prior commitments
 *   A  latent continuation, visibility, admissibility, disclosure
 *   W  stratum / layer, carried with the coordinates
 *   XYZ  spatial position under a stated CRS
 *
 * This changes what the axes MEAN, and therefore what a result is evidence of.
 * MetaFlow is no longer "which colour dominates" but which of evidence, gap or
 * assumption dominates a finding - and the refusal slot is the case where none
 * does, which for governance is the most interesting cell rather than a
 * leftover: it is a claim resting equally on evidence, uncertainty and
 * assumption, i.e. one that cannot be defended on any single ground.
 *
 * A IS NOT A WEIGHT. Admissibility gates whether a point may be counted at
 * all. A point with A = 0 is withheld, and it is REPORTED as withheld rather
 * than dropped - a survey that silently discarded its undisclosed rows would
 * report the disclosed subset as if it were the population, which is the
 * specific failure a provenance survey exists to prevent.
 *
 * The CRS is carried, never interpreted. This code does no reprojection: it
 * bins coordinates as given and prints the CRS string it was told, so a reader
 * can see which frame the bins are in rather than assume one.
 */

typedef struct {
  int32_t r, g, b, a;
  int32_t w, x, y, z;
  int survey;
} survey_point;

#define SURVEY_MAX_ID 16
#define HEAT_W 32
#define HEAT_H 14

static bool survey_load(const char *path, survey_point **out, long *count,
                        char *crs, int crscap) {
  FILE *f = fopen(path, "r");
  survey_point *pts = NULL;
  long n = 0, cap = 0;
  if (!f) return false;
  snprintf(crs, crscap, "unstated");
  for (;;) {
    char line[512];
    survey_point p;
    const char *q;
    int got;
    if (!fgets(line, sizeof line, f)) break;
    for (q = line; *q == ' ' || *q == '\t'; ++q) { }
    if (*q == '#') {
      const char *c = strstr(q, "CRS:");
      if (c) {
        int k = 0;
        c += 4;
        while (*c == ' ') ++c;
        while (*c && *c != '\n' && *c != '\r' && k + 1 < crscap) crs[k++] = *c++;
        crs[k] = 0;
      }
      continue;
    }
    if (*q == '\n' || *q == '\r' || *q == 0) continue;
    p.survey = 0;
    got = sscanf(q, "%d %d %d %d %d %d %d %d %d", &p.r, &p.g, &p.b, &p.a,
                 &p.w, &p.x, &p.y, &p.z, &p.survey);
    if (got < 8) continue;
    if (p.survey < 0 || p.survey >= SURVEY_MAX_ID) p.survey = 0;
    if (n == cap) {
      survey_point *np;
      cap = cap ? cap * 2 : 1024;
      np = (survey_point *)realloc(pts, (size_t)cap * sizeof *np);
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

static void survey_synth(const canor_strategy *s, uint64_t i, int nsurvey,
                         survey_point *p) {
  uint64_t u = canor_rand(&s->id, 0x5C09Eull, i);
  uint64_t v = canor_rand(&s->id, 0x5EEDull, i);
  uint64_t t = canor_rand(&s->id, 0xA11Dull, i);
  p->survey = (int)(t % (uint64_t)(nsurvey > 0 ? nsurvey : 1));
  /* Each survey has its own evidentiary character, so the sort has something
   * to find. Survey 0 is evidence-led, 1 gap-led, 2 assumption-led, and the
   * rest are balanced - which is the contested case. */
  p->r = (int)(u & 0x3f);
  p->g = (int)((u >> 6) & 0x3f);
  p->b = (int)((u >> 12) & 0x3f);
  switch (p->survey % 4) {
    case 0: p->r += 48; break;
    case 1: p->g += 48; break;
    case 2: p->b += 48; break;
    default: break;
  }
  p->a = (int)((u >> 18) & 0xff);
  p->w = (int)((v >> 40) & 0x3);
  p->x = (int)(v & 0xffff) - 32768;
  p->y = (int)((v >> 16) & 0xffff) - 32768;
  p->z = (int)((v >> 32) & 0xff);
}

int canor_demo_cmd_survey(const canor_demo_config_t *c, int sel, const char *path, long want,
                      int nsurvey, int amin) {
  survey_point *pts = NULL;
  long n = 0, i;
  char crs[64];
  long joint[CANOR_DEMO_AXIS_N][8];
  long per[SURVEY_MAX_ID][CANOR_DEMO_AXIS_N];
  long perflow[SURVEY_MAX_ID][8];
  long seen[SURVEY_MAX_ID];
  long withheld = 0, admitted = 0, nofold = 0;
  long wband[4][CANOR_DEMO_AXIS_N];
  static long heat[HEAT_H][HEAT_W][CANOR_DEMO_AXIS_N];
  static long heata[HEAT_H][HEAT_W], heatn[HEAT_H][HEAT_W];
  int mi, fi, k, sv, hx, hy;

  snprintf(crs, sizeof crs, "unstated");
  if (path) {
    if (!survey_load(path, &pts, &n, crs, sizeof crs)) {
      fprintf(stderr, "follow: %s: no readable RGBA-WXYZ rows "
                      "(8 or 9 integers per line)\n", path);
      return 2;
    }
  } else {
    pts = (survey_point *)malloc((size_t)want * sizeof *pts);
    if (!pts) { fprintf(stderr, "follow: out of memory\n"); return 2; }
    for (i = 0; i < want; ++i) survey_synth(&c->strat[sel], (uint64_t)i, nsurvey, &pts[i]);
    n = want;
    snprintf(crs, sizeof crs, "synthetic (no CRS)");
  }

  memset(joint, 0, sizeof joint);
  memset(per, 0, sizeof per);
  memset(perflow, 0, sizeof perflow);
  memset(seen, 0, sizeof seen);
  memset(wband, 0, sizeof wband);
  memset(heat, 0, sizeof heat);
  memset(heata, 0, sizeof heata);
  memset(heatn, 0, sizeof heatn);

  for (i = 0; i < n; ++i) {
    const survey_point *p = &pts[i];
    int cnt[3];
    char word[CANOR_FLOW_WORD_MAX];
    canor_demo_cloud_point_t cp;
    canor_demo_ext_flow_t ef;
    canor_demo_ext_fold_t fr;
    int len;

    /* Admissibility first. A point below the disclosure threshold is counted
     * as withheld and takes no further part - it must not contribute to a
     * distribution that will be read as the population. */
    if (p->a < amin) { ++withheld; continue; }
    ++admitted;

    cp.r = p->r; cp.g = p->g; cp.b = p->b;
    cp.a = 255;                 /* length comes from evidence volume, not from A */
    cp.x = p->x; cp.y = p->y; cp.z = p->z;
    len = canor_demo_cloud_word(c, &cp, word, CANOR_FLOW_WORD_MAX, cnt);
    if (len == 0) { ++nofold; continue; }
    if (!canor_demo_ext_parse(&ef, "s", word) || !canor_demo_ext_fold_word(&ef, &fr)) { ++nofold; continue; }

    mi = canor_demo_meta_of(c, cnt);
    fi = (fr.unit < (uint8_t)c->flows.count) ? (int)fr.unit : c->flows.count;
    sv = p->survey;
    joint[mi][fi]++;
    per[sv][mi]++;
    perflow[sv][fi]++;
    seen[sv]++;
    wband[p->w & 3][mi]++;

    hx = (int)(((int64_t)p->x + 32768) * HEAT_W / 65536);
    hy = (int)(((int64_t)p->y + 32768) * HEAT_H / 65536);
    if (hx < 0) hx = 0;
    if (hx >= HEAT_W) hx = HEAT_W - 1;
    if (hy < 0) hy = 0;
    if (hy >= HEAT_H) hy = HEAT_H - 1;
    heat[hy][hx][mi]++;
    heata[hy][hx] += p->a;
    heatn[hy][hx]++;
  }

  printf("Governance & Provenance survey\n");
  printf("  source        %s\n", path ? path : "synthesised");
  printf("  CRS           %s\n", crs);
  printf("  rows          %ld\n", n);
  printf("  admitted      %ld   (A >= %d)\n", admitted, amin);
  printf("  withheld      %ld   (A < %d: undisclosed, NOT dropped)\n", withheld, amin);
  if (nofold) printf("  unfoldable    %ld\n", nofold);
  printf("\n  channels: R evidence  G gap  B basis  A admissibility\n");

  printf("\n=== sort 1: MetaFlow x Flow over admitted points ===\n\n");
  printf("  %-14s", "");
  for (fi = 0; fi < c->flows.count; ++fi) printf(" %10s", c->flows.flow[fi].name);
  printf(" %10s\n", "total");
  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) {
    long row = 0;
    printf("  %-14s", c->meta[mi].name);
    for (fi = 0; fi < c->flows.count; ++fi) { printf(" %10ld", joint[mi][fi]); row += joint[mi][fi]; }
    printf(" %10ld\n", row);
  }

  printf("\n=== sort 2: per-survey MetaFlow profile ===\n\n");
  printf("  %-8s %8s", "survey", "points");
  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) printf(" %12s", c->meta[mi].name);
  printf("\n");
  for (sv = 0; sv < SURVEY_MAX_ID; ++sv) {
    if (!seen[sv]) continue;
    printf("  %-8d %8ld", sv, seen[sv]);
    for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi)
      printf(" %11.1f%%", 100.0 * per[sv][mi] / seen[sv]);
    printf("\n");
  }

  printf("\n=== sort 3: per-survey Flow profile ===\n\n");
  printf("  %-8s %8s", "survey", "points");
  for (fi = 0; fi < c->flows.count; ++fi) printf(" %12s", c->flows.flow[fi].name);
  printf("\n");
  for (sv = 0; sv < SURVEY_MAX_ID; ++sv) {
    if (!seen[sv]) continue;
    printf("  %-8d %8ld", sv, seen[sv]);
    for (fi = 0; fi < c->flows.count; ++fi)
      printf(" %11.1f%%", 100.0 * perflow[sv][fi] / seen[sv]);
    printf("\n");
  }

  printf("\n=== sort 4: W stratum x MetaFlow ===\n\n");
  printf("  %-8s", "W");
  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) printf(" %12s", c->meta[mi].name);
  printf("\n");
  for (k = 0; k < 4; ++k) {
    long row = 0;
    for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) row += wband[k][mi];
    if (!row) continue;
    printf("  %-8d", k);
    for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) printf(" %12ld", wband[k][mi]);
    printf("\n");
  }

  printf("\n=== heatmap: dominant MetaFlow over XY ===\n");
  printf("  %c=%s  %c=%s  %c=%s  %c=%s   . = no data\n",
         c->meta[0].name[0], c->meta[0].name, c->meta[1].name[0], c->meta[1].name,
         c->meta[2].name[0], c->meta[2].name, c->meta[3].name[0], c->meta[3].name);
  for (hy = 0; hy < HEAT_H; ++hy) {
    printf("  ");
    for (hx = 0; hx < HEAT_W; ++hx) {
      int best = -1;
      long bv = 0;
      for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi)
        if (heat[hy][hx][mi] > bv) { bv = heat[hy][hx][mi]; best = mi; }
      putchar(best < 0 ? '.' : c->meta[best].name[0]);
    }
    putchar('\n');
  }

  printf("\n=== heatmap: mean admissibility A over XY ===\n");
  printf("  ' '<32  .<64  :<96  -<128  =<160  +<192  *<224  #>=224\n");
  for (hy = 0; hy < HEAT_H; ++hy) {
    printf("  ");
    for (hx = 0; hx < HEAT_W; ++hx) {
      static const char ramp[8] = { ' ', '.', ':', '-', '=', '+', '*', '#' };
      long m;
      if (!heatn[hy][hx]) { putchar('.'); continue; }
      m = heata[hy][hx] / heatn[hy][hx];
      k = (int)(m / 32);
      if (k < 0) k = 0;
      if (k > 7) k = 7;
      putchar(ramp[k]);
    }
    putchar('\n');
  }

  free(pts);
  return 0;
}
