/* canor_demo_mandel.c - mandel - the fifth schema, benchmarked
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* mandel - the fifth schema, benchmarked                                     */
/* ========================================================================== */
/* Two magnitude comparisons over the same points: exact rationals with every
 * multiply checked for overflow, and Q32 fixed point. The fifth schema is
 * DERIVED from whether they agree, not declared:
 *
 *   escaped       both say |z|^2 > 4 within fuel
 *   bounded       both say it does not
 *   boundary      they disagree - the point is close enough to the frontier
 *                 that finite precision decides differently from exact
 *   undetermined  the exact path overflowed and could not decide at all
 *
 * The grid is DYADIC (coordinates k/64, denominators are powers of two). That
 * is the fairest case for the exact path: an arbitrary denominator overflows
 * sooner, so a dyadic grid measures the method rather than the grid. */

typedef enum { ESCAPED, BOUNDED, BOUNDARY, UNDETERMINED } escape_class;
static const char *const eclass_name[4] =
  { "escaped", "bounded", "boundary", "undetermined" };
static const char eclass_char[4] = { '.', '#', '+', '?' };

#define Q 32
#define ONE (1LL << Q)

/* --- the exact path, in bounded precision ----------------------------------
 * No overflow is possible; the limit is the per-value limb budget (at most
 * CANOR_BIG_LIMBS_MAX), and hitting it is reported rather than absorbed.
 * `bits` reports the widest value the orbit held, which is what the sweep is
 * really measuring: how fast exactness costs width. */
static size_t g_value_limbs = CANOR_BIG_LIMBS_MAX;

void canor_demo_set_value_limbs(size_t limbs) {
  g_value_limbs = (limbs == 0 || limbs > CANOR_BIG_LIMBS_MAX) ? CANOR_BIG_LIMBS_MAX : limbs;
}
size_t canor_demo_value_limbs(void) { return g_value_limbs; }

int canor_demo_escape_exact(const canor_big_rat_t *cr, const canor_big_rat_t *ci, int fuel,
                        int *steps, int *bits) {
  canor_big_rat_t zr, zi, zr2, zi2, mag, nr, ni, two;
  int i, res = 0;
  canor_big_rat_init(&zr); canor_big_rat_init(&zi); canor_big_rat_init(&zr2); canor_big_rat_init(&zi2);
  canor_big_rat_init(&mag); canor_big_rat_init(&nr); canor_big_rat_init(&ni); canor_big_rat_init(&two);
  *bits = 0;
  *steps = 0;
  if (canor_big_rat_set_i64(&zr, 0, 1) != CANOR_OK || canor_big_rat_set_i64(&zi, 0, 1) != CANOR_OK ||
      canor_big_rat_set_i64(&two, 2, 1) != CANOR_OK) res = -1;
  for (i = 0; res == 0 && i < fuel; ++i) {
    int b;
    if (canor_big_rat_mul(&zr2, &zr, &zr) != CANOR_OK || canor_big_rat_mul(&zi2, &zi, &zi) != CANOR_OK ||
        canor_big_rat_add(&mag, &zr2, &zi2) != CANOR_OK) { res = -1; break; }
    if (canor_big_rat_cmp_i64(&mag, 4, NULL) > 0) { res = 1; break; }
    /* z <- (zr^2 - zi^2 + cr) + (2 zr zi + ci) i */
    if (canor_big_rat_sub(&nr, &zr2, &zi2) != CANOR_OK || canor_big_rat_add(&nr, &nr, cr) != CANOR_OK ||
        canor_big_rat_mul(&ni, &zr, &zi) != CANOR_OK || canor_big_rat_mul(&ni, &ni, &two) != CANOR_OK ||
        canor_big_rat_add(&ni, &ni, ci) != CANOR_OK) { res = -1; break; }
    zr = nr;
    zi = ni;
    b = canor_big_rat_bits(&zr);
    { int bi = canor_big_rat_bits(&zi); if (bi > b) b = bi; }
    if (b > *bits) *bits = b;
    if ((size_t)b > g_value_limbs * CANOR_BIG_LIMB_BITS) { res = -1; break; }
  }
  *steps = i;
  return res;
}

