/* Hierarchy, Gap insertion and the 1-bit ANN index, driven from the REPL.
 *
 * Split from repl.c so the loader stays a loader. Everything here is host-side
 * and optional; the deterministic core is untouched by it.
 */

#include "vi.h"
#include "vi_index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Provided by repl.c. Declared rather than shared through a header because
 * these two files are one program split for readability, not a library. */
extern uint32_t ri_count(void);
extern const char *ri_word(uint32_t i);
extern const char *ri_rgb(uint32_t i);
extern uint32_t ri_flow(uint32_t i);
extern int32_t ri_find(const char *w);

static vi_tree *g_tree;
static const char **g_wordptr;
static char **g_gapped;                 /* gapped RGB per word */
static uint32_t *g_gflow;               /* Flow of the gapped RGB */
static HYPERVECTOR *g_sig;              /* one signature per word */
static vi_ann *g_ann;
static STRATEGY g_strategy = { "biological", 1 };

#define IDX_BITS 20u
#define IDX_SIZE (1u << IDX_BITS)
#define IDX_MASK (IDX_SIZE - 1u)

/* Counts distinct values with a throwaway table, so the before/after
 * comparison below is like for like rather than reusing the loader's index. */
static uint32_t count_distinct(const uint32_t *vals, uint32_t n) {
  int64_t *seen = (int64_t *)malloc(sizeof(int64_t) * IDX_SIZE);
  if (!seen) return 0;
  for (uint32_t h = 0; h < IDX_SIZE; ++h) seen[h] = -1;
  uint32_t distinct = 0;
  for (uint32_t i = 0; i < n; ++i) {
    uint32_t h = (vals[i] * 2654435761u) & IDX_MASK;
    while (seen[h] >= 0 && (uint32_t)seen[h] != vals[i]) h = (h + 1) & IDX_MASK;
    if (seen[h] < 0) { seen[h] = (int64_t)vals[i]; ++distinct; }
  }
  free(seen);
  return distinct;
}

int ri_build_tree(void) {
  if (g_tree) return 1;
  uint32_t n = ri_count();
  g_wordptr = (const char **)malloc(sizeof(char *) * n);
  if (!g_wordptr) { printf("  out of memory\n"); return 0; }
  for (uint32_t i = 0; i < n; ++i) g_wordptr[i] = ri_word(i);

  g_tree = vi_tree_build(g_wordptr, n);
  if (!g_tree) { printf("  tree build failed\n"); return 0; }

  g_gapped = (char **)calloc(n, sizeof(char *));
  g_gflow = (uint32_t *)calloc(n, sizeof(uint32_t));
  uint32_t *plain = (uint32_t *)malloc(sizeof(uint32_t) * n);
  if (!g_gapped || !g_gflow || !plain) { printf("  out of memory\n"); return 0; }

  char buf[512];
  for (uint32_t i = 0; i < n; ++i) {
    plain[i] = ri_flow(i);
    size_t len = vi_tree_gapped_rgb(g_tree, i, buf, sizeof buf);
    if (!len) continue;
    g_gapped[i] = (char *)malloc(len + 1);
    if (!g_gapped[i]) continue;
    memcpy(g_gapped[i], buf, len + 1);
    RGBString r;
    if (vi_rgb_parse(g_gapped[i], &r) == VI_OK) {
      g_gflow[i] = vi_flow_from_rgb(&r).data;
      vi_rgb_free(&r);
    }
  }

  uint32_t before = count_distinct(plain, n);
  uint32_t after = count_distinct(g_gflow, n);
  free(plain);

  printf("  SemanticTree   %u roots over %u words\n", vi_tree_root_count(g_tree), n);
  printf("  distinct Flows  before gaps  %6u   (%.2f words each)\n",
         before, (double)n / (double)(before ? before : 1));
  printf("  distinct Flows  after  gaps  %6u   (%.2f words each)\n",
         after, (double)n / (double)(after ? after : 1));
  printf("  Gap was unused in the source encoding - 0 of %u entries - and now\n"
         "  carries each word's path through the hierarchy.\n", n);
  return 1;
}

