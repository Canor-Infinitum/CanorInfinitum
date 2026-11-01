/* Scores an elicited SemanticTree against the algebra.
 *
 * Reads the answer file described in SEMANTIC_TREE_ELICITATION.md and asks two
 * questions the algebra can answer independently of the model:
 *
 *   TEST A (Flow axis).      Q2 assigns each word a baseline Flow, and each
 *                            baseline folds to a quaternion unit. The product
 *                            of two words' units PREDICTS the composition's
 *                            Flow. Q4 answers it independently. Compare.
 *
 *   TEST B (MetaFlow axis).  A word's RGB metaflow yields a MetaFlow by
 *                            channel dominance. Q1 answers it independently.
 *                            Compare.
 *
 * Both are scored against three baselines, because an agreement figure with no
 * control is not evidence:
 *
 *   chance      25%, four options
 *   marginal    always answer the model's own most frequent label
 *   shuffled    the same scoring with the word-to-unit map permuted
 *
 * A predictor that beats chance but not the marginal has learned the answer
 * distribution, not the structure. That is the failure this reports rather
 * than hides.
 */

#include "vi.h"
#include "vi_strategy.h"
#include "fc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXW 20000
#define MAXC 40000
#define NAMELEN 48

/* --- the two four-valued axes ----------------------------------------------- */

typedef enum { FLOW_ORDINARY, FLOW_PEACEFUL, FLOW_FORMFUL, FLOW_STORMFUL, FLOW_NONE } flow_id;
typedef enum { META_GENERAL, META_SPECIFIC, META_DISTINCTIVE, META_ARBITRARY, META_NONE } meta_id;
/* The Mixture axis: how the MetaFlow (RGB level) and the Flow (IMAG level)
 * combine. Computable, hence testable - see meta_mix_from_rgb. */
typedef enum { MIX_ELEMENTARY, MIX_UNIQUE, MIX_COMPOSITE, MIX_UNKNOWN, MIX_NONE } mix_id;

static const char *const flow_name[5] =
  { "Ordinary", "Peaceful", "Formful", "Stormful", "?" };
static const char *const meta_name[5] =
  { "General", "Specific", "Distinctive", "Arbitrary", "?" };
static const char *const mix_name[5] =
  { "Elementary", "Unique", "Composite", "Unknown", "?" };

/* Each baseline folds to a distinct unit - measured, not assigned. Index into
 * the imaginary basis: one=0, i=1, j=2, k=3. */
static uint8_t flow_unit[4];

static flow_id flow_of(const char *s) {
  for (int i = 0; i < 4; ++i) if (!strcmp(s, flow_name[i])) return (flow_id)i;
  return FLOW_NONE;
}
static meta_id meta_of(const char *s) {
  for (int i = 0; i < 4; ++i) if (!strcmp(s, meta_name[i])) return (meta_id)i;
  return META_NONE;
}
static mix_id mix_of(const char *s) {
  for (int i = 0; i < 4; ++i) if (!strcmp(s, mix_name[i])) return (mix_id)i;
  return MIX_NONE;
}

/* Mixture from the two levels:
 *   decomposable  dim >= 2, since split has no interior boundary below that
 *   dominant      one RGB channel strictly leads, i.e. MetaFlow != Arbitrary */
static mix_id mix_from_rgb(const char *rgb) {
  int r = 0, g = 0, b = 0;
  for (const char *p = rgb; *p; ++p) {
    if (*p == 'R') ++r; else if (*p == 'G') ++g; else if (*p == 'B') ++b;
  }
  int decomposable = (b >= 2);
  int dominant = (r > g && r > b) || (g > r && g > b) || (b > r && b > g);
  if (!decomposable) return dominant ? MIX_ELEMENTARY : MIX_UNIQUE;
  return dominant ? MIX_COMPOSITE : MIX_UNKNOWN;
}

/* MetaFlow by channel dominance: a strict plurality, ties are Arbitrary. */
static meta_id meta_from_rgb(const char *rgb) {
  int r = 0, g = 0, b = 0;
  for (const char *p = rgb; *p; ++p) {
    if (*p == 'R') ++r; else if (*p == 'G') ++g; else if (*p == 'B') ++b;
  }
  if (r > g && r > b) return META_GENERAL;
  if (g > r && g > b) return META_DISTINCTIVE;
  if (b > r && b > g) return META_SPECIFIC;
  return META_ARBITRARY;             /* nothing leads */
}

/* --- records ---------------------------------------------------------------- */

