#ifndef CANOR_SYM_H
#define CANOR_SYM_H

/* Generalised symbolic graphing for the type tower.
 *
 * WHAT WAS MISSING. canor_expression is a CONTAINER - units holding reals,
 * intervals and tensors. It has no variable and no evaluation at a point, so
 * nothing in the tower could be graphed as a function of anything. Every
 * plot so far has been a hand-written loop over one specific operator, which
 * meant the axis choice was hand-made too, and made once per plot.
 *
 * This adds the missing piece: a small symbolic term with ONE variable,
 * evaluated in the tower's own exact rationals, plus the thing that actually
 * generalises - a measured choice of axis.
 *
 * THE AXIS IS MEASURED, NOT DECLARED. This is the whole point. A series is
 * sampled, its growth is classified, and the transform follows from the class:
 *
 *   CANOR_GROWTH_BOUNDED   |f| stays in a band            -> linear axis
 *   CANOR_GROWTH_POLY      |f| grows like a power of t    -> linear, or log-log
 *   CANOR_GROWTH_GEOMETRIC |f| multiplies by r per step   -> signed log
 *   CANOR_GROWTH_DOUBLE    log|f| itself multiplies       -> log-log
 *
 * Those four are not arbitrary: they are exactly the cases this system has
 * already run into by hand. The Ore operators at sigma = 1 are BOUNDED and
 * were drawn linear. At sigma != 1 they are GEOMETRIC, and a linear axis
 * clipped the whole orbit into a square wave until it was drawn on a signed
 * log. The exact-rational Mandelbrot is DOUBLE - its denominator width doubles
 * per iteration, 6 -> 12 -> 24 -> 48 - which is why it dies at three steps and
 * why the honest axis for it is log-log. Each of those was diagnosed
 * separately and fixed separately; classifying growth turns three ad-hoc
 * decisions into one rule.
 *
 * ON THE MANDELBROT AXIS SPECIFICALLY. |z_n| for an escaping orbit grows like
 * |z|^(2^n), so log log |z_n| is linear in n - that is the Douady-Hubbard
 * potential, up to scale. CANOR_GROWTH_DOUBLE with the log-log transform IS that
 * potential; it is not an approximation of it. And no higher-dimensional
 * embedding is needed to see it: q^2 = a^2 - |v|^2 + 2a.v preserves the
 * imaginary DIRECTION, so a Cayley-Dickson orbit stays in the 2-plane
 * span{1, v} at every dimension, and the quaternionic set is the plane set
 * revolved. There is no extra structure for a projective lowering to carry.
 */

#include <stdbool.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANOR_SYM_MAX 64

typedef enum {
  CANOR_SYM_CONST = 0,
  CANOR_SYM_VAR,          /* the single variable, t */
  CANOR_SYM_ADD,
  CANOR_SYM_SUB,
  CANOR_SYM_MUL,
  CANOR_SYM_DIV,
  CANOR_SYM_NEG,
  CANOR_SYM_POW           /* integer exponent in `k`, base in `a` */
} canor_sym_op;

/* A term is an index into the pool, so a whole expression is one flat array -
 * no pointers to chase and no ownership to get wrong, and it copies by value. */
typedef struct {
  canor_sym_op op;
  canor_rational value;   /* CONST */
  int a, b;            /* child indices; -1 when unused */
  int k;               /* POW exponent */
} canor_sym_term;

typedef struct {
  canor_sym_term t[CANOR_SYM_MAX];
  int n;
  int root;
} canor_sym;

void canor_sym_init(canor_sym *e);
int canor_sym_const(canor_sym *e, canor_rational v);        /* returns term index, -1 full */
int canor_sym_var(canor_sym *e);
int canor_sym_bin(canor_sym *e, canor_sym_op op, int a, int b);
int canor_sym_neg(canor_sym *e, int a);
int canor_sym_pow(canor_sym *e, int a, int k);
void canor_sym_set_root(canor_sym *e, int idx);

/* Evaluates at t. Returns the tower's poison (den == 0) on overflow or on
 * division by zero, so failure propagates instead of producing a number. */
canor_rational canor_sym_eval(const canor_sym *e, canor_rational t);

/* --- growth classification and axes ---------------------------------------- */

