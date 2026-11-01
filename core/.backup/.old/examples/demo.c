/* Conformance demo AND acceptance suite for the pure-C core.
 *
 * WHITEPAPER section 20's five numbered properties are checked as C1..C5
 * rather than paraphrased, plus the laws a plausible implementation would get
 * wrong. Exit status is nonzero if any check fails, so `make run` is a test.
 */

#include "vi.h"

#include <stdio.h>
#include <string.h>

static int checks = 0, failures = 0;
static void check(int c, const char *what) {
    ++checks;
    if (!c) { ++failures; printf("  FAIL  %s\n", what); }
}

int main(void) {
    Dictionary dict;
    WordFlowMap map;
    FlowStore store;
    RGBString dog_rgb, wolf_rgb;
    WORD dog, canine, puppy, wolf, out;
    FLOW dog_flow, wolf_flow, f1, f2;

    check(vi_dictionary_init(&dict, 8) == VI_OK, "dictionary init");
    check(vi_wordflow_init(&map, 8) == VI_OK, "wordflow init");
    check(vi_flowstore_init(&store, 8) == VI_OK, "flowstore init");

    check(vi_rgb_parse("RBRBRBRB", &dog_rgb) == VI_OK, "parse dog rgb");
    check(vi_rgb_parse("RBGRBGRBGRB", &wolf_rgb) == VI_OK, "parse wolf rgb");
    check(vi_rgb_dimension(&dog_rgb) == 4, "dimension = count(Base)");
    check(vi_rgb_dimension(&wolf_rgb) == 4, "hypervector-like primitive is 4-dimensional");
    check(vi_rgb_parse("RBXRB", &(RGBString){0}) == VI_ERR, "an unknown symbol is refused");

    check(vi_flowstore_add(&store, &dog_rgb, &dog_flow) == VI_OK, "intern dog flow");
    check(vi_flowstore_add(&store, &wolf_rgb, &wolf_flow) == VI_OK, "intern wolf flow");
    check(dog_flow.data != wolf_flow.data, "different content, different Flow");

    /* Interning is idempotent, which is what lets identity be an id compare. */
    FLOW again;
    check(vi_flowstore_add(&store, &dog_rgb, &again) == VI_OK && again.data == dog_flow.data,
          "re-interning identical content returns the same Flow");

    vi_dictionary_add(&dict, "dog", &dog);
    vi_dictionary_add(&dict, "canine", &canine);
    vi_dictionary_add(&dict, "puppy", &puppy);
    vi_dictionary_add(&dict, "wolf", &wolf);
    vi_wordflow_put(&map, dog, dog_flow);
    vi_wordflow_put(&map, canine, dog_flow);
    vi_wordflow_put(&map, puppy, dog_flow);
    vi_wordflow_put(&map, wolf, wolf_flow);

    /* Section 7: the mapping is many-to-one. */
    WORD syn[8];
    size_t n_syn = vi_wordflow_reverse_all(&map, dog_flow, syn, 8);
    check(n_syn == 3, "three words converge on one Flow");

    /* --- section 21: the EXACT invariant ---------------------------------- */
    HYPERVECTOR hv;
    RGBString back;
    check(vi_hv_encode_rgb(&dog_rgb, &hv) == VI_OK, "lossless encode");
    check(vi_hv_decode_rgb(&hv, &back) == VI_OK, "lossless decode");
    check(vi_rgb_equal(&dog_rgb, &back), "decode(encode(f)) is EXACT");
    vi_rgb_free(&back);

    int all_lengths = 1;
    for (size_t n = 1; n <= 64; ++n) {
        RGBString a, b;
        RGBPrimitive buf[64];
        for (size_t i = 0; i < n; ++i) buf[i] = (RGBPrimitive)(i % 3);
        a.primitives = buf; a.length = n;
        HYPERVECTOR h;
        if (vi_hv_encode_rgb(&a, &h) != VI_OK) { all_lengths = 0; break; }
        if (vi_hv_decode_rgb(&h, &b) != VI_OK) { all_lengths = 0; break; }
        if (!vi_rgb_equal(&a, &b)) all_lengths = 0;
        vi_rgb_free(&b);
        if (!all_lengths) break;
    }
    check(all_lengths, "exact for every length 1..64");

    /* --- WHITEPAPER section 20, C1..C5 ------------------------------------ */
    check(vi_word_from_cstr("dog").data == vi_word_from_cstr("dog").data,
          "C1 word -> flow is deterministic");

    int rc = vi_conformance_word_to_flow_to_hv_to_flow_to_word(
        &dict, &map, &store, dog, &out, &f1, &f2);
    check(rc == VI_OK, "C2 conformance path runs");
    check(f1.data == f2.data, "C3 Flow identity survives the hypervector");
    const char *label = vi_dictionary_label(&dict, out);
    check(label != NULL, "C4 projects back to a word");
    /* dog, canine and puppy share a Flow, so any of them is correct. Asserting
     * "dog" would be asserting that the mapping is injective, which it is not
     * and is not meant to be. */
    check(label && (!strcmp(label, "dog") || !strcmp(label, "canine") || !strcmp(label, "puppy")),
          "and the word is a synonym of the input");

    WORD out2; FLOW g1, g2;
    int rc2 = vi_conformance_word_to_flow_to_hv_to_flow_to_word(
        &dict, &map, &store, dog, &out2, &g1, &g2);
    check(rc2 == VI_OK && out2.data == out.data && g1.data == f1.data && g2.data == f2.data,
          "C5 reproducible across executions");

    /* --- determinism (FLOWALGEBRA 27) ------------------------------------- */
    STRATEGY bio = { "biological", 1 };
    STRATEGY bio2 = { "biological", 2 };
    STRATEGY fin = { "financial", 1 };
    check(vi_rand(&bio, 7, 3) == vi_rand(&bio, 7, 3), "rand is a pure function");
    check(vi_rand(&bio, 7, 3) != vi_rand(&bio2, 7, 3), "strategy VERSION changes the stream");
    check(vi_rand(&bio, 7, 3) != vi_rand(&fin, 7, 3), "strategy name changes the stream");

    /* --- attention is integer, bounded, strategy-indexed ------------------ */
    int32_t a_bio = vi_attention(&bio, &dog_rgb, &wolf_rgb);
    check(a_bio >= 0 && a_bio <= VI_ONE, "attention is bounded to [0,1]");
    check(vi_attention(&bio, &dog_rgb, &wolf_rgb) == a_bio, "attention is deterministic");
    check(vi_attention(&fin, &dog_rgb, &wolf_rgb) != a_bio,
          "the same pair attends differently under a different Strategy");
    check(vi_attention(&bio, &dog_rgb, &dog_rgb) == VI_ONE, "a Flow attends to itself maximally");

    /* Position binding: RB and BR must not collapse. */
    RGBString rb, br;
    vi_rgb_parse("RB", &rb); vi_rgb_parse("BR", &br);
    check(vi_attention(&bio, &rb, &br) < VI_ONE, "RB and BR are distinguishable");
    vi_rgb_free(&rb); vi_rgb_free(&br);

    /* --- signed-zero values (sections 10, 14, 15) ------------------------- */
    check(FV_NegZero == 0 && FV_NegOne == 1 && FV_PosZero == 2 && FV_PosOne == 3,
          "the specified bit patterns");
    check(vi_flowvalue_eq_numeric(FV_NegZero, FV_PosZero), "NegZero =n PosZero");
    check(!vi_flowvalue_eq_repr(FV_NegZero, FV_PosZero), "not (NegZero =r PosZero)");
    check(vi_flowvalue_add(LeftBiased, NULL, FV_PosOne, FV_NegOne) == FV_PosZero,
          "LeftBiased: PosOne + NegOne = PosZero");
    check(vi_flowvalue_add(RightBiased, NULL, FV_PosOne, FV_NegOne) == FV_NegZero,
          "RightBiased: PosOne + NegOne = NegZero");
    check(vi_flowvalue_add(LeftBiased, NULL, FV_NegOne, FV_PosOne) == FV_NegZero,
          "LeftBiased: NegOne + PosOne = NegZero");
    check(vi_flowvalue_add(RightBiased, NULL, FV_NegOne, FV_PosOne) == FV_PosZero,
          "RightBiased: NegOne + PosOne = PosZero");
    check(vi_flowvalue_add(LeftBiased, NULL, FV_PosOne, FV_PosOne) == FV_PosOne,
          "non-cancelling addition saturates");
    check(vi_flowvalue_add(StrategyBiased, &bio, FV_PosOne, FV_NegOne) ==
          vi_flowvalue_add(StrategyBiased, &bio, FV_PosOne, FV_NegOne),
          "strategy bias resolves reproducibly");

    /* --- capacity refusals ------------------------------------------------ */
    Dictionary small;
    vi_dictionary_init(&small, 1);
    check(vi_dictionary_add(&small, "one", NULL) == VI_OK, "first fits");
    check(vi_dictionary_add(&small, "two", NULL) == VI_FULL,
          "a full dictionary refuses rather than overrunning");
    vi_dictionary_free(&small);

    printf("\ninput=dog flow1=%u flow2=%u output=%s\n", f1.data, f2.data, label ? label : "(none)");
    printf("attention(dog,wolf) under biological = %d/%d\n", a_bio, VI_ONE);
    printf("%d checks, %d failures\n", checks, failures);

    vi_rgb_free(&dog_rgb);
    vi_rgb_free(&wolf_rgb);
    vi_flowstore_free(&store);
    vi_wordflow_free(&map);
    vi_dictionary_free(&dict);
    return failures ? 1 : 0;
}
