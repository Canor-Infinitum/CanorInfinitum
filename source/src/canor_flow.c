/* Canor Intelligence - pure C core. See include/vi.h.
 *
 * No floating point anywhere in this file. That is a conformance property, not
 * a style choice: FLOWALGEBRA section 27 forbids floating-point
 * nondeterminism in verifiable mode, and two hosts that sum the same floats in
 * a different order disagree. <math.h> is deliberately not included.
 */

#include "canor_flow.h"

#include <stdlib.h>
#include <string.h>

/* --- deterministic utility ------------------------------------------------ */

uint32_t canor_hash_bytes(const uint8_t *bytes, size_t length, uint32_t seed) {
    uint32_t h = 2166136261u ^ seed;
    for (size_t i = 0; i < length; ++i) {
        h ^= bytes[i];
        h *= 16777619u;
    }
    /* Never zero, so a valid FLOW id is distinguishable from a zeroed struct.
     * Kept from the skeleton; it is a real property callers may rely on. */
    return h ? h : 1u;
}

uint64_t canor_rand(const STRATEGY *s, uint64_t seed, uint64_t index) {
    uint64_t k = 0x9E3779B97F4A7C15ull;
    if (s && s->name) {
        for (const char *p = s->name; *p; ++p)
            k = (k ^ (uint64_t)(unsigned char)*p) * 0x100000001B3ull;
    }
    if (s) k ^= (uint64_t)s->version * 0xD6E8FEB86659FD93ull;
    uint64_t x = k ^ (seed * 0xBF58476D1CE4E5B9ull) ^ (index + 0x9E3779B97F4A7C15ull);
    /* SplitMix64 finalizer. Unsigned throughout, so the overflow is defined and
     * there is no undefined behaviour for section 39 to object to. */
    x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
    x ^= x >> 27; x *= 0x94D049BB133111EBull;
    x ^= x >> 31;
    return x;
}

WORD canor_word_from_cstr(const char *s) {
    WORD w;
    w.data = s ? canor_hash_bytes((const uint8_t*)s, strlen(s), 0x574f5244u) : 0u;
    return w;
}

int canor_rgb_parse(const char *s, RGBString *out) {
    if (!s || !out) return CANOR_ERR;
    size_t n = strlen(s);
    RGBPrimitive *p = (RGBPrimitive*)calloc(n ? n : 1, sizeof(RGBPrimitive));
    if (!p) return CANOR_ERR;
    size_t j = 0;
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == 'R' || s[i] == 'r') p[j++] = RGB_Real;
        else if (s[i] == 'G' || s[i] == 'g') p[j++] = RGB_Gap;
        else if (s[i] == 'B' || s[i] == 'b') p[j++] = RGB_Base;
        else if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n') continue;
        else { free(p); return CANOR_ERR; }
    }
    out->length = j;
    out->primitives = p;
    return CANOR_OK;
}

int canor_rgb_copy(const RGBString *src, RGBString *dst) {
    if (!src || !dst) return CANOR_ERR;
    RGBPrimitive *p = (RGBPrimitive*)calloc(src->length ? src->length : 1, sizeof(RGBPrimitive));
    if (!p) return CANOR_ERR;
    memcpy(p, src->primitives, src->length * sizeof(RGBPrimitive));
    dst->primitives = p;
    dst->length = src->length;
    return CANOR_OK;
}

int canor_rgb_equal(const RGBString *a, const RGBString *b) {
    if (!a || !b) return 0;
    if (a->length != b->length) return 0;
    if (a->length == 0) return 1;
    return memcmp(a->primitives, b->primitives, a->length * sizeof(RGBPrimitive)) == 0;
}

void canor_rgb_free(RGBString *rgb) {
    if (!rgb) return;
    free(rgb->primitives);
    rgb->primitives = NULL;
    rgb->length = 0;
}

size_t canor_rgb_dimension(const RGBString *rgb) {
    if (!rgb) return 0;
    size_t d = 0;
    for (size_t i = 0; i < rgb->length; ++i) if (rgb->primitives[i] == RGB_Base) d++;
    return d;
}

FLOW canor_flow_from_rgb(const RGBString *rgb) {
    FLOW f = { 0 };
    if (!rgb || (!rgb->primitives && rgb->length)) return f;
    /* Hashed a byte at a time rather than through a heap temporary. The
     * skeleton allocated one, and returned an all-zero FLOW when the
     * allocation failed - which silently collapsed every Flow onto one id at
     * exactly the moment memory was scarce. */
    uint32_t h = 2166136261u ^ (0x464c4f57u ^ (uint32_t)canor_rgb_dimension(rgb));
    for (size_t i = 0; i < rgb->length; ++i) {
        h ^= (uint8_t)rgb->primitives[i];
        h *= 16777619u;
    }
    f.data = h ? h : 1u;
    return f;
}

