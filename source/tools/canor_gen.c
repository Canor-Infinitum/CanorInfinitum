/* canor_gen - writes the Flow / MetaFlow / Soul / Action / Maneuver sources.
 *
 *     canor_gen --out DIR [--sample K] [--author NAME] [--date YYYY-MM-DD]
 *
 * Layout follows the workspace rule ("each language keeps its sources in
 * .<project>/<language>/"): DIR/flow/flow.fwh, DIR/flow/flow.fwb, and the same
 * for metaflow, soul, action and maneuver. Point DIR at a .<project> workspace
 * or, by default, at the build tree (generated sources are not committed).
 *
 * Every line is one ATOM_DEFINITION of CanorLinguae/grammar/atoms.ebnf:
 *
 *     ATOM_PATH HEADER :: TEMPLATE PARAMETER_GROUP? (: ATOM_VALUE)? ;
 *
 * The grammar has no comment production, so provenance, policies and
 * diagnostics are atoms too (AGENTS.md invariant 8).
 *
 * WRITTEN IN FULL: the 7,776 Flows, 24 MetaFlows, 90 Strategies and 2,160
 * MetaFlow invocations. INDEX-ADDRESSED: Soul, Action and Maneuver, whose
 * .fwb holds the first and last K elements of each representation, a gap
 * record naming the unrank function for the rest, and the counts.
 *
 * DIGESTS are profile-defined and left unset here; the United digest is
 * specified elsewhere. Integer-only, like the core. */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define GEN_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define GEN_MKDIR(p) mkdir((p), 0777)
#endif

#include "canor_enum.h"

#define GEN_VERSION 2
#define GEN_PROFILE_VERSION 1
#define GEN_SETTINGS_VERSION 1
#define GEN_SAMPLE_DEFAULT 16
#define GEN_SAMPLE_MAX 1000
#define GEN_PATH_MAX 1024
#define GEN_LINE_MAX 4096
#define GEN_UNSET "unset"
#define GEN_IMAG_PINNED "[one, (i,j,k), (l,m,n), (o,p,q), (r,s,t), (e,f,g)]"
#define GEN_ASSIGNMENT_DEFAULT "unassigned"
#define GEN_AUX_ATOM 0                                   /* `one`: outside every trifecta */

typedef struct {
  FILE *fp;
  char path[GEN_PATH_MAX];
  int err;
} gen_file;

typedef struct {
  const char *dir;
  const char *author;
  const char *date;
  uint64_t sample;
} gen_ctx;

/* --- output ------------------------------------------------------------------ */

static int gen_open(gen_file *g, const gen_ctx *c, const char *lang, const char *ext) {
  char dir[GEN_PATH_MAX];
  int n = snprintf(dir, sizeof dir, "%s/%s", c->dir, lang);
  g->err = 0;
  if (n < 0 || (size_t)n >= sizeof dir) return -1;
  (void)GEN_MKDIR(dir);                       /* exists already: fopen decides */
  n = snprintf(g->path, sizeof g->path, "%s/%s.%s", dir, lang, ext);
  if (n < 0 || (size_t)n >= sizeof g->path) return -1;
  g->fp = fopen(g->path, "wb");
  if (!g->fp) fprintf(stderr, "canor_gen: cannot open %s\n", g->path);
  return g->fp ? 0 : -1;
}

static void gen_line(gen_file *g, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (vfprintf(g->fp, fmt, ap) < 0) g->err = 1;
  va_end(ap);
  if (fputs(";\n", g->fp) == EOF) g->err = 1;
}

static int gen_close(gen_file *g) {
  int bad = g->err;
  if (fclose(g->fp) != 0) bad = 1;
  if (bad) fprintf(stderr, "canor_gen: write failed: %s\n", g->path);
  else printf("wrote %s\n", g->path);
  return bad ? -1 : 0;
}

static void gen_provenance(gen_file *g, const gen_ctx *c, const char *spec) {
  gen_line(g,
           "Canor.provenance header :: Provenance{generator = \"canor_gen\", "
           "generator_version = %d, profile = %d, settings = %d, locale = \"en\", "
           "spec = \"%s\", author = \"%s\", date = \"%s\", imag = \"%s\", "
           "license = \"AGPL-3.0-or-later\"}",
           GEN_VERSION, GEN_PROFILE_VERSION, GEN_SETTINGS_VERSION, spec, c->author, c->date,
           GEN_IMAG_PINNED);
}

