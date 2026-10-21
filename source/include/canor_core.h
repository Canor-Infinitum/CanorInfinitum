#ifndef CANOR_CORE_H
#define CANOR_CORE_H

/* Canor-Core type tower.
 *
 * Implements FOLLOW-CORE.md's type list:
 *   BASIS, NUM, NUMBER, INT, TOKEN, VALUE, REFERENCE, FUNCTIONAL, OBSERVABLE,
 *   EXPR, OP, OPERATOR, DATA, REAL, INTERVAL, IMAG, IMAGINARY, BASUM, BASE,
 *   MODULATION, CALCULUS, TENSORIAL, TENSOR, UNIT, ATOM, CHANGE, FORCE,
 *   EXPRESSION, BBST, TST, NODE, LEAF.
 *
 * FOLLOW-CORE.md is REFERENCE, not authority. Where C cannot express what the
 * document assumes - it is written against a language with generics, traits and
 * enums-with-payload - the substitution is stated at the type rather than left
 * to be inferred.
 *
 * NO FLOATING POINT. The document's NUM carries an integer value with an
 * integer basis and exponent, and BASIS evaluates to a fraction 1/|B| for
 * negative B. That is exact rational arithmetic, not approximate real
 * arithmetic, and representing it in double would throw away the exactness the
 * design is built on. FLOWALGEBRA section 27's determinism requirement points
 * the same way.
 *
 * THE ONE PLACE THE SPECIFICATION DOES NOT CLOSE
 * ---------------------------------------------
 * IMAG is given as
 *
 *     [one=0, (i=1,j=2,k=3), (m=4,n=5,o=6), (r=7,s=8,t=9), (e=10,f=11,g=12)]
 *
 * which is four trifectas plus one: THIRTEEN basis elements. IMAGINARY is then
 * specified as "Hamilton's Rules: Column Arithmetic", and BASE as a permutation
 * group over IMAGINARY.
 *
 * Hamilton's construction - Cayley-Dickson doubling - closes only at dimensions
 * 1, 2, 4, 8 and 16: reals, complex, quaternions, octonions, sedenions. There
 * is no consistent multiplication table over 13 basis elements, so IMAG as
 * written cannot carry the arithmetic IMAGINARY requires of it. This is not a
 * reading difficulty; it is a counting one, and the workspace reached the same
 * conclusion independently - INDEX.md records "Extending IMAG to five trifectas
 * gives 16 = sedenions and resolves it."
 *
 * So the dimension is a DECLARED PROFILE here, not a constant:
 *
 *   CANOR_IMAG_TRIFECTAS = 5  ->  16 basis elements, sedenions. CLOSES. Default.
 *   CANOR_IMAG_TRIFECTAS = 4  ->  13 basis elements, as literally specified.
 *                              REFUSED at init by canor_base_init, with a message,
 *                              rather than quietly producing a table that is
 *                              not a multiplication.
 *
 * The table is COMPUTED by Cayley-Dickson doubling rather than written out.
 * A hand-written 16x16 signed table is 256 entries with no way to check it by
 * eye; doubling is thirty lines and is correct by construction, and the suite
 * then verifies the properties that follow from it (quaternion i*j=k and
 * j*i=-k, octonion non-associativity, sedenion zero divisors).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- capacities. Fixed, so every type below is a value the caller places. --- */

/* Factorial LEVELS for the Flow topology, which is indexed at n! levels with
 * six the practical minimum and about ninety-six the maximum.
 *
 * 96! is roughly 1e149, so the topology is NEVER materialized - there is not
 * enough matter to hold it. The factoradic index IS the address, and that is
 * precisely why FOLLOW-CORE makes NUMBER factoradic and BASE a permutation
 * group: a Lehmer code names a permutation directly, in n bytes rather than
 * n! entries.
 *
 * Kept as its own compact type (canor_lehmer, one byte per level) rather than as
 * an array of NUM. An array of 96 NUMs would be about 3 KB, and a payload
 * carrying two of those inside a term inside an observable inside a tensor
 * multiplies into tens of megabytes for a structure that only ever needs to
 * hold an index. The rich canor_number stays small and keeps its sign, basis and
 * exponent per digit; the index is bytes. */