/* --- the fixed-width path, unchanged --------------------------------------- */
static int64_t qmul(int64_t a, int64_t b) { return (int64_t)(((__int128)a * b) >> Q); }
static int escape_fixed(int64_t cr, int64_t ci, int fuel) {
  int64_t zr = 0, zi = 0;
  for (int i = 0; i < fuel; ++i) {
    int64_t zr2 = qmul(zr, zr), zi2 = qmul(zi, zi), nr, ni;
    if (zr2 + zi2 > 4LL * ONE) return 1;
    nr = zr2 - zi2 + cr;
    ni = qmul(2 * zr, zi) + ci;
    zr = nr; zi = ni;
  }
  return 0;
}

static escape_class classify(int ex, int fx) {
  if (ex < 0) return UNDETERMINED;     /* the exact path ran out of budget */
  if (ex == 1 && fx == 1) return ESCAPED;
  if (ex == 0 && fx == 0) return BOUNDED;
  return BOUNDARY;                     /* exact and finite precision disagree */
}

/* --- frame-relative grid ---------------------------------------------------
 * A point is
 *     c = centre + span * (2x + 1 - w) / (2w)
 * so the sample sits at the CENTRE of its cell rather than its corner - the
 * corner convention puts samples on the frame edge, where a zoom would then
 * resample the same coordinates at two magnifications and appear to be stable
 * when it is not.
 *
 * The whole expression is exact: the frame is rational, the offset is
 * rational, and no step leaves the field. That is what makes a deep zoom
 * meaningful here - the grid does not lose the precision the orbit is about to
 * spend. */
static bool frame_point(canor_big_rat_t *out, const canor_big_rat_t *centre, const canor_big_rat_t *span,
                        int idx, int n) {
  canor_big_rat_t off;
  bool ok;
  canor_big_rat_init(&off);
  ok = (canor_big_rat_set_i64(&off, 2 * (int64_t)idx + 1 - n, 2 * (int64_t)n) == CANOR_OK) &&
       (canor_big_rat_mul(&off, &off, span) == CANOR_OK) &&
       (canor_big_rat_add(out, centre, &off) == CANOR_OK);
  return ok;
}

typedef struct { long n[4]; int maxbits; int maxsteps; } pass_result;

static bool mandel_pass(const canor_demo_config_t *c, int fuel, pass_result *pr,
                        char *canvas) {
  const int w = c->set.grid_w, h = c->set.grid_h;
  canor_big_rat_t cr, ci;
  bool ok = true;
  int x, y;
  memset(pr, 0, sizeof *pr);
  canor_big_rat_init(&cr); canor_big_rat_init(&ci);
  for (y = 0; y < h && ok; ++y) {
    if (!frame_point(&ci, &c->frame_ci, &c->frame_si, y, h)) { ok = false; break; }
    for (x = 0; x < w; ++x) {
      int steps = 0, bits = 0, ex, fx = 0;
      int64_t qr = 0, qi = 0;
      escape_class k;
      if (!frame_point(&cr, &c->frame_cr, &c->frame_sr, x, w)) { ok = false; break; }
      ex = canor_demo_escape_exact(&cr, &ci, fuel, &steps, &bits);
      /* The fixed path needs the same point in Q32. Deep inside a zoom the
       * coordinate no longer fits 32 fractional bits, and that is precisely
       * when the two paths should be expected to disagree - so a projection
       * failure is reported as a disagreement, not hidden. */
      if ((canor_big_rat_to_fixed(&cr.num, &cr.den, Q, &qr) == CANOR_OK) &&
          (canor_big_rat_to_fixed(&ci.num, &ci.den, Q, &qi) == CANOR_OK))
        fx = escape_fixed(qr, qi, fuel);
      else
        fx = ex == 1 ? 0 : 1;
      k = classify(ex, fx);
      pr->n[k]++;
      if (bits > pr->maxbits) pr->maxbits = bits;
      if (steps > pr->maxsteps) pr->maxsteps = steps;
      if (canvas) canvas[(size_t)y * w + x] = eclass_char[k];
    }
  }
  return ok;
}

