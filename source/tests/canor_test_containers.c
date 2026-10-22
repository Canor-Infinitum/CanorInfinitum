/* Property tests for the fixed-capacity containers ported off the heap
 * (AGENTS.md invariant 2): canor_flow's Dictionary / WordFlowMap / FlowStore /
 * RGBString, canor_index's SemanticTree and ANN index, canor_golay's M24
 * orbit workspace and timing telemetry, and canor_fw's file reader. Every
 * capacity is exercised up to its edge and one past it (CANOR_ERR_FULL). */

#include <stdio.h>
#include <string.h>

#include "canor_flow.h"
#include "canor_fw.h"
#include "canor_golay.h"
#include "canor_index.h"
#include "canor_test.h"

/* Large caller-placed structures live in static storage, as an embedded
 * caller would place them. */
static Dictionary g_dict;
static WordFlowMap g_map;
static FlowStore g_store;
static canor_tree g_tree;
static canor_ann g_ann;
static canor_golay_tele_t g_tele, g_tele2;
static uint64_t g_visited[CANOR_GOLAY_M24_VISITED_WORDS];
#define M24_QUEUE_TEST 200000              /* > C(24,6) = 134596 */
static uint32_t g_queue[M24_QUEUE_TEST];

static void test_rgb(void) {
  static char text[CANOR_RGB_MAX + 2];
  RGBString a, b;
  HYPERVECTOR hv;
  memset(text, 'R', CANOR_RGB_MAX);
  text[CANOR_RGB_MAX] = 0;
  CANOR_CHECK(canor_rgb_parse(text, &a) == CANOR_OK && a.length == CANOR_RGB_MAX);
  text[CANOR_RGB_MAX] = 'B';
  text[CANOR_RGB_MAX + 1] = 0;
  CANOR_CHECK(canor_rgb_parse(text, &a) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_rgb_parse("RGB x", &a) == CANOR_ERR_ARGS);
  /* Lossless round trip through the hypervector, by value. */
  CANOR_CHECK(canor_rgb_parse("RGBBGR", &a) == CANOR_OK);
  CANOR_CHECK(canor_hv_encode_rgb(&a, &hv) == CANOR_OK);
  CANOR_CHECK(canor_hv_decode_rgb(&hv, &b) == CANOR_OK);
  CANOR_CHECK(canor_rgb_equal(&a, &b));
  CANOR_CHECK(canor_rgb_dimension(&a) == 2);
}

static void test_dictionary(void) {
  char longlabel[CANOR_LABEL_MAX + 1];
  WORD w;
  CANOR_CHECK(canor_dictionary_init(&g_dict, CANOR_DICT_MAX + 1) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_dictionary_init(&g_dict, 2) == CANOR_OK);
  CANOR_CHECK(canor_dictionary_add(&g_dict, "dog", &w) == CANOR_OK);
  CANOR_CHECK(strcmp(canor_dictionary_label(&g_dict, w), "dog") == 0);
  CANOR_CHECK(canor_dictionary_add(&g_dict, "cat", NULL) == CANOR_OK);
  CANOR_CHECK(canor_dictionary_add(&g_dict, "wolf", NULL) == CANOR_ERR_FULL);
  CANOR_CHECK(g_dict.count == 2);
  /* A label that does not fit is refused, never cut. */
  CANOR_CHECK(canor_dictionary_init(&g_dict, 0) == CANOR_OK && g_dict.capacity == CANOR_DICT_MAX);
  memset(longlabel, 'a', CANOR_LABEL_MAX);
  longlabel[CANOR_LABEL_MAX] = 0;
  CANOR_CHECK(canor_dictionary_add(&g_dict, longlabel, NULL) == CANOR_ERR_FULL);
  longlabel[CANOR_LABEL_MAX - 1] = 0;
  CANOR_CHECK(canor_dictionary_add(&g_dict, longlabel, NULL) == CANOR_OK);
  /* Fill to the compiled capacity, then one more. */
  {
    size_t i;
    char lbl[16];
    for (i = g_dict.count; i < CANOR_DICT_MAX; ++i) {
      snprintf(lbl, sizeof lbl, "w%u", (unsigned)i);
      CANOR_CHECK(canor_dictionary_add(&g_dict, lbl, NULL) == CANOR_OK);
    }
    CANOR_CHECK(canor_dictionary_add(&g_dict, "overflow", NULL) == CANOR_ERR_FULL);
  }
}

