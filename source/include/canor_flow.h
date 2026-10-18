#ifndef CANOR_FLOW_H
#define CANOR_FLOW_H

/* Canor Intelligence - pure C core.
 *
 * Sources: VERIFIABLEINTELLIGENCE.md (the library), FLOWALGEBRA.md (the
 * algebra), WHITEPAPER.md sections 19-20 (the implementation model and the
 * minimal conformance target). Section numbers below cite them.
 *
 * License: GNU AGPL v3.0-or-later, per the source documents.
 *
 * WHAT CHANGED FROM THE SKELETON, AND WHY
 * ---------------------------------------
 * The skeleton established the API surface and that surface is kept. Four
 * things in it contradicted the documents it implements, and each is corrected
 * here rather than left for a reader to trip over:
 *
 * 1. THE CONFORMANCE FUNCTION DID NOT ROUND-TRIP. It computed a hypervector,
 *    discarded it, and assigned `f2 = f1` with the comment "Skeleton
 *    conformance preserves Flow identity by design." WHITEPAPER section 20
 *    point 3 asks the system to "recover or preserve the Flow identity", and
 *    an assignment proves nothing about the encoding. It now decodes, and the
 *    recovered Flow is derived from the decoded content.
 *
 * 2. HYPERVECTORS WERE FLOATS. `float data[1024]`, cosine similarity in
 *    double, sqrt. FLOWALGEBRA section 27 forbids floating-point
 *    nondeterminism in verifiable mode, and WHITEPAPER section 19 specifies a
 *    "fixed-width integer substrate" with "packed ternary/quaternary values".
 *    Hypervectors are now packed 64-bit words and similarity is an integer in
 *    [0, CANOR_ONE]. Two hosts now agree bit for bit, which is what section 38's
 *    reproducibility theorem needs in order to be provable at all.
 *
 * 3. THE SIGNED-ZERO ENCODING CONTRADICTED THE SPEC. Both documents fix it:
 *        00 -> NegZero   01 -> NegOne   10 -> PosZero   11 -> PosOne
 *    so NegZero=0, NegOne=1, PosZero=2, PosOne=3. The skeleton had FLOWVALUE
 *    as {NegOne=0, PosZero=1, NegZero=2, PosOne=3} and a SECOND enum VALUE
 *    with a third ordering. The bit patterns are specification, not internal
 *    convenience, so they are pinned; the duplicate enum is gone, because one
 *    concept with two encodings is a defect waiting for a caller to mix them.
 *
 * 4. Dictionary and WordFlowMap had NO CAPACITY CHECK. `canor_dictionary_add`
 *    wrote at `dict->count` with nothing stopping it passing the allocation.
 *    Capacity is now stored and both refuse when full.
 *
 * A FIFTH CHANGE IS AN ADDITION, not a correction. FLOW is a 32-bit identifier
 * derived from content by hash, and a hash cannot be inverted - so
 * `encode(FLOW)` could never satisfy section 21's exact invariant. The lossless
 * path therefore runs over RGB strings, which are the actual Flow content, and
 * a FlowStore resolves an id back to its content. The old signature-style
 * encode is kept under a name that says what it is.
 */

#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Bits, not floats. A multiple of 64. */
#define CANOR_HV_BITS  1024u
#define CANOR_HV_WORDS (CANOR_HV_BITS / 64u)

/* Fixed-point one. Scores are exact integers in [0, CANOR_ONE]. */
#define CANOR_ONE 1000

#include "canor_core.h"

#define CANOR_ERR       CANOR_ERR_ARGS
#define CANOR_NOT_FOUND CANOR_ERR_NOT_FOUND
#define CANOR_FULL      CANOR_ERR_FULL


/* 2. Primitive Types */
typedef struct { uint32_t data; } WORD;
typedef struct { uint64_t data; } TOKEN;
typedef struct { uint32_t data; } SEED;

/* 3. RGB Primitives.
 *   Real  realization        Gap  absence or transition   Base  anchoring */
typedef enum {
    RGB_Real = 0,
    RGB_Gap  = 1,
    RGB_Base = 2
} RGBPrimitive;

typedef struct {
    size_t length;
    RGBPrimitive *primitives;
} RGBString;

/* 5. Flow. An identifier derived from content; see the FlowStore note above. */
typedef struct { uint32_t data; } FLOW;

typedef struct {
    RGBString rgb;
} FlowState;

/* 6. Dictionary */
typedef struct {
    WORD *words;
    char **labels;
    size_t count;
    size_t capacity;         /* added: the skeleton could overrun */
} Dictionary;

/* 7. Word-To-Flow Mapping. Many-to-one by design: several words may share one
 * Flow, which is what synonym convergence means. */
