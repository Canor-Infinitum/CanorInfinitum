/* canor_demo_tree.c - tree - a deterministic SemanticTree from a bare word list
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* tree - a deterministic SemanticTree from a bare word list                  */
/* ========================================================================== */
/* words_dictionary.json is a word list and nothing else - no RGB, no
 * semantics. So the projection has to be DERIVED, and that is the interesting
 * part rather than a chore: v1-beta arrived with its MetaFlow axis 73.7%
 * Distinctive and 0.1% General, which is a property of whatever generator
 * wrote it, not of English. Deriving the projection here means the balance can
 * be designed and then MEASURED.
 *
 * THE LETTER MAP. Partitioning the alphabet evenly (a-i, j-r, s-z) would be
 * balanced by alphabet and wildly unbalanced by usage, because English letter
 * frequency spans two orders of magnitude - 'e' is 180x 'z'. The default here
 * partitions by FREQUENCY MASS instead: each channel receives about a third of
 * the letters actually written, so a typical word has no built-in channel
 * preference and MetaFlow measures the word rather than the encoding. It is
 * configurable, because "balanced for English" is not balanced for anything
 * else.
 *
 * THE TREE. Five levels, each a strictly coarser question than the next:
 *
 *   1 MetaFlow   which RGB channel dominates            (4, refusal included)
 *   2 Flow       what the word folds to under Hamilton  (4)
 *   3 Mixture    the MetaFlow/Flow pair                 (4)
 *   4 Left       di/dj/dk under LeftBiased              (up to 64)
 *   5 Right      how RightBiased diverges from Left     (2: agrees, differs)
 *
 * Levels 4 and 5 are the Left/Right modelling. Level 4 is what a left-biased
 * reader concludes; level 5 is whether a right-biased reader would have
 * concluded anything different - which is exactly the subset of the language
 * where the bias choice is load-bearing. Everywhere else the two agree and the
 * distinction costs nothing to ignore. */

#define TREE_L4 64

typedef struct {
  long meta[CANOR_DEMO_AXIS_N];
  long flow[CANOR_DEMO_AXIS_N][8];
  long mix[CANOR_DEMO_AXIS_N][8][CANOR_DEMO_AXIS_N];
  long l4[CANOR_DEMO_AXIS_N][8][CANOR_DEMO_AXIS_N][TREE_L4];
  long diverge[CANOR_DEMO_AXIS_N][8][CANOR_DEMO_AXIS_N][TREE_L4];
  long words, undef, skipped;
  long chan_letters[3];
} semtree;

int canor_demo_letter_channel(const canor_demo_config_t *c, char ch) {
  int i;
  if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
  for (i = 0; i < 3; ++i) {
    const char *s = c->letters[i];
    while (*s) { if (*s == ch) return i; ++s; }
  }
  return -1;                        /* not a letter this map knows */
}