static void test_map_store_conformance(void) {
  RGBString dog, cat;
  FLOW fd, fc, f1, f2;
  WORD wd, wc, out;
  CANOR_CHECK(canor_wordflow_init(&g_map, 1) == CANOR_OK);
  CANOR_CHECK(canor_flowstore_init(&g_store, 1) == CANOR_OK);
  CANOR_CHECK(canor_flowstore_init(&g_store, CANOR_FLOWSTORE_MAX + 1) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_flowstore_init(&g_store, 1) == CANOR_OK);
  canor_rgb_parse("BRB", &dog);
  canor_rgb_parse("BRBB", &cat);
  CANOR_CHECK(canor_flowstore_add(&g_store, &dog, &fd) == CANOR_OK);
  CANOR_CHECK(canor_flowstore_add(&g_store, &dog, &f1) == CANOR_OK && f1.data == fd.data);
  CANOR_CHECK(canor_flowstore_add(&g_store, &cat, &fc) == CANOR_ERR_FULL);

  wd = canor_word_from_cstr("dog");
  wc = canor_word_from_cstr("cat");
  CANOR_CHECK(canor_wordflow_put(&g_map, wd, fd) == CANOR_OK);
  CANOR_CHECK(canor_wordflow_put(&g_map, wd, fd) == CANOR_OK);    /* update, not insert */
  CANOR_CHECK(canor_wordflow_put(&g_map, wc, fd) == CANOR_ERR_FULL);

  /* WORD -> FLOW -> HYPERVECTOR -> FLOW -> WORD, with no heap in the path. */
  CANOR_CHECK(canor_conformance_word_to_flow_to_hv_to_flow_to_word(
                  NULL, &g_map, &g_store, wd, &out, &f1, &f2) == CANOR_OK);
  CANOR_CHECK(f1.data == f2.data && out.data == wd.data);
}

static void test_tree(void) {
  static const char *const words[] = { "a", "aa", "aah", "aahing", "dog" };
  char rgb[32];
  uint32_t anc[8];
  CANOR_CHECK(canor_tree_build(&g_tree, words, 5) == CANOR_OK);
  CANOR_CHECK(canor_tree_parent(&g_tree, 3) == 2);
  CANOR_CHECK(canor_tree_depth(&g_tree, 3) == 3);
  CANOR_CHECK(canor_tree_ancestors(&g_tree, 3, anc, 8) == 3 && anc[2] == 0);
  CANOR_CHECK(canor_tree_region(&g_tree, 3) == 0);
  CANOR_CHECK(canor_tree_root_count(&g_tree) == 2);
  CANOR_CHECK(canor_tree_gapped_rgb(&g_tree, 3, rgb, sizeof rgb) == 9);
  CANOR_CHECK(strcmp(rgb, "RGRGBGRBB") == 0);
  CANOR_CHECK(canor_tree_build(&g_tree, words, CANOR_TREE_WORDS_MAX + 1) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_tree_build(&g_tree, words, 0) == CANOR_ERR_ARGS);
}

#define ANN_N 64
static void test_ann(void) {
  static HYPERVECTOR sigs[ANN_N];
  STRATEGY s = { "canor_test", 1 };
  uint64_t seed = 7;
  uint32_t out[4], probed = 0, i, w;
  int32_t score[4];
  for (i = 0; i < ANN_N; ++i)
    for (w = 0; w < CANOR_HV_WORDS; ++w) sigs[i].w[w] = canor_test_next(&seed);
  CANOR_CHECK(canor_ann_build(&g_ann, sigs, ANN_N, &s, CANOR_ANN_TABLES_MAX + 1, 8) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_ann_build(&g_ann, sigs, ANN_N, &s, 4, CANOR_ANN_BITS_MAX + 1) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_ann_build(&g_ann, sigs, CANOR_ANN_N_MAX + 1, &s, 4, 8) == CANOR_ERR_FULL);
  CANOR_CHECK(canor_ann_build(&g_ann, sigs, ANN_N, &s, 4, 8) == CANOR_OK);
  CANOR_CHECK(canor_ann_query(&g_ann, sigs, &sigs[17], 1, out, score, &probed) == 1);
  CANOR_CHECK(out[0] == 17 && score[0] == CANOR_ONE && probed >= 1);
  CANOR_CHECK(canor_ann_query_exact(sigs, ANN_N, &sigs[5], 1, out, score) == 1 && out[0] == 5);
}

