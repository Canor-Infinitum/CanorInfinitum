/* Stress test v1-alpha: semantic dictionary instead of an orthographic one.
 *
 * The previous stress fixture was mapped_words_dictionary.json, which encodes
 * SPELLING - R for a vowel, B for a consonant, G never. It exercised the
 * algebra at scale and told us nothing about meaning, and it produced one
 * result that says so plainly: `dimension(s) = count(Base, s)` reported the
 * word "quaternion" as FIVE-dimensional, because BRRBRBBRRB has five
 * consonants. Quaternions are four-dimensional, and FLOWALGEBRA section 4 says
 * so itself, giving RBRBRBRB as the quaternion-like primitive.
 *
 * Two different things were sharing a name. This fixture separates them.
 *
 * WHAT IS ASSERTED
 * ----------------
 * The encoding rules are checked, not described:
 *   1. count(Base) is the concept's dimension in its own algebra.
 *   2. count(Gap) is its transition order.
 *   3. Layer is carried in Real cells, which affect neither count, so adding
 *      it cannot disturb rules 1 and 2.
 *
 * Rule 1 is the interesting one. The Cayley-Dickson ladder has to come out at
 * 1, 2, 4, 8, 16 - the same ladder the Follow-Core tower refuses to violate -
 * and if the encoding is right that falls out of counting Base cells.
 *
 * The two anchors are carried verbatim and are not re-derived:
 *     quaternion  RBRBRBRB        FLOWALGEBRA section 4
 *     ether       RGBRGBRGBRGB    four-dimensional, gap-saturated
 */

#include "vi.h"
#include "vi_index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXC 256

static int checks = 0, failures = 0;
static void check(int c, const char *what) {
  ++checks;
  if (!c) { ++failures; printf("  FAIL  %s\n", what); }
}

static char g_name[MAXC][32];
static char g_rgb[MAXC][64];
static FLOW g_flow[MAXC];
static HYPERVECTOR g_sig[MAXC];
static int g_n;

static int find(const char *w) {
  for (int i = 0; i < g_n; ++i) if (!strcmp(g_name[i], w)) return i;
  return -1;
}

static int load(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) { printf("cannot open %s\n", path); return 0; }
  static char buf[1 << 16];
  size_t got = fread(buf, 1, sizeof buf - 1, f);
  buf[got] = '\0';
  fclose(f);

  const char *p = buf;
  while (*p && g_n < MAXC) {
    const char *ks = strchr(p, '"');            if (!ks) break;
    const char *ke = strchr(ks + 1, '"');       if (!ke) break;
    const char *colon = strchr(ke + 1, ':');    if (!colon) break;
    const char *vs = strchr(colon + 1, '"');    if (!vs) break;
    const char *ve = strchr(vs + 1, '"');       if (!ve) break;
    size_t kl = (size_t)(ke - ks - 1), vl = (size_t)(ve - vs - 1);
    if (kl < sizeof g_name[0] && vl < sizeof g_rgb[0]) {
      memcpy(g_name[g_n], ks + 1, kl); g_name[g_n][kl] = '\0';
      memcpy(g_rgb[g_n], vs + 1, vl);  g_rgb[g_n][vl] = '\0';
      ++g_n;
    }
    p = ve + 1;
  }
  return g_n > 0;
}

static size_t count_of(const char *s, char c) {
  size_t n = 0;
  for (; *s; ++s) if (*s == c) ++n;
  return n;
}

