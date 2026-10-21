/* Elite(TM) Programming & Assembler, and the multilateral Diffeomorphic Basis.
 *
 * Design and rationale: canor_elite.h. Canor WD v1 §7, §6; CanorLinguae
 * Z-Notation Draft §7 (UnitedApproval, BilateralUnitedValidity), §8, §12.
 *
 * Everything here is pure: const inputs in, caller-placed outputs out. The
 * only calls that leave this file are the profile's digest and verify
 * functions, which the profile contract requires to be pure as well.
 */

#include "canor_elite.h"

#include <string.h>

#include "canor_flow.h" /* CANOR_ONE */

/* Versioned domain-separation tags. A signature over one statement kind can
 * never verify as another kind, or as an older layout: bump the suffix when a
 * layout changes. */
static const char canor_elite_tag_basis[] = "canor.elite.basis.v1";
static const char canor_elite_tag_ratify[] = "canor.elite.ratify.v1";
static const char canor_elite_tag_cert[] = "canor.elite.cert.v1";
static const char canor_elite_tag_approve[] = "canor.elite.approve.v1";
static const char canor_elite_tag_vectors[] = "canor.elite.vectors.v1";

/* Tags are encoded as names, so the size macros in the header stay exact. */
_Static_assert(sizeof canor_elite_tag_basis <= CANOR_ELITE_NAME_MAX, "tag");
_Static_assert(sizeof canor_elite_tag_ratify <= CANOR_ELITE_NAME_MAX, "tag");
_Static_assert(sizeof canor_elite_tag_cert <= CANOR_ELITE_NAME_MAX, "tag");
_Static_assert(sizeof canor_elite_tag_approve <= CANOR_ELITE_NAME_MAX, "tag");
_Static_assert(sizeof canor_elite_tag_vectors <= CANOR_ELITE_NAME_MAX, "tag");
/* Indices are stored in uint8_t with 0xFF reserved for "none". */
_Static_assert(CANOR_ELITE_SIGNATORY_MAX < 0xFF && CANOR_ELITE_CATEGORY_MAX < 0xFF &&
                   CANOR_ELITE_VOCAB_MAX < 0xFF && CANOR_ELITE_INSN_MAX < 0xFF,
               "index capacities must leave 0xFF free");
_Static_assert(CANOR_ELITE_DIAG_MAX <= UINT16_MAX, "report counters are u16");
_Static_assert(CANOR_ELITE_EXPANSION_MAX >= 1 && CANOR_ELITE_EXPANSION_MAX <= UINT8_MAX &&
                   CANOR_ELITE_DEPTH_MAX <= UINT8_MAX && CANOR_ELITE_ALT_MAX <= UINT8_MAX,
               "leaf indices and rewrite paths are u8");
_Static_assert(CANOR_ELITE_TEXT_MAX >= 2, "a text holds at least one byte");
_Static_assert(CANOR_ELITE_BRANCH_MAX <= UINT16_MAX, "branch_count is u16");

/* One scratch buffer serves every canonical encoding the evaluator needs. */
#define CANOR_ELITE_MAX2(a, b) ((a) > (b) ? (a) : (b))
#define CANOR_ELITE_SCRATCH_MAX                                                \
  CANOR_ELITE_MAX2(CANOR_ELITE_STATEMENT_MAX,                                  \
                   CANOR_ELITE_MAX2(CANOR_ELITE_BASIS_ENCODED_MAX,             \
                                    CANOR_ELITE_VECTORS_ENCODED_MAX))

/* splitmix64 constants (Steele, Lea, Flood 2014): the built-in stochastic
 * tool's mixing function. Integer-only and identical on every host. */
#define CANOR_ELITE_SPLITMIX_GAMMA UINT64_C(0x9E3779B97F4A7C15)
#define CANOR_ELITE_SPLITMIX_MUL1 UINT64_C(0xBF58476D1CE4E5B9)
#define CANOR_ELITE_SPLITMIX_MUL2 UINT64_C(0x94D049BB133111EB)

/* ===========================================================================
 * Names and blobs
 * ========================================================================= */

/* Length of a name held in a fixed array; CANOR_ELITE_NAME_MAX means the array
 * holds no terminator (a hand-filled struct), which callers treat as invalid. */
static size_t canor_elite_strlen(const char *s) {
  size_t n = 0;
  while (n < CANOR_ELITE_NAME_MAX && s[n]) ++n;
  return n;
}

static bool canor_elite_name_ok(const char *s) {
  size_t n = canor_elite_strlen(s);
  return n > 0 && n < CANOR_ELITE_NAME_MAX;
}

static bool canor_elite_name_eq(const char *a, const char *b) {
  return strncmp(a, b, CANOR_ELITE_NAME_MAX) == 0;
}

/* Required name: NULL or "" is CANOR_ERR_ARGS, too long is CANOR_ERR_RANGE. */
static canor_status canor_elite_name_copy(char *dst, const char *src) {
  size_t n;
  if (!src || !src[0]) return CANOR_ERR_ARGS;
  n = strlen(src);
  if (n >= CANOR_ELITE_NAME_MAX) return CANOR_ERR_RANGE;
  memcpy(dst, src, n + 1);
  return CANOR_OK;
}

/* Optional name: NULL or "" stores the empty name. */
static canor_status canor_elite_name_copy_opt(char *dst, const char *src) {
  if (!src || !src[0]) {
    dst[0] = '\0';
    return CANOR_OK;
  }
  return canor_elite_name_copy(dst, src);
}

/* Optional blob: NULL stores an UNSET blob (len 0). */
static canor_status canor_elite_blob_copy(canor_elite_blob_t *dst,
                                          const canor_elite_blob_t *src) {
  if (!src) {
    memset(dst, 0, sizeof *dst);
    return CANOR_OK;
  }
  if (src->len > CANOR_ELITE_BLOB_MAX) return CANOR_ERR_RANGE;
  *dst = *src;
  return CANOR_OK;
}

/* Required signature: present and non-empty. */
static canor_status canor_elite_sig_copy(canor_elite_blob_t *dst,
                                         const canor_elite_blob_t *src) {
  if (!src || src->len == 0) return CANOR_ERR_ARGS;
  return canor_elite_blob_copy(dst, src);
}

static bool canor_elite_blob_eq(const canor_elite_blob_t *a,
                                const canor_elite_blob_t *b) {
  return a->len == b->len && memcmp(a->bytes, b->bytes, a->len) == 0;
}

static bool canor_elite_interval_ok(canor_elite_interval_t i) {
  return i.lo >= 0 && i.lo <= i.hi;
}

/* inner within outer, both closed. */
static bool canor_elite_within(canor_elite_interval_t inner,
                               canor_elite_interval_t outer) {
  return inner.lo >= outer.lo && inner.hi <= outer.hi;
}

/* ===========================================================================
 * Builders
 * ========================================================================= */

canor_status canor_elite_profile_init(canor_elite_profile_t *p, const char *name,
                                      uint32_t version) {
  if (!p) return CANOR_ERR_ARGS;
  memset(p, 0, sizeof *p);
  p->version = version;
  return canor_elite_name_copy(p->name, name);
}

canor_status canor_elite_profile_add_setting(canor_elite_profile_t *p,
                                             const char *name, uint32_t version) {
  canor_elite_setting_t *s;
  uint8_t i;
  canor_status st;
  if (!p) return CANOR_ERR_ARGS;
  if (p->settings_count >= CANOR_ELITE_SETTINGS_MAX) return CANOR_ERR_FULL;
  s = &p->settings[p->settings_count];
  st = canor_elite_name_copy(s->name, name);
  if (st != CANOR_OK) return st;
  for (i = 0; i < p->settings_count; ++i)
    if (canor_elite_name_eq(p->settings[i].name, s->name)) return CANOR_ERR_ARGS;
  s->version = version;
  ++p->settings_count;
  return CANOR_OK;
}

canor_status canor_elite_profile_add_contract(canor_elite_profile_t *p,
                                              const char *contract) {
  uint8_t i;
  canor_status st;
  if (!p) return CANOR_ERR_ARGS;
  if (p->contract_count >= CANOR_ELITE_CONTRACT_MAX) return CANOR_ERR_FULL;
  st = canor_elite_name_copy(p->contract[p->contract_count], contract);
  if (st != CANOR_OK) return st;
  for (i = 0; i < p->contract_count; ++i)
    if (canor_elite_name_eq(p->contract[i], p->contract[p->contract_count]))
      return CANOR_ERR_ARGS;
  ++p->contract_count;
  return CANOR_OK;
}

canor_status canor_elite_basis_init(canor_elite_basis_t *b, const char *name,
                                    uint32_t version) {
  if (!b) return CANOR_ERR_ARGS;
  memset(b, 0, sizeof *b);
  b->version = version;
  return canor_elite_name_copy(b->name, name);
}

canor_status canor_elite_basis_add_word(canor_elite_basis_t *b, const char *token,
                                        uint8_t arity, bool effect,
                                        const char *contract) {
  canor_elite_word_t *w;
  uint8_t i;
  canor_status st;
  if (!b) return CANOR_ERR_ARGS;
  if (b->vocab_count >= CANOR_ELITE_VOCAB_MAX) return CANOR_ERR_FULL;
  if (arity > CANOR_ELITE_OPERAND_MAX) return CANOR_ERR_RANGE;
  w = &b->vocab[b->vocab_count];
  memset(w, 0, sizeof *w);
  st = canor_elite_name_copy(w->token, token);
  if (st != CANOR_OK) return st;
  for (i = 0; i < b->vocab_count; ++i)
    if (canor_elite_name_eq(b->vocab[i].token, w->token)) return CANOR_ERR_ARGS;
  st = canor_elite_name_copy_opt(w->contract, contract);
  if (st != CANOR_OK) return st;
  w->arity = arity;
  w->effect = effect;
  ++b->vocab_count;
  return CANOR_OK;
}

canor_status canor_elite_basis_add_category(canor_elite_basis_t *b,
                                            const char *name, const char *metric,
                                            int64_t scale,
                                            canor_elite_interval_t bound) {
  canor_elite_category_t *c;
  uint8_t i;
  canor_status st;
  if (!b) return CANOR_ERR_ARGS;
  if (b->category_count >= CANOR_ELITE_CATEGORY_MAX) return CANOR_ERR_FULL;
  if (scale <= 0 || scale > CANOR_ELITE_SCALE_MAX) return CANOR_ERR_RANGE;
  if (!canor_elite_interval_ok(bound)) return CANOR_ERR_RANGE;
  c = &b->category[b->category_count];
  memset(c, 0, sizeof *c);
  st = canor_elite_name_copy(c->name, name);
  if (st != CANOR_OK) return st;
  for (i = 0; i < b->category_count; ++i)
    if (canor_elite_name_eq(b->category[i].name, c->name)) return CANOR_ERR_ARGS;
  st = canor_elite_name_copy(c->metric, metric);
  if (st != CANOR_OK) return st;
  c->scale = scale;
  c->bound = bound;
  ++b->category_count;
  return CANOR_OK;
}

canor_status canor_elite_accord_init(canor_elite_accord_t *a,
                                     const canor_elite_basis_t *basis,
                                     const canor_elite_blob_t *object_digest,
                                     uint32_t object_version,
                                     const canor_elite_blob_t *provenance) {
  canor_status st;
  if (!a || !basis) return CANOR_ERR_ARGS;
  memset(a, 0, sizeof *a);
  a->basis = *basis;
  a->object_version = object_version;
  st = canor_elite_blob_copy(&a->object_digest, object_digest);
  if (st != CANOR_OK) return st;
  return canor_elite_blob_copy(&a->provenance, provenance);
}

canor_status canor_elite_accord_add_signatory(canor_elite_accord_t *a,
                                              const char *name,
                                              canor_elite_role_t role,
                                              bool external,
                                              const canor_elite_blob_t *key,
                                              const canor_elite_blob_t *profile,
                                              uint8_t *index_out) {
  canor_elite_signatory_t *s;
  uint8_t i;
  canor_status st;
  if (!a) return CANOR_ERR_ARGS;
  if (a->signatory_count >= CANOR_ELITE_SIGNATORY_MAX) return CANOR_ERR_FULL;
  if (role != CANOR_ELITE_ROLE_USER && role != CANOR_ELITE_ROLE_APP &&
      role != CANOR_ELITE_ROLE_AGENT)
    return CANOR_ERR_RANGE;
  s = &a->signatory[a->signatory_count];
  memset(s, 0, sizeof *s);
  st = canor_elite_name_copy(s->name, name);
  if (st != CANOR_OK) return st;
  /* Z §7: a.signatory = b.signatory => a = b. Distinct parties only. */
  for (i = 0; i < a->signatory_count; ++i)
    if (canor_elite_name_eq(a->signatory[i].name, s->name)) return CANOR_ERR_ARGS;
  st = canor_elite_blob_copy(&s->key, key);
  if (st != CANOR_OK) return st;
  st = canor_elite_blob_copy(&s->profile, profile);
  if (st != CANOR_OK) return st;
  s->role = role;
  s->external = external;
  /* The roster changed, so every roster-bound consent is void. */
  memset(a->ratification, 0, sizeof a->ratification);
  memset(a->approval, 0, sizeof a->approval);
  if (index_out) *index_out = a->signatory_count;
  ++a->signatory_count;
  return CANOR_OK;
}