static void test_m24(void) {
  canor_golay_m24_ws_t ws = { g_visited, g_queue, M24_QUEUE_TEST };
  canor_golay_m24_ws_t tiny = { g_visited, g_queue, 10 };
  m24_orbit_census c;
  uint32_t rep = 0;
  long size = 0;
  CANOR_CHECK(golay_init());
  CANOR_CHECK(m24_canonical(1u << 7, &ws, &rep, &size) == CANOR_OK);
  CANOR_CHECK(rep == 1 && size == 24);            /* transitive on points */
  CANOR_CHECK(m24_canonical(1u << 7, &tiny, &rep, &size) == CANOR_ERR_FULL);
  CANOR_CHECK(m24_canonical(1, NULL, &rep, &size) == CANOR_ERR_ARGS);
  /* 5-transitive: one orbit up to weight 5; it must split at weight 6. */
  CANOR_CHECK(m24_orbits_of_weight(2, &ws, &c) == CANOR_OK);
  CANOR_CHECK(c.norbits == 1 && c.total == 276);
  CANOR_CHECK(m24_orbits_of_weight(6, &ws, &c) == CANOR_OK);
  CANOR_CHECK(c.norbits == 2 && c.total == 134596);
  CANOR_CHECK(m24_orbits_of_weight(6, &tiny, &c) == CANOR_ERR_FULL);
}

#define TICKS 100
#define TICKS_DEV 10

static void fill_tele(canor_golay_tele_t *t) {
  int i;
  canor_golay_tele_reset(t);
  for (i = 0; i < CANOR_TIMING_SAMPLES_MAX; ++i) {
    CANOR_CHECK(canor_golay_tele_observe(t, CANOR_TIMING_PRE, 1, TICKS) == CANOR_OK);
    CANOR_CHECK(canor_golay_tele_observe(t, CANOR_TIMING_PERI, 1,
                                         (i & 1) ? TICKS + TICKS_DEV : TICKS - TICKS_DEV) == CANOR_OK);
    CANOR_CHECK(canor_golay_tele_observe(t, CANOR_TIMING_POST, 1, TICKS) == CANOR_OK);
  }
}

static void test_telemetry(void) {
  canor_golay_forecast_t f;
  canor_timing_metric_t m;
  int d = -1;
  fill_tele(&g_tele);
  /* A full series refuses; it never drops the oldest sample. */
  CANOR_CHECK(canor_golay_tele_observe(&g_tele, CANOR_TIMING_PRE, 1, TICKS) == CANOR_ERR_FULL);
  CANOR_CHECK(g_tele.n[CANOR_TIMING_PRE] == CANOR_TIMING_SAMPLES_MAX);
  CANOR_CHECK(canor_golay_tele_observe(&g_tele, (canor_timing_phase)3, 1, TICKS) == CANOR_ERR_ARGS);
  CANOR_CHECK(canor_golay_tele_observe(&g_tele, CANOR_TIMING_PRE, 0, TICKS) == CANOR_ERR_ARGS);

  /* PERI alternates 90/110: sigma = 10 exactly, CV = 0.1. Margin is
   * ceil(1.96 . 10) = 19.6 ticks = 19600 milli-ticks, rounded outward. */
  CANOR_CHECK(canor_golay_tele_forecast(&g_tele, &f) == CANOR_OK);
  CANOR_CHECK(f.expected_milli == 3 * TICKS * 1000);
  CANOR_CHECK(f.lo_milli == 3 * TICKS * 1000 - 19600 && f.hi_milli == 3 * TICKS * 1000 + 19600);
  CANOR_CHECK(f.jitter_milli[CANOR_TIMING_PRE] == 0 && f.jitter_milli[CANOR_TIMING_PERI] == 100);
  CANOR_CHECK(f.n == CANOR_TIMING_SAMPLES_MAX);
  /* 10^6 . 64 / ((64 + 8)(1000 + 100)) */
  CANOR_CHECK(f.confidence_milli == 808);

  CANOR_CHECK(canor_golay_tele_measure(&g_tele, 1000, &m) == CANOR_OK);
  CANOR_CHECK(m.latency_defined == 7 && m.jitter_defined == 7);

  fill_tele(&g_tele2);
  CANOR_CHECK(canor_golay_tele_similar(&g_tele, &g_tele2, &d) && d == 0);
}

static void test_fw_read(void) {
  const char *path = "canor_test_fw_read.tmp";
  char small[4], big[64];
  size_t len = 0;
  FILE *f = fopen(path, "wb");
  CANOR_CHECK(f != NULL);
  if (!f) return;
  fputs("&type a;", f);
  fclose(f);
  CANOR_CHECK(fw_read_file(path, small, sizeof small, &len) == CANOR_ERR_FULL);
  CANOR_CHECK(fw_read_file(path, big, sizeof big, &len) == CANOR_OK && len == 8);
  CANOR_CHECK(strcmp(big, "&type a;") == 0);
  CANOR_CHECK(fw_read_file("canor_test_no_such_file.tmp", big, sizeof big, &len) == CANOR_ERR_NOT_FOUND);
  remove(path);
}

int main(void) {
  test_rgb();
  test_dictionary();
  test_map_store_conformance();
  test_tree();
  test_ann();
  test_m24();
  test_telemetry();
  test_fw_read();
  return CANOR_TEST_DONE("canor_test_containers");
}
