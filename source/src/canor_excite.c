/* Excitations. See canor_excite.h for what is and is not claimed.
 *
 * Everything here is integer or exact-rational. The bracket of two basis
 * elements is 0 or +-2.e_k, so no rounding can enter, and the Jacobiator is a
 * small integer vector rather than a residual to be compared against a
 * tolerance. That matters: "Jacobi fails at dimension 8" is then a statement
 * about zero versus nonzero, not about a threshold someone chose.
 */

#include "canor_excite.h"

#include <string.h>

static void vec_zero(ex_vec *v) { memset(v, 0, sizeof *v); }

static void vec_add(ex_vec *v, int32_t coeff, uint8_t unit) {
  if (unit < EX_MAX_DIM) v->c[unit] += coeff;
}

bool ex_vec_is_zero(const ex_vec *v) {
  int i;
  for (i = 0; i < EX_MAX_DIM; ++i) if (v->c[i]) return false;
  return true;
}

bool ex_bracket_of(uint8_t a, uint8_t b, ex_bracket *out) {
  canor_imag ab, ba;
  int sab, sba;
  if (!out) return false;
  if (canor_imag_mul(a, b, &ab, &sab) != CANOR_OK) return false;
  if (canor_imag_mul(b, a, &ba, &sba) != CANOR_OK) return false;
  /* The two products land on the same unit - a Cayley-Dickson basis is closed
   * and e_a e_b and e_b e_a differ at most in sign - so the bracket is
   * (sab - sba) e_ab. Asserting rather than assuming: if the units ever
   * differed the subtraction below would be meaningless, so it is refused. */
  if (ab != ba) return false;
  out->coeff = (int32_t)(sab - sba);
  out->unit = out->coeff ? (uint8_t)ab : 0;
  return true;
}

/* [coeff.e_u, e_c] = coeff.[e_u, e_c] */
static bool bracket_scaled(int32_t coeff, uint8_t u, uint8_t c, ex_vec *acc) {
  ex_bracket br;
  if (coeff == 0) return true;
  if (!ex_bracket_of(u, c, &br)) return false;
  if (br.coeff) vec_add(acc, coeff * br.coeff, br.unit);
  return true;
}

bool ex_jacobiator(uint8_t a, uint8_t b, uint8_t c, ex_vec *out) {
  ex_bracket ab, bc, ca;
  if (!out) return false;
  vec_zero(out);
  if (!ex_bracket_of(a, b, &ab)) return false;
  if (!ex_bracket_of(b, c, &bc)) return false;
  if (!ex_bracket_of(c, a, &ca)) return false;
  if (!bracket_scaled(ab.coeff, ab.unit, c, out)) return false;
  if (!bracket_scaled(bc.coeff, bc.unit, a, out)) return false;
  if (!bracket_scaled(ca.coeff, ca.unit, b, out)) return false;
  return true;
}

bool ex_associator(uint8_t a, uint8_t b, uint8_t c, ex_vec *out) {
  canor_imag ab, ab_c, bc, a_bc;
  int s1, s2, s3, s4;
  if (!out) return false;
  vec_zero(out);
  if (canor_imag_mul(a, b, &ab, &s1) != CANOR_OK) return false;
  if (canor_imag_mul(ab, c, &ab_c, &s2) != CANOR_OK) return false;
  if (canor_imag_mul(b, c, &bc, &s3) != CANOR_OK) return false;
  if (canor_imag_mul(a, bc, &a_bc, &s4) != CANOR_OK) return false;
  vec_add(out, s1 * s2, (uint8_t)ab_c);
  vec_add(out, -(s3 * s4), (uint8_t)a_bc);
  return true;
}

bool ex_census_run(int dim, ex_census *out) {
  int a, b, c;
  if (!out || dim < 1 || dim > EX_MAX_DIM) return false;
  memset(out, 0, sizeof *out);
  for (a = 0; a < dim; ++a)
    for (b = 0; b < dim; ++b) {
      ex_bracket br;
      ++out->pairs;
      if (!ex_bracket_of((uint8_t)a, (uint8_t)b, &br)) return false;
      if (br.coeff == 0) ++out->commuting;
    }
  /* Unordered triples of DISTINCT elements. Repeats make both the Jacobiator
   * and the associator vanish for trivial reasons and would dilute the counts
   * with cases that could never have failed. */
  for (a = 0; a < dim; ++a)
    for (b = a + 1; b < dim; ++b)
      for (c = b + 1; c < dim; ++c) {
        ex_vec j, s;
        ++out->triples;
        if (!ex_jacobiator((uint8_t)a, (uint8_t)b, (uint8_t)c, &j)) return false;
        if (!ex_associator((uint8_t)a, (uint8_t)b, (uint8_t)c, &s)) return false;
        if (!ex_vec_is_zero(&j)) ++out->jacobi_fail;
        if (!ex_vec_is_zero(&s)) ++out->assoc_fail;
      }
  return true;
}

