/* Pluggable Strategies and baseline Flows. See canor_strategy.h.
 *
 * No floating point. The abstract operators are integer maps, which is what
 * FLOWALGEBRA section 13 asks for by declining trigonometric identities.
 */

#include "canor_strategy.h"
#include "canor_core.h"

#include <stdio.h>
#include <string.h>

/* --- FLOWVALUE arithmetic --------------------------------------------------- */

const char *canor_fv_name(FLOWVALUE v) {
  switch (v) {
    case FV_NegZero: return "NegZero";
    case FV_NegOne:  return "NegOne";
    case FV_PosZero: return "PosZero";
    case FV_PosOne:  return "PosOne";
  }
  return "?";
}

FLOWVALUE canor_fv_negate(FLOWVALUE v) {
  /* The pole flips with the sign. Mapping NegZero to itself would make
   * negation lose the representational distinction section 10 requires be
   * kept, so a latent positive null negates to a latent negative one. */
  switch (v) {
    case FV_PosOne:  return FV_NegOne;
    case FV_NegOne:  return FV_PosOne;
    case FV_PosZero: return FV_NegZero;
    case FV_NegZero: return FV_PosZero;
  }
  return v;
}

static int fv_num(FLOWVALUE v) { return canor_flowvalue_numeric(v); }
static int fv_pole(FLOWVALUE v) { return (v == FV_PosOne || v == FV_PosZero) ? 1 : -1; }

FLOWVALUE canor_fv_mul(FLOWVALUE a, FLOWVALUE b) {
  int n = fv_num(a) * fv_num(b);
  int pole = fv_pole(a) * fv_pole(b);
  if (n > 0) return FV_PosOne;
  if (n < 0) return FV_NegOne;
  /* Zero result. The POLE still carries information - it is the product of the
   * operands' poles - so multiplying by a null does not erase which side the
   * null was on. That is the one place this algebra is more informative than
   * the integers, and discarding it would make PosZero and NegZero
   * interchangeable, which section 10 forbids. */
  return pole > 0 ? FV_PosZero : FV_NegZero;
}

bool canor_fv_div(FLOWVALUE a, FLOWVALUE b, FLOWVALUE *out) {
  if (!out) return false;
  if (fv_num(b) == 0) return false;   /* undefined; reported, never invented */
  int n = fv_num(a) / fv_num(b);
  int pole = fv_pole(a) * fv_pole(b);
  if (n > 0) *out = FV_PosOne;
  else if (n < 0) *out = FV_NegOne;
  else *out = pole > 0 ? FV_PosZero : FV_NegZero;
  return true;
}

/* --- abstract operators ----------------------------------------------------
 * Strategy-indexed integer maps. NOT trigonometry: section 13 says the
 * operators "are not required to satisfy conventional trigonometric
 * identities", and building them out of sin() would import exactly the
 * identities it declines, plus floating point that section 27 forbids.
 *
 * What IS preserved is the shape that makes the names apt: each operator is a
 * different deterministic map over the same argument, so the three disagree,
 * and each is periodic in the argument under the Strategy's stream. */
static FLOWVALUE default_absop(const STRATEGY *s, canor_absop op, int32_t x) {
  uint64_t r = canor_rand(s, 0xD1FFull + (uint64_t)op, (uint64_t)(uint32_t)x);
  /* Two bits, mapped onto the specified encoding: 00 NegZero, 01 NegOne,
   * 10 PosZero, 11 PosOne. Taken from the high end of the word, where the
   * finalizer has mixed hardest. */
  return (FLOWVALUE)((r >> 61) & 3u);
}

static FLOWVALUE apply_op(const canor_strategy *s, canor_absop op, int32_t x) {
  if (s->ops) return s->ops(&s->id, op, x);
  return default_absop(&s->id, op, x);
}

/* --- families --------------------------------------------------------------- */