/* --- dictionary ----------------------------------------------------------- */

int canor_dictionary_init(Dictionary *dict, size_t capacity) {
    if (!dict) return CANOR_ERR;
    if (!capacity) capacity = 1;
    dict->count = 0;
    dict->capacity = capacity;
    dict->words = (WORD*)calloc(capacity, sizeof(WORD));
    dict->labels = (char**)calloc(capacity, sizeof(char*));
    if (!dict->words || !dict->labels) {
        free(dict->words); free(dict->labels);
        dict->words = NULL; dict->labels = NULL; dict->capacity = 0;
        return CANOR_ERR;
    }
    return CANOR_OK;
}

void canor_dictionary_free(Dictionary *dict) {
    if (!dict) return;
    for (size_t i = 0; i < dict->count; ++i) free(dict->labels[i]);
    free(dict->words); free(dict->labels);
    dict->words = NULL; dict->labels = NULL;
    dict->count = 0; dict->capacity = 0;
}

static char *canor_strdup_local(const char *s) {
    size_t n = strlen(s) + 1;
    char *r = (char*)malloc(n);
    if (r) memcpy(r, s, n);
    return r;
}

int canor_dictionary_add(Dictionary *dict, const char *label, WORD *out_word) {
    if (!dict || !label || !dict->words || !dict->labels) return CANOR_ERR;
    /* The skeleton had no such check and wrote past the allocation. */
    if (dict->count >= dict->capacity) return CANOR_FULL;
    char *copy = canor_strdup_local(label);
    if (!copy) return CANOR_ERR;
    dict->words[dict->count] = canor_word_from_cstr(label);
    dict->labels[dict->count] = copy;
    if (out_word) *out_word = dict->words[dict->count];
    dict->count++;
    return CANOR_OK;
}

const char *canor_dictionary_label(const Dictionary *dict, WORD word) {
    if (!dict) return NULL;
    for (size_t i = 0; i < dict->count; ++i)
        if (dict->words[i].data == word.data) return dict->labels[i];
    return NULL;
}

/* --- word -> flow map ----------------------------------------------------- */

int canor_wordflow_init(WordFlowMap *map, size_t capacity) {
    if (!map) return CANOR_ERR;
    if (!capacity) capacity = 1;
    map->entries = (WordFlowEntry*)calloc(capacity, sizeof(WordFlowEntry));
    map->length = 0;
    map->capacity = capacity;
    return map->entries ? CANOR_OK : CANOR_ERR;
}

void canor_wordflow_free(WordFlowMap *map) {
    if (!map) return;
    free(map->entries);
    map->entries = NULL;
    map->length = 0;
    map->capacity = 0;
}

int canor_wordflow_put(WordFlowMap *map, WORD word, FLOW flow) {
    if (!map || !map->entries) return CANOR_ERR;
    for (size_t i = 0; i < map->length; ++i)
        if (map->entries[i].word.data == word.data) { map->entries[i].flow = flow; return CANOR_OK; }
    if (map->length >= map->capacity) return CANOR_FULL;
    map->entries[map->length].word = word;
    map->entries[map->length].flow = flow;
    map->length++;
    return CANOR_OK;
}

int canor_wordflow_get(const WordFlowMap *map, WORD word, FLOW *out) {
    if (!map || !out) return CANOR_ERR;
    for (size_t i = 0; i < map->length; ++i)
        if (map->entries[i].word.data == word.data) { *out = map->entries[i].flow; return CANOR_OK; }
    return CANOR_NOT_FOUND;
}

int canor_wordflow_reverse_first(const WordFlowMap *map, FLOW flow, WORD *out) {
    if (!map || !out) return CANOR_ERR;
    for (size_t i = 0; i < map->length; ++i)
        if (map->entries[i].flow.data == flow.data) { *out = map->entries[i].word; return CANOR_OK; }
    return CANOR_NOT_FOUND;
}

size_t canor_wordflow_reverse_all(const WordFlowMap *map, FLOW flow,
                               WORD *out, size_t max) {
    if (!map || !out) return 0;
    size_t n = 0;
    for (size_t i = 0; i < map->length && n < max; ++i)
        if (map->entries[i].flow.data == flow.data) out[n++] = map->entries[i].word;
    return n;
}

/* --- flow store ----------------------------------------------------------- */