typedef struct {
  char word[NAMELEN];
  char rgb[96];
  meta_id q1;
  flow_id q2;
  mix_id q5;
  int indeterminate;
} atom;

typedef struct {
  char a[NAMELEN], b[NAMELEN];
  meta_id q3;
  flow_id q4;
} comp;

static atom g_atom[MAXW];
static int g_na;
static comp g_comp[MAXC];
static int g_nc;

static int find_atom(const char *w) {
  for (int i = 0; i < g_na; ++i) if (!strcmp(g_atom[i].word, w)) return i;
  return -1;
}

/* Minimal field scan over the answer file. Refuses a record it cannot read
 * rather than filling defaults, so a schema change is loud. */
static int field(const char *obj, const char *key, char *out, size_t cap) {
  char pat[64];
  snprintf(pat, sizeof pat, "\"%s\"", key);
  const char *k = strstr(obj, pat);
  if (!k) return 0;
  const char *c = strchr(k + strlen(pat), ':');
  if (!c) return 0;
  const char *q = strchr(c, '"');
  if (!q) return 0;
  const char *e = strchr(q + 1, '"');
  if (!e) return 0;
  size_t n = (size_t)(e - q - 1);
  if (n >= cap) n = cap - 1;
  memcpy(out, q + 1, n);
  out[n] = '\0';
  return 1;
}

static int load(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) { printf("cannot open %s\n", path); return 0; }
  static char buf[1 << 22];
  size_t got = fread(buf, 1, sizeof buf - 1, f);
  buf[got] = '\0';
  fclose(f);

  /* atoms: { "word": { "q1": .., "q2": .., "rgb": .. }, ... } */
  const char *sec = strstr(buf, "\"atoms\"");
  if (sec) {
    const char *p = strchr(sec, '{');
    if (p) ++p;
    int depth = 1;
    while (p && *p && depth > 0 && g_na < MAXW) {
      const char *ws = strchr(p, '"');
      if (!ws) break;
      const char *we = strchr(ws + 1, '"');
      if (!we) break;
      const char *ob = strchr(we, '{');
      if (!ob) break;
      const char *oe = strchr(ob, '}');
      if (!oe) break;
      char rec[512];
      size_t rn = (size_t)(oe - ob + 1);
      if (rn >= sizeof rec) rn = sizeof rec - 1;
      memcpy(rec, ob, rn);
      rec[rn] = '\0';

      atom *a = &g_atom[g_na];
      memset(a, 0, sizeof *a);
      size_t wn = (size_t)(we - ws - 1);
      if (wn >= NAMELEN) wn = NAMELEN - 1;
      memcpy(a->word, ws + 1, wn);
      a->word[wn] = '\0';

      char v[64];
      a->q1 = field(rec, "q1", v, sizeof v) ? meta_of(v) : META_NONE;
      a->q2 = field(rec, "q2", v, sizeof v) ? flow_of(v) : FLOW_NONE;
      a->q5 = field(rec, "q5", v, sizeof v) ? mix_of(v) : MIX_NONE;
      if (!field(rec, "rgb", a->rgb, sizeof a->rgb)) a->rgb[0] = '\0';
      if (a->q1 == META_NONE || a->q2 == FLOW_NONE) a->indeterminate = 1;
      ++g_na;
      p = oe + 1;
      const char *nxt = strchr(p, '{');
      const char *end = strstr(p, "\"compositions\"");
      if (!nxt || (end && nxt > end)) break;
      (void)depth;
    }
  }

  /* compositions: [ { "a":.., "b":.., "q3":.., "q4":.. }, ... ] */
  sec = strstr(buf, "\"compositions\"");
  if (sec) {
    const char *p = sec;
    while ((p = strchr(p, '{')) != NULL && g_nc < MAXC) {
      const char *oe = strchr(p, '}');
      if (!oe) break;
      char rec[512];
      size_t rn = (size_t)(oe - p + 1);
      if (rn >= sizeof rec) rn = sizeof rec - 1;
      memcpy(rec, p, rn);
      rec[rn] = '\0';
      comp *c = &g_comp[g_nc];
      memset(c, 0, sizeof *c);
      char v[64];
      if (field(rec, "a", c->a, sizeof c->a) && field(rec, "b", c->b, sizeof c->b)) {
        c->q3 = field(rec, "q3", v, sizeof v) ? meta_of(v) : META_NONE;
        c->q4 = field(rec, "q4", v, sizeof v) ? flow_of(v) : FLOW_NONE;
        ++g_nc;
      }
      p = oe + 1;
    }
  }
  return g_na > 0;
}