canor_status canor_elite_accord_ratify(canor_elite_accord_t *a, uint8_t s,
                                       const canor_elite_blob_t *signature) {
  canor_status st;
  if (!a) return CANOR_ERR_ARGS;
  if (s >= a->signatory_count) return CANOR_ERR_RANGE;
  st = canor_elite_sig_copy(&a->ratification[s].signature, signature);
  if (st != CANOR_OK) return st;
  a->ratification[s].present = true;
  return CANOR_OK;
}

canor_status canor_elite_accord_submit(canor_elite_accord_t *a, uint8_t s,
                                       uint8_t c,
                                       canor_elite_interval_t allowance,
                                       canor_elite_interval_t measured,
                                       const char *method,
                                       const canor_elite_blob_t *evidence) {
  canor_elite_submission_t sub;
  uint8_t t;
  canor_status st;
  if (!a) return CANOR_ERR_ARGS;
  if (s >= a->signatory_count || c >= a->basis.category_count)
    return CANOR_ERR_RANGE;
  /* Well-formedness only. Whether the numbers are within bounds is the
   * evaluator's verdict, reported as a diagnostic, never a refusal here. */
  if (!canor_elite_interval_ok(allowance) || !canor_elite_interval_ok(measured))
    return CANOR_ERR_RANGE;
  memset(&sub, 0, sizeof sub);
  st = canor_elite_name_copy(sub.method, method);
  if (st != CANOR_OK) return st;
  st = canor_elite_blob_copy(&sub.evidence, evidence);
  if (st != CANOR_OK) return st;
  sub.present = true;
  sub.allowance = allowance;
  sub.measured = measured;
  a->submission[s][c] = sub;
  /* Certificates signed the old numbers; they no longer apply. */
  for (t = 0; t < a->signatory_count; ++t)
    memset(&a->cert[t][s][c], 0, sizeof a->cert[t][s][c]);
  return CANOR_OK;
}

canor_status canor_elite_accord_certify(canor_elite_accord_t *a,
                                        uint8_t certifier, uint8_t subject,
                                        uint8_t c, canor_elite_verdict_t verdict,
                                        const canor_elite_blob_t *signature) {
  canor_status st;
  if (!a) return CANOR_ERR_ARGS;
  if (certifier >= a->signatory_count || subject >= a->signatory_count ||
      c >= a->basis.category_count)
    return CANOR_ERR_RANGE;
  /* "certified within bounds by the OTHER signatory parts": never oneself. */
  if (certifier == subject) return CANOR_ERR_ARGS;
  if (verdict != CANOR_ELITE_VERDICT_CERTIFIED &&
      verdict != CANOR_ELITE_VERDICT_REJECTED)
    return CANOR_ERR_ARGS;
  st = canor_elite_sig_copy(&a->cert[certifier][subject][c].signature, signature);
  if (st != CANOR_OK) return st;
  a->cert[certifier][subject][c].verdict = verdict;
  return CANOR_OK;
}

canor_status canor_elite_accord_approve(canor_elite_accord_t *a, uint8_t s,
                                        const canor_elite_blob_t *signature) {
  canor_status st;
  if (!a) return CANOR_ERR_ARGS;
  if (s >= a->signatory_count) return CANOR_ERR_RANGE;
  st = canor_elite_sig_copy(&a->approval[s].signature, signature);
  if (st != CANOR_OK) return st;
  a->approval[s].present = true;
  return CANOR_OK;
}

canor_status canor_elite_program_init(canor_elite_program_t *p,
                                      canor_elite_direction_t direction) {
  if (!p) return CANOR_ERR_ARGS;
  if (direction != CANOR_ELITE_L2R && direction != CANOR_ELITE_R2L)
    return CANOR_ERR_RANGE;
  memset(p, 0, sizeof *p);
  p->direction = direction;
  return CANOR_OK;
}

/* ===========================================================================
 * Freeform Elite source (rules and program lines; design in canor_elite.h)
 * ========================================================================= */

/* Length of a text held in a fixed array, bounded like canor_elite_strlen. */
static size_t canor_elite_textlen(const char *s) {
  size_t n = 0;
  while (n < CANOR_ELITE_TEXT_MAX && s[n]) ++n;
  return n;
}

static bool canor_elite_space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

/* Word bytes for match boundaries. Every non-ASCII byte counts, so a UTF-8
 * letter is never split in the middle. */
static bool canor_elite_word(unsigned char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_' || c >= 0x80u;
}

/* The only two escapes: "\:" and "\|". */
static bool canor_elite_escape_at(const char *s, size_t i, size_t e) {
  return s[i] == '\\' && i + 1 < e && (s[i + 1] == ':' || s[i + 1] == '|');
}

/* s[0..n) trimmed of surrounding whitespace and unescaped, into
 * dst[CANOR_ELITE_TEXT_MAX]. RANGE if too long, ARGS on a NUL byte. */
static canor_status canor_elite_text_copy(char *dst, const char *s, size_t n) {
  size_t b = 0, e = n, i, k = 0;
  while (b < e && canor_elite_space((unsigned char)s[b])) ++b;
  while (e > b && canor_elite_space((unsigned char)s[e - 1])) --e;
  for (i = b; i < e; ++i) {
    char c = s[i];
    if (canor_elite_escape_at(s, i, e)) c = s[++i];
    if (c == '\0') return CANOR_ERR_ARGS;
    if (k + 1 >= CANOR_ELITE_TEXT_MAX) return CANOR_ERR_RANGE;
    dst[k++] = c;
  }
  dst[k] = '\0';
  return CANOR_OK;
}

static int canor_elite_rule_find(const canor_elite_program_t *pg,
                                 const char *spec) {
  uint8_t i;
  for (i = 0; i < pg->rule_count; ++i)
    if (strncmp(pg->rule[i].spec, spec, CANOR_ELITE_TEXT_MAX) == 0) return i;
  return -1;
}

canor_status canor_elite_program_define(canor_elite_program_t *p,
                                        const char *spec, const char *def,
                                        uint16_t source_line) {
  canor_elite_rule_t *r;
  size_t ns, nd;
  int ri;
  if (!p || !spec || !def) return CANOR_ERR_ARGS;
  ns = strlen(spec);
  nd = strlen(def);
  if (ns == 0) return CANOR_ERR_ARGS; /* an empty spec would match everywhere */
  if (ns >= CANOR_ELITE_TEXT_MAX || nd >= CANOR_ELITE_TEXT_MAX)
    return CANOR_ERR_RANGE;
  ri = canor_elite_rule_find(p, spec);
  if (ri < 0) {
    if (p->rule_count >= CANOR_ELITE_RULE_MAX) return CANOR_ERR_FULL;
    r = &p->rule[p->rule_count++];
    memset(r, 0, sizeof *r);
    memcpy(r->spec, spec, ns + 1);
  } else {
    r = &p->rule[ri];
    if (r->def_count >= CANOR_ELITE_ALT_MAX) return CANOR_ERR_FULL;
  }
  /* A redefinition appends: repeated definitions are distinct positions, as
   * duplicate atoms are in a Flow (§1). */
  memcpy(r->def[r->def_count], def, nd + 1);
  r->def_line[r->def_count] = source_line;
  ++r->def_count;
  return CANOR_OK;
}

canor_status canor_elite_program_add_rule(canor_elite_program_t *p,
                                          const char *symbol,
                                          const char *const *alt, uint8_t count) {
  uint8_t i, have = 0;
  int ri;
  canor_status st;
  if (!p || !symbol || !alt) return CANOR_ERR_ARGS;
  if (count == 0 || count > CANOR_ELITE_ALT_MAX) return CANOR_ERR_RANGE;
  for (i = 0; i < count; ++i) {
    if (!alt[i]) return CANOR_ERR_ARGS;
    if (strlen(alt[i]) >= CANOR_ELITE_TEXT_MAX) return CANOR_ERR_RANGE;
  }
  if (!symbol[0]) return CANOR_ERR_ARGS;
  if (strlen(symbol) >= CANOR_ELITE_TEXT_MAX) return CANOR_ERR_RANGE;
  /* Capacity first, so the rule is added whole or not at all. */
  ri = canor_elite_rule_find(p, symbol);
  if (ri < 0 && p->rule_count >= CANOR_ELITE_RULE_MAX) return CANOR_ERR_FULL;
  if (ri >= 0) have = p->rule[ri].def_count;
  if (have + count > CANOR_ELITE_ALT_MAX) return CANOR_ERR_FULL;
  for (i = 0; i < count; ++i) {
    st = canor_elite_program_define(p, symbol, alt[i], 0);
    if (st != CANOR_OK) return st;
  }
  return CANOR_OK;
}

canor_status canor_elite_program_add_line(canor_elite_program_t *p,
                                          const char *text,
                                          uint16_t source_line) {
  canor_elite_line_t *l;
  size_t n;
  if (!p || !text || !text[0]) return CANOR_ERR_ARGS;
  if (p->line_count >= CANOR_ELITE_INSN_MAX) return CANOR_ERR_FULL;
  n = strlen(text);
  if (n >= CANOR_ELITE_TEXT_MAX) return CANOR_ERR_RANGE;
  l = &p->line[p->line_count++];
  memset(l, 0, sizeof *l);
  memcpy(l->text, text, n + 1);
  l->source_line = source_line;
  return CANOR_OK;
}

canor_status canor_elite_program_add_insn(canor_elite_program_t *p,
                                          const char *head,
                                          const char *const *operand,
                                          uint8_t count) {
  char buf[CANOR_ELITE_TEXT_MAX];
  size_t k = 0, n;
  uint8_t i;
  if (!p || !head || !head[0] || (count > 0 && !operand)) return CANOR_ERR_ARGS;
  if (count > CANOR_ELITE_OPERAND_MAX) return CANOR_ERR_RANGE;
  n = strlen(head);
  if (n >= sizeof buf) return CANOR_ERR_RANGE;
  memcpy(buf, head, n);
  k = n;
  for (i = 0; i < count; ++i) {
    const char *sep = i ? ", " : " ";
    if (!operand[i] || !operand[i][0]) return CANOR_ERR_ARGS;
    n = strlen(operand[i]);
    if (k + strlen(sep) + n >= sizeof buf) return CANOR_ERR_RANGE;
    memcpy(buf + k, sep, strlen(sep));
    k += strlen(sep);
    memcpy(buf + k, operand[i], n);
    k += n;
  }
  buf[k] = '\0';
  return canor_elite_program_add_line(p, buf, 0);
}

static canor_elite_parse_kind_t canor_elite_parse_kind(canor_status st) {
  if (st == CANOR_ERR_RANGE) return CANOR_ELITE_PARSE_TOO_LONG;
  if (st == CANOR_ERR_FULL) return CANOR_ELITE_PARSE_FULL;
  return CANOR_ELITE_PARSE_MALFORMED;
}

/* Record a parse diagnostic. Past the capacity it is still COUNTED, so the
 * program says how many it could not store: reported, never hidden. */
static canor_status canor_elite_parse_diag(canor_elite_program_t *p,
                                           uint16_t line, canor_status st,
                                           canor_elite_parse_kind_t kind) {
  if (p->diag_total < UINT16_MAX) ++p->diag_total;
  if (p->diag_count < CANOR_ELITE_PARSE_DIAG_MAX) {
    canor_elite_parse_diag_t *d = &p->diag[p->diag_count++];
    d->line = line;
    d->status = (int8_t)st;
    d->kind = (uint8_t)kind;
  }
  return st;
}