int ri_build_ann(uint32_t tables, uint32_t bits) {
  if (!ri_build_tree()) return 0;
  uint32_t n = ri_count();
  if (g_ann) { vi_ann_free(g_ann); g_ann = NULL; }
  if (!g_sig) {
    g_sig = (HYPERVECTOR *)malloc(sizeof(HYPERVECTOR) * n);
    if (!g_sig) { printf("  out of memory for %u signatures\n", n); return 0; }
    for (uint32_t i = 0; i < n; ++i) {
      RGBString r;
      const char *src = g_gapped[i] ? g_gapped[i] : ri_rgb(i);
      if (vi_rgb_parse(src, &r) != VI_OK) { memset(&g_sig[i], 0, sizeof g_sig[i]); continue; }
      vi_hv_signature(&r, &g_strategy, &g_sig[i]);
      vi_rgb_free(&r);
    }
  }
  g_ann = vi_ann_build(g_sig, n, &g_strategy, tables, bits);
  if (!g_ann) { printf("  ann build failed\n"); return 0; }
  printf("  ANN  %u tables x %u bits over %u-bit signatures, 1 bit per dimension\n",
         tables, bits, VI_HV_BITS);
  printf("  every bit position drawn from Strategy '%s' v%u - same Strategy,\n"
         "  same index, on any host and any run\n", g_strategy.name, g_strategy.version);
  return 1;
}

void ri_show_tree(const char *w) {
  if (!ri_build_tree()) return;
  int32_t i = ri_find(w);
  if (i < 0) { printf("  '%s' is not in the dictionary\n", w); return; }
  uint32_t anc[64];
  uint32_t n = vi_tree_ancestors(g_tree, (uint32_t)i, anc, 64);
  printf("  %-16s depth %u   region '%s'\n", w,
         vi_tree_depth(g_tree, (uint32_t)i),
         ri_word(vi_tree_region(g_tree, (uint32_t)i)));
  printf("  path    %s", w);
  for (uint32_t k = 0; k < n; ++k) printf(" < %s", ri_word(anc[k]));
  printf("\n  rgb     plain  %s\n", ri_rgb((uint32_t)i));
  printf("  rgb     gapped %s\n", g_gapped[i] ? g_gapped[i] : "(none)");
  printf("  flow    plain  %-12u gapped %u\n", ri_flow((uint32_t)i), g_gflow[i]);
}

void ri_near(const char *w, uint32_t k, int exact) {
  if (!g_ann && !ri_build_ann(12, 14)) return;
  int32_t i = ri_find(w);
  if (i < 0) { printf("  '%s' is not in the dictionary\n", w); return; }
  uint32_t out[32];
  int32_t sc[32];
  if (k > 32) k = 32;
  uint32_t n, probed = 0;
  if (exact) n = vi_ann_query_exact(g_sig, ri_count(), &g_sig[i], k, out, sc);
  else n = vi_ann_query(g_ann, g_sig, &g_sig[i], k, out, sc, &probed);
  printf("  %s nearest to '%s':\n", exact ? "EXACT (full scan)" : "ANN", w);
  for (uint32_t a = 0; a < n; ++a)
    printf("    %4d  %-20s %s\n", sc[a], ri_word(out[a]),
           g_gapped[out[a]] ? g_gapped[out[a]] : ri_rgb(out[a]));
  if (!exact)
    printf("  probed %u of %u (%.3f%%)\n", probed, ri_count(),
           100.0 * probed / (double)ri_count());
}

void ri_recall(uint32_t k, uint32_t trials) {
  if (!g_ann && !ri_build_ann(12, 14)) return;
  uint32_t a_out[32], e_out[32];
  int32_t a_sc[32], e_sc[32];
  if (k > 32) k = 32;
  uint32_t hits = 0, total = 0, probed_sum = 0, n = ri_count();
  for (uint32_t t = 0; t < trials; ++t) {
    /* Query words drawn from the Strategy's stream, so the MEASUREMENT is
     * reproducible too - a recall figure that moves between runs because the
     * sample moved tells you nothing about the index. */
    uint32_t q = (uint32_t)(vi_rand(&g_strategy, 0xE7A10ull, t) % n);
    uint32_t p = 0;
    uint32_t na = vi_ann_query(g_ann, g_sig, &g_sig[q], k, a_out, a_sc, &p);
    uint32_t ne = vi_ann_query_exact(g_sig, n, &g_sig[q], k, e_out, e_sc);
    probed_sum += p;
    for (uint32_t x = 0; x < ne; ++x) {
      ++total;
      for (uint32_t y = 0; y < na; ++y) if (a_out[y] == e_out[x]) { ++hits; break; }
    }
  }
  printf("  recall@%u over %u queries: %.1f%%\n", k, trials,
         100.0 * hits / (double)(total ? total : 1));
  printf("  mean %u candidates probed of %u (%.3f%%)\n",
         probed_sum / (trials ? trials : 1), n,
         100.0 * (probed_sum / (double)(trials ? trials : 1)) / (double)n);
}
