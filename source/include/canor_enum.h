#ifndef CANOR_ENUM_H
#define CANOR_ENUM_H

/* Index-addressed enumeration of the MOT pipeline spaces:
 *
 *     Strategy x Flow x MetaFlow -> Soul,   Soul x Action -> Maneuver
 *
 * Canor WD v1 §1 Flow, §2 MetaFlow, §3 Action, §4 Soul, §5 Maneuver.
 *
 * NOTHING LARGE IS MATERIALIZED. Every element is named by its rank and
 * produced on demand (/soul skill "Large spaces"; MEMORY.md 2026-10-20).
 *
 * SPACES
 *
 *   Strategy  One Ore extension per differential: the IMAG unit u (di .. dg,
 *             pinned order) is adjoined as the variable of K[x_u][d_u; sigma,
 *             delta], with d_u . f = sigma(f) . d_u + delta(f). Order k in
 *             1..6 (lazily expanded) is the power d_u^k, evaluated exactly by
 *             canor_ore as the skew-trigonometric triple C(k), S(k), T(k).
 *             15 x 6 = 90.  rank = (u - 1) * 6 + (k - 1)
 *   Flow      All 15 atoms, trifecta order fixed, each trifecta over its
 *             order-6 permutohedron (3! = 6). 6^5 = 7776. These particular
 *             Flows contain each atom once because each one is a permutation;
 *             the Flow VALUE type (canor_soul_flow_t) admits repeats and ring
 *             indices, and every occurrence counts.
 *   MetaFlow  The order-24 permutohedron of R, G, B, A (4! = 24).
 *             An invocation (MetaFlow, Strategy) excites a COMPLEX-FRACTAL
 *             STRING, slot for slot:
 *               R  Reals        what remains of [0, b^k] after k generations
 *               G  Gaps         the removed lengths, with multiplicities
 *               B  Bases        the scaling base b and the imaginary base e_u
 *               A  Auxiliaries  the generating equation z -> z**2 + c, c in
 *                               span{1, e_u} (the Mandelbrot map)
 *             with depth k = the Strategy's order and e_u its differential;
 *             the MetaFlow permutation is the slot order of the string.
 *   Soul      (Strategy * 7776 + Flow) * 24 + MetaFlow.  16,796,160
 *             Its value is the §4 count stream of the Flow (L->R).
 *   Action    m ≣ n (mod o), m, n, o in 0..289.  290^3 = 24,389,000.
 *             o = 0 is kept and diagnosed CANOR_ERR_UNDEFINED (84,100 of them).
 *   Maneuver  Soul * 24,389,000 + Action.  409,641,546,240,000
 *             The Soul Symbol bound one-to-one to the Action: the Action maps
 *             the Soul's count stream to canonical residues mod o.
 *
 * REPRESENTATIONS of Souls and Maneuvers are the four Real(s) x Base(s)
 * kinds of canor_soul.h; representation r of kind K is
 *     element = r / canor_soul_kind_count(K, 15),  idx = r % that.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_soul.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANOR_ENUM_TRIFECTAS 5
#define CANOR_ENUM_TRIFECTA_SIZE 3
#define CANOR_ENUM_FLOW_ATOMS (CANOR_ENUM_TRIFECTAS * CANOR_ENUM_TRIFECTA_SIZE)
#define CANOR_ENUM_TRIFECTA_PERMS 6                 /* 3!: order-6 permutohedron */
#define CANOR_ENUM_FLOW_COUNT UINT64_C(7776)        /* 6^5 */

#define CANOR_ENUM_DIFFERENTIALS CANOR_ENUM_FLOW_ATOMS
#define CANOR_ENUM_ORDER_MIN 1
#define CANOR_ENUM_ORDER_MAX 6
#define CANOR_ENUM_ORDERS (CANOR_ENUM_ORDER_MAX - CANOR_ENUM_ORDER_MIN + 1)
#define CANOR_ENUM_STRATEGY_COUNT UINT64_C(90)      /* 15 x 6 */

/* The Ore profile the Strategies are evaluated under. tau = sigma = 1 is the
 * untwisted shift algebra with period 6 (Niven), so Orders 1..6 run exactly
 * one period. Profile-overridable. */
