/* canor_demo_ensemble.c - ensemble - relative Left/Right bias across a population of Strategies
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* ensemble - relative Left/Right bias across a population of Strategies      */
/* ========================================================================== */
/* WHY A POPULATION AND NOT A STRATEGY. Bias is not a quantity a single
 * Strategy has. LeftBiased and RightBiased name which OPERAND of a cancelling
 * sum keeps its sign, and "left" and "right" are labels on a convention; there
 * is no experiment on one Strategy in isolation that could tell you its bias
 * is "really" left rather than that the labels are swapped. What is real, and
 * what is measurable, is that two Strategies resolve the SAME tie oppositely.
 * So bias is a relation between Strategies, and the instrument has to be a
 * population.
 *
 * WHAT THIS MEASURES, per Strategy:
 *
 *   SEPARATION  the fraction of probe items on which that Strategy's own
 *               LeftBiased and RightBiased readings differ. This is the
 *               Strategy's BIAS-CARRYING CAPACITY - how much of an L/R signal
 *               it can express at all. A Strategy with separation 0 is not
 *               unbiased; it is BLIND, and using it as an instrument would
 *               report every model as unbiased no matter what the model did.
 *               Capacity has to be reported before any reading taken with it
 *               means anything.
 *
 *   DEFINED     the fraction of items where the differential exists at all.
 *               A Strategy that refuses most items has a small effective
 *               sample however high its separation.
 *
 * and, across pairs of Strategies, AGREEMENT under a fixed bias - which is the
 * axis an external answer set gets positioned along.
 *
 * THE ENSEMBLE IS GENERATED, NOT LISTED, so that the population spans the
 * structural dimension that actually governs capacity: the number of terms in
 * the families. Everything else - operator kind, Ore parameters, seed - is
 * crossed against it so that the effect of term count can be read off rather
 * than assumed.
 */

typedef struct {
  const char *tag;
  const char *di, *dj, *dk;
  int terms;
} fam_template;

static const fam_template FAMS[] = {
  { "single",  "+sin1",                   "+cos1",             "+tan1",             3 },
  { "sq",      "+sin1*sin1",              "+cos1*cos1",        "+tan1",             3 },
  { "pair",    "+sin1 -sin2",             "+cos1 -cos2",       "+tan1 -tan2",       6 },
  { "triple",  "+sin1 +sin2 -sin3",       "+cos1*cos2 -cos3",  "+tan1 -tan2 +tan3", 9 },
  { "example", "-sin1 +tan2 +sin3 -sin4", "+cos1*sin2 -cos3*tan4", "+cos1 -sin2*tan3/sin4", 9 },
  { "wide",    "+sin1 -sin2 +sin3 -sin4", "+cos1 -cos2 +cos3 -cos4", "+tan1 -tan2 +tan3", 11 },
};
#define NFAMS ((int)(sizeof FAMS / sizeof FAMS[0]))

typedef struct {
  const char *ops;      /* "hash" or an Ore `a` */
  const char *q;
} op_template;

static const op_template OPS[] = {
  { "hash", "1" }, { "1", "1" }, { "0", "1" }, { "-1", "1" }, { "1", "2" },
};
#define NOPS ((int)(sizeof OPS / sizeof OPS[0]))

typedef struct {
  char tag[48];
  long defined, separated, total;
  int terms;
  int f, o;                 /* so a row can be rebuilt after sorting */
  int codes;
  double quality;
} ens_row;

/* Rebuilds ensemble member (f, o). Needed because the pairwise matrix must be
 * taken over the MOST CAPABLE members, which is only known after the capacity
 * pass has run and been sorted - and a matrix over blind members would show
 * near-zero agreement everywhere and mean nothing. */
static bool ens_build(int f, int o, char *name, int namecap,
                      canor_strategy *s, canor_ore *ore, int *use_ore) {
  snprintf(name, namecap, "ens_%d_%d", f, o);
  canor_strategy_default(s, name, 1);
  if (!canor_family_parse(&s->d[0][0], "di", FAMS[f].di) ||
      !canor_family_parse(&s->d[0][1], "dj", FAMS[f].dj) ||
      !canor_family_parse(&s->d[0][2], "dk", FAMS[f].dk)) return false;
  *use_ore = strcmp(OPS[o].ops, "hash") != 0;
  if (*use_ore) {
    if (!canor_ore_init(ore, OPS[o].ops, OPS[o].q, "1")) return false;
    if (!canor_ore_bind(name, ore)) return false;
    s->ops = canor_ore_absop;
  }
  return true;
}