/* One source line, without its line ending. Returns its first problem. */
static canor_status canor_elite_parse_line(canor_elite_program_t *p,
                                           const char *s, size_t n,
                                           uint16_t line_no) {
  char spec[CANOR_ELITE_TEXT_MAX], def[CANOR_ELITE_TEXT_MAX];
  canor_status st, first = CANOR_OK;
  size_t b = 0, e = n, i, colon = 0, start;
  unsigned depth = 0;
  bool is_rule = false;
  while (b < e && canor_elite_space((unsigned char)s[b])) ++b;
  while (e > b && canor_elite_space((unsigned char)s[e - 1])) --e;
  if (b == e) return CANOR_OK; /* blank */
  for (i = b; i < e; ++i) {
    if (canor_elite_escape_at(s, i, e)) {
      ++i;
      continue;
    }
    if (s[i] == ':') {
      is_rule = true;
      colon = i;
      break;
    }
  }
  if (!is_rule) {
    st = canor_elite_text_copy(def, s + b, e - b);
    if (st == CANOR_OK) st = canor_elite_program_add_line(p, def, line_no);
    return st == CANOR_OK
               ? CANOR_OK
               : canor_elite_parse_diag(p, line_no, st, canor_elite_parse_kind(st));
  }
  st = canor_elite_text_copy(spec, s + b, colon - b);
  if (st != CANOR_OK)
    return canor_elite_parse_diag(p, line_no, st, canor_elite_parse_kind(st));
  if (!spec[0])
    return canor_elite_parse_diag(p, line_no, CANOR_ERR_ARGS,
                                  CANOR_ELITE_PARSE_EMPTY_SPEC);
  /* Definitions: split at each top-level, unescaped '|'. i == e closes the
   * last one, so "spec :" defines the empty text (a deletion). */
  start = colon + 1;
  for (i = colon + 1; i <= e; ++i) {
    if (i < e) {
      char c = s[i];
      if (canor_elite_escape_at(s, i, e)) {
        ++i;
        continue;
      }
      if (c == '(' || c == '[' || c == '{') {
        ++depth;
        continue;
      }
      if (c == ')' || c == ']' || c == '}') {
        if (depth) --depth;
        continue;
      }
      if (c != '|' || depth) continue;
    }
    st = canor_elite_text_copy(def, s + start, i - start);
    if (st == CANOR_OK) st = canor_elite_program_define(p, spec, def, line_no);
    if (st != CANOR_OK) {
      canor_elite_parse_diag(p, line_no, st, canor_elite_parse_kind(st));
      if (first == CANOR_OK) first = st;
    }
    start = i + 1;
  }
  return first;
}

canor_status canor_elite_program_parse(canor_elite_program_t *p,
                                       const char *text, size_t len) {
  canor_status st, first = CANOR_OK;
  size_t pos = 0;
  uint32_t line_no = 0;
  if (!p || (!text && len)) return CANOR_ERR_ARGS;
  while (pos < len) {
    size_t end = pos;
    while (end < len && text[end] != '\n') ++end;
    ++line_no;
    /* trailing '\r' of CRLF is whitespace to the trim */
    st = canor_elite_parse_line(p, text + pos, end - pos,
                                line_no > UINT16_MAX ? (uint16_t)UINT16_MAX
                                                     : (uint16_t)line_no);
    if (first == CANOR_OK) first = st;
    pos = end + 1;
  }
  return first;
}

/* ===========================================================================
 * Canonical bytes
 *
 * The encoder keeps counting past the end of the buffer so a too-small call
 * still reports the size required. A malformed field (unterminated name,
 * oversized blob) marks the encoding bad: CANOR_ERR_ARGS, never a silently
 * truncated statement that someone might sign.
 * ========================================================================= */

typedef struct {
  uint8_t *buf;
  size_t cap;
  size_t len;
  bool bad;
} canor_elite_enc_t;

static void canor_elite_enc_bytes(canor_elite_enc_t *e, const void *p, size_t n) {
  if (e->buf && e->len <= e->cap && n <= e->cap - e->len)
    memcpy(e->buf + e->len, p, n);
  e->len += n;
}

static void canor_elite_enc_u8(canor_elite_enc_t *e, uint8_t v) {
  canor_elite_enc_bytes(e, &v, 1);
}

static void canor_elite_enc_u16(canor_elite_enc_t *e, size_t v) {
  uint8_t b[2];
  b[0] = (uint8_t)((v >> 8) & 0xFFu);
  b[1] = (uint8_t)(v & 0xFFu);
  canor_elite_enc_bytes(e, b, 2);
}

/* 8-byte big-endian two's complement. The uint64_t conversion is defined
 * modulo 2^64 by C11 6.3.1.3, so this is the same bytes on every host. */
static void canor_elite_enc_i64(canor_elite_enc_t *e, int64_t v) {
  uint64_t u = (uint64_t)v;
  uint8_t b[CANOR_ELITE_ENC_INT];
  int i;
  for (i = CANOR_ELITE_ENC_INT - 1; i >= 0; --i) {
    b[i] = (uint8_t)(u & 0xFFu);
    u >>= 8;
  }
  canor_elite_enc_bytes(e, b, CANOR_ELITE_ENC_INT);
}

static void canor_elite_enc_str(canor_elite_enc_t *e, const char *s) {
  size_t n = canor_elite_strlen(s);
  if (n >= CANOR_ELITE_NAME_MAX) {
    e->bad = true;
    return;
  }
  canor_elite_enc_u16(e, n);
  canor_elite_enc_bytes(e, s, n);
}

static void canor_elite_enc_blob(canor_elite_enc_t *e,
                                 const canor_elite_blob_t *b) {
  if (b->len > CANOR_ELITE_BLOB_MAX) {
    e->bad = true;
    return;
  }
  canor_elite_enc_u16(e, b->len);
  canor_elite_enc_bytes(e, b->bytes, b->len);
}

static canor_status canor_elite_enc_finish(const canor_elite_enc_t *e,
                                           size_t *len) {
  *len = e->len;
  if (e->bad) return CANOR_ERR_ARGS;
  return e->len > e->cap ? CANOR_ERR_FULL : CANOR_OK;
}

static bool canor_elite_enc_args_ok(const void *x, const uint8_t *buf,
                                    size_t cap, const size_t *len) {
  return x && len && (buf || cap == 0);
}

static void canor_elite_enc_init(canor_elite_enc_t *e, uint8_t *buf, size_t cap,
                                 const char *tag) {
  e->buf = buf;
  e->cap = cap;
  e->len = 0;
  e->bad = false;
  canor_elite_enc_str(e, tag);
}

/* The roster: who is party to this consent (see add_signatory). */
static void canor_elite_enc_roster(canor_elite_enc_t *e,
                                   const canor_elite_accord_t *a) {
  uint8_t i;
  canor_elite_enc_u8(e, a->signatory_count);
  for (i = 0; i < a->signatory_count; ++i) {
    canor_elite_enc_str(e, a->signatory[i].name);
    canor_elite_enc_u8(e, (uint8_t)a->signatory[i].role);
    canor_elite_enc_u8(e, a->signatory[i].external ? 1u : 0u);
    canor_elite_enc_blob(e, &a->signatory[i].profile);
  }
}

canor_status canor_elite_basis_encode(const canor_elite_basis_t *b, uint8_t *buf,
                                      size_t cap, size_t *len) {
  canor_elite_enc_t e;
  uint8_t i;
  if (!canor_elite_enc_args_ok(b, buf, cap, len)) return CANOR_ERR_ARGS;
  if (b->vocab_count > CANOR_ELITE_VOCAB_MAX ||
      b->category_count > CANOR_ELITE_CATEGORY_MAX)
    return CANOR_ERR_RANGE;
  canor_elite_enc_init(&e, buf, cap, canor_elite_tag_basis);
  canor_elite_enc_str(&e, b->name);
  canor_elite_enc_i64(&e, (int64_t)b->version);
  /* Declaration order is significant, like the United manifest (§6). */
  canor_elite_enc_u8(&e, b->vocab_count);
  for (i = 0; i < b->vocab_count; ++i) {
    canor_elite_enc_str(&e, b->vocab[i].token);
    canor_elite_enc_u8(&e, b->vocab[i].arity);
    canor_elite_enc_u8(&e, b->vocab[i].effect ? 1u : 0u);
    canor_elite_enc_str(&e, b->vocab[i].contract);
  }
  canor_elite_enc_u8(&e, b->category_count);
  for (i = 0; i < b->category_count; ++i) {
    canor_elite_enc_str(&e, b->category[i].name);
    canor_elite_enc_str(&e, b->category[i].metric);
    canor_elite_enc_i64(&e, b->category[i].scale);
    canor_elite_enc_i64(&e, b->category[i].bound.lo);
    canor_elite_enc_i64(&e, b->category[i].bound.hi);
  }
  return canor_elite_enc_finish(&e, len);
}

canor_status canor_elite_statement_ratify(const canor_elite_accord_t *a,
                                          uint8_t s, uint8_t *buf, size_t cap,
                                          size_t *len) {
  canor_elite_enc_t e;
  if (!canor_elite_enc_args_ok(a, buf, cap, len)) return CANOR_ERR_ARGS;
  if (s >= a->signatory_count || a->signatory_count > CANOR_ELITE_SIGNATORY_MAX)
    return CANOR_ERR_RANGE;
  canor_elite_enc_init(&e, buf, cap, canor_elite_tag_ratify);
  canor_elite_enc_blob(&e, &a->basis.digest);
  canor_elite_enc_str(&e, a->basis.name);
  canor_elite_enc_i64(&e, (int64_t)a->basis.version);
  canor_elite_enc_str(&e, a->signatory[s].name);
  canor_elite_enc_roster(&e, a);
  return canor_elite_enc_finish(&e, len);
}

canor_status canor_elite_statement_cert(const canor_elite_accord_t *a,
                                        uint8_t certifier, uint8_t subject,
                                        uint8_t c, canor_elite_verdict_t verdict,
                                        uint8_t *buf, size_t cap, size_t *len) {
  canor_elite_enc_t e;
  const canor_elite_submission_t *sub;
  const canor_elite_category_t *cat;
  if (!canor_elite_enc_args_ok(a, buf, cap, len)) return CANOR_ERR_ARGS;
  if (certifier >= a->signatory_count || subject >= a->signatory_count ||
      c >= a->basis.category_count ||
      a->signatory_count > CANOR_ELITE_SIGNATORY_MAX ||
      a->basis.category_count > CANOR_ELITE_CATEGORY_MAX)
    return CANOR_ERR_RANGE;
  if (certifier == subject) return CANOR_ERR_ARGS;
  sub = &a->submission[subject][c];
  if (!sub->present) return CANOR_ERR_NOT_FOUND; /* nothing to certify */
  cat = &a->basis.category[c];
  canor_elite_enc_init(&e, buf, cap, canor_elite_tag_cert);
  canor_elite_enc_blob(&e, &a->basis.digest);
  canor_elite_enc_str(&e, a->signatory[certifier].name);
  canor_elite_enc_str(&e, a->signatory[subject].name);
  /* "a signatory's profile AND error-metric allowance is certified" */
  canor_elite_enc_blob(&e, &a->signatory[subject].profile);
  canor_elite_enc_str(&e, cat->name);
  canor_elite_enc_str(&e, cat->metric);
  canor_elite_enc_i64(&e, cat->scale);
  canor_elite_enc_i64(&e, sub->allowance.lo);
  canor_elite_enc_i64(&e, sub->allowance.hi);
  canor_elite_enc_i64(&e, sub->measured.lo);
  canor_elite_enc_i64(&e, sub->measured.hi);
  canor_elite_enc_str(&e, sub->method);
  canor_elite_enc_blob(&e, &sub->evidence);
  canor_elite_enc_u8(&e, (uint8_t)verdict);
  return canor_elite_enc_finish(&e, len);
}

canor_status canor_elite_statement_approve(const canor_elite_accord_t *a,
                                           uint8_t s, uint8_t *buf, size_t cap,
                                           size_t *len) {
  canor_elite_enc_t e;
  uint8_t i;
  if (!canor_elite_enc_args_ok(a, buf, cap, len)) return CANOR_ERR_ARGS;
  if (s >= a->signatory_count || a->signatory_count > CANOR_ELITE_SIGNATORY_MAX ||
      a->basis.category_count > CANOR_ELITE_CATEGORY_MAX)
    return CANOR_ERR_RANGE;
  /* Z §7 UnitedApproval: objectDigest, objectVersion, basis, provenance,
   * acceptedErrorMargin - here the basis bound vector, identical for every
   * approval by construction (Z: a.acceptedErrorMargin = b.acceptedErrorMargin). */
  canor_elite_enc_init(&e, buf, cap, canor_elite_tag_approve);
  canor_elite_enc_blob(&e, &a->object_digest);
  canor_elite_enc_i64(&e, (int64_t)a->object_version);
  canor_elite_enc_blob(&e, &a->basis.digest);
  canor_elite_enc_blob(&e, &a->provenance);
  canor_elite_enc_str(&e, a->signatory[s].name);
  canor_elite_enc_roster(&e, a);
  canor_elite_enc_u8(&e, a->basis.category_count);
  for (i = 0; i < a->basis.category_count; ++i) {
    canor_elite_enc_str(&e, a->basis.category[i].name);
    canor_elite_enc_str(&e, a->basis.category[i].metric);
    canor_elite_enc_i64(&e, a->basis.category[i].scale);
    canor_elite_enc_i64(&e, a->basis.category[i].bound.lo);
    canor_elite_enc_i64(&e, a->basis.category[i].bound.hi);
  }
  return canor_elite_enc_finish(&e, len);
}

canor_status canor_elite_vectors_encode(const int32_t *v, uint8_t count,
                                        uint8_t dim, int64_t vscale,
                                        uint8_t *buf, size_t cap, size_t *len) {
  canor_elite_enc_t e;
  size_t i, n;
  if (!canor_elite_enc_args_ok(v, buf, cap, len)) return CANOR_ERR_ARGS;
  if (count == 0 || count > CANOR_ELITE_VECTOR_MAX || dim == 0 ||
      dim > CANOR_ELITE_DIM_MAX)
    return CANOR_ERR_RANGE;
  canor_elite_enc_init(&e, buf, cap, canor_elite_tag_vectors);
  canor_elite_enc_u8(&e, count);
  canor_elite_enc_u8(&e, dim);
  canor_elite_enc_i64(&e, vscale);
  n = (size_t)count * dim;
  for (i = 0; i < n; ++i) canor_elite_enc_i64(&e, v[i]);
  return canor_elite_enc_finish(&e, len);
}

