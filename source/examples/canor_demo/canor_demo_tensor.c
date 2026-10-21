/* canor_demo_tensor.c - tensor - H^(x)m, and per-TST rotations/convolutions with voxel zooms
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* tensor - H^(x)m, and per-TST rotations/convolutions with voxel zooms       */
/* ========================================================================== */
/* Two things in one command, because they are the same object seen twice.
 *
 * FIRST, THE ALGEBRA CHECKS ITSELF. Every structural claim in canor_tensor.h is
 * asserted here rather than assumed, and the assertions are the ones that
 * would fail if a sign were wrong: dim = 4^m, associativity on every sampled
 * triple, the Jacobiator identically zero, a centre of dimension one, and an
 * explicit zero divisor in H (x) H. A module that only ever agreed with itself
 * would not be worth linking.
 *
 * THE JACOBI RESULT IS THE POINT OF CONTRAST. In canor_excite the Jacobiator
 * vanishes on 105 of 1365 channel-triples; here it vanishes on all of them,
 * because the commutator of an associative algebra is always a Lie algebra.
 * So the tensor direction supplies exact associative structure to compute
 * WITH, and the Cayley-Dickson direction supplies the graded failure to
 * measure. Running the relative-bias probe over a tensor algebra would find no
 * structure at all - not because the Strategies were unbiased, but because the
 * instrument would have no gradient. That is worth knowing before anyone tries
 * it.
 *
 * SECOND, THE FIVE TST TREES ROTATE AND CONVOLVE. The spread gives five
 * closing quaternion subalgebras, so the five charts of the selmer atlas are
 * exactly five H factors and H^(x)5 has dimension 4^5 = 1024. Each chart's
 * measured axis is rotated by a rational quaternion acting on that factor, the
 * five results are convolved along the chart axis, and the outcome is binned
 * into k-d voxels at a ladder of zoom levels.
 *
 * EVERYTHING STAYS EXACT, AND THE COST IS STATED. A rational quaternion gives
 * a conjugation entirely in Q - no trigonometry, nothing to round. The price
 * is that only rotations with rational direction cosines are reachable; an
 * arbitrary angle would need a declared approximation policy, and none is
 * assumed here. The voxel zoom is an arithmetic shift rather than a division
 * by a scale, so it is exact at every level and composes: two zooms of one
 * equal one zoom of two, which a rounding division does not.
 */

static int tcheck_fail, tcheck_ran;
static void tcheck(int ok, const char *what) {
  ++tcheck_ran;
  printf("    %-58s %s\n", what, ok ? "ok" : "FAILED");
  if (!ok) ++tcheck_fail;
}

