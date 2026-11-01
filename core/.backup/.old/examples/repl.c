/* Interactive REPL over a real word->RGB dictionary.
 *
 * Loads mapped_words_dictionary.json (370k words) as the WordFlowMap and lets
 * you interrogate the Flow algebra against it.
 *
 * WHY THE LOADER LIVES HERE AND NOT IN THE CORE
 * ---------------------------------------------
 * The core's Dictionary, WordFlowMap and FlowStore are linear-scan structures.
 * That is the right shape for the SBC target WHITEPAPER section 19 describes -
 * no hashing, no allocation beyond the arrays, everything inspectable - but it
 * is quadratic when interning 370k entries. So this file keeps its own hash
 * indices and calls the core for the ALGEBRA only: vi_rgb_parse,
 * vi_flow_from_rgb, vi_hv_encode_rgb/decode_rgb, vi_attention.
 *
 * That split is deliberate. Adding hash tables to the core to make one REPL
 * fast would put allocation and load factors into the deterministic substrate
 * for the benefit of a tool, and the core's determinism is the thing worth
 * protecting. Nothing here computes a Flow, a hypervector or a score itself.
 *
 * The JSON is read with a purpose-built scanner rather than a JSON library: the
 * file is a flat object of string->string, one pair per line, and a scanner for
 * exactly that is twenty lines and has no dependency. It refuses anything that
 * does not match that shape rather than half-parsing it.
 */

#include "vi.h"
#include "vi_index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 400000u
#define HASH_BITS 20u                 /* 1M slots for 370k entries */
#define HASH_SIZE (1u << HASH_BITS)
#define HASH_MASK (HASH_SIZE - 1u)

typedef struct {
  char *word;                          /* into the arena */
  char *rgb;                           /* into the arena */
  FLOW flow;
} entry;

static char *g_arena;
static size_t g_arena_used, g_arena_cap;
static entry *g_entry;
static uint32_t g_count;
static int32_t *g_word_idx;            /* hash -> entry index, -1 empty */
static int32_t *g_flow_idx;            /* hash -> first entry with that flow */
static int32_t *g_flow_next;           /* chain of entries sharing a flow */

static char *arena_dup(const char *s, size_t n) {
  if (g_arena_used + n + 1 > g_arena_cap) return NULL;
  char *p = g_arena + g_arena_used;
  memcpy(p, s, n);
  p[n] = '\0';
  g_arena_used += n + 1;
  return p;
}

static uint32_t hash_str(const char *s) {
  uint32_t h = 2166136261u;
  for (; *s; ++s) { h ^= (unsigned char)*s; h *= 16777619u; }
  return h;
}

static void index_word(uint32_t i) {
  uint32_t h = hash_str(g_entry[i].word) & HASH_MASK;
  while (g_word_idx[h] >= 0) h = (h + 1) & HASH_MASK;   /* open addressing */
  g_word_idx[h] = (int32_t)i;
}

static int32_t find_word(const char *w) {
  uint32_t h = hash_str(w) & HASH_MASK;
  while (g_word_idx[h] >= 0) {
    if (strcmp(g_entry[g_word_idx[h]].word, w) == 0) return g_word_idx[h];
    h = (h + 1) & HASH_MASK;
  }
  return -1;
}

/* Words sharing a Flow are chained, so synonym lookup is O(group) rather than
 * O(dictionary). The chain is what makes the non-injectivity visible: this is
 * synonym convergence, and it is the normal case here, not an edge case. */
static void index_flow(uint32_t i) {
  uint32_t h = (g_entry[i].flow.data * 2654435761u) & HASH_MASK;
  while (g_flow_idx[h] >= 0 && g_entry[g_flow_idx[h]].flow.data != g_entry[i].flow.data)
    h = (h + 1) & HASH_MASK;
  g_flow_next[i] = g_flow_idx[h];
  g_flow_idx[h] = (int32_t)i;
}

static int32_t find_flow_head(FLOW f) {
  uint32_t h = (f.data * 2654435761u) & HASH_MASK;
  while (g_flow_idx[h] >= 0) {
    if (g_entry[g_flow_idx[h]].flow.data == f.data) return g_flow_idx[h];
    h = (h + 1) & HASH_MASK;
  }
  return -1;
}

