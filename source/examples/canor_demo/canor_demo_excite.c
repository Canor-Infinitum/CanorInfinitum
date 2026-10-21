/* canor_demo_excite.c - excite - Cayley-Dickson bracket structure
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* excite - Cayley-Dickson bracket structure                                  */
/* ========================================================================== */

static int xchecks, xfails;
static void xcheck(int cond, const char *what) {
  ++xchecks;
  if (!cond) { ++xfails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_excite(const canor_demo_config_t *c, int sel) {
  static const int DIMS[4] = { 2, 4, 8, 16 };
  static const char *const DNAME[4] =
    { "complex", "quaternion", "octonion", "sedenion" };
  ex_census cen;
  ex_excitation ea, eb;
  ex_distortion d;
  int i, a, b;

  printf("Excitations: Cayley-Dickson bracket structure\n\n");
  printf("  [e_a, e_b] = e_a.e_b - e_b.e_a, which for basis elements is\n");
  printf("  exactly 0 or +-2.e_k - so the whole structure is integer and the\n");
  printf("  Jacobi test is zero-versus-nonzero, not a tolerance.\n\n");

  printf("  %-12s %5s %8s %10s %12s %12s\n", "algebra", "dim", "pairs",
         "commuting", "triples", "Jacobi fails");
  for (i = 0; i < 4; ++i) {
    if (!ex_census_run(DIMS[i], &cen)) {
      printf("  %-12s  census refused (the table does not close)\n", DNAME[i]);
      continue;
    }
    printf("  %-12s %5d %8ld %10ld %12ld %12ld\n", DNAME[i], DIMS[i],
           cen.pairs, cen.commuting, cen.triples, cen.jacobi_fail);
  }

  printf("\n  %-12s %5s %12s %12s\n", "algebra", "dim", "triples", "assoc fails");
  for (i = 0; i < 4; ++i) {
    if (!ex_census_run(DIMS[i], &cen)) continue;
    printf("  %-12s %5d %12ld %12ld\n", DNAME[i], DIMS[i],
           cen.triples, cen.assoc_fail);
  }

  /* The quaternion bracket table, which is small enough to read and is the
   * one case where the answer is known independently: it must be su(2). */
  printf("\n  quaternion brackets, [row, col] / 2\n\n      ");
  for (b = 0; b < 4; ++b) printf(" %5s", canor_imag_name((canor_imag)b));
  printf("\n");
  for (a = 0; a < 4; ++a) {
    printf("  %3s ", canor_imag_name((canor_imag)a));
    for (b = 0; b < 4; ++b) {
      ex_bracket br;
      if (!ex_bracket_of((uint8_t)a, (uint8_t)b, &br)) { printf(" %5s", "?"); continue; }
      if (br.coeff == 0) printf(" %5s", "0");
      else printf(" %4s%s", br.coeff < 0 ? "-" : "+", canor_imag_name(br.unit));
    }
    printf("\n");
  }

  /* --- the checks, with answers fixed before the run ---------------------- */
  ex_census_run(2, &cen);
  xcheck(cen.commuting == cen.pairs, "dim 2 is commutative: every bracket is zero");
  ex_census_run(4, &cen);
  xcheck(cen.jacobi_fail == 0, "dim 4 satisfies Jacobi on every triple (it is su(2))");
  xcheck(cen.assoc_fail == 0, "dim 4 is associative");
  ex_census_run(8, &cen);
  xcheck(cen.assoc_fail > 0, "dim 8 is NOT associative - octonions lose it");
  xcheck(cen.jacobi_fail > 0,
         "dim 8 FAILS Jacobi - the commutator is not a Lie algebra there");
  ex_census_run(16, &cen);
  xcheck(cen.jacobi_fail > 0, "dim 16 fails Jacobi too");

  /* Antisymmetry must hold at every dimension - it is a property of the
   * commutator itself and survives the loss of associativity. A version that
   * failed this would be computing something other than a bracket. */
  {
    int ok = 1;
    for (a = 0; a < 16 && ok; ++a)
      for (b = 0; b < 16 && ok; ++b) {
        ex_bracket p, q;
        if (!ex_bracket_of((uint8_t)a, (uint8_t)b, &p) ||
            !ex_bracket_of((uint8_t)b, (uint8_t)a, &q)) { ok = 0; break; }
        if (p.coeff != -q.coeff) ok = 0;
        if (p.coeff && p.unit != q.unit) ok = 0;
      }
    xcheck(ok, "[a,b] = -[b,a] at every dimension, associativity or not");
  }
  {
    int ok = 1;
    for (a = 0; a < 16; ++a) {
      ex_bracket p;
      if (!ex_bracket_of((uint8_t)a, (uint8_t)a, &p) || p.coeff != 0) { ok = 0; break; }
    }
    xcheck(ok, "[a,a] = 0 at every dimension");
  }

  /* --- frames and distortion --------------------------------------------- */
  printf("\n  frame distortion: the same channels under two Excitations\n\n");
  {
    const canor_ore *o1 = canor_ore_bound(c->strat[sel].id.name, CANOR_ABS_SIN);
    canor_ore alt;
    if (!ex_build(&ea, 8, NULL, CANOR_ABS_SIN)) return 2;
    xcheck(true, "unskewed Excitation builds");

    if (o1 && ex_build(&eb, 8, o1, CANOR_ABS_SIN) && ex_distort(&ea, &eb, &d)) {
      printf("    integer vs sin_S-skewed (%s)\n", c->strat[sel].id.name);
      printf("      pairs %ld   unit differs %ld   weight differs %ld\n",
             d.pairs, d.unit_differs, d.weight_differs);
      xcheck(d.unit_differs == 0,
             "a skew scalar changes WEIGHTS only - never the algebra");
      xcheck(d.weight_differs > 0, "and it does change some weight");
    }
    if (canor_ore_init(&alt, "0", "1", "1") && ex_build(&eb, 8, &alt, CANOR_ABS_COS) &&
        ex_distort(&ea, &eb, &d)) {
      printf("    integer vs cos_S-skewed (a=0, period 4)\n");
      printf("      pairs %ld   unit differs %ld   weight differs %ld\n",
             d.pairs, d.unit_differs, d.weight_differs);
    }
  }

  /* --- compactification by the zero set ----------------------------------
   * A compactification glues at a boundary, and the bracket tensor already
   * has one: the pairs that COMMUTE. [e_a, e_b] = 0 carries no direction, so
   * every commuting pair is the same zero point and identifying them is the
   * quotient. What survives is the non-commuting part, where the structure is.
   */
  printf("\n  compactification: all commuting pairs identified to one zero point\n\n");
  printf("  %-12s %5s %8s %10s %10s %10s\n", "algebra", "dim", "pairs",
         "-> zero", "non-zero", "quotient");
  for (i = 0; i < 4; ++i) {
    long distinct = 0;
    unsigned char seen[2 * EX_MAX_DIM + 2];
    int dim = DIMS[i];
    memset(seen, 0, sizeof seen);
    if (!ex_census_run(dim, &cen)) continue;
    for (a = 0; a < dim; ++a)
      for (b = 0; b < dim; ++b) {
        ex_bracket br;
        int slot;
        if (!ex_bracket_of((uint8_t)a, (uint8_t)b, &br) || br.coeff == 0) continue;
        /* On a basis the coefficient magnitude is always 2, so (sign, unit)
         * exhausts the distinctions a bracket can carry. */
        slot = (br.coeff > 0 ? 0 : EX_MAX_DIM) + br.unit;
        if (slot >= 0 && slot < (int)sizeof seen && !seen[slot]) {
          seen[slot] = 1;
          ++distinct;
        }
      }
    printf("  %-12s %5d %8ld %10ld %10ld %10ld\n", DNAME[i], dim, cen.pairs,
           cen.commuting, cen.pairs - cen.commuting, distinct + 1);
  }
  printf("\n  The quotient does not grow like the tensor. A basis bracket is\n"
         "  always +-2.e_k, so there are at most 2.dim distinct non-zero\n"
         "  brackets however large the algebra gets, plus the glued zero. The\n"
         "  tensor grows as dim^2 and its image as dim - and that gap IS the\n"
         "  compactness, not a metaphor for it.\n");

  /* --- the M24 quantizer on top of the compactification -------------------
   * The zero-set quotient collapses commuting pairs. M24 collapses further,
   * by identifying descriptors that the group maps to one another - and the
   * amount it collapses is not a parameter, it is what 5-transitivity dictates.
   * Weights 1..5 must fall to a single orbit; weight 6 must split. */
  printf("\n  M24 quantizer: subsets of the 24 points, by orbit\n");
  printf("  buckets are orbits, so the compression is the group's, not ours\n\n");
  printf("  %6s %12s %8s   %s\n", "weight", "subsets", "buckets", "orbit sizes");
  {
    int wt;
    for (wt = 1; wt <= 8; ++wt) {
      m24_orbit_census cen;
      int k;
      if (!m24_orbits_of_weight(wt, &cen)) {
        printf("  %6d   census refused\n", wt);
        continue;
      }
      printf("  %6d %12ld %8d  ", wt, cen.total, cen.norbits);
      for (k = 0; k < cen.norbits; ++k) printf(" %ld", cen.size[k]);
      if (wt <= 5) xcheck(cen.norbits == 1,
                          "weights 1..5 collapse to ONE bucket (5-transitivity)");
      if (wt == 6) xcheck(cen.norbits > 1,
                          "weight 6 SPLITS - M24 is 5-transitive, not 6");
      if (wt == 8) {
        int has759 = 0;
        for (k = 0; k < cen.norbits; ++k) if (cen.size[k] == 759) has759 = 1;
        xcheck(has759, "weight 8 has an orbit of exactly 759 - the octads");
      }
      printf("\n");
    }
  }
  printf("\n  Below weight 6 the quantizer is maximally compressive: every\n"
         "  subset of that size is one bucket, so a descriptor of 5 or fewer\n"
         "  points carries no information M24 can see. Structure only becomes\n"
         "  distinguishable at 6, and at 8 the octads separate out as their\n"
         "  own bucket of 759. That is the resolution limit of this quantizer\n"
         "  and it is a property of the group, not a setting.\n");

  printf("\n  NOTE. Everything above is arithmetic. Reading a Jacobiator as a\n"
         "  distortion metric for inter-observer communication is a modelling\n"
         "  claim of the framework; nothing here establishes it, and the\n"
         "  numbers are the same either way.\n");

  printf("\n%d checks, %d failures\n", xchecks, xfails);
  return xfails ? 1 : 0;
}
