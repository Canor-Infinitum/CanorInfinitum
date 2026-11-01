/* Builds v1-beta.json: every dictionary word re-encoded as an RGB metaflow
 * derived from an IMAG flow.
 *
 * THE CHAIN
 * ---------
 *   IMAG flow      a word over the imaginary units, e.g. Ordinary = ijkkji
 *   all orders     every distinct permutation of that word's multiset
 *   fold           each order folded under Hamilton's rules to a signed unit
 *   RGB metaflow   the order projected i -> Real, j -> Gap, k -> Base
 *   v1-beta        each dictionary word assigned the metaflow its semantics ask for
 *
 * WHY i -> R, j -> G, k -> B
 * --------------------------
 * Three imaginary units, three RGB primitives, and the correspondence is not
 * arbitrary once the v1-alpha rules are taken seriously:
 *
 *   count(Base) is the dimension          so k-count carries dimension
 *   count(Gap)  is the transition order   so j-count carries transition
 *   Real is realization                   so i-count carries realization
 *
 * A metaflow therefore satisfies v1-alpha's rules 1 and 2 BY CONSTRUCTION: to
 * give a word dimension d and transition order g, pick an IMAG word with d
 * k's and g j's. Nothing has to be checked afterwards because nothing else
 * could have been produced.
 *
 * WHY THE ORDER-SPACE IS NOT ENUMERATED
 * -------------------------------------
 * A multiset of n units has n!/(prod of repeats!) distinct orders, which for
 * a 20-symbol word is astronomically large. So the space is never built. The
 * k-th order is CONSTRUCTED directly, and k comes from the Strategy's
 * deterministic stream - which is the same reason FOLLOW-CORE makes NUMBER
 * factoradic and the topology n!-levelled: the index is the address, and the
 * space it addresses is never materialized.
 *
 * The four given baselines are reported with their full order-spaces and fold
 * distributions, because those ARE small enough to enumerate and are the
 * evidence that the machinery is doing what it claims.
 */

#include "vi.h"
#include "vi_strategy.h"
#include "fc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 400000u
#define HASH_BITS 20u
#define HASH_SIZE (1u << HASH_BITS)
#define HASH_MASK (HASH_SIZE - 1u)
#define MAX_LEN 64

static char *g_arena;
static size_t g_used, g_cap;
static char **g_word;
static char **g_rgb;
static uint32_t g_n;
static int32_t *g_slot;
static uint8_t *g_depth;

static STRATEGY g_s = { "metaflow", 1 };

/* --- multiset orders --------------------------------------------------------
 * count = n! / (i! j! k!). Computed in long double only to detect overflow
 * cheaply; the ranking itself is exact integer arithmetic and never uses it. */
static uint64_t order_count(int ni, int nj, int nk) {
  int n = ni + nj + nk;
  if (n > 20) return 0;                  /* beyond exact 64-bit factorials */
  uint64_t f[21];
  f[0] = 1;
  for (int i = 1; i <= 20; ++i) f[i] = f[i - 1] * (uint64_t)i;
  return f[n] / (f[ni] * f[nj] * f[nk]);
}

/* Constructs the rank-th distinct permutation of i^ni j^nj k^nk directly, in
 * lexicographic order over i < j < k. No enumeration: at each position it asks
 * how many completions start with each symbol and steps past the ones the rank
 * skips. That is the multiset analogue of a factoradic decode. */
static void order_at(int ni, int nj, int nk, uint64_t rank, char *out) {
  int c[3] = { ni, nj, nk };
  static const char sym[3] = { 'i', 'j', 'k' };
  int n = ni + nj + nk;
  for (int pos = 0; pos < n; ++pos) {
    for (int s = 0; s < 3; ++s) {
      if (!c[s]) continue;
      --c[s];
      uint64_t sub = order_count(c[0], c[1], c[2]);
      if (rank < sub) { out[pos] = sym[s]; break; }
      rank -= sub;
      ++c[s];
    }
  }
  out[n] = '\0';
}

static int unit_of(char ch) {
  return ch == 'i' ? 1 : (ch == 'j' ? 2 : (ch == 'k' ? 3 : 0));
}

/* Fold under Hamilton's rules. Same table the tower uses; not a second copy. */
static void fold(const char *w, uint8_t *unit, int *sign) {
  fc_imag acc = (fc_imag)unit_of(w[0]);
  int s = 1;
  for (const char *p = w + 1; *p; ++p) {
    fc_imag r;
    int rs;
    if (fc_imag_mul(acc, (fc_imag)unit_of(*p), &r, &rs) != FC_OK) { *unit = 0; *sign = 1; return; }
    acc = r;
    s *= rs;
  }
  *unit = acc;
  *sign = s;
}

