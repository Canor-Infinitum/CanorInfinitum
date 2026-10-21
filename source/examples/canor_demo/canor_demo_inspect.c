/* canor_demo_inspect.c - commands
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* commands                                                                   */
/* ========================================================================== */

int canor_demo_strat_find(const canor_demo_config_t *c, const char *name) {
  for (int i = 0; i < c->nstrat; ++i)
    if (!strcmp(c->strat[i].id.name, name)) return i;
  return -1;
}

int canor_demo_cmd_config(const canor_demo_config_t *c) {
  char b[256];
  printf("config      %s\n", c->path);
  printf("strategies  %d\n", c->nstrat);
  printf("flows       %d\n", c->flows.count);
  printf("settings    fuel=%d levels=%d grid=%dx%d hv=%dx%d\n",
         c->set.fuel, c->set.levels, c->set.grid_w, c->set.grid_h,
         c->set.hv_tables, c->set.hv_bits);
  fb_rat_format(&c->frame_cr, b, sizeof b); printf("frame       centre %s", b);
  fb_rat_format(&c->frame_ci, b, sizeof b); printf(" %s i\n", b);
  fb_rat_format(&c->frame_sr, b, sizeof b); printf("            span   %s", b);
  fb_rat_format(&c->frame_si, b, sizeof b); printf(" x %s\n", b);
  printf("budget      %lu limbs (%lu KB)\n",
         (unsigned long)c->budget, (unsigned long)(c->budget * 4 / 1024));
  printf("default     %s\n",
         c->default_strategy[0] ? c->default_strategy : "(first listed)");
  printf("points      %ld\n", c->points);
  return 0;
}

/* What to show for differential n of Strategy i: the declared text, or the
 * default family formatted from the Strategy itself and MARKED as a default.
 * Printing nothing would suggest there is no family; printing the default
 * unmarked would suggest it had been asked for. Neither is true. */
const char *canor_demo_cfg_dshow(const canor_demo_config_t *c, int i, int n, char *buf, size_t cap) {
  char fam[128];
  if (c->strat_d[i][n][0]) return c->strat_d[i][n];
  canor_family_format(&c->strat[i].d[n / 3][n % 3], fam, sizeof fam);
  snprintf(buf, cap, "%s   (default, not declared)", fam);
  return buf;
}

int canor_demo_cmd_strategies(const canor_demo_config_t *c, int sel) {
  printf("%-14s %4s %-8s  %s\n", "strategy", "ver", "bias", "differential families");
  for (int i = 0; i < c->nstrat; ++i) {
    char dbuf[192];
    int n, live = c->strat[i].ntrifecta * 3;
    printf("%c%-13s %4u %-8s  trifectas %d of %d%s\n", i == sel ? '*' : ' ',
           c->strat[i].id.name, (unsigned)c->strat[i].id.version,
           canor_demo_bias_name(c->strat[i].bias),
           c->strat[i].ntrifecta, CANOR_TRIFECTAS,
           c->strat[i].ntrifecta == CANOR_TRIFECTAS ? "  (fully-fledged)" : "");
    /* Only the live trifectas are listed. An undeclared trifecta has no family
     * that canor_dtc will evaluate, so printing one would name something the
     * Strategy cannot use. */
    for (n = 0; n < live && n < CANOR_DEMO_CFG_DNAMES; ++n)
      printf("%-14s %4s %-8s  %-3s %s\n", "", "", "", canor_demo_cfg_dname[n],
             canor_demo_cfg_dshow(c, i, n, dbuf, sizeof dbuf));
    if (c->strat_use_ore[i]) {
      static const char *const opn[3] = { "sin", "cos", "tan" };
      int k;
      for (k = 0; k < 3; ++k) {
        char ab[32], qb[32], sb[32];
        const canor_ore *o = &c->strat_ore[i][k];
        int per = canor_ore_period(o);
        canor_demo_rat_show(o->tau, ab, sizeof ab);
        canor_demo_rat_show(o->sigma, qb, sizeof qb);
        canor_demo_rat_show(o->delta_amp, sb, sizeof sb);
        printf("%-14s %4s %-8s  %s_S tau=%-4s sigma=%-3s delta=%-3s %s", "", "", "",
               opn[k], ab, qb, sb,
               canor_ore_degenerate(o) ? "degenerate" : per ? "period " : "aperiodic");
        if (per && !canor_ore_degenerate(o)) printf("%d", per);
        printf("\n");
      }
    }
  }
  printf("\n  * = selected (--strategy NAME)\n");
  return 0;
}