int canor_flowstore_init(FlowStore *fs, size_t capacity) {
    if (!fs) return CANOR_ERR;
    if (!capacity) capacity = 1;
    fs->flows = (FLOW*)calloc(capacity, sizeof(FLOW));
    fs->content = (RGBString*)calloc(capacity, sizeof(RGBString));
    fs->count = 0;
    fs->capacity = capacity;
    if (!fs->flows || !fs->content) {
        free(fs->flows); free(fs->content);
        fs->flows = NULL; fs->content = NULL; fs->capacity = 0;
        return CANOR_ERR;
    }
    return CANOR_OK;
}

void canor_flowstore_free(FlowStore *fs) {
    if (!fs) return;
    for (size_t i = 0; i < fs->count; ++i) canor_rgb_free(&fs->content[i]);
    free(fs->flows); free(fs->content);
    fs->flows = NULL; fs->content = NULL;
    fs->count = 0; fs->capacity = 0;
}

int canor_flowstore_add(FlowStore *fs, const RGBString *rgb, FLOW *out) {
    if (!fs || !rgb || !fs->flows) return CANOR_ERR;
    FLOW f = canor_flow_from_rgb(rgb);
    for (size_t i = 0; i < fs->count; ++i) {
        if (fs->flows[i].data != f.data) continue;
        /* Same id: confirm the content actually matches before treating it as
         * the same Flow. A 32-bit hash collides eventually, and silently
         * merging two different Flows is the kind of error that surfaces much
         * later as an inexplicable synonym. */
        if (!canor_rgb_equal(&fs->content[i], rgb)) return CANOR_ERR;
        if (out) *out = f;
        return CANOR_OK;
    }
    if (fs->count >= fs->capacity) return CANOR_FULL;
    if (canor_rgb_copy(rgb, &fs->content[fs->count]) != CANOR_OK) return CANOR_ERR;
    fs->flows[fs->count] = f;
    fs->count++;
    if (out) *out = f;
    return CANOR_OK;
}

const RGBString *canor_flowstore_content(const FlowStore *fs, FLOW flow) {
    if (!fs) return NULL;
    for (size_t i = 0; i < fs->count; ++i)
        if (fs->flows[i].data == flow.data) return &fs->content[i];
    return NULL;
}

/* --- hypervectors --------------------------------------------------------- */

static void hv_clear(HYPERVECTOR *h) { memset(h, 0, sizeof *h); }

static void hv_set2(HYPERVECTOR *h, size_t slot, uint8_t v) {
    size_t bit = slot * 2u, w = bit / 64u, off = bit % 64u;
    h->w[w] &= ~((uint64_t)3u << off);
    h->w[w] |= ((uint64_t)(v & 3u)) << off;
}

static uint8_t hv_get2(const HYPERVECTOR *h, size_t slot) {
    size_t bit = slot * 2u, w = bit / 64u, off = bit % 64u;
    return (uint8_t)((h->w[w] >> off) & 3u);
}

int canor_hv_encode_rgb(const RGBString *rgb, HYPERVECTOR *out) {
    if (!rgb || !out) return CANOR_ERR;
    if (rgb->length * 2u > CANOR_HV_BITS) return CANOR_ERR;
    hv_clear(out);
    for (size_t i = 0; i < rgb->length; ++i)
        hv_set2(out, i, (uint8_t)(rgb->primitives[i] & 3u));
    out->length = (uint16_t)rgb->length;
    return CANOR_OK;
}

int canor_hv_decode_rgb(const HYPERVECTOR *hv, RGBString *out) {
    if (!hv || !out) return CANOR_ERR;
    size_t n = hv->length;
    if (n * 2u > CANOR_HV_BITS) return CANOR_ERR;
    RGBPrimitive *p = (RGBPrimitive*)calloc(n ? n : 1, sizeof(RGBPrimitive));
    if (!p) return CANOR_ERR;
    for (size_t i = 0; i < n; ++i) {
        uint8_t v = hv_get2(hv, i);
        if (v > 2u) { free(p); return CANOR_ERR; }  /* not a symbol we ever wrote */
        p[i] = (RGBPrimitive)v;
    }
    out->primitives = p;
    out->length = n;
    return CANOR_OK;
}

int canor_hv_signature(const RGBString *rgb, const STRATEGY *s, HYPERVECTOR *out) {
    if (!rgb || !out) return CANOR_ERR;
    hv_clear(out);
    out->length = (uint16_t)rgb->length;
    for (size_t i = 0; i < rgb->length; ++i) {
        uint8_t sym = (uint8_t)rgb->primitives[i];
        /* Position enters as a rotation. Without it, RB and BR bundle to the
         * same signature and the hypervector layer would contradict the
         * non-commutativity the Flow layer asserts. */
        unsigned rot = (unsigned)(i % 64u);
        for (unsigned w = 0; w < CANOR_HV_WORDS; ++w) {
            uint64_t basis = canor_rand(s, 0x5EEDu + sym, w);
            uint64_t bound = rot ? ((basis << rot) | (basis >> (64u - rot))) : basis;
            out->w[w] ^= bound;
        }
    }
    return CANOR_OK;
}