int canor_demo_cmd_tensor(const canor_demo_config_t *c, int sel, const char *out) {
  ex_groups spread;
  ft_census cen;
  int m, u, t, k, lvl;
  FILE *f;
  canor_rational axis[CANOR_TRIFECTAS][3], rot[CANOR_TRIFECTAS][3];
  canor_rational conv[CANOR_TRIFECTAS * 2];
  int nconv = 0;

  tcheck_fail = tcheck_ran = 0;
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;

  printf("H^(x)m - generalized quaternionic tensor product algebras\n\n");

  /* --- dimensions --------------------------------------------------------- */
  printf("  dimension ladder\n");
  for (m = 1; m <= 6; ++m)
    printf("    m=%d   dim %llu%s\n", m, (unsigned long long)ft_dim(m),
           m == 1 ? "   H" : m == 2 ? "   H(x)H = M_4(R)" : "");
  tcheck(ft_dim(1) == 4 && ft_dim(2) == 16 && ft_dim(5) == 1024,
         "dim(H^(x)m) = 4^m");
  tcheck(ft_dim(0) == 0 && ft_dim(FT_MAX_FACTORS + 1) == 0,
         "out-of-range m is refused, not clamped");

  /* --- structure ---------------------------------------------------------- */
  printf("\n  structure, checked rather than assumed\n");
  for (m = 1; m <= 3; ++m) {
    char label[80];
    if (!ft_census_run(m, 64, &cen)) { printf("    census failed\n"); return 2; }
    snprintf(label, sizeof label, "m=%d associative on all %ld sampled triples",
             m, cen.triples);
    tcheck(cen.assoc_fail == 0, label);
    snprintf(label, sizeof label, "m=%d Jacobiator identically zero", m);
    tcheck(cen.jacobi_fail == 0, label);
    snprintf(label, sizeof label, "m=%d centre is 1-dimensional (central simple)", m);
    tcheck(cen.central == 1, label);
  }

  /* THE CONTRAST, MEASURED SIDE BY SIDE rather than asserted in a comment. */
  {
    ex_census ec;
    if (ex_census_run(16, &ec)) {
      printf("\n  the two directions from H, on the same question\n");
      printf("    Cayley-Dickson dim 16   Jacobi fails on %ld of %ld triples\n",
             ec.jacobi_fail, ec.triples);
      if (ft_census_run(2, 16, &cen))
        printf("    tensor        dim 16   Jacobi fails on %ld of %ld triples\n",
               cen.jacobi_fail, cen.triples);
      tcheck(ec.jacobi_fail > 0 && cen.jacobi_fail == 0,
             "same dimension, opposite behaviour - the contrast is real");
    }
  }

  /* --- a zero divisor, exhibited ------------------------------------------- */
  /* (1(x)1 - i(x)i)(1(x)1 + i(x)i) = 1(x)1 - (i.i)(x)(i.i) = 1(x)1 - 1(x)1 = 0.
   * H is a division algebra and H(x)H is not, so the tensor direction loses
   * division exactly where it keeps associativity. Exhibited rather than
   * stated, because a claimed zero divisor that does not multiply to zero is
   * the easiest kind of wrong to miss. */
  {
    ft_term p11, pii, cross1, cross2;
    uint32_t E = 0u;            /* 1 (x) 1 */
    uint32_t I = 1u | (1u << 2); /* i (x) i */
    bool ok = ft_mul(2, E, E, &p11) && ft_mul(2, I, I, &pii) &&
              ft_mul(2, E, I, &cross1) && ft_mul(2, I, E, &cross2);
    printf("\n  H(x)H is not a division algebra\n");
    if (ok) {
      printf("    (1(x)1 - i(x)i)(1(x)1 + i(x)i)\n");
      printf("      1(x)1 . 1(x)1 = %+d e_%u\n", p11.coeff, p11.unit);
      printf("      i(x)i . i(x)i = %+d e_%u   (both factors square to -1)\n",
             pii.coeff, pii.unit);
      printf("      cross terms   = %+d e_%u and %+d e_%u, which cancel\n",
             cross1.coeff, cross1.unit, cross2.coeff, cross2.unit);
      /* The product is p11 - pii on e_0 plus the cancelling cross terms. */
      tcheck(p11.unit == 0 && pii.unit == 0 && p11.coeff - pii.coeff == 0,
             "1(x)1 - i(x)i is a zero divisor");
      /* (A-B)(A+B) = A^2 + AB - BA - B^2. The cross terms cancel only if they
       * are the SAME term: same unit and equal coefficient, so AB - BA = 0.
       * Both conjuncts are needed - an earlier form of this check used `||`
       * with a disjunct that is true by construction, which passed without
       * establishing anything. */
      tcheck(cross1.unit == cross2.unit && cross1.coeff - cross2.coeff == 0,
             "AB - BA = 0, so the cross terms genuinely cancel");
    }
  }

  /* --- rational rotation --------------------------------------------------- */
  printf("\n  exact rational rotation, one quaternion per chart\n");
  {
    /* Rational unit quaternions. (1,1,1,1)/2 is the order-3 rotation cycling
     * the axes; (3,4,0,0)/5 is a Pythagorean rotation about x. Both have
     * rational direction cosines, so the conjugation never leaves Q. */
    static const int64_t QW[CANOR_TRIFECTAS][4] = {
      { 1, 0, 0, 0 },  /* identity - the control */
      { 1, 1, 1, 1 },  /* order 3: x->y->z->x */
      { 3, 4, 0, 0 },
      { 0, 1, 0, 0 },  /* pi about x */
      { 5, 0, 12, 0 }
    };
    for (t = 0; t < CANOR_TRIFECTAS && t < spread.count; ++t) {
      ft_quat q;
      q.w = canor_rat(QW[t][0], 1); q.x = canor_rat(QW[t][1], 1);
      q.y = canor_rat(QW[t][2], 1); q.z = canor_rat(QW[t][3], 1);
      /* The chart's axis is its three units' indices - a real coordinate of
       * the chart, not an arbitrary vector. */
      for (k = 0; k < 3; ++k) axis[t][k] = canor_rat(spread.u[t][k], 1);
      if (!ft_rotate3(&q, axis[t], rot[t])) {
        printf("    chart c%d: rotation refused\n", t);
        ++tcheck_fail;
        continue;
      }
      printf("    c%d  q=(%lld,%lld,%lld,%lld)  (%lld,%lld,%lld) -> "
             "(%lld/%lld, %lld/%lld, %lld/%lld)\n", t,
             (long long)QW[t][0], (long long)QW[t][1],
             (long long)QW[t][2], (long long)QW[t][3],
             (long long)axis[t][0].num, (long long)axis[t][1].num,
             (long long)axis[t][2].num,
             (long long)rot[t][0].num, (long long)rot[t][0].den,
             (long long)rot[t][1].num, (long long)rot[t][1].den,
             (long long)rot[t][2].num, (long long)rot[t][2].den);
    }
    /* A rotation is an isometry, so the squared length is invariant. That is
     * the check that catches a wrong sign in the Hamilton product, which a
     * plausible-looking output would not. */
    for (t = 0; t < CANOR_TRIFECTAS && t < spread.count; ++t) {
      canor_rational n0 = canor_rat(0, 1), n1 = canor_rat(0, 1);
      char label[80];
      for (k = 0; k < 3; ++k) {
        n0 = canor_rat_add(n0, canor_rat_mul(axis[t][k], axis[t][k]));
        n1 = canor_rat_add(n1, canor_rat_mul(rot[t][k], rot[t][k]));
      }
      snprintf(label, sizeof label, "c%d rotation preserves |v|^2 exactly", t);
      tcheck(canor_rat_eq(n0, n1), label);
    }
    tcheck(canor_rat_eq(rot[0][0], axis[0][0]) && canor_rat_eq(rot[0][2], axis[0][2]),
           "the identity quaternion is the identity rotation");
  }

  /* --- convolution along the chart axis ------------------------------------ */
  printf("\n  convolution along the chart axis\n");
  {
    canor_rational fx[CANOR_TRIFECTAS], gx[CANOR_TRIFECTAS];
    int n = (spread.count < CANOR_TRIFECTAS) ? spread.count : CANOR_TRIFECTAS;
    for (t = 0; t < n; ++t) { fx[t] = rot[t][0]; gx[t] = rot[t][1]; }
    if (ft_convolve(fx, n, gx, n, conv, CANOR_TRIFECTAS * 2)) {
      nconv = 2 * n - 1;
      printf("    (x * y)[n] over %d charts, %d taps\n", n, nconv);
      for (t = 0; t < nconv; ++t)
        printf("      [%d] %lld/%lld\n", t,
               (long long)conv[t].num, (long long)conv[t].den);
      tcheck(nconv == 2 * n - 1, "convolution length is nf + ng - 1");
    } else {
      printf("    convolution refused\n");
      ++tcheck_fail;
    }
  }

  /* --- k-d voxel zoom ------------------------------------------------------ */
  printf("\n  k-d voxel zoom ladder\n");
  {
    int64_t key[CANOR_KD], vox[CANOR_KD], prev[CANOR_KD];
    bool compose_ok = true;
    key[0] = 1000; key[1] = -1000; key[2] = 12345;
    printf("    key (%lld, %lld, %lld)\n",
           (long long)key[0], (long long)key[1], (long long)key[2]);
    for (lvl = 0; lvl <= 8; lvl += 2) {
      ft_voxel(key, lvl, vox);
      printf("      level %d   voxel (%lld, %lld, %lld)\n", lvl,
             (long long)vox[0], (long long)vox[1], (long long)vox[2]);
    }
    /* Composition: zooming by 1 twice must equal zooming by 2. This is the
     * property an arithmetic shift has and a division towards zero does not,
     * and it is the reason for the shift. */
    ft_voxel(key, 1, prev);
    ft_voxel(prev, 1, vox);
    ft_voxel(key, 2, prev);
    for (k = 0; k < CANOR_KD; ++k) if (vox[k] != prev[k]) compose_ok = false;
    tcheck(compose_ok, "two zooms of 1 equal one zoom of 2");
    key[0] = -1;
    ft_voxel(key, 1, vox);
    tcheck(vox[0] == -1, "negative coordinates floor, not truncate toward zero");
  }

  /* --- the artifact -------------------------------------------------------- */
  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); return 2; }
  fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n");
  fprintf(f, "<title>Quaternionic tensor product algebras</title>\n");
  fprintf(f, "<style>body{background:#0d1117;color:#c9d1d9;"
             "font:14px/1.6 system-ui,sans-serif;margin:0;padding:32px}"
             ".w{max-width:1100px;margin:0 auto}h1{font-size:20px;margin:0 0 4px}"
             "h2{font-size:15px;margin:28px 0 8px;color:#e6edf3}"
             ".sub{color:#8b949e;font-size:13px;max-width:72ch}"
             "table{border-collapse:collapse;font:12px ui-monospace,monospace}"
             "td,th{border:1px solid #30363d;padding:3px 8px;text-align:right}"
             "th{color:#8b949e;font-weight:500}"
             "td:first-child,th:first-child{text-align:left}"
             "svg{background:#010409;border:1px solid #30363d;border-radius:6px}"
             "</style>\n<div class=\"w\">\n");
  fprintf(f, "<h1>Generalized quaternionic tensor product algebras</h1>\n");
  fprintf(f, "<p class=\"sub\">H<sup>&otimes;m</sup> over <b>Q</b>, and the five "
             "spread charts of Strategy <b>%s</b> rotated, convolved and "
             "voxelized. Every value below is an exact rational.</p>\n",
          c->strat[sel].id.name);

  fprintf(f, "<h2>The two directions from H</h2>\n<table>\n");
  fprintf(f, "<tr><th></th><th>Cayley&ndash;Dickson</th><th>tensor</th></tr>\n");
  fprintf(f, "<tr><td>dimension</td><td>2<sup>n</sup></td><td>4<sup>m</sup></td></tr>\n");
  fprintf(f, "<tr><td>associative</td><td>only to dim 4</td><td>always</td></tr>\n");
  fprintf(f, "<tr><td>Jacobi</td><td>fails from dim 8</td><td>holds identically</td></tr>\n");
  fprintf(f, "<tr><td>division algebra</td><td>to dim 8</td><td>no, from H&otimes;H</td></tr>\n");
  fprintf(f, "<tr><td>zero divisors</td><td>from dim 16</td><td>from dim 16</td></tr>\n");
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">They fail oppositely, and that is what makes them "
             "complementary rather than alternative. The relative-bias probe "
             "reads the Cayley&ndash;Dickson side because its graded failure is "
             "the gradient the probe measures; over a tensor algebra the "
             "Jacobiator vanishes everywhere and the instrument would have no "
             "structure to report &mdash; which would look like an absence of "
             "bias and would not be one.</p>\n");

  fprintf(f, "<h2>Chart rotations, exact in Q</h2>\n<table>\n");
  fprintf(f, "<tr><th>chart</th><th>units</th><th>rotated x</th>"
             "<th>rotated y</th><th>rotated z</th></tr>\n");
  for (t = 0; t < CANOR_TRIFECTAS && t < spread.count; ++t)
    fprintf(f, "<tr><td>c%d</td><td>%s %s %s</td><td>%lld/%lld</td>"
               "<td>%lld/%lld</td><td>%lld/%lld</td></tr>\n", t,
            canor_imag_name(spread.u[t][0]), canor_imag_name(spread.u[t][1]),
            canor_imag_name(spread.u[t][2]),
            (long long)rot[t][0].num, (long long)rot[t][0].den,
            (long long)rot[t][1].num, (long long)rot[t][1].den,
            (long long)rot[t][2].num, (long long)rot[t][2].den);
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">Conjugation by a rational quaternion, "
             "v &rarr; qvq&#772;/|q|&sup2;. No trigonometry enters, so nothing "
             "rounds. The restriction is real and is stated rather than hidden: "
             "only rotations with rational direction cosines are reachable, and "
             "an arbitrary angle would need a declared approximation "
             "policy.</p>\n");

  fprintf(f, "<h2>Convolution along the chart axis</h2>\n<table>\n<tr><th>tap</th>"
             "<th>value</th></tr>\n");
  for (t = 0; t < nconv; ++t)
    fprintf(f, "<tr><td>%d</td><td>%lld/%lld</td></tr>\n", t,
            (long long)conv[t].num, (long long)conv[t].den);
  fprintf(f, "</table>\n");

  fprintf(f, "<h2>Voxel zoom</h2>\n");
  fprintf(f, "<p class=\"sub\">An arithmetic shift of the k-d key, not a "
             "division by a scale: exact at every level, and composing exactly "
             "&mdash; two zooms of one equal one zoom of two. A division "
             "towards zero would break both, and would quietly merge two "
             "adjacent regions on the negative side of each axis.</p>\n");
  fprintf(f, "</div>\n");
  fclose(f);

  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  /* Counted, not asserted: a refused rotation skips its own checks, and a
   * fixed literal here would report a total the run did not perform. */
  printf("\n%d checks, %d failures\n", tcheck_ran, tcheck_fail);
  return tcheck_fail ? 1 : 0;
}