bool canor_family_eval(const canor_strategy *s, const canor_family *f,
                    const int32_t *args, int argc, FLOWVALUE *out) {
  if (!s || !f || !args || !out) return false;
  /* Fewer arguments than the family reads is an ERROR, not a zero-fill: a
   * silently defaulted argument produces a plausible FLOWVALUE that nothing
   * asked for. */
  if (argc < (int)f->arity) return false;

  FLOWVALUE acc = FV_PosZero;
  bool first = true;
  uint64_t nadd = 0;              /* additions folded in so far */

  for (int t = 0; t < f->nterms; ++t) {
    const canor_product *p = &f->term[t];
    if (p->nf == 0) continue;

    /* A term is a product chain, evaluated left to right. Precedence lives in
     * the structure, so this loop never has to know about it. */
    FLOWVALUE prod = apply_op(s, p->f[0].op, args[p->f[0].arg]);
    for (int k = 1; k < p->nf; ++k) {
      FLOWVALUE rhs = apply_op(s, p->f[k].op, args[p->f[k].arg]);
      if (p->f[k].join == CANOR_JOIN_MUL) {
        prod = canor_fv_mul(prod, rhs);
      } else {
        FLOWVALUE q;
        if (!canor_fv_div(prod, rhs, &q)) return false;   /* division by a null */
        prod = q;
      }
    }
    if (p->sign < 0) prod = canor_fv_negate(prod);

    if (first) { acc = prod; first = false; }
    else {
      /* The context is the term index and the position in the fold. Both are
       * passed because they are not the same thing: a family may skip empty
       * terms, so term index t and addition count nadd diverge, and a rule
       * that saw only one of them would treat two structurally different
       * cancellations as identical. */
      acc = canor_flowvalue_add_ctx(s->bias, &s->id, acc, prod,
                                 ((uint64_t)t << 8) | nadd);
      ++nadd;
    }
  }
  *out = acc;
  return true;
}

bool canor_di(const canor_strategy *s, const int32_t *a, int n, FLOWVALUE *o) {
  return canor_family_eval(s, &s->d[0][0], a, n, o);
}
bool canor_dj(const canor_strategy *s, const int32_t *a, int n, FLOWVALUE *o) {
  return canor_family_eval(s, &s->d[0][1], a, n, o);
}
bool canor_dk(const canor_strategy *s, const int32_t *a, int n, FLOWVALUE *o) {
  return canor_family_eval(s, &s->d[0][2], a, n, o);
}

bool canor_dtc(const canor_strategy *s, int t, int c,
            const int32_t *a, int n, FLOWVALUE *o) {
  if (!s || t < 0 || c < 0 || c > 2) return false;
  if (t >= (s->ntrifecta > 0 ? s->ntrifecta : 1)) return false;
  return canor_family_eval(s, &s->d[t][c], a, n, o);
}

bool canor_flow_multiflow(const canor_strategy *s, const canor_flow_registry *r, int idx,
                       FLOWVALUE out[CANOR_TRIFECTAS][3], uint32_t *defined) {
  int32_t args[CANOR_FAMILY_ARGS_MAX];
  int t, c, i, n;
  uint32_t mask = 0;
  if (!s || !r || idx < 0 || idx >= r->count || !out) return false;
  /* Same argument construction as canor_flow_differential: the Flow's own units
   * supply the arguments, so the word drives the families rather than the
   * caller inventing numbers. */
  n = r->flow[idx].len;
  if (n > CANOR_FAMILY_ARGS_MAX) n = CANOR_FAMILY_ARGS_MAX;
  for (i = 0; i < n; ++i) args[i] = (int32_t)r->flow[idx].unit[i];
  for (; i < CANOR_FAMILY_ARGS_MAX; ++i) args[i] = 0;
  for (t = 0; t < CANOR_TRIFECTAS; ++t)
    for (c = 0; c < 3; ++c) {
      out[t][c] = FV_PosZero;
      if (canor_dtc(s, t, c, args, CANOR_FAMILY_ARGS_MAX, &out[t][c]))
        mask |= 1u << (t * 3 + c);
    }
  if (defined) *defined = mask;
  return true;
}