bool ex_build(ex_excitation *e, int dim, const canor_ore *o, canor_absop op) {
  int a, b;
  if (!e || dim < 1 || dim > EX_MAX_DIM) return false;
  memset(e, 0, sizeof *e);
  e->dim = dim;
  e->skewed = (o != NULL);
  for (a = 0; a < dim; ++a)
    for (b = 0; b < dim; ++b) {
      if (!ex_bracket_of((uint8_t)a, (uint8_t)b, &e->b[a][b])) return false;
      e->w[a][b] = canor_rat(1, 1);
      if (o) {
        /* The weight is a function of the channel PAIR and the operator only.
         * Nothing external enters, so two runs of the same Excitation are the
         * same Excitation. */
        canor_rational v;
        bool ok;
        int32_t idx = (int32_t)(a * dim + b);
        switch (op) {
          case CANOR_ABS_SIN: ok = canor_ore_sin(o, idx, &v); break;
          case CANOR_ABS_COS: ok = canor_ore_cos(o, idx, &v); break;
          default:         ok = canor_ore_tan(o, idx, &v); break;
        }
        /* A refused operator leaves the weight at 1 rather than at 0: zero
         * would silently delete the bracket, which is a different Excitation,
         * not a missing weight. */
        if (ok && canor_rat_valid(v)) e->w[a][b] = v;
      }
    }
  return true;
}

bool ex_distort(const ex_excitation *a, const ex_excitation *b,
                ex_distortion *out) {
  int i, j, dim;
  if (!a || !b || !out) return false;
  memset(out, 0, sizeof *out);
  dim = a->dim < b->dim ? a->dim : b->dim;
  for (i = 0; i < dim; ++i)
    for (j = 0; j < dim; ++j) {
      ++out->pairs;
      if (a->b[i][j].coeff != b->b[i][j].coeff ||
          a->b[i][j].unit != b->b[i][j].unit) {
        ++out->unit_differs;
        continue;                       /* structural difference dominates */
      }
      if (!canor_rat_eq(a->w[i][j], b->w[i][j])) ++out->weight_differs;
    }
  return true;
}

/* --- groupings -------------------------------------------------------------- */

const char *ex_grouping_name(ex_grouping k) {
  return k == EX_GROUP_INDEX ? "index" :
         k == EX_GROUP_QUATERNION ? "quaternion" :
         k == EX_GROUP_SPREAD ? "spread" : "fano";
}

bool ex_is_quaternion_triple(uint8_t a, uint8_t b, uint8_t c) {
  canor_imag p;
  int s;
  if (a == 0 || b == 0 || c == 0) return false;
  if (a == b || b == c || a == c) return false;
  if (canor_imag_mul(a, b, &p, &s) != CANOR_OK || p != c) return false;
  if (canor_imag_mul(b, c, &p, &s) != CANOR_OK || p != a) return false;
  if (canor_imag_mul(c, a, &p, &s) != CANOR_OK || p != b) return false;
  return true;
}

/* True when every product of two members lands back inside the triple. */
static bool triple_closes(const uint8_t t[3]) {
  int i, j;
  for (i = 0; i < 3; ++i)
    for (j = 0; j < 3; ++j) {
      canor_imag p;
      int s;
      if (canor_imag_mul(t[i], t[j], &p, &s) != CANOR_OK) return false;
      if (p == 0) continue;                 /* the identity is always allowed */
      if (p != t[0] && p != t[1] && p != t[2]) return false;
    }
  return true;
}

/* Depth-first pick of disjoint closing triples, taking the first solution.
 * Lexicographic, hence deterministic: the partition is a function of the
 * multiplication table, not of the order the search happened to run in. */
static bool spread_search(const ex_groups *all, ex_groups *out, int depth,
                          unsigned used, int start, int want) {
  int t, k;
  if (depth == want) return true;
  for (t = start; t < all->count; ++t) {
    unsigned m = 0;
    int clash = 0;
    for (k = 0; k < 3; ++k) {
      unsigned bit = 1u << all->u[t][k];
      if (used & bit) clash = 1;
      m |= bit;
    }
    if (clash) continue;
    for (k = 0; k < 3; ++k) out->u[depth][k] = all->u[t][k];
    out->closes[depth] = true;
    if (spread_search(all, out, depth + 1, used | m, t + 1, want)) return true;
  }
  return false;
}

