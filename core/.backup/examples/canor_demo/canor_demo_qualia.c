/* canor_demo_qualia.c - qualia - six-tuple of 5 x 96-symbol Flows, one per imaginary trifecta
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* qualia - six-tuple of 5 x 96-symbol Flows, one per imaginary trifecta      */
/* ========================================================================== */
/* A Qualia-set is 6 x 5 x 96 symbols: six components, each carrying one
 * 96-symbol Flow per imaginary trifecta, across all five trifectas.
 *
 * WHY THE TRIFECTA PARTITION MATTERS. CANOR_IMAG_TRIFECTAS = 5 gives
 * 1 + 3.5 = 16 basis elements, and the fifteen imaginary units fall into five
 * disjoint triples - trifecta t holding units 3t+1, 3t+2, 3t+3. A Flow written
 * "across all imaginary trifectas" therefore is not one word over sixteen
 * letters; it is five words, each over its own three. Writing it as a single
 * alphabet would lose the partition, and the partition is what makes the five
 * components separable observations rather than one long one.
 *
 * 96 is CANOR_LEVELS_MAX, the declared topology ceiling, so a component Flow is as
 * long as the topology admits.
 *
 * Each of the 30 Flows folds under Cayley-Dickson to (unit, sign, ring), and
 * those thirty folds ARE the Qualia-set's content. The symbols are how it is
 * written; the folds are what it says.
 */

#define QUALIA_TUPLE 6
#define QUALIA_TRIFECTAS CANOR_IMAG_TRIFECTAS
#define QUALIA_LEN 96

typedef struct {
  uint8_t sym[QUALIA_TUPLE][QUALIA_TRIFECTAS][QUALIA_LEN];
} qualia_set;

/* Every symbol must lie in its own trifecta. Checked rather than assumed:
 * a symbol from the wrong trifecta still folds, so the error would produce
 * numbers instead of a complaint. */

/* Builds over an arbitrary grouping. Component t draws its symbols from group
 * t of the selected grouping, so one builder serves index, quaternion and Fano
 * without any of them being the privileged case. */
static void qualia_build_g(const canor_strategy *s, const ex_groups *g,
                           qualia_set *q) {
  int a, t, i;
  memset(q, 0, sizeof *q);
  for (a = 0; a < QUALIA_TUPLE; ++a)
    for (t = 0; t < QUALIA_TRIFECTAS && t < g->count; ++t)
      for (i = 0; i < QUALIA_LEN; ++i) {
        uint64_t r = canor_rand(&s->id, 0x0DA11Aull + (uint64_t)(a * 8 + t),
                             (uint64_t)i);
        q->sym[a][t][i] = g->u[t][(int)(r % 3u)];
      }
}

static bool qualia_valid_g(const qualia_set *q, const ex_groups *g, int ncomp) {
  int a, t, i, k;
  for (a = 0; a < QUALIA_TUPLE; ++a)
    for (t = 0; t < ncomp; ++t)
      for (i = 0; i < QUALIA_LEN; ++i) {
        int u = q->sym[a][t][i], in = 0;
        for (k = 0; k < 3; ++k) if (g->u[t][k] == u) in = 1;
        if (!in) return false;
      }
  return true;
}

static bool qualia_fold(const qualia_set *q, int a, int t,
                        uint8_t *unit, int *sign) {
  canor_imag acc = 0;
  int sg = 1, i;
  for (i = 0; i < QUALIA_LEN; ++i) {
    canor_imag prod;
    int s2;
    if (canor_imag_mul(acc, q->sym[a][t][i], &prod, &s2) != CANOR_OK) return false;
    acc = prod;
    sg *= s2;
  }
  *unit = (uint8_t)acc;
  *sign = sg;
  return true;
}

int canor_demo_cmd_spreads(void) {
  ex_spread_set sp;
  ex_spread_orbits orb;
  int i, k;

  printf("Spreads of PG(3,2) and their A8 symmetry\n\n");
  printf("  e_a . e_b = +-e_(a XOR b) holds: %s\n",
         ex_index_xor_holds() ? "YES" : "NO");
  if (!ex_index_xor_holds()) {
    printf("  Without that, reading a unit index as a vector of F2^4 is not\n"
           "  justified, GL(4,2) is the wrong group, and every orbit below\n"
           "  would be computed against something that is not the\n"
           "  automorphism group - while still looking plausible.\n");
    return 2;
  }
  if (!ex_spreads_all(&sp)) {
    fprintf(stderr, "follow: spread search failed\n");
    return 2;
  }
  printf("  spreads found                %d\n", sp.count);
  if (!ex_spread_orbit_split(&sp, &orb)) return 2;
  printf("  |GL(4,2)| computed           %ld\n", orb.group_order);
  printf("  orbits under GL(4,2) ~ A8    %d\n\n", orb.norbits);

  printf("  %-7s %8s %12s %14s\n", "orbit", "size", "stabiliser", "size x stab");
  for (i = 0; i < orb.norbits; ++i)
    printf("  %-7d %8d %12ld %14ld\n", i, orb.size[i], orb.stabiliser[i],
           (long)orb.size[i] * orb.stabiliser[i]);
  printf("\n  Orbit-stabiliser: size x stabiliser must equal |GL(4,2)| = %ld\n",
         orb.group_order);

  for (i = 0; i < orb.norbits; ++i) {
    printf("\n  orbit %d representative:\n   ", i);
    for (k = 0; k < 5; ++k)
      printf(" {%s,%s,%s}", canor_imag_name(sp.line[orb.rep[i]][k][0]),
             canor_imag_name(sp.line[orb.rep[i]][k][1]),
             canor_imag_name(sp.line[orb.rep[i]][k][2]));
    printf("\n");
  }

  printf("\n  One orbit would mean every spread is a relabelling of every\n"
         "  other, so which one the extraction picks cannot matter. More than\n"
         "  one means the choice is a real parameter and has to be declared\n"
         "  with the artifact.\n");
  return 0;
}