#ifndef CANOR_LEVELS_MAX
#define CANOR_LEVELS_MAX 96
#endif
#define CANOR_LEVELS_MIN 6

#ifndef CANOR_NUMBER_DIGITS
#define CANOR_NUMBER_DIGITS 12      /* rich factoradic NUM digits; 12! = 479001600 */
#endif
#ifndef CANOR_IMAG_TRIFECTAS
#define CANOR_IMAG_TRIFECTAS 5      /* see the header note. 5 -> 16, closes. */
#endif
#define CANOR_IMAG_COUNT (1 + 3 * CANOR_IMAG_TRIFECTAS)
#ifndef CANOR_BASE_MAX
#define CANOR_BASE_MAX 8           /* IMAGINARY nodes in one BASE list */
#endif
#ifndef CANOR_TENSORIAL_MAX
#define CANOR_TENSORIAL_MAX 4       /* MODULATIONs in one TENSORIAL */
#endif
#ifndef CANOR_TENSOR_RANK_MAX
#define CANOR_TENSOR_RANK_MAX 4     /* TENSORIALs per variance */
#endif
#ifndef CANOR_UNIT_MAX
#define CANOR_UNIT_MAX 2
#endif
#ifndef CANOR_IDENT_MAX
#define CANOR_IDENT_MAX 32
#endif
#ifndef CANOR_BBST_MAX
#define CANOR_BBST_MAX 64
#endif

typedef enum {
  CANOR_OK = 0,
  CANOR_ERR_ARGS = -1,
  CANOR_ERR_RANGE = -2,
  CANOR_ERR_FULL = -3,
  CANOR_ERR_NOT_CLOSED = -4,   /* the IMAG dimension does not admit a product */
  CANOR_ERR_UNDEFINED = -5,    /* e.g. division by a zero divisor */
  CANOR_ERR_NOT_FOUND = -6     /* key/element not found in container/store */
} canor_status;

const char *canor_strerror(canor_status s);

/* ===========================================================================
 * BASIS - "generic signed value-implementation [T: i8..isize] != 0"
 *
 * Generated from `^[+-][1-9][0-9]*$`, so it is a nonzero signed integer that
 * always carries an explicit sign. eval() gives |B| for B > 0 and the fraction
 * 1/|B| for B < 0 - which is why the result is a rational and not an integer.
 * =========================================================================== */

typedef struct { int64_t v; } canor_basis;          /* invariant: v != 0 */

/* Exact rational. den > 0 for every VALUE; the sign lives in num. Reduced on
 * every operation, so equality is structural and two equal values compare
 * equal.
 *
 * den == 0 is the poison: "this is not a value". It is produced by a zero
 * denominator and by an arithmetic result that will not fit in int64, and it
 * propagates through every operation below. Check it with canor_rat_valid.
 *
 * The alternative was to let an overflowing product wrap, which is undefined
 * behaviour (section 39 forbids it) and, worse, yields a plausible number. A
 * poison that spreads is noisy; a wrong rational is not, and this core exists
 * to be checkable. */
typedef struct { int64_t num, den; } canor_rational;

bool canor_rat_valid(canor_rational a);

canor_status canor_basis_parse(const char *text, canor_basis *out);  /* enforces the regex */
canor_status canor_basis_make(int64_t v, canor_basis *out);
canor_rational canor_basis_eval(canor_basis b);

canor_rational canor_rat(int64_t num, int64_t den);
/* Both cancel across the operands before multiplying, so the intermediates
 * stay far smaller than the naive cross product, and both return the poison
 * rather than wrapping when a result genuinely will not fit. */
canor_rational canor_rat_add(canor_rational a, canor_rational b);
canor_rational canor_rat_mul(canor_rational a, canor_rational b);
/* A poison equals nothing, including another poison: two failed computations
 * agreeing that they failed is not evidence they agree. */
bool canor_rat_eq(canor_rational a, canor_rational b);
/* Cannot overflow for any pair of valid rationals - it compares by continued
 * fraction expansion and never multiplies. Returns 0 if either is poison,
 * which is why the ordering of poison values is not meaningful; test
 * canor_rat_valid when that distinction matters. */