bool ex_groups_build(ex_groups *g, ex_grouping kind, int dim) {
  int a, b, c;
  if (!g || dim < 1 || dim > EX_MAX_DIM) return false;
  memset(g, 0, sizeof *g);
  g->kind = kind;

  if (kind == EX_GROUP_SPREAD) {
    ex_groups all;
    int want = (dim - 1) / 3;
    if (want < 1) return false;
    if (!ex_groups_build(&all, EX_GROUP_QUATERNION, dim)) return false;
    if (!spread_search(&all, g, 0, 0u, 0, want)) return false;
    g->kind = kind;
    g->count = want;
    g->partition = true;
    return true;
  }

  if (kind == EX_GROUP_INDEX) {
    int t;
    for (t = 0; 3 * t + 3 < dim && g->count < EX_MAX_GROUPS; ++t) {
      g->u[g->count][0] = (uint8_t)(3 * t + 1);
      g->u[g->count][1] = (uint8_t)(3 * t + 2);
      g->u[g->count][2] = (uint8_t)(3 * t + 3);
      g->closes[g->count] = triple_closes(g->u[g->count]);
      ++g->count;
    }
    g->partition = true;                    /* disjoint by construction */
    return true;
  }

  /* Both remaining kinds enumerate genuine quaternion triples; Fano restricts
   * the search to e1..e7, where those triples ARE the seven lines. */
  {
    int hi = (kind == EX_GROUP_FANO) ? 8 : dim;
    if (hi > dim) hi = dim;
    for (a = 1; a < hi; ++a)
      for (b = a + 1; b < hi; ++b)
        for (c = b + 1; c < hi; ++c) {
          /* Closure is orientation-independent, so test both cyclic senses
           * rather than missing a triple because it was written backwards. */
          if (!ex_is_quaternion_triple((uint8_t)a, (uint8_t)b, (uint8_t)c) &&
              !ex_is_quaternion_triple((uint8_t)a, (uint8_t)c, (uint8_t)b))
            continue;
          if (g->count >= EX_MAX_GROUPS) return true;   /* truncated, reported */
          g->u[g->count][0] = (uint8_t)a;
          g->u[g->count][1] = (uint8_t)b;
          g->u[g->count][2] = (uint8_t)c;
          g->closes[g->count] = true;
          ++g->count;
        }
  }
  /* Overlapping by nature: a unit sits in many triples. Saying so matters,
   * because a caller that assumed a partition would double-count. */
  g->partition = false;
  return true;
}


/* --- spreads and GL(4,2) ---------------------------------------------------- */

bool ex_index_xor_holds(void) {
  int a, b;
  for (a = 1; a < CANOR_IMAG_COUNT; ++a)
    for (b = 1; b < CANOR_IMAG_COUNT; ++b) {
      canor_imag p;
      int sg;
      if (a == b) continue;
      if (canor_imag_mul((canor_imag)a, (canor_imag)b, &p, &sg) != CANOR_OK) return false;
      if ((int)p != (a ^ b)) return false;
    }
  return true;
}

static void spread_rec(const ex_groups *all, ex_spread_set *out,
                       uint8_t cur[5][3], int depth, unsigned used, int start) {
  int t, k;
  if (depth == 5) {
    if (out->count < EX_SPREADS_MAX) {
      memcpy(out->line[out->count], cur, sizeof cur[0] * 5);
      ++out->count;
    }
    return;
  }
  for (t = start; t < all->count; ++t) {
    unsigned m = 0;
    int clash = 0;
    for (k = 0; k < 3; ++k) {
      unsigned bit = 1u << all->u[t][k];
      if (used & bit) clash = 1;
      m |= bit;
    }
    if (clash) continue;
    for (k = 0; k < 3; ++k) cur[depth][k] = all->u[t][k];
    spread_rec(all, out, cur, depth + 1, used | m, t + 1);
  }
}

bool ex_spreads_all(ex_spread_set *out) {
  ex_groups all;
  uint8_t cur[5][3];
  if (!out) return false;
  memset(out, 0, sizeof *out);
  if (!ex_groups_build(&all, EX_GROUP_QUATERNION, CANOR_IMAG_COUNT)) return false;
  spread_rec(&all, out, cur, 0, 0u, 0);
  return out->count > 0;
}