int canor_demo_cmd_flows(const canor_demo_config_t *c) {
  printf("%-12s %-12s %-10s\n", "flow", "word", "folds to");
  for (int i = 0; i < c->flows.count; ++i) {
    uint8_t u;
    int sg;
    if (canor_flow_fold(&c->flows, i, &u, &sg))
      printf("%-12s %-12s %s%s\n", c->flows.flow[i].name, c->flows.flow[i].word,
             sg < 0 ? "-" : "+", canor_unit_name(u));
    else
      printf("%-12s %-12s %s\n", c->flows.flow[i].name, c->flows.flow[i].word,
             "(does not close)");
  }
  return 0;
}

/* Declared with the extended-base section, which sits later in the file. */

int canor_demo_cmd_diff(const canor_demo_config_t *c, int sel, const char *only) {
  const canor_strategy *s = &c->strat[sel];
  printf("strategy %s v%u, bias %s\n", s->id.name, (unsigned)s->id.version,
         canor_demo_bias_name(s->bias));
  {
    char dbuf[192];
    int n, live = s->ntrifecta * 3;
    for (n = 0; n < live && n < CANOR_DEMO_CFG_DNAMES; ++n)
      printf("  %-3s %s\n", canor_demo_cfg_dname[n], canor_demo_cfg_dshow(c, sel, n, dbuf, sizeof dbuf));
    printf("\n");
  }
  printf("%-12s %-12s %-10s %-10s %-10s\n", "flow", "word", "di", "dj", "dk");
  for (int i = 0; i < c->flows.count; ++i) {
    FLOWVALUE di, dj, dk;
    if (only && strcmp(c->flows.flow[i].name, only)) continue;
    if (!canor_flow_differential(s, &c->flows, i, &di, &dj, &dk)) {
      printf("%-12s %-12s %s\n", c->flows.flow[i].name, c->flows.flow[i].word,
             "undefined (division by a zero-valued operand)");
      continue;
    }
    printf("%-12s %-12s %-10s %-10s %-10s\n",
           c->flows.flow[i].name, c->flows.flow[i].word,
           canor_fv_name(di), canor_fv_name(dj), canor_fv_name(dk));
  }
  return 0;
}

int canor_demo_cmd_kapprox(const canor_demo_config_t *c) {
  static const uint64_t ns[] = { 1, 2, 3, 4, 6, 12, 20, 48, 96 };
  printf("Kapprox ladder, log2 in milli-bits, 2n! read as 2*(n!)\n\n");
  printf("   n  %14s %14s %14s %14s  separated\n",
         "General", "Specific", "Distinctive", "Arbitrary*");
  for (size_t i = 0; i < sizeof ns / sizeof ns[0]; ++i) {
    uint64_t n = ns[i];
    printf("  %3llu  %14lld %14lld %14lld %14lld  %s\n",
           (unsigned long long)n,
           (long long)canor_kapprox_order_milli(CANOR_K_GENERAL, n),
           (long long)canor_kapprox_order_milli(CANOR_K_SPECIFIC, n),
           (long long)canor_kapprox_order_milli(CANOR_K_DISTINCTIVE, n),
           (long long)canor_kapprox_order_milli(CANOR_K_ARBITRARY, n),
           canor_kapprox_separated(n) ? "yes" : "NO");
  }
  printf("\n  * reported as an ORDER - log2 of the log2 - because log2((2(n!)+1)!)\n"
         "    is about 1e152 at n=96 and is not a quantity a caller can compute\n"
         "    with. The class stays comparable; precision it does not have is\n"
         "    not claimed.\n");
  printf("\n  configured level %d: %s\n", c->set.levels,
         canor_kapprox_separated((uint64_t)c->set.levels) ? "separated" : "COLLAPSED");
  return 0;
}

/* --- rgb: project an RGB string onto the three axes ------------------------ */

/* MetaFlow of a channel-count vector: the configured entry whose channel
 * strictly dominates, or the refusal slot when none does. */
int canor_demo_meta_of(const canor_demo_config_t *c, const int cnt[3]) {
  int dom = 0, i;
  for (i = 1; i < 3; ++i) if (cnt[i] > cnt[dom]) dom = i;
  for (i = 0; i < 3; ++i)
    if (i != dom && cnt[i] == cnt[dom]) return CANOR_DEMO_AXIS_N - 1;   /* nothing dominates */
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i) if (c->meta[i].channel == dom) return i;
  return CANOR_DEMO_AXIS_N - 1;
}

/* Mixture: the first configured entry whose declared shape the string meets.
 * The refusal slot has dims == -1 and therefore matches nothing, so it is
 * reached only by falling off the end - which is what a refusal should be. */