static void gen_digest(gen_file *g, const char *path) {
  gen_line(g, "%s.digest digest :: ProfileDefined : \"" GEN_UNSET "\"", path);
}

/* A small text builder, so a record can be assembled piece by piece. */
typedef struct {
  char s[GEN_LINE_MAX];
  size_t n;
  int full;
} gen_buf;

static void gen_put(gen_buf *b, const char *fmt, ...) {
  va_list ap;
  int k;
  if (b->full) return;
  va_start(ap, fmt);
  k = vsnprintf(b->s + b->n, sizeof b->s - b->n, fmt, ap);
  va_end(ap);
  if (k < 0 || (size_t)k >= sizeof b->s - b->n) b->full = 1;
  else b->n += (size_t)k;
}

/* Writes the samples of an index space of `n` elements: [0, K) and [n-K, n),
 * merged when they meet, then one gap record. */
typedef int (*gen_emit_fn)(gen_file *g, uint64_t idx, const void *arg);

static int gen_samples(gen_file *g, const gen_ctx *c, const char *path, uint64_t n,
                       const char *unrank, gen_emit_fn emit, const void *arg) {
  uint64_t k = c->sample;
  if (n <= 2 * k) {
    for (uint64_t i = 0; i < n; ++i)
      if (emit(g, i, arg)) return -1;
    gen_line(g, "%s.gap index :: Enumerated{count = %llu}", path, (unsigned long long)n);
    return 0;
  }
  for (uint64_t i = 0; i < k; ++i)
    if (emit(g, i, arg)) return -1;
  gen_line(g, "%s.gap index :: Unenumerated{from = %llu, to = %llu, count = %llu, unrank = \"%s\"}",
           path, (unsigned long long)k, (unsigned long long)(n - k - 1),
           (unsigned long long)(n - 2 * k), unrank);
  for (uint64_t i = n - k; i < n; ++i)
    if (emit(g, i, arg)) return -1;
  return 0;
}

/* --- shared renderings ------------------------------------------------------- */

static void gen_flow_group(gen_buf *b, const canor_soul_flow_t *f) {
  for (uint16_t p = 0; p < f->n; ++p) {
    if (f->at[p].ring) gen_put(b, "%s%s_%u", p ? ", " : "", canor_imag_name(f->at[p].atom),
                               (unsigned)f->at[p].ring);
    else gen_put(b, "%s%s", p ? ", " : "", canor_imag_name(f->at[p].atom));
  }
}

/* The representation's values, shaped by its kind. */
static void gen_rep(gen_buf *b, const canor_soul_rep_t *r, const char *value_key) {
  bool reals = canor_soul_kind_reals(r->kind), bases = canor_soul_kind_bases(r->kind);
  if (!reals) gen_put(b, "step = %u, ", (unsigned)r->step);
  if (!bases) gen_put(b, "base = %s, ", canor_imag_name((canor_imag)r->base));
  if (!reals && !bases) {
    gen_put(b, "%s = %u", value_key, (unsigned)r->value[0]);
  } else if (!reals) {                                   /* Real, Bases */
    gen_put(b, "bases = {");
    for (uint16_t i = 0; i < r->nbases; ++i)
      gen_put(b, "%s%s = %u", i ? ", " : "", canor_imag_name((canor_imag)(1 + i)),
              (unsigned)r->value[i]);
    gen_put(b, "}");
  } else if (!bases) {                                   /* Reals, Base */
    gen_put(b, "reals = {");
    for (uint16_t i = 0; i < r->nreals; ++i) gen_put(b, "%s%u", i ? ", " : "", (unsigned)r->value[i]);
    gen_put(b, "}");
  } else {                                               /* Reals, Bases */
    gen_put(b, "stream = {");
    for (uint16_t i = 0; i < r->nreals; ++i) {
      gen_put(b, "%s{", i ? ", " : "");
      for (uint16_t j = 0; j < r->nbases; ++j)
        gen_put(b, "%s%u", j ? ", " : "", (unsigned)r->value[i * r->nbases + j]);
      gen_put(b, "}");
    }
    gen_put(b, "}");
  }
}

/* --- Flow ------------------------------------------------------------------- */

