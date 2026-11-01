#ifndef CANOR_INDEX_H
#define CANOR_INDEX_H

/* Host-side indices over the deterministic core: a SemanticTree, Gap insertion
 * driven by it, and an approximate-nearest-neighbour index over 1-bit
 * signatures.
 *
 * NOT PART OF THE SBC CORE, and not synced to the Prolog or Rust layers. The
 * core's Dictionary, WordFlowMap and FlowStore are linear-scan structures with
 * no hashing, which is the right shape for WHITEPAPER section 19's embedded
 * target. Everything here hashes. Keeping it in a separate module means the
 * substrate stays inspectable and this stays optional.
 *
 * FIXED CAPACITY (AGENTS.md invariant 2). canor_tree and canor_ann are plain
 * structs the caller places (static, stack or its own arena); the module
 * allocates nothing. Building past a CANOR_TREE_* / CANOR_ANN_* capacity is
 * refused with CANOR_ERR_FULL. The defaults suit an embedded profile; the full
 * 370,100-word dictionary needs them raised at build time.
 *
 * WHAT IT IS FOR
 * --------------
 * Loading a real dictionary exposed a problem the small fixtures could not.
 * `mapped_words_dictionary.json` maps 370,100 words to RGB strings, and the
 * mapping is purely orthographic: R for a vowel, B for a consonant, and G never
 * appears at all - zero occurrences in 370,100 entries. So `dog` is `BRB`, and
 * so is every other consonant-vowel-consonant word: 914 of them share one Flow,
 * and the whole dictionary collapses to 14,425 distinct Flows, 25.66 words
 * apiece.
 *
 * That exercises the algebra at scale but measures spelling, not meaning, and
 * it leaves a third of the RGB alphabet untouched.
 *
 * GAP CARRIES THE HIERARCHY
 * -------------------------
 * VERIFIABLEINTELLIGENCE section 8 gives the SemanticTree - "a semantic tree
 * captures topological relations" - with the invariant that every mapped word
 * belongs to at least one semantic region. FLOWALGEBRA section 3 gives Gap as
 * "absence, transition, discontinuity, latent possibility".
 *
 * A morpheme boundary is a transition. So the tree is built from the dictionary
 * itself - a word's parent is the longest proper prefix of it that is also a
 * word - and a Gap is inserted at every ancestor boundary. `aahing` has the
 * ancestor chain a < aa < aah, so its boundaries are at 1, 2 and 3:
 *
 *     mask       R R B R B B          (aahing)
 *     with gaps  R G R G B G R B B
 *
 * The Gap pattern now encodes the word's PATH through the hierarchy, which is
 * exactly what the previous encoding threw away. Words sharing ancestors share
 * Gap positions; unrelated words of the same shape no longer collide.
 *
 * Worth being explicit about what this is and is not. It is MORPHOLOGICAL
 * hierarchy, derived from the dictionary with no outside knowledge. It is not
 * semantic in the sense of the Animal/Dog/Cat/Wolf example, which needs labels
 * this file does not have. FLOWALGEBRA section 43's open question Q6 asks
 * whether the Semantic Tree is primary or learned from Flow topology; this
 * derives one from the lexicon, which is a third answer and a declared choice
 * rather than a reading of either document.
 *
 * THE ANN INDEX
 * -------------
 * Locality-sensitive hashing over the 1-bit signatures, with every random
 * choice drawn from the Strategy's stream (canor_rand). Same Strategy, same bit
 * selections, same buckets, same candidate order, same answer - on any host,
 * on any run. That is what makes it a DETERMINISTIC vector index rather than
 * merely a reproducible-looking one, and it is the same requirement
 * FLOWALGEBRA section 27 places on everything else.
 *
 * The quantization is 1 bit per dimension, which is what the signatures
 * already are. A 4-bit representation would be four times larger here, not
 * smaller.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * SemanticTree - VERIFIABLEINTELLIGENCE section 8
 * =========================================================================== */

#ifndef CANOR_TREE_WORDS_MAX
#define CANOR_TREE_WORDS_MAX 4096
#endif
/* Open-addressing slots: a power of two at least twice the word capacity, so
 * the load factor stays <= 1/2. */
#ifndef CANOR_TREE_SLOTS_MAX
#define CANOR_TREE_SLOTS_MAX (2 * CANOR_TREE_WORDS_MAX)
#endif
#define CANOR_TREE_DEPTH_MAX 64        /* ancestor walk cap */
#define CANOR_TREE_WORD_LEN_MAX 256    /* Gap boundary bitmap width */

