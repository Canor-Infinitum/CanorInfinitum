/* canor_demo_spec.c - spec - a PggSpec widget tree, the shared contract with the GUI
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* spec - a PggSpec widget tree, the shared contract with the GUI             */
/* ========================================================================== */
/* PGG already has exactly one contract between a generator and a renderer:
 * the PggSpec widget tree, parsed by PggSpec.ParseSpec and realised by
 * PggCompose in Lazarus or by the web renderer from byte-identical JSON. So
 * the right way for follow-core to reach a GUI is to EMIT that tree, not to
 * grow a second UI description alongside it.
 *
 * That decides several things at once. The layout vocabulary is the flexbox
 * subset PggSpec permits and nothing else, because anything outside it makes
 * the web renderer a reimplementation rather than a translation. The values
 * are the MEASURED ones - strategies that exist, capacities that were
 * computed - so the emitted panel is a view of this run, not a mockup that
 * would drift from it.
 *
 * The tab is "Bias-modelling Strategy-Flow relations". Its controls are the
 * parameters that actually change a reading: which Strategy, which grouping,
 * how many probe points, and the admissibility floor. */

static void spec_pad(FILE *f, int v, int h) {
  fprintf(f, "\"padding\":{\"top\":%d,\"right\":%d,\"bottom\":%d,\"left\":%d},", v, h, v, h);
}

static void spec_label(FILE *f, const char *id, const char *cap, int first) {
  fprintf(f, "%s{\"kind\":\"label\",\"id\":\"%s\",\"caption\":\"%s\"}",
          first ? "" : ",", id, cap);
}