/* A GL(4,2) element as four 4-bit rows. Applying it to an index means reading
 * the index as a vector of F2^4 and taking parities. */
static int gl_apply(const uint8_t row[4], int v) {
  int out = 0, i;
  for (i = 0; i < 4; ++i) {
    int dot = row[i] & v, par = 0;
    while (dot) { par ^= dot & 1; dot >>= 1; }
    if (par) out |= 1 << i;
  }
  return out;
}

static bool gl_invertible(const uint8_t row[4]) {
  int seen[16], v;
  memset(seen, 0, sizeof seen);
  for (v = 1; v < 16; ++v) {
    int w = gl_apply(row, v);
    if (w == 0 || seen[w]) return false;
    seen[w] = 1;
  }
  return true;
}

/* Canonical form of a spread: each line sorted, then the five lines sorted, so
 * two spreads are equal exactly when their canonical forms are. Without this
 * the same spread reached by a different group element would count twice. */
static void spread_canon(const uint8_t in[5][3], uint8_t out[5][3]) {
  int i, j, k;
  uint8_t tmp[5][3];
  for (i = 0; i < 5; ++i) {
    for (k = 0; k < 3; ++k) tmp[i][k] = in[i][k];
    for (j = 0; j < 3; ++j)
      for (k = j + 1; k < 3; ++k)
        if (tmp[i][k] < tmp[i][j]) { uint8_t t = tmp[i][j]; tmp[i][j] = tmp[i][k]; tmp[i][k] = t; }
  }
  for (i = 0; i < 5; ++i)
    for (j = i + 1; j < 5; ++j)
      if (tmp[j][0] < tmp[i][0]) {
        uint8_t t[3];
        memcpy(t, tmp[i], 3); memcpy(tmp[i], tmp[j], 3); memcpy(tmp[j], t, 3);
      }
  memcpy(out, tmp, sizeof tmp);
}

static int spread_find(const ex_spread_set *sp, const uint8_t c[5][3]) {
  int i;
  uint8_t k1[5][3], k2[5][3];
  spread_canon(c, k1);
  for (i = 0; i < sp->count; ++i) {
    spread_canon(sp->line[i], k2);
    if (!memcmp(k1, k2, sizeof k1)) return i;
  }
  return -1;
}

bool ex_spread_orbit_split(const ex_spread_set *sp, ex_spread_orbits *out) {
  uint8_t rows[4];
  int seen[EX_SPREADS_MAX];
  int i, j, k, m0, m1, m2, m3;

  if (!sp || !out) return false;
  memset(out, 0, sizeof *out);
  memset(seen, 0, sizeof seen);

  /* The whole of GL(4,2), found by filtering 2^16 candidate matrices. */
  for (m0 = 0; m0 < 16; ++m0)
    for (m1 = 0; m1 < 16; ++m1)
      for (m2 = 0; m2 < 16; ++m2)
        for (m3 = 0; m3 < 16; ++m3) {
          rows[0] = (uint8_t)m0; rows[1] = (uint8_t)m1;
          rows[2] = (uint8_t)m2; rows[3] = (uint8_t)m3;
          if (gl_invertible(rows)) ++out->group_order;
        }

  for (i = 0; i < sp->count; ++i) {
    long orbit = 0, stab = 0;
    if (seen[i]) continue;
    for (m0 = 0; m0 < 16; ++m0)
      for (m1 = 0; m1 < 16; ++m1)
        for (m2 = 0; m2 < 16; ++m2)
          for (m3 = 0; m3 < 16; ++m3) {
            uint8_t img[5][3];
            int idx;
            rows[0] = (uint8_t)m0; rows[1] = (uint8_t)m1;
            rows[2] = (uint8_t)m2; rows[3] = (uint8_t)m3;
            if (!gl_invertible(rows)) continue;
            for (j = 0; j < 5; ++j)
              for (k = 0; k < 3; ++k)
                img[j][k] = (uint8_t)gl_apply(rows, sp->line[i][j][k]);
            idx = spread_find(sp, img);
            if (idx < 0) continue;             /* image is not a spread: impossible */
            if (idx == i) ++stab;
            if (!seen[idx]) { seen[idx] = 1; ++orbit; }
          }
    out->size[out->norbits] = (int)orbit;
    out->rep[out->norbits] = i;
    out->stabiliser[out->norbits] = stab;
    ++out->norbits;
  }
  return true;
}