int canor_rat_cmp(canor_rational a, canor_rational b);

/* ===========================================================================
 * NUM / NUMBER / INT
 *
 * NUM sign is "[-0, -1, +1, +0]-signage": four values, not two. That is the
 * same signed-zero quaternary as FLOWALGEBRA's FLOWVALUE, and the two are
 * deliberately given the same ordering here so a conversion is a cast rather
 * than a lookup table that could disagree.
 * =========================================================================== */

typedef enum {
  CANOR_NEG_ZERO = -2,
  CANOR_NEG_ONE  = -1,
  CANOR_POS_ONE  =  1,
  CANOR_POS_ZERO =  2
} canor_sign;

typedef struct {
  canor_sign sign;
  uint64_t value;
  canor_basis basis;      /* default +1 */
  canor_basis exponent;   /* default +1 */
} canor_num;

canor_status canor_num_make(canor_sign sign, uint64_t value, int64_t basis, int64_t exponent,
                      canor_num *out);
/* value * eval(basis)^eval(exponent), with the sign applied. Exponent is
 * restricted to integer powers: a fractional exponent would be a root, which
 * is not generally rational, and returning an approximation here would put a
 * float back into the substrate through the side door. */
canor_status canor_num_eval(const canor_num *n, canor_rational *out);

/* NUMBER - "a factoradics implementation of an Array of NUM".
 *
 * Factoradic: digit i has radix (i+1), so digit i is in [0, i]. Digit 0 is
 * therefore always 0. The array is little-endian by radix, which is what makes
 * the carry rule local. */
typedef struct {
  canor_num d[CANOR_NUMBER_DIGITS];
  uint8_t n;
} canor_number;

void canor_number_zero(canor_number *out);
/* Encodes a nonnegative integer into factoradic digits. */
canor_status canor_number_from_u64(uint64_t v, canor_number *out);
/* The exact inverse. */
canor_status canor_number_to_u64(const canor_number *n, uint64_t *out);
/* Every digit i must satisfy value <= i, which is the whole factoradic
 * invariant and the thing an arithmetic bug breaks first. */
bool canor_number_valid(const canor_number *n);

typedef struct { canor_number lo, hi; } canor_int;
canor_status canor_int_make(const canor_number *lo, const canor_number *hi, canor_int *out);

/* ===========================================================================
 * LEHMER - the factoradic index into an n!-level Flow topology.
 *
 * Digit i lies in [0, i], so digit 0 is always zero and the value of a code
 * with `levels` digits ranges over [0, levels!). At the stated maximum that is
 * [0, 96!), which no machine enumerates - so every operation here works on the
 * code itself and never on the number it denotes.
 *
 * to_u64 exists for the small end and REFUSES above 20 levels rather than
 * wrapping: 20! is 2.4e18 and fits, 21! does not. A silent wrap here would
 * alias two distinct topology addresses onto one, which is the worst possible
 * failure for an index.
 * =========================================================================== */

typedef struct {
  uint8_t d[CANOR_LEVELS_MAX];
  uint8_t levels;
} canor_lehmer;

void canor_lehmer_zero(canor_lehmer *l, int levels);
bool canor_lehmer_valid(const canor_lehmer *l);
int  canor_lehmer_cmp(const canor_lehmer *a, const canor_lehmer *b);
/* Odometer increment in the factorial number system. Returns CANOR_ERR_RANGE on
 * wrap past the last address, rather than silently returning to zero. */
canor_status canor_lehmer_inc(canor_lehmer *l);
canor_status canor_lehmer_from_u64(uint64_t v, int levels, canor_lehmer *out);
canor_status canor_lehmer_to_u64(const canor_lehmer *l, uint64_t *out);   /* <= 20 levels */

/* The Lehmer code IS a permutation, which is what ties NUMBER to BASE: BASE is
 * a permutation group and this is how an index names one of its elements. Both
 * directions, so the correspondence is checkable rather than asserted. */