int canor_hv_encode(FLOW flow, HYPERVECTOR *out) {
    if (!out) return CANOR_ERR;
    hv_clear(out);
    /* Derived from the identifier alone, so it cannot be inverted. Kept for
     * similarity over Flows whose content is not to hand; canor_hv_encode_rgb is
     * the one to use when identity matters. */
    for (unsigned w = 0; w < CANOR_HV_WORDS; ++w) {
        uint32_t h = canor_hash_bytes((const uint8_t*)&flow.data, sizeof flow.data,
                                   0x48560000u ^ w);
        out->w[w] = ((uint64_t)h << 32) ^ canor_hash_bytes((const uint8_t*)&h, sizeof h, w);
    }
    out->length = 0;
    return CANOR_OK;
}

int32_t canor_hv_hamming(const HYPERVECTOR *a, const HYPERVECTOR *b) {
    if (!a || !b) return 0;
    int32_t n = 0;
    for (unsigned w = 0; w < CANOR_HV_WORDS; ++w) {
        uint64_t x = a->w[w] ^ b->w[w];
        /* Written out rather than __builtin_popcountll: this core has to build
         * under a cross-compiler with no configure step. */
        x = x - ((x >> 1) & 0x5555555555555555ull);
        x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
        x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Full;
        n += (int32_t)((x * 0x0101010101010101ull) >> 56);
    }
    return n;
}

int32_t canor_hv_similarity(const HYPERVECTOR *a, const HYPERVECTOR *b) {
    if (!a || !b) return 0;
    int32_t h = canor_hv_hamming(a, b);
    /* FULL width, not half.
     *
     * The obvious construction puts the zero point at half the width, since two
     * independent random vectors differ on about half their bits, and then
     * clamps the negative half away. That is what this did, and it creates a
     * DEAD ZONE: every unrelated pair clamps to exactly 0 and becomes
     * indistinguishable from every other unrelated pair. The acceptance suite
     * caught it immediately - attention(dog, wolf) was 0 under every Strategy,
     * so "the same pair attends differently under a different Strategy",
     * which is the whole point of section 23, could not be true.
     *
     * Over the full width the score is monotone in the Hamming distance with
     * no clamping: identical is CANOR_ONE, complementary is 0, and independent
     * sits near CANOR_ONE/2. Still bounded to [0, CANOR_ONE] as section 23 requires,
     * still exact integer arithmetic, and now injective enough that a Strategy
     * change actually shows. */
    int64_t s = ((int64_t)((int32_t)CANOR_HV_BITS - h) * CANOR_ONE) / (int32_t)CANOR_HV_BITS;
    if (s < 0) s = 0;
    if (s > CANOR_ONE) s = CANOR_ONE;
    return (int32_t)s;
}

/* --- attention ------------------------------------------------------------ */

int32_t canor_attention(const STRATEGY *s, const RGBString *a, const RGBString *b) {
    HYPERVECTOR ha, hb;
    if (canor_hv_signature(a, s, &ha) != CANOR_OK) return 0;
    if (canor_hv_signature(b, s, &hb) != CANOR_OK) return 0;
    return canor_hv_similarity(&ha, &hb);
}

int32_t canor_attention_default(FLOW a, FLOW b) {
    HYPERVECTOR ha, hb;
    canor_hv_encode(a, &ha);
    canor_hv_encode(b, &hb);
    return canor_hv_similarity(&ha, &hb);
}

int canor_q_default(FLOW flow, HYPERVECTOR *out) { return canor_hv_encode(flow, out); }
int canor_k_default(FLOW flow, HYPERVECTOR *out) { return canor_hv_encode(flow, out); }
int canor_v_default(FLOW flow, HYPERVECTOR *out) { return canor_hv_encode(flow, out); }

/* --- conformance ---------------------------------------------------------- */

