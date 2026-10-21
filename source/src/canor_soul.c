/* canor_soul.c - count streams and their representations (Canor WD v1 §4 Soul).
 * See canor_soul.h for the definitions this implements. */

#include "canor_soul.h"

#include <string.h>

/* Trifecta / component of an IMAG unit under the pinned basis, or -1 for the
 * atoms that belong to no trifecta (`one`). */
static int soul_slot(canor_imag atom, int *t, int *c) {
  if (atom == 0 || atom >= CANOR_IMAG_COUNT) return -1;
  *t = (atom - 1) / CANOR_SOUL_COMPONENTS;
  *c = (atom - 1) % CANOR_SOUL_COMPONENTS;
  return 0;
}

canor_status canor_soul_flow_make(const canor_imag *atoms, uint16_t n, canor_soul_flow_t *out) {
  if (!out || (n && !atoms)) return CANOR_ERR_ARGS;
  if (n > CANOR_SOUL_FLOW_MAX) return CANOR_ERR_FULL;
  out->n = n;
  for (uint16_t k = 0; k < n; ++k) {
    if (atoms[k] >= CANOR_IMAG_COUNT) return CANOR_ERR_RANGE;
    out->at[k].atom = atoms[k];
    out->at[k].ring = 0;
  }
  return CANOR_OK;
}

canor_status canor_soul_order_check(const uint16_t *order, uint16_t n) {
  bool seen[CANOR_SOUL_FLOW_MAX] = {false};
  if (n > CANOR_SOUL_FLOW_MAX) return CANOR_ERR_FULL;
  if (n && !order) return CANOR_ERR_ARGS;
  for (uint16_t k = 0; k < n; ++k) {
    if (order[k] >= n || seen[order[k]]) return CANOR_ERR_ARGS;
    seen[order[k]] = true;
  }
  return CANOR_OK;
}

canor_status canor_soul_stream(const canor_soul_flow_t *f, const uint16_t *order,
                               canor_soul_direction dir, canor_soul_stream_t *out) {
  canor_soul_stream_t s;
  canor_status st;
  int t, c;
  if (!f || !out) return CANOR_ERR_ARGS;
  if (f->n > CANOR_SOUL_FLOW_MAX) return CANOR_ERR_FULL;
  if (dir != CANOR_SOUL_L2R && dir != CANOR_SOUL_R2L) return CANOR_ERR_ARGS;
  if (order && (st = canor_soul_order_check(order, f->n)) != CANOR_OK) return st;

  memset(&s, 0, sizeof s);
  s.n = f->n;
  /* Initial vector: every occurrence counts, duplicates included. */
  for (uint16_t k = 0; k < f->n; ++k) {
    if (f->at[k].atom >= CANOR_IMAG_COUNT) return CANOR_ERR_RANGE;
    if (soul_slot(f->at[k].atom, &t, &c) == 0) ++s.v[0][t][c];
    else ++s.aux[0];
  }
  for (uint16_t k = 0; k < f->n; ++k) {
    uint16_t p = order ? order[k] : (dir == CANOR_SOUL_L2R ? k : (uint16_t)(f->n - 1u - k));
    s.pos[k] = p;
    memcpy(s.v[k + 1], s.v[k], sizeof s.v[k]);
    s.aux[k + 1] = s.aux[k];
    /* Decrement every component whose name matches. Under the pinned basis an
     * atom has exactly one component; the loop states the general rule. */
    for (t = 0; t < CANOR_SOUL_TRIFECTAS; ++t)
      for (c = 0; c < CANOR_SOUL_COMPONENTS; ++c)
        if (f->at[p].atom == (canor_imag)(1 + t * CANOR_SOUL_COMPONENTS + c)) {
          if (s.v[k + 1][t][c] == 0) return CANOR_ERR_RANGE;   /* cannot go below 0 */
          --s.v[k + 1][t][c];
        }
    if (f->at[p].atom == 0) {
      if (s.aux[k + 1] == 0) return CANOR_ERR_RANGE;
      --s.aux[k + 1];
    }
  }
  *out = s;
  return CANOR_OK;
}

