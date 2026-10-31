#include "manifold.h"
#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "fail: %s\n", msg);
    fails++;
  }
}

int main(void) {
  int i, j;
  canordb_pop p;
  canordb_chart c;
  canordb_inference a, b;
  canordb_flow flow;
  uint64_t rank = 0, back = 0;
  canor_lehmer restored;

  expect(CANORDB_DIM_COUNT == 288, "count");
  expect(DIM_STATISTICS == 1, "statistics");
  expect(DIM_SOLVED == 8, "solved");
  expect(DIM_NARRATIVE_SCOPE == 9, "narrative");
  expect(DIM_PROCEDURAL_CLOSURE == 288, "procedural");
  expect(canordb_dim_lookup("quantum") == DIM_QUANTUM, "lookup quantum");
  expect(canordb_dim_lookup("DIM_QUANTUM") == 3, "lookup prefixed");
  for (i = 1; i <= 288; i++) {
    const char *ni = canordb_dim_name(i);
    expect(ni != 0, "name present");
    expect(canordb_dim_lookup(ni) == i, "roundtrip name");
    for (j = i + 1; j <= 288; j++)
      expect(strcmp(ni, canordb_dim_name(j)) != 0, "unique");
  }

  expect(canordb_population(CANORDB_GENERAL, 1, &p) == 0 && p.exact && p.value == 4, "g1");
  expect(canordb_population(CANORDB_SPECIFIC, 1, &p) == 0 && p.exact && p.value == 4, "s1");
  expect(canordb_population(CANORDB_DISTINCTIVE, 1, &p) == 0 && p.exact && p.value == 7, "d1");
  expect(canordb_population(CANORDB_ARBITRARY, 1, &p) == 0 && p.exact && p.value == 7, "a1");
  expect(canordb_population(CANORDB_GENERAL, 2, &p) == 0 && p.exact && p.value == 6, "g2");
  expect(canordb_population(CANORDB_SPECIFIC, 2, &p) == 0 && p.exact && p.value == 6, "s2");
  expect(canordb_population(CANORDB_DISTINCTIVE, 2, &p) == 0 && p.exact && p.value == 121, "d2");
  expect(canordb_population(CANORDB_ARBITRARY, 2, &p) == 0 && p.exact && p.value == 121, "a2");
  expect(canordb_population(CANORDB_ARBITRARY, 30, &p) == 0 && !p.exact && p.log2_milli != 0, "a30");
  expect(p.value == 0, "a30 not wrapped");

  expect(canordb_chart_stage(2, 0, &c) == 0 && c.dimension == 4, "stage2");
  expect(canordb_chart_stage(3, 0, &c) == 0 && c.dimension == 8, "stage3");
  expect(canordb_chart_stage(15, 0, &c) == 0 && c.dimension == 32768, "stage15");
  expect(canordb_chart_dimension(6, 0, &c) != 0, "reject 6");
  expect(canordb_chart_dimension(32768, 1, &c) == 0 && c.stage == 15, "dim 32768");

  expect(canordb_infer("quantum", &a) == 0 && a.dim == 3, "infer quantum");
  expect(canordb_infer("quantum", &b) == 0, "infer again");
  expect(a.dim == b.dim && a.n == b.n && a.order == b.order, "stable fields");
  expect(canor_lehmer_cmp(&a.code, &b.code) == 0, "stable code");
  expect(a.chart.dimension == b.chart.dimension && a.chart.rgba_rank == b.chart.rgba_rank, "stable chart");

  expect(canordb_flow_address(3, CANORDB_GENERAL, 6, &flow) == 0, "flow");
  expect(canor_lehmer_to_u64(&flow.code, &rank) == CANOR_OK, "to u64");
  expect(canor_lehmer_from_u64(rank, flow.code.levels, &restored) == CANOR_OK, "from u64");
  expect(canor_lehmer_cmp(&flow.code, &restored) == 0, "lehmer roundtrip");
  back = rank;
  expect(back == rank, "rank kept");

  if (fails) {
    fprintf(stderr, "%d failed\n", fails);
    return 1;
  }
  printf("ok\n");
  return 0;
}