static int gen_flow(const gen_ctx *c) {
  gen_file h, b;
  canor_enum_flow_t f;
  canor_soul_stream_t s;
  char sym[CANOR_ENUM_SYMBOL_MAX];

  if (gen_open(&h, c, "flow", "fwh")) return -1;
  gen_provenance(&h, c, "Canor WD v1 \xC2\xA7" "1 Flow");
  for (unsigned t = 0; t < CANOR_ENUM_TRIFECTAS; ++t) {
    unsigned u = 1 + t * CANOR_ENUM_TRIFECTA_SIZE;
    gen_line(&h, "Flow.trifecta.T%u trifecta :: Trifecta{%s, %s, %s} : {index = {%u, %u, %u}}", t,
             canor_imag_name((canor_imag)u), canor_imag_name((canor_imag)(u + 1)),
             canor_imag_name((canor_imag)(u + 2)), u, u + 1, u + 2);
  }
  gen_line(&h, "Flow.policy.occurrence policy :: Occurrence : \"every occurrence counts; repeated "
               "atoms are distinct positions; nothing is deduplicated\"");
  gen_line(&h, "Flow.policy.ring policy :: RingIndex{omitted = 0} : \"categorical: an index selects "
               "an indexed instance of the atom name and is carried with the occurrence\"");
  gen_line(&h, "Flow.policy.auxiliary policy :: Auxiliary{atoms = {%s}} : \"counted in the "
               "auxiliary channel; its interpretation is " GEN_UNSET "\"",
           canor_imag_name(GEN_AUX_ATOM));
  gen_line(&h,
           "Flow.space index :: MixedRadix{count = %llu, radix = {6, 6, 6, 6, 6}, "
           "digit = \"lexicographic Lehmer rank of each trifecta's order-6 permutohedron\", "
           "significance = \"trifecta T0 first\"}",
           (unsigned long long)CANOR_ENUM_FLOW_COUNT);
  if (gen_close(&h)) return -1;

  if (gen_open(&b, c, "flow", "fwb")) return -1;
  gen_provenance(&b, c, "Canor WD v1 \xC2\xA7" "1 Flow");
  for (uint64_t r = 0; r < CANOR_ENUM_FLOW_COUNT; ++r) {
    gen_buf line = {{0}, 0, 0};
    if (canor_enum_flow_unrank(r, &f) != CANOR_OK) return -1;
    if (canor_soul_stream(&f.flow, NULL, CANOR_SOUL_L2R, &s) != CANOR_OK) return -1;
    canor_enum_flow_format(&f.flow, sym, sizeof sym);
    gen_put(&line, "Flow.F%llu flow :: Flow{", (unsigned long long)r);
    gen_flow_group(&line, &f.flow);
    gen_put(&line, "} : {rank = %llu, perm = {%u, %u, %u, %u, %u}, symbol = \"%s\", initial = {",
            (unsigned long long)r, f.perm[0], f.perm[1], f.perm[2], f.perm[3], f.perm[4], sym);
    for (unsigned t = 0; t < CANOR_ENUM_TRIFECTAS; ++t)
      gen_put(&line, "%s{%u, %u, %u}", t ? ", " : "", (unsigned)s.v[0][t][0],
              (unsigned)s.v[0][t][1], (unsigned)s.v[0][t][2]);
    gen_put(&line, "}, auxiliary = %u}", (unsigned)s.aux[0]);
    if (line.full) return -1;
    gen_line(&b, "%s", line.s);
  }
  gen_digest(&b, "Flow");
  return gen_close(&b);
}

/* --- MetaFlow + Strategy ------------------------------------------------------ */

static void gen_strategy_line(gen_file *g, uint64_t r) {
  canor_enum_strategy_t s;
  canor_enum_ore_t o;
  char sym[CANOR_ENUM_SYMBOL_MAX], cs[32], ss[32], ts[48];
  canor_enum_strategy_unrank(r, &s);
  canor_enum_strategy_format(&s, sym, sizeof sym);
  const char *u = canor_imag_name(s.differential);
  if (canor_enum_strategy_ore(&s, &o) != CANOR_OK) {
    gen_line(g, "Strategy.S%llu strategy :: Strategy{%s} : {status = \"RANGE: Ore evaluation "
                "overflowed\"}",
             (unsigned long long)r, sym);
    return;
  }
  canor_enum_rational_format(o.c, cs, sizeof cs);
  canor_enum_rational_format(o.s, ss, sizeof ss);
  if (o.t_status == CANOR_OK) {
    char tq[32];
    canor_enum_rational_format(o.t, tq, sizeof tq);
    snprintf(ts, sizeof ts, "%s", tq);
  } else {
    snprintf(ts, sizeof ts, "UNDEFINED: C(%u) = 0", (unsigned)s.order);
  }
  gen_line(g,
           "Strategy.S%llu strategy :: Strategy{%s} : {rank = %llu, variable = %s, "
           "extension = \"K[x_%s][d_%s; sigma, delta]\", rule = \"d_%s * f = sigma(f) * d_%s + "
           "delta(f)\", power = \"d_%s**%u\", C = \"%s\", S = \"%s\", T = \"%s\"}",
           (unsigned long long)r, sym, (unsigned long long)r, u, u, u, u, u, u, (unsigned)s.order,
           cs, ss, ts);
}