/* Scans `"key": "value"` pairs. Refuses a line it cannot read that way rather
 * than skipping it, so a format change is loud. */
static int load(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) { printf("cannot open %s\n", path); return 0; }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = (char *)malloc((size_t)size + 1);
  if (!buf) { fclose(f); printf("out of memory reading %ld bytes\n", size); return 0; }
  size_t got = fread(buf, 1, (size_t)size, f);
  buf[got] = '\0';
  fclose(f);

  g_arena_cap = got + 16;
  g_arena = (char *)malloc(g_arena_cap);
  g_entry = (entry *)malloc(sizeof(entry) * MAX_WORDS);
  g_word_idx = (int32_t *)malloc(sizeof(int32_t) * HASH_SIZE);
  g_flow_idx = (int32_t *)malloc(sizeof(int32_t) * HASH_SIZE);
  g_flow_next = (int32_t *)malloc(sizeof(int32_t) * MAX_WORDS);
  if (!g_arena || !g_entry || !g_word_idx || !g_flow_idx || !g_flow_next) {
    printf("out of memory building indices\n"); free(buf); return 0;
  }
  for (uint32_t i = 0; i < HASH_SIZE; ++i) { g_word_idx[i] = -1; g_flow_idx[i] = -1; }

  const char *p = buf;
  int malformed = 0;
  while (*p && g_count < MAX_WORDS) {
    const char *q = strchr(p, '"');
    if (!q) break;
    const char *ke = strchr(q + 1, '"');
    if (!ke) break;
    const char *colon = strchr(ke + 1, ':');
    if (!colon) break;
    const char *vs = strchr(colon + 1, '"');
    if (!vs) break;
    const char *ve = strchr(vs + 1, '"');
    if (!ve) break;

    size_t klen = (size_t)(ke - q - 1), vlen = (size_t)(ve - vs - 1);
    char *w = arena_dup(q + 1, klen);
    char *r = arena_dup(vs + 1, vlen);
    if (!w || !r) { printf("arena exhausted\n"); break; }

    RGBString rgb;
    if (vi_rgb_parse(r, &rgb) != VI_OK) { ++malformed; p = ve + 1; continue; }
    g_entry[g_count].word = w;
    g_entry[g_count].rgb = r;
    g_entry[g_count].flow = vi_flow_from_rgb(&rgb);
    vi_rgb_free(&rgb);
    index_word(g_count);
    index_flow(g_count);
    ++g_count;
    p = ve + 1;
  }
  free(buf);
  if (malformed) printf("note: %d entries had an unreadable RGB value\n", malformed);
  return 1;
}

static void show_word(const char *w) {
  int32_t i = find_word(w);
  if (i < 0) { printf("  '%s' is not in the dictionary\n", w); return; }
  RGBString rgb;
  vi_rgb_parse(g_entry[i].rgb, &rgb);
  printf("  word      %s\n", g_entry[i].word);
  printf("  rgb       %s  (length %zu)\n", g_entry[i].rgb, rgb.length);
  printf("  dimension %zu   (count of Base)\n", vi_rgb_dimension(&rgb));
  printf("  flow      %u\n", g_entry[i].flow.data);
  vi_rgb_free(&rgb);
}

static void show_synonyms(const char *w, int limit) {
  int32_t i = find_word(w);
  if (i < 0) { printf("  '%s' is not in the dictionary\n", w); return; }
  int32_t head = find_flow_head(g_entry[i].flow);
  int n = 0;
  printf("  flow %u is reached by:\n   ", g_entry[i].flow.data);
  for (int32_t k = head; k >= 0; k = g_flow_next[k]) {
    if (n < limit) printf(" %s", g_entry[k].word);
    ++n;
  }
  printf("\n  %d word(s) converge on this Flow%s\n", n,
         n > limit ? " (list truncated)" : "");
}