/* ===========================================================================
 * Error metric and certification
 * ========================================================================= */

canor_status canor_elite_gram_linf(const int32_t *v, uint8_t count, uint8_t dim,
                                   int64_t vscale, int64_t cscale,
                                   canor_elite_interval_t *out) {
  int64_t s2, d = 0, q, r, t, lo;
  size_t i, x, y;
  if (!v || !out) return CANOR_ERR_ARGS;
  if (count == 0 || count > CANOR_ELITE_VECTOR_MAX || dim == 0 ||
      dim > CANOR_ELITE_DIM_MAX)
    return CANOR_ERR_RANGE;
  if (vscale <= 0 || vscale > CANOR_ELITE_SCALE_MAX || cscale <= 0 ||
      cscale > CANOR_ELITE_SCALE_MAX)
    return CANOR_ERR_RANGE;
  for (i = 0; i < (size_t)count * dim; ++i)
    if (v[i] > CANOR_ELITE_COMPONENT_MAX || v[i] < -CANOR_ELITE_COMPONENT_MAX)
      return CANOR_ERR_RANGE;
  s2 = vscale * vscale; /* <= 2^40 */
  for (x = 0; x < count; ++x) {
    for (y = x; y < count; ++y) {
      int64_t g = 0, dev;
      for (i = 0; i < dim; ++i)
        g += (int64_t)v[x * dim + i] * (int64_t)v[y * dim + i]; /* |g| < 2^53 */
      dev = g - (x == y ? s2 : 0);
      if (dev < 0) dev = -dev;
      if (dev > d) d = dev;
    }
  }
  /* D * cscale / s2 = q * cscale + r * cscale / s2 with D = q * s2 + r.
   * r < s2 <= 2^40 and cscale <= 2^20 keep r * cscale below 2^60. Only
   * q * cscale can overflow, and that is checked, not wrapped. */
  q = d / s2;
  r = d % s2;
  if (q > INT64_MAX / cscale) return CANOR_ERR_RANGE;
  t = r * cscale;
  lo = q * cscale;
  if (lo > INT64_MAX - t / s2 - 1) return CANOR_ERR_RANGE;
  lo += t / s2;
  out->lo = lo;
  out->hi = lo + (t % s2 != 0 ? 1 : 0);
  return CANOR_OK;
}

canor_status canor_elite_certify_check(const canor_elite_profile_t *p,
                                       const canor_elite_accord_t *a,
                                       uint8_t subject, uint8_t c,
                                       const int32_t *v, uint8_t count,
                                       uint8_t dim, int64_t vscale,
                                       canor_elite_verdict_t *verdict,
                                       uint16_t *flags) {
  uint8_t buf[CANOR_ELITE_VECTORS_ENCODED_MAX];
  const canor_elite_submission_t *sub;
  const canor_elite_category_t *cat;
  canor_elite_interval_t e;
  canor_elite_blob_t d;
  size_t len;
  uint16_t f = 0;
  canor_status st;
  if (!p || !a || !v || !verdict || !flags) return CANOR_ERR_ARGS;
  *verdict = CANOR_ELITE_VERDICT_PENDING;
  *flags = 0;
  if (subject >= a->signatory_count || c >= a->basis.category_count)
    return CANOR_ERR_RANGE;
  sub = &a->submission[subject][c];
  cat = &a->basis.category[c];
  if (!sub->present) return CANOR_ERR_NOT_FOUND;
  if (!p->digest || !p->digest_algorithm[0] ||
      !canor_elite_name_eq(cat->metric, CANOR_ELITE_METRIC_GRAM_LINF)) {
    *flags = CANOR_ELITE_F_UNSET;
    return CANOR_ERR_UNDEFINED;
  }
  st = canor_elite_gram_linf(v, count, dim, vscale, cat->scale, &e);
  if (st != CANOR_OK) return st;
  st = canor_elite_vectors_encode(v, count, dim, vscale, buf, sizeof buf, &len);
  if (st != CANOR_OK) return st;
  st = p->digest(p->crypto_ctx, buf, len, &d);
  if (st != CANOR_OK) {
    *flags = CANOR_ELITE_F_UNSET;
    return CANOR_ERR_UNDEFINED;
  }
  if (sub->evidence.len == 0 || !canor_elite_blob_eq(&d, &sub->evidence))
    f |= CANOR_ELITE_F_DIGEST_MISMATCH;
  if (!canor_elite_within(e, sub->measured)) f |= CANOR_ELITE_F_INCONSISTENT;
  if (!canor_elite_within(sub->measured, sub->allowance))
    f |= CANOR_ELITE_F_OVER_ALLOWANCE;
  if (!canor_elite_within(sub->allowance, cat->bound))
    f |= CANOR_ELITE_F_OUT_OF_BOUND;
  *flags = f;
  *verdict = f ? CANOR_ELITE_VERDICT_REJECTED : CANOR_ELITE_VERDICT_CERTIFIED;
  return CANOR_OK;
}

/* ===========================================================================
 * Accord evaluation
 * ========================================================================= */

#define CANOR_ELITE_F_REJECTING                                                \
  (CANOR_ELITE_F_SIG_INVALID | CANOR_ELITE_F_REJECTED |                        \
   CANOR_ELITE_F_OUT_OF_BOUND | CANOR_ELITE_F_OVER_ALLOWANCE |                 \
   CANOR_ELITE_F_MALFORMED | CANOR_ELITE_F_INCONSISTENT |                      \
   CANOR_ELITE_F_DUPLICATE | CANOR_ELITE_F_DIGEST_MISMATCH)
#define CANOR_ELITE_F_UNDEFINING (CANOR_ELITE_F_SIG_UNDEFINED | CANOR_ELITE_F_UNSET)

static canor_elite_outcome_t canor_elite_flags_outcome(uint16_t f) {
  if (f & CANOR_ELITE_F_REJECTING) return CANOR_ELITE_ACCORD_REJECTED;
  if (f & CANOR_ELITE_F_UNDEFINING) return CANOR_ELITE_ACCORD_UNDEFINED;
  if (f) return CANOR_ELITE_ACCORD_INCOMPLETE; /* PENDING, TOO_FEW */
  return CANOR_ELITE_ACCORD_ACCEPTED;
}

static canor_status canor_elite_flags_status(uint16_t f) {
  if (f & (CANOR_ELITE_F_OUT_OF_BOUND | CANOR_ELITE_F_OVER_ALLOWANCE |
           CANOR_ELITE_F_TOO_FEW))
    return CANOR_ERR_RANGE;
  if (f & CANOR_ELITE_F_REJECTING) return CANOR_ERR_ARGS;
  if (f & CANOR_ELITE_F_UNDEFINING) return CANOR_ERR_UNDEFINED;
  return CANOR_ERR_NOT_FOUND; /* pending: not given yet */
}

typedef struct {
  const canor_elite_profile_t *p;
  const canor_elite_accord_t *a;
  canor_elite_report_t *r;
  canor_elite_outcome_t outcome;
  uint16_t checked;
  uint16_t passed;
  bool overflow;
  uint8_t scratch[CANOR_ELITE_SCRATCH_MAX];
} canor_elite_eval_t;

static void canor_elite_eval_item(canor_elite_eval_t *ev,
                                  canor_elite_stage_t stage, uint8_t subject,
                                  uint8_t peer, uint8_t index, uint16_t flags) {
  canor_elite_outcome_t o;
  ++ev->checked;
  if (!flags) {
    ++ev->passed;
    return;
  }
  o = canor_elite_flags_outcome(flags);
  if (o > ev->outcome) ev->outcome = o;
  if (!ev->r) return;
  if (ev->r->diag_count >= CANOR_ELITE_DIAG_MAX) {
    ev->overflow = true; /* unreachable at the default bound; reported, not hidden */
    return;
  }
  {
    canor_elite_diag_t *dg = &ev->r->diag[ev->r->diag_count++];
    dg->stage = (uint8_t)stage;
    dg->status = (int8_t)canor_elite_flags_status(flags);
    dg->subject = subject;
    dg->peer = peer;
    dg->index = index;
    dg->flags = flags;
  }
}

/* Verify a statement already encoded in ev->scratch[0..len). */
static uint16_t canor_elite_eval_verify(canor_elite_eval_t *ev,
                                        const canor_elite_signatory_t *who,
                                        size_t len,
                                        const canor_elite_blob_t *signature) {
  if (!ev->p->verify || !ev->p->signature_algorithm[0])
    return CANOR_ELITE_F_SIG_UNDEFINED;
  switch (ev->p->verify(ev->p->crypto_ctx, who, ev->scratch, len, signature)) {
    case CANOR_ELITE_SIG_VALID: return 0;
    case CANOR_ELITE_SIG_INVALID: return CANOR_ELITE_F_SIG_INVALID;
    case CANOR_ELITE_SIG_UNDEFINED: return CANOR_ELITE_F_SIG_UNDEFINED;
  }
  return CANOR_ELITE_F_SIG_UNDEFINED;
}

static uint16_t canor_elite_blob_flags(const canor_elite_blob_t *b) {
  if (b->len > CANOR_ELITE_BLOB_MAX) return CANOR_ELITE_F_MALFORMED;
  return b->len == 0 ? CANOR_ELITE_F_UNSET : 0;
}

static void canor_elite_eval_accord_level(canor_elite_eval_t *ev) {
  const canor_elite_profile_t *p = ev->p;
  const canor_elite_accord_t *a = ev->a;
  uint16_t f;
  uint8_t i, j;

  /* MOT phase D: "at least two consenting parts". */
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_SIGNATORIES,
                        a->signatory_count < 2 ? CANOR_ELITE_F_TOO_FEW : 0);
  /* Z §7: distinct signatories. The builder refuses duplicates; a struct
   * filled by hand is still checked. */
  for (j = 1; j < a->signatory_count; ++j) {
    f = 0;
    for (i = 0; i < j; ++i)
      if (canor_elite_name_eq(a->signatory[i].name, a->signatory[j].name))
        f = CANOR_ELITE_F_DUPLICATE;
    if (f)
      canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, j, CANOR_ELITE_NONE,
                            CANOR_ELITE_ITEM_SIGNATORIES, f);
  }
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_DIGEST_ALG,
                        (!p->digest || !p->digest_algorithm[0]) ? CANOR_ELITE_F_UNSET
                                                                : 0);
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_SIGNATURE_ALG,
                        (!p->verify || !p->signature_algorithm[0])
                            ? CANOR_ELITE_F_UNSET
                            : 0);
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_OBJECT_DIGEST,
                        canor_elite_blob_flags(&a->object_digest));
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_PROVENANCE,
                        canor_elite_blob_flags(&a->provenance));

  /* The basis digest must be the profile digest of the basis content: a
   * signature over a digest that does not match the vocabulary and envelope
   * actually in force would bind nobody to anything. */
  f = canor_elite_blob_flags(&a->basis.digest);
  if (!f) {
    if (!p->digest || !p->digest_algorithm[0]) {
      f = CANOR_ELITE_F_UNSET;
    } else {
      size_t len;
      canor_elite_blob_t d;
      if (canor_elite_basis_encode(&a->basis, ev->scratch, sizeof ev->scratch,
                                   &len) != CANOR_OK)
        f = CANOR_ELITE_F_MALFORMED;
      else if (p->digest(p->crypto_ctx, ev->scratch, len, &d) != CANOR_OK)
        f = CANOR_ELITE_F_UNSET;
      else if (!canor_elite_blob_eq(&d, &a->basis.digest))
        f = CANOR_ELITE_F_DIGEST_MISMATCH;
    }
  }
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_BASIS_DIGEST, f);
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_VOCAB,
                        a->basis.vocab_count == 0 ? CANOR_ELITE_F_UNSET : 0);
  canor_elite_eval_item(ev, CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE,
                        CANOR_ELITE_NONE, CANOR_ELITE_ITEM_CATEGORIES,
                        a->basis.category_count == 0 ? CANOR_ELITE_F_UNSET : 0);
}

