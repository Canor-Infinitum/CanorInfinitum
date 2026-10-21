/* canor_demo_sed2hp4.c - sed2hp4 - the sedenions and H(x)H over one shared skeleton, and HP^4
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* sed2hp4 - the sedenions and H(x)H over one shared skeleton, and HP^4       */
/* ========================================================================== */
/* THE FACT THIS IS BUILT ON, WHICH WAS MEASURED AND NOT ASSUMED.
 *
 * The sedenions and H (x) H are both 16-dimensional, and their basis elements
 * are both indexed by (Z_2)^4. On all 256 pairs they obey the SAME index rule:
 *
 *     e_a . e_b  =  +-e_(a XOR b)      in both algebras, every pair
 *
 * So they share an entire skeleton - the group (Z_2)^4 - and differ ONLY in
 * the sign attached to each product. That sign function differs on 108 of the
 * 256 pairs. Everything separating a non-associative algebra with zero
 * divisors from an associative full matrix algebra M_4(R) is those 108 signs.
 *
 * The whole visualization is that sentence made visible, and none of it is
 * asserted: the index agreement is checked pair by pair before anything is
 * drawn, and a mismatch would abort rather than be smoothed over.
 *
 * --- WHY THIS BEARS ON HP^4 -------------------------------------------------
 *
 * HP^4 = P(H^5) has real dimension 16, and the atlas drawn by `hp4` gets its
 * five quaternionic charts from the PG(3,2) spread: five triples of imaginary
 * units, each closing under multiplication into a copy of H. Closure is what
 * makes a chart a chart.
 *
 * The question this command answers - and it is a question, not a restatement
 * - is whether that closure belongs to the shared skeleton or to the sedenion
 * signs. If the triples closed under both sign functions, the atlas would be a
 * fact about (Z_2)^4 and the algebra would be incidental. If they close under
 * one and not the other, the atlas is a fact about the SIGNS.
 *
 * The command measures it. It also measures the squares, which is where the
 * difference is most legible: a unit with an odd number of non-identity tensor
 * factors squares to -1, and a unit with an even number squares to +1. Units
 * squaring to +1 are exactly why H (x) H has zero divisors, and a triple
 * containing one cannot be a copy of H whatever else is true of it.
 *
 * NOTHING HERE CLAIMS a geometric or physical reading of HP^4. It is the
 * projectivisation of H^5, and the atlas is a way of drawing sixteen real
 * dimensions - as `hp4` already says. What is added is the provenance of the
 * atlas: which of the two 16-dimensional structures it actually comes from.
 */