/* --- describe: the n-definition ladder applied to real orbits ---------------
 * The Kapprox classes are description lengths as a function of n:
 *
 *     General      2n + 1          linear
 *     Specific     2(n!) + 1       factorial
 *     Distinctive  (2n + 1)!       factorial of linear
 *     Arbitrary    (2(n!) + 1)!    factorial of factorial
 *
 * and canor_kapprox_order_milli reports each as log2 of log2, so that Arbitrary
 * stays a number rather than an overflow. An exact orbit has a description
 * length too, and it is not a metaphor: the widest rational the orbit holds is
 * literally how many bits are needed to write the state down at step n. So the
 * two are on the same scale and can be compared without inventing a mapping.
 *
 * A point is assigned the HIGHEST class whose predicted order it reaches. That
 * direction matters. Taking the nearest rung would let a point that comfortably
 * exceeds General be called General because it also fell short of Specific;
 * taking the highest reached says what the orbit demonstrably costs and
 * declines to credit it with more.
 *
 * What this measures is not the Mandelbrot set. It is how expensive each point
 * is to know exactly - and those are different questions, which is the
 * interesting part: escaping cheaply and being outside the set are not the
 * same property, and the map below shows where they come apart. */
static int describe_class(int bits, int n) {
  int64_t observed;
  int k, best = 0;
  if (bits <= 0) return 0;                      /* nothing to write down */
  observed = canor_log2_milli((uint64_t)bits);
  for (k = 0; k < CANOR_DEMO_AXIS_N; ++k)
    if (canor_kapprox_order_milli((canor_kclass)k, (uint64_t)n) <= observed) best = k;
  return best;
}

int canor_demo_cmd_describe(const canor_demo_config_t *c) {
  const int w = c->set.grid_w, h = c->set.grid_h, n = c->set.fuel;
  const long total = (long)w * h;
  long cls[CANOR_DEMO_AXIS_N] = { 0, 0, 0, 0 };
  long esc[CANOR_DEMO_AXIS_N] = { 0, 0, 0, 0 };
  canor_big_rat_t cr, ci;
  char *canvas;
  int x, y, i;
  bool ok = true;

  canvas = (char *)malloc((size_t)total);
  if (!canvas) { fprintf(stderr, "follow: out of memory\n"); return 2; }

  printf("description-length classes over the frame, fuel %d\n\n", n);
  printf("  %-14s %-14s %14s\n", "class", "Kapprox", "order at n");
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i)
    printf("  %-14s %-14s %14lld\n", c->meta[i].name, canor_demo_kclass_name(c->meta[i].kclass),
           (long long)canor_kapprox_order_milli(c->meta[i].kclass, (uint64_t)n));
  printf("\n");

  canor_big_rat_init(&cr); canor_big_rat_init(&ci);
  for (y = 0; y < h && ok; ++y) {
    if (!frame_point(&ci, &c->frame_ci, &c->frame_si, y, h)) { ok = false; break; }
    for (x = 0; x < w; ++x) {
      int steps = 0, bits = 0, ex, k;
      if (!frame_point(&cr, &c->frame_cr, &c->frame_sr, x, w)) { ok = false; break; }
      ex = canor_demo_escape_exact(&cr, &ci, n, &steps, &bits);
      /* Always at the full n, never at the point's own stopping step. Scoring
       * an orbit that escaped at step 1 against the ladder at n=1 compares it
       * to rungs that are all tiny there, and every such point comes out
       * Arbitrary - which reads as "maximally complex" when what actually
       * happened is that it finished immediately. A point that stops is a
       * point whose description is COMPLETE, and a complete short description
       * is the definition of General, not of Arbitrary. */
      k = ex < 0 ? CANOR_DEMO_AXIS_N - 1 : describe_class(bits, n);
      ++cls[k];
      if (ex == 1) ++esc[k];
      canvas[(size_t)y * w + x] = "GSDA"[k];
    }
  }
  if (!ok) { free(canvas); fprintf(stderr, "follow: budget exhausted\n"); return 2; }

  printf("  %-14s %8s %8s %10s\n", "class", "points", "share", "of those,");
  printf("  %-14s %8s %8s %10s\n", "", "", "", "escaped");
  for (i = 0; i < CANOR_DEMO_AXIS_N; ++i)
    printf("  %-14s %8ld %7.1f%% %9ld  '%c'\n", c->meta[i].name, cls[i],
           100.0 * cls[i] / total, esc[i], "GSDA"[i]);
  printf("\n");
  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) putchar(canvas[(size_t)y * w + x]);
    putchar('\n');
  }
  free(canvas);
  return 0;
}