int canor_demo_cmd_qualia(const canor_demo_config_t *c, int sel, const char *grouping) {
  qualia_set q;
  ex_groups g;
  /* Default: the spread. It is the only grouping that is BOTH a partition and
   * fold-closed, so a component is independently addressable and its words
   * stay inside the algebra they are written over. The other three remain
   * selectable because they answer different questions. */
  ex_grouping kind = EX_GROUP_SPREAD;
  int a, t, closed = 0, total = 0, ncomp, escaped = 0;
  long hist[EX_MAX_DIM];
  memset(hist, 0, sizeof hist);

  if (grouping) {
    if (!strcmp(grouping, "index")) kind = EX_GROUP_INDEX;
    else if (!strcmp(grouping, "quaternion")) kind = EX_GROUP_QUATERNION;
    else if (!strcmp(grouping, "fano")) kind = EX_GROUP_FANO;
    else if (!strcmp(grouping, "spread")) kind = EX_GROUP_SPREAD;
    else {
      fprintf(stderr, "follow: grouping must be index, quaternion, "
                      "fano or spread\n");
      return 2;
    }
  }
  if (!ex_groups_build(&g, kind, CANOR_IMAG_COUNT)) {
    fprintf(stderr, "follow: cannot build the %s grouping\n",
            ex_grouping_name(kind));
    return 2;
  }
  ncomp = g.count < QUALIA_TRIFECTAS ? g.count : QUALIA_TRIFECTAS;
  qualia_build_g(&c->strat[sel], &g, &q);
  /* PROVENANCE. A Qualia-set means nothing without the grouping it was drawn
   * over: the same strategy and the same seed give DIFFERENT sets under index,
   * quaternion and Fano. So the grouping travels with the data rather than
   * being left to whoever reads it later. */
  printf("Qualia-set\n");
  printf("  provenance\n");
  printf("    strategy    %s v%u\n", c->strat[sel].id.name,
         (unsigned)c->strat[sel].id.version);
  printf("    grouping    %s\n", ex_grouping_name(kind));
  printf("    groups      %d available, %d used%s\n", g.count, ncomp,
         g.partition ? ", disjoint (a partition)"
                     : ", OVERLAPPING (a cover, not a partition)");
  printf("    shape       %d-tuple x %d x %d = %d symbols\n", QUALIA_TUPLE,
         ncomp, QUALIA_LEN, QUALIA_TUPLE * ncomp * QUALIA_LEN);
  printf("  groups\n");
  for (t = 0; t < ncomp; ++t)
    printf("    G%-2d  {%s, %s, %s}  %s\n", t,
           canor_imag_name(g.u[t][0]), canor_imag_name(g.u[t][1]),
           canor_imag_name(g.u[t][2]),
           g.closes[t] ? "closes" : "does NOT close");
  printf("\n");

  if (!qualia_valid_g(&q, &g, ncomp)) {
    fprintf(stderr, "follow: a symbol fell outside its group\n");
    return 2;
  }
  printf("  every symbol lies in its own group: yes\n\n");

  printf("  folds, (sign)unit per component x group\n\n      ");
  for (t = 0; t < ncomp; ++t) printf("  G%-8d", t);
  printf("\n");
  for (a = 0; a < QUALIA_TUPLE; ++a) {
    printf("  %-4d", a);
    for (t = 0; t < ncomp; ++t) {
      uint8_t u;
      int sg, k, inside = 0;
      ++total;
      if (!qualia_fold(&q, a, t, &u, &sg)) { printf("  %-9s", "refused"); continue; }
      ++closed;
      if (u < EX_MAX_DIM) hist[u]++;
      for (k = 0; k < 3; ++k) if (g.u[t][k] == u) inside = 1;
      if (u != 0 && !inside) ++escaped;
      printf("  %c%-8s", sg < 0 ? '-' : '+', canor_imag_name(u));
    }
    printf("\n");
  }

  printf("\n  %d of %d component Flows fold and close\n", closed, total);
  printf("  %d fold OUTSIDE their own group%s\n", escaped,
         escaped ? "  - separable as written, not as folded" : "");
  printf("\n  where the 30 folds land\n");
  for (a = 0; a < EX_MAX_DIM; ++a)
    if (hist[a]) printf("    %-6s %ld\n", canor_imag_name((canor_imag)a), hist[a]);
  return 0;
}