typedef enum {
  CANOR_GROWTH_BOUNDED = 0,
  CANOR_GROWTH_POLY,
  CANOR_GROWTH_GEOMETRIC,
  CANOR_GROWTH_DOUBLE,
  CANOR_GROWTH_UNDEFINED       /* the series could not be evaluated far enough */
} canor_growth;

typedef enum {
  CANOR_AXIS_LINEAR = 0,
  CANOR_AXIS_SIGNED_LOG,
  CANOR_AXIS_LOGLOG            /* the Mandelbrot/potential axis */
} canor_axis;

const char *canor_growth_name(canor_growth g);
const char *canor_axis_name(canor_axis a);

typedef struct {
  canor_growth growth;
  canor_axis axis;
  int samples, valid;
  /* WIDTH of the widest rational representation, in bits. This is the
   * feasibility cost, NOT the growth of the value - a bounded function can
   * have an unbounded representation, and classifying on this instead of on
   * magnitude is what made 1/(t^2+1) read as doubly exponential. */
  int64_t max_bits;
  int64_t ratio_milli;      /* per-step growth of log2|f|, in milli-units */
  double lo, hi;            /* range AFTER the transform */
} canor_series;

/* Samples f at t = lo_num/den .. hi_num/den, classifies the growth, and picks
 * the axis. `out_vals` receives the TRANSFORMED values when non-NULL. */
bool canor_sym_series(const canor_sym *e, int64_t lo_num, int64_t hi_num, int64_t den,
                   int nsamples, double *out_vals, canor_series *out);

/* The transform itself, exposed so a renderer can label an axis with the same
 * function that produced the values. */
double canor_axis_apply(canor_axis a, double v);


/* --- OrigoDelta as an expression --------------------------------------------
 * WORLD_MODEL puts an OrigoDelta on every node of the triadic phase tree: a
 * Minkowski-interval biquaternionic Verification Boundary, relative to the
 * parent. canor_origo already carries exactly that, with signature (-,+,+,+) and
 * exact rational components.
 *
 * What it could not do was be a FUNCTION of anything. Turning it into an
 * expression lets a node carry an interval that VARIES - along a phase
 * parameter, a resolution level, or a Flow index - while still reducing to the
 * plain interval at a point. The Minkowski structure is not replaced; it is
 * what the expression evaluates to.
 *
 *     s^2(t) = -T(t)^2 + X(t)^2 + Y(t)^2 + Z(t)^2
 *
 * with each component its own canor_sym. At any fixed t this is an canor_origo and
 * classifies as timelike, spacelike or null exactly as before - so a varying
 * OrigoDelta is a family of Verification Boundaries rather than a different
 * kind of object.
 *
 * BY INDEX, NOT BY VALUE. An canor_sym is ~2.5 KB, and embedding four of them in
 * every TST node and BBST leaf would put megabytes into structures that are
 * built once and read many times - the same mistake canor_expression already made
 * inside BBST nodes, which cost 55 MB a tree before it was made an index.
 * So a node stores an index into a caller-held pool, and -1 means "no
 * expression, use the static origo". */

#define CANOR_SYMPOOL_MAX 64

typedef struct {
  canor_sym e[CANOR_SYMPOOL_MAX];
  int n;
} canor_sympool;

/* T, X, Y, Z as four expression indices into the same pool. */
typedef struct { int t, x, y, z; } canor_origo_expr;

void canor_sympool_init(canor_sympool *p);
int canor_sympool_add(canor_sympool *p, const canor_sym *e);      /* -1 when full */

/* Evaluates the four components at `at`, producing an ordinary canor_origo. Any
 * component that is poison makes the whole thing fail: a Verification Boundary
 * with one unusable coordinate is not a boundary with three good ones. */
bool canor_origo_expr_eval(const canor_sympool *p, const canor_origo_expr *oe,
                        canor_rational at, canor_origo *out);

/* The interval s^2 as a function of `at`, and its causal class. Same
 * classification the static canor_origo gets - the expression changes what varies,
 * not what the interval means. */
bool canor_origo_expr_interval(const canor_sympool *p, const canor_origo_expr *oe,
                            canor_rational at, canor_rational *s2);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_SYM_H */
