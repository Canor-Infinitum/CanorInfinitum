/* Generalized quaternionic tensor product algebras. See canor_tensor.h. */

#include "canor_tensor.h"

#include <string.h>

/* fc.h supplies canor_rat, canor_rat_add and canor_rat_mul and nothing else, so
 * negation, subtraction and division are built here from those. Each routes
 * through canor_rat, which returns the poison for a zero denominator and for the
 * unnegatable INT64_MIN - so poison propagates rather than a plausible wrong
 * number appearing, which is the invariant the rest of the core relies on. */
static canor_rational rneg(canor_rational a) { return canor_rat(-a.num, a.den); }
static canor_rational rsub(canor_rational a, canor_rational b) {
  return canor_rat_add(a, rneg(b));
}
static canor_rational rdiv(canor_rational a, canor_rational b) {
  /* canor_rat(b.den, 0) is the poison, so a zero divisor needs no special case. */
  return canor_rat_mul(a, canor_rat(b.den, b.num));
}

/* Quaternion index multiplication. The INDEX is XOR - 1.i=i is 0^1, i.j=k is
 * 1^2=3, j.k=i is 2^3=1 - so only the sign needs a table. Writing the index as
 * XOR rather than tabling it is not a micro-optimisation: it is what makes the
 * m-factor product a single word XOR instead of an m-step loop over indices. */
static const int QSGN[4][4] = {
  /*        1   i   j   k  */
  /* 1 */ { 1,  1,  1,  1 },
  /* i */ { 1, -1,  1, -1 },
  /* j */ { 1, -1, -1,  1 },
  /* k */ { 1,  1, -1, -1 }
};

uint64_t ft_dim(int m) {
  uint64_t d = 1;
  int i;
  if (m < 1 || m > FT_MAX_FACTORS) return 0;
  for (i = 0; i < m; ++i) {
    if (d > (uint64_t)-1 / 4) return 0;
    d *= 4;
  }
  return d;
}

/* Valid basis indices use exactly 2m bits. An index with bits set above that
 * names a factor the algebra does not have, so it is refused rather than
 * masked - masking would silently answer a different question. */
static bool ft_valid(int m, uint32_t a) {
  if (m < 1 || m > FT_MAX_FACTORS) return false;
  if (m >= 16) return true;
  return a < (1u << (2 * m));
}

bool ft_sign(int m, uint32_t a, uint32_t b, int *sign) {
  int s = 1, i;
  if (!sign) return false;
  if (!ft_valid(m, a) || !ft_valid(m, b)) return false;
  for (i = 0; i < m; ++i) {
    int qa = (int)((a >> (2 * i)) & 3u);
    int qb = (int)((b >> (2 * i)) & 3u);
    s *= QSGN[qa][qb];
  }
  *sign = s;
  return true;
}

bool ft_mul(int m, uint32_t a, uint32_t b, ft_term *out) {
  int s;
  if (!out) return false;
  if (!ft_sign(m, a, b, &s)) return false;
  out->coeff = s;
  out->unit = a ^ b;
  return true;
}

bool ft_bracket(int m, uint32_t a, uint32_t b, ft_term *out) {
  int sab, sba;
  if (!out) return false;
  if (!ft_sign(m, a, b, &sab)) return false;
  if (!ft_sign(m, b, a, &sba)) return false;
  /* Both products land on a XOR b, so the bracket is a single term and the
   * coefficient is the whole of the difference. */
  out->coeff = sab - sba;
  out->unit = a ^ b;
  return true;
}

bool ft_associator(int m, uint32_t a, uint32_t b, uint32_t c, ft_term *out) {
  int s1, s2, s3, s4;
  if (!out) return false;
  if (!ft_sign(m, a, b, &s1)) return false;
  if (!ft_sign(m, a ^ b, c, &s2)) return false;
  if (!ft_sign(m, b, c, &s3)) return false;
  if (!ft_sign(m, a, b ^ c, &s4)) return false;
  out->coeff = s1 * s2 - s3 * s4;
  out->unit = a ^ b ^ c;
  return true;
}

bool ft_jacobiator(int m, uint32_t a, uint32_t b, uint32_t c, ft_term *out) {
  ft_term ab, bc, ca;
  int s1, s2, s3;
  if (!out) return false;
  /* [[a,b],c] + [[b,c],a] + [[c,a],b]. Each inner bracket is a multiple of a
   * single basis element, so each outer bracket is too, and all three land on
   * a XOR b XOR c - which is why this sums to one term. */
  if (!ft_bracket(m, a, b, &ab)) return false;
  if (!ft_bracket(m, b, c, &bc)) return false;
  if (!ft_bracket(m, c, a, &ca)) return false;
  {
    ft_term t1, t2, t3;
    if (!ft_bracket(m, ab.unit, c, &t1)) return false;
    if (!ft_bracket(m, bc.unit, a, &t2)) return false;
    if (!ft_bracket(m, ca.unit, b, &t3)) return false;
    s1 = ab.coeff * t1.coeff;
    s2 = bc.coeff * t2.coeff;
    s3 = ca.coeff * t3.coeff;
  }
  out->coeff = s1 + s2 + s3;
  out->unit = a ^ b ^ c;
  return true;
}

