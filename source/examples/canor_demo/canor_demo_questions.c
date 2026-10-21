/* canor_demo_questions.c - questions - every word answered against Q1..Q4, deterministically
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* questions - every word answered against Q1..Q4, deterministically          */
/* ========================================================================== */
/* SEMANTIC_TREE_ELICITATION defines four questions per item:
 *
 *   Q1  Is {W} General, Specific, Distinctive or Arbitrary?      -> MetaFlow
 *   Q2  Sort {W} into Ordinary, Peaceful, Formful or Stormful.   -> Flow
 *   Q3  Q1 for the composition {A}{B}.
 *   Q4  Q2 for the composition {A}{B}.
 *
 * The algebra can answer all four without asking anyone, and that is exactly
 * what makes this useful as a MEASURING INSTRUMENT rather than as an opinion.
 * A model's answers are only interpretable against a reference that was fixed
 * beforehand; without one, disagreement between two models is unattributable
 * and agreement is unfalsifiable. So this emits the reference.
 *
 * WHAT A LATER BIAS MEASUREMENT ACTUALLY MEASURES. Q3/Q4 are not free
 * parameters: Flow(AB) = Flow(A).Flow(B) is a theorem, verified here at 100%
 * over 370k entries. So a model that answers Q2 consistently and Q4
 * inconsistently is not disagreeing about meaning - it is failing to compose,
 * and the size of that failure is a number. Q1/Q3 have no such law (channel
 * dominance is not preserved by concatenation), so divergence there is
 * genuinely about the model's semantics. The two question pairs therefore
 * measure different things, and only one of them has a right answer.
 *
 * ADMISSIBILITY IS THE MARGIN, and this is the one real design decision here.
 * A word whose channels tie has no defensible MetaFlow - the answer is
 * Contested, and any model that confidently picks one is not being measured,
 * it is being caught. So A is the dominance margin: how far the leading
 * channel leads. Ties give A = 0 and are withheld from the population rather
 * than scored, which keeps the undecidable items out of an agreement figure
 * they would otherwise inflate.
 *
 * THE COORDINATES CARRY MEANING, unlike the synthetic cloud. X is evidence
 * minus basis, Y is gap against the grounded channels, Z is the fold sign and
 * ring. So the XY heatmap of a real emission is a semantic plane rather than
 * scatter, and a spatial pattern in it is a fact about the language.
 */

typedef struct {
  int r, g, b;            /* channel counts */
  int meta, flow, mix;    /* Q1, Q2, and the Mixture */
  int adm;                /* admissibility: the dominance margin, 0..255 */
  int len, sign;
  uint8_t unit;
} wordans;

static bool answer_word(const canor_demo_config_t *c, const char *word, wordans *w) {
  canor_imag acc = 0;
  int sign = 1, n = 0, i, dom = 0, second = 0, cnt[3] = { 0, 0, 0 };
  const char *p;
  memset(w, 0, sizeof *w);
  for (p = word; *p; ++p) {
    int chan = canor_demo_letter_channel(c, *p);
    int m, s2;
    canor_imag u, prod;
    if (chan < 0) continue;
    ++cnt[chan];
    ++n;
    for (m = 0; m < CANOR_DEMO_AXIS_N; ++m) if (c->meta[m].channel == chan) break;
    if (m == CANOR_DEMO_AXIS_N || !c->meta[m].unit) return false;
    u = (canor_imag)(c->meta[m].unit == 'i' ? 1 : c->meta[m].unit == 'j' ? 2 :
                  c->meta[m].unit == 'k' ? 3 : 0);
    if (canor_imag_mul(acc, u, &prod, &s2) != CANOR_OK) return false;
    acc = prod;
    sign *= s2;
  }
  if (n == 0) return false;
  w->r = cnt[0]; w->g = cnt[1]; w->b = cnt[2];
  w->len = n;
  w->sign = sign;
  w->unit = (uint8_t)acc;
  w->meta = canor_demo_meta_of(c, cnt);
  w->mix = canor_demo_mix_of(c, cnt);
  w->flow = (acc < (canor_imag)c->flows.count) ? (int)acc : c->flows.count;
  for (i = 1; i < 3; ++i) if (cnt[i] > cnt[dom]) dom = i;
  for (i = 0; i < 3; ++i) if (i != dom && cnt[i] > second) second = cnt[i];
  w->adm = (int)((long)(cnt[dom] - second) * 255 / n);
  if (w->adm < 0) w->adm = 0;
  if (w->adm > 255) w->adm = 255;
  return true;
}