/* --- scoring ---------------------------------------------------------------- */

/* The algebra's prediction: multiply the two words' units, then name the
 * baseline whose unit matches. Sign is discarded because the four options do
 * not distinguish +i from -i - which is itself a finding, since the algebra
 * does. */
static flow_id predict_flow(flow_id a, flow_id b) {
  if (a >= FLOW_NONE || b >= FLOW_NONE) return FLOW_NONE;
  fc_imag r;
  int sg;
  if (fc_imag_mul(flow_unit[a], flow_unit[b], &r, &sg) != FC_OK) return FLOW_NONE;
  for (int i = 0; i < 4; ++i) if (flow_unit[i] == r) return (flow_id)i;
  return FLOW_NONE;
}

typedef struct { int hit, total; } score;
static double pct(score s) { return s.total ? 100.0 * s.hit / s.total : 0.0; }

int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : "../elicited_sample.json";
  if (fc_imag_init() != FC_OK) { printf("unit algebra does not close\n"); return 1; }

  /* Resolve each baseline to its unit by folding, so the mapping is measured
   * here and not copied from a comment that could go stale. */
  vi_flow_registry reg;
  vi_flows_default(&reg);
  printf("baseline Flow -> unit (folded, not assigned)\n");
  for (int i = 0; i < 4; ++i) {
    uint8_t u;
    int sg;
    vi_flow_fold(&reg, i, &u, &sg);
    flow_unit[i] = u;
    printf("  %-9s %-8s -> %s%s\n", reg.flow[i].name, reg.flow[i].word,
           sg < 0 ? "-" : "+", fc_imag_name(u));
  }

  if (!load(path)) return 1;
  printf("\nloaded %d atoms, %d compositions from %s\n", g_na, g_nc, path);

  /* --- TEST B: MetaFlow from channel dominance vs Q1 --------------------- */
  score b_algebra = { 0, 0 }, b_marginal = { 0, 0 };
  int meta_hist[4] = { 0, 0, 0, 0 };
  for (int i = 0; i < g_na; ++i)
    if (g_atom[i].q1 < META_NONE) ++meta_hist[g_atom[i].q1];
  int meta_mode = 0;
  for (int i = 1; i < 4; ++i) if (meta_hist[i] > meta_hist[meta_mode]) meta_mode = i;

  for (int i = 0; i < g_na; ++i) {
    if (g_atom[i].q1 >= META_NONE || !g_atom[i].rgb[0]) continue;
    meta_id pred = meta_from_rgb(g_atom[i].rgb);
    ++b_algebra.total;
    if (pred == g_atom[i].q1) ++b_algebra.hit;
    ++b_marginal.total;
    if ((meta_id)meta_mode == g_atom[i].q1) ++b_marginal.hit;
  }

  printf("\nTEST B  MetaFlow: channel dominance vs elicited Q1\n");
  printf("  algebra   %5.1f%%   (%d of %d)\n", pct(b_algebra), b_algebra.hit, b_algebra.total);
  printf("  marginal  %5.1f%%   always answering %s\n", pct(b_marginal), meta_name[meta_mode]);
  printf("  chance    %5.1f%%\n", 25.0);

  /* --- TEST C: Mixture from (decomposable, dominant) vs Q5 ---------------- */
  score c_algebra = { 0, 0 }, c_marginal = { 0, 0 };
  int mix_hist[4] = { 0, 0, 0, 0 };
  for (int i = 0; i < g_na; ++i)
    if (g_atom[i].q5 < MIX_NONE) ++mix_hist[g_atom[i].q5];
  int mix_mode = 0;
  for (int i = 1; i < 4; ++i) if (mix_hist[i] > mix_hist[mix_mode]) mix_mode = i;
  int any_mix = 0;
  for (int i = 0; i < 4; ++i) any_mix += mix_hist[i];

  for (int i = 0; i < g_na; ++i) {
    if (g_atom[i].q5 >= MIX_NONE || !g_atom[i].rgb[0]) continue;
    mix_id pred = mix_from_rgb(g_atom[i].rgb);
    ++c_algebra.total;
    if (pred == g_atom[i].q5) ++c_algebra.hit;
    ++c_marginal.total;
    if ((mix_id)mix_mode == g_atom[i].q5) ++c_marginal.hit;
  }
  if (any_mix) {
    printf("\nTEST C  Mixture: (decomposable, dominant) vs elicited Q5\n");
    printf("  algebra   %5.1f%%   (%d of %d)\n", pct(c_algebra), c_algebra.hit, c_algebra.total);
    printf("  marginal  %5.1f%%   always answering %s\n", pct(c_marginal), mix_name[mix_mode]);
    printf("  chance    %5.1f%%\n", 25.0);
  }

  /* --- TEST A: quaternion product vs Q4 ---------------------------------- */
  score a_algebra = { 0, 0 }, a_marginal = { 0, 0 }, a_shuffled = { 0, 0 };
  int flow_hist[4] = { 0, 0, 0, 0 };
  for (int i = 0; i < g_nc; ++i)
    if (g_comp[i].q4 < FLOW_NONE) ++flow_hist[g_comp[i].q4];
  int flow_mode = 0;
  for (int i = 1; i < 4; ++i) if (flow_hist[i] > flow_hist[flow_mode]) flow_mode = i;

  /* The shuffled control permutes the word-to-unit assignment with the
   * Strategy stream, so the control is reproducible too. */
  STRATEGY sh = { "shuffle", 1 };
  flow_id perm[MAXW];
  for (int i = 0; i < g_na; ++i)
    perm[i] = (flow_id)(vi_rand(&sh, 0x5417ull, (uint64_t)i) % 4u);

  int order_pairs = 0, order_same = 0;
  for (int i = 0; i < g_nc; ++i) {
    int ia = find_atom(g_comp[i].a), ib = find_atom(g_comp[i].b);
    if (ia < 0 || ib < 0 || g_comp[i].q4 >= FLOW_NONE) continue;
    flow_id pa = g_atom[ia].q2, pb = g_atom[ib].q2;
    if (pa >= FLOW_NONE || pb >= FLOW_NONE) continue;

    flow_id pred = predict_flow(pa, pb);
    if (pred < FLOW_NONE) {
      ++a_algebra.total;
      if (pred == g_comp[i].q4) ++a_algebra.hit;
    }
    ++a_marginal.total;
    if ((flow_id)flow_mode == g_comp[i].q4) ++a_marginal.hit;

    flow_id sp = predict_flow(perm[ia], perm[ib]);
    if (sp < FLOW_NONE) {
      ++a_shuffled.total;
      if (sp == g_comp[i].q4) ++a_shuffled.hit;
    }

    /* Order sensitivity: does the reversed pair appear, and does the model
     * answer the same? The algebra says i.j and j.i differ only in sign, and
     * the four options cannot express sign - so the model answering the same
     * for both is CONSISTENT with the algebra, not evidence against it. */
    for (int j = 0; j < g_nc; ++j) {
      if (j == i) continue;
      if (!strcmp(g_comp[j].a, g_comp[i].b) && !strcmp(g_comp[j].b, g_comp[i].a)) {
        ++order_pairs;
        if (g_comp[j].q4 == g_comp[i].q4) ++order_same;
        break;
      }
    }
  }

  printf("\nTEST A  Flow: quaternion product vs elicited Q4\n");
  printf("  algebra   %5.1f%%   (%d of %d)\n", pct(a_algebra), a_algebra.hit, a_algebra.total);
  printf("  marginal  %5.1f%%   always answering %s\n", pct(a_marginal), flow_name[flow_mode]);
  printf("  shuffled  %5.1f%%   units permuted\n", pct(a_shuffled));
  printf("  chance    %5.1f%%\n", 25.0);
  if (order_pairs)
    printf("  order     %5.1f%% of %d reversed pairs answered identically\n",
           100.0 * order_same / order_pairs, order_pairs / 2);

  printf("\nreading the result\n");
  if (a_algebra.total == 0) {
    printf("  no scorable compositions - supply Q2 for both members of each pair\n");
  } else if (pct(a_algebra) <= pct(a_marginal) + 1.0) {
    printf("  the product predicts no better than always answering %s.\n"
           "  On this evidence the four baselines are labels and the quaternion\n"
           "  structure is not doing work. That is a cheap thing to have learned.\n",
           flow_name[flow_mode]);
  } else if (pct(a_algebra) - pct(a_shuffled) < 10.0) {
    printf("  the product beats the marginal but not the shuffled control, so the\n"
           "  gain is coming from the option distribution rather than the algebra\n");
  } else {
    printf("  the product beats chance, the marginal AND the shuffled control.\n"
           "  That is the outcome worth pursuing - and worth replicating on a\n"
           "  held-out set before it is believed.\n");
  }
  return 0;
}