#ifndef CANOR_ENUM_ORE_TAU
#define CANOR_ENUM_ORE_TAU "1"
#endif
#ifndef CANOR_ENUM_ORE_SIGMA
#define CANOR_ENUM_ORE_SIGMA "1"
#endif
#ifndef CANOR_ENUM_ORE_DELTA_AMP
#define CANOR_ENUM_ORE_DELTA_AMP "1"
#endif

#define CANOR_ENUM_METAFLOW_ATOMS 4                 /* R, G, B, A (PINNED) */
#define CANOR_ENUM_METAFLOW_COUNT UINT64_C(24)      /* 4! */
#define CANOR_ENUM_INVOCATION_COUNT (CANOR_ENUM_METAFLOW_COUNT * CANOR_ENUM_STRATEGY_COUNT)
/* Fractal-string scaling base: each generation splits an interval into b
 * parts and removes the middle one. 3 = the trifecta's arity (ternary,
 * Cantor). Profile-overridable; must be >= 2. */
#ifndef CANOR_ENUM_FRACTAL_BASE
#define CANOR_ENUM_FRACTAL_BASE 3
#endif

#define CANOR_ENUM_SOUL_COUNT \
  (CANOR_ENUM_STRATEGY_COUNT * CANOR_ENUM_FLOW_COUNT * CANOR_ENUM_METAFLOW_COUNT)
#define CANOR_ENUM_ACTION_BOUND UINT64_C(290)       /* m, n, o in 0..289 */
#define CANOR_ENUM_ACTION_COUNT \
  (CANOR_ENUM_ACTION_BOUND * CANOR_ENUM_ACTION_BOUND * CANOR_ENUM_ACTION_BOUND)
#define CANOR_ENUM_ACTION_UNDEFINED_COUNT (CANOR_ENUM_ACTION_BOUND * CANOR_ENUM_ACTION_BOUND)
#define CANOR_ENUM_MANEUVER_COUNT (CANOR_ENUM_SOUL_COUNT * CANOR_ENUM_ACTION_COUNT)

#ifndef CANOR_ENUM_SYMBOL_MAX
#define CANOR_ENUM_SYMBOL_MAX 160                   /* rendered symbol, with NUL */
#endif

typedef enum {
  CANOR_ENUM_STRATEGY = 0,
  CANOR_ENUM_FLOW,
  CANOR_ENUM_METAFLOW,
  CANOR_ENUM_INVOCATION,
  CANOR_ENUM_SOUL,
  CANOR_ENUM_ACTION,
  CANOR_ENUM_MANEUVER
} canor_enum_space;

typedef struct {
  canor_imag differential;   /* IMAG unit 1..15: the Ore variable x_u, di .. dg */
  uint8_t order;             /* k: the power d_u^k */
} canor_enum_strategy_t;

/* d_u^k under the Ore profile: C(k), S(k) always, T(k) = S/C when C != 0. */
typedef struct {
  canor_rational c, s, t;
  canor_status t_status;     /* CANOR_ERR_UNDEFINED where C(k) = 0 */
} canor_enum_ore_t;

typedef struct {
  uint8_t perm[CANOR_ENUM_TRIFECTAS];   /* Lehmer rank per trifecta, 0..5 */
  canor_soul_flow_t flow;
} canor_enum_flow_t;

typedef struct {
  uint8_t atom[CANOR_ENUM_METAFLOW_ATOMS];   /* 0 R, 1 G, 2 B, 3 A */
} canor_enum_metaflow_t;

/* A complex-fractal string after k generations, in integer units of b^-k:
 *   support = b^k, reals = (b-1)^k, gap level j (0..k-1) has length
 *   b^(k-1-j) with multiplicity (b-1)^j, and reals + sum(gaps) = support. */
typedef struct {
  canor_enum_metaflow_t slots;          /* slot order of the string */
  canor_enum_strategy_t strategy;
  uint32_t base;                        /* b */
  canor_imag direction;                 /* e_u */
  uint8_t depth;                        /* k */
  uint64_t support, reals, gaps;
  uint64_t gap_len[CANOR_ENUM_ORDER_MAX];
  uint64_t gap_mult[CANOR_ENUM_ORDER_MAX];
} canor_enum_fractal_t;

typedef struct {
  uint64_t rank;
  uint32_t strategy_rank, flow_rank, metaflow_rank;
  canor_enum_strategy_t strategy;
  canor_enum_flow_t flow;
  canor_enum_metaflow_t metaflow;
  canor_soul_stream_t stream;          /* §4 count stream, L->R */
  canor_soul_category category;
} canor_enum_soul_t;

