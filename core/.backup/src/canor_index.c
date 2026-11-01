/* Host-side SemanticTree, Gap insertion and 1-bit ANN index. See canor_index.h. */

#include "canor_index.h"

#include <string.h>

/* Rejection-sampling retries per sampled bit position before a duplicate is
 * accepted (it only narrows the key; it cannot make the index wrong). */
#define ANN_DRAW_RETRIES 64

/* --- shared string hash ---------------------------------------------------- */

static uint32_t hash_bytes(const char *s, size_t n) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < n; ++i) { h ^= (unsigned char)s[i]; h *= 16777619u; }
  return h;
}

/* --- SemanticTree ---------------------------------------------------------- */

static int32_t tree_find(const canor_tree *t, const char *s, size_t len) {
  uint32_t h = hash_bytes(s, len) & t->mask;
  while (t->slot[h] >= 0) {
    const char *w = t->words[t->slot[h]];
    if (strlen(w) == len && memcmp(w, s, len) == 0) return t->slot[h];
    h = (h + 1) & t->mask;
  }
  return -1;
}

canor_status canor_tree_build(canor_tree *t, const char *const *words, uint32_t n) {
  if (!t || !words || n == 0) return CANOR_ERR_ARGS;
  if (n > CANOR_TREE_WORDS_MAX) return CANOR_ERR_FULL;   /* refused, not truncated */
  uint32_t cap = 1;
  while (cap < n * 2u) cap <<= 1;      /* load factor <= 0.5 */
  if (cap > CANOR_TREE_SLOTS_MAX) return CANOR_ERR_FULL;
  t->words = words;
  t->n = n;
  t->roots = 0;
  t->mask = cap - 1;
  for (uint32_t i = 0; i < cap; ++i) t->slot[i] = -1;

  for (uint32_t i = 0; i < n; ++i) {
    const char *w = words[i];
    uint32_t h = hash_bytes(w, strlen(w)) & t->mask;
    while (t->slot[h] >= 0) h = (h + 1) & t->mask;
    t->slot[h] = (int32_t)i;
  }

  /* The parent is the LONGEST proper prefix that is itself a word, so the
   * search runs downward from len-1 and stops at the first hit. Longest rather
   * than shortest because a chain of nested stems should be a chain, not a
   * star: aahing -> aah -> aa -> a, not aahing -> a directly. */
  for (uint32_t i = 0; i < n; ++i) {
    const char *w = words[i];
    size_t len = strlen(w);
    t->parent[i] = -1;
    for (size_t k = len - 1; k > 0; --k) {
      int32_t p = tree_find(t, w, k);
      if (p >= 0) { t->parent[i] = p; break; }
    }
    if (t->parent[i] < 0) ++t->roots;
  }

  /* Depth by memoized walk. Iterative, and capped: a cycle is impossible here
   * because a parent is strictly shorter, but the cap costs nothing and turns
   * a corrupted table into a bounded wrong answer instead of a hang. */
  for (uint32_t i = 0; i < n; ++i) {
    uint32_t d = 0;
    int32_t cur = t->parent[i];
    while (cur >= 0 && d < CANOR_TREE_DEPTH_MAX) { ++d; cur = t->parent[cur]; }
    t->depth[i] = d;
  }
  return CANOR_OK;
}

int32_t canor_tree_parent(const canor_tree *t, uint32_t w) {
  return (t && w < t->n) ? t->parent[w] : -1;
}

uint32_t canor_tree_depth(const canor_tree *t, uint32_t w) {
  return (t && w < t->n) ? t->depth[w] : 0;
}

uint32_t canor_tree_ancestors(const canor_tree *t, uint32_t w, uint32_t *out, uint32_t max) {
  if (!t || w >= t->n || !out) return 0;
  uint32_t n = 0;
  for (int32_t cur = t->parent[w]; cur >= 0 && n < max; cur = t->parent[cur])
    out[n++] = (uint32_t)cur;
  return n;
}