typedef struct {
    WORD word;
    FLOW flow;
} WordFlowEntry;

typedef struct {
    WordFlowEntry *entries;
    size_t length;
    size_t capacity;         /* added */
} WordFlowMap;

/* Resolves a FLOW id back to the RGB content it was derived from. Needed
 * because a hash is not invertible and section 21's exact invariant is over
 * content, not over identifiers. */
typedef struct {
    FLOW *flows;
    RGBString *content;
    size_t count;
    size_t capacity;
} FlowStore;

/* 8. Semantic Tree placeholder - unchanged, still a placeholder. */
typedef struct { uint32_t data; } SemanticTree;

/* 9. Flow Topology Space */
typedef struct {
    FLOW *flows;
    size_t count;
} FlowTopology;

/* 10. Hypervectors. Packed bits, per WHITEPAPER section 19. */
typedef struct {
    uint64_t w[CANOR_HV_WORDS];
    uint16_t length;         /* RGB symbols encoded, for the lossless form */
} HYPERVECTOR;

/* 11. Seed System */
typedef RGBString SeedPattern;

/* 12/14. Signed ternary logic. ONE enum, and the bit patterns are the
 * specification's:  00 NegZero | 01 NegOne | 10 PosZero | 11 PosOne. */
typedef enum {
    FV_NegZero = 0,
    FV_NegOne  = 1,
    FV_PosZero = 2,
    FV_PosOne  = 3
} FLOWVALUE;

/* A Strategy is part of formal system state, not a prompt (library section 4).
 * Versioned, so two Strategies differing only in version are different states
 * and produce different deterministic streams. */
typedef struct { const char *name; uint32_t version; } STRATEGY;

/* 15. Biased Resolution. Left and Right are required by the library; Neutral
 * and Strategy come from FLOWALGEBRA section 11. An implementation must
 * DECLARE its mode, so there is no default. */
typedef enum {
    LeftBiased = 0,
    RightBiased = 1,
    NeutralBiased = 2,
    StrategyBiased = 3
} BiasMode;

/* 17. VIState */
typedef struct {
    STRATEGY strategy;
    Dictionary dictionary;
    SeedPattern *seeds;
    size_t seed_count;
    FLOW *flows;
    size_t flow_count;
    WordFlowMap mapping;
    FlowTopology topology;
} VIState;

typedef void (*LearnFunction)(VIState *state, const char *training_data);

/* --- deterministic utility ------------------------------------------------ */

uint32_t canor_hash_bytes(const uint8_t *bytes, size_t length, uint32_t seed);
/* Pure function of (strategy, seed, index). No clock, no entropy, no state -
 * FLOWALGEBRA section 27's requirement, and the reason it takes every input
 * explicitly instead of keeping a generator. */
uint64_t canor_rand(const STRATEGY *s, uint64_t seed, uint64_t index);

WORD canor_word_from_cstr(const char *s);
FLOW canor_flow_from_rgb(const RGBString *rgb);
int canor_rgb_parse(const char *s, RGBString *out);
int canor_rgb_copy(const RGBString *src, RGBString *dst);
int canor_rgb_equal(const RGBString *a, const RGBString *b);
void canor_rgb_free(RGBString *rgb);
/* dimension(s) = count(Base, s), per FLOWALGEBRA section 4. */
size_t canor_rgb_dimension(const RGBString *rgb);

/* --- dictionary, mapping, flow store -------------------------------------- */

int canor_dictionary_init(Dictionary *dict, size_t capacity);
void canor_dictionary_free(Dictionary *dict);
int canor_dictionary_add(Dictionary *dict, const char *label, WORD *out_word);
const char *canor_dictionary_label(const Dictionary *dict, WORD word);

int canor_wordflow_init(WordFlowMap *map, size_t capacity);
void canor_wordflow_free(WordFlowMap *map);
int canor_wordflow_put(WordFlowMap *map, WORD word, FLOW flow);
int canor_wordflow_get(const WordFlowMap *map, WORD word, FLOW *out);
int canor_wordflow_reverse_first(const WordFlowMap *map, FLOW flow, WORD *out);
/* All words mapping to one Flow. Projection is one-to-many because M is
 * non-injective; reverse_first alone hides that. */
size_t canor_wordflow_reverse_all(const WordFlowMap *map, FLOW flow,
                               WORD *out, size_t max);

int canor_flowstore_init(FlowStore *fs, size_t capacity);
void canor_flowstore_free(FlowStore *fs);
/* Interns content and returns its FLOW. Re-adding identical content returns
 * the same FLOW, which is what makes identity comparison an id comparison. */
