/* canor_enum.c - rank / unrank over the MOT pipeline spaces.
 *
 * Canor WD v1 §1 Flow ... §5 Maneuver. Every space is a mixed-radix product
 * of small permutohedra and integer ranges, so rank and unrank are exact
 * integer arithmetic and need no storage beyond the element produced. Values
 * come from the modules that own them: count streams from canor_soul, the
 * Ore extension from canor_ore. See canor_enum.h for the declared readings. */

#include "canor_enum.h"

#include <stdio.h>
#include <string.h>

#include "canor_ore.h"

_Static_assert(CANOR_ENUM_TRIFECTAS == CANOR_IMAG_TRIFECTAS,
               "enumeration trifectas must match the IMAG profile");
_Static_assert(CANOR_ENUM_FLOW_ATOMS <= CANOR_SOUL_FLOW_MAX, "a product Flow must fit a Flow value");
_Static_assert(CANOR_ENUM_FRACTAL_BASE >= 2, "a fractal string needs a base of at least 2");

#define ENUM_FLOW_RADIX CANOR_ENUM_TRIFECTA_PERMS
/* 6^4: the weight of trifecta 0's digit in a Flow rank. */
#define ENUM_FLOW_TOP_WEIGHT (CANOR_ENUM_FLOW_COUNT / ENUM_FLOW_RADIX)