static void gen_metaflow_group(const canor_enum_metaflow_t *m, char *buf, size_t cap) {
  snprintf(buf, cap, "%s, %s, %s, %s", canor_enum_metaflow_label(m->atom[0]),
           canor_enum_metaflow_label(m->atom[1]), canor_enum_metaflow_label(m->atom[2]),
           canor_enum_metaflow_label(m->atom[3]));
}

/* One slot of the complex-fractal string. */
static void gen_slot(gen_buf *b, const canor_enum_fractal_t *x, uint8_t slot) {
  const char *u = canor_imag_name(x->direction);
  switch (slot) {
    case 0:
      gen_put(b, "R = {support = %llu, reals = %llu}", (unsigned long long)x->support,
              (unsigned long long)x->reals);
      break;
    case 1:
      gen_put(b, "G = {total = %llu, gaps = {", (unsigned long long)x->gaps);
      for (unsigned j = 0; j < x->depth; ++j)
        gen_put(b, "%s{length = %llu, count = %llu}", j ? ", " : "",
                (unsigned long long)x->gap_len[j], (unsigned long long)x->gap_mult[j]);
      gen_put(b, "}}");
      break;
    case 2:
      gen_put(b, "B = {base = %u, imaginary = %s, dimension = \"log_%u(%u) + 2*pi*%s*n/ln(%u)\"}",
              (unsigned)x->base, u, (unsigned)x->base, (unsigned)(x->base - 1u), u,
              (unsigned)x->base);
      break;
    default:
      gen_put(b, "A = \"z -> z**2 + c, c in span{1, %s}\"", u);
      break;
  }
}

static int gen_metaflow(const gen_ctx *c) {
  gen_file h, b;
  canor_enum_metaflow_t m;
  char grp[CANOR_ENUM_SYMBOL_MAX], sym[CANOR_ENUM_SYMBOL_MAX];

  if (gen_open(&h, c, "metaflow", "fwh")) return -1;
  gen_provenance(&h, c, "Canor WD v1 \xC2\xA7" "2 MetaFlow");
  for (uint8_t a = 0; a < CANOR_ENUM_METAFLOW_ATOMS; ++a)
    gen_line(&h, "MetaFlow.label.%s label :: Atom : \"%s\"", canor_enum_metaflow_label(a),
             canor_enum_metaflow_label_name(a));
  gen_line(&h, "MetaFlow.space index :: Permutohedron{count = %llu, atoms = %d, order = \"lex\"}",
           (unsigned long long)CANOR_ENUM_METAFLOW_COUNT, CANOR_ENUM_METAFLOW_ATOMS);
  for (uint64_t r = 0; r < CANOR_ENUM_METAFLOW_COUNT; ++r) {
    canor_enum_metaflow_unrank(r, &m);
    gen_metaflow_group(&m, grp, sizeof grp);
    canor_enum_metaflow_format(&m, sym, sizeof sym);
    gen_line(&h, "MetaFlow.M%llu metaflow :: MetaFlow{%s} : {rank = %llu, symbol = \"%s\"}",
             (unsigned long long)r, grp, (unsigned long long)r, sym);
  }
  gen_line(&h,
           "MetaFlow.excitation template :: ComplexFractalString{R, G, B, A} : {R = \"what remains "
           "of [0, b**k] after k generations\", G = \"the removed lengths with multiplicities\", "
           "B = \"scaling base b and imaginary base e_u\", A = \"z -> z**2 + c, c in span{1, e_u}\", "
           "depth = \"Strategy order k\", direction = \"Strategy differential u\", "
           "slots = \"MetaFlow order\"}");
  gen_line(&h, "MetaFlow.excitation.base setting :: FractalBase{b = %d} : \"middle removal; b = "
               "trifecta arity\"",
           CANOR_ENUM_FRACTAL_BASE);
  gen_line(&h,
           "Strategy.space index :: MixedRadix{count = %llu, radix = {%d, %d}, rank = \"(u - 1) * "
           "%d + (k - 1)\"}",
           (unsigned long long)CANOR_ENUM_STRATEGY_COUNT, CANOR_ENUM_DIFFERENTIALS, CANOR_ENUM_ORDERS,
           CANOR_ENUM_ORDERS);
  gen_line(&h,
           "Strategy.ore setting :: OreProfile{tau = \"%s\", sigma = \"%s\", delta_amp = \"%s\", "
           "delta = 0} : \"untwisted shift algebra; period 6, so Orders 1..6 run one period\"",
           CANOR_ENUM_ORE_TAU, CANOR_ENUM_ORE_SIGMA, CANOR_ENUM_ORE_DELTA_AMP);
  for (uint64_t r = 0; r < CANOR_ENUM_STRATEGY_COUNT; ++r) gen_strategy_line(&h, r);
  if (gen_close(&h)) return -1;

  if (gen_open(&b, c, "metaflow", "fwb")) return -1;
  gen_provenance(&b, c, "Canor WD v1 \xC2\xA7" "2 MetaFlow");
  for (uint64_t r = 0; r < CANOR_ENUM_INVOCATION_COUNT; ++r) {
    canor_enum_fractal_t x;
    gen_buf line = {{0}, 0, 0};
    if (canor_enum_invocation_unrank(r, &x) != CANOR_OK) return -1;
    gen_put(&line, "MetaFlow.I%llu invocation :: Invocation{M%llu, S%llu} : {",
            (unsigned long long)r, (unsigned long long)(r / CANOR_ENUM_STRATEGY_COUNT),
            (unsigned long long)(r % CANOR_ENUM_STRATEGY_COUNT));
    for (unsigned k = 0; k < CANOR_ENUM_METAFLOW_ATOMS; ++k) {
      if (k) gen_put(&line, ", ");
      gen_slot(&line, &x, x.slots.atom[k]);
    }
    gen_put(&line, "}");
    if (line.full) return -1;
    gen_line(&b, "%s", line.s);
  }
  gen_digest(&b, "MetaFlow");
  return gen_close(&b);
}