static void canor_elite_eval_basis(canor_elite_eval_t *ev) {
  const canor_elite_basis_t *b = &ev->a->basis;
  uint16_t f;
  uint8_t i, j;
  for (i = 0; i < b->vocab_count; ++i) {
    const canor_elite_word_t *w = &b->vocab[i];
    f = 0;
    if (!canor_elite_name_ok(w->token) ||
        canor_elite_strlen(w->contract) >= CANOR_ELITE_NAME_MAX ||
        w->arity > CANOR_ELITE_OPERAND_MAX)
      f |= CANOR_ELITE_F_MALFORMED;
    for (j = 0; j < i; ++j)
      if (canor_elite_name_eq(b->vocab[j].token, w->token))
        f |= CANOR_ELITE_F_DUPLICATE;
    canor_elite_eval_item(ev, CANOR_ELITE_STAGE_VOCAB, CANOR_ELITE_NONE,
                          CANOR_ELITE_NONE, i, f);
  }
  for (i = 0; i < b->category_count; ++i) {
    const canor_elite_category_t *c = &b->category[i];
    f = 0;
    if (!canor_elite_name_ok(c->name) || !canor_elite_name_ok(c->metric) ||
        c->scale <= 0 || c->scale > CANOR_ELITE_SCALE_MAX ||
        !canor_elite_interval_ok(c->bound))
      f |= CANOR_ELITE_F_MALFORMED;
    for (j = 0; j < i; ++j)
      if (canor_elite_name_eq(b->category[j].name, c->name))
        f |= CANOR_ELITE_F_DUPLICATE;
    canor_elite_eval_item(ev, CANOR_ELITE_STAGE_CATEGORY, CANOR_ELITE_NONE,
                          CANOR_ELITE_NONE, i, f);
  }
}

canor_status canor_elite_accord_evaluate(const canor_elite_profile_t *p,
                                         const canor_elite_accord_t *a,
                                         canor_elite_report_t *report,
                                         canor_elite_outcome_t *outcome) {
  canor_elite_eval_t ev;
  bool sub_ok[CANOR_ELITE_SIGNATORY_MAX][CANOR_ELITE_CATEGORY_MAX];
  uint8_t s, t, c, S, C;
  size_t len;
  uint16_t f;

  if (!p || !a || !outcome) return CANOR_ERR_ARGS;
  if (a->signatory_count > CANOR_ELITE_SIGNATORY_MAX ||
      a->basis.vocab_count > CANOR_ELITE_VOCAB_MAX ||
      a->basis.category_count > CANOR_ELITE_CATEGORY_MAX)
    return CANOR_ERR_RANGE;
  memset(&ev, 0, sizeof ev);
  ev.p = p;
  ev.a = a;
  ev.r = report;
  ev.outcome = CANOR_ELITE_ACCORD_ACCEPTED;
  if (report) report->diag_count = 0;
  S = a->signatory_count;
  C = a->basis.category_count;

  canor_elite_eval_accord_level(&ev);
  canor_elite_eval_basis(&ev);

  /* 1. Ratification of the preset basis, by every signatory. */
  for (s = 0; s < S; ++s) {
    if (!a->ratification[s].present) {
      f = CANOR_ELITE_F_PENDING;
    } else if (canor_elite_statement_ratify(a, s, ev.scratch, sizeof ev.scratch,
                                            &len) != CANOR_OK) {
      f = CANOR_ELITE_F_MALFORMED;
    } else {
      f = canor_elite_eval_verify(&ev, &a->signatory[s], len,
                                  &a->ratification[s].signature);
    }
    canor_elite_eval_item(&ev, CANOR_ELITE_STAGE_RATIFY, s, CANOR_ELITE_NONE,
                          CANOR_ELITE_NONE, f);
  }

  /* 2. Per-category submissions: E within A within B. */
  for (s = 0; s < S; ++s) {
    for (c = 0; c < C; ++c) {
      const canor_elite_submission_t *sub = &a->submission[s][c];
      const canor_elite_category_t *cat = &a->basis.category[c];
      f = 0;
      if (!sub->present) {
        f = CANOR_ELITE_F_PENDING;
      } else {
        if (!canor_elite_interval_ok(sub->allowance) ||
            !canor_elite_interval_ok(sub->measured) ||
            !canor_elite_name_ok(sub->method))
          f |= CANOR_ELITE_F_MALFORMED;
        if (!canor_elite_within(sub->allowance, cat->bound))
          f |= CANOR_ELITE_F_OUT_OF_BOUND;
        if (!canor_elite_within(sub->measured, sub->allowance))
          f |= CANOR_ELITE_F_OVER_ALLOWANCE;
        f |= canor_elite_blob_flags(&sub->evidence);
      }
      sub_ok[s][c] = sub->present && !(f & CANOR_ELITE_F_REJECTING);
      canor_elite_eval_item(&ev, CANOR_ELITE_STAGE_SUBMIT, s, CANOR_ELITE_NONE,
                            c, f);
    }
  }

  /* 3. Certification by every OTHER signatory. */
  for (t = 0; t < S; ++t) {
    for (s = 0; s < S; ++s) {
      if (t == s) continue;
      for (c = 0; c < C; ++c) {
        const canor_elite_cert_t *ct = &a->cert[t][s][c];
        f = 0;
        if (ct->verdict == CANOR_ELITE_VERDICT_PENDING) {
          f = CANOR_ELITE_F_PENDING;
        } else if (ct->verdict != CANOR_ELITE_VERDICT_CERTIFIED &&
                   ct->verdict != CANOR_ELITE_VERDICT_REJECTED) {
          f = CANOR_ELITE_F_MALFORMED;
        } else if (!a->submission[s][c].present) {
          f = CANOR_ELITE_F_INCONSISTENT; /* a verdict on nothing */
        } else if (canor_elite_statement_cert(a, t, s, c, ct->verdict, ev.scratch,
                                              sizeof ev.scratch,
                                              &len) != CANOR_OK) {
          f = CANOR_ELITE_F_MALFORMED;
        } else {
          f = canor_elite_eval_verify(&ev, &a->signatory[t], len, &ct->signature);
          if (ct->verdict == CANOR_ELITE_VERDICT_REJECTED)
            f |= CANOR_ELITE_F_REJECTED;
          /* Certifying what fails the bounds is itself a finding. */
          else if (!sub_ok[s][c])
            f |= CANOR_ELITE_F_INCONSISTENT;
        }
        canor_elite_eval_item(&ev, CANOR_ELITE_STAGE_CERTIFY, s, t, c, f);
      }
    }
  }

  /* 4. Approval of the United object, by every signatory. */
  for (s = 0; s < S; ++s) {
    if (!a->approval[s].present) {
      f = CANOR_ELITE_F_PENDING;
    } else if (canor_elite_statement_approve(a, s, ev.scratch, sizeof ev.scratch,
                                             &len) != CANOR_OK) {
      f = CANOR_ELITE_F_MALFORMED;
    } else {
      f = canor_elite_eval_verify(&ev, &a->signatory[s], len,
                                  &a->approval[s].signature);
    }
    canor_elite_eval_item(&ev, CANOR_ELITE_STAGE_APPROVE, s, CANOR_ELITE_NONE,
                          CANOR_ELITE_NONE, f);
  }

  if (report) {
    report->outcome = ev.outcome;
    report->checked = ev.checked;
    report->passed = ev.passed;
  }
  *outcome = ev.outcome;
  return ev.overflow ? CANOR_ERR_FULL : CANOR_OK;
}

/* ===========================================================================
 * Assembly
 * ========================================================================= */

static int canor_elite_word_find(const canor_elite_basis_t *b, const char *token,
                                 size_t n) {
  uint8_t i;
  if (n == 0 || n >= CANOR_ELITE_NAME_MAX) return -1;
  for (i = 0; i < b->vocab_count; ++i)
    if (strncmp(b->vocab[i].token, token, n) == 0 && b->vocab[i].token[n] == '\0')
      return i;
  return -1;
}

static bool canor_elite_contract_active(const canor_elite_profile_t *p,
                                        const char *contract) {
  uint8_t i;
  if (!contract[0]) return false;
  for (i = 0; i < p->contract_count; ++i)
    if (canor_elite_name_eq(p->contract[i], contract)) return true;
  return false;
}

typedef struct {
  size_t pos, len;
  int rule; /* -1: no specification matches */
} canor_elite_site_t;

/* Does spec[0..m) match t at pos without splitting a word on either side? */
static bool canor_elite_match_at(const char *t, size_t n, size_t pos,
                                 const char *spec, size_t m) {
  if (m == 0 || m > n - pos || memcmp(t + pos, spec, m) != 0) return false;
  if (pos > 0 && canor_elite_word((unsigned char)t[pos - 1]) &&
      canor_elite_word((unsigned char)spec[0]))
    return false;
  if (pos + m < n && canor_elite_word((unsigned char)t[pos + m]) &&
      canor_elite_word((unsigned char)spec[m - 1]))
    return false;
  return true;
}

/* L2R: leftmost-longest. R2L: the site that ends last, longest among those. */
static canor_elite_site_t canor_elite_find_site(const canor_elite_program_t *pg,
                                                const char *t) {
  canor_elite_site_t best = {0, 0, -1};
  size_t n = canor_elite_textlen(t), pos;
  uint8_t r;
  for (pos = 0; pos < n; ++pos) {
    for (r = 0; r < pg->rule_count; ++r) {
      size_t m = canor_elite_textlen(pg->rule[r].spec);
      bool take;
      if (!canor_elite_match_at(t, n, pos, pg->rule[r].spec, m)) continue;
      if (best.rule < 0)
        take = true;
      else if (pg->direction == CANOR_ELITE_L2R)
        take = pos < best.pos || (pos == best.pos && m > best.len);
      else
        take = pos + m > best.pos + best.len ||
               (pos + m == best.pos + best.len && m > best.len);
      if (take) {
        best.pos = pos;
        best.len = m;
        best.rule = r;
      }
    }
  }
  return best;
}

/* dst = t with the site replaced by def. RANGE if the result would not fit:
 * a rewrite is never truncated. */
static canor_status canor_elite_rewrite(char *dst, const char *t,
                                        canor_elite_site_t s, const char *def) {
  size_t n = canor_elite_textlen(t), d = canor_elite_textlen(def);
  size_t out = n - s.len + d, tail = n - s.pos - s.len;
  if (out >= CANOR_ELITE_TEXT_MAX) return CANOR_ERR_RANGE;
  memcpy(dst, t, s.pos);
  memcpy(dst + s.pos, def, d);
  memcpy(dst + s.pos + d, t + s.pos + s.len, tail);
  dst[out] = '\0';
  return CANOR_OK;
}

/* Basis checks on a fully rewritten line: `head operand, operand, ...`. */
static uint8_t canor_elite_text_check(const canor_elite_profile_t *p,
                                      const canor_elite_accord_t *accord,
                                      const char *t) {
  const canor_elite_word_t *w;
  size_t n = canor_elite_textlen(t), b = 0, h, i;
  unsigned depth = 0, operands = 0;
  uint8_t f = 0;
  int wi;
  while (b < n && canor_elite_space((unsigned char)t[b])) ++b;
  if (b == n) return CANOR_ELITE_S_EMPTY;
  /* §7: "raw Elite pattern syntax remains permissive" - with no agreed basis
   * there is nothing to check a line against. */
  if (!accord) return CANOR_ELITE_S_UNCHECKED;
  h = b;
  while (h < n && !canor_elite_space((unsigned char)t[h])) ++h;
  i = h;
  while (i < n && canor_elite_space((unsigned char)t[i])) ++i;
  if (i < n) {
    operands = 1;
    for (; i < n; ++i) {
      char c = t[i];
      if (c == '(' || c == '[' || c == '{')
        ++depth;
      else if ((c == ')' || c == ']' || c == '}') && depth)
        --depth;
      else if (c == ',' && !depth)
        ++operands;
    }
  }
  wi = canor_elite_word_find(&accord->basis, t + b, h - b);
  if (wi < 0) return CANOR_ELITE_S_NOT_IN_BASIS;
  w = &accord->basis.vocab[wi];
  if (w->arity != operands) f |= CANOR_ELITE_S_ARITY;
  /* §7: assembly RECORDS the declared effect without performing it. */
  if (w->effect)
    f |= canor_elite_contract_active(p, w->contract)
             ? CANOR_ELITE_S_EFFECT_RECORDED
             : CANOR_ELITE_S_EFFECT_UNAUTHORIZED;
  return f;
}

typedef struct {
  const canor_elite_profile_t *p;
  const canor_elite_accord_t *accord;
  const canor_elite_program_t *pg;
  canor_elite_leaf_t *leaf; /* this line's interpretations */
  uint8_t count;
  bool more;
  uint8_t path[CANOR_ELITE_DEPTH_MAX];
  char text[CANOR_ELITE_DEPTH_MAX + 1][CANOR_ELITE_TEXT_MAX];
} canor_elite_expand_t;

static void canor_elite_emit(canor_elite_expand_t *x, uint8_t depth,
                             uint8_t flags) {
  canor_elite_leaf_t *l;
  if (x->count >= CANOR_ELITE_EXPANSION_MAX) {
    x->more = true; /* past the configured limit: reported, not hidden */
    return;
  }
  l = &x->leaf[x->count++];
  memset(l, 0, sizeof *l);
  memcpy(l->path, x->path, depth);
  l->depth = depth;
  l->flags = flags;
}

/* Depth-first over rewrite choices, so interpretations come out in the
 * lexicographic order of their paths. With one level of rules that is the
 * plain mixed-radix order. Recursion depth is bounded by DEPTH_MAX + 1. */