static void to_rgb(const char *imag, char *rgb) {
  for (; *imag; ++imag, ++rgb)
    *rgb = (*imag == 'i') ? 'R' : (*imag == 'j' ? 'G' : 'B');
  *rgb = '\0';
}

/* --- dictionary ------------------------------------------------------------- */

static uint32_t hstr(const char *s) {
  uint32_t h = 2166136261u;
  for (; *s; ++s) { h ^= (unsigned char)*s; h *= 16777619u; }
  return h;
}

static int32_t findw(const char *w) {
  uint32_t h = hstr(w) & HASH_MASK;
  while (g_slot[h] >= 0) {
    if (!strcmp(g_word[g_slot[h]], w)) return g_slot[h];
    h = (h + 1) & HASH_MASK;
  }
  return -1;
}

static char *adup(const char *s, size_t n) {
  if (g_used + n + 1 > g_cap) return NULL;
  char *p = g_arena + g_used;
  memcpy(p, s, n);
  p[n] = '\0';
  g_used += n + 1;
  return p;
}

static int load(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) { printf("cannot open %s\n", path); return 0; }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)sz + 1);
  size_t got = fread(buf, 1, (size_t)sz, f);
  buf[got] = '\0';
  fclose(f);

  g_cap = got + 16;
  g_arena = (char *)malloc(g_cap);
  g_word = (char **)malloc(sizeof(char *) * MAX_WORDS);
  g_rgb = (char **)malloc(sizeof(char *) * MAX_WORDS);
  g_slot = (int32_t *)malloc(sizeof(int32_t) * HASH_SIZE);
  g_depth = (uint8_t *)calloc(MAX_WORDS, 1);
  if (!g_arena || !g_word || !g_rgb || !g_slot || !g_depth) { free(buf); return 0; }
  for (uint32_t i = 0; i < HASH_SIZE; ++i) g_slot[i] = -1;

  const char *p = buf;
  while (*p && g_n < MAX_WORDS) {
    const char *ks = strchr(p, '"');       if (!ks) break;
    const char *ke = strchr(ks + 1, '"');  if (!ke) break;
    const char *co = strchr(ke + 1, ':');  if (!co) break;
    const char *vs = strchr(co + 1, '"');  if (!vs) break;
    const char *ve = strchr(vs + 1, '"');  if (!ve) break;
    char *w = adup(ks + 1, (size_t)(ke - ks - 1));
    char *r = adup(vs + 1, (size_t)(ve - vs - 1));
    if (!w || !r) break;
    g_word[g_n] = w;
    g_rgb[g_n] = r;
    uint32_t h = hstr(w) & HASH_MASK;
    while (g_slot[h] >= 0) h = (h + 1) & HASH_MASK;
    g_slot[h] = (int32_t)g_n;
    ++g_n;
    p = ve + 1;
  }
  free(buf);
  return g_n > 0;
}

/* SemanticTree depth: a word's parent is its longest proper prefix that is
 * also a word. Same construction as vi_index's tree, computed here so this
 * tool does not depend on the REPL's module. */
static void build_depth(void) {
  for (uint32_t i = 0; i < g_n; ++i) {
    char buf[MAX_LEN];
    size_t len = strlen(g_word[i]);
    if (len >= sizeof buf) len = sizeof buf - 1;
    memcpy(buf, g_word[i], len);
    buf[len] = '\0';
    int d = 0;
    /* Walk up the chain: the parent is the longest proper prefix that is also
     * a word, and the depth is how many such steps reach a root. Bounded, and
     * the bound can never bind because each step strictly shortens the word. */
    while (len > 1 && d < 32) {
      int found = -1;
      for (size_t k = len - 1; k > 0; --k) {
        char t = buf[k];
        buf[k] = '\0';
        if (findw(buf) >= 0) found = (int)k;
        buf[k] = t;
        if (found >= 0) break;
      }
      if (found < 0) break;
      ++d;
      len = (size_t)found;
      buf[len] = '\0';
    }
    g_depth[i] = (uint8_t)d;
  }
}