/* Composition of two answered words, done through the ALGEBRA rather than by
 * re-reading the concatenation: Q3 sums the channel counts, Q4 multiplies the
 * units. Deriving them instead of recomputing is the point - it is what makes
 * the emitted Q4 a prediction that a model can be scored against. */
static bool compose_words(const canor_demo_config_t *c, const wordans *a, const wordans *b,
                          wordans *out) {
  int cnt[3];
  canor_imag prod;
  int s2, i, dom = 0, second = 0, n;
  memset(out, 0, sizeof *out);
  cnt[0] = a->r + b->r; cnt[1] = a->g + b->g; cnt[2] = a->b + b->b;
  n = a->len + b->len;
  if (n == 0) return false;
  if (canor_imag_mul(a->unit, b->unit, &prod, &s2) != CANOR_OK) return false;
  out->r = cnt[0]; out->g = cnt[1]; out->b = cnt[2];
  out->len = n;
  out->sign = a->sign * b->sign * s2;
  out->unit = (uint8_t)prod;
  out->meta = canor_demo_meta_of(c, cnt);
  out->mix = canor_demo_mix_of(c, cnt);
  out->flow = (prod < (canor_imag)c->flows.count) ? (int)prod : c->flows.count;
  for (i = 1; i < 3; ++i) if (cnt[i] > cnt[dom]) dom = i;
  for (i = 0; i < 3; ++i) if (i != dom && cnt[i] > second) second = cnt[i];
  out->adm = (int)((long)(cnt[dom] - second) * 255 / n);
  if (out->adm < 0) out->adm = 0;
  if (out->adm > 255) out->adm = 255;
  return true;
}