static void canor_elite_expand(canor_elite_expand_t *x, uint8_t level) {
  canor_elite_site_t s;
  const canor_elite_rule_t *r;
  uint8_t k;
  if (x->more) return;
  s = canor_elite_find_site(x->pg, x->text[level]);
  if (s.rule < 0) {
    canor_elite_emit(x, level,
                     canor_elite_text_check(x->p, x->accord, x->text[level]));
    return;
  }
  if (level == CANOR_ELITE_DEPTH_MAX) {
    canor_elite_emit(x, level,
                     (uint8_t)(CANOR_ELITE_S_DEPTH |
                               canor_elite_text_check(x->p, x->accord,
                                                      x->text[level])));
    return;
  }
  r = &x->pg->rule[s.rule];
  for (k = 0; k < r->def_count && !x->more; ++k) {
    x->path[level] = k;
    if (canor_elite_rewrite(x->text[level + 1], x->text[level], s, r->def[k]) !=
        CANOR_OK)
      canor_elite_emit(x, (uint8_t)(level + 1), CANOR_ELITE_S_TOO_LONG);
    else
      canor_elite_expand(x, (uint8_t)(level + 1));
  }
}

canor_status canor_elite_leaf_text(const canor_elite_program_t *p, uint8_t line,
                                   const canor_elite_leaf_t *leaf, char *out) {
  char buf[2][CANOR_ELITE_TEXT_MAX];
  size_t n;
  uint8_t d;
  int cur = 0;
  if (!p || !leaf || !out) return CANOR_ERR_ARGS;
  if (line >= p->line_count || line >= CANOR_ELITE_INSN_MAX ||
      leaf->depth > CANOR_ELITE_DEPTH_MAX)
    return CANOR_ERR_RANGE;
  n = canor_elite_textlen(p->line[line].text);
  if (n >= CANOR_ELITE_TEXT_MAX) return CANOR_ERR_RANGE;
  memcpy(buf[0], p->line[line].text, n + 1);
  for (d = 0; d < leaf->depth; ++d) {
    canor_elite_site_t s = canor_elite_find_site(p, buf[cur]);
    if (s.rule < 0 || leaf->path[d] >= p->rule[s.rule].def_count)
      return CANOR_ERR_RANGE; /* the leaf is not from this program */
    if (canor_elite_rewrite(buf[1 - cur], buf[cur], s,
                            p->rule[s.rule].def[leaf->path[d]]) != CANOR_OK)
      break; /* TOO_LONG: the last text that fitted */
    cur = 1 - cur;
  }
  memcpy(out, buf[cur], canor_elite_textlen(buf[cur]) + 1);
  return CANOR_OK;
}

canor_status canor_elite_assemble(const canor_elite_profile_t *p,
                                  const canor_elite_accord_t *accord,
                                  const canor_elite_program_t *program,
                                  uint32_t limit, canor_elite_assembly_t *out) {
  canor_elite_expand_t x;
  uint64_t radix[CANOR_ELITE_INSN_MAX];
  uint64_t total = 1, consumed, k;
  canor_elite_record_t *rec;
  bool overflow = false;
  uint8_t i, n;

  if (!p || !program || !out) return CANOR_ERR_ARGS;
  if (limit == 0) return CANOR_ERR_RANGE;
  if (limit > CANOR_ELITE_BRANCH_MAX) return CANOR_ERR_FULL;
  n = program->line_count;
  if (n == 0 || n > CANOR_ELITE_INSN_MAX ||
      program->rule_count > CANOR_ELITE_RULE_MAX ||
      (program->direction != CANOR_ELITE_L2R &&
       program->direction != CANOR_ELITE_R2L))
    return CANOR_ERR_RANGE;
  for (i = 0; i < program->rule_count; ++i) {
    size_t m = canor_elite_textlen(program->rule[i].spec);
    uint8_t d;
    if (m == 0 || m >= CANOR_ELITE_TEXT_MAX || program->rule[i].def_count == 0 ||
        program->rule[i].def_count > CANOR_ELITE_ALT_MAX)
      return CANOR_ERR_RANGE;
    for (d = 0; d < program->rule[i].def_count; ++d)
      if (canor_elite_textlen(program->rule[i].def[d]) >= CANOR_ELITE_TEXT_MAX)
        return CANOR_ERR_RANGE;
  }
  for (i = 0; i < n; ++i)
    if (canor_elite_textlen(program->line[i].text) >= CANOR_ELITE_TEXT_MAX)
      return CANOR_ERR_RANGE;

  memset(out, 0, sizeof *out);
  rec = &out->record;
  rec->selected = CANOR_ELITE_NO_BRANCH;
  rec->configured = limit;
  if (accord) {
    canor_status st;
    rec->basis_bound = true;
    st = canor_elite_accord_evaluate(p, accord, NULL, &rec->basis_outcome);
    if (st != CANOR_OK) return st;
  }

  /* Every line's interpretations, each a digit of the program's mixed radix.
   * A line that no specification matches has exactly one. */
  for (i = 0; i < n; ++i) {
    memset(&x, 0, sizeof x);
    x.p = p;
    x.accord = accord;
    x.pg = program;
    x.leaf = out->leaf[i];
    memcpy(x.text[0], program->line[i].text,
           canor_elite_textlen(program->line[i].text) + 1);
    canor_elite_expand(&x, 0);
    out->leaf_count[i] = x.count; /* >= 1: the first emit always fits */
    out->leaf_more[i] = x.more;
    if (x.more) rec->expansion_limited = true;
    radix[i] = x.count;
    if (!overflow) {
      if (total > UINT64_MAX / radix[i])
        overflow = true;
      else
        total *= radix[i];
    }
  }
  rec->total_overflow = overflow;
  rec->total = overflow ? 0 : total;
  consumed = (!overflow && total <= limit) ? total : limit;
  rec->consumed = consumed;
  rec->termination =
      (!overflow && !rec->expansion_limited && consumed == total)
          ? CANOR_ELITE_TERM_EXHAUSTED
          : CANOR_ELITE_TERM_LIMIT;
  rec->complete = rec->termination == CANOR_ELITE_TERM_EXHAUSTED;

  /* The FIRST-traversed line is the most significant digit: L2R traverses
   * line 0 first, R2L the last line first. */
  for (k = 0; k < consumed; ++k) {
    canor_elite_branch_t *br = &out->branch[k];
    uint64_t rem = k;
    uint8_t step;
    br->id = k;
    for (step = 0; step < n; ++step) {
      uint8_t ln =
          program->direction == CANOR_ELITE_L2R ? (uint8_t)(n - 1 - step) : step;
      br->choice[ln] = (uint8_t)(rem % radix[ln]);
      rem /= radix[ln];
    }
    for (i = 0; i < n; ++i) {
      uint8_t f = out->leaf[i][br->choice[i]].flags;
      br->site_flags[i] = f;
      br->flags |= f;
      if (f & CANOR_ELITE_S_EFFECT_RECORDED) ++rec->effects_declared;
    }
    br->candidate = !(br->flags & CANOR_ELITE_S_ERRORS);
    if (br->candidate)
      ++rec->candidates;
    else
      ++rec->diagnosed;
  }
  out->branch_count = (uint16_t)consumed;
  return canor_elite_select(out);
}

canor_status canor_elite_tool_declare(canor_elite_assembly_t *as,
                                      const char *name, const char *version,
                                      canor_elite_tool_kind_t kind,
                                      const canor_elite_blob_t *config,
                                      uint64_t seed) {
  canor_elite_tool_t *t;
  canor_status st;
  if (!as) return CANOR_ERR_ARGS;
  if (as->tool_count >= CANOR_ELITE_TOOL_MAX) return CANOR_ERR_FULL;
  /* int casts: the enum's underlying type may be unsigned (-Wtype-limits). */
  if ((int)kind < (int)CANOR_ELITE_TOOL_AGENT ||
      (int)kind > (int)CANOR_ELITE_TOOL_OTHER)
    return CANOR_ERR_RANGE;
  t = &as->tool[as->tool_count];
  memset(t, 0, sizeof *t);
  st = canor_elite_name_copy(t->name, name);
  if (st != CANOR_OK) return st;
  st = canor_elite_name_copy(t->version, version);
  if (st != CANOR_OK) return st;
  st = canor_elite_blob_copy(&t->config, config);
  if (st != CANOR_OK) return st;
  t->kind = kind;
  t->seed = seed;
  ++as->tool_count;
  return CANOR_OK;
}

canor_status canor_elite_select(canor_elite_assembly_t *as) {
  int64_t best = CANOR_ELITE_NO_BRANCH;
  uint16_t i;
  if (!as) return CANOR_ERR_ARGS;
  for (i = 0; i < as->branch_count; ++i) {
    const canor_elite_branch_t *b = &as->branch[i];
    if (!b->candidate) continue;
    if (best == CANOR_ELITE_NO_BRANCH) {
      best = i;
    } else {
      const canor_elite_branch_t *bb = &as->branch[best];
      /* strictly greater only: ties keep the lower branch id */
      if (b->scored && (!bb->scored || b->score > bb->score)) best = i;
    }
  }
  as->record.selected = best;
  return CANOR_OK;
}

canor_status canor_elite_score(canor_elite_assembly_t *as, uint16_t pos,
                               int32_t score) {
  if (!as) return CANOR_ERR_ARGS;
  if (pos >= as->branch_count || score < 0 || score > CANOR_ONE)
    return CANOR_ERR_RANGE;
  as->branch[pos].scored = true;
  as->branch[pos].score = score;
  return canor_elite_select(as);
}

static uint64_t canor_elite_splitmix64(uint64_t x) {
  uint64_t z = x + CANOR_ELITE_SPLITMIX_GAMMA;
  z = (z ^ (z >> 30)) * CANOR_ELITE_SPLITMIX_MUL1;
  z = (z ^ (z >> 27)) * CANOR_ELITE_SPLITMIX_MUL2;
  return z ^ (z >> 31);
}

canor_status canor_elite_score_seeded(canor_elite_assembly_t *as,
                                      uint64_t seed) {
  uint8_t i;
  uint16_t k;
  bool declared = false;
  if (!as) return CANOR_ERR_ARGS;
  /* Tool identity and seed must be on record before its outputs count. */
  for (i = 0; i < as->tool_count; ++i)
    if (as->tool[i].kind == CANOR_ELITE_TOOL_STOCHASTIC && as->tool[i].seed == seed)
      declared = true;
  if (!declared) return CANOR_ERR_NOT_FOUND;
  for (k = 0; k < as->branch_count; ++k) {
    as->branch[k].scored = true;
    as->branch[k].score = (int32_t)(canor_elite_splitmix64(seed ^ as->branch[k].id) %
                                    (uint64_t)(CANOR_ONE + 1));
  }
  return canor_elite_select(as);
}

/* ===========================================================================
 * Names for records and logs
 * ========================================================================= */

const char *canor_elite_status_name(canor_status s) {
  switch (s) {
    case CANOR_OK: return "CANOR_OK";
    case CANOR_ERR_ARGS: return "CANOR_ERR_ARGS";
    case CANOR_ERR_RANGE: return "CANOR_ERR_RANGE";
    case CANOR_ERR_FULL: return "CANOR_ERR_FULL";
    case CANOR_ERR_NOT_CLOSED: return "CANOR_ERR_NOT_CLOSED";
    case CANOR_ERR_UNDEFINED: return "CANOR_ERR_UNDEFINED";
    case CANOR_ERR_NOT_FOUND: return "CANOR_ERR_NOT_FOUND";
  }
  return "CANOR_ERR_UNKNOWN";
}

const char *canor_elite_outcome_name(canor_elite_outcome_t o) {
  switch (o) {
    case CANOR_ELITE_ACCORD_ACCEPTED: return "accepted";
    case CANOR_ELITE_ACCORD_INCOMPLETE: return "incomplete";
    case CANOR_ELITE_ACCORD_UNDEFINED: return "undefined";
    case CANOR_ELITE_ACCORD_REJECTED: return "rejected";
  }
  return "unknown";
}

static const char *canor_elite_role_name(canor_elite_role_t r) {
  switch (r) {
    case CANOR_ELITE_ROLE_USER: return "user";
    case CANOR_ELITE_ROLE_APP: return "app";
    case CANOR_ELITE_ROLE_AGENT: return "agent";
  }
  return "unknown";
}

static const char *canor_elite_tool_kind_name(canor_elite_tool_kind_t k) {
  switch (k) {
    case CANOR_ELITE_TOOL_AGENT: return "agent";
    case CANOR_ELITE_TOOL_LANGUAGE_MODEL: return "language-model";
    case CANOR_ELITE_TOOL_STOCHASTIC: return "stochastic";
    case CANOR_ELITE_TOOL_PROCEDURAL: return "procedural";
    case CANOR_ELITE_TOOL_OTHER: return "other";
  }
  return "unknown";
}

static const char *const canor_elite_stage_names[] = {
    "accord", "vocabulary", "category", "ratify", "submit", "certify", "approve"};