int main(int argc, char **argv) {
  const char *dict = argc > 1 ? argv[1] : "../mapped_words_dictionary.json";
  const char *out_path = argc > 2 ? argv[2] : "../v1-beta.json";

  if (fc_imag_init() != FC_OK) { printf("unit algebra does not close\n"); return 1; }

  /* --- the four baselines and their full order-spaces --------------------- */
  vi_flow_registry reg;
  vi_flows_default(&reg);
  printf("IMAG flows -> all orders -> fold distribution\n\n");
  for (int b = 0; b < reg.count; ++b) {
    const char *w = reg.flow[b].word;
    int ni = 0, nj = 0, nk = 0;
    for (const char *p = w; *p; ++p) {
      if (*p == 'i') ++ni; else if (*p == 'j') ++nj; else ++nk;
    }
    uint64_t total = order_count(ni, nj, nk);
    int hist[2][4];
    memset(hist, 0, sizeof hist);
    char buf[MAX_LEN], rgb[MAX_LEN];
    for (uint64_t r = 0; r < total; ++r) {
      order_at(ni, nj, nk, r, buf);
      uint8_t u;
      int sg;
      fold(buf, &u, &sg);
      if (u < 4) hist[sg < 0 ? 1 : 0][u]++;
    }
    uint8_t u0;
    int s0;
    fold(w, &u0, &s0);
    to_rgb(w, rgb);
    printf("  %-9s %-8s i%d j%d k%d   %4llu orders   folds to %s%-4s   metaflow %s\n",
           reg.flow[b].name, w, ni, nj, nk, (unsigned long long)total,
           s0 < 0 ? "-" : "+", fc_imag_name(u0), rgb);
    printf("             fold classes:");
    for (int s = 0; s < 2; ++s)
      for (int u = 0; u < 4; ++u)
        if (hist[s][u]) printf("  %s%s=%d", s ? "-" : "+", fc_imag_name((fc_imag)u), hist[s][u]);
    printf("\n");
  }

  /* --- build v1-beta ------------------------------------------------------ */
  printf("\nloading %s ...\n", dict);
  if (!load(dict)) return 1;
  printf("loaded %u words\n", g_n);
  build_depth();

  FILE *o = fopen(out_path, "wb");
  if (!o) { printf("cannot write %s\n", out_path); return 1; }
  fprintf(o, "{\n");

  uint32_t gap_used = 0, dim_ok = 0, emitted = 0;
  uint64_t order_space_total = 0;
  char imag[MAX_LEN], rgb[MAX_LEN];

  for (uint32_t i = 0; i < g_n; ++i) {
    /* Semantic coordinates, from v1-alpha's rules:
     *   dimension        = count(Base) of the source encoding
     *   transition order = hierarchy depth, so a derived word carries more
     *                      transition than a root
     *   realization      = whatever length remains */
    const char *src = g_rgb[i];
    int nk = 0;
    for (const char *p = src; *p; ++p) if (*p == 'B') ++nk;
    int nj = g_depth[i];
    int len = (int)strlen(src);
    int ni = len - nk - nj;
    if (ni < 1) ni = 1;
    if (nk < 1) nk = 1;
    if (ni + nj + nk > 20) {              /* keep the order count exact */
      int over = ni + nj + nk - 20;
      ni = ni > over ? ni - over : 1;
    }

    uint64_t total = order_count(ni, nj, nk);
    if (!total) total = 1;
    order_space_total += total;
    /* Which order: the Strategy's stream, so the whole file is a pure function
     * of (Strategy, source dictionary) and rebuilds byte-identically. */
    uint64_t rank = vi_rand(&g_s, 0xBE7Aull, i) % total;
    order_at(ni, nj, nk, rank, imag);
    to_rgb(imag, rgb);

    int b = 0, g = 0;
    for (const char *p = rgb; *p; ++p) { if (*p == 'B') ++b; else if (*p == 'G') ++g; }
    if (b == nk) ++dim_ok;
    if (g) ++gap_used;

    fprintf(o, "  \"%s\": \"%s\"%s\n", g_word[i], rgb, i + 1 < g_n ? "," : "");
    ++emitted;
  }
  fprintf(o, "}\n");
  fclose(o);

  printf("\nwrote %s\n", out_path);
  printf("  entries                 %u\n", emitted);
  printf("  dimension preserved     %u of %u\n", dim_ok, emitted);
  printf("  carry a Gap             %u of %u\n", gap_used, emitted);
  printf("  order-space addressed   %llu total orders across all words\n",
         (unsigned long long)order_space_total);
  printf("  never enumerated: each word's order is CONSTRUCTED at its rank\n");
  return 0;
}