/* Lexicographic permutations of three letters, by Lehmer rank. */
static const uint8_t k_perm3[CANOR_ENUM_TRIFECTA_PERMS][CANOR_ENUM_TRIFECTA_SIZE] = {
  {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
};

/* --- small permutations ------------------------------------------------------ */

static uint64_t enum_factorial(unsigned n) {
  uint64_t f = 1;
  for (unsigned k = 2; k <= n; ++k) f *= k;
  return f;
}

/* Lexicographic Lehmer unrank / rank for n <= 4 letters. */
static void enum_perm_unrank(uint64_t r, unsigned n, uint8_t *perm) {
  uint8_t pool[CANOR_ENUM_METAFLOW_ATOMS];
  for (unsigned k = 0; k < n; ++k) pool[k] = (uint8_t)k;
  for (unsigned k = 0; k < n; ++k) {
    uint64_t w = enum_factorial(n - 1 - k);
    unsigned d = (unsigned)(r / w);
    r %= w;
    perm[k] = pool[d];
    for (unsigned j = d; j + 1 < n - k; ++j) pool[j] = pool[j + 1];
  }
}

static bool enum_perm_rank(const uint8_t *perm, unsigned n, uint64_t *r) {
  bool used[CANOR_ENUM_METAFLOW_ATOMS] = {false};
  uint64_t acc = 0;
  for (unsigned k = 0; k < n; ++k) {
    if (perm[k] >= n || used[perm[k]]) return false;
    unsigned smaller = 0;
    for (unsigned j = 0; j < perm[k]; ++j) smaller += !used[j];
    used[perm[k]] = true;
    acc += smaller * enum_factorial(n - 1 - k);
  }
  *r = acc;
  return true;
}

/* --- counts ------------------------------------------------------------------ */

uint64_t canor_enum_count(canor_enum_space space) {
  switch (space) {
    case CANOR_ENUM_STRATEGY: return CANOR_ENUM_STRATEGY_COUNT;
    case CANOR_ENUM_FLOW: return CANOR_ENUM_FLOW_COUNT;
    case CANOR_ENUM_METAFLOW: return CANOR_ENUM_METAFLOW_COUNT;
    case CANOR_ENUM_INVOCATION: return CANOR_ENUM_INVOCATION_COUNT;
    case CANOR_ENUM_SOUL: return CANOR_ENUM_SOUL_COUNT;
    case CANOR_ENUM_ACTION: return CANOR_ENUM_ACTION_COUNT;
    case CANOR_ENUM_MANEUVER: return CANOR_ENUM_MANEUVER_COUNT;
    default: return 0;
  }
}

uint64_t canor_enum_rep_count(canor_enum_space space, canor_soul_kind k) {
  if (space != CANOR_ENUM_SOUL && space != CANOR_ENUM_MANEUVER) return 0;
  return canor_enum_count(space) * canor_soul_kind_count(k, CANOR_ENUM_FLOW_ATOMS);
}

/* --- Strategy ---------------------------------------------------------------- */

canor_status canor_enum_strategy_unrank(uint64_t r, canor_enum_strategy_t *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (r >= CANOR_ENUM_STRATEGY_COUNT) return CANOR_ERR_RANGE;
  out->differential = (canor_imag)(1u + r / CANOR_ENUM_ORDERS);
  out->order = (uint8_t)(CANOR_ENUM_ORDER_MIN + r % CANOR_ENUM_ORDERS);
  return CANOR_OK;
}

canor_status canor_enum_strategy_rank(const canor_enum_strategy_t *s, uint64_t *r) {
  if (!s || !r) return CANOR_ERR_ARGS;
  if (s->differential < 1 || s->differential > CANOR_ENUM_DIFFERENTIALS) return CANOR_ERR_RANGE;
  if (s->order < CANOR_ENUM_ORDER_MIN || s->order > CANOR_ENUM_ORDER_MAX) return CANOR_ERR_RANGE;
  *r = (uint64_t)(s->differential - 1u) * CANOR_ENUM_ORDERS + (s->order - CANOR_ENUM_ORDER_MIN);
  return CANOR_OK;
}

canor_status canor_enum_strategy_ore(const canor_enum_strategy_t *s, canor_enum_ore_t *out) {
  canor_ore o;
  canor_enum_ore_t r;
  if (!s || !out) return CANOR_ERR_ARGS;
  if (s->order < CANOR_ENUM_ORDER_MIN || s->order > CANOR_ENUM_ORDER_MAX) return CANOR_ERR_RANGE;
  if (!canor_ore_init(&o, CANOR_ENUM_ORE_TAU, CANOR_ENUM_ORE_SIGMA, CANOR_ENUM_ORE_DELTA_AMP))
    return CANOR_ERR_ARGS;
  /* C and S always exist for an in-range Order; a failure is overflow. */
  if (!canor_ore_cos(&o, s->order, &r.c) || !canor_ore_sin(&o, s->order, &r.s))
    return CANOR_ERR_RANGE;
  r.t.num = 0;
  r.t.den = 1;
  if (r.c.num == 0) r.t_status = CANOR_ERR_UNDEFINED;          /* T = S / 0 */
  else r.t_status = canor_ore_tan(&o, s->order, &r.t) ? CANOR_OK : CANOR_ERR_RANGE;
  *out = r;
  return CANOR_OK;
}

/* --- Flow -------------------------------------------------------------------- */

canor_status canor_enum_flow_unrank(uint64_t r, canor_enum_flow_t *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (r >= CANOR_ENUM_FLOW_COUNT) return CANOR_ERR_RANGE;
  uint64_t w = ENUM_FLOW_TOP_WEIGHT;
  out->flow.n = CANOR_ENUM_FLOW_ATOMS;
  for (unsigned t = 0; t < CANOR_ENUM_TRIFECTAS; ++t) {
    unsigned p = (unsigned)(r / w);
    r %= w;
    w /= ENUM_FLOW_RADIX;
    out->perm[t] = (uint8_t)p;
    for (unsigned k = 0; k < CANOR_ENUM_TRIFECTA_SIZE; ++k) {
      canor_soul_usage_t *u = &out->flow.at[t * CANOR_ENUM_TRIFECTA_SIZE + k];
      u->atom = (canor_imag)(1u + t * CANOR_ENUM_TRIFECTA_SIZE + k_perm3[p][k]);
      u->ring = 0;                       /* omitted index */
    }
  }
  return CANOR_OK;
}

canor_status canor_enum_flow_rank(const canor_soul_flow_t *f, uint64_t *r) {
  if (!f || !r) return CANOR_ERR_ARGS;
  if (f->n != CANOR_ENUM_FLOW_ATOMS) return CANOR_ERR_NOT_FOUND;
  uint64_t acc = 0;
  for (unsigned t = 0; t < CANOR_ENUM_TRIFECTAS; ++t) {
    uint8_t local[CANOR_ENUM_TRIFECTA_SIZE];
    uint64_t p;
    unsigned base = 1u + t * CANOR_ENUM_TRIFECTA_SIZE;
    for (unsigned k = 0; k < CANOR_ENUM_TRIFECTA_SIZE; ++k) {
      const canor_soul_usage_t *u = &f->at[t * CANOR_ENUM_TRIFECTA_SIZE + k];
      if (u->ring != 0 || u->atom < base || u->atom >= base + CANOR_ENUM_TRIFECTA_SIZE)
        return CANOR_ERR_NOT_FOUND;
      local[k] = (uint8_t)(u->atom - base);
    }
    if (!enum_perm_rank(local, CANOR_ENUM_TRIFECTA_SIZE, &p)) return CANOR_ERR_NOT_FOUND;
    acc = acc * ENUM_FLOW_RADIX + p;
  }
  *r = acc;
  return CANOR_OK;
}

/* --- MetaFlow ---------------------------------------------------------------- */

canor_status canor_enum_metaflow_unrank(uint64_t r, canor_enum_metaflow_t *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (r >= CANOR_ENUM_METAFLOW_COUNT) return CANOR_ERR_RANGE;
  enum_perm_unrank(r, CANOR_ENUM_METAFLOW_ATOMS, out->atom);
  return CANOR_OK;
}

canor_status canor_enum_metaflow_rank(const canor_enum_metaflow_t *m, uint64_t *r) {
  if (!m || !r) return CANOR_ERR_ARGS;
  return enum_perm_rank(m->atom, CANOR_ENUM_METAFLOW_ATOMS, r) ? CANOR_OK : CANOR_ERR_ARGS;
}

canor_status canor_enum_invocation_unrank(uint64_t r, canor_enum_fractal_t *out) {
  canor_enum_fractal_t x;
  canor_status st;
  if (!out) return CANOR_ERR_ARGS;
  if (r >= CANOR_ENUM_INVOCATION_COUNT) return CANOR_ERR_RANGE;
  memset(&x, 0, sizeof x);
  if ((st = canor_enum_metaflow_unrank(r / CANOR_ENUM_STRATEGY_COUNT, &x.slots)) != CANOR_OK)
    return st;
  if ((st = canor_enum_strategy_unrank(r % CANOR_ENUM_STRATEGY_COUNT, &x.strategy)) != CANOR_OK)
    return st;
  x.base = CANOR_ENUM_FRACTAL_BASE;
  x.direction = x.strategy.differential;
  x.depth = x.strategy.order;
  /* Generation j holds (b-1)^j intervals of length b^(k-j); each loses its
   * middle part of length b^(k-1-j). */
  uint64_t bp[CANOR_ENUM_ORDER_MAX + 1], kp[CANOR_ENUM_ORDER_MAX + 1];
  bp[0] = kp[0] = 1;
  for (unsigned j = 1; j <= x.depth; ++j) {
    bp[j] = bp[j - 1] * x.base;
    kp[j] = kp[j - 1] * (x.base - 1u);
  }
  x.support = bp[x.depth];
  x.reals = kp[x.depth];
  for (unsigned j = 0; j < x.depth; ++j) {
    x.gap_len[j] = bp[x.depth - 1 - j];
    x.gap_mult[j] = kp[j];
    x.gaps += x.gap_len[j] * x.gap_mult[j];
  }
  *out = x;
  return CANOR_OK;
}

/* --- Soul -------------------------------------------------------------------- */

canor_status canor_enum_soul_unrank(uint64_t r, canor_enum_soul_t *out) {
  canor_status st;
  if (!out) return CANOR_ERR_ARGS;
  if (r >= CANOR_ENUM_SOUL_COUNT) return CANOR_ERR_RANGE;
  out->rank = r;
  out->metaflow_rank = (uint32_t)(r % CANOR_ENUM_METAFLOW_COUNT);
  r /= CANOR_ENUM_METAFLOW_COUNT;
  out->flow_rank = (uint32_t)(r % CANOR_ENUM_FLOW_COUNT);
  out->strategy_rank = (uint32_t)(r / CANOR_ENUM_FLOW_COUNT);
  if ((st = canor_enum_strategy_unrank(out->strategy_rank, &out->strategy)) != CANOR_OK) return st;
  if ((st = canor_enum_flow_unrank(out->flow_rank, &out->flow)) != CANOR_OK) return st;
  if ((st = canor_enum_metaflow_unrank(out->metaflow_rank, &out->metaflow)) != CANOR_OK) return st;
  if ((st = canor_soul_stream(&out->flow.flow, NULL, CANOR_SOUL_L2R, &out->stream)) != CANOR_OK)
    return st;
  /* An enumerated symbol is one being written to a store. */
  out->category = CANOR_SOUL_STORAGE;
  return CANOR_OK;
}

canor_status canor_enum_soul_rep_unrank(canor_soul_kind k, uint64_t r, canor_enum_soul_t *soul,
                                        canor_soul_rep_t *rep) {
  canor_status st;
  if (!soul || !rep || (unsigned)k >= CANOR_SOUL_KINDS) return CANOR_ERR_ARGS;
  uint64_t per = canor_soul_kind_count(k, CANOR_ENUM_FLOW_ATOMS);
  if (r >= CANOR_ENUM_SOUL_COUNT * per) return CANOR_ERR_RANGE;
  if ((st = canor_enum_soul_unrank(r / per, soul)) != CANOR_OK) return st;
  return canor_soul_represent(&soul->stream, k, r % per, rep);
}

/* --- Action ------------------------------------------------------------------ */

static void enum_action_fill(uint64_t m, uint64_t n, uint64_t o, canor_enum_action_t *out) {
  out->m = (uint16_t)m;
  out->n = (uint16_t)n;
  out->o = (uint16_t)o;
  out->rank = (m * CANOR_ENUM_ACTION_BOUND + n) * CANOR_ENUM_ACTION_BOUND + o;
  if (o == 0) {
    /* No canonical residue exists (0 <= r < 0 is empty): kept, diagnosed. */
    out->rm = out->rn = 0;
    out->status = CANOR_ERR_UNDEFINED;
    out->holds = false;
    return;
  }
  out->rm = (uint16_t)(m % o);
  out->rn = (uint16_t)(n % o);
  out->status = CANOR_OK;
  out->holds = out->rm == out->rn;
}

canor_status canor_enum_action_unrank(uint64_t r, canor_enum_action_t *out) {
  if (!out) return CANOR_ERR_ARGS;
  if (r >= CANOR_ENUM_ACTION_COUNT) return CANOR_ERR_RANGE;
  uint64_t o = r % CANOR_ENUM_ACTION_BOUND, t = r / CANOR_ENUM_ACTION_BOUND;
  enum_action_fill(t / CANOR_ENUM_ACTION_BOUND, t % CANOR_ENUM_ACTION_BOUND, o, out);
  return CANOR_OK;
}

canor_status canor_enum_action_rank(uint64_t m, uint64_t n, uint64_t o, uint64_t *r) {
  if (!r) return CANOR_ERR_ARGS;
  if (m >= CANOR_ENUM_ACTION_BOUND || n >= CANOR_ENUM_ACTION_BOUND || o >= CANOR_ENUM_ACTION_BOUND)
    return CANOR_ERR_RANGE;
  *r = (m * CANOR_ENUM_ACTION_BOUND + n) * CANOR_ENUM_ACTION_BOUND + o;
  return CANOR_OK;
}

/* --- Maneuver ---------------------------------------------------------------- */

canor_status canor_enum_maneuver_unrank(uint64_t r, canor_enum_maneuver_t *out) {
  canor_status st;
  if (!out) return CANOR_ERR_ARGS;
  if (r >= CANOR_ENUM_MANEUVER_COUNT) return CANOR_ERR_RANGE;
  out->rank = r;
  if ((st = canor_enum_soul_unrank(r / CANOR_ENUM_ACTION_COUNT, &out->soul)) != CANOR_OK) return st;
  if ((st = canor_enum_action_unrank(r % CANOR_ENUM_ACTION_COUNT, &out->action)) != CANOR_OK)
    return st;
  if (out->action.status == CANOR_OK)
    return canor_soul_residues(&out->soul.stream, out->action.o, &out->residues);
  memset(&out->residues, 0, sizeof out->residues);   /* none: the Action is undefined */
  return CANOR_OK;
}

canor_status canor_enum_maneuver_rep_unrank(canor_soul_kind k, uint64_t r,
                                            canor_enum_maneuver_t *man, canor_soul_rep_t *rep) {
  canor_status st;
  if (!man || !rep || (unsigned)k >= CANOR_SOUL_KINDS) return CANOR_ERR_ARGS;
  uint64_t per = canor_soul_kind_count(k, CANOR_ENUM_FLOW_ATOMS);
  if (r / per >= CANOR_ENUM_MANEUVER_COUNT) return CANOR_ERR_RANGE;
  if ((st = canor_enum_maneuver_unrank(r / per, man)) != CANOR_OK) return st;
  if (man->action.status != CANOR_OK) return CANOR_ERR_UNDEFINED;
  return canor_soul_represent(&man->residues, k, r % per, rep);
}

/* --- rendering --------------------------------------------------------------- */

static canor_status enum_fit(int n, size_t cap) {
  return (n < 0 || (size_t)n >= cap) ? CANOR_ERR_FULL : CANOR_OK;
}

canor_status canor_enum_strategy_format(const canor_enum_strategy_t *s, char *buf, size_t cap) {
  if (!s || !buf || s->differential < 1 || s->differential > CANOR_ENUM_DIFFERENTIALS)
    return CANOR_ERR_ARGS;
  return enum_fit(snprintf(buf, cap, "d%s^%u", canor_imag_name(s->differential),
                           (unsigned)s->order), cap);
}

canor_status canor_enum_flow_format(const canor_soul_flow_t *f, char *buf, size_t cap) {
  size_t k = 0;
  if (!f || !buf || cap == 0) return CANOR_ERR_ARGS;
  for (uint16_t p = 0; p < f->n; ++p) {
    char one[24];
    /* A trifecta boundary every three positions, rings shown only when set. */
    int len = f->at[p].ring
                  ? snprintf(one, sizeof one, "%s%s%u", (p && p % CANOR_ENUM_TRIFECTA_SIZE == 0) ? "|" : "",
                             canor_imag_name(f->at[p].atom), (unsigned)f->at[p].ring)
                  : snprintf(one, sizeof one, "%s%s", (p && p % CANOR_ENUM_TRIFECTA_SIZE == 0) ? "|" : "",
                             canor_imag_name(f->at[p].atom));
    if (len < 0 || k + (size_t)len >= cap) return CANOR_ERR_FULL;
    memcpy(buf + k, one, (size_t)len);
    k += (size_t)len;
  }
  buf[k] = '\0';
  return CANOR_OK;
}

canor_status canor_enum_metaflow_format(const canor_enum_metaflow_t *m, char *buf, size_t cap) {
  if (!m || !buf) return CANOR_ERR_ARGS;
  if (cap <= CANOR_ENUM_METAFLOW_ATOMS) return CANOR_ERR_FULL;
  for (unsigned k = 0; k < CANOR_ENUM_METAFLOW_ATOMS; ++k)
    buf[k] = canor_enum_metaflow_label(m->atom[k])[0];
  buf[CANOR_ENUM_METAFLOW_ATOMS] = '\0';
  return CANOR_OK;
}

canor_status canor_enum_soul_format(const canor_enum_soul_t *s, char *buf, size_t cap) {
  char st[CANOR_ENUM_SYMBOL_MAX], fl[CANOR_ENUM_SYMBOL_MAX], mf[CANOR_ENUM_SYMBOL_MAX];
  canor_status r;
  if (!s || !buf) return CANOR_ERR_ARGS;
  if ((r = canor_enum_strategy_format(&s->strategy, st, sizeof st)) != CANOR_OK) return r;
  if ((r = canor_enum_flow_format(&s->flow.flow, fl, sizeof fl)) != CANOR_OK) return r;
  if ((r = canor_enum_metaflow_format(&s->metaflow, mf, sizeof mf)) != CANOR_OK) return r;
  return enum_fit(snprintf(buf, cap, "%s:%s:%s", st, fl, mf), cap);
}

/* U+2263 STRICTLY EQUIVALENT TO, the operator the brief writes. */
#define ENUM_STRICTLY_EQUIVALENT "\xE2\x89\xA3"

canor_status canor_enum_action_format(const canor_enum_action_t *a, char *buf, size_t cap) {
  if (!a || !buf) return CANOR_ERR_ARGS;
  return enum_fit(snprintf(buf, cap, "%u " ENUM_STRICTLY_EQUIVALENT " %u (mod %u)",
                           (unsigned)a->m, (unsigned)a->n, (unsigned)a->o), cap);
}

canor_status canor_enum_rational_format(canor_rational q, char *buf, size_t cap) {
  if (!buf || q.den <= 0) return CANOR_ERR_ARGS;
  if (q.den == 1) return enum_fit(snprintf(buf, cap, "%lld", (long long)q.num), cap);
  return enum_fit(snprintf(buf, cap, "%lld/%lld", (long long)q.num, (long long)q.den), cap);
}

const char *canor_enum_metaflow_label(uint8_t atom) {
  static const char *const n[CANOR_ENUM_METAFLOW_ATOMS] = {"R", "G", "B", "A"};
  return atom < CANOR_ENUM_METAFLOW_ATOMS ? n[atom] : "?";
}

const char *canor_enum_metaflow_label_name(uint8_t atom) {
  static const char *const n[CANOR_ENUM_METAFLOW_ATOMS] = {"Reals", "Gaps", "Bases", "Auxiliaries"};
  return atom < CANOR_ENUM_METAFLOW_ATOMS ? n[atom] : "?";
}