int canor_flowstore_add(FlowStore *fs, const RGBString *rgb, FLOW *out);
const RGBString *canor_flowstore_content(const FlowStore *fs, FLOW flow);

/* --- hypervectors --------------------------------------------------------- */

/* LOSSLESS. Two bits per RGB symbol. decode is an exact inverse, so
 * FLOWALGEBRA section 21's exact invariant holds and the conformance target is
 * provable rather than asserted. Fails if the string exceeds CANOR_HV_BITS/2. */
int canor_hv_encode_rgb(const RGBString *rgb, HYPERVECTOR *out);
int canor_hv_decode_rgb(const HYPERVECTOR *hv, RGBString *out);

/* LOSSY, and named so. A position-bound bundled signature over the content,
 * for similarity only. This is the classic holographic construction, and it
 * cannot be inverted - which is exactly why the exact path above exists
 * separately. Section 21 permits the split. */
int canor_hv_signature(const RGBString *rgb, const STRATEGY *s, HYPERVECTOR *out);
/* Kept from the skeleton: a signature derived from a FLOW identifier alone.
 * Cannot round-trip; use canor_hv_encode_rgb when identity matters. */
int canor_hv_encode(FLOW flow, HYPERVECTOR *out);

/* Integer similarity in [0, CANOR_ONE]. CANOR_ONE means identical. */
int32_t canor_hv_similarity(const HYPERVECTOR *a, const HYPERVECTOR *b);
int32_t canor_hv_hamming(const HYPERVECTOR *a, const HYPERVECTOR *b);

/* --- flow attention (library section 16, algebra sections 23-24) ---------- */

/* Integer, bounded, and Strategy-indexed: the same pair may attend differently
 * under different Strategies, which is the point of section 23. */
int32_t canor_attention(const STRATEGY *s, const RGBString *a, const RGBString *b);
int32_t canor_attention_default(FLOW a, FLOW b);
int canor_q_default(FLOW flow, HYPERVECTOR *out);
int canor_k_default(FLOW flow, HYPERVECTOR *out);
int canor_v_default(FLOW flow, HYPERVECTOR *out);

/* --- conformance (WHITEPAPER section 20) ---------------------------------- */

/*     WORD -> FLOW -> HYPERVECTOR -> FLOW -> WORD
 *
 * `flow2` is derived from the DECODED hypervector, not copied from `flow1`, so
 * `*flow1 == *flow2` is now evidence rather than an assignment. Requires the
 * FlowStore, because recovering content from an identifier is the step a hash
 * cannot do. */
int canor_conformance_word_to_flow_to_hv_to_flow_to_word(
    const Dictionary *dict,
    const WordFlowMap *map,
    const FlowStore *store,
    WORD input,
    WORD *output,
    FLOW *flow1,
    FLOW *flow2
);

/* --- signed-zero biased addition (library section 15) --------------------- */

/* The four required cancelling cases are fixed by the documents and are NOT
 * symmetric in the operands. Non-cancelling addition saturates: FLOWVALUE
 * carries no magnitude above one, so widening would leave the algebra.
 * StrategyBiased consults `s`; NeutralBiased is declared, not arbitrary. */
/* Cancelling resolution WITH CONTEXT. `ctx` identifies WHERE in a computation
 * the cancellation happened - which term of a family, and how many additions
 * had already been folded in. It matters because a cancelling sum has only two
 * operand configurations, (PosOne, NegOne) and (NegOne, PosOne), so any rule
 * that reads only the operands can produce at most four behaviours and all
 * four are already reachable by LeftBiased, RightBiased and NeutralBiased (and
 * Neutral's mirror). A fourth mode that duplicates the first three is not a
 * fourth mode. Position is the smallest piece of information that breaks the
 * tie, because Left/Right/Neutral are all position-INDEPENDENT and a
 * position-dependent rule therefore cannot coincide with any of them.
 *
 * canor_flowvalue_add is this with ctx = 0, so existing callers are unchanged. */
FLOWVALUE canor_flowvalue_add_ctx(BiasMode mode, const STRATEGY *s,
                               FLOWVALUE left, FLOWVALUE right, uint64_t ctx);
FLOWVALUE canor_flowvalue_add(BiasMode mode, const STRATEGY *s,
                           FLOWVALUE left, FLOWVALUE right);
int canor_flowvalue_numeric(FLOWVALUE v);          /* -1, 0, +1 */
int canor_flowvalue_eq_numeric(FLOWVALUE a, FLOWVALUE b);
int canor_flowvalue_eq_repr(FLOWVALUE a, FLOWVALUE b);

#ifdef __cplusplus
}
#endif
#endif