typedef struct {
  const char *const *words;     /* borrowed, see below */
  uint32_t n;
  int32_t parent[CANOR_TREE_WORDS_MAX];   /* -1 for a root */
  uint32_t depth[CANOR_TREE_WORDS_MAX];
  int32_t slot[CANOR_TREE_SLOTS_MAX];     /* open-addressed word -> index */
  uint32_t mask;                          /* slot count in use - 1 */
  uint32_t roots;
} canor_tree;

/* Words are borrowed, not copied: the caller owns the strings and must outlive
 * the tree. CANOR_ERR_ARGS for no words, CANOR_ERR_FULL for more than
 * CANOR_TREE_WORDS_MAX. */
canor_status canor_tree_build(canor_tree *t, const char *const *words, uint32_t n);

/* The parent of a word is the longest PROPER prefix of it that is also a word.
 * Returns -1 for a root word. */
int32_t canor_tree_parent(const canor_tree *t, uint32_t word);
/* Ancestor chain, root-most last. Returns how many were written. */
uint32_t canor_tree_ancestors(const canor_tree *t, uint32_t word, uint32_t *out, uint32_t max);
uint32_t canor_tree_depth(const canor_tree *t, uint32_t word);
/* The region a word belongs to: its root ancestor, or itself if it is a root.
 * This is section 8's "every mapped word shall belong to at least one semantic
 * region", made concrete. */
uint32_t canor_tree_region(const canor_tree *t, uint32_t word);
uint32_t canor_tree_root_count(const canor_tree *t);

/* Gap insertion. Writes an RGB string for `word` with the orthographic mask of
 * the base encoding and a Gap at every ancestor boundary. `out` receives a NUL
 * terminated string of R/G/B; returns its length, or 0 on failure. */
size_t canor_tree_gapped_rgb(const canor_tree *t, uint32_t word, char *out, size_t cap);

/* ===========================================================================
 * ANN index - LSH over 1-bit signatures, Strategy-seeded
 * =========================================================================== */

#ifndef CANOR_ANN_TABLES_MAX
#define CANOR_ANN_TABLES_MAX 8
#endif
#ifndef CANOR_ANN_BITS_MAX
#define CANOR_ANN_BITS_MAX 12          /* buckets per table: 2^bits */
#endif
#ifndef CANOR_ANN_N_MAX
#define CANOR_ANN_N_MAX 4096           /* signatures indexed */
#endif

typedef struct {
  uint32_t tables, bits, n;
  uint16_t pos[CANOR_ANN_TABLES_MAX * CANOR_ANN_BITS_MAX];        /* sampled bits */
  int32_t head[CANOR_ANN_TABLES_MAX << CANOR_ANN_BITS_MAX];       /* -1 empty */
  int32_t next[CANOR_ANN_TABLES_MAX * CANOR_ANN_N_MAX];           /* chain links */
} canor_ann;

/* tables: independent hash tables (more tables, better recall, more memory).
 * bits: bit positions sampled per table; the bucket count is 2^bits.
 *
 * Both bit selection and probe order derive from `s`, so the index is a pure
 * function of (Strategy, contents). Two builds with the same Strategy are
 * byte-identical structures. CANOR_ERR_ARGS for zero sizes, CANOR_ERR_FULL
 * when tables, bits or n exceed CANOR_ANN_TABLES_MAX / _BITS_MAX / _N_MAX. */
canor_status canor_ann_build(canor_ann *a, const HYPERVECTOR *sigs, uint32_t n,
                             const STRATEGY *s, uint32_t tables, uint32_t bits);

/* Top-k by exact Hamming distance over the candidates the tables surface.
 * Writes indices and their similarity in [0, CANOR_ONE], best first, and returns
 * how many were written. Ties break on the lower index, so the result is
 * single-valued.
 *
 * `probed` receives how many candidates were examined - the honest measure of
 * what the index saved, and the number to watch when tuning tables and bits. */
uint32_t canor_ann_query(const canor_ann *a, const HYPERVECTOR *sigs,
                      const HYPERVECTOR *query, uint32_t k,
                      uint32_t *out, int32_t *score, uint32_t *probed);

/* Exact linear scan, for measuring what the approximation costs. Recall is not
 * something to assume: the whole point of an ANN index is that it trades
 * accuracy for speed, and an index whose recall is never measured is an index
 * whose accuracy is unknown. */
uint32_t canor_ann_query_exact(const HYPERVECTOR *sigs, uint32_t n,
                            const HYPERVECTOR *query, uint32_t k,
                            uint32_t *out, int32_t *score);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_INDEX_H */