int canor_demo_cmd_mandel(const canor_demo_config_t *c) {
  const int w = c->set.grid_w, h = c->set.grid_h;
  const long total = (long)w * h;
  pass_result pr;
  char fbuf[256];
  char *canvas;
  int f, map_fuel = 1, y, x;

  printf("frame       centre ");
  canor_big_rat_format(&c->frame_cr, fbuf, sizeof fbuf); printf("%s", fbuf);
  canor_big_rat_format(&c->frame_ci, fbuf, sizeof fbuf); printf(" %s i\n", fbuf);
  printf("            span   ");
  canor_big_rat_format(&c->frame_sr, fbuf, sizeof fbuf); printf("%s", fbuf);
  canor_big_rat_format(&c->frame_si, fbuf, sizeof fbuf); printf(" x %s\n", fbuf);
  printf("grid        %d x %d, cell centres, exact rational\n", w, h);
  printf("budget      %lu limbs per value (%lu bits)\n\n",
         (unsigned long)canor_demo_value_limbs(),
         (unsigned long)(canor_demo_value_limbs() * CANOR_BIG_LIMB_BITS));

  printf("fuel sweep - exact rationals, so the cost is width, not overflow\n");
  printf("  %5s %9s %9s %9s %9s %8s\n",
         "fuel", "escaped", "bounded", "boundary", "undet.", "maxbits");
  for (f = 1; f <= c->set.fuel; ++f) {
    if (!mandel_pass(c, f, &pr, NULL)) {
      printf("  %5d  (budget exhausted before the pass completed)\n", f);
      break;
    }
    printf("  %5d %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8d\n", f,
           100.0 * pr.n[0] / total, 100.0 * pr.n[1] / total,
           100.0 * pr.n[2] / total, 100.0 * pr.n[3] / total, pr.maxbits);
    /* Deepest fuel still mostly decidable. Choosing the fuel with the fewest
     * undetermined would trivially choose 1, where nothing has had a chance to
     * exhaust anything and nothing has escaped either. */
    if (pr.n[3] * 2 < total) map_fuel = f;
  }

  canvas = (char *)malloc((size_t)total);
  if (!canvas) { fprintf(stderr, "follow: out of memory\n"); return 2; }
  memset(canvas, ' ', (size_t)total);
  if (!mandel_pass(c, map_fuel, &pr, canvas)) {
    free(canvas);
    fprintf(stderr, "follow: budget exhausted rendering the map\n");
    return 2;
  }

  printf("\nfifth schema over %ld points, fuel %d\n", total, map_fuel);
  for (f = 0; f < 4; ++f)
    printf("  %-14s %5ld  %5.1f%%   '%c'\n", eclass_name[f], pr.n[f],
           100.0 * pr.n[f] / total, eclass_char[f]);
  printf("  widest orbit value    %d bits\n", pr.maxbits);
  printf("  deepest orbit         %d steps\n\n", pr.maxsteps);

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) putchar(canvas[(size_t)y * w + x]);
    putchar('\n');
  }
  free(canvas);
  return 0;
}