/* --- representations (Soul, Maneuver) ------------------------------------------ */

static void gen_kind_header(gen_file *g, const char *space, canor_enum_space sp, const char *unrank) {
  for (int k = 0; k < CANOR_SOUL_KINDS; ++k)
    gen_line(g,
             "%s.kind.%s kind :: Representation{%s} : {per_element = %llu, count = %llu, "
             "unrank = \"%s\"}",
             space, canor_soul_kind_name((canor_soul_kind)k),
             canor_soul_kind_shape((canor_soul_kind)k),
             (unsigned long long)canor_soul_kind_count((canor_soul_kind)k, CANOR_ENUM_FLOW_ATOMS),
             (unsigned long long)canor_enum_rep_count(sp, (canor_soul_kind)k), unrank);
}

static int gen_soul_emit(gen_file *g, uint64_t idx, const void *arg) {
  canor_soul_kind k = *(const canor_soul_kind *)arg;
  canor_enum_soul_t s;
  canor_soul_rep_t rep;
  char st[CANOR_ENUM_SYMBOL_MAX], sym[CANOR_ENUM_SYMBOL_MAX];
  gen_buf line = {{0}, 0, 0};
  if (canor_enum_soul_rep_unrank(k, idx, &s, &rep) != CANOR_OK) return -1;
  canor_enum_strategy_format(&s.strategy, st, sizeof st);
  canor_enum_soul_format(&s, sym, sizeof sym);
  gen_put(&line, "Soul.%s.%c%llu soul :: Soul{%s, F%u, M%u} : {index = %llu, soul = %llu, ",
          canor_soul_kind_name(k), canor_soul_kind_name(k)[0], (unsigned long long)idx, st,
          (unsigned)s.flow_rank, (unsigned)s.metaflow_rank, (unsigned long long)idx,
          (unsigned long long)s.rank);
  gen_rep(&line, &rep, "real");
  gen_put(&line, ", symbol = \"%s\", category = \"%s\"}", sym, canor_soul_category_name(s.category));
  if (line.full) return -1;
  gen_line(g, "%s", line.s);
  return 0;
}