static int ens_cmp(const void *a, const void *b) {
  const ens_row *x = (const ens_row *)a, *y = (const ens_row *)b;
  double sx = x->defined ? (double)x->separated / x->defined : 0.0;
  double sy = y->defined ? (double)y->separated / y->defined : 0.0;
  return sx < sy ? 1 : (sx > sy ? -1 : 0);
}

int canor_demo_cmd_ensemble(const canor_demo_config_t *c, const char *path, long want) {
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0, i;
  ens_row *rows;
  int f, o, nrows = 0, r1, r2;
  /* Cross-Strategy agreement, computed for the most capable Strategies only:
   * a pair of blind Strategies agrees trivially, and reporting that alongside
   * a real agreement would flatter both. */
  #define ENS_TOP 6
  static canor_strategy keep[ENS_TOP];
  static char keeptag[ENS_TOP][48];
  static canor_ore keepore[ENS_TOP];
  static int keepore_on[ENS_TOP];
  int nkeep = 0;

  if (!canor_demo_cloud_get(c, 0, path, want, &pts, &n)) return 2;
  rows = (ens_row *)calloc((size_t)(NFAMS * NOPS), sizeof *rows);
  if (!rows) { free(pts); fprintf(stderr, "follow: out of memory\n"); return 2; }

  printf("Strategy ensemble: relative Left/Right bias\n\n");
  printf("  probe items       %ld\n", n);
  printf("  ensemble size     %d  (%d family templates x %d operator settings)\n\n",
         NFAMS * NOPS, NFAMS, NOPS);

  for (f = 0; f < NFAMS; ++f)
    for (o = 0; o < NOPS; ++o) {
      canor_strategy sL, sR;
      canor_ore ore;
      int use_ore = strcmp(OPS[o].ops, "hash") != 0;
      ens_row *row = &rows[nrows];

      snprintf(row->tag, sizeof row->tag, "%s/%s%s%s", FAMS[f].tag,
               use_ore ? "ore a=" : "hash", use_ore ? OPS[o].ops : "",
               use_ore && strcmp(OPS[o].q, "1") ? " q=2" : "");
      row->terms = FAMS[f].terms;
      row->f = f;
      row->o = o;

      /* Distinct names so canor_rand gives each ensemble member its own stream -
       * two members differing only in family must not share a seed, or the
       * comparison would confound structure with sampling. */
      {
        static char names[NFAMS * NOPS][48];
        snprintf(names[nrows], sizeof names[0], "ens_%d_%d", f, o);
        canor_strategy_default(&sL, names[nrows], 1);
      }
      if (!canor_family_parse(&sL.d[0][0], "di", FAMS[f].di) ||
          !canor_family_parse(&sL.d[0][1], "dj", FAMS[f].dj) ||
          !canor_family_parse(&sL.d[0][2], "dk", FAMS[f].dk)) continue;
      if (use_ore) {
        if (!canor_ore_init(&ore, OPS[o].ops, OPS[o].q, "1")) continue;
        if (!canor_ore_bind(sL.id.name, &ore)) {
          fprintf(stderr, "follow: ore binding table full at '%s' - the "
                          "ensemble would measure table capacity, not bias "
                          "capacity\n", sL.id.name);
          free(rows); free(pts);
          return 2;
        }
        sL.ops = canor_ore_absop;
      }
      sR = sL;
      sL.bias = LeftBiased;
      sR.bias = RightBiased;

      for (i = 0; i < n; ++i) {
        char word[CANOR_FLOW_WORD_MAX];
        int cnt[3], len;
        int32_t args[4];
        FLOWVALUE l[3], rr[3];
        len = canor_demo_cloud_word(c, &pts[i], word, CANOR_FLOW_WORD_MAX, cnt);
        if (len == 0) continue;
        ++row->total;
        args[0] = pts[i].x; args[1] = pts[i].y; args[2] = pts[i].z; args[3] = pts[i].a;
        if (!canor_di(&sL, args, 4, &l[0]) || !canor_dj(&sL, args, 4, &l[1]) ||
            !canor_dk(&sL, args, 4, &l[2])) continue;
        if (!canor_di(&sR, args, 4, &rr[0]) || !canor_dj(&sR, args, 4, &rr[1]) ||
            !canor_dk(&sR, args, 4, &rr[2])) continue;
        ++row->defined;
        if (l[0] != rr[0] || l[1] != rr[1] || l[2] != rr[2]) ++row->separated;
      }
      {
        canor_demo_qual_t qq;
        /* Ensemble members bind one Ore setting across all three operators, so
         * the triple has a single period by construction. */
        int per = use_ore ? canor_ore_period(&ore) : 0;
        canor_demo_qual_measure(c, &sL, pts, n, 1, per > 0 ? per : 1, &qq);
        row->codes = qq.codes_distinct;
        row->quality = qq.quality;
      }
      ++nrows;
    }

  qsort(rows, (size_t)nrows, sizeof *rows, ens_cmp);

  /* Now that capacity is known, rebuild the most capable members for the
   * pairwise matrix. Members with zero capacity are excluded outright: they
   * emit a constant, so their "agreement" with anything is an artifact of that
   * constant and not a comparison. */
  for (i = 0; i < nrows && nkeep < ENS_TOP; ++i) {
    static char knames[ENS_TOP][48];
    if (rows[i].defined == 0 || rows[i].separated == 0) continue;
    if (!ens_build(rows[i].f, rows[i].o, knames[nkeep], sizeof knames[0],
                   &keep[nkeep], &keepore[nkeep], &keepore_on[nkeep])) continue;
    snprintf(keeptag[nkeep], sizeof keeptag[0], "%s", rows[i].tag);
    ++nkeep;
  }

  printf("  instrument qualification, most capable first\n");
  printf("  the 3-tuple is (Quality, Capacity, (Quality x Capacity) x 0.5)\n\n");
  printf("  %-22s %5s %6s %9s %9s %9s\n", "strategy", "terms", "codes",
         "Quality", "Capacity", "QxC/2");
  for (i = 0; i < nrows; ++i) {
    ens_row *row = &rows[i];
    double cap = row->defined ? 100.0 * row->separated / row->defined : 0.0;
    /* Half the PRODUCT, not a mean. A Strategy scoring zero on either term
     * collapses the composite, which is right for an instrument rating: an
     * elaborate Strategy that cannot express bias is not half-useful, it is
     * useless, and an average would report it as middling. */
    printf("  %-22s %5d %6d %8.1f%% %8.1f%% %9.1f\n", row->tag, row->terms,
           row->codes, row->quality, cap, row->quality * cap * 0.5 / 100.0);
  }

  printf("\n  A capacity of 0 does not mean unbiased. It means BLIND: that\n"
         "  Strategy cannot express an L/R distinction, so any model measured\n"
         "  through it reads as unbiased whatever the model does. Capacity is\n"
         "  a property of the instrument and has to be reported with any\n"
         "  reading taken using it.\n");

  /* Cross-Strategy agreement under a fixed bias: the axis along which an
   * external answer set is positioned. */
  printf("\n  cross-Strategy agreement on di/dj/dk, LeftBiased throughout\n");
  printf("  (how much two ensemble members say the same thing at all)\n\n");
  printf("  %-24s", "");
  for (r2 = 0; r2 < nkeep; ++r2) printf(" %7d", r2);
  printf("\n");
  for (r1 = 0; r1 < nkeep; ++r1) {
    printf("  %-2d %-21s", r1, keeptag[r1]);
    for (r2 = 0; r2 < nkeep; ++r2) {
      long same = 0, both = 0;
      if (r1 == r2) { printf(" %7s", "-"); continue; }
      for (i = 0; i < n; ++i) {
        char word[CANOR_FLOW_WORD_MAX];
        int cnt[3], len;
        int32_t args[4];
        FLOWVALUE a[3], b[3];
        canor_strategy sa = keep[r1], sb = keep[r2];
        len = canor_demo_cloud_word(c, &pts[i], word, CANOR_FLOW_WORD_MAX, cnt);
        if (len == 0) continue;
        args[0] = pts[i].x; args[1] = pts[i].y; args[2] = pts[i].z; args[3] = pts[i].a;
        sa.bias = LeftBiased; sb.bias = LeftBiased;
        if (keepore_on[r1]) canor_ore_bind(sa.id.name, &keepore[r1]);
        if (keepore_on[r2]) canor_ore_bind(sb.id.name, &keepore[r2]);
        if (!canor_di(&sa, args, 4, &a[0]) || !canor_dj(&sa, args, 4, &a[1]) ||
            !canor_dk(&sa, args, 4, &a[2])) continue;
        if (!canor_di(&sb, args, 4, &b[0]) || !canor_dj(&sb, args, 4, &b[1]) ||
            !canor_dk(&sb, args, 4, &b[2])) continue;
        ++both;
        if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]) ++same;
      }
      printf(" %6.1f%%", both ? 100.0 * same / both : 0.0);
    }
    printf("\n");
  }

  free(rows);
  free(pts);
  return 0;
}
