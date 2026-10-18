#ifndef CANOR_INDEX_H
#define CANOR_INDEX_H

/* Host-side indices over the deterministic core: a SemanticTree, Gap insertion
 * driven by it, and an approximate-nearest-neighbour index over 1-bit
 * signatures.
 *
 * NOT PART OF THE SBC CORE, and not synced to the Prolog or Rust layers. The
 * core's Dictionary, WordFlowMap and FlowStore are linear-scan structures with
 * no hashing and no allocation beyond their arrays, which is the right shape
 * for WHITEPAPER section 19's embedded target. Everything here allocates and
 * hashes. Keeping it in a separate module means the substrate stays inspectable
 * and this stays optional.
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

typedef struct canor_tree canor_tree;

/* Words are borrowed, not copied: the caller owns the strings and must outlive
 * the tree. A dictionary of this size copied twice is 22 MB for nothing. */
canor_tree *canor_tree_build(const char *const *words, uint32_t n);
void canor_tree_free(canor_tree *t);

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

typedef struct canor_ann canor_ann;

/* tables: independent hash tables (more tables, better recall, more memory).
 * bits: bit positions sampled per table; the bucket count is 2^bits.
 *
 * Both bit selection and probe order derive from `s`, so the index is a pure
 * function of (Strategy, contents). Two builds with the same Strategy are
 * byte-identical structures. */
canor_ann *canor_ann_build(const HYPERVECTOR *sigs, uint32_t n,
                     const STRATEGY *s, uint32_t tables, uint32_t bits);
void canor_ann_free(canor_ann *a);

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