canor_status canor_lehmer_to_permutation(const canor_lehmer *l, uint8_t *perm, int max);
canor_status canor_lehmer_from_permutation(const uint8_t *perm, int levels, canor_lehmer *out);

/* ===========================================================================
 * TOKEN / VALUE / REFERENCE / FUNCTIONAL / OBSERVABLE / OPERATOR
 *
 * The document specifies TOKEN as a trait with .eval() returning an INT, and
 * VALUE/REFERENCE/FUNCTIONAL as generic {pre, peri, post} triples implementing
 * it. C has no traits, so a trait becomes a vtable: an explicit function
 * pointer carried by the value. That is the same dispatch, made visible.
 * =========================================================================== */

typedef enum { CANOR_T_VALUE, CANOR_T_REFERENCE, CANOR_T_FUNCTIONAL } canor_termkind;

/* The payload a triple slot holds. The document's T ranges over
 * (INT, NUMBER, String); a tagged union is the C spelling of that. */
typedef enum { CANOR_P_INT, CANOR_P_NUMBER, CANOR_P_STRING } canor_payload_kind;

/* A UNION, not a struct with three fields. The kind tag says which member is
 * live, and holding all three at once would trible the size of every term,
 * observable, tensor and tree node that carries one - which compounds badly
 * once these nest. */
typedef struct {
  canor_payload_kind kind;
  union {
    canor_int as_int;
    canor_number as_number;
    char as_string[CANOR_IDENT_MAX];
  } u;
} canor_payload;

struct canor_term;
/* TOKEN's eval(). */
typedef canor_status (*canor_token_eval_fn)(const struct canor_term *self, canor_int *out);

/* VALUE, REFERENCE and FUNCTIONAL differ in what the slots MEAN, not in shape:
 *   VALUE       the payload itself
 *   REFERENCE   a pointer to a payload (`@T` in the document)
 *   FUNCTIONAL  a function returning a payload
 * so one struct with a kind tag carries all three, and the eval function is
 * what actually differs. */
typedef struct canor_term {
  canor_termkind kind;
  canor_payload pre, peri, post;          /* VALUE: direct */
  const canor_payload *ref_pre, *ref_peri, *ref_post;   /* REFERENCE */
  canor_status (*fn_pre)(canor_payload *out);              /* FUNCTIONAL */
  canor_status (*fn_peri)(canor_payload *out);
  canor_status (*fn_post)(canor_payload *out);
  canor_token_eval_fn eval;
} canor_term;

void canor_term_init_value(canor_term *t, const canor_payload *pre, const canor_payload *peri,
                        const canor_payload *post);
void canor_term_init_reference(canor_term *t, const canor_payload *pre, const canor_payload *peri,
                            const canor_payload *post);
void canor_term_init_functional(canor_term *t,
                             canor_status (*pre)(canor_payload *),
                             canor_status (*peri)(canor_payload *),
                             canor_status (*post)(canor_payload *));
canor_status canor_term_eval(const canor_term *t, canor_int *out);

/* Resolves the three slots and checks the ordering the document states as
 * `pre <= peri <= post`. Quantum-uncertainty ordering is an invariant of the
 * triple, so it is checkable, and a term that violates it is malformed rather
 * than merely unusual. */
bool canor_term_ordered(const canor_term *t);

/* OBSERVABLE - the same triple one level up, over terms. EXPR is
 * OBSERVABLE[T, String], which here is an observable whose payloads are
 * strings; the alias is expressed by a predicate rather than a distinct type,
 * because C typedefs do not constrain. */
typedef struct {
  canor_term pre, peri, post;
} canor_observable;

bool canor_observable_is_expr(const canor_observable *o);   /* all payloads are strings */

/* OPERATOR - "strict infix-only", with semantics
 * {LHS : OBSERVABLE::pre, OP : OBSERVABLE::peri, RHS : OBSERVABLE::post}. */
typedef enum {
  CANOR_OP_ADD, CANOR_OP_SUB, CANOR_OP_MUL, CANOR_OP_DIV
} canor_op_kind;

typedef struct {
  canor_op_kind op;
  canor_term lhs, rhs;
} canor_operator;