/* --- family text form -------------------------------------------------------
 * "-sin1 +tan2 +sin3 -sin4" / "+cos1*sin2 -cos3*tan4"
 * Terms are separated by whitespace and each carries its own leading sign;
 * factors inside a term are joined by * or / with no spaces. Refuses anything
 * else rather than guessing. */

static bool read_op(const char **p, canor_absop *op) {
  if (!strncmp(*p, "sin", 3)) { *op = CANOR_ABS_SIN; *p += 3; return true; }
  if (!strncmp(*p, "cos", 3)) { *op = CANOR_ABS_COS; *p += 3; return true; }
  if (!strncmp(*p, "tan", 3)) { *op = CANOR_ABS_TAN; *p += 3; return true; }
  return false;
}

bool canor_family_parse(canor_family *f, const char *name, const char *text) {
  if (!f || !text) return false;
  memset(f, 0, sizeof *f);
  snprintf(f->name, sizeof f->name, "%s", name ? name : "");

  const char *p = text;
  while (*p) {
    while (*p == ' ' || *p == '\t') ++p;
    if (!*p) break;
    if (f->nterms >= CANOR_FAMILY_TERMS_MAX) return false;
    canor_product *term = &f->term[f->nterms];
    memset(term, 0, sizeof *term);

    term->sign = 1;
    if (*p == '+') { term->sign = 1; ++p; }
    else if (*p == '-') { term->sign = -1; ++p; }
    else return false;                 /* every term states its sign */

    for (;;) {
      if (term->nf >= CANOR_FAMILY_FACTORS_MAX) return false;
      canor_factor *fa = &term->f[term->nf];
      fa->join = CANOR_JOIN_MUL;          /* meaningless for the first factor */
      if (term->nf > 0) {
        if (*p == '*') fa->join = CANOR_JOIN_MUL;
        else if (*p == '/') fa->join = CANOR_JOIN_DIV;
        else break;
        ++p;
      }
      if (!read_op(&p, &fa->op)) return false;
      if (*p < '1' || *p > '9') return false;   /* arguments are 1-based */
      uint8_t idx = (uint8_t)(*p - '1');
      ++p;
      if (idx >= CANOR_FAMILY_ARGS_MAX) return false;
      fa->arg = idx;
      if (idx + 1 > f->arity) f->arity = (uint8_t)(idx + 1);
      ++term->nf;
      if (*p != '*' && *p != '/') break;
    }
    ++f->nterms;
  }
  return f->nterms > 0;
}

int canor_family_format(const canor_family *f, char *out, int cap) {
  static const char *const opn[3] = { "sin", "cos", "tan" };
  int n = 0;
  for (int t = 0; t < f->nterms; ++t) {
    const canor_product *p = &f->term[t];
    n += snprintf(out + n, (size_t)(cap - n), "%s%c", t ? " " : "",
                  p->sign < 0 ? '-' : '+');
    for (int k = 0; k < p->nf; ++k) {
      if (k) n += snprintf(out + n, (size_t)(cap - n), "%c",
                           p->f[k].join == CANOR_JOIN_MUL ? '*' : '/');
      n += snprintf(out + n, (size_t)(cap - n), "%s%d", opn[p->f[k].op], p->f[k].arg + 1);
    }
  }
  return n;
}

void canor_strategy_default(canor_strategy *s, const char *name, uint32_t version) {
  if (!s) return;
  memset(s, 0, sizeof *s);
  s->id.name = name;
  s->id.version = version;
  s->bias = LeftBiased;
  s->ops = NULL;                       /* the default abstract operators */
  /* The brief's example, verbatim, in trifecta 0. */
  canor_family_parse(&s->d[0][0], "di", "-sin1 +tan2 +sin3 -sin4");
  canor_family_parse(&s->d[0][1], "dj", "+cos1*sin2 -cos3*tan4");
  canor_family_parse(&s->d[0][2], "dk", "+cos1 -sin2*tan3/sin4");
  /* The remaining trifectas start as copies. A Strategy that declares five
   * without specifying them gets five that behave alike, which is honest -
   * they are declared, just not differentiated - whereas leaving them empty
   * would evaluate to zeros that look like measurements. */
  {
    int t, c;
    for (t = 1; t < CANOR_TRIFECTAS; ++t)
      for (c = 0; c < 3; ++c) s->d[t][c] = s->d[0][c];
  }
  s->ntrifecta = 1;
}