int canor_demo_cmd_spec(const canor_demo_config_t *c, int sel, const char *emit, long want) {
  FILE *f;
  char path[600];
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0;
  int i;

  snprintf(path, sizeof path, "%s", emit ? emit : "follow_bias_tab.json");
  f = fopen(path, "w");
  if (!f) { fprintf(stderr, "follow: cannot write %s\n", path); return 2; }

  if (!canor_demo_cloud_get(c, sel, NULL, want, &pts, &n)) { fclose(f); return 2; }

  fprintf(f, "{\n \"root\":{\n");
  fprintf(f, "  \"kind\":\"group\",\"id\":\"bias_tab\",\"direction\":\"column\","
             "\"gap\":12,");
  spec_pad(f, 16, 16);
  fprintf(f, "\"align\":\"stretch\",\"children\":[\n");

  /* --- selection card --- */
  fprintf(f, "   {\"kind\":\"card\",\"id\":\"sel\",\"caption\":\"Instrument\","
             "\"direction\":\"column\",\"gap\":8,");
  spec_pad(f, 12, 12);
  fprintf(f, "\"children\":[\n");
  fprintf(f, "    {\"kind\":\"choice\",\"id\":\"strategy\",\"caption\":\"Strategy\","
             "\"value\":%d,\"hint\":\"Which Strategy the reading is taken through\","
             "\"options\":[", sel);
  for (i = 0; i < c->nstrat; ++i)
    fprintf(f, "%s\"%s\"", i ? "," : "", c->strat[i].id.name);
  fprintf(f, "]},\n");
  fprintf(f, "    {\"kind\":\"choice\",\"id\":\"grouping\",\"caption\":\"Grouping\","
             "\"value\":3,\"hint\":\"spread is disjoint AND closing; the 56 spreads"
             " form one A8 orbit, so which one is immaterial\","
             "\"options\":[\"index\",\"quaternion\",\"fano\",\"spread\"]},\n");
  fprintf(f, "    {\"kind\":\"slider\",\"id\":\"points\",\"caption\":\"Probe points\","
             "\"min\":1000,\"max\":50000,\"step\":1000,\"value\":%ld},\n", n);
  fprintf(f, "    {\"kind\":\"slider\",\"id\":\"amin\",\"caption\":\"Admissibility floor\","
             "\"min\":0,\"max\":255,\"step\":1,\"value\":32,"
             "\"hint\":\"Below this a point is withheld, and reported as withheld"
             " rather than dropped\"}\n");
  fprintf(f, "   ]},\n");

  /* --- qualification card: the 3-tuple, measured --- */
  {
    canor_strategy sL = c->strat[sel], sR = c->strat[sel];
    canor_demo_qual_t q;
    long defined = 0, separated = 0, i2;
    double cap;
    char buf[192];

    canor_demo_qual_t pq;
    memset(&pq, 0, sizeof pq);
    canor_demo_qual_measure_ore(c, sel, &pq);
    canor_demo_qual_measure(c, &sL, pts, n, pq.periods_distinct, pq.period_lcm, &q);
    sL.bias = LeftBiased;
    sR.bias = RightBiased;
    for (i2 = 0; i2 < n; ++i2) {
      char word[CANOR_FLOW_WORD_MAX];
      int cnt[3], len;
      int32_t args[4];
      FLOWVALUE l[3], r[3];
      len = canor_demo_cloud_word(c, &pts[i2], word, CANOR_FLOW_WORD_MAX, cnt);
      if (len == 0) continue;
      args[0] = pts[i2].x; args[1] = pts[i2].y; args[2] = pts[i2].z; args[3] = pts[i2].a;
      if (!canor_di(&sL, args, 4, &l[0]) || !canor_dj(&sL, args, 4, &l[1]) ||
          !canor_dk(&sL, args, 4, &l[2])) continue;
      if (!canor_di(&sR, args, 4, &r[0]) || !canor_dj(&sR, args, 4, &r[1]) ||
          !canor_dk(&sR, args, 4, &r[2])) continue;
      ++defined;
      if (l[0] != r[0] || l[1] != r[1] || l[2] != r[2]) ++separated;
    }
    cap = defined ? 100.0 * (double)separated / (double)defined : 0.0;

    fprintf(f, "   {\"kind\":\"card\",\"id\":\"qual\",\"caption\":"
               "\"Instrument qualification\",\"direction\":\"column\",\"gap\":6,");
    spec_pad(f, 12, 12);
    fprintf(f, "\"children\":[\n    ");
    snprintf(buf, sizeof buf, "Quality %.1f%%   Capacity %.1f%%   QxC/2 %.1f",
             q.quality, cap, q.quality * cap * 0.5 / 100.0);
    spec_label(f, "tuple", buf, 1);
    snprintf(buf, sizeof buf, "terms %d   factors %d   codes %d of 64   "
             "distinct periods %d   period lcm %d",
             q.terms, q.factors, q.codes_distinct, q.periods_distinct, q.period_lcm);
    fprintf(f, ",\n    ");
    spec_label(f, "detail", buf, 1);
    fprintf(f, ",\n    {\"kind\":\"divider\",\"id\":\"d1\"}");
    if (cap <= 0.0) {
      fprintf(f, ",\n    ");
      spec_label(f, "blind", "BLIND: this Strategy cannot express an L/R "
                 "distinction, so any model measured through it reads as "
                 "unbiased whatever it does", 1);
    }
    fprintf(f, "\n   ]},\n");
  }

  /* --- operators card: one row per operator --- */
  fprintf(f, "   {\"kind\":\"card\",\"id\":\"ops\",\"caption\":"
             "\"Ore operators (per-operator parameters)\",\"direction\":\"column\","
             "\"gap\":6,");
  spec_pad(f, 12, 12);
  fprintf(f, "\"children\":[\n");
  {
    static const char *const opn[3] = { "sin", "cos", "tan" };
    for (i = 0; i < 3; ++i) {
      char ab[32], qb[32], sb[32], buf[192];
      const canor_ore *o = c->strat_use_ore[sel] ? &c->strat_ore[sel][i] : NULL;
      int per = o ? canor_ore_period(o) : 0;
      if (o) {
        canor_demo_rat_show(o->tau, ab, sizeof ab);
        canor_demo_rat_show(o->sigma, qb, sizeof qb);
        canor_demo_rat_show(o->delta_amp, sb, sizeof sb);
      } else { snprintf(ab, 32, "-"); snprintf(qb, 32, "-"); snprintf(sb, 32, "-"); }
      snprintf(buf, sizeof buf, "%s_S   tau=%s  sigma=%s  delta=%s   %s%d",
               opn[i], ab, qb, sb,
               o && canor_ore_degenerate(o) ? "degenerate " :
               per ? "period " : "aperiodic",
               per);
      fprintf(f, "    ");
      spec_label(f, opn[i], buf, 1);
      fprintf(f, "%s\n", i < 2 ? "," : "");
    }
  }
  fprintf(f, "   ]},\n");

  /* --- actions --- */
  fprintf(f, "   {\"kind\":\"group\",\"id\":\"actions\",\"direction\":\"row\","
             "\"gap\":8,\"justify\":\"end\",\"children\":[\n");
  fprintf(f, "    {\"kind\":\"button\",\"id\":\"run_ensemble\","
             "\"caption\":\"Qualify ensemble\"},\n");
  fprintf(f, "    {\"kind\":\"button\",\"id\":\"extract\","
             "\"caption\":\"Request extracted Excitations from Model\"},\n");
  fprintf(f, "    {\"kind\":\"button\",\"id\":\"graph\","
             "\"caption\":\"Open operator graphs\"}\n");
  fprintf(f, "   ]}\n");

  fprintf(f, "  ]\n }\n}\n");
  fclose(f);
  free(pts);

  printf("wrote %s\n", path);
  printf("  PggSpec widget tree: flexbox subset only, so the web renderer is a\n"
         "  translation of the Lazarus one rather than a second implementation.\n");
  canor_demo_prov_write_sidecar(path);
  return 0;
}