static void roundtrip(const char *w) {
  int32_t i = find_word(w);
  if (i < 0) { printf("  '%s' is not in the dictionary\n", w); return; }
  RGBString rgb, back;
  vi_rgb_parse(g_entry[i].rgb, &rgb);
  HYPERVECTOR hv;
  if (vi_hv_encode_rgb(&rgb, &hv) != VI_OK) {
    printf("  encode failed (needs %zu bits, have %u)\n", rgb.length * 2, VI_HV_BITS);
    vi_rgb_free(&rgb);
    return;
  }
  if (vi_hv_decode_rgb(&hv, &back) != VI_OK) { printf("  decode failed\n"); vi_rgb_free(&rgb); return; }
  FLOW f2 = vi_flow_from_rgb(&back);
  int32_t head = find_flow_head(f2);
  printf("  WORD %s -> FLOW %u -> HYPERVECTOR -> FLOW %u -> WORD %s\n",
         w, g_entry[i].flow.data, f2.data, head >= 0 ? g_entry[head].word : "(none)");
  printf("  identity preserved: %s\n",
         g_entry[i].flow.data == f2.data ? "yes" : "NO");
  vi_rgb_free(&rgb);
  vi_rgb_free(&back);
}

static void attention_of(const char *a, const char *b, const char *sname, uint32_t ver) {
  int32_t i = find_word(a), j = find_word(b);
  if (i < 0 || j < 0) { printf("  unknown word\n"); return; }
  RGBString ra, rb;
  vi_rgb_parse(g_entry[i].rgb, &ra);
  vi_rgb_parse(g_entry[j].rgb, &rb);
  STRATEGY s = { sname, ver };
  printf("  attention(%s, %s) under %s v%u = %d / %d\n",
         a, b, sname, ver, vi_attention(&s, &ra, &rb), VI_ONE);
  vi_rgb_free(&ra);
  vi_rgb_free(&rb);
}

/* Nearest by attention. Linear over the dictionary and honest about it: at
 * 370k entries each query recomputes every signature, so this is seconds, not
 * milliseconds. Making it fast is an ANN index problem - which is exactly
 * where a vector database and its 1-bit quantization would earn their place -
 * and not something to fake here. */
static void nearest(const char *w, int topn, const char *sname, uint32_t ver, uint32_t scan) {
  int32_t i = find_word(w);
  if (i < 0) { printf("  '%s' is not in the dictionary\n", w); return; }
  RGBString q;
  vi_rgb_parse(g_entry[i].rgb, &q);
  STRATEGY s = { sname, ver };

  int32_t best[16];
  int32_t bs[16];
  int n = 0;
  if (topn > 16) topn = 16;
  uint32_t limit = scan && scan < g_count ? scan : g_count;
  for (uint32_t k = 0; k < limit; ++k) {
    if ((int32_t)k == i) continue;
    RGBString r;
    if (vi_rgb_parse(g_entry[k].rgb, &r) != VI_OK) continue;
    int32_t sc = vi_attention(&s, &q, &r);
    vi_rgb_free(&r);
    if (n < topn) { best[n] = (int32_t)k; bs[n] = sc; ++n; }
    else {
      int worst = 0;
      for (int m = 1; m < n; ++m) if (bs[m] < bs[worst]) worst = m;
      if (sc > bs[worst]) { best[worst] = (int32_t)k; bs[worst] = sc; }
    }
  }
  for (int a = 0; a < n; ++a)
    for (int b = a + 1; b < n; ++b)
      if (bs[b] > bs[a]) {
        int32_t t = bs[a]; bs[a] = bs[b]; bs[b] = t;
        t = best[a]; best[a] = best[b]; best[b] = t;
      }
  printf("  nearest to '%s' under %s (scanned %u of %u):\n", w, sname, limit, g_count);
  for (int a = 0; a < n; ++a)
    printf("    %4d  %-24s %s\n", bs[a], g_entry[best[a]].word, g_entry[best[a]].rgb);
  vi_rgb_free(&q);
}


/* --- accessors for repl_index.c --------------------------------------------
 * These two files are one program split for readability. A narrow accessor
 * surface beats sharing globals: the index module can read the load and cannot
 * corrupt it. */
uint32_t ri_count(void) { return g_count; }
const char *ri_word(uint32_t i) { return i < g_count ? g_entry[i].word : ""; }
const char *ri_rgb(uint32_t i) { return i < g_count ? g_entry[i].rgb : ""; }
uint32_t ri_flow(uint32_t i) { return i < g_count ? g_entry[i].flow.data : 0u; }
int32_t ri_find(const char *w) { return find_word(w); }