/* --- baseline Flows ---------------------------------------------------------- */

const char *canor_unit_name(uint8_t unit) { return canor_imag_name(unit); }

static int unit_of_char(char c) {
  switch (c) {
    case '1': return 0;
    case 'i': case 'I': return 1;
    case 'j': case 'J': return 2;
    case 'k': case 'K': return 3;
    default: return -1;
  }
}

bool canor_flows_add(canor_flow_registry *r, const char *name, const char *word) {
  if (!r || !name || !word) return false;
  if (r->count >= CANOR_REGISTRY_MAX) return false;
  size_t len = strlen(word);
  if (len == 0 || len >= CANOR_FLOW_WORD_MAX) return false;
  canor_baseline *b = &r->flow[r->count];
  memset(b, 0, sizeof *b);
  snprintf(b->name, sizeof b->name, "%s", name);
  for (size_t i = 0; i < len; ++i) {
    int u = unit_of_char(word[i]);
    if (u < 0) return false;           /* refuse a word with a foreign symbol */
    b->unit[i] = (uint8_t)u;
  }
  memcpy(b->word, word, len + 1);
  b->len = (int)len;
  ++r->count;
  return true;
}

void canor_flows_default(canor_flow_registry *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
  canor_flows_add(r, "Ordinary", "ijkkji");
  canor_flows_add(r, "Peaceful", "iiikji");
  canor_flows_add(r, "Formful",  "jikkkj");
  canor_flows_add(r, "Stormful", "kkjikk");
}

int canor_flows_find(const canor_flow_registry *r, const char *name) {
  if (!r || !name) return -1;
  for (int i = 0; i < r->count; ++i) if (!strcmp(r->flow[i].name, name)) return i;
  return -1;
}

bool canor_flow_fold(const canor_flow_registry *r, int idx, uint8_t *unit, int *sign) {
  if (!r || idx < 0 || idx >= r->count || !unit || !sign) return false;
  const canor_baseline *b = &r->flow[idx];
  canor_imag acc = b->unit[0];
  int s = 1;
  /* Left to right, and the order is the content: the quaternion product is
   * non-commutative, so ijkkji and its reverse are different Flows. */
  for (int i = 1; i < b->len; ++i) {
    canor_imag res;
    int rs;
    if (canor_imag_mul(acc, b->unit[i], &res, &rs) != CANOR_OK) return false;
    acc = res;
    s *= rs;
  }
  *unit = acc;
  *sign = s;
  return true;
}

bool canor_flow_differential(const canor_strategy *s, const canor_flow_registry *r,
                          int idx, FLOWVALUE *di, FLOWVALUE *dj, FLOWVALUE *dk) {
  if (!s || !r || idx < 0 || idx >= r->count) return false;
  const canor_baseline *b = &r->flow[idx];
  /* The Flow supplies the arguments: each unit in the word is one integer, so
   * the differential family is evaluated ON the Flow rather than on numbers a
   * caller had to invent. A word shorter than the family's arity is padded
   * with its own units, cyclically, so a 6-symbol Flow can drive a 4-argument
   * family without the caller padding by hand - and cyclically rather than
   * with zeros, because zero is a FLOWVALUE pole here and would bias the
   * result toward one side. */
  int32_t args[CANOR_FAMILY_ARGS_MAX];
  for (int i = 0; i < CANOR_FAMILY_ARGS_MAX; ++i)
    args[i] = (int32_t)b->unit[i % b->len] + (int32_t)(i / b->len);
  if (di && !canor_di(s, args, CANOR_FAMILY_ARGS_MAX, di)) return false;
  if (dj && !canor_dj(s, args, CANOR_FAMILY_ARGS_MAX, dj)) return false;
  if (dk && !canor_dk(s, args, CANOR_FAMILY_ARGS_MAX, dk)) return false;
  return true;
}