bool ft_census_run(int m, long cap, ft_census *out) {
  uint64_t dim = ft_dim(m);
  long n, a, b, c;
  if (!out || dim == 0) return false;
  memset(out, 0, sizeof *out);
  out->dim = dim;
  n = (cap > 0 && (uint64_t)cap < dim) ? cap : (long)dim;
  out->sampled = ((uint64_t)n < dim);

  for (a = 0; a < n; ++a) {
    bool central = true;
    for (b = 0; b < n; ++b) {
      ft_term br;
      if (!ft_bracket(m, (uint32_t)a, (uint32_t)b, &br)) return false;
      if (a < b) {
        ++out->pairs;
        if (br.coeff == 0) ++out->commuting;
      }
      if (br.coeff != 0) central = false;
    }
    if (central) ++out->central;
  }

  /* Triples are cubic, so they are bounded separately: a full census at m=5
   * would be 1024^3. The cap is reported through `sampled` rather than left to
   * be inferred from the counts. */
  {
    long tn = n > 24 ? 24 : n;
    if ((uint64_t)tn < dim) out->sampled = true;
    for (a = 0; a < tn; ++a)
      for (b = 0; b < tn; ++b)
        for (c = 0; c < tn; ++c) {
          ft_term as, ja;
          if (!ft_associator(m, (uint32_t)a, (uint32_t)b, (uint32_t)c, &as)) return false;
          if (!ft_jacobiator(m, (uint32_t)a, (uint32_t)b, (uint32_t)c, &ja)) return false;
          ++out->triples;
          if (as.coeff != 0) ++out->assoc_fail;
          if (ja.coeff != 0) ++out->jacobi_fail;
        }
  }
  return true;
}

/* --- rational rotation ---------------------------------------------------- */

canor_rational ft_quat_norm2(const ft_quat *q) {
  canor_rational s;
  if (!q) return canor_rat(0, 0);
  s = canor_rat_add(canor_rat_mul(q->w, q->w), canor_rat_mul(q->x, q->x));
  s = canor_rat_add(s, canor_rat_mul(q->y, q->y));
  s = canor_rat_add(s, canor_rat_mul(q->z, q->z));
  return s;
}

/* Hamilton product of two pure-rational quaternions. */
static void qmul(const ft_quat *a, const ft_quat *b, ft_quat *o) {
  o->w = rsub(rsub(rsub(canor_rat_mul(a->w, b->w),
                                          canor_rat_mul(a->x, b->x)),
                               canor_rat_mul(a->y, b->y)),
                    canor_rat_mul(a->z, b->z));
  o->x = canor_rat_add(canor_rat_add(canor_rat_add(canor_rat_mul(a->w, b->x),
                                          canor_rat_mul(a->x, b->w)),
                               canor_rat_mul(a->y, b->z)),
                    rneg(canor_rat_mul(a->z, b->y)));
  o->y = canor_rat_add(canor_rat_add(canor_rat_add(canor_rat_mul(a->w, b->y),
                                          rneg(canor_rat_mul(a->x, b->z))),
                               canor_rat_mul(a->y, b->w)),
                    canor_rat_mul(a->z, b->x));
  o->z = canor_rat_add(canor_rat_add(canor_rat_add(canor_rat_mul(a->w, b->z),
                                          canor_rat_mul(a->x, b->y)),
                               rneg(canor_rat_mul(a->y, b->x))),
                    canor_rat_mul(a->z, b->w));
}

bool ft_rotate3(const ft_quat *q, const canor_rational v[3], canor_rational out[3]) {
  ft_quat vq, qc, t, r;
  canor_rational n2;
  int i;
  if (!q || !v || !out) return false;
  n2 = ft_quat_norm2(q);
  if (!canor_rat_valid(n2) || n2.num == 0) return false;

  vq.w = canor_rat(0, 1); vq.x = v[0]; vq.y = v[1]; vq.z = v[2];
  qc.w = q->w; qc.x = rneg(q->x);
  qc.y = rneg(q->y); qc.z = rneg(q->z);

  qmul(q, &vq, &t);
  qmul(&t, &qc, &r);

  out[0] = rdiv(r.x, n2);
  out[1] = rdiv(r.y, n2);
  out[2] = rdiv(r.z, n2);
  for (i = 0; i < 3; ++i) if (!canor_rat_valid(out[i])) return false;
  /* The scalar part of q v q~ is exactly zero for any q and any pure v. If it
   * is not, the arithmetic above is wrong - so it is checked rather than
   * discarded silently. */
  if (canor_rat_valid(r.w) && r.w.num != 0) return false;
  return true;
}

bool ft_convolve(const canor_rational *f, int nf, const canor_rational *g, int ng,
                 canor_rational *out, int cap) {
  int n, i, j;
  if (!f || !g || !out || nf < 1 || ng < 1) return false;
  n = nf + ng - 1;
  if (cap < n) return false;
  for (i = 0; i < n; ++i) out[i] = canor_rat(0, 1);
  for (i = 0; i < nf; ++i)
    for (j = 0; j < ng; ++j) {
      canor_rational p = canor_rat_mul(f[i], g[j]);
      if (!canor_rat_valid(p)) return false;
      out[i + j] = canor_rat_add(out[i + j], p);
      if (!canor_rat_valid(out[i + j])) return false;
    }
  return true;
}

void ft_voxel(const int64_t key[CANOR_KD], int level, int64_t out[CANOR_KD]) {
  int i;
  if (!key || !out) return;
  if (level < 0) level = 0;
  if (level > 62) level = 62;
  for (i = 0; i < CANOR_KD; ++i) {
    /* Arithmetic shift, so -1 >> 1 is -1 and not 0: a floor, consistently, in
     * both half-spaces. A division towards zero would map two adjacent
     * negative-side regions into the same bin and quietly merge them. */
    out[i] = key[i] >> level;
  }
}