int canor_demo_mix_of(const canor_demo_config_t *c, const int cnt[3]) {
  int dims = 0, dom = 0, ties = 0, i;
  for (i = 0; i < 3; ++i) {
    if (cnt[i]) ++dims;
    if (cnt[i] > cnt[dom]) dom = i;
  }
  for (i = 0; i < 3; ++i) if (i != dom && cnt[i] == cnt[dom]) ++ties;
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i) {
    if (c->mix[i].dims < 0) continue;
    if (c->mix[i].dims == 1 ? dims != 1 : dims < c->mix[i].dims) continue;
    if (c->mix[i].dominant && ties) continue;
    return i;
  }
  return CANOR_DEMO_AXIS_N - 1;
}

int canor_demo_cmd_rgb(const canor_demo_config_t *c, const char *str) {
  canor_flow_registry r;
  char word[CANOR_FLOW_WORD_MAX];
  int n = 0, cnt[3] = { 0, 0, 0 };
  uint8_t u;
  int sg, mi, xi;

  for (const char *p = str; *p; ++p) {
    char ch = *p;
    int chan = (ch == 'R' || ch == 'r') ? 0 : (ch == 'G' || ch == 'g') ? 1
             : (ch == 'B' || ch == 'b') ? 2 : -1;
    if (chan < 0) {
      fprintf(stderr, "follow: '%c' is not an RGB channel\n", ch);
      return 2;
    }
    if (n + 1 >= CANOR_FLOW_WORD_MAX) {
      fprintf(stderr, "follow: string longer than %d\n", CANOR_FLOW_WORD_MAX - 1);
      return 2;
    }
    /* The projection is the configured one, not a built-in R->i. The MetaFlow
     * entry that owns this channel is the same entry that names it, so the two
     * halves of "what R means" cannot drift apart. */
    {
      int m;
      for (m = 0; m < CANOR_DEMO_AXIS_N; ++m) if (c->meta[m].channel == chan) break;
      if (m == CANOR_DEMO_AXIS_N || !c->meta[m].unit) {
        fprintf(stderr, "follow: channel %c has no configured unit\n", ch);
        return 2;
      }
      word[n++] = c->meta[m].unit;
    }
    ++cnt[chan];
  }
  word[n] = 0;
  if (!n) { fprintf(stderr, "follow: empty string\n"); return 2; }

  memset(&r, 0, sizeof r);
  if (!canor_flows_add(&r, "rgb", word) || !canor_flow_fold(&r, 0, &u, &sg)) {
    fprintf(stderr, "follow: fold failed\n");
    return 2;
  }
  mi = canor_demo_meta_of(c, cnt);
  xi = canor_demo_mix_of(c, cnt);

  printf("string      %s\n", str);
  printf("word        %s\n", word);
  printf("counts      R=%d G=%d B=%d\n", cnt[0], cnt[1], cnt[2]);
  printf("fold        %s%s\n", sg < 0 ? "-" : "+", canor_unit_name(u));
  /* The fold lands on a unit, and the unit indexes the Flow registry in
   * declaration order - one=first, i=second, j=third, k=fourth. A registry
   * with fewer than four entries cannot name every fold, and says so rather
   * than reading past its end. */
  if (u < (uint8_t)c->flows.count)
    printf("Flow        %s\n", c->flows.flow[u].name);
  else
    printf("Flow        (unit %s has no configured Flow)\n", canor_unit_name(u));
  printf("MetaFlow    %-14s %s\n", c->meta[mi].name, canor_demo_kclass_name(c->meta[mi].kclass));
  printf("Mixture     %s\n", c->mix[xi].name);
  return 0;
}

int canor_demo_cmd_metaflows(const canor_demo_config_t *c) {
  int i;
  printf("MetaFlow axis - RGB level, four-valued, last slot is the refusal\n\n");
  printf("  %-14s %-8s %-5s %-12s %14s\n", "name", "channel", "unit",
         "Kapprox", "order at n");
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i)
    printf("  %-14s %-8s %-5c %-12s %14lld\n", c->meta[i].name,
           c->meta[i].channel < 0 ? "none" : (const char *[]){"R","G","B"}[c->meta[i].channel],
           c->meta[i].unit ? c->meta[i].unit : '-',
           canor_demo_kclass_name(c->meta[i].kclass),
           (long long)canor_kapprox_order_milli(c->meta[i].kclass,
                                             (uint64_t)c->set.levels));
  printf("\nMixture axis - MetaFlow and Flow together\n\n");
  printf("  %-14s %-6s %-10s\n", "name", "dims", "dominant");
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i) {
    if (c->mix[i].dims < 0) printf("  %-14s %-6s %-10s\n", c->mix[i].name, "-", "(refusal)");
    else printf("  %-14s %-6d %-10s\n", c->mix[i].name, c->mix[i].dims,
                c->mix[i].dominant ? "required" : "no");
  }
  return 0;
}