int canor_conformance_word_to_flow_to_hv_to_flow_to_word(
    const Dictionary *dict,
    const WordFlowMap *map,
    const FlowStore *store,
    WORD input,
    WORD *output,
    FLOW *flow1,
    FLOW *flow2
) {
    (void)dict;
    if (!map || !store || !output) return CANOR_ERR;

    /* WORD -> FLOW */
    FLOW f1;
    if (canor_wordflow_get(map, input, &f1) != CANOR_OK) return CANOR_NOT_FOUND;

    /* FLOW -> content -> HYPERVECTOR */
    const RGBString *content = canor_flowstore_content(store, f1);
    if (!content) return CANOR_NOT_FOUND;
    HYPERVECTOR hv;
    if (canor_hv_encode_rgb(content, &hv) != CANOR_OK) return CANOR_ERR;

    /* HYPERVECTOR -> FLOW. Decoded, then re-derived from the decoded content.
     * The skeleton assigned f2 = f1 here, which made the identity check a
     * tautology; this makes *flow1 == *flow2 evidence about the encoding. */
    RGBString back;
    if (canor_hv_decode_rgb(&hv, &back) != CANOR_OK) return CANOR_ERR;
    FLOW f2 = canor_flow_from_rgb(&back);
    canor_rgb_free(&back);

    /* FLOW -> WORD */
    int rc = canor_wordflow_reverse_first(map, f2, output);
    if (flow1) *flow1 = f1;
    if (flow2) *flow2 = f2;
    return rc == CANOR_OK ? CANOR_OK : CANOR_NOT_FOUND;
}

/* --- signed-zero arithmetic ----------------------------------------------- */

int canor_flowvalue_numeric(FLOWVALUE v) {
    switch (v) {
        case FV_NegOne: return -1;
        case FV_PosOne: return 1;
        case FV_NegZero:
        case FV_PosZero: return 0;
    }
    return 0;
}

int canor_flowvalue_eq_numeric(FLOWVALUE a, FLOWVALUE b) {
    return canor_flowvalue_numeric(a) == canor_flowvalue_numeric(b);
}

int canor_flowvalue_eq_repr(FLOWVALUE a, FLOWVALUE b) { return a == b; }

FLOWVALUE canor_flowvalue_add(BiasMode mode, const STRATEGY *s,
                           FLOWVALUE left, FLOWVALUE right) {
    return canor_flowvalue_add_ctx(mode, s, left, right, 0);
}

FLOWVALUE canor_flowvalue_add_ctx(BiasMode mode, const STRATEGY *s,
                               FLOWVALUE left, FLOWVALUE right, uint64_t ctx) {
    int nl = canor_flowvalue_numeric(left), nr = canor_flowvalue_numeric(right);
    int sum = nl + nr;

    if (sum == 0 && nl != 0 && nr != 0) {
        /* The cancelling case. All four outcomes are fixed by the documents
         * and they are NOT symmetric: under LeftBiased, PosOne + NegOne is
         * PosZero while NegOne + PosOne is NegZero. The bias names which side's
         * sign survives as the zero's pole. */
        switch (mode) {
            case LeftBiased:  return nl > 0 ? FV_PosZero : FV_NegZero;
            case RightBiased: return nr > 0 ? FV_PosZero : FV_NegZero;
            case NeutralBiased:
                /* FLOWALGEBRA section 11 permits either. DECLARED as PosZero,
                 * fixed - "implementation-declared" means declared once, not
                 * chosen per call, or the result stops being reproducible. */
                return FV_PosZero;
            case StrategyBiased:
                if (!s) return FV_PosZero;
                /* Deterministic in the Strategy AND in the operands. The index
                 * was previously a constant 0, which made a Strategy draw the
                 * same bit for every cancellation it ever performed - so the
                 * mode collapsed to "always PosZero" or "always NegZero", both
                 * of which LeftBiased/RightBiased/NeutralBiased already reach.
                 * A fourth mode that duplicates the first three is not a fourth
                 * mode. Mixing the operands in makes the resolution depend on
                 * WHICH cancellation occurred, which is the only way this can
                 * be a distinct rule rather than a coin flipped once per name.
                 *
                 * Still fully deterministic: the same Strategy and the same
                 * operand pair always resolve identically, on any machine. */
                return (canor_rand(s, 0xB1A5ull,
                                (ctx << 4) |
                                (uint64_t)((unsigned)left * 4u + (unsigned)right))
                        & 1ull) ? FV_PosZero : FV_NegZero;
        }
    }

    /* Non-cancelling. Saturating, declared: FLOWVALUE has no magnitude above
     * one, so PosOne + PosOne is PosOne. */
    if (sum > 0) return FV_PosOne;
    if (sum < 0) return FV_NegOne;

    /* Two zeros. Keeps a pole rather than inventing one: like poles survive, a
     * mixed pair takes the left operand's, matching the left-preference the
     * bias cases use. */
    if (left == FV_NegZero && right == FV_NegZero) return FV_NegZero;
    if (left == FV_PosZero && right == FV_PosZero) return FV_PosZero;
    return left;
}