int canor_demo_cmd_tree(const canor_demo_config_t *c, int sel, const char *path, const char *emit) {
  FILE *f;
  long flen;
  char *buf;
  canor_demo_jscan_t j;
  semtree *t;
  canor_strategy sL = c->strat[sel], sR = c->strat[sel];
  int mi, fi, xi, k;
  long nodes4 = 0, nodes5 = 0, biggest = 0, live3 = 0;

  sL.bias = LeftBiased;
  sR.bias = RightBiased;

  f = fopen(path, "rb");
  if (!f) { fprintf(stderr, "follow: cannot open %s\n", path); return 2; }
  fseek(f, 0, SEEK_END); flen = ftell(f); fseek(f, 0, SEEK_SET);
  if (flen <= 0) { fclose(f); fprintf(stderr, "follow: %s is empty\n", path); return 2; }
  buf = (char *)malloc((size_t)flen + 1);
  t = (semtree *)calloc(1, sizeof *t);
  if (!buf || !t) { fclose(f); free(buf); free(t);
                    fprintf(stderr, "follow: out of memory\n"); return 2; }
  if (fread(buf, 1, (size_t)flen, f) != (size_t)flen) {
    fclose(f); free(buf); free(t);
    fprintf(stderr, "follow: short read on %s\n", path);
    return 2;
  }
  fclose(f);
  buf[flen] = 0;

  memset(&j, 0, sizeof j);
  j.p = buf; j.end = buf + flen;
  if (!canor_demo_jlit(&j, '{')) {
    free(buf); free(t);
    fprintf(stderr, "follow: %s is not a JSON object\n", path);
    return 2;
  }
  if (!canor_demo_jlit(&j, '}')) {
    do {
      char word[256];
      int cnt[3] = { 0, 0, 0 };
      canor_imag acc = 0;
      int sign = 1, ok = 1, n = 0;
      const char *p;
      FLOWVALUE vl[3], vr[3];
      int32_t args[4];
      int code, div;

      if (!canor_demo_jstr(&j, word, sizeof word)) break;
      if (!canor_demo_jlit(&j, ':')) break;
      if (!canor_demo_jskip(&j)) break;              /* the value is 1; the word is the datum */

      for (p = word; *p; ++p) {
        int chan = canor_demo_letter_channel(c, *p);
        int m, s2;
        canor_imag u, prod;
        if (chan < 0) continue;           /* apostrophes, digits: not letters */
        ++cnt[chan];
        ++t->chan_letters[chan];
        ++n;
        for (m = 0; m < CANOR_DEMO_AXIS_N; ++m) if (c->meta[m].channel == chan) break;
        if (m == CANOR_DEMO_AXIS_N || !c->meta[m].unit) { ok = 0; break; }
        u = (canor_imag)(c->meta[m].unit == 'i' ? 1 : c->meta[m].unit == 'j' ? 2 :
                      c->meta[m].unit == 'k' ? 3 : 0);
        if (canor_imag_mul(acc, u, &prod, &s2) != CANOR_OK) { ok = 0; break; }
        acc = prod;
        sign *= s2;
      }
      if (!ok || n == 0) { ++t->skipped; continue; }
      ++t->words;

      mi = canor_demo_meta_of(c, cnt);
      fi = (acc < (canor_imag)c->flows.count) ? (int)acc : c->flows.count;
      xi = canor_demo_mix_of(c, cnt);

      /* The differential arguments come from the word itself: the three
       * channel counts and its length. No external numbers, so the whole tree
       * is a function of the word list and the config. */
      args[0] = cnt[0]; args[1] = cnt[1]; args[2] = cnt[2]; args[3] = n * sign;
      if (!canor_di(&sL, args, 4, &vl[0]) || !canor_dj(&sL, args, 4, &vl[1]) ||
          !canor_dk(&sL, args, 4, &vl[2]) ||
          !canor_di(&sR, args, 4, &vr[0]) || !canor_dj(&sR, args, 4, &vr[1]) ||
          !canor_dk(&sR, args, 4, &vr[2])) {
        ++t->undef;
        continue;
      }
      code = (int)vl[0] * 16 + (int)vl[1] * 4 + (int)vl[2];
      div = (vl[0] != vr[0] || vl[1] != vr[1] || vl[2] != vr[2]) ? 1 : 0;

      t->meta[mi]++;
      t->flow[mi][fi]++;
      t->mix[mi][fi][xi]++;
      t->l4[mi][fi][xi][code]++;
      if (div) t->diverge[mi][fi][xi][code]++;
    } while (canor_demo_jlit(&j, ','));
  }
  free(buf);

  printf("SemanticTree from %s\n\n", path);
  printf("  words placed         %ld\n", t->words);
  printf("  skipped (no letters) %ld\n", t->skipped);
  printf("  undefined differential %ld\n\n", t->undef);

  {
    long tl = t->chan_letters[0] + t->chan_letters[1] + t->chan_letters[2];
    printf("  letter mass per channel - the balance the map was designed for\n");
    for (k = 0; k < 3; ++k)
      printf("    %c  %-14s %10ld  %5.1f%%\n", "RGB"[k], c->letters[k],
             t->chan_letters[k], tl ? 100.0 * t->chan_letters[k] / tl : 0.0);
    printf("\n");
  }

  printf("  level 1, MetaFlow\n");
  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi)
    printf("    %-14s %10ld  %5.1f%%\n", c->meta[mi].name, t->meta[mi],
           t->words ? 100.0 * t->meta[mi] / t->words : 0.0);

  printf("\n  level 2, Flow within MetaFlow\n    %-14s", "");
  for (fi = 0; fi < c->flows.count; ++fi) printf(" %10s", c->flows.flow[fi].name);
  printf("\n");
  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) {
    printf("    %-14s", c->meta[mi].name);
    for (fi = 0; fi < c->flows.count; ++fi) printf(" %10ld", t->flow[mi][fi]);
    printf("\n");
  }

  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi)
    for (fi = 0; fi <= c->flows.count; ++fi)
      for (xi = 0; xi < CANOR_DEMO_AXIS_N; ++xi) {
        if (t->mix[mi][fi][xi]) ++live3;
        for (k = 0; k < TREE_L4; ++k) {
          if (!t->l4[mi][fi][xi][k]) continue;
          ++nodes4;
          if (t->l4[mi][fi][xi][k] > biggest) biggest = t->l4[mi][fi][xi][k];
          if (t->diverge[mi][fi][xi][k]) ++nodes5;
        }
      }

  {
    long dtot = 0;
    for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi)
      for (fi = 0; fi <= c->flows.count; ++fi)
        for (xi = 0; xi < CANOR_DEMO_AXIS_N; ++xi)
          for (k = 0; k < TREE_L4; ++k) dtot += t->diverge[mi][fi][xi][k];
    printf("\n  shape\n");
    printf("    level 3 nodes (Mixture)      %ld\n", live3);
    printf("    level 4 nodes (Left bias)    %ld\n", nodes4);
    printf("    level 5 splits (Right bias)  %ld  of those level-4 nodes\n", nodes5);
    printf("    largest leaf                 %ld words\n", biggest);
    printf("    mean leaf                    %.1f words\n",
           nodes4 ? (double)t->words / nodes4 : 0.0);
    printf("\n  Left/Right modelling\n");
    printf("    words where the bias matters %ld", dtot);
    if (t->words) printf("  (%.2f%%)", 100.0 * dtot / t->words);
    printf("\n");
    if (!dtot)
      printf("    The two biases agree everywhere, so level 5 adds nothing and\n"
             "    this Strategy cannot support Left/Right modelling on this\n"
             "    corpus. Use a Strategy with multi-term families.\n");
    else
      printf("    Level 5 subdivides %ld of %ld level-4 nodes, so the bias is\n"
             "    load-bearing in %.1f%% of the tree and ignorable elsewhere.\n",
             nodes5, nodes4, nodes4 ? 100.0 * nodes5 / nodes4 : 0.0);
  }

  if (emit) {
    FILE *o = fopen(emit, "w");
    if (!o) { free(t); fprintf(stderr, "follow: cannot write %s\n", emit); return 2; }
    fprintf(o, "{\n  \"source\": \"%s\",\n  \"strategy\": \"%s\",\n"
               "  \"words\": %ld,\n  \"tree\": {\n", path,
            c->strat[sel].id.name, t->words);
    for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) {
      fprintf(o, "    \"%s\": {\n", c->meta[mi].name);
      for (fi = 0; fi < c->flows.count; ++fi) {
        fprintf(o, "      \"%s\": {", c->flows.flow[fi].name);
        for (xi = 0; xi < CANOR_DEMO_AXIS_N; ++xi)
          fprintf(o, "%s\"%s\": %ld", xi ? ", " : " ", c->mix[xi].name,
                  t->mix[mi][fi][xi]);
        fprintf(o, " }%s\n", fi + 1 < c->flows.count ? "," : "");
      }
      fprintf(o, "    }%s\n", mi + 1 < CANOR_DEMO_AXIS_N ? "," : "");
    }
    fprintf(o, "  }\n}\n");
    fclose(o);
    printf("\n  wrote %s\n", emit);
    canor_demo_prov_write_sidecar(emit);
  }

  free(t);
  return 0;
}