canor_status canor_operator_eval(const canor_operator *o, canor_observable *out);

/* ===========================================================================
 * DATA / REAL / INTERVAL
 *
 * DATA is a 3-tuple {low, value, high} OF a 3-tuple {min, nom, max}. Both
 * levels are three-point estimates, and min <= nom <= max is checked - the
 * same distinction pragma.ebnf draws between an estimate envelope and an
 * interval, and the same one uu_follow_timing emits.
 * =========================================================================== */

typedef struct { canor_rational min, nom, max; } canor_triple;
typedef struct { canor_triple low, value, high; } canor_data;

bool canor_triple_ordered(const canor_triple *t);
bool canor_data_ordered(const canor_data *d);

typedef struct { canor_rational val; } canor_real;
typedef struct { canor_rational lo, hi; } canor_interval;
bool canor_interval_ordered(const canor_interval *i);

/* ===========================================================================
 * Kapprox - FLOWALGEBRA section 30's complexity measure
 *
 * "Kapprox(f) estimates the size of the shortest known procedural description
 * of f. This is not true Kolmogorov complexity, which is generally
 * uncomputable, but an implementation-bound approximation."
 *
 * The four MetaFlow classes form a description-length ladder:
 *
 *   General      2n + 1          linear     - a short rule covers it
 *   Specific     2(n!) + 1       factorial  - needs a permutation index
 *   Distinctive  (2n + 1)!       factorial of linear
 *   Arbitrary    (2(n!) + 1)!    factorial of factorial - incompressible
 *
 * Read as description LENGTH, not population: an arbitrary object has no
 * description shorter than itself, which is Kolmogorov's point and why
 * Arbitrary sits at the top rather than the bottom.
 *
 * TWO DECLARED CHOICES, because the notation does not make either:
 *
 *   1. `2n!` is 2*(n!), NOT (2n)!. At n=2 those are 5 and 25, so the reading
 *      changes the ladder and has to be stated.
 *   2. The ladder is strictly increasing only for n >= 3. At n=1 it gives
 *      3, 3, 6, 6 and at n=2 it gives 5, 5, 120, 120 - the four classes
 *      collapse to two. CANOR_LEVELS_MIN is 6 so this never binds here, but it
 *      is a property of the ladder rather than of the implementation.
 *
 * RETURNED AS LOG2 IN MILLI-BITS, AS AN INTEGER. Three of the four exceed
 * uint64 almost at once - (2n+1)! passes it at n=10 - and at n=96 the top
 * class is around 1e152 BITS wide, so the value is not representable and the
 * logarithm is. Integer milli-bits rather than a double because section 27
 * forbids floating-point nondeterminism and two hosts must agree exactly.
 * =========================================================================== */

typedef enum {
  CANOR_K_GENERAL = 0,      /* 2n + 1       */
  CANOR_K_SPECIFIC = 1,     /* 2(n!) + 1    */
  CANOR_K_DISTINCTIVE = 2,  /* (2n + 1)!    */
  CANOR_K_ARBITRARY = 3     /* (2(n!) + 1)! */
} canor_kclass;

/* log2(x) * 1000, truncated. x = 0 gives 0. Exact integer arithmetic. */
int64_t canor_log2_milli(uint64_t x);
/* log2(n!) * 1000. Summed exactly below the declared crossover, Stirling
 * above it. */
int64_t canor_log2_factorial_milli(uint64_t n);
/* The ladder, in milli-bits of description length. The top class is reported
 * as an ORDER - log2 of the log2 - because its value's own logarithm exceeds
 * what a caller can use arithmetically. */
int64_t canor_kapprox_milli(canor_kclass c, uint64_t n);
/* Same ladder on ONE scale: log2(log2(value)). The top class does not fit in
 * log2 bits, so comparisons must use this rather than canor_kapprox_milli -
 * mixing the two makes the ladder appear to invert. */
int64_t canor_kapprox_order_milli(canor_kclass c, uint64_t n);
/* True where all four classes are strictly separated, on the order scale. */
bool canor_kapprox_separated(uint64_t n);
const char *canor_kclass_name(canor_kclass c);