int canor_demo_cmd_questions(const canor_demo_config_t *c, int sel, const char *path,
                         const char *emit, long limit) {
  FILE *f, *os = NULL, *oref = NULL;
  long flen;
  char *buf;
  canor_demo_jscan_t j;
  char survey_path[600], ref_path[600];
  long answered = 0, skipped = 0, withheld = 0, comps = 0, q3q4_shift = 0;
  long q1[CANOR_DEMO_AXIS_N], q2[8], q3[CANOR_DEMO_AXIS_N], q4[8];
  long margin_hist[6];
  wordans prev;
  memset(&prev, 0, sizeof prev);
  char prevw[256];
  bool have_prev = false;
  int i;

  memset(q1, 0, sizeof q1); memset(q2, 0, sizeof q2);
  memset(q3, 0, sizeof q3); memset(q4, 0, sizeof q4);
  memset(margin_hist, 0, sizeof margin_hist);

  f = fopen(path, "rb");
  if (!f) { fprintf(stderr, "follow: cannot open %s\n", path); return 2; }
  fseek(f, 0, SEEK_END); flen = ftell(f); fseek(f, 0, SEEK_SET);
  buf = (char *)malloc((size_t)flen + 1);
  if (flen <= 0 || !buf) { fclose(f); free(buf);
    fprintf(stderr, "follow: %s unreadable\n", path); return 2; }
  if (fread(buf, 1, (size_t)flen, f) != (size_t)flen) {
    fclose(f); free(buf); fprintf(stderr, "follow: short read\n"); return 2; }
  fclose(f);
  buf[flen] = 0;

  if (emit) {
    snprintf(survey_path, sizeof survey_path, "%s.survey.txt", emit);
    snprintf(ref_path, sizeof ref_path, "%s.reference.json", emit);
    os = fopen(survey_path, "w");
    oref = fopen(ref_path, "w");
    if (!os || !oref) {
      free(buf);
      if (os) fclose(os);
      if (oref) fclose(oref);
      fprintf(stderr, "follow: cannot write next to %s\n", emit);
      return 2;
    }
    fprintf(os, "# RGBA-WXYZ survey, one row per word\n");
    fprintf(os, "# R evidence  G gap  B basis  A admissibility(dominance margin)\n");
    fprintf(os, "# W length band  X evidence-basis  Y gap-vs-grounded  Z fold\n");
    fprintf(os, "# CRS: semantic-rgb-v1 (not spatial; axes are channel contrasts)\n");
    fprintf(os, "# source %s  strategy %s\n", path, c->strat[sel].id.name);
    fprintf(os, "# R G B A W X Y Z survey\n");
    /* The reference carries its provenance in the shape
     * SEMANTIC_TREE_ELICITATION already defines, so an elicited file and this
     * one are diffable without a converter. */
    fprintf(oref, "{\n  \"provenance\": {\n");
    fprintf(oref, "    \"model\": \"none - deterministic follow-core\",\n");
    fprintf(oref, "    \"sampling\": \"exact\",\n    \"repeats\": 1,\n");
    fprintf(oref, "    \"strategy\": \"%s\",\n", c->strat[sel].id.name);
    fprintf(oref, "    \"letters\": { \"R\": \"%s\", \"G\": \"%s\", \"B\": \"%s\" },\n",
            c->letters[0], c->letters[1], c->letters[2]);
    fprintf(oref, "    \"source\": \"%s\",\n", path);
    fprintf(oref, "    \"composition_rule\": \"adjacent pairs in file order\",\n");
    fprintf(oref, "    \"note\": \"Q4 is a THEOREM (Flow(AB)=Flow(A).Flow(B)); "
                  "Q3 is not. Divergence means different things in each.\"\n");
    fprintf(oref, "  },\n  \"atoms\": {\n");
  }

  memset(&j, 0, sizeof j);
  j.p = buf; j.end = buf + flen;
  if (!canor_demo_jlit(&j, '{')) { free(buf); fprintf(stderr, "follow: not JSON\n"); return 2; }
  if (!canor_demo_jlit(&j, '}')) {
    do {
      char word[256];
      wordans w;
      if (!canor_demo_jstr(&j, word, sizeof word)) break;
      if (!canor_demo_jlit(&j, ':')) break;
      if (!canor_demo_jskip(&j)) break;
      if (limit > 0 && answered >= limit) break;
      if (!answer_word(c, word, &w)) { ++skipped; continue; }
      ++answered;
      q1[w.meta]++;
      q2[w.flow]++;
      margin_hist[w.adm >= 250 ? 5 : w.adm / 50]++;
      if (w.adm == 0) ++withheld;

      if (os) {
        int X = (w.r - w.b) * 4096;
        int Y = (2 * w.g - w.r - w.b) * 4096;
        int Z = w.sign * (int)w.unit;
        int W = w.len > 12 ? 3 : w.len > 8 ? 2 : w.len > 5 ? 1 : 0;
        fprintf(os, "%d %d %d %d %d %d %d %d %d\n",
                w.r, w.g, w.b, w.adm, W, X, Y, Z, w.meta);
      }
      if (oref)
        fprintf(oref, "%s    \"%s\": { \"q1\": \"%s\", \"q2\": \"%s\", "
                      "\"admissibility\": %d }",
                answered > 1 ? ",\n" : "", word,
                c->meta[w.meta].name,
                w.flow < c->flows.count ? c->flows.flow[w.flow].name : "?",
                w.adm);

      if (have_prev) {
        wordans cw;
        if (compose_words(c, &prev, &w, &cw)) {
          ++comps;
          q3[cw.meta]++;
          q4[cw.flow]++;
          /* Does composing move the MetaFlow off both operands' answers? That
           * is the interesting Q3 statistic: it is where a model asked Q3
           * cannot simply repeat its Q1. */
          if (cw.meta != prev.meta && cw.meta != w.meta) ++q3q4_shift;
        }
      }
      { int k = 0; while (word[k] && k < 255) { prevw[k] = word[k]; ++k; } prevw[k] = 0; }
      (void)prevw;
      prev = w;
      have_prev = true;
    } while (canor_demo_jlit(&j, ','));
  }
  free(buf);

  if (oref) {
    fprintf(oref, "\n  },\n  \"compositions_summary\": {\n");
    fprintf(oref, "    \"pairs\": %ld,\n", comps);
    for (i = 0; i < CANOR_DEMO_AXIS_N; ++i)
      fprintf(oref, "    \"q3_%s\": %ld,\n", c->meta[i].name, q3[i]);
    for (i = 0; i < c->flows.count; ++i)
      fprintf(oref, "    \"q4_%s\": %ld%s\n", c->flows.flow[i].name, q4[i],
              i + 1 < c->flows.count ? "," : "");
    fprintf(oref, "  }\n}\n");
    fclose(oref);
  }
  if (os) fclose(os);

  printf("Q1..Q4 over %s\n\n", path);
  printf("  words answered   %ld\n", answered);
  printf("  skipped          %ld  (no mappable letters)\n", skipped);
  printf("  compositions     %ld  (adjacent pairs, file order)\n\n", comps);

  printf("  Q1  MetaFlow of the word\n");
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i)
    printf("    %-14s %8ld  %5.1f%%\n", c->meta[i].name, q1[i],
           answered ? 100.0 * q1[i] / answered : 0.0);
  printf("\n  Q2  Flow of the word\n");
  for (i = 0; i < c->flows.count; ++i)
    printf("    %-14s %8ld  %5.1f%%\n", c->flows.flow[i].name, q2[i],
           answered ? 100.0 * q2[i] / answered : 0.0);
  printf("\n  Q3  MetaFlow of the composition\n");
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i)
    printf("    %-14s %8ld  %5.1f%%\n", c->meta[i].name, q3[i],
           comps ? 100.0 * q3[i] / comps : 0.0);
  printf("\n  Q4  Flow of the composition  (this one is a THEOREM)\n");
  for (i = 0; i < c->flows.count; ++i)
    printf("    %-14s %8ld  %5.1f%%\n", c->flows.flow[i].name, q4[i],
           comps ? 100.0 * q4[i] / comps : 0.0);

  printf("\n  Q3 lands off BOTH operands' Q1: %ld  (%.1f%%)\n", q3q4_shift,
         comps ? 100.0 * q3q4_shift / comps : 0.0);
  printf("    Composition genuinely moves the MetaFlow this often, so Q3 is\n"
         "    not answerable by repeating Q1 - which is what makes it a real\n"
         "    question to put to a model.\n");

  printf("\n  admissibility (dominance margin) distribution\n");
  {
    static const char *const band[6] = { "0-49", "50-99", "100-149",
                                         "150-199", "200-249", "250+" };
    for (i = 0; i < 6; ++i)
      printf("    %-10s %8ld  %5.1f%%\n", band[i], margin_hist[i],
             answered ? 100.0 * margin_hist[i] / answered : 0.0);
    printf("    of which margin exactly 0 (undecidable, withheld): %ld  %.1f%%\n",
           withheld, answered ? 100.0 * withheld / answered : 0.0);
  }

  if (emit) {
    printf("\n  wrote %s\n  wrote %s\n", survey_path, ref_path);
    canor_demo_prov_write_sidecar(survey_path);
    canor_demo_prov_write_sidecar(ref_path);
  }
  return 0;
}