static int gen_soul(const gen_ctx *c) {
  gen_file h, b;
  char path[64], fn[64];

  if (gen_open(&h, c, "soul", "fwh")) return -1;
  gen_provenance(&h, c, "Canor WD v1 \xC2\xA7" "4 Soul");
  gen_line(&h, "Soul.factor.Strategy factor :: Space{count = %llu, source = \"metaflow/metaflow.fwh\"}",
           (unsigned long long)CANOR_ENUM_STRATEGY_COUNT);
  gen_line(&h, "Soul.factor.Flow factor :: Space{count = %llu, source = \"flow/flow.fwb\"}",
           (unsigned long long)CANOR_ENUM_FLOW_COUNT);
  gen_line(&h, "Soul.factor.MetaFlow factor :: Space{count = %llu, source = \"metaflow/metaflow.fwh\"}",
           (unsigned long long)CANOR_ENUM_METAFLOW_COUNT);
  gen_line(&h,
           "Soul.space index :: Product{count = %llu, radix = {%llu, %llu, %llu}, "
           "rank = \"(strategy * %llu + flow) * %llu + metaflow\"}",
           (unsigned long long)CANOR_ENUM_SOUL_COUNT, (unsigned long long)CANOR_ENUM_STRATEGY_COUNT,
           (unsigned long long)CANOR_ENUM_FLOW_COUNT, (unsigned long long)CANOR_ENUM_METAFLOW_COUNT,
           (unsigned long long)CANOR_ENUM_FLOW_COUNT, (unsigned long long)CANOR_ENUM_METAFLOW_COUNT);
  gen_line(&h, "Soul.traversal setting :: Traversal{direction = \"L->R\", order = \"identity\"} : "
               "\"count stream: n + 1 vectors per trifecta, each consumed position decrements its "
               "matching components\"");
  gen_line(&h, "Soul.grayscale setting :: Grayscale : \"" GEN_UNSET "\"");
  gen_line(&h, "Soul.form setting :: Form{realification, complexification} : \"" GEN_UNSET "\"");
  gen_line(&h, "Soul.category setting :: Category{Storage, Learning, Operation} : \"Storage\"");
  gen_kind_header(&h, "Soul", CANOR_ENUM_SOUL, "canor_enum_soul_rep_unrank");
  if (gen_close(&h)) return -1;

  if (gen_open(&b, c, "soul", "fwb")) return -1;
  gen_provenance(&b, c, "Canor WD v1 \xC2\xA7" "4 Soul");
  for (int k = 0; k < CANOR_SOUL_KINDS; ++k) {
    canor_soul_kind kind = (canor_soul_kind)k;
    snprintf(path, sizeof path, "Soul.%s", canor_soul_kind_name(kind));
    snprintf(fn, sizeof fn, "canor_enum_soul_rep_unrank(%s)", canor_soul_kind_name(kind));
    if (gen_samples(&b, c, path, canor_enum_rep_count(CANOR_ENUM_SOUL, kind), fn, gen_soul_emit,
                    &kind))
      return -1;
  }
  gen_digest(&b, "Soul");
  return gen_close(&b);
}

/* --- Action ------------------------------------------------------------------- */

static int gen_action_emit(gen_file *g, uint64_t idx, const void *arg) {
  canor_enum_action_t a;
  char sym[CANOR_ENUM_SYMBOL_MAX];
  (void)arg;
  if (canor_enum_action_unrank(idx, &a) != CANOR_OK) return -1;
  canor_enum_action_format(&a, sym, sizeof sym);
  if (a.status == CANOR_OK)
    gen_line(g, "Action.A%llu action :: Congruence{m = %u, n = %u, o = %u} : {rank = %llu, "
                "symbol = \"%s\", residues = {%u, %u}, holds = %d}",
             (unsigned long long)idx, (unsigned)a.m, (unsigned)a.n, (unsigned)a.o,
             (unsigned long long)idx, sym, (unsigned)a.rm, (unsigned)a.rn, a.holds);
  else
    gen_line(g, "Action.A%llu action :: Congruence{m = %u, n = %u, o = %u} : {rank = %llu, "
                "symbol = \"%s\", residues = {}, holds = 0, status = \"UNDEFINED: modulus 0 has no "
                "canonical residue\"}",
             (unsigned long long)idx, (unsigned)a.m, (unsigned)a.n, (unsigned)a.o,
             (unsigned long long)idx, sym);
  return 0;
}