/* ===========================================================================
 * IMAG / IMAGINARY / BASUM / BASE - the Cayley-Dickson layer.
 * =========================================================================== */

/* Basis index 0..CANOR_IMAG_COUNT-1. 0 is `one`; the rest are the trifectas in
 * order. Under the default profile these are the sedenion units e0..e15.
 *
 * Names are PINNED (SCHEMA.md "IMAG basis", MEMORY.md 2026-10-20):
 *   [one=0, (i,j,k)=1..3, (l,m,n)=4..6, (o,p,q)=7..9, (r,s,t)=10..12,
 *    (e,f,g)=13..15]
 * The four-trifecta list quoted at the top of this header is the reference
 * document's; the pinned naming supersedes its letters, not its indices. */
typedef uint8_t canor_imag;

typedef struct {
  canor_imag imag;
  size_t ring;
} canor_imaginary;

/* Must be called once before any product. Builds the multiplication table by
 * Cayley-Dickson doubling and REFUSES a dimension that does not close -
 * returning CANOR_ERR_NOT_CLOSED rather than a table that is not a
 * multiplication. */
canor_status canor_imag_init(void);
/* e_a * e_b = sign * e_index. Both outputs are written; sign is +1 or -1. */
canor_status canor_imag_mul(canor_imag a, canor_imag b, canor_imag *index, int *sign);
/* Conjugation: e0 is fixed, every other unit negates. */
int canor_imag_conj_sign(canor_imag a);
const char *canor_imag_name(canor_imag a);

/* BASE - "strict double-linked-list of IMAGINARY nodes ... A permutation
 * group". Stored as an array with explicit prev/next indices: the document
 * asks for a doubly-linked list and this is one, without a per-node
 * allocation that an SBC target would rather not make. */
typedef struct {
  canor_imaginary node[CANOR_BASE_MAX];
  int prev[CANOR_BASE_MAX];
  int next[CANOR_BASE_MAX];
  int head, tail;
  int count;
} canor_base;

void canor_base_init(canor_base *b);
canor_status canor_base_push(canor_base *b, canor_imaginary v);
/* BASUM's eval(): folds the list under the Cayley-Dickson product, in list
 * order. Order matters - the product is non-commutative from dimension 4 up -
 * so "in list order" is part of the contract and not an implementation note. */
canor_status canor_base_eval(const canor_base *b, canor_imaginary *out, int *sign);
/* The permutation-group property the document asserts: the set of units
 * reachable by multiplying by a fixed unit is a permutation of the units. */
bool canor_base_is_permutation(canor_imag by);

/* ===========================================================================
 * MODULATION / CALCULUS / TENSORIAL / TENSOR
 * =========================================================================== */

typedef struct {
  canor_real real;
  canor_interval bounds;
  canor_base base;
} canor_modulation;

typedef struct {
  canor_modulation data[CANOR_TENSORIAL_MAX];
  int count;
  bool regular;
  bool symmetric;
  bool antisymmetric;
  bool unknownsymmetric;
  size_t group_id;
} canor_tensorial;

void canor_tensorial_init(canor_tensorial *t, size_t group_id);
canor_status canor_tensorial_push(canor_tensorial *t, const canor_modulation *m);
/* The four symmetry flags are not independent: symmetric and antisymmetric
 * cannot both hold, and unknownsymmetric cannot hold beside either. Checked,
 * because a TENSORIAL claiming both is the sort of state that produces a
 * plausible wrong answer downstream. */
bool canor_tensorial_flags_consistent(const canor_tensorial *t);

typedef struct {
  canor_observable input;                              /* EXPR[T] */
  canor_tensorial covariant[CANOR_TENSOR_RANK_MAX];
  int n_cov;
  canor_tensorial contravariant[CANOR_TENSOR_RANK_MAX];
  int n_con;
} canor_tensor;

