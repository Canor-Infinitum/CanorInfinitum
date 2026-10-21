/* canor_demo_graph.c - graph - step baseline overlaid with the exact rational interpolant
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* graph - step baseline overlaid with the exact rational interpolant         */
/* ========================================================================== */
/* Two curves, drawn together because neither alone tells the truth.
 *
 *   THE STEP PLOT is the operator. C(n) and S(n) are defined on integers and
 *   on nothing else; between two indices the sequence has no value, and a
 *   piecewise-constant trace is the honest picture of that.
 *
 *   THE POLYNOMIAL is a reading aid. For period p there is a unique degree
 *   p-1 polynomial through the p points of one period, and its coefficients
 *   are RATIONAL - so the smooth curve is exact rather than fitted, and it
 *   agrees with the operator at every integer. Between integers it is an
 *   interpolation and claims nothing.
 *
 * Drawing only the smooth curve would misrepresent a discrete sequence as a
 * continuous function; drawing only the steps would hide the structure the eye
 * is there to find. So both, always, with the steps drawn on top.
 *
 * The polynomial is evaluated in EXACT rationals through fb_rat, then converted
 * to a coordinate at the last moment. Evaluating in floating point would put
 * rounding inside the one artifact whose whole claim is exactness - and the
 * error would be invisible, because a slightly wrong curve still looks like a
 * curve.
 *
 * PERIODIC EXTENSION. A degree p-1 polynomial diverges outside its period, so
 * the index is reduced modulo p before evaluation and the interpolant is
 * repeated. That matches the operator, which is genuinely periodic there.
 *
 * INDEX TO ANGLE: n |-> n . 2pi/p, so one period is one turn and the frame
 * -2pi..+2pi holds four of them. 360 samples per turn, as specified.
 */

#define GRAPH_PER_TURN 360
#define GRAPH_TURNS 4

/* Lagrange value at x = num/den, exactly. y[] are the period's sample values. */
static bool graph_lagrange(const canor_rational *y, int p, long num, long den,
                           fb_rat *out) {
  fb_rat term, acc, x, node, diff, w;
  int i, k;
  bool ok = true;

  fb_rat_init(&term); fb_rat_init(&acc); fb_rat_init(&x);
  fb_rat_init(&node); fb_rat_init(&diff); fb_rat_init(&w);
  if (!fb_rat_set_i64(&acc, 0, 1) || !fb_rat_set_i64(&x, num, den)) ok = false;

  for (i = 0; ok && i < p; ++i) {
    if (!fb_rat_set_i64(&term, y[i].num, y[i].den)) { ok = false; break; }
    for (k = 0; ok && k < p; ++k) {
      if (k == i) continue;
      /* (x - k) / (i - k) */
      if (!fb_rat_set_i64(&node, k, 1) ||
          !fb_rat_sub(&diff, &x, &node) ||
          !fb_rat_set_i64(&w, i - k, 1)) { ok = false; break; }
      {
        fb_rat inv;
        fb_rat_init(&inv);
        ok = fb_copy(&inv.num, &w.den) && fb_copy(&inv.den, &w.num) &&
             fb_rat_normalize(&inv) &&
             fb_rat_mul(&diff, &diff, &inv) &&
             fb_rat_mul(&term, &term, &diff);
        fb_rat_free(&inv);
      }
    }
    if (ok && !fb_rat_add(&acc, &acc, &term)) ok = false;
  }
  if (ok) ok = fb_rat_copy(out, &acc);
  fb_rat_free(&term); fb_rat_free(&acc); fb_rat_free(&x);
  fb_rat_free(&node); fb_rat_free(&diff); fb_rat_free(&w);
  return ok;
}

/* Rational -> double, at the very last step and nowhere earlier. */
static double graph_to_double(const fb_rat *q) {
  int64_t fixed;
  if (!fb_rat_to_fixed(&q->num, &q->den, 20, &fixed)) return 0.0;
  return (double)fixed / (double)(1 << 20);
}

/* Draws one operator panel. Split out so the per-strategy loop below reads as
 * a loop rather than as three copies. */