typedef struct {
  uint64_t rank;
  uint16_t m, n, o;
  uint16_t rm, rn;           /* canonical residues, 0 <= r < o; 0 when undefined */
  canor_status status;       /* CANOR_OK, or CANOR_ERR_UNDEFINED for o = 0 */
  bool holds;                /* m ≣ n (mod o); false when undefined */
} canor_enum_action_t;

typedef struct {
  uint64_t rank;
  canor_enum_soul_t soul;
  canor_enum_action_t action;
  canor_soul_stream_t residues;  /* the Soul's stream mod o; valid when action.status OK */
} canor_enum_maneuver_t;

/* Element count of a space. */
uint64_t canor_enum_count(canor_enum_space space);
/* Representations of kind k over a Soul or Maneuver space (each element has
 * canor_soul_kind_count(k, 15) of them). 0 for any other space. */
uint64_t canor_enum_rep_count(canor_enum_space space, canor_soul_kind k);

canor_status canor_enum_strategy_unrank(uint64_t r, canor_enum_strategy_t *out);
canor_status canor_enum_strategy_rank(const canor_enum_strategy_t *s, uint64_t *r);
canor_status canor_enum_strategy_ore(const canor_enum_strategy_t *s, canor_enum_ore_t *out);

canor_status canor_enum_flow_unrank(uint64_t r, canor_enum_flow_t *out);
/* CANOR_ERR_NOT_FOUND for a Flow value outside this product space (wrong
 * length, a repeat, an atom outside its trifecta, a nonzero ring). */
canor_status canor_enum_flow_rank(const canor_soul_flow_t *f, uint64_t *r);

canor_status canor_enum_metaflow_unrank(uint64_t r, canor_enum_metaflow_t *out);
canor_status canor_enum_metaflow_rank(const canor_enum_metaflow_t *m, uint64_t *r);
/* Invocation r = metaflow * 90 + strategy, excited into its fractal string. */
canor_status canor_enum_invocation_unrank(uint64_t r, canor_enum_fractal_t *out);

canor_status canor_enum_soul_unrank(uint64_t r, canor_enum_soul_t *out);
canor_status canor_enum_soul_rep_unrank(canor_soul_kind k, uint64_t r, canor_enum_soul_t *soul,
                                        canor_soul_rep_t *rep);

canor_status canor_enum_action_unrank(uint64_t r, canor_enum_action_t *out);
canor_status canor_enum_action_rank(uint64_t m, uint64_t n, uint64_t o, uint64_t *r);

canor_status canor_enum_maneuver_unrank(uint64_t r, canor_enum_maneuver_t *out);
/* Representation of the Maneuver's residue stream. A Maneuver whose Action is
 * undefined has no residues: the Maneuver is still produced and the status
 * CANOR_ERR_UNDEFINED returned, with `rep` untouched. */
canor_status canor_enum_maneuver_rep_unrank(canor_soul_kind k, uint64_t r,
                                            canor_enum_maneuver_t *man, canor_soul_rep_t *rep);

/* Renderings. CANOR_ERR_FULL if `cap` cannot hold the whole text.
 *   strategy  di^3          flow      ijk|lnm|opq|rst|efg
 *   metaflow  RGBA          soul      di^3:ijk|lnm|opq|rst|efg:RGBA
 *   action    17 ≣ 5 (mod 12)   (UTF-8, the brief's own operator)
 *   rational  -1/2 */
canor_status canor_enum_strategy_format(const canor_enum_strategy_t *s, char *buf, size_t cap);
canor_status canor_enum_flow_format(const canor_soul_flow_t *f, char *buf, size_t cap);
canor_status canor_enum_metaflow_format(const canor_enum_metaflow_t *m, char *buf, size_t cap);
canor_status canor_enum_soul_format(const canor_enum_soul_t *s, char *buf, size_t cap);
canor_status canor_enum_action_format(const canor_enum_action_t *a, char *buf, size_t cap);
canor_status canor_enum_rational_format(canor_rational q, char *buf, size_t cap);
const char *canor_enum_metaflow_label(uint8_t atom);       /* "R" .. "A" */
const char *canor_enum_metaflow_label_name(uint8_t atom);  /* "Reals" .. "Auxiliaries" */

#ifdef __cplusplus
}
#endif
#endif /* CANOR_ENUM_H */