void canor_tensor_init(canor_tensor *t);
canor_status canor_tensor_add_covariant(canor_tensor *t, const canor_tensorial *v);
canor_status canor_tensor_add_contravariant(canor_tensor *t, const canor_tensorial *v);
/* CALCULUS's eval(): "results in a MODULATION". */
canor_status canor_tensor_eval(const canor_tensor *t, canor_modulation *out);
/* Rank as the document implies it: (contravariant, covariant). */
void canor_tensor_rank(const canor_tensor *t, int *contra, int *cov);

/* ===========================================================================
 * UNIT / ATOM / CHANGE / FORCE / EXPRESSION
 * =========================================================================== */

typedef struct {
  canor_termkind head;                 /* VALUE -> ATOM, REFERENCE -> CHANGE,
                                     * FUNCTIONAL -> FORCE */
  canor_real reals[CANOR_UNIT_MAX];
  int n_reals;
  canor_interval intervals[CANOR_UNIT_MAX];
  int n_intervals;
  canor_tensor base[CANOR_UNIT_MAX];
  int n_base;
  canor_operator op;
  bool has_op;
} canor_unit;

void canor_unit_init(canor_unit *u, canor_termkind head);
canor_status canor_unit_add_real(canor_unit *u, canor_real r);
canor_status canor_unit_add_interval(canor_unit *u, canor_interval i);
canor_status canor_unit_add_tensor(canor_unit *u, const canor_tensor *t);
/* ATOM, CHANGE and FORCE are UNIT with the head fixed. Predicates rather than
 * distinct types: they are sub-types by the document's own wording, and three
 * identical structs would let a caller pass one where another belongs. */
bool canor_unit_is_atom(const canor_unit *u);
bool canor_unit_is_change(const canor_unit *u);
bool canor_unit_is_force(const canor_unit *u);

typedef struct {
  canor_observable input;
  canor_unit output[CANOR_UNIT_MAX];
  int n_output;
} canor_expression;

void canor_expression_init(canor_expression *e);
canor_status canor_expression_add(canor_expression *e, const canor_unit *u);

/* ===========================================================================
 * BBST / TST / NODE / LEAF
 *
 * BBST is "kd-mapped": each node carries a k-dimensional key and the level
 * decides which dimension is compared, which is a k-d tree's rule rather than
 * a plain BST's. Balanced by construction here - built from a sorted array by
 * median split - because insertion rebalancing on a fixed array buys nothing
 * for a structure that is built once and then read.
 * =========================================================================== */

#ifndef CANOR_KD
#define CANOR_KD 3                        /* the document's 3D kd-mapping */
#endif

/* A node REFERENCES its EXPRESSION; it does not embed one.
 *
 * FOLLOW-CORE says a BBST node "contain[s] an EXPRESSION property", and in a
 * language with references that is what embedding means. In C it means a copy,
 * and the copy compounds: EXPRESSION holds UNITs, a UNIT holds TENSORs, a
 * TENSOR holds TENSORIALs, a TENSORIAL holds MODULATIONs, and a MODULATION
 * holds a BASE. Measured at the first capacities chosen here, one EXPRESSION
 * came to roughly 850 KB and a 64-node tree to about 55 MB - so declaring two
 * trees as locals overflowed the stack before main() printed anything, which
 * is exactly how this was found.
 *
 * The index is the fix, and it is also the more faithful reading: the document
 * describes a property, not a copy, and two nodes referring to one EXPRESSION
 * is a thing the type should be able to express. The caller owns the pool. */
typedef struct {
  int64_t key[CANOR_KD];
  int expr;                            /* index into the caller's pool, -1 none */
  int left, right;                     /* indices, -1 for none */
} canor_bbst_node;

typedef struct {
  canor_bbst_node node[CANOR_BBST_MAX];
  int count;
  int root;
} canor_bbst;

void canor_bbst_init(canor_bbst *t);
/* expr is an index into whatever pool the caller keeps, or -1. */
canor_status canor_bbst_insert(canor_bbst *t, const int64_t key[CANOR_KD], int expr);
int canor_bbst_expr(const canor_bbst *t, int node);
/* Builds a balanced kd-tree over everything inserted so far. Deterministic:
 * the median split uses a stable order, so two identical input sets give
 * identical trees. */