static void graph_operator_svg(FILE *f, const canor_ore *o, int op,
                               const char *opn) {
  canor_rational y[20];
  int p, i, j, npts = 0;
  double lo = 0.0, hi = 0.0;
  const int W = 880, H = 190, PADL = 46, PADR = 12, PADT = 12, PADB = 24;
  int plotw = W - PADL - PADR, ploth = H - PADT - PADB;

  p = canor_ore_period(o);
  if (p <= 0) {
    /* NO PERIOD IS NOT NO CURVE. The interpolant needs a period to close over,
     * but the closed form of the recurrence does not - and an aperiodic
     * operator is exactly where the skew envelope becomes visible, so bailing
     * out here left the one interesting case blank. Draw the analytic
     * continuation alone, with no step trace and no interpolant, and say which
     * is which. */
    double av = (double)o->tau.num / (double)o->tau.den;
    double qv = (double)o->sigma.num / (double)o->sigma.den;
    double disc = av * av - 4.0 * qv;
    double span = (double)(p > 0 ? p : 6);
    int jj;
    char ab[32], qb[32];
    canor_demo_rat_show(o->tau, ab, sizeof ab);
    canor_demo_rat_show(o->sigma, qb, sizeof qb);

    if (!(disc < 0.0 && qv > 0.0)) {
      fprintf(f, "<h3>%s<sub>S</sub> &nbsp; &tau;=%s &sigma;=%s</h3><p class=\"sub\">%s: the "
                 "roots of x&sup2;&minus;&tau;x+&sigma; are real, so the closed form is a "
                 "sum of exponentials with no oscillation to draw.</p>\n",
              opn, ab, qb,
              canor_ore_degenerate(o) ? "degenerate (a&sup2;=4q, repeated root)"
                                   : "aperiodic, real roots");
      return;
    }
    {
      double rmod = sqrt(qv);
      double cth = av / (2.0 * rmod), th, sth, s0v;
      if (cth > 1.0) cth = 1.0;
      if (cth < -1.0) cth = -1.0;
      th = acos(cth);
      sth = sin(th);
      s0v = (double)o->delta_amp.num / (double)o->delta_amp.den;
      fprintf(f, "<h3>%s<sub>S</sub> &nbsp; &tau;=%s &sigma;=%s &nbsp; TWISTED, no period "
                 "&mdash; |r|=%.3f per step</h3>\n", opn, ab, qb, rmod);
      fprintf(f, "<p class=\"sub\">Complex roots, so the closed form exists and is "
                 "drawn; there is no period to interpolate over, so no red curve "
                 "and no step trace. The envelope &sigma;^(t/2) grows by %.3f per step "
                 "&mdash; that geometric amplitude IS the twist, and it is why the "
                 "orbit is unbounded and therefore aperiodic.</p>\n", rmod);
      /* Signed log: y = sign(v).log10(1+|v|). Keeps the small-t oscillation
       * legible while the envelope, which is exponential, straightens out. */
      lo = -log10(1.0 + pow(rmod, span * 4.0));
      hi = -lo;
      fprintf(f, "<svg viewBox=\"0 0 %d %d\" role=\"img\">\n", W, H);
      {
        double zy = PADT + ploth * (hi - 0.0) / (hi - lo);
        fprintf(f, "<line class=\"axis\" x1=\"%d\" y1=\"%.1f\" x2=\"%d\" y2=\"%.1f\"/>\n",
                PADL, zy, W - PADR, zy);
      }
      for (jj = 0; jj <= GRAPH_PER_TURN * GRAPH_TURNS; ++jj) { (void)jj; break; }
      fprintf(f, "<polyline class=\"cont\" points=\"");
      for (jj = 0; jj <= GRAPH_PER_TURN * GRAPH_TURNS; ++jj) {
        double t = (double)span * 4.0 * jj / (GRAPH_PER_TURN * GRAPH_TURNS);
        double raw = (op == 0)
            ? (sth != 0.0 ? s0v * pow(rmod, t - 1.0) * sin(t * th) / sth : 0.0)
            : pow(rmod, t) * cos(t * th);
        double val = (raw < 0 ? -1.0 : 1.0) * log10(1.0 + fabs(raw));
        double x = PADL + (double)plotw * jj / (GRAPH_PER_TURN * GRAPH_TURNS);
        double yy;
        if (val < lo) val = lo;
        if (val > hi) val = hi;
        yy = PADT + ploth * (hi - val) / (hi - lo);
        fprintf(f, "%.2f,%.2f ", x, yy);
      }
      fprintf(f, "\"/>\n");
      {
        int sgn;
        for (sgn = -1; sgn <= 1; sgn += 2) {
          fprintf(f, "<polyline class=\"env\" points=\"");
          for (jj = 0; jj <= GRAPH_PER_TURN * GRAPH_TURNS; ++jj) {
            double t = (double)span * 4.0 * jj / (GRAPH_PER_TURN * GRAPH_TURNS);
            double raw = pow(rmod, t) * (op == 0 ? fabs(s0v) / rmod : 1.0);
            double val = sgn * log10(1.0 + raw);
            double x = PADL + (double)plotw * jj / (GRAPH_PER_TURN * GRAPH_TURNS);
            double yy;
            if (val < lo) val = lo;
            if (val > hi) val = hi;
            yy = PADT + ploth * (hi - val) / (hi - lo);
            fprintf(f, "%.2f,%.2f ", x, yy);
          }
          fprintf(f, "\"/>\n");
        }
      }
      /* Integer samples, which still exist even though the sequence never
       * repeats - the operator is defined at every n regardless of period. */
      for (jj = 0; jj <= (int)span * 4; ++jj) {
        double t = (double)jj;
        double raw = (op == 0)
            ? (sth != 0.0 ? s0v * pow(rmod, t - 1.0) * sin(t * th) / sth : 0.0)
            : pow(rmod, t) * cos(t * th);
        double val = (raw < 0 ? -1.0 : 1.0) * log10(1.0 + fabs(raw));
        double x = PADL + (double)plotw * jj / (span * 4.0);
        double yy;
        if (val < lo) val = lo;
        if (val > hi) val = hi;
        yy = PADT + ploth * (hi - val) / (hi - lo);
        fprintf(f, "<circle class=\"dot\" cx=\"%.2f\" cy=\"%.2f\" r=\"2.6\"/>\n", x, yy);
      }
      fprintf(f, "</svg>\n");
      return;
    }
  }
  for (i = 0; i < p; ++i) {
    bool ok;
    switch (op) {
      case 0: ok = canor_ore_sin(o, i, &y[i]); break;
      case 1: ok = canor_ore_cos(o, i, &y[i]); break;
      default: ok = canor_ore_tan(o, i, &y[i]); break;
    }
    if (!ok || !canor_rat_valid(y[i])) y[i] = canor_rat(0, 1);
    else ++npts;
  }
  /* The wrap node: a polynomial through one period is NOT periodic, so
   * repeating it would jump at every boundary. Forcing P(p) = P(0) closes it. */
  y[p] = y[0];

  for (i = 0; i < p; ++i) {
    double v = (double)y[i].num / (double)y[i].den;
    if (i == 0 || v < lo) lo = v;
    if (i == 0 || v > hi) hi = v;
  }
  /* Scale over the CURVE too: the interpolant overshoots between samples, and
   * clamping it into the sample range would draw a plateau that is not data. */
  for (j = 0; j <= GRAPH_PER_TURN * GRAPH_TURNS; ++j) {
    long num = (long)p * j - 2L * p * GRAPH_PER_TURN, den = GRAPH_PER_TURN;
    long modnum = num % ((long)p * den);
    fb_rat v;
    if (modnum < 0) modnum += (long)p * den;
    fb_rat_init(&v);
    if (graph_lagrange(y, p + 1, modnum, den, &v)) {
      double val = graph_to_double(&v);
      if (val < lo) lo = val;
      if (val > hi) hi = val;
    }
    fb_rat_free(&v);
  }
  if (hi - lo < 1e-9) { lo -= 1.0; hi += 1.0; }
  { double span = hi - lo; lo -= span * 0.10; hi += span * 0.10; }

  {
    char ab[32], qb[32], sb[32];
    canor_demo_rat_show(o->tau, ab, sizeof ab);
    canor_demo_rat_show(o->sigma, qb, sizeof qb);
    canor_demo_rat_show(o->delta_amp, sb, sizeof sb);
    fprintf(f, "<h3>%s<sub>S</sub> &nbsp; &tau;=%s &sigma;=%s &delta;-amp=%s &nbsp; period %d</h3>\n",
            opn, ab, qb, sb, p);
  }
  fprintf(f, "<svg viewBox=\"0 0 %d %d\" role=\"img\">\n", W, H);
  {
    double zy = PADT + ploth * (hi - 0.0) / (hi - lo);
    static const char *const tick[5] =
      { "-2\xcf\x80", "-\xcf\x80", "0", "\xcf\x80", "2\xcf\x80" };
    fprintf(f, "<line class=\"axis\" x1=\"%d\" y1=\"%.1f\" x2=\"%d\" y2=\"%.1f\"/>\n",
            PADL, zy, W - PADR, zy);
    for (i = 0; i <= GRAPH_TURNS; ++i) {
      double gx = PADL + (double)plotw * i / GRAPH_TURNS;
      fprintf(f, "<line class=\"grid\" x1=\"%.1f\" y1=\"%d\" x2=\"%.1f\" y2=\"%d\"/>\n",
              gx, PADT, gx, H - PADB);
      fprintf(f, "<text class=\"lab\" x=\"%.1f\" y=\"%d\" text-anchor=\"middle\">%s</text>\n",
              gx, H - 7, tick[i]);
    }
    fprintf(f, "<text class=\"lab\" x=\"4\" y=\"%.1f\">0</text>\n", zy + 3);
  }
  fprintf(f, "<polyline class=\"poly\" points=\"");
  for (j = 0; j <= GRAPH_PER_TURN * GRAPH_TURNS; ++j) {
    long num = (long)p * j - 2L * p * GRAPH_PER_TURN, den = GRAPH_PER_TURN;
    long modnum = num % ((long)p * den);
    fb_rat v;
    double val, x, yy;
    if (modnum < 0) modnum += (long)p * den;
    fb_rat_init(&v);
    if (!graph_lagrange(y, p + 1, modnum, den, &v)) { fb_rat_free(&v); continue; }
    val = graph_to_double(&v);
    fb_rat_free(&v);
    x = PADL + (double)plotw * j / (GRAPH_PER_TURN * GRAPH_TURNS);
    yy = PADT + ploth * (hi - val) / (hi - lo);
    fprintf(f, "%.2f,%.2f ", x, yy);
  }
  fprintf(f, "\"/>\n");

  /* --- the analytic continuation, and its envelope ------------------------
   * The recurrence is linear with characteristic polynomial x^2 - a.x + q, so
   * the sequence has a CLOSED FORM and that closed form is the honest
   * continuous object: an interpolant merely passes through the samples, while
   * this IS the operator extended to real argument.
   *
   * With complex conjugate roots r = sqrt(q).e^{i.theta}, and using
   * cos(theta) = a / (2.sqrt(q)) which is exactly the initial condition
   * C(1) = a/2, the cosine solution has no sine component at all:
   *
   *     C(t) = q^{t/2} . cos(t.theta)
   *     S(t) = s0 . q^{(t-1)/2} . sin(t.theta) / sin(theta)
   *
   * THE ENVELOPE q^{t/2} IS THE SKEW. This is where a skew operator differs
   * from an ordinary one, and it is not a change of period: it is a geometric
   * amplitude, so the orbit and every partial sum of it scale by q^{1/2} per
   * step. Periodicity fails at q != 1 because BOUNDEDNESS fails, and period is
   * the symptom rather than the property. Drawing the envelope puts that on
   * the picture instead of leaving it in a footnote.
   *
   * Floating point appears here and only here - the continuation needs cos,
   * sin and a fractional power, which the exact core has no business carrying.
   * It is a render-layer curve, drawn dashed to say so. */
  {
    double av = (double)o->tau.num / (double)o->tau.den;
    double qv = (double)o->sigma.num / (double)o->sigma.den;
    double s0v = (double)o->delta_amp.num / (double)o->delta_amp.den;
    double disc = av * av - 4.0 * qv;
    if (disc < 0.0 && qv > 0.0) {
      double rmod = sqrt(qv);
      double cth = av / (2.0 * rmod);
      double th, sth;
      if (cth > 1.0) cth = 1.0;
      if (cth < -1.0) cth = -1.0;
      th = acos(cth);
      sth = sin(th);
      fprintf(f, "<polyline class=\"cont\" points=\"");
      for (j = 0; j <= GRAPH_PER_TURN * GRAPH_TURNS; ++j) {
        double t = -2.0 * p + (double)p * j / GRAPH_PER_TURN;
        double amp = pow(rmod, t);
        double val = (op == 0)
            ? (sth != 0.0 ? s0v * pow(rmod, t - 1.0) * sin(t * th) / sth : 0.0)
            : amp * cos(t * th);
        double x = PADL + (double)plotw * j / (GRAPH_PER_TURN * GRAPH_TURNS);
        double yy;
        if (op == 2) continue;              /* tan is a ratio; no single mode */
        if (val < lo) val = lo;
        if (val > hi) val = hi;
        yy = PADT + ploth * (hi - val) / (hi - lo);
        fprintf(f, "%.2f,%.2f ", x, yy);
      }
      fprintf(f, "\"/>\n");
      if (fabs(qv - 1.0) > 1e-12 && op != 2) {
        int sgn;
        for (sgn = -1; sgn <= 1; sgn += 2) {
          fprintf(f, "<polyline class=\"env\" points=\"");
          for (j = 0; j <= GRAPH_PER_TURN * GRAPH_TURNS; ++j) {
            double t = -2.0 * p + (double)p * j / GRAPH_PER_TURN;
            double val = sgn * pow(rmod, t) * (op == 0 ? fabs(s0v) / rmod : 1.0);
            double x = PADL + (double)plotw * j / (GRAPH_PER_TURN * GRAPH_TURNS);
            double yy;
            if (val < lo) val = lo;
            if (val > hi) val = hi;
            yy = PADT + ploth * (hi - val) / (hi - lo);
            fprintf(f, "%.2f,%.2f ", x, yy);
          }
          fprintf(f, "\"/>\n");
        }
      }
    }
  }
  fprintf(f, "<polyline class=\"step\" points=\"");
  for (j = -2 * p; j <= 2 * p; ++j) {
    int idx = ((j % p) + p) % p;
    double val = (double)y[idx].num / (double)y[idx].den;
    double x0 = PADL + (double)plotw * (j + 2 * p) / (4.0 * p);
    double x1 = PADL + (double)plotw * (j + 1 + 2 * p) / (4.0 * p);
    double yy = PADT + ploth * (hi - val) / (hi - lo);
    fprintf(f, "%.2f,%.2f %.2f,%.2f ", x0, yy, x1, yy);
  }
  fprintf(f, "\"/>\n");
  for (j = -2 * p; j <= 2 * p; ++j) {
    int idx = ((j % p) + p) % p;
    double val = (double)y[idx].num / (double)y[idx].den;
    double x = PADL + (double)plotw * (j + 2 * p) / (4.0 * p);
    double yy = PADT + ploth * (hi - val) / (hi - lo);
    fprintf(f, "<circle class=\"dot\" cx=\"%.2f\" cy=\"%.2f\" r=\"3.4\"/>"
               "<circle class=\"dot-in\" cx=\"%.2f\" cy=\"%.2f\" r=\"1.3\"/>\n",
            x, yy, x, yy);
  }
  fprintf(f, "</svg>\n<table><tr><th>n</th>");
  for (i = 0; i < p; ++i) fprintf(f, "<th>%d</th>", i);
  fprintf(f, "</tr><tr><th>%s</th>", opn);
  for (i = 0; i < p; ++i) {
    char b[32];
    canor_demo_rat_show(y[i], b, sizeof b);
    fprintf(f, "<td>%s</td>", b);
  }
  fprintf(f, "</tr></table>\n");
  if (npts < p)
    fprintf(f, "<p class=\"sub\">%d of %d indices are poles and are shown as "
               "zero; tan is UNDEFINED there, not zero.</p>\n", p - npts, p);
}