static const char *const canor_elite_item_names[CANOR_ELITE_ITEM_COUNT] = {
    "signatories",    "digest-algorithm", "signature-algorithm",
    "object-digest",  "provenance",       "basis-digest",
    "vocabulary",     "categories"};

/* Bit i of an accord flag word. */
static const char *const canor_elite_flag_names[] = {
    "pending",        "signature-invalid", "signature-undefined", "rejected",
    "out-of-bound",   "over-allowance",    "malformed",           "inconsistent",
    "duplicate",      "unset",             "digest-mismatch",     "too-few"};

/* Bit i of a site flag word. */
static const char *const canor_elite_site_flag_names[] = {
    "not-in-basis", "arity",    "effect-unauthorized", "effect-recorded",
    "depth-limit",  "too-long", "unchecked",           "empty"};

static const char *const canor_elite_parse_kind_names[] = {
    "empty-spec", "too-long", "full", "malformed"};

/* Explicit open items carried in every record (Z §13, WD v1 §7, §9). */
static const char *const canor_elite_open_items[] = {
    "candidate-ordering schema (Z s13); declared: lexicographic over rewrite "
    "paths, first-traversed line most significant",
    "tool determinism guarantees (Z s13)",
    "resource accounting and traversal-limit policy (Z s13)",
    "signature lifecycle, trust roots, revocation (Z s7, s12)",
    "governance-record retention (Z s13)",
    "Aether s9 latency/jitter field per branch traversal: not linked",
    "freeform rules, declared readings: redefinitions accumulate; word-boundary "
    "leftmost-longest (L2R) / rightmost-longest (R2L) matching"};

/* ===========================================================================
 * JSON
 *
 * Once a chunk does not fit, nothing more is written (so the buffer never
 * holds a spliced document), but the length keeps counting so the caller
 * learns the size required.
 * ========================================================================= */

typedef struct {
  char *buf;
  size_t cap;
  size_t len;
  bool full;
} canor_elite_json_t;

static void canor_elite_jput(canor_elite_json_t *j, const char *s, size_t n) {
  /* keep one byte for the terminating NUL */
  if (!j->full && j->buf && j->len < j->cap && n < j->cap - j->len)
    memcpy(j->buf + j->len, s, n);
  else
    j->full = true;
  j->len += n;
}

static void canor_elite_jraw(canor_elite_json_t *j, const char *s) {
  canor_elite_jput(j, s, strlen(s));
}

static void canor_elite_ju64(canor_elite_json_t *j, uint64_t v) {
  char d[24];
  size_t n = sizeof d;
  do {
    d[--n] = (char)('0' + (int)(v % 10u));
    v /= 10u;
  } while (v);
  canor_elite_jput(j, d + n, sizeof d - n);
}

static void canor_elite_ji64(canor_elite_json_t *j, int64_t v) {
  if (v < 0) {
    canor_elite_jraw(j, "-");
    /* -(v + 1) + 1 avoids negating INT64_MIN */
    canor_elite_ju64(j, (uint64_t)(-(v + 1)) + 1u);
  } else {
    canor_elite_ju64(j, (uint64_t)v);
  }
}

/* Escaped string body of exactly n bytes. */
static void canor_elite_jesc_n(canor_elite_json_t *j, const char *s, size_t n) {
  static const char hex[] = "0123456789abcdef";
  size_t i;
  for (i = 0; i < n; ++i) {
    unsigned char ch = (unsigned char)s[i];
    if (ch == '"' || ch == '\\') {
      char e[2];
      e[0] = '\\';
      e[1] = (char)ch;
      canor_elite_jput(j, e, 2);
    } else if (ch < 0x20u) {
      char e[6] = {'\\', 'u', '0', '0', 0, 0};
      e[4] = hex[ch >> 4];
      e[5] = hex[ch & 0xFu];
      canor_elite_jput(j, e, 6);
    } else {
      canor_elite_jput(j, (const char *)&s[i], 1);
    }
  }
}

/* A name held in a fixed array: bounded by the name capacity, so a
 * hand-filled array with no terminator cannot run past its end. */
static void canor_elite_jesc(canor_elite_json_t *j, const char *s) {
  canor_elite_jesc_n(j, s, canor_elite_strlen(s));
}

static void canor_elite_jstr(canor_elite_json_t *j, const char *s) {
  canor_elite_jraw(j, "\"");
  canor_elite_jesc(j, s);
  canor_elite_jraw(j, "\"");
}

/* Freeform Elite text (spec, definition, line), bounded by its capacity. */
static void canor_elite_jtext(canor_elite_json_t *j, const char *s) {
  canor_elite_jraw(j, "\"");
  canor_elite_jesc_n(j, s, canor_elite_textlen(s));
  canor_elite_jraw(j, "\"");
}

/* A string constant from this file: its full length, never cut to a name. */
static void canor_elite_jlit(canor_elite_json_t *j, const char *s) {
  canor_elite_jraw(j, "\"");
  canor_elite_jesc_n(j, s, strlen(s));
  canor_elite_jraw(j, "\"");
}

/* "" prints as null: an unset optional name. */
static void canor_elite_jstr_opt(canor_elite_json_t *j, const char *s) {
  if (!s[0])
    canor_elite_jraw(j, "null");
  else
    canor_elite_jstr(j, s);
}

static void canor_elite_jkey(canor_elite_json_t *j, const char *k) {
  canor_elite_jlit(j, k);
  canor_elite_jraw(j, ":");
}

static void canor_elite_jbool(canor_elite_json_t *j, bool b) {
  canor_elite_jraw(j, b ? "true" : "false");
}

/* Lowercase hex, or null when UNSET. */
static void canor_elite_jhex(canor_elite_json_t *j, const canor_elite_blob_t *b) {
  static const char hex[] = "0123456789abcdef";
  uint8_t i, n;
  if (b->len == 0) {
    canor_elite_jraw(j, "null");
    return;
  }
  n = b->len > CANOR_ELITE_BLOB_MAX ? CANOR_ELITE_BLOB_MAX : b->len;
  canor_elite_jraw(j, "\"");
  for (i = 0; i < n; ++i) {
    char e[2];
    e[0] = hex[b->bytes[i] >> 4];
    e[1] = hex[b->bytes[i] & 0xFu];
    canor_elite_jput(j, e, 2);
  }
  canor_elite_jraw(j, "\"");
}

static void canor_elite_jinterval(canor_elite_json_t *j, canor_elite_interval_t i) {
  canor_elite_jraw(j, "[");
  canor_elite_ji64(j, i.lo);
  canor_elite_jraw(j, ",");
  canor_elite_ji64(j, i.hi);
  canor_elite_jraw(j, "]");
}

static void canor_elite_jflags(canor_elite_json_t *j, unsigned flags,
                               const char *const *names, size_t count) {
  size_t i;
  bool first = true;
  canor_elite_jraw(j, "[");
  for (i = 0; i < count; ++i) {
    if (!(flags & (1u << i))) continue;
    if (!first) canor_elite_jraw(j, ",");
    canor_elite_jlit(j, names[i]);
    first = false;
  }
  canor_elite_jraw(j, "]");
}

static void canor_elite_jindex(canor_elite_json_t *j, uint8_t v) {
  if (v == CANOR_ELITE_NONE)
    canor_elite_jraw(j, "null");
  else
    canor_elite_ju64(j, v);
}

static canor_status canor_elite_jfinish(canor_elite_json_t *j, size_t *len) {
  *len = j->len;
  if (j->buf && j->cap > 0) j->buf[j->full ? j->cap - 1 : j->len] = '\0';
  return j->full ? CANOR_ERR_FULL : CANOR_OK;
}

static void canor_elite_json_profile(canor_elite_json_t *j,
                                     const canor_elite_profile_t *p) {
  uint8_t i;
  canor_elite_jkey(j, "profile");
  canor_elite_jraw(j, "{");
  canor_elite_jkey(j, "name");
  canor_elite_jstr(j, p->name);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "version");
  canor_elite_ju64(j, p->version);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "settings");
  canor_elite_jraw(j, "[");
  for (i = 0; i < p->settings_count && i < CANOR_ELITE_SETTINGS_MAX; ++i) {
    if (i) canor_elite_jraw(j, ",");
    canor_elite_jraw(j, "{");
    canor_elite_jkey(j, "name");
    canor_elite_jstr(j, p->settings[i].name);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "version");
    canor_elite_ju64(j, p->settings[i].version);
    canor_elite_jraw(j, "}");
  }
  canor_elite_jraw(j, "],");
  canor_elite_jkey(j, "contracts");
  canor_elite_jraw(j, "[");
  for (i = 0; i < p->contract_count && i < CANOR_ELITE_CONTRACT_MAX; ++i) {
    if (i) canor_elite_jraw(j, ",");
    canor_elite_jstr(j, p->contract[i]);
  }
  canor_elite_jraw(j, "],");
  canor_elite_jkey(j, "digest_algorithm");
  canor_elite_jstr_opt(j, p->digest_algorithm);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "signature_algorithm");
  canor_elite_jstr_opt(j, p->signature_algorithm);
  canor_elite_jraw(j, "}");
}

static void canor_elite_json_basis(canor_elite_json_t *j,
                                   const canor_elite_accord_t *a,
                                   canor_elite_outcome_t outcome) {
  const canor_elite_basis_t *b = &a->basis;
  uint8_t i;
  canor_elite_jkey(j, "basis");
  canor_elite_jraw(j, "{");
  canor_elite_jkey(j, "name");
  canor_elite_jstr(j, b->name);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "version");
  canor_elite_ju64(j, b->version);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "digest");
  canor_elite_jhex(j, &b->digest);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "outcome");
  canor_elite_jlit(j, canor_elite_outcome_name(outcome));
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "object");
  canor_elite_jraw(j, "{");
  canor_elite_jkey(j, "digest");
  canor_elite_jhex(j, &a->object_digest);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "version");
  canor_elite_ju64(j, a->object_version);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "provenance");
  canor_elite_jhex(j, &a->provenance);
  canor_elite_jraw(j, "},");
  canor_elite_jkey(j, "signatories");
  canor_elite_jraw(j, "[");
  for (i = 0; i < a->signatory_count && i < CANOR_ELITE_SIGNATORY_MAX; ++i) {
    const canor_elite_signatory_t *s = &a->signatory[i];
    if (i) canor_elite_jraw(j, ",");
    canor_elite_jraw(j, "{");
    canor_elite_jkey(j, "name");
    canor_elite_jstr(j, s->name);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "role");
    canor_elite_jlit(j, canor_elite_role_name(s->role));
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "external");
    canor_elite_jbool(j, s->external);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "profile");
    canor_elite_jhex(j, &s->profile);
    canor_elite_jraw(j, "}");
  }
  canor_elite_jraw(j, "],");
  canor_elite_jkey(j, "vocabulary");
  canor_elite_jraw(j, "[");
  for (i = 0; i < b->vocab_count && i < CANOR_ELITE_VOCAB_MAX; ++i) {
    const canor_elite_word_t *w = &b->vocab[i];
    if (i) canor_elite_jraw(j, ",");
    canor_elite_jraw(j, "{");
    canor_elite_jkey(j, "token");
    canor_elite_jstr(j, w->token);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "arity");
    canor_elite_ju64(j, w->arity);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "effect");
    canor_elite_jbool(j, w->effect);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "contract");
    canor_elite_jstr_opt(j, w->contract);
    canor_elite_jraw(j, "}");
  }
  canor_elite_jraw(j, "],");
  canor_elite_jkey(j, "categories");
  canor_elite_jraw(j, "[");
  for (i = 0; i < b->category_count && i < CANOR_ELITE_CATEGORY_MAX; ++i) {
    const canor_elite_category_t *c = &b->category[i];
    if (i) canor_elite_jraw(j, ",");
    canor_elite_jraw(j, "{");
    canor_elite_jkey(j, "name");
    canor_elite_jstr(j, c->name);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "metric");
    canor_elite_jstr(j, c->metric);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "scale");
    canor_elite_ji64(j, c->scale);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "bound");
    canor_elite_jinterval(j, c->bound);
    canor_elite_jraw(j, "}");
  }
  canor_elite_jraw(j, "]}");
}