canor_status canor_soul_residues(const canor_soul_stream_t *s, uint32_t modulus,
                                 canor_soul_stream_t *out) {
  canor_soul_stream_t r;
  if (!s || !out) return CANOR_ERR_ARGS;
  if (modulus == 0) return CANOR_ERR_UNDEFINED;
  r = *s;
  for (uint16_t k = 0; k <= s->n; ++k) {
    for (int t = 0; t < CANOR_SOUL_TRIFECTAS; ++t)
      for (int c = 0; c < CANOR_SOUL_COMPONENTS; ++c)
        r.v[k][t][c] = (uint16_t)(s->v[k][t][c] % modulus);
    r.aux[k] = (uint16_t)(s->aux[k] % modulus);
  }
  *out = r;
  return CANOR_OK;
}

canor_status canor_soul_value(const canor_soul_stream_t *s, uint16_t step, canor_imag base,
                              uint16_t *out) {
  int t, c;
  if (!s || !out) return CANOR_ERR_ARGS;
  if (step > s->n || soul_slot(base, &t, &c) != 0) return CANOR_ERR_RANGE;
  *out = s->v[step][t][c];
  return CANOR_OK;
}

bool canor_soul_kind_reals(canor_soul_kind k) {
  return k == CANOR_SOUL_COMPOSITE || k == CANOR_SOUL_UNKNOWN;
}

bool canor_soul_kind_bases(canor_soul_kind k) {
  return k == CANOR_SOUL_UNIQUE || k == CANOR_SOUL_UNKNOWN;
}

uint64_t canor_soul_kind_count(canor_soul_kind k, uint16_t n) {
  if ((unsigned)k >= CANOR_SOUL_KINDS) return 0;
  uint64_t steps = canor_soul_kind_reals(k) ? 1 : (uint64_t)n + 1;
  uint64_t bases = canor_soul_kind_bases(k) ? 1 : CANOR_SOUL_BASES;
  return steps * bases;
}

canor_status canor_soul_represent(const canor_soul_stream_t *s, canor_soul_kind k, uint64_t idx,
                                  canor_soul_rep_t *out) {
  canor_soul_rep_t r;
  uint16_t s0, s1, b0, b1;
  if (!s || !out || (unsigned)k >= CANOR_SOUL_KINDS) return CANOR_ERR_ARGS;
  if (idx >= canor_soul_kind_count(k, s->n)) return CANOR_ERR_RANGE;
  bool reals = canor_soul_kind_reals(k), bases = canor_soul_kind_bases(k);
  if (!reals && !bases) {
    s0 = (uint16_t)(idx / CANOR_SOUL_BASES);
    b0 = (uint16_t)(1 + idx % CANOR_SOUL_BASES);
  } else if (!reals) {
    s0 = (uint16_t)idx;
    b0 = 1;
  } else if (!bases) {
    s0 = 0;
    b0 = (uint16_t)(1 + idx);
  } else {
    s0 = 0;
    b0 = 1;
  }
  s1 = reals ? s->n : s0;
  b1 = bases ? (uint16_t)CANOR_SOUL_BASES : b0;
  r.kind = k;
  r.step = reals ? CANOR_SOUL_ALL : s0;
  r.base = bases ? CANOR_SOUL_ALL : b0;
  r.nreals = (uint16_t)(s1 - s0 + 1);
  r.nbases = (uint16_t)(b1 - b0 + 1);
  for (uint16_t st = s0; st <= s1; ++st)
    for (uint16_t b = b0; b <= b1; ++b)
      (void)canor_soul_value(s, st, (canor_imag)b,
                             &r.value[(st - s0) * r.nbases + (b - b0)]);
  *out = r;
  return CANOR_OK;
}

const char *canor_soul_kind_name(canor_soul_kind k) {
  static const char *const n[CANOR_SOUL_KINDS] = {"Elementary", "Unique", "Composite", "Unknown"};
  return (unsigned)k < CANOR_SOUL_KINDS ? n[k] : "?";
}

const char *canor_soul_kind_shape(canor_soul_kind k) {
  static const char *const n[CANOR_SOUL_KINDS] = {"Real, Base", "Real, Bases", "Reals, Base",
                                                  "Reals, Bases"};
  return (unsigned)k < CANOR_SOUL_KINDS ? n[k] : "?";
}

const char *canor_soul_category_name(canor_soul_category c) {
  switch (c) {
    case CANOR_SOUL_STORAGE: return "Storage";
    case CANOR_SOUL_LEARNING: return "Learning";
    case CANOR_SOUL_OPERATION: return "Operation";
    default: return "?";
  }
}