uint32_t canor_tree_region(const canor_tree *t, uint32_t w) {
  if (!t || w >= t->n) return w;
  uint32_t cur = w;
  int32_t p = t->parent[cur];
  while (p >= 0) { cur = (uint32_t)p; p = t->parent[cur]; }
  return cur;                          /* the root of this word's chain */
}

uint32_t canor_tree_root_count(const canor_tree *t) { return t ? t->roots : 0; }

size_t canor_tree_gapped_rgb(const canor_tree *t, uint32_t w, char *out, size_t cap) {
  if (!t || w >= t->n || !out || cap < 2) return 0;
  const char *word = t->words[w];
  size_t len = strlen(word);

  /* Ancestor lengths are the boundaries. Collected into a bitmap over
   * positions so the emit loop stays a single pass and the order of the
   * ancestor walk cannot affect the result. */
  unsigned char boundary[CANOR_TREE_WORD_LEN_MAX];
  memset(boundary, 0, sizeof boundary);
  for (int32_t cur = t->parent[w]; cur >= 0; cur = t->parent[cur]) {
    size_t bl = strlen(t->words[cur]);
    if (bl < sizeof boundary) boundary[bl] = 1;
  }

  size_t j = 0;
  for (size_t i = 0; i < len; ++i) {
    if (boundary[i] && j + 1 < cap) out[j++] = 'G';
    if (j + 1 >= cap) return 0;
    /* The base encoding's own rule: vowels are Real, consonants are Base.
     * Derived here rather than read from the dictionary so a word absent from
     * it still encodes - and because depending on the file for the mask would
     * make Gap insertion unavailable exactly when a word is new. */
    char c = word[i];
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    int vowel = (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y');
    out[j++] = vowel ? 'R' : 'B';
  }
  out[j] = '\0';
  return j;
}

/* --- ANN index -------------------------------------------------------------
 * LSH by bit sampling. Each table draws `bits` positions from the Strategy's
 * stream and buckets a signature by those bits. Two signatures that agree
 * almost everywhere agree on a sampled subset with high probability, so they
 * land together in at least one table; that is the whole mechanism.
 * =========================================================================== */

static uint32_t bucket_of(const canor_ann *a, const HYPERVECTOR *h, uint32_t table) {
  uint32_t key = 0;
  const uint16_t *p = a->pos + (size_t)table * a->bits;
  for (uint32_t b = 0; b < a->bits; ++b) {
    uint32_t bit = p[b];
    uint64_t w = h->w[bit >> 6];
    key = (key << 1) | (uint32_t)((w >> (bit & 63)) & 1u);
  }
  return key;
}

canor_status canor_ann_build(canor_ann *a, const HYPERVECTOR *sigs, uint32_t n,
                             const STRATEGY *s, uint32_t tables, uint32_t bits) {
  if (!a || !sigs || n == 0 || tables == 0 || bits == 0) return CANOR_ERR_ARGS;
  if (tables > CANOR_ANN_TABLES_MAX || bits > CANOR_ANN_BITS_MAX || n > CANOR_ANN_N_MAX)
    return CANOR_ERR_FULL;
  a->tables = tables; a->bits = bits; a->n = n;

  size_t buckets = (size_t)1u << bits;

  /* EVERY random choice comes from the Strategy's stream. No clock, no
   * rand(), no address-derived entropy - so the same Strategy builds the same
   * index, bit for bit, on any host. Rejection-sampled to avoid repeating a
   * position within a table, which would silently reduce the effective key
   * width and make recall worse for no visible reason. */
  for (uint32_t t = 0; t < tables; ++t) {
    for (uint32_t b = 0; b < bits; ++b) {
      uint32_t tries = 0, p;
      for (;;) {
        p = (uint32_t)(canor_rand(s, 0xA55EEDull + t, b * 97u + tries) % CANOR_HV_BITS);
        bool dup = false;
        for (uint32_t k = 0; k < b; ++k)
          if (a->pos[(size_t)t * bits + k] == (uint16_t)p) { dup = true; break; }
        if (!dup || ++tries > ANN_DRAW_RETRIES) break;
      }
      a->pos[(size_t)t * bits + b] = (uint16_t)p;
    }
  }

  for (size_t i = 0; i < (size_t)tables * buckets; ++i) a->head[i] = -1;

  /* Inserted in DESCENDING index order so each chain ends up ascending. That
   * makes the candidate walk deterministic and makes ties resolve to the lower
   * index without a sort. */
  for (uint32_t t = 0; t < tables; ++t) {
    for (uint32_t i = n; i-- > 0; ) {
      uint32_t key = bucket_of(a, &sigs[i], t);
      size_t slot = (size_t)t * buckets + key;
      a->next[(size_t)t * n + i] = a->head[slot];
      a->head[slot] = (int32_t)i;
    }
  }
  return CANOR_OK;
}

/* Keeps the k best by similarity, ties to the lower index. Insertion into a
 * small array beats a heap at these sizes and keeps the order defined. */
static void topk_push(uint32_t *out, int32_t *score, uint32_t *count, uint32_t k,
                      uint32_t idx, int32_t sc) {
  uint32_t n = *count;
  if (n < k) {
    uint32_t i = n;
    while (i > 0 && (score[i - 1] < sc || (score[i - 1] == sc && out[i - 1] > idx))) {
      out[i] = out[i - 1]; score[i] = score[i - 1]; --i;
    }
    out[i] = idx; score[i] = sc;
    *count = n + 1;
    return;
  }
  if (sc < score[k - 1] || (sc == score[k - 1] && idx > out[k - 1])) return;
  uint32_t i = k - 1;
  while (i > 0 && (score[i - 1] < sc || (score[i - 1] == sc && out[i - 1] > idx))) {
    out[i] = out[i - 1]; score[i] = score[i - 1]; --i;
  }
  out[i] = idx; score[i] = sc;
}

uint32_t canor_ann_query(const canor_ann *a, const HYPERVECTOR *sigs,
                      const HYPERVECTOR *query, uint32_t k,
                      uint32_t *out, int32_t *score, uint32_t *probed) {
  if (!a || !sigs || !query || !out || !score || k == 0) return 0;
  /* A candidate found twice is one candidate: a fixed bitset over the index
   * capacity, on the stack (CANOR_ANN_N_MAX / 8 bytes). */
  uint64_t seen[(CANOR_ANN_N_MAX + 63) / 64];
  memset(seen, 0, sizeof seen);
  size_t buckets = (size_t)1u << a->bits;
  uint32_t count = 0, examined = 0;

  for (uint32_t t = 0; t < a->tables; ++t) {
    uint32_t key = bucket_of(a, query, t);
    for (int32_t i = a->head[(size_t)t * buckets + key]; i >= 0;
         i = a->next[(size_t)t * a->n + i]) {
      if ((seen[i >> 6] >> (i & 63)) & 1u) continue;
      seen[i >> 6] |= (uint64_t)1u << (i & 63);
      ++examined;
      topk_push(out, score, &count, k, (uint32_t)i, canor_hv_similarity(query, &sigs[i]));
    }
  }
  if (probed) *probed = examined;
  return count;
}

uint32_t canor_ann_query_exact(const HYPERVECTOR *sigs, uint32_t n,
                            const HYPERVECTOR *query, uint32_t k,
                            uint32_t *out, int32_t *score) {
  if (!sigs || !query || !out || !score || k == 0) return 0;
  uint32_t count = 0;
  for (uint32_t i = 0; i < n; ++i)
    topk_push(out, score, &count, k, i, canor_hv_similarity(query, &sigs[i]));
  return count;
}