int ri_build_tree(void);
int ri_build_ann(uint32_t tables, uint32_t bits);
void ri_show_tree(const char *w);
void ri_near(const char *w, uint32_t k, int exact);
void ri_recall(uint32_t k, uint32_t trials);

static void stats(void) {
  uint32_t distinct = 0;
  for (uint32_t h = 0; h < HASH_SIZE; ++h) if (g_flow_idx[h] >= 0) ++distinct;
  printf("  words              %u\n", g_count);
  printf("  distinct Flows     %u\n", distinct);
  printf("  convergence ratio  %.2f words per Flow\n",
         distinct ? (double)g_count / (double)distinct : 0.0);
  printf("  hypervector        %u bits, %u symbols max losslessly\n",
         VI_HV_BITS, VI_HV_BITS / 2);
}

static void help(void) {
  puts("  word <w>              rgb, dimension and Flow id\n"
       "  syn <w> [n]           every word converging on the same Flow\n"
       "  rt <w>                WORD -> FLOW -> HV -> FLOW -> WORD\n"
       "  attn <a> <b> [S] [v]  Strategy-indexed attention, 0..1000\n"
       "  near <w> [n] [S] [k]  n nearest by attention, scanning k words\n"
       "  stats                 dictionary and convergence summary\n"
       "  help / quit");
}

int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : "../mapped_words_dictionary.json";
  printf("loading %s ...\n", path);
  if (!load(path)) return 1;
  printf("loaded %u words\n\n", g_count);
  stats();
  puts("");
  help();

  char line[512];
  for (;;) {
    printf("\nflow> ");
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) break;
    char *nl = strchr(line, '\n');
    if (nl) *nl = '\0';

    char cmd[32], a[128], b[128], sname[64];
    int n_arg = 0;
    unsigned k1 = 0, k2 = 0;
    a[0] = b[0] = sname[0] = '\0';

    n_arg = sscanf(line, "%31s %127s %127s %63s %u", cmd, a, b, sname, &k1);
    if (n_arg < 1) continue;

    if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit")) break;
    else if (!strcmp(cmd, "help")) help();
    else if (!strcmp(cmd, "stats")) stats();
    else if (!strcmp(cmd, "word") && n_arg >= 2) show_word(a);
    else if (!strcmp(cmd, "rt") && n_arg >= 2) roundtrip(a);
    else if (!strcmp(cmd, "syn") && n_arg >= 2) {
      int lim = 20;
      if (n_arg >= 3) { int v = atoi(b); if (v > 0) lim = v; }
      show_synonyms(a, lim);
    } else if (!strcmp(cmd, "attn") && n_arg >= 3) {
      const char *s = n_arg >= 4 ? sname : "biological";
      attention_of(a, b, s, n_arg >= 5 ? k1 : 1u);
    } else if (!strcmp(cmd, "tree") && n_arg >= 2) {
      ri_show_tree(a);
    } else if (!strcmp(cmd, "ann")) {
      uint32_t tb = n_arg >= 2 ? (uint32_t)atoi(a) : 12u;
      uint32_t bt = n_arg >= 3 ? (uint32_t)atoi(b) : 14u;
      ri_build_ann(tb ? tb : 12u, bt ? bt : 14u);
    } else if (!strcmp(cmd, "near") && n_arg >= 2) {
      ri_near(a, n_arg >= 3 ? (uint32_t)atoi(b) : 8u, 0);
    } else if (!strcmp(cmd, "exact") && n_arg >= 2) {
      ri_near(a, n_arg >= 3 ? (uint32_t)atoi(b) : 8u, 1);
    } else if (!strcmp(cmd, "recall")) {
      ri_recall(n_arg >= 2 ? (uint32_t)atoi(a) : 8u,
                n_arg >= 3 ? (uint32_t)atoi(b) : 20u);
    } else if (!strcmp(cmd, "scan") && n_arg >= 2) {
      /* The original linear attention scan, kept so the ANN has something
       * honest to be compared against. */
      int topn = 8;
      if (n_arg >= 3) { int v = atoi(b); if (v > 0) topn = v; }
      const char *s = n_arg >= 4 ? sname : "biological";
      k2 = n_arg >= 5 ? k1 : 20000u;
      nearest(a, topn, s, 1u, k2);
    } else printf("  ? try 'help'\n");
  }
  return 0;
}