canor_status canor_bbst_build(canor_bbst *t);
int canor_bbst_find(const canor_bbst *t, const int64_t key[CANOR_KD]);   /* -1 if absent */
int canor_bbst_depth(const canor_bbst *t);

/* A TST NODE's three leaves, which the document names explicitly:
 *   PRE  CONSTRAINTS  -> applied to ORIGO, gives a 3D object
 *   PERI VALUES       -> applied to that, gives a volumetric object
 *   POST FREEDOMS     -> applied to that, gives an animated object
 * The ordering is strict and the stages are not interchangeable. */
typedef enum { CANOR_LEAF_PRE = 0, CANOR_LEAF_PERI = 1, CANOR_LEAF_POST = 2 } canor_leaf_kind;

/* ORIGO: a Cayley-Dickson-Minkowski spacetime interval delta, relative to the
 * parent node's ORIGO. Minkowski signature (-,+,+,+) with an exact rational
 * interval, so the classification below is decided rather than approximated. */
typedef struct {
  canor_rational t, x, y, z;
} canor_origo;

typedef enum {
  CANOR_INTERVAL_TIMELIKE,
  CANOR_INTERVAL_NULL,
  CANOR_INTERVAL_SPACELIKE,
  /* WORLD_MODEL section 8.4's fourth class. Not an error state: a delta whose
   * provenance is too poor to classify has its own class, and the system is
   * required to represent that rather than guess. */
  CANOR_INTERVAL_INDETERMINATE
} canor_interval_class;

canor_rational canor_origo_interval2(const canor_origo *o);       /* s^2 = -t^2+x^2+y^2+z^2 */
canor_interval_class canor_origo_classify(const canor_origo *o, bool provenance_ok);

typedef struct {
  canor_origo origo;                      /* relative to the parent's ORIGO */
  /* The OrigoDelta as an EXPRESSION, when the node's Verification Boundary
   * varies rather than being fixed: four indices (T, X, Y, Z) into a caller's
   * canor_sympool, or -1 for each when the static `origo` above is the whole
   * story. Held by index for the same reason canor_bbst holds `expr` by index -
   * embedding the expressions themselves put 55 MB into a tree that is built
   * once and read many times.
   *
   * The static origo remains the value at a point, so a node with an
   * expression is a FAMILY of boundaries and a node without one is a single
   * boundary; neither is a different kind of node. */
  int origo_expr[4];
  canor_bbst leaf[3];                     /* PRE, PERI, POST */
  int left, middle, right;             /* TST children, -1 for none */
  bool used;
} canor_tst_node;

typedef struct {
  canor_tst_node node[CANOR_BBST_MAX];
  int count;
  int root;
} canor_tst;

void canor_tst_init(canor_tst *t);
int canor_tst_add(canor_tst *t, const canor_origo *origo);         /* -1 if full */
/* Attaches a varying OrigoDelta. Pass -1 for a component that does not vary;
 * it then reads from the node's static origo. Returns CANOR_ERR_RANGE for an
 * unknown node. */
canor_status canor_tst_set_origo_expr(canor_tst *t, int node, int et, int ex, int ey, int ez);
bool canor_tst_has_origo_expr(const canor_tst *t, int node);
canor_status canor_tst_link(canor_tst *t, int parent, int which, int child);  /* which 0..2 */
/* ORIGO is relative to the parent, so an absolute position is the fold of the
 * deltas along the path from the root. */
canor_status canor_tst_absolute_origo(const canor_tst *t, int node, canor_origo *out);

/* One step-wise iteration, in the document's order: PRE to a Lorentzian frame,
 * PERI to a topological spacetime object, POST to a temporally animated one.
 * The stages are reported rather than rendered - this layer has no geometry
 * kernel, and pretending otherwise would be the overreach the tower is meant
 * to avoid. */
typedef struct {
  bool frame;        /* PRE applied  */
  bool volumetric;   /* PERI applied */
  bool animated;     /* POST applied */
  int expressions;   /* total across the three leaves */
} canor_iteration;

canor_status canor_tst_iterate(const canor_tst *t, int node, canor_iteration *out);

#ifdef __cplusplus
}
#endif
#endif /* CANOR_CORE_H */