/* Rule 1, against the algebra rather than against the file. */
static void test_dimension(void) {
  printf("rule 1: count(Base) is the dimension\n");
  struct { const char *name; size_t dim; } expect[] = {
    { "real", 1 }, { "complex", 2 }, { "quaternion", 4 },
    { "octonion", 8 }, { "sedenion", 16 },
    { "ether", 4 }, { "hypervector", 4 }, { "spacetime", 4 },
  };
  for (size_t e = 0; e < sizeof expect / sizeof expect[0]; ++e) {
    int i = find(expect[e].name);
    if (i < 0) { check(0, expect[e].name); continue; }
    RGBString r;
    vi_rgb_parse(g_rgb[i], &r);
    size_t d = vi_rgb_dimension(&r);
    vi_rgb_free(&r);
    if (d != expect[e].dim) {
      printf("  FAIL  %s is %zu-dimensional, expected %zu\n", expect[e].name, d, expect[e].dim);
      ++failures;
    }
    ++checks;
  }

  /* The Cayley-Dickson ladder must double, exactly. This is the same ladder
   * the Follow-Core tower refuses to violate, arrived at from the encoding
   * side rather than from the multiplication table. */
  const char *ladder[] = { "real", "complex", "quaternion", "octonion", "sedenion" };
  size_t prev = 0;
  int doubles = 1;
  for (int k = 0; k < 5; ++k) {
    int i = find(ladder[k]);
    RGBString r;
    vi_rgb_parse(g_rgb[i], &r);
    size_t d = vi_rgb_dimension(&r);
    vi_rgb_free(&r);
    if (k && d != prev * 2) doubles = 0;
    prev = d;
  }
  check(doubles, "the ladder doubles: 1, 2, 4, 8, 16");

  /* The headline correction. */
  int q = find("quaternion");
  check(q >= 0 && !strcmp(g_rgb[q], "RBRBRBRB"),
        "quaternion is FLOWALGEBRA section 4's primitive, verbatim");
  RGBString r;
  vi_rgb_parse(g_rgb[q], &r);
  check(vi_rgb_dimension(&r) == 4,
        "and it is 4-dimensional, not 5 as the orthographic spelling gave");
  vi_rgb_free(&r);
}

static void test_gap(void) {
  printf("rule 2: count(Gap) is the transition order\n");
  int e = find("ether"), qn = find("quaternion");
  check(e >= 0 && !strcmp(g_rgb[e], "RGBRGBRGBRGB"), "ether is carried verbatim");
  check(count_of(g_rgb[e], 'G') == 4, "ether is gap-saturated: one Gap per anchor");
  check(count_of(g_rgb[qn], 'G') == 0, "a division algebra carries no transitions");

  /* Both are four-dimensional and they are NOT the same kind of four. That
   * distinction is the whole reason the Gap channel exists, and the
   * orthographic dictionary could not express it at all - it never emitted a
   * single Gap in 370,100 entries. */
  RGBString a, b;
  vi_rgb_parse(g_rgb[e], &a);
  vi_rgb_parse(g_rgb[qn], &b);
  check(vi_rgb_dimension(&a) == vi_rgb_dimension(&b),
        "ether and quaternion share a dimension");
  check(count_of(g_rgb[e], 'G') != count_of(g_rgb[qn], 'G'),
        "and are still distinguishable, by Gap");
  STRATEGY s = { "algebraic", 1 };
  int32_t att = vi_attention(&s, &a, &b);
  check(att >= 0 && att < VI_ONE, "so they do not collapse onto one Flow");
  printf("      attention(ether, quaternion) = %d / %d\n", att, VI_ONE);
  vi_rgb_free(&a);
  vi_rgb_free(&b);
}

static void test_gap_channel_used(void) {
  printf("the Gap channel is actually exercised\n");
  int with_gap = 0;
  for (int i = 0; i < g_n; ++i) if (strchr(g_rgb[i], 'G')) ++with_gap;
  check(with_gap > 0, "at least one concept uses Gap");
  printf("      %d of %d concepts carry a Gap (orthographic fixture: 0 of 370100)\n",
         with_gap, g_n);
  check(with_gap * 4 >= g_n, "and it is not a token appearance");
}

static void test_convergence(void) {
  printf("section 7: convergence is declared, not accidental\n");
  int collisions = 0;
  for (int i = 0; i < g_n; ++i)
    for (int j = i + 1; j < g_n; ++j)
      if (g_flow[i].data == g_flow[j].data) {
        ++collisions;
        printf("      converge: %s = %s  (%s)\n", g_name[i], g_name[j], g_rgb[i]);
      }
  /* One declared equivalence class survives the encoding rules: a latent
   * positive null and a transition have the same dimension, the same
   * transition order and the same layer. Asserted exactly, so a change in the
   * dictionary shows up here rather than silently. */
  check(collisions == 1, "exactly one declared convergence class");
}

static void test_roundtrip(void) {
  printf("WHITEPAPER 20: the conformance path, on every concept\n");
  int ok = 1, checked = 0;
  for (int i = 0; i < g_n; ++i) {
    RGBString r, back;
    if (vi_rgb_parse(g_rgb[i], &r) != VI_OK) { ok = 0; break; }
    HYPERVECTOR hv;
    if (vi_hv_encode_rgb(&r, &hv) != VI_OK) { ok = 0; vi_rgb_free(&r); break; }
    if (vi_hv_decode_rgb(&hv, &back) != VI_OK) { ok = 0; vi_rgb_free(&r); break; }
    if (!vi_rgb_equal(&r, &back)) ok = 0;
    if (vi_flow_from_rgb(&back).data != g_flow[i].data) ok = 0;
    vi_rgb_free(&r);
    vi_rgb_free(&back);
    ++checked;
    if (!ok) break;
  }
  check(ok && checked == g_n, "every concept round-trips exactly");
}