static int gen_action(const gen_ctx *c) {
  gen_file h, b;
  uint64_t holds = 0, undefined = 0;
  canor_enum_action_t a;

  if (gen_open(&h, c, "action", "fwh")) return -1;
  gen_provenance(&h, c, "Canor WD v1 \xC2\xA7" "3 Action");
  gen_line(&h, "Action.equation template :: Congruence{m, n, o} : \"m \xE2\x89\xA3 n (mod o)\"");
  gen_line(&h, "Action.range.m range :: Interval{lo = 0, hi = %llu}",
           (unsigned long long)(CANOR_ENUM_ACTION_BOUND - 1));
  gen_line(&h, "Action.range.n range :: Interval{lo = 0, hi = %llu}",
           (unsigned long long)(CANOR_ENUM_ACTION_BOUND - 1));
  gen_line(&h, "Action.range.o range :: Interval{lo = 0, hi = %llu}",
           (unsigned long long)(CANOR_ENUM_ACTION_BOUND - 1));
  gen_line(&h,
           "Action.space index :: MixedRadix{count = %llu, radix = {%llu, %llu, %llu}, "
           "rank = \"(m * %llu + n) * %llu + o\", unrank = \"canor_enum_action_unrank\"}",
           (unsigned long long)CANOR_ENUM_ACTION_COUNT, (unsigned long long)CANOR_ENUM_ACTION_BOUND,
           (unsigned long long)CANOR_ENUM_ACTION_BOUND, (unsigned long long)CANOR_ENUM_ACTION_BOUND,
           (unsigned long long)CANOR_ENUM_ACTION_BOUND, (unsigned long long)CANOR_ENUM_ACTION_BOUND);
  gen_line(&h,
           "Action.undefined diagnostic :: Undefined{count = %llu, where = \"o = 0\"} : \"no "
           "canonical residue 0 <= r < 0; kept, never dropped\"",
           (unsigned long long)CANOR_ENUM_ACTION_UNDEFINED_COUNT);
  if (gen_close(&h)) return -1;

  if (gen_open(&b, c, "action", "fwb")) return -1;
  gen_provenance(&b, c, "Canor WD v1 \xC2\xA7" "3 Action");
  if (gen_samples(&b, c, "Action", CANOR_ENUM_ACTION_COUNT, "canor_enum_action_unrank",
                  gen_action_emit, NULL))
    return -1;
  for (uint64_t r = 0; r < CANOR_ENUM_ACTION_COUNT; ++r) {
    canor_enum_action_unrank(r, &a);
    holds += a.holds;
    undefined += a.status != CANOR_OK;
  }
  gen_line(&b, "Action.census census :: Census{holds = %llu, fails = %llu, undefined = %llu}",
           (unsigned long long)holds,
           (unsigned long long)(CANOR_ENUM_ACTION_COUNT - holds - undefined),
           (unsigned long long)undefined);
  gen_digest(&b, "Action");
  return gen_close(&b);
}

/* --- Maneuver ----------------------------------------------------------------- */

static int gen_maneuver_emit(gen_file *g, uint64_t idx, const void *arg) {
  canor_soul_kind k = *(const canor_soul_kind *)arg;
  canor_enum_maneuver_t m;
  canor_soul_rep_t rep;
  char as[CANOR_ENUM_SYMBOL_MAX];
  gen_buf line = {{0}, 0, 0};
  canor_status st = canor_enum_maneuver_rep_unrank(k, idx, &m, &rep);
  if (st != CANOR_OK && st != CANOR_ERR_UNDEFINED) return -1;
  canor_enum_action_format(&m.action, as, sizeof as);
  gen_put(&line, "Maneuver.%s.%c%llu maneuver :: Maneuver{Soul.Unknown.U%llu, Action.A%llu} : {index = %llu, "
                 "maneuver = %llu, action = \"%s\", ",
          canor_soul_kind_name(k), canor_soul_kind_name(k)[0], (unsigned long long)idx,
          (unsigned long long)m.soul.rank, (unsigned long long)m.action.rank,
          (unsigned long long)idx, (unsigned long long)m.rank, as);
  if (st == CANOR_OK) {
    gen_rep(&line, &rep, "residue");
    gen_put(&line, ", holds = %d", m.action.holds);
  } else {
    gen_put(&line, "status = \"UNDEFINED: modulus 0 has no canonical residue\", holds = 0");
  }
  gen_put(&line, ", junction = \"one-to-one\", assignment = \"" GEN_ASSIGNMENT_DEFAULT "\"}");
  if (line.full) return -1;
  gen_line(g, "%s", line.s);
  return 0;
}