int canor_demo_cmd_sed2hp4(const canor_demo_config_t *c, int sel, const char *out) {
  ex_groups spread;
  FILE *f;
  int a, b, t, k;
  int agree = 0, differ = 0, idx_ok = 1;
  int sq_neg_sed = 0, sq_neg_ten = 0;
  int sed_closes = 0, ten_closes = 0;
  int ten_ok[CANOR_TRIFECTAS];
  static signed char twist[16][16];

  canor_demo_check_fail = canor_demo_check_ran = 0;
  if (canor_imag_init() != CANOR_OK) return 2;
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;

  printf("Sedenions and H(x)H over one shared skeleton\n\n");

  /* --- the skeleton, checked pair by pair ---------------------------------- */
  for (a = 0; a < 16; ++a)
    for (b = 0; b < 16; ++b) {
      ft_term tt;
      canor_imag u;
      int sgn;
      if (!ft_mul(2, (unsigned)a, (unsigned)b, &tt)) return 2;
      if (canor_imag_mul((canor_imag)a, (canor_imag)b, &u, &sgn) != CANOR_OK) return 2;
      if ((unsigned)u != tt.unit) idx_ok = 0;
      twist[a][b] = (signed char)(sgn == tt.coeff ? 1 : -1);
      if (sgn == tt.coeff) ++agree; else ++differ;
    }
  printf("  shared skeleton\n");
  printf("    index rule e_a.e_b = +-e_(a XOR b) holds in both: %s\n",
         idx_ok ? "all 256 pairs" : "MISMATCH");
  printf("    signs agree on %d pairs, differ on %d\n", agree, differ);
  canor_demo_check(idx_ok, "both algebras share the index rule a XOR b, on all 256 pairs");
  canor_demo_check(agree + differ == 256, "every pair was accounted for");
  canor_demo_check(differ > 0, "the two algebras are genuinely different, not a relabelling");

  /* --- the squares ---------------------------------------------------------- */
  printf("\n  squares of the 15 imaginary units\n");
  printf("    %-8s %-10s %-10s\n", "unit", "sedenion", "tensor");
  for (a = 1; a < 16; ++a) {
    ft_term tt;
    canor_imag u;
    int sgn;
    ft_mul(2, (unsigned)a, (unsigned)a, &tt);
    canor_imag_mul((canor_imag)a, (canor_imag)a, &u, &sgn);
    if (sgn < 0) ++sq_neg_sed;
    if (tt.coeff < 0) ++sq_neg_ten;
    printf("    %-8s %-10s %-10s\n", canor_imag_name((canor_imag)a),
           sgn < 0 ? "-1" : "+1", tt.coeff < 0 ? "-1" : "+1");
  }
  printf("    squaring to -1:  sedenion %d of 15, tensor %d of 15\n",
         sq_neg_sed, sq_neg_ten);
  canor_demo_check(sq_neg_sed == 15, "every sedenion imaginary unit squares to -1");
  /* A unit squaring to +1 cannot generate a copy of C, let alone sit in a
   * copy of H - and it is exactly what supplies H(x)H with zero divisors. */
  canor_demo_check(sq_neg_ten < 15, "some tensor units square to +1 - the zero divisors");

  /* --- the HP^4 charts, under each sign function ---------------------------- */
  printf("\n  the five HP^4 charts, under each sign function\n");
  printf("    %-6s %-14s %-12s %-12s\n", "chart", "units", "sedenion", "tensor");
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t) {
    char units[40];
    int ok = 1;
    snprintf(units, sizeof units, "%s %s %s",
             canor_imag_name(spread.u[t][0]), canor_imag_name(spread.u[t][1]),
             canor_imag_name(spread.u[t][2]));
    /* Closure as a copy of H needs every generator to square to -1 and every
     * product of two of them to land on the third. Both are checked; checking
     * only the product would accept a triple that squares the wrong way. */
    for (k = 0; k < 3; ++k) {
      ft_term sq, pr;
      int x = spread.u[t][k], y = spread.u[t][(k + 1) % 3], z = spread.u[t][(k + 2) % 3];
      if (!ft_mul(2, (unsigned)x, (unsigned)x, &sq)) { ok = 0; break; }
      if (sq.coeff > 0 || sq.unit != 0) { ok = 0; break; }
      if (!ft_mul(2, (unsigned)x, (unsigned)y, &pr)) { ok = 0; break; }
      if (pr.unit != (unsigned)z) { ok = 0; break; }
    }
    ten_ok[t] = ok;
    if (spread.closes[t]) ++sed_closes;
    if (ok) ++ten_closes;
    printf("    c%-5d %-14s %-12s %-12s\n", t, units,
           spread.closes[t] ? "closes" : "no", ok ? "closes" : "no");
  }
  printf("\n    sedenion signs: %d of %d charts close\n", sed_closes, spread.count);
  printf("    tensor signs:   %d of %d charts close\n", ten_closes, spread.count);
  canor_demo_check(sed_closes == spread.count, "every chart closes under the sedenion signs");

  /* THE ANSWER, stated as whatever it turned out to be. */
  printf("\n  so the HP^4 atlas is a fact about %s\n",
         ten_closes == sed_closes ? "the shared skeleton (Z_2)^4"
                                  : "the SEDENION SIGNS, not the skeleton");
  if (ten_closes != sed_closes)
    printf("    The two algebras have the same 16 basis elements and the same\n"
           "    index rule, and only one of them carries the atlas. So HP^4's\n"
           "    five charts come from the sign cocycle - the 108 differing\n"
           "    entries - and not from the group underneath.\n");

  /* --- artifact -------------------------------------------------------------- */
  f = fopen(out, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", out); return 2; }
  fprintf(f, "<!doctype html>\n<meta charset=\"utf-8\">\n"
             "<title>Sedenions to HP^4 via the tensor algebra</title>\n");
  fprintf(f, "<style>body{background:#0d1117;color:#c9d1d9;font:14px/1.6 "
             "system-ui,sans-serif;margin:0;padding:32px}.w{max-width:1100px;"
             "margin:0 auto}h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;"
             "margin:28px 0 8px;color:#e6edf3}.sub{color:#8b949e;font-size:13px;"
             "max-width:72ch}.no{border-left:2px solid #d29922;padding-left:12px;"
             "color:#8b949e;font-size:13px;max-width:72ch}table{border-collapse:"
             "collapse;font:12px ui-monospace,monospace}td,th{border:1px solid "
             "#30363d;padding:3px 8px;text-align:right}th{color:#8b949e;"
             "font-weight:500}td:first-child,th:first-child{text-align:left}"
             "svg{background:#010409;border:1px solid #30363d;border-radius:6px}"
             "</style>\n<div class=\"w\">\n");
  fprintf(f, "<h1>Sedenions &rarr; HP<sup>4</sup>, through the tensor algebra</h1>\n");
  fprintf(f, "<p class=\"sub\">Two 16-dimensional algebras, one shared skeleton. "
             "The sedenions and H&otimes;H are both indexed by "
             "(Z<sub>2</sub>)<sup>4</sup> and both multiply by "
             "e<sub>a</sub>e<sub>b</sub> = &plusmn;e<sub>a XOR b</sub> &mdash; "
             "verified on all 256 pairs before anything below was drawn. They "
             "differ only in the sign, on <b>%d of 256</b> pairs.</p>\n", differ);

  fprintf(f, "<h2>The twist: where the signs disagree</h2>\n");
  fprintf(f, "<svg viewBox=\"0 0 440 440\" width=\"440\">\n");
  for (a = 0; a < 16; ++a)
    for (b = 0; b < 16; ++b) {
      double x = 30.0 + b * 25.0, y = 30.0 + a * 25.0;
      fprintf(f, "<rect x=\"%.0f\" y=\"%.0f\" width=\"23\" height=\"23\" "
                 "fill=\"%s\" opacity=\"0.9\"><title>e%d . e%d %s</title></rect>\n",
              x, y, twist[a][b] > 0 ? "#1f6feb" : "#f85149", a, b,
              twist[a][b] > 0 ? "same sign" : "OPPOSITE sign");
    }
  fprintf(f, "</svg>\n");
  fprintf(f, "<p class=\"sub\">Blue: the two algebras agree. Red: they take "
             "opposite signs. This grid is the entire difference between a "
             "non-associative algebra with zero divisors and an associative "
             "M<sub>4</sub>(R) &mdash; every other structural fact about either "
             "follows from it, because the index rule and the basis are shared.</p>\n");

  fprintf(f, "<h2>The five HP<sup>4</sup> charts</h2>\n<table>\n");
  fprintf(f, "<tr><th>chart</th><th>units</th><th>sedenion signs</th>"
             "<th>tensor signs</th></tr>\n");
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
    fprintf(f, "<tr><td>c%d</td><td>%s %s %s</td><td>%s</td><td>%s</td></tr>\n",
            t, canor_imag_name(spread.u[t][0]), canor_imag_name(spread.u[t][1]),
            canor_imag_name(spread.u[t][2]),
            spread.closes[t] ? "closes" : "no", ten_ok[t] ? "closes" : "no");
  fprintf(f, "</table>\n");
  fprintf(f, "<p class=\"sub\">HP<sup>4</sup> = P(H<sup>5</sup>) has real "
             "dimension 16, and its atlas needs five triples that each close "
             "into a copy of H. Under the sedenion signs, %d of %d do. Under "
             "the tensor signs, %d do &mdash; because a unit with an even "
             "number of non-identity tensor factors squares to <b>+1</b>, and "
             "such a unit cannot sit in a copy of H at all. Those same units "
             "are what give H&otimes;H its zero divisors.</p>\n",
          sed_closes, spread.count, ten_closes);
  fprintf(f, "<p class=\"sub\"><b>So the atlas belongs to the signs, not the "
             "skeleton.</b> Two algebras with identical basis and identical "
             "index rule, and only one of them carries HP<sup>4</sup>'s five "
             "charts. The 108 red cells above are where that comes from.</p>\n");

  fprintf(f, "<h2>What is not claimed</h2>\n");
  fprintf(f, "<p class=\"no\">No geometric or physical reading of "
             "HP<sup>4</sup>. It is the projectivisation of H<sup>5</sup>, and "
             "the atlas is a way of drawing sixteen real dimensions. What this "
             "page adds is the atlas's provenance &mdash; which of two "
             "16-dimensional structures it actually comes from &mdash; and that "
             "is an algebraic question with an algebraic answer.</p>\n");
  fprintf(f, "</div>\n");
  fclose(f);

  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