static void canor_elite_json_branch(canor_elite_json_t *j,
                                    const canor_elite_program_t *pg,
                                    const canor_elite_assembly_t *as,
                                    const canor_elite_branch_t *br) {
  char text[CANOR_ELITE_TEXT_MAX];
  uint8_t i, d;
  canor_elite_jraw(j, "{");
  canor_elite_jkey(j, "id");
  canor_elite_ju64(j, br->id);
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "status");
  canor_elite_jlit(j, br->candidate ? "candidate" : "diagnosed");
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "score");
  if (br->scored)
    canor_elite_ji64(j, br->score);
  else
    canor_elite_jraw(j, "null");
  canor_elite_jraw(j, ",");
  canor_elite_jkey(j, "trace");
  canor_elite_jraw(j, "[");
  for (i = 0; i < pg->line_count && i < CANOR_ELITE_INSN_MAX; ++i) {
    const canor_elite_leaf_t *lf = &as->leaf[i][br->choice[i]];
    if (i) canor_elite_jraw(j, ",");
    canor_elite_jraw(j, "{");
    canor_elite_jkey(j, "line");
    canor_elite_ju64(j, pg->line[i].source_line);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "insn");
    if (canor_elite_leaf_text(pg, i, lf, text) == CANOR_OK)
      canor_elite_jtext(j, text);
    else
      canor_elite_jraw(j, "null");
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "leaf");
    canor_elite_ju64(j, br->choice[i]);
    canor_elite_jraw(j, ",");
    canor_elite_jkey(j, "path");
    canor_elite_jraw(j, "[");
    for (d = 0; d < lf->depth && d < CANOR_ELITE_DEPTH_MAX; ++d) {
      if (d) canor_elite_jraw(j, ",");
      canor_elite_ju64(j, lf->path[d]);
    }
    canor_elite_jraw(j, "],");
    canor_elite_jkey(j, "flags");
    canor_elite_jflags(j, br->site_flags[i], canor_elite_site_flag_names,
                       sizeof canor_elite_site_flag_names /
                           sizeof canor_elite_site_flag_names[0]);
    canor_elite_jraw(j, "}");
  }
  canor_elite_jraw(j, "]}");
}

canor_status canor_elite_record_json(const canor_elite_profile_t *p,
                                     const canor_elite_accord_t *accord,
                                     const canor_elite_program_t *program,
                                     const canor_elite_assembly_t *as, char *buf,
                                     size_t cap, size_t *len) {
  canor_elite_json_t j;
  const canor_elite_record_t *rec;
  size_t i;
  uint16_t k;
  if (!p || !program || !as || !len || (!buf && cap)) return CANOR_ERR_ARGS;
  if (program->line_count > CANOR_ELITE_INSN_MAX ||
      program->rule_count > CANOR_ELITE_RULE_MAX ||
      as->branch_count > CANOR_ELITE_BRANCH_MAX ||
      as->tool_count > CANOR_ELITE_TOOL_MAX)
    return CANOR_ERR_RANGE;
  /* Every branch must index a stored interpretation of every line. */
  for (k = 0; k < as->branch_count; ++k)
    for (i = 0; i < program->line_count; ++i)
      if (as->branch[k].choice[i] >= as->leaf_count[i] ||
          as->leaf_count[i] > CANOR_ELITE_EXPANSION_MAX)
        return CANOR_ERR_RANGE;
  rec = &as->record;
  j.buf = buf;
  j.cap = cap;
  j.len = 0;
  j.full = false;

  canor_elite_jraw(&j, "{");
  canor_elite_jkey(&j, "canor");
  canor_elite_jlit(&j, "elite.record");
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "layout");
  canor_elite_ju64(&j, 1);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "spec");
  canor_elite_jlit(&j, "Canor WD v1 s7; Z-Notation Draft s7, s8");
  canor_elite_jraw(&j, ",");
  canor_elite_json_profile(&j, p);
  canor_elite_jraw(&j, ",");
  if (accord && rec->basis_bound) {
    canor_elite_json_basis(&j, accord, rec->basis_outcome);
  } else {
    canor_elite_jkey(&j, "basis");
    canor_elite_jraw(&j, "null");
  }
  canor_elite_jraw(&j, ",");

  canor_elite_jkey(&j, "traversal");
  canor_elite_jraw(&j, "{");
  canor_elite_jkey(&j, "direction");
  canor_elite_jlit(&j, program->direction == CANOR_ELITE_R2L ? "R2L" : "L2R");
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "configured");
  canor_elite_ju64(&j, rec->configured);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "consumed");
  canor_elite_ju64(&j, rec->consumed);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "total");
  if (rec->total_overflow || rec->expansion_limited)
    canor_elite_jraw(&j, "null"); /* not known exactly */
  else
    canor_elite_ju64(&j, rec->total);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "expansion_limited");
  canor_elite_jbool(&j, rec->expansion_limited);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "termination");
  canor_elite_jlit(&j, rec->termination == CANOR_ELITE_TERM_EXHAUSTED ? "exhausted"
                                                                      : "limit");
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "complete");
  canor_elite_jbool(&j, rec->complete);
  canor_elite_jraw(&j, "},");

  canor_elite_jkey(&j, "tools");
  canor_elite_jraw(&j, "[");
  for (k = 0; k < as->tool_count; ++k) {
    const canor_elite_tool_t *t = &as->tool[k];
    if (k) canor_elite_jraw(&j, ",");
    canor_elite_jraw(&j, "{");
    canor_elite_jkey(&j, "name");
    canor_elite_jstr(&j, t->name);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "version");
    canor_elite_jstr(&j, t->version);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "kind");
    canor_elite_jlit(&j, canor_elite_tool_kind_name(t->kind));
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "config");
    canor_elite_jhex(&j, &t->config);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "seed");
    canor_elite_ju64(&j, t->seed);
    canor_elite_jraw(&j, "}");
  }
  canor_elite_jraw(&j, "],");

  canor_elite_jkey(&j, "rules");
  canor_elite_jraw(&j, "[");
  for (k = 0; k < program->rule_count; ++k) {
    const canor_elite_rule_t *r = &program->rule[k];
    uint8_t m;
    if (k) canor_elite_jraw(&j, ",");
    canor_elite_jraw(&j, "{");
    canor_elite_jkey(&j, "spec");
    canor_elite_jtext(&j, r->spec);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "definitions");
    canor_elite_jraw(&j, "[");
    for (m = 0; m < r->def_count && m < CANOR_ELITE_ALT_MAX; ++m) {
      if (m) canor_elite_jraw(&j, ",");
      canor_elite_jraw(&j, "{");
      canor_elite_jkey(&j, "text");
      canor_elite_jtext(&j, r->def[m]);
      canor_elite_jraw(&j, ",");
      canor_elite_jkey(&j, "line");
      canor_elite_ju64(&j, r->def_line[m]);
      canor_elite_jraw(&j, "}");
    }
    canor_elite_jraw(&j, "]}");
  }
  canor_elite_jraw(&j, "],");

  canor_elite_jkey(&j, "program");
  canor_elite_jraw(&j, "[");
  for (k = 0; k < program->line_count; ++k) {
    if (k) canor_elite_jraw(&j, ",");
    canor_elite_jraw(&j, "{");
    canor_elite_jkey(&j, "line");
    canor_elite_ju64(&j, program->line[k].source_line);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "text");
    canor_elite_jtext(&j, program->line[k].text);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "interpretations");
    canor_elite_ju64(&j, as->leaf_count[k]);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "more");
    canor_elite_jbool(&j, as->leaf_more[k]);
    canor_elite_jraw(&j, "}");
  }
  canor_elite_jraw(&j, "],");

  /* Every parse problem, with how many there were past the stored ones. */
  canor_elite_jkey(&j, "parse");
  canor_elite_jraw(&j, "{");
  canor_elite_jkey(&j, "total");
  canor_elite_ju64(&j, program->diag_total);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "diagnostics");
  canor_elite_jraw(&j, "[");
  for (k = 0; k < program->diag_count && k < CANOR_ELITE_PARSE_DIAG_MAX; ++k) {
    const canor_elite_parse_diag_t *pd = &program->diag[k];
    if (k) canor_elite_jraw(&j, ",");
    canor_elite_jraw(&j, "{");
    canor_elite_jkey(&j, "line");
    canor_elite_ju64(&j, pd->line);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "status");
    canor_elite_jlit(&j, canor_elite_status_name((canor_status)pd->status));
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "kind");
    canor_elite_jlit(&j, pd->kind < sizeof canor_elite_parse_kind_names /
                                        sizeof canor_elite_parse_kind_names[0]
                             ? canor_elite_parse_kind_names[pd->kind]
                             : "unknown");
    canor_elite_jraw(&j, "}");
  }
  canor_elite_jraw(&j, "]},");

  canor_elite_jkey(&j, "branches");
  canor_elite_jraw(&j, "[");
  for (k = 0; k < as->branch_count; ++k) {
    if (k) canor_elite_jraw(&j, ",");
    canor_elite_json_branch(&j, program, as, &as->branch[k]);
  }
  canor_elite_jraw(&j, "],");

  canor_elite_jkey(&j, "selected");
  if (rec->selected == CANOR_ELITE_NO_BRANCH)
    canor_elite_jraw(&j, "null");
  else
    canor_elite_ji64(&j, rec->selected);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "counts");
  canor_elite_jraw(&j, "{");
  canor_elite_jkey(&j, "candidates");
  canor_elite_ju64(&j, rec->candidates);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "diagnosed");
  canor_elite_ju64(&j, rec->diagnosed);
  canor_elite_jraw(&j, "},");
  /* §7: assembly records the declared effect without performing it. */
  canor_elite_jkey(&j, "effects");
  canor_elite_jraw(&j, "{");
  canor_elite_jkey(&j, "declared");
  canor_elite_ju64(&j, rec->effects_declared);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "performed");
  canor_elite_ju64(&j, 0);
  canor_elite_jraw(&j, "},");

  canor_elite_jkey(&j, "open_items");
  canor_elite_jraw(&j, "[");
  for (i = 0; i < sizeof canor_elite_open_items / sizeof canor_elite_open_items[0];
       ++i) {
    if (i) canor_elite_jraw(&j, ",");
    canor_elite_jlit(&j, canor_elite_open_items[i]);
  }
  if (!p->digest_algorithm[0]) {
    canor_elite_jraw(&j, ",");
    canor_elite_jlit(&j, "digest algorithm unset (SCHEMA.md, United)");
  }
  if (!p->signature_algorithm[0]) {
    canor_elite_jraw(&j, ",");
    canor_elite_jlit(&j, "signature algorithm unset (Z s7, s12)");
  }
  canor_elite_jraw(&j, "]}");
  return canor_elite_jfinish(&j, len);
}

canor_status canor_elite_report_json(const canor_elite_accord_t *a,
                                     const canor_elite_report_t *r, char *buf,
                                     size_t cap, size_t *len) {
  canor_elite_json_t j;
  uint16_t i;
  if (!a || !r || !len || (!buf && cap)) return CANOR_ERR_ARGS;
  if (r->diag_count > CANOR_ELITE_DIAG_MAX) return CANOR_ERR_RANGE;
  j.buf = buf;
  j.cap = cap;
  j.len = 0;
  j.full = false;
  canor_elite_jraw(&j, "{");
  canor_elite_jkey(&j, "canor");
  canor_elite_jlit(&j, "elite.accord.report");
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "basis");
  canor_elite_jstr(&j, a->basis.name);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "outcome");
  canor_elite_jlit(&j, canor_elite_outcome_name(r->outcome));
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "checked");
  canor_elite_ju64(&j, r->checked);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "passed");
  canor_elite_ju64(&j, r->passed);
  canor_elite_jraw(&j, ",");
  canor_elite_jkey(&j, "diagnostics");
  canor_elite_jraw(&j, "[");
  for (i = 0; i < r->diag_count; ++i) {
    const canor_elite_diag_t *d = &r->diag[i];
    if (i) canor_elite_jraw(&j, ",");
    canor_elite_jraw(&j, "{");
    canor_elite_jkey(&j, "stage");
    canor_elite_jlit(&j, d->stage < sizeof canor_elite_stage_names /
                                        sizeof canor_elite_stage_names[0]
                             ? canor_elite_stage_names[d->stage]
                             : "unknown");
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "status");
    canor_elite_jlit(&j, canor_elite_status_name((canor_status)d->status));
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "subject");
    if (d->subject < a->signatory_count)
      canor_elite_jstr(&j, a->signatory[d->subject].name);
    else
      canor_elite_jindex(&j, d->subject);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "certifier");
    if (d->peer < a->signatory_count)
      canor_elite_jstr(&j, a->signatory[d->peer].name);
    else
      canor_elite_jindex(&j, d->peer);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "item");
    if (d->stage == CANOR_ELITE_STAGE_ACCORD && d->index < CANOR_ELITE_ITEM_COUNT)
      canor_elite_jlit(&j, canor_elite_item_names[d->index]);
    else if ((d->stage == CANOR_ELITE_STAGE_CATEGORY ||
              d->stage == CANOR_ELITE_STAGE_SUBMIT ||
              d->stage == CANOR_ELITE_STAGE_CERTIFY) &&
             d->index < a->basis.category_count)
      canor_elite_jstr(&j, a->basis.category[d->index].name);
    else if (d->stage == CANOR_ELITE_STAGE_VOCAB && d->index < a->basis.vocab_count)
      canor_elite_jstr(&j, a->basis.vocab[d->index].token);
    else
      canor_elite_jindex(&j, d->index);
    canor_elite_jraw(&j, ",");
    canor_elite_jkey(&j, "flags");
    canor_elite_jflags(&j, d->flags, canor_elite_flag_names,
                       sizeof canor_elite_flag_names / sizeof canor_elite_flag_names[0]);
    canor_elite_jraw(&j, "}");
  }
  canor_elite_jraw(&j, "]}");
  return canor_elite_jfinish(&j, len);
}