/* The test that the orthographic fixture could not pass: are a concept's
 * nearest neighbours RELATED to it? */
static void test_neighbours(void) {
  printf("neighbours under attention\n");
  STRATEGY s = { "algebraic", 1 };
  for (int i = 0; i < g_n; ++i) {
    RGBString r;
    vi_rgb_parse(g_rgb[i], &r);
    vi_hv_signature(&r, &s, &g_sig[i]);
    vi_rgb_free(&r);
  }

  const char *probes[] = { "quaternion", "ether" };
  for (int p = 0; p < 2; ++p) {
    int q = find(probes[p]);
    uint32_t out[6];
    int32_t sc[6];
    uint32_t n = vi_ann_query_exact(g_sig, (uint32_t)g_n, &g_sig[q], 6, out, sc);
    printf("      %s:", probes[p]);
    for (uint32_t a = 0; a < n; ++a) printf(" %s(%d)", g_name[out[a]], sc[a]);
    printf("\n");
  }

  /* CHARACTERIZATION, not a desired property.
   *
   * The ladder ought to show up here: octonion CONTAINS quaternion's pattern
   * as a prefix - they agree on all eight of quaternion's positions - so a
   * similarity that tracked structure would rank octonion above ether, which
   * shares only three. Measured, it does not:
   *
   *     quaternion~octonion  482      8 of 8 positions agree
   *     quaternion~sedenion  490      8 of 8 positions agree
   *     quaternion~ether     519      3 of 8 positions agree
   *
   * Everything sits within about twenty points of 500, which is the noise
   * floor for signatures of this width. The ordering is not merely absent, it
   * is inverted.
   *
   * The cause is vi_hv_signature, not the dictionary. It XORs one
   * position-rotated basis vector per symbol, so every extra symbol mixes in
   * another full-width pseudorandom vector and a longer string's signature
   * becomes independent of its prefix's. Containment therefore produces no
   * signal, and the residual differences are length artifacts.
   *
   * That is worth stating rather than fixing here, because it is a finding
   * about the SIGNATURE scheme that the orthographic fixture could never have
   * surfaced - every word there was short and shape-collided anyway. Fixing it
   * means a representation where a prefix contributes a stable component:
   * hierarchical or permutation binding rather than flat XOR bundling.
   *
   * These assertions pin the CURRENT behaviour, so improving the signature
   * fails this test and forces the expectation to be updated deliberately. */
  int qn = find("quaternion"), oc = find("octonion"), sd = find("sedenion"), et = find("ether");
  int32_t q_o = vi_hv_similarity(&g_sig[qn], &g_sig[oc]);
  int32_t q_s = vi_hv_similarity(&g_sig[qn], &g_sig[sd]);
  int32_t q_e = vi_hv_similarity(&g_sig[qn], &g_sig[et]);
  printf("      quaternion~octonion %d  quaternion~sedenion %d  quaternion~ether %d\n",
         q_o, q_s, q_e);
  printf("\n  FINDING: containment carries no similarity signal. octonion agrees\n"
         "  with quaternion on all 8 of its positions and scores %d; ether agrees\n"
         "  on 3 and scores %d. Flat XOR bundling makes a longer signature\n"
         "  independent of its prefix. The dictionary is not the problem here.\n\n",
         q_o, q_e);
  check(q_o < VI_ONE / 2 + 50 && q_e < VI_ONE / 2 + 50,
        "characterized: all pairs sit at the noise floor");
  check(q_e > q_o,
        "characterized: the ladder ordering is inverted, not merely absent");
}

int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : "../semantic_v1a.json";
  if (!load(path)) return 1;
  printf("semantic dictionary v1-alpha: %d concepts from %s\n\n", g_n, path);

  for (int i = 0; i < g_n; ++i) {
    RGBString r;
    if (vi_rgb_parse(g_rgb[i], &r) != VI_OK) { printf("bad RGB for %s\n", g_name[i]); return 1; }
    g_flow[i] = vi_flow_from_rgb(&r);
    vi_rgb_free(&r);
  }

  test_dimension();
  test_gap();
  test_gap_channel_used();
  test_convergence();
  test_roundtrip();
  test_neighbours();

  printf("\n%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