/* Per-strategy. Every configured Strategy gets its own section, preceded by a
 * comparison table - because the question these graphs answer is not "what
 * does one operator look like" but "which of these Strategies is a usable
 * instrument", and that is a comparison. */
int canor_demo_cmd_graph(const canor_demo_config_t *c, int sel, const char *emit, long want) {
  static const char *const opn[3] = { "sin", "cos", "tan" };
  FILE *f;
  char path[600];
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0;
  int si, op;
  double qual_of[CANOR_DEMO_CFG_STRAT_MAX], cap_of[CANOR_DEMO_CFG_STRAT_MAX];

  snprintf(path, sizeof path, "%s", emit ? emit : "operators.html");
  f = fopen(path, "w");
  if (!f) { fprintf(stderr, "follow: cannot write %s\n", path); return 2; }
  if (!canor_demo_cloud_get(c, sel, NULL, want, &pts, &n)) { fclose(f); return 2; }

  fprintf(f, "<title>Ore operators - all strategies</title>\n");
  fprintf(f, "<style>\n"
             ":root{color-scheme:light dark}\n"
             "body{font:14px/1.5 ui-monospace,SFMono-Regular,Menlo,monospace;"
             "margin:0;padding:24px;background:#fbfbfc;color:#1a1a1c}\n"
             "@media(prefers-color-scheme:dark){body{background:#0f1012;color:#e8e8ea}}\n"
             ".panel{max-width:940px;margin:0 auto}\n"
             "h1{font-size:17px;margin:0 0 4px}\n"
             "h2{font-size:15px;margin:30px 0 2px;padding-top:14px;"
             "border-top:1px solid currentColor;border-color:color-mix(in srgb,currentColor 18%%,transparent)}\n"
             "h3{font-size:13px;margin:16px 0 4px;font-weight:600}\n"
             ".sub{opacity:.65;font-size:12px;margin:0 0 12px}\n"
             "svg{width:100%%;height:auto;display:block}\n"
             ".axis{stroke:currentColor;stroke-opacity:.28;stroke-width:1}\n"
             ".grid{stroke:currentColor;stroke-opacity:.10;stroke-width:1}\n"
             ".poly{fill:none;stroke:#c02040;stroke-width:2;stroke-opacity:.95}\n"
             ".step{fill:none;stroke:currentColor;stroke-opacity:.55;stroke-width:1.5}\n"
             ".dot{fill:none;stroke:#c02040;stroke-width:1.6}\n"
             ".dot-in{fill:currentColor;fill-opacity:.9}\n"
             ".cont{fill:none;stroke:#2a7fd4;stroke-width:1.6;stroke-dasharray:5 3;stroke-opacity:.95}\n"
             ".env{fill:none;stroke:#2a7fd4;stroke-width:1;stroke-dasharray:2 4;stroke-opacity:.5}\n"
             ".lab{font-size:11px;fill:currentColor;fill-opacity:.6}\n"
             "table{border-collapse:collapse;font-size:12px;margin-top:6px}\n"
             "td,th{padding:2px 12px 2px 0;text-align:left;white-space:nowrap}\n"
             "table.cmp th{border-bottom:1px solid currentColor;"
             "border-color:color-mix(in srgb,currentColor 22%%,transparent)}\n"
             ".blind{color:#c02040;font-weight:600}\n"
             ".bar{display:inline-block;height:9px;background:#c02040;"
             "vertical-align:middle;border-radius:2px}\n"
             "</style>\n");
  fprintf(f, "<div class=\"panel\">\n<h1>Ore skew-trigonometric operators, per strategy</h1>\n");
  fprintf(f, "<p class=\"sub\">Step trace = the operator, defined on integers only. "
             "Red curve = the exact rational interpolant, closed over its period so "
             "the repetition is continuous, at %d samples per turn. They agree at "
             "every integer; between integers only the steps are claims. Blue dashed = the ANALYTIC CONTINUATION q^(t/2)&middot;cos(t&theta;), the closed form of the recurrence extended to real argument; its faint envelope q^(t/2) is the skew &mdash; at q&ne;1 the orbit and its partial sums scale geometrically, and periodicity fails because BOUNDEDNESS fails.</p>\n",
          GRAPH_PER_TURN);

  /* Comparison first. */
  for (si = 0; si < c->nstrat; ++si) {
    canor_strategy sL = c->strat[si], sR = c->strat[si];
    canor_demo_qual_t q, pq;
    long defined = 0, separated = 0, i2;
    memset(&pq, 0, sizeof pq);
    canor_demo_qual_measure_ore(c, si, &pq);
    canor_demo_qual_measure(c, &sL, pts, n, pq.periods_distinct, pq.period_lcm, &q);
    sL.bias = LeftBiased;
    sR.bias = RightBiased;
    for (i2 = 0; i2 < n; ++i2) {
      char word[CANOR_FLOW_WORD_MAX];
      int cnt[3], len;
      int32_t args[4];
      FLOWVALUE l[3], r[3];
      len = canor_demo_cloud_word(c, &pts[i2], word, CANOR_FLOW_WORD_MAX, cnt);
      if (len == 0) continue;
      args[0] = pts[i2].x; args[1] = pts[i2].y; args[2] = pts[i2].z; args[3] = pts[i2].a;
      if (!canor_di(&sL, args, 4, &l[0]) || !canor_dj(&sL, args, 4, &l[1]) ||
          !canor_dk(&sL, args, 4, &l[2])) continue;
      if (!canor_di(&sR, args, 4, &r[0]) || !canor_dj(&sR, args, 4, &r[1]) ||
          !canor_dk(&sR, args, 4, &r[2])) continue;
      ++defined;
      if (l[0] != r[0] || l[1] != r[1] || l[2] != r[2]) ++separated;
    }
    qual_of[si] = q.quality;
    cap_of[si] = defined ? 100.0 * (double)separated / (double)defined : 0.0;
  }

  fprintf(f, "<table class=\"cmp\"><tr><th>strategy</th><th>bias</th>"
             "<th>operators</th><th>periods</th><th>Quality</th>"
             "<th>Capacity</th><th>QxC/2</th><th></th></tr>\n");
  for (si = 0; si < c->nstrat; ++si) {
    fprintf(f, "<tr><td>%s%s</td><td>%s</td><td>%s</td><td>", c->strat[si].id.name,
            si == sel ? " *" : "", canor_demo_bias_name(c->strat[si].bias),
            c->strat_use_ore[si] ? "ore" : "hash");
    if (c->strat_use_ore[si])
      for (op = 0; op < 3; ++op) {
        int per = canor_ore_period(&c->strat_ore[si][op]);
        fprintf(f, "%s%d", op ? "/" : "", per);
      }
    else fprintf(f, "&mdash;");
    fprintf(f, "</td><td>%.1f%%</td><td%s>%.1f%%</td><td>%.1f</td>"
               "<td><span class=\"bar\" style=\"width:%.0fpx\"></span></td></tr>\n",
            qual_of[si], cap_of[si] <= 0.0 ? " class=\"blind\"" : "",
            cap_of[si], qual_of[si] * cap_of[si] * 0.5 / 100.0,
            qual_of[si] * cap_of[si] * 0.5 / 100.0 * 4.0);
  }
  fprintf(f, "</table>\n<p class=\"sub\">Capacity 0 does not mean unbiased. It "
             "means BLIND: that Strategy cannot express a Left/Right distinction, "
             "so any model measured through it reads as unbiased whatever it does. "
             "* marks the selected Strategy.</p>\n");

  /* Then a section per strategy. */
  for (si = 0; si < c->nstrat; ++si) {
    fprintf(f, "<h2>%s &nbsp;v%u &nbsp;&middot;&nbsp; bias %s</h2>\n",
            c->strat[si].id.name, (unsigned)c->strat[si].id.version,
            canor_demo_bias_name(c->strat[si].bias));
    {
      char dbuf[192];
      int n, live = c->strat[si].ntrifecta * 3;
      fprintf(f, "<p class=\"sub\">");
      for (n = 0; n < live && n < CANOR_DEMO_CFG_DNAMES; ++n)
        fprintf(f, "%s %s<br>", canor_demo_cfg_dname[n],
                canor_demo_cfg_dshow(c, si, n, dbuf, sizeof dbuf));
      fprintf(f, "</p>\n");
    }
    if (!c->strat_use_ore[si]) {
      /* Drawing the default Ore curves here would show a structure this
       * Strategy does not have. It uses the hash operators, which have no
       * period and no interpolant - so the panel says so instead. */
      fprintf(f, "<p class=\"sub\">This Strategy uses the <b>hash</b> operators. "
                 "They are reproducible but carry no period, no invariant and no "
                 "interpolant, so there is nothing to graph &mdash; drawing the "
                 "default Ore curves here would show structure it does not have. "
                 "Set <code>\"operators\"</code> to get a graph.</p>\n");
      continue;
    }
    for (op = 0; op < 3; ++op)
      graph_operator_svg(f, &c->strat_ore[si][op], op, opn[op]);
  }

  fprintf(f, "</div>\n");
  fclose(f);
  free(pts);
  printf("wrote %s  (%d strategies)\n", path, c->nstrat);
  canor_demo_prov_write_sidecar(path);
  return 0;
}