static int gen_maneuver(const gen_ctx *c) {
  gen_file h, b;
  char path[64], fn[64];

  if (gen_open(&h, c, "maneuver", "fwh")) return -1;
  gen_provenance(&h, c, "Canor WD v1 \xC2\xA7" "5 Maneuver");
  gen_line(&h, "Maneuver.factor.Soul factor :: Space{count = %llu, source = \"soul/soul.fwh\"}",
           (unsigned long long)CANOR_ENUM_SOUL_COUNT);
  gen_line(&h, "Maneuver.factor.Action factor :: Space{count = %llu, source = \"action/action.fwh\"}",
           (unsigned long long)CANOR_ENUM_ACTION_COUNT);
  gen_line(&h,
           "Maneuver.space index :: Product{count = %llu, radix = {%llu, %llu}, "
           "rank = \"soul * %llu + action\"}",
           (unsigned long long)CANOR_ENUM_MANEUVER_COUNT, (unsigned long long)CANOR_ENUM_SOUL_COUNT,
           (unsigned long long)CANOR_ENUM_ACTION_COUNT, (unsigned long long)CANOR_ENUM_ACTION_COUNT);
  gen_line(&h, "Maneuver.binding rule :: Binding{junction = \"one-to-one\", input = \"Soul count "
               "stream\", output = \"canonical residues mod o\"} : \"the Soul Symbol bound to m "
               "\xE2\x89\xA3 n (mod o)\"");
  gen_line(&h, "Maneuver.effect policy :: Effect{canordb = none, runtime = none} : \"pure\"");
  gen_line(&h, "Maneuver.assignment category :: Assignment{assigned, tasked, scheduled, unassigned} "
               ": \"" GEN_ASSIGNMENT_DEFAULT "\"");
  gen_kind_header(&h, "Maneuver", CANOR_ENUM_MANEUVER, "canor_enum_maneuver_rep_unrank");
  if (gen_close(&h)) return -1;

  if (gen_open(&b, c, "maneuver", "fwb")) return -1;
  gen_provenance(&b, c, "Canor WD v1 \xC2\xA7" "5 Maneuver");
  for (int k = 0; k < CANOR_SOUL_KINDS; ++k) {
    canor_soul_kind kind = (canor_soul_kind)k;
    snprintf(path, sizeof path, "Maneuver.%s", canor_soul_kind_name(kind));
    snprintf(fn, sizeof fn, "canor_enum_maneuver_rep_unrank(%s)", canor_soul_kind_name(kind));
    if (gen_samples(&b, c, path, canor_enum_rep_count(CANOR_ENUM_MANEUVER, kind), fn,
                    gen_maneuver_emit, &kind))
      return -1;
  }
  gen_digest(&b, "Maneuver");
  return gen_close(&b);
}

/* --- main --------------------------------------------------------------------- */

static int gen_usage(void) {
  fprintf(stderr, "usage: canor_gen --out DIR [--sample K (0..%d, default %d)] [--author NAME] "
                  "[--date YYYY-MM-DD]\n",
          GEN_SAMPLE_MAX, GEN_SAMPLE_DEFAULT);
  return 2;
}

int main(int argc, char **argv) {
  gen_ctx c;
  memset(&c, 0, sizeof c);
  c.sample = GEN_SAMPLE_DEFAULT;
  c.author = GEN_UNSET;
  c.date = GEN_UNSET;     /* reproducible by default: no clock is read */
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--out") && i + 1 < argc) {
      c.dir = argv[++i];
    } else if (!strcmp(argv[i], "--author") && i + 1 < argc) {
      c.author = argv[++i];
    } else if (!strcmp(argv[i], "--date") && i + 1 < argc) {
      c.date = argv[++i];
    } else if (!strcmp(argv[i], "--sample") && i + 1 < argc) {
      unsigned long long k;
      char extra;
      if (sscanf(argv[++i], "%llu%c", &k, &extra) != 1 || k > GEN_SAMPLE_MAX) return gen_usage();
      c.sample = k;
    } else {
      return gen_usage();
    }
  }
  if (!c.dir) return gen_usage();
  if (canor_imag_init() != CANOR_OK) {
    fprintf(stderr, "canor_gen: IMAG profile does not close\n");
    return 1;
  }
  if (gen_flow(&c) || gen_metaflow(&c) || gen_soul(&c) || gen_action(&c) || gen_maneuver(&c))
    return 1;
  return 0;
}
