/* Property tests for canor_elite: the multilateral Diffeomorphic Basis accord
 * and the Elite(TM) assembler. Canor WD v1 §7, §6; Z-Notation Draft §7, §8.
 *
 *   canor_test_elite            run every property; exit 0 when all hold
 *   canor_test_elite --record   also print the worked example's governance
 *                               record and accord report (JSON)
 *
 * Build (self-contained: canor_elite.c needs no other source):
 *   cc -std=c11 -Wall -Wextra -Werror -pedantic -Isource/include \
 *      source/src/canor_elite.c source/tests/canor_test_elite.c -o canor_test_elite
 * With source/CMakeLists.txt, add `elite` to the canor_test_* foreach list.
 *
 * The cryptography below is a TEST STAND-IN, NOT CRYPTOGRAPHIC: a keyed
 * FNV-1a 64. The digest and signature algorithms are profile-defined and still
 * open (SCHEMA.md, United; Z §7, §12); the core only needs the profile to
 * supply pure digest and verify functions, which is exactly what this does.
 */

#include <stdio.h>
#include <string.h>

#include "canor_elite.h"

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
    }                                                                          \
  } while (0)

/* --- test stand-in cryptography (NOT CRYPTOGRAPHIC) --- */

#define TOY_FNV_OFFSET UINT64_C(0xcbf29ce484222325)
#define TOY_FNV_PRIME UINT64_C(0x100000001b3)
#define TOY_DIGEST_BYTES 8

static uint64_t toy_fnv(uint64_t h, const uint8_t *p, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i) {
    h ^= p[i];
    h *= TOY_FNV_PRIME;
  }
  return h;
}

static void toy_put(uint64_t h, canor_elite_blob_t *out) {
  int i;
  memset(out, 0, sizeof *out);
  out->len = TOY_DIGEST_BYTES;
  for (i = TOY_DIGEST_BYTES - 1; i >= 0; --i) {
    out->bytes[i] = (uint8_t)(h & 0xFFu);
    h >>= 8;
  }
}

static canor_status toy_digest(void *ctx, const uint8_t *msg, size_t len,
                               canor_elite_blob_t *out) {
  (void)ctx;
  toy_put(toy_fnv(TOY_FNV_OFFSET, msg, len), out);
  return CANOR_OK;
}

static canor_elite_blob_t toy_sign(const canor_elite_signatory_t *who,
                                   const uint8_t *msg, size_t len) {
  canor_elite_blob_t b;
  toy_put(toy_fnv(toy_fnv(TOY_FNV_OFFSET, who->key.bytes, who->key.len), msg, len),
          &b);
  return b;
}

static canor_elite_sig_t toy_verify(void *ctx, const canor_elite_signatory_t *who,
                                    const uint8_t *msg, size_t len,
                                    const canor_elite_blob_t *signature) {
  canor_elite_blob_t expect;
  (void)ctx;
  if (who->key.len == 0) return CANOR_ELITE_SIG_UNDEFINED; /* no trust root */
  expect = toy_sign(who, msg, len);
  return (signature->len == expect.len &&
          memcmp(signature->bytes, expect.bytes, expect.len) == 0)
             ? CANOR_ELITE_SIG_VALID
             : CANOR_ELITE_SIG_INVALID;
}

static canor_elite_blob_t toy_blob(const char *text) {
  canor_elite_blob_t b;
  toy_digest(NULL, (const uint8_t *)text, strlen(text), &b);
  return b;
}

/* --- fixture: three parties, two error categories --- */

enum { CAT_ANTI_CHEAT = 0, CAT_ANTI_MALWARE = 1, CAT_COUNT = 2 };
enum { SIG_USER = 0, SIG_APP = 1, SIG_AGENT = 2, SIG_COUNT = 3 };

#define FIX_SCALE 1000 /* category scale: values in thousandths */
#define FIX_VSCALE 1000 /* vector component scale */
#define FIX_DIM 4
#define FIX_VECTORS 2
#define FIX_BOUND_CHEAT 50   /* B[anti-cheat]   = [0, 0.050] */
#define FIX_BOUND_MALWARE 20 /* B[anti-malware] = [0, 0.020] */
#define FIX_ALLOW_CHEAT 30
#define FIX_ALLOW_MALWARE 15

/* Each party's published, orthonormalized meta-schema extraction. */
static const int32_t fix_vectors[SIG_COUNT][FIX_VECTORS * FIX_DIM] = {
    /* user: exact basis, error 0 */
    {1000, 0, 0, 0, /**/ 0, 1000, 0, 0},
    /* app: D = |<v0,v1>| = 10000 -> 10000 * 1000 / 10^6 = 10 thousandths */
    {1000, 10, 0, 0, /**/ 0, 1000, 0, 0},
    /* agent: D = 3000 -> 3 thousandths */
    {1000, 0, 0, 0, /**/ 3, 999, 0, 0},
};

static canor_elite_profile_t g_profile;
static canor_elite_basis_t g_basis;
static canor_elite_accord_t g_accord;
static canor_elite_accord_t g_work;
static canor_elite_report_t g_report;
static canor_elite_program_t g_program;
static canor_elite_assembly_t g_asm;
static canor_elite_assembly_t g_asm2;
static char g_json[1 << 16];
static char g_json2[1 << 16];

static void fix_profile(canor_elite_profile_t *p, bool crypto) {
  CHECK(canor_elite_profile_init(p, "canor.default", 1) == CANOR_OK);
  CHECK(canor_elite_profile_add_setting(p, "canor_lexis.fws", 1) == CANOR_OK);
  CHECK(canor_elite_profile_add_setting(p, "canor_contracts.fws", 1) == CANOR_OK);
  CHECK(canor_elite_profile_add_contract(p, "canordb-write") == CANOR_OK);
  if (crypto) {
    strcpy(p->digest_algorithm, "toy-fnv1a64 (test only)");
    strcpy(p->signature_algorithm, "toy-keyed-fnv1a64 (test only)");
    p->digest = toy_digest;
    p->verify = toy_verify;
  }
}

static void fix_basis_digest(canor_elite_basis_t *b) {
  uint8_t buf[CANOR_ELITE_BASIS_ENCODED_MAX];
  size_t len;
  CHECK(canor_elite_basis_encode(b, buf, sizeof buf, &len) == CANOR_OK);
  toy_digest(NULL, buf, len, &b->digest);
}

static void fix_basis(canor_elite_basis_t *b) {
  canor_elite_interval_t cheat = {0, FIX_BOUND_CHEAT};
  canor_elite_interval_t malware = {0, FIX_BOUND_MALWARE};
  CHECK(canor_elite_basis_init(b, "elite-basis", 1) == CANOR_OK);
  CHECK(canor_elite_basis_add_word(b, "mov", 2, false, NULL) == CANOR_OK);
  CHECK(canor_elite_basis_add_word(b, "jmp", 1, false, NULL) == CANOR_OK);
  CHECK(canor_elite_basis_add_word(b, "cmp", 2, false, NULL) == CANOR_OK);
  CHECK(canor_elite_basis_add_word(b, "nop", 0, false, NULL) == CANOR_OK);
  CHECK(canor_elite_basis_add_word(b, "store", 2, true, "canordb-write") == CANOR_OK);
  CHECK(canor_elite_basis_add_category(b, "anti-cheat", CANOR_ELITE_METRIC_GRAM_LINF,
                                       FIX_SCALE, cheat) == CANOR_OK);
  CHECK(canor_elite_basis_add_category(b, "anti-malware",
                                       CANOR_ELITE_METRIC_GRAM_LINF, FIX_SCALE,
                                       malware) == CANOR_OK);
  fix_basis_digest(b);
}

static void sign_ratify(canor_elite_accord_t *a, uint8_t s) {
  uint8_t buf[CANOR_ELITE_STATEMENT_MAX];
  size_t len;
  canor_elite_blob_t sig;
  CHECK(canor_elite_statement_ratify(a, s, buf, sizeof buf, &len) == CANOR_OK);
  sig = toy_sign(&a->signatory[s], buf, len);
  CHECK(canor_elite_accord_ratify(a, s, &sig) == CANOR_OK);
}

static void sign_approve(canor_elite_accord_t *a, uint8_t s) {
  uint8_t buf[CANOR_ELITE_STATEMENT_MAX];
  size_t len;
  canor_elite_blob_t sig;
  CHECK(canor_elite_statement_approve(a, s, buf, sizeof buf, &len) == CANOR_OK);
  sig = toy_sign(&a->signatory[s], buf, len);
  CHECK(canor_elite_accord_approve(a, s, &sig) == CANOR_OK);
}

static void sign_cert(canor_elite_accord_t *a, uint8_t t, uint8_t s, uint8_t c,
                      canor_elite_verdict_t verdict) {
  uint8_t buf[CANOR_ELITE_STATEMENT_MAX];
  size_t len;
  canor_elite_blob_t sig;
  CHECK(canor_elite_statement_cert(a, t, s, c, verdict, buf, sizeof buf, &len) ==
        CANOR_OK);
  sig = toy_sign(&a->signatory[t], buf, len);
  CHECK(canor_elite_accord_certify(a, t, s, c, verdict, &sig) == CANOR_OK);
}

static canor_elite_blob_t evidence_of(const int32_t *v) {
  uint8_t buf[CANOR_ELITE_VECTORS_ENCODED_MAX];
  size_t len;
  canor_elite_blob_t d;
  CHECK(canor_elite_vectors_encode(v, FIX_VECTORS, FIX_DIM, FIX_VSCALE, buf,
                                   sizeof buf, &len) == CANOR_OK);
  toy_digest(NULL, buf, len, &d);
  return d;
}

static void submit_honest(canor_elite_accord_t *a, uint8_t s, uint8_t c,
                          int64_t allow_hi) {
  canor_elite_interval_t measured, allow = {0, allow_hi};
  canor_elite_blob_t ev = evidence_of(fix_vectors[s]);
  CHECK(canor_elite_gram_linf(fix_vectors[s], FIX_VECTORS, FIX_DIM, FIX_VSCALE,
                              FIX_SCALE, &measured) == CANOR_OK);
  CHECK(canor_elite_accord_submit(a, s, c, allow, measured, "gram-schmidt-agent",
                                  &ev) == CANOR_OK);
}

/* Every certifier recomputes, then signs what certify_check concludes. */
static void certify_all(const canor_elite_profile_t *p, canor_elite_accord_t *a) {
  uint8_t t, s, c;
  for (t = 0; t < a->signatory_count; ++t)
    for (s = 0; s < a->signatory_count; ++s)
      for (c = 0; c < a->basis.category_count; ++c) {
        canor_elite_verdict_t v;
        uint16_t flags;
        if (t == s) continue;
        CHECK(canor_elite_certify_check(p, a, s, c, fix_vectors[s], FIX_VECTORS,
                                        FIX_DIM, FIX_VSCALE, &v,
                                        &flags) == CANOR_OK);
        sign_cert(a, t, s, c, v);
      }
}

static void fix_accord(canor_elite_accord_t *a, uint8_t parties) {
  static const char *const names[SIG_COUNT] = {"user", "app", "agent"};
  static const canor_elite_role_t roles[SIG_COUNT] = {
      CANOR_ELITE_ROLE_USER, CANOR_ELITE_ROLE_APP, CANOR_ELITE_ROLE_AGENT};
  canor_elite_blob_t object = toy_blob("iiiijk xor (mod 4)"); /* WD v1 §6 body */
  canor_elite_blob_t prov = toy_blob("provenance: canor.default v1, 2026-10-20");
  uint8_t s;
  CHECK(canor_elite_accord_init(a, &g_basis, &object, 1, &prov) == CANOR_OK);
  for (s = 0; s < parties; ++s) {
    canor_elite_blob_t key = toy_blob(names[s]);
    canor_elite_blob_t prof = toy_blob(roles[s] == CANOR_ELITE_ROLE_AGENT
                                           ? "agent.fwp"
                                           : "local.fwp");
    uint8_t idx = CANOR_ELITE_NONE;
    CHECK(canor_elite_accord_add_signatory(a, names[s], roles[s],
                                           s == SIG_AGENT, &key, &prof,
                                           &idx) == CANOR_OK);
    CHECK(idx == s);
  }
}

/* The full unanimous protocol: ratify, submit, certify, approve. */
static void run_protocol(const canor_elite_profile_t *p, canor_elite_accord_t *a) {
  uint8_t s;
  for (s = 0; s < a->signatory_count; ++s) sign_ratify(a, s);
  for (s = 0; s < a->signatory_count; ++s) {
    submit_honest(a, s, CAT_ANTI_CHEAT, FIX_ALLOW_CHEAT);
    submit_honest(a, s, CAT_ANTI_MALWARE, FIX_ALLOW_MALWARE);
  }
  certify_all(p, a);
  for (s = 0; s < a->signatory_count; ++s) sign_approve(a, s);
}

static canor_elite_outcome_t outcome_of(const canor_elite_profile_t *p,
                                        const canor_elite_accord_t *a) {
  canor_elite_outcome_t o = CANOR_ELITE_ACCORD_ACCEPTED;
  CHECK(canor_elite_accord_evaluate(p, a, &g_report, &o) == CANOR_OK);
  CHECK(g_report.outcome == o);
  return o;
}

/* Does the report hold a diagnostic at (stage, subject, peer) with flag? */
static bool has_diag(uint8_t stage, uint8_t subject, uint8_t peer, uint16_t flag) {
  uint16_t i;
  for (i = 0; i < g_report.diag_count; ++i) {
    const canor_elite_diag_t *d = &g_report.diag[i];
    if (d->stage == stage && d->subject == subject && d->peer == peer &&
        (d->flags & flag))
      return true;
  }
  return false;
}

/* ======================================================================= */

static void test_gram_metric(void) {
  canor_elite_interval_t e;
  /* identity: exactly orthonormal */
  CHECK(canor_elite_gram_linf(fix_vectors[SIG_USER], FIX_VECTORS, FIX_DIM,
                              FIX_VSCALE, FIX_SCALE, &e) == CANOR_OK);
  CHECK(e.lo == 0 && e.hi == 0);
  /* exact integer result */
  CHECK(canor_elite_gram_linf(fix_vectors[SIG_APP], FIX_VECTORS, FIX_DIM,
                              FIX_VSCALE, FIX_SCALE, &e) == CANOR_OK);
  CHECK(e.lo == 10 && e.hi == 10);
  CHECK(canor_elite_gram_linf(fix_vectors[SIG_AGENT], FIX_VECTORS, FIX_DIM,
                              FIX_VSCALE, FIX_SCALE, &e) == CANOR_OK);
  CHECK(e.lo == 3 && e.hi == 3);
  {
    /* two 1-d vectors: G00 = 0, G01 = 1234000, G11 = 1234^2 - 10^6 = 522756,
     * so D = 1234000 and D * 1000 / 10^6 = 1234 exactly */
    const int32_t v[2] = {1000, 1234};
    const int32_t one[1] = {1000};
    CHECK(canor_elite_gram_linf(v, 2, 1, FIX_VSCALE, FIX_SCALE, &e) == CANOR_OK);
    CHECK(e.lo == 1234 && e.hi == 1234);
    CHECK(canor_elite_gram_linf(one, 1, 1, FIX_VSCALE, FIX_SCALE, &e) == CANOR_OK);
    CHECK(e.lo == 0 && e.hi == 0);
  }
  {
    /* fractional enclosure: G00 = 1001^2 - 10^6 = 2001 -> 2.001 -> [2, 3] */
    const int32_t v[1] = {1001};
    CHECK(canor_elite_gram_linf(v, 1, 1, FIX_VSCALE, FIX_SCALE, &e) == CANOR_OK);
    CHECK(e.lo == 2 && e.hi == 3);
  }
  {
    const int32_t big[1] = {(int32_t)(CANOR_ELITE_COMPONENT_MAX + 1)};
    const int32_t ok[1] = {1};
    CHECK(canor_elite_gram_linf(big, 1, 1, FIX_VSCALE, FIX_SCALE, &e) ==
          CANOR_ERR_RANGE);
    CHECK(canor_elite_gram_linf(ok, 0, 1, FIX_VSCALE, FIX_SCALE, &e) ==
          CANOR_ERR_RANGE);
    CHECK(canor_elite_gram_linf(ok, 1, 1, 0, FIX_SCALE, &e) == CANOR_ERR_RANGE);
    CHECK(canor_elite_gram_linf(ok, 1, 1, FIX_VSCALE, CANOR_ELITE_SCALE_MAX + 1,
                                &e) == CANOR_ERR_RANGE);
    CHECK(canor_elite_gram_linf(NULL, 1, 1, FIX_VSCALE, FIX_SCALE, &e) ==
          CANOR_ERR_ARGS);
  }
  {
    /* Extremes stay exact or say RANGE: never a wrapped number. A component
     * of 2^24 at vscale 1 gives D = 2^48 - 1 per unit, times cscale 2^20. */
    const int32_t v[1] = {(int32_t)CANOR_ELITE_COMPONENT_MAX};
    canor_status st =
        canor_elite_gram_linf(v, 1, 1, 1, CANOR_ELITE_SCALE_MAX, &e);
    CHECK(st == CANOR_ERR_RANGE || (st == CANOR_OK && e.lo >= 0));
    st = canor_elite_gram_linf(v, 1, 1, 1, 1, &e);
    CHECK(st == CANOR_OK && e.lo == (INT64_C(1) << 48) - 1 && e.hi == e.lo);
  }
}

static void test_encoding(void) {
  uint8_t a[CANOR_ELITE_STATEMENT_MAX], b[CANOR_ELITE_STATEMENT_MAX];
  size_t la, lb, need;
  fix_accord(&g_work, SIG_COUNT);
  /* deterministic */
  CHECK(canor_elite_statement_ratify(&g_work, 0, a, sizeof a, &la) == CANOR_OK);
  CHECK(canor_elite_statement_ratify(&g_work, 0, b, sizeof b, &lb) == CANOR_OK);
  CHECK(la == lb && memcmp(a, b, la) == 0);
  /* size query */
  CHECK(canor_elite_statement_ratify(&g_work, 0, NULL, 0, &need) == CANOR_ERR_FULL);
  CHECK(need == la);
  CHECK(canor_elite_statement_ratify(&g_work, 0, a, la - 1, &need) ==
        CANOR_ERR_FULL);
  /* domain separation: a ratification never reads as an approval */
  CHECK(canor_elite_statement_approve(&g_work, 0, b, sizeof b, &lb) == CANOR_OK);
  CHECK(la != lb || memcmp(a, b, la) != 0);
  /* each signatory signs different bytes */
  CHECK(canor_elite_statement_ratify(&g_work, 1, b, sizeof b, &lb) == CANOR_OK);
  CHECK(la != lb || memcmp(a, b, la) != 0);
  /* nothing to certify yet */
  CHECK(canor_elite_statement_cert(&g_work, 0, 1, 0, CANOR_ELITE_VERDICT_CERTIFIED,
                                   a, sizeof a, &la) == CANOR_ERR_NOT_FOUND);
  CHECK(canor_elite_statement_cert(&g_work, 1, 1, 0, CANOR_ELITE_VERDICT_CERTIFIED,
                                   a, sizeof a, &la) == CANOR_ERR_ARGS);
  CHECK(canor_elite_statement_ratify(&g_work, SIG_COUNT, a, sizeof a, &la) ==
        CANOR_ERR_RANGE);
}

static void test_builders(void) {
  canor_elite_basis_t b;
  canor_elite_interval_t ok = {0, 10}, bad = {5, 1}, neg = {-1, 1};
  uint8_t i;
  char longname[CANOR_ELITE_NAME_MAX + 1];
  memset(longname, 'x', sizeof longname - 1);
  longname[sizeof longname - 1] = '\0';
  CHECK(canor_elite_basis_init(&b, "b", 1) == CANOR_OK);
  CHECK(canor_elite_basis_init(&b, "", 1) == CANOR_ERR_ARGS);
  CHECK(canor_elite_basis_init(&b, longname, 1) == CANOR_ERR_RANGE);
  CHECK(canor_elite_basis_init(&b, "b", 1) == CANOR_OK);
  CHECK(canor_elite_basis_add_word(&b, "mov", 2, false, NULL) == CANOR_OK);
  CHECK(canor_elite_basis_add_word(&b, "mov", 1, false, NULL) == CANOR_ERR_ARGS);
  CHECK(canor_elite_basis_add_word(&b, "wide", CANOR_ELITE_OPERAND_MAX + 1, false,
                                   NULL) == CANOR_ERR_RANGE);
  CHECK(canor_elite_basis_add_category(&b, "c", "gram-linf", 0, ok) ==
        CANOR_ERR_RANGE);
  CHECK(canor_elite_basis_add_category(&b, "c", "gram-linf", 1000, bad) ==
        CANOR_ERR_RANGE);
  CHECK(canor_elite_basis_add_category(&b, "c", "gram-linf", 1000, neg) ==
        CANOR_ERR_RANGE);
  /* capacity: every insert checks first and refuses with FULL */
  for (i = 0; i < CANOR_ELITE_CATEGORY_MAX; ++i) {
    char n[8];
    n[0] = 'c';
    n[1] = (char)('a' + i);
    n[2] = '\0';
    CHECK(canor_elite_basis_add_category(&b, n, "gram-linf", 1000, ok) == CANOR_OK);
  }
  CHECK(canor_elite_basis_add_category(&b, "overflow", "gram-linf", 1000, ok) ==
        CANOR_ERR_FULL);
  CHECK(b.category_count == CANOR_ELITE_CATEGORY_MAX);

  fix_accord(&g_work, 2);
  CHECK(canor_elite_accord_add_signatory(&g_work, "user", CANOR_ELITE_ROLE_USER,
                                         false, NULL, NULL,
                                         NULL) == CANOR_ERR_ARGS); /* distinct */
  /* "certified by the OTHER signatory parts": never oneself */
  {
    canor_elite_blob_t sig = toy_blob("x");
    CHECK(canor_elite_accord_certify(&g_work, 1, 1, 0,
                                     CANOR_ELITE_VERDICT_CERTIFIED,
                                     &sig) == CANOR_ERR_ARGS);
    CHECK(canor_elite_accord_certify(&g_work, 0, 1, 0, CANOR_ELITE_VERDICT_PENDING,
                                     &sig) == CANOR_ERR_ARGS);
    CHECK(canor_elite_accord_ratify(&g_work, 0, NULL) == CANOR_ERR_ARGS);
    CHECK(canor_elite_accord_ratify(&g_work, 2, &sig) == CANOR_ERR_RANGE);
  }
  CHECK(canor_elite_accord_submit(&g_work, 0, 0, bad, ok, "m", NULL) ==
        CANOR_ERR_RANGE);
}

static void test_unanimous_acceptance(void) {
  fix_accord(&g_accord, SIG_COUNT);
  /* nothing given yet: incomplete, never accepted by default */
  CHECK(outcome_of(&g_profile, &g_accord) == CANOR_ELITE_ACCORD_INCOMPLETE);
  CHECK(has_diag(CANOR_ELITE_STAGE_RATIFY, SIG_APP, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_PENDING));
  run_protocol(&g_profile, &g_accord);
  CHECK(outcome_of(&g_profile, &g_accord) == CANOR_ELITE_ACCORD_ACCEPTED);
  CHECK(g_report.diag_count == 0);
  /* items: 8 accord-level + 5 words + 2 categories + 3 ratify + 3*2 submit
   * + 3*2*2 certify + 3 approve = 39 */
  CHECK(g_report.checked == 39);
  CHECK(g_report.passed == g_report.checked);
}

static void test_bilateral_is_the_two_party_case(void) {
  fix_accord(&g_work, 2);
  run_protocol(&g_profile, &g_work);
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_ACCEPTED);
  fix_accord(&g_work, 1);
  run_protocol(&g_profile, &g_work);
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_INCOMPLETE);
  CHECK(has_diag(CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_TOO_FEW));
}

static void test_one_missing_consent(void) {
  g_work = g_accord;
  g_work.approval[SIG_AGENT].present = false;
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_INCOMPLETE);
  CHECK(has_diag(CANOR_ELITE_STAGE_APPROVE, SIG_AGENT, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_PENDING));
  CHECK(g_report.diag_count == 1);

  g_work = g_accord;
  memset(&g_work.cert[SIG_USER][SIG_APP][CAT_ANTI_MALWARE], 0,
         sizeof g_work.cert[0][0][0]);
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_INCOMPLETE);
  CHECK(has_diag(CANOR_ELITE_STAGE_CERTIFY, SIG_APP, SIG_USER,
                 CANOR_ELITE_F_PENDING));
}

static void test_one_rejection_rejects(void) {
  g_work = g_accord;
  sign_cert(&g_work, SIG_AGENT, SIG_USER, CAT_ANTI_CHEAT,
            CANOR_ELITE_VERDICT_REJECTED);
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_REJECTED);
  CHECK(has_diag(CANOR_ELITE_STAGE_CERTIFY, SIG_USER, SIG_AGENT,
                 CANOR_ELITE_F_REJECTED));
  /* a rejection outranks anything merely pending */
  g_work.approval[SIG_APP].present = false;
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_REJECTED);
}

static void test_out_of_bound_and_collusion(void) {
  canor_elite_verdict_t v;
  uint16_t flags;
  g_work = g_accord;
  /* app asks for more anti-malware allowance than the basis bound */
  submit_honest(&g_work, SIG_APP, CAT_ANTI_MALWARE, FIX_BOUND_MALWARE + 5);
  CHECK(canor_elite_certify_check(&g_profile, &g_work, SIG_APP, CAT_ANTI_MALWARE,
                                  fix_vectors[SIG_APP], FIX_VECTORS, FIX_DIM,
                                  FIX_VSCALE, &v, &flags) == CANOR_OK);
  CHECK(v == CANOR_ELITE_VERDICT_REJECTED);
  CHECK(flags & CANOR_ELITE_F_OUT_OF_BOUND);
  /* resubmission voided the old certificates */
  CHECK(g_work.cert[SIG_USER][SIG_APP][CAT_ANTI_MALWARE].verdict ==
        CANOR_ELITE_VERDICT_PENDING);
  /* a colluding certifier signs CERTIFIED anyway: that is itself diagnosed */
  sign_cert(&g_work, SIG_USER, SIG_APP, CAT_ANTI_MALWARE,
            CANOR_ELITE_VERDICT_CERTIFIED);
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_REJECTED);
  CHECK(has_diag(CANOR_ELITE_STAGE_SUBMIT, SIG_APP, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_OUT_OF_BOUND));
  CHECK(has_diag(CANOR_ELITE_STAGE_CERTIFY, SIG_APP, SIG_USER,
                 CANOR_ELITE_F_INCONSISTENT));
  /* the agent's certificate is still pending, and reported too: none pruned */
  CHECK(has_diag(CANOR_ELITE_STAGE_CERTIFY, SIG_APP, SIG_AGENT,
                 CANOR_ELITE_F_PENDING));
}

static void test_cheating_claims(void) {
  canor_elite_verdict_t v;
  uint16_t flags;
  canor_elite_interval_t zero = {0, 0}, allow = {0, FIX_ALLOW_CHEAT};
  canor_elite_blob_t ev = evidence_of(fix_vectors[SIG_APP]);
  g_work = g_accord;
  /* app under-reports its error: claims 0, actually 10 */
  CHECK(canor_elite_accord_submit(&g_work, SIG_APP, CAT_ANTI_CHEAT, allow, zero,
                                  "gram-schmidt-agent", &ev) == CANOR_OK);
  CHECK(canor_elite_certify_check(&g_profile, &g_work, SIG_APP, CAT_ANTI_CHEAT,
                                  fix_vectors[SIG_APP], FIX_VECTORS, FIX_DIM,
                                  FIX_VSCALE, &v, &flags) == CANOR_OK);
  CHECK(v == CANOR_ELITE_VERDICT_REJECTED && (flags & CANOR_ELITE_F_INCONSISTENT));
  /* evidence that is not the published vectors */
  CHECK(canor_elite_certify_check(&g_profile, &g_work, SIG_APP, CAT_ANTI_CHEAT,
                                  fix_vectors[SIG_USER], FIX_VECTORS, FIX_DIM,
                                  FIX_VSCALE, &v, &flags) == CANOR_OK);
  CHECK(flags & CANOR_ELITE_F_DIGEST_MISMATCH);
  /* forged signature */
  g_work = g_accord;
  g_work.approval[SIG_USER].signature.bytes[0] ^= 0x01u;
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_REJECTED);
  CHECK(has_diag(CANOR_ELITE_STAGE_APPROVE, SIG_USER, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_SIG_INVALID));
  /* a signature from the wrong party does not verify */
  g_work = g_accord;
  g_work.ratification[SIG_APP] = g_work.ratification[SIG_USER];
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_REJECTED);
  /* tampering with the basis after it was digested and ratified */
  g_work = g_accord;
  g_work.basis.vocab[0].arity = 3;
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_REJECTED);
  CHECK(has_diag(CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_DIGEST_MISMATCH));
  /* widening the envelope breaks every ratification and approval */
  g_work = g_accord;
  g_work.basis.category[CAT_ANTI_CHEAT].bound.hi = 1000;
  fix_basis_digest(&g_work.basis);
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_REJECTED);
  CHECK(has_diag(CANOR_ELITE_STAGE_RATIFY, SIG_AGENT, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_SIG_INVALID));
}

static void test_roster_change_requires_reconsent(void) {
  canor_elite_blob_t key = toy_blob("observer");
  g_work = g_accord;
  CHECK(canor_elite_accord_add_signatory(&g_work, "observer", CANOR_ELITE_ROLE_AGENT,
                                         true, &key, NULL, NULL) == CANOR_OK);
  CHECK(!g_work.ratification[SIG_USER].present && !g_work.approval[SIG_USER].present);
  CHECK(outcome_of(&g_profile, &g_work) != CANOR_ELITE_ACCORD_ACCEPTED);
}

static void test_unset_profile_is_never_agreed(void) {
  canor_elite_profile_t bare;
  fix_profile(&bare, false);
  CHECK(outcome_of(&bare, &g_accord) == CANOR_ELITE_ACCORD_UNDEFINED);
  CHECK(has_diag(CANOR_ELITE_STAGE_ACCORD, CANOR_ELITE_NONE, CANOR_ELITE_NONE,
                 CANOR_ELITE_F_UNSET));
  {
    canor_elite_verdict_t v;
    uint16_t flags;
    CHECK(canor_elite_certify_check(&bare, &g_accord, SIG_APP, CAT_ANTI_CHEAT,
                                    fix_vectors[SIG_APP], FIX_VECTORS, FIX_DIM,
                                    FIX_VSCALE, &v, &flags) == CANOR_ERR_UNDEFINED);
    CHECK(v == CANOR_ELITE_VERDICT_PENDING && flags == CANOR_ELITE_F_UNSET);
  }
  /* a party with no trust root cannot be verified */
  g_work = g_accord;
  g_work.signatory[SIG_AGENT].key.len = 0;
  CHECK(outcome_of(&g_profile, &g_work) == CANOR_ELITE_ACCORD_UNDEFINED);
}

static void test_diagnostic_bound(void) {
  /* Worst case: every party, every category, nothing given yet. */
  canor_elite_basis_t b;
  canor_elite_interval_t ok = {0, 10};
  uint8_t i;
  CHECK(canor_elite_basis_init(&b, "wide", 1) == CANOR_OK);
  for (i = 0; i < CANOR_ELITE_CATEGORY_MAX; ++i) {
    char n[4] = {'c', (char)('a' + i), '\0', '\0'};
    CHECK(canor_elite_basis_add_category(&b, n, "gram-linf", 1000, ok) == CANOR_OK);
  }
  CHECK(canor_elite_accord_init(&g_work, &b, NULL, 1, NULL) == CANOR_OK);
  for (i = 0; i < CANOR_ELITE_SIGNATORY_MAX; ++i) {
    char n[4] = {'p', (char)('a' + i), '\0', '\0'};
    CHECK(canor_elite_accord_add_signatory(&g_work, n, CANOR_ELITE_ROLE_USER, false,
                                           NULL, NULL, NULL) == CANOR_OK);
  }
  {
    canor_elite_outcome_t o;
    CHECK(canor_elite_accord_evaluate(&g_profile, &g_work, &g_report, &o) ==
          CANOR_OK);
    CHECK(o == CANOR_ELITE_ACCORD_UNDEFINED); /* digests unset outrank pending */
    CHECK(g_report.diag_count <= CANOR_ELITE_DIAG_MAX);
    /* every item that failed is in the report: checked - passed == diags */
    CHECK(g_report.checked - g_report.passed == g_report.diag_count);
  }
}

/* --- assembler --- */

static void fix_program(canor_elite_program_t *pg, canor_elite_direction_t dir) {
  static const char *const dollar[] = {"mov", "jmp", "cmp"}; /* WD v1 §7 */
  static const char *const ab[] = {"r0", "r1"};
  static const char *const x[] = {"r2"};
  CHECK(canor_elite_program_init(pg, dir) == CANOR_OK);
  CHECK(canor_elite_program_add_rule(pg, "$", dollar, 3) == CANOR_OK);
  CHECK(canor_elite_program_add_insn(pg, "$", ab, 2) == CANOR_OK);
  CHECK(canor_elite_program_add_insn(pg, "$", x, 1) == CANOR_OK);
  CHECK(canor_elite_program_add_insn(pg, "store", ab, 2) == CANOR_OK);
}

static void test_assembler_branches(void) {
  canor_elite_record_t *r = &g_asm.record;
  fix_program(&g_program, CANOR_ELITE_L2R);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm) ==
        CANOR_OK);
  /* 3 * 3 * 1 interpretations, every one retained */
  CHECK(r->total == 9 && r->consumed == 9 && g_asm.branch_count == 9);
  CHECK(r->termination == CANOR_ELITE_TERM_EXHAUSTED && r->complete);
  CHECK(r->basis_bound && r->basis_outcome == CANOR_ELITE_ACCORD_ACCEPTED);
  /* site 0 wants arity 2 (mov, cmp), site 1 arity 1 (jmp): two candidates */
  CHECK(r->candidates == 2 && r->diagnosed == 7);
  /* L2R: site 0 most significant, so id = 3 * c0 + c1 */
  CHECK(g_asm.branch[1].candidate && g_asm.branch[1].choice[0] == 0 &&
        g_asm.branch[1].choice[1] == 1);
  CHECK(g_asm.branch[7].candidate && g_asm.branch[7].choice[0] == 2);
  CHECK(g_asm.branch[0].site_flags[1] & CANOR_ELITE_S_ARITY); /* mov r2 */
  CHECK(!g_asm.branch[0].candidate); /* diagnosed, still there */
  /* the effect is recorded on every branch, performed on none */
  CHECK(r->effects_declared == 9);
  CHECK(g_asm.branch[1].site_flags[2] == CANOR_ELITE_S_EFFECT_RECORDED);
  CHECK(r->selected == 1);

  /* R2L: the last rule site is most significant, so id = 3 * c1 + c0 */
  fix_program(&g_program, CANOR_ELITE_R2L);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm) ==
        CANOR_OK);
  CHECK(g_asm.branch[3].candidate && g_asm.branch[3].choice[0] == 0 &&
        g_asm.branch[3].choice[1] == 1);
  CHECK(g_asm.branch[5].candidate && r->selected == 3);

  /* traversal limit: retained up to it, and the record says incomplete */
  fix_program(&g_program, CANOR_ELITE_L2R);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 4, &g_asm) ==
        CANOR_OK);
  CHECK(r->consumed == 4 && r->total == 9 && g_asm.branch_count == 4);
  CHECK(r->termination == CANOR_ELITE_TERM_LIMIT && !r->complete);
  CHECK(r->candidates == 1 && r->diagnosed == 3);

  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 0, &g_asm) ==
        CANOR_ERR_RANGE);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program,
                             CANOR_ELITE_BRANCH_MAX + 1, &g_asm) == CANOR_ERR_FULL);
}

static void test_assembler_checks(void) {
  canor_elite_profile_t nocontract;
  canor_elite_record_t *r = &g_asm.record;
  fix_program(&g_program, CANOR_ELITE_L2R);
  /* no basis: raw pattern syntax is permissive, nothing is checked */
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_program, 64, &g_asm) == CANOR_OK);
  CHECK(!r->basis_bound && r->candidates == 9 && r->selected == 0);
  CHECK(g_asm.branch[0].site_flags[0] == CANOR_ELITE_S_UNCHECKED);
  /* the effect's contract is not active in this profile */
  CHECK(canor_elite_profile_init(&nocontract, "canor.default", 1) == CANOR_OK);
  nocontract.digest = toy_digest;
  nocontract.verify = toy_verify;
  strcpy(nocontract.digest_algorithm, g_profile.digest_algorithm);
  strcpy(nocontract.signature_algorithm, g_profile.signature_algorithm);
  CHECK(canor_elite_assemble(&nocontract, &g_accord, &g_program, 64, &g_asm) ==
        CANOR_OK);
  CHECK(r->candidates == 0 && r->diagnosed == 9 && r->effects_declared == 0);
  CHECK(r->selected == CANOR_ELITE_NO_BRANCH);
  CHECK(g_asm.branch[1].site_flags[2] == CANOR_ELITE_S_EFFECT_UNAUTHORIZED);
  {
    /* tokens outside the agreed vocabulary, and nested replacement:
     * "%" -> "$" -> mov | jmp, so line 1 reads mov, jmp or nop */
    static const char *const alts[] = {"hlt", "nop"};
    static const char *const nest[] = {"$", "nop"};
    static const char *const dollar[] = {"mov", "jmp"};
    canor_elite_program_t pg;
    CHECK(canor_elite_program_init(&pg, CANOR_ELITE_L2R) == CANOR_OK);
    CHECK(canor_elite_program_add_rule(&pg, "?", alts, 2) == CANOR_OK);
    CHECK(canor_elite_program_add_rule(&pg, "%", nest, 2) == CANOR_OK);
    CHECK(canor_elite_program_add_rule(&pg, "$", dollar, 2) == CANOR_OK);
    CHECK(canor_elite_program_add_insn(&pg, "?", NULL, 0) == CANOR_OK);
    CHECK(canor_elite_program_add_insn(&pg, "%", NULL, 0) == CANOR_OK);
    CHECK(canor_elite_assemble(&g_profile, &g_accord, &pg, 64, &g_asm) == CANOR_OK);
    CHECK(g_asm.leaf_count[0] == 2 && g_asm.leaf_count[1] == 3);
    CHECK(g_asm.leaf[1][0].depth == 2 && g_asm.leaf[1][0].path[0] == 0 &&
          g_asm.leaf[1][0].path[1] == 0); /* % -> $ -> mov */
    CHECK(g_asm.leaf[1][2].depth == 1 && g_asm.leaf[1][2].path[0] == 1); /* nop */
    CHECK(g_asm.branch_count == 6 && r->complete);
    CHECK(g_asm.branch[0].site_flags[0] == CANOR_ELITE_S_NOT_IN_BASIS); /* hlt */
    CHECK(g_asm.branch[0].site_flags[1] == CANOR_ELITE_S_ARITY); /* mov, no operands */
    CHECK(g_asm.branch[5].candidate); /* nop, nop */
    CHECK(r->candidates == 1 && r->selected == 5);
  }
  {
    /* the largest program the capacities allow: 8^16 = 2^48 interpretations */
    static const char *const eight[] = {"mov", "jmp", "cmp", "nop",
                                        "mov", "jmp", "cmp", "nop"};
    canor_elite_program_t pg;
    uint8_t i;
    CHECK(canor_elite_program_init(&pg, CANOR_ELITE_L2R) == CANOR_OK);
    CHECK(canor_elite_program_add_rule(&pg, "$", eight, 8) == CANOR_OK);
    for (i = 0; i < CANOR_ELITE_INSN_MAX; ++i)
      CHECK(canor_elite_program_add_insn(&pg, "$", NULL, 0) == CANOR_OK);
    CHECK(canor_elite_program_add_insn(&pg, "$", NULL, 0) == CANOR_ERR_FULL);
    CHECK(canor_elite_assemble(&g_profile, &g_accord, &pg, CANOR_ELITE_BRANCH_MAX,
                               &g_asm) == CANOR_OK);
    CHECK(!r->total_overflow && r->total == (UINT64_C(1) << 48));
    CHECK(r->consumed == CANOR_ELITE_BRANCH_MAX && !r->complete);
  }
}

static void test_tools_and_selection(void) {
  canor_elite_blob_t cfg = toy_blob("temperature=0");
  int32_t first[CANOR_ELITE_BRANCH_MAX];
  uint16_t k;
  fix_program(&g_program, CANOR_ELITE_L2R);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm) ==
        CANOR_OK);
  CHECK(g_asm.record.selected == 1);
  /* an agent prefers the cmp branch; scores rank, never prune */
  CHECK(canor_elite_tool_declare(&g_asm, "intent-agent", "0.1",
                                 CANOR_ELITE_TOOL_AGENT, &cfg, 0) == CANOR_OK);
  CHECK(canor_elite_score(&g_asm, 1, 500) == CANOR_OK);
  CHECK(canor_elite_score(&g_asm, 7, 900) == CANOR_OK);
  CHECK(g_asm.record.selected == 7);
  CHECK(canor_elite_score(&g_asm, 1, 900) == CANOR_OK); /* tie: lower id */
  CHECK(g_asm.record.selected == 1);
  CHECK(canor_elite_score(&g_asm, 0, 1000) == CANOR_OK); /* diagnosed: ignored */
  CHECK(g_asm.record.selected == 1 && g_asm.branch_count == 9);
  CHECK(canor_elite_score(&g_asm, 1, CANOR_ELITE_SCALE_MAX) == CANOR_ERR_RANGE);
  CHECK(canor_elite_score(&g_asm, 9, 1) == CANOR_ERR_RANGE);

  /* stochastic tool: must be declared with its seed; reproducible */
  CHECK(canor_elite_score_seeded(&g_asm, 42) == CANOR_ERR_NOT_FOUND);
  CHECK(canor_elite_tool_declare(&g_asm, "splitmix64", "1",
                                 CANOR_ELITE_TOOL_STOCHASTIC, NULL, 42) == CANOR_OK);
  CHECK(canor_elite_score_seeded(&g_asm, 42) == CANOR_OK);
  for (k = 0; k < g_asm.branch_count; ++k) {
    first[k] = g_asm.branch[k].score;
    CHECK(first[k] >= 0 && first[k] <= 1000);
  }
  CHECK(canor_elite_score_seeded(&g_asm, 42) == CANOR_OK);
  for (k = 0; k < g_asm.branch_count; ++k) CHECK(g_asm.branch[k].score == first[k]);
  CHECK(g_asm.record.selected == 1 || g_asm.record.selected == 7);
}

static void test_side_effect_free(void) {
  static canor_elite_accord_t accord_before;
  canor_elite_program_t program_before;
  canor_elite_profile_t profile_before;
  size_t l1, l2;
  fix_program(&g_program, CANOR_ELITE_L2R);
  accord_before = g_accord;
  program_before = g_program;
  profile_before = g_profile;
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm) ==
        CANOR_OK);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm2) ==
        CANOR_OK);
  /* inputs untouched; outputs bit-identical */
  CHECK(memcmp(&accord_before, &g_accord, sizeof g_accord) == 0);
  CHECK(memcmp(&program_before, &g_program, sizeof g_program) == 0);
  CHECK(memcmp(&profile_before, &g_profile, sizeof g_profile) == 0);
  CHECK(memcmp(&g_asm, &g_asm2, sizeof g_asm) == 0);
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm, g_json,
                                sizeof g_json, &l1) == CANOR_OK);
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm2, g_json2,
                                sizeof g_json2, &l2) == CANOR_OK);
  CHECK(l1 == l2 && memcmp(g_json, g_json2, l1) == 0);
  CHECK(strstr(g_json, "\"performed\":0") != NULL);
  CHECK(strstr(g_json, "\"insn\":\"mov r0, r1\"") != NULL);
  CHECK(strstr(g_json, "{\"spec\":\"$\",\"definitions\":[{\"text\":\"mov\"") != NULL);
  /* built-in strings longer than a name are written whole, never cut */
  CHECK(strstr(g_json, "\"spec\":\"Canor WD v1 s7; Z-Notation Draft s7, s8\"") !=
        NULL);
  CHECK(strstr(g_json,
               "\"Aether s9 latency/jitter field per branch traversal: not linked\"") !=
        NULL);
}

static void test_json_buffers(void) {
  size_t need, got;
  char tiny[16];
  fix_program(&g_program, CANOR_ELITE_L2R);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm) ==
        CANOR_OK);
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm, NULL, 0,
                                &need) == CANOR_ERR_FULL);
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm, g_json,
                                sizeof g_json, &got) == CANOR_OK);
  CHECK(need == got && strlen(g_json) == got);
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm, tiny,
                                sizeof tiny, &got) == CANOR_ERR_FULL);
  CHECK(got == need && memchr(tiny, '\0', sizeof tiny) != NULL);
  /* exactly enough room: need + 1 for the NUL */
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm, g_json2,
                                need + 1, &got) == CANOR_OK);
  CHECK(got == need && strcmp(g_json, g_json2) == 0);
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm, g_json2,
                                need, &got) == CANOR_ERR_FULL);
}

/* --- freeform source --- */

static canor_elite_program_t g_pga;
static canor_elite_program_t g_pgb;

static bool parse_into(canor_elite_program_t *pg, canor_elite_direction_t dir,
                       const char *src) {
  if (canor_elite_program_init(pg, dir) != CANOR_OK) return false;
  return canor_elite_program_parse(pg, src, strlen(src)) == CANOR_OK;
}

static bool leaf_is(const canor_elite_program_t *pg,
                    const canor_elite_assembly_t *as, uint8_t line, uint8_t leaf,
                    const char *expect) {
  char t[CANOR_ELITE_TEXT_MAX];
  if (canor_elite_leaf_text(pg, line, &as->leaf[line][leaf], t) != CANOR_OK)
    return false;
  return strcmp(t, expect) == 0;
}

static void test_freeform_source(void) {
  static const char lf[] = "$ : mov | jmp | cmp\n$ r0, r1\n$ r2\nstore r0, r1\n";
  static const char crlf[] =
      "$ : mov | jmp | cmp\r\n$ r0, r1\r\n$ r2\r\nstore r0, r1\r\n";
  /* §7's example as freeform text: LF and CRLF read identically */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, lf));
  CHECK(parse_into(&g_pgb, CANOR_ELITE_L2R, crlf));
  CHECK(memcmp(&g_pga, &g_pgb, sizeof g_pga) == 0);
  CHECK(g_pga.rule_count == 1 && g_pga.rule[0].def_count == 3);
  CHECK(g_pga.line_count == 3 && g_pga.line[0].source_line == 2);
  CHECK(strcmp(g_pga.rule[0].def[2], "cmp") == 0 && g_pga.rule[0].def_line[2] == 1);
  /* and assembles exactly as the builder-made program does */
  fix_program(&g_program, CANOR_ELITE_L2R);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm2) ==
        CANOR_OK);
  CHECK(memcmp(g_asm.branch, g_asm2.branch, sizeof g_asm.branch) == 0);
  CHECK(g_asm.record.candidates == 2 && g_asm.record.selected == 1);
  CHECK(leaf_is(&g_pga, &g_asm, 0, 2, "cmp r0, r1"));

  /* a specification redefined on a later line keeps every definition */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, "$ : mov\n$ : jmp | cmp\n$\n"));
  CHECK(g_pga.rule_count == 1 && g_pga.rule[0].def_count == 3);
  CHECK(g_pga.rule[0].def_line[0] == 1 && g_pga.rule[0].def_line[1] == 2 &&
        g_pga.rule[0].def_line[2] == 2);
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(g_asm.leaf_count[0] == 3 && leaf_is(&g_pga, &g_asm, 0, 0, "mov") &&
        leaf_is(&g_pga, &g_asm, 0, 2, "cmp"));

  /* nested replacement, interpretations in lexicographic path order */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, "op : $ | nop\n$ : mov | jmp\nop r0, r1\n"));
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(g_asm.leaf_count[0] == 3);
  CHECK(leaf_is(&g_pga, &g_asm, 0, 0, "mov r0, r1"));
  CHECK(leaf_is(&g_pga, &g_asm, 0, 1, "jmp r0, r1"));
  CHECK(leaf_is(&g_pga, &g_asm, 0, 2, "nop r0, r1"));
  CHECK(g_asm.leaf[0][0].flags == 0);                   /* mov, arity 2 */
  CHECK(g_asm.leaf[0][1].flags == CANOR_ELITE_S_ARITY); /* jmp takes 1 */
  CHECK(g_asm.record.candidates == 1 && g_asm.record.selected == 0);

  /* a cycle meets the depth limit: diagnosed and kept, never looping */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, "a : b\nb : a\na\n"));
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(g_asm.leaf_count[0] == 1 && g_asm.leaf[0][0].depth == CANOR_ELITE_DEPTH_MAX);
  CHECK(g_asm.leaf[0][0].flags & CANOR_ELITE_S_DEPTH);
  CHECK(g_asm.branch_count == 1 && g_asm.record.diagnosed == 1);
  CHECK(leaf_is(&g_pga, &g_asm, 0, 0, "a")); /* eight rewrites: back to a */

  /* explosive growth stops at the expansion limit, and says so */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, "x : x x | y\nx\n"));
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(g_asm.leaf_more[0] && g_asm.leaf_count[0] == CANOR_ELITE_EXPANSION_MAX);
  CHECK(g_asm.record.expansion_limited && !g_asm.record.complete);
  CHECK(g_asm.record.termination == CANOR_ELITE_TERM_LIMIT);

  /* a rewrite that would not fit is diagnosed, never truncated */
  {
    char src[CANOR_ELITE_TEXT_MAX + 32], t[CANOR_ELITE_TEXT_MAX];
    size_t k;
    memcpy(src, "w : ", 4);
    for (k = 0; k < 100; ++k) src[4 + k] = 'z';
    memcpy(src + 104, "\nw w\n", 6);
    CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, src));
    CHECK(canor_elite_assemble(&g_profile, NULL, &g_pga, 64, &g_asm) == CANOR_OK);
    CHECK(g_asm.leaf_count[0] == 1 &&
          g_asm.leaf[0][0].flags == CANOR_ELITE_S_TOO_LONG);
    CHECK(canor_elite_leaf_text(&g_pga, 0, &g_asm.leaf[0][0], t) == CANOR_OK);
    CHECK(strlen(t) == 102); /* the last text that fitted */
  }

  /* escapes, brackets, and only the first ':' separates */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R,
                   "ket : <a\\|b>\n"
                   "pipe : a \\| b | c\n"
                   "f : g(a|b) | h\n"
                   "t\\:x : y\n"
                   "set : x := 1\n"
                   "ket\npipe\nsay t\\:x\n"));
  CHECK(g_pga.rule_count == 5);
  CHECK(strcmp(g_pga.rule[0].def[0], "<a|b>") == 0);
  CHECK(g_pga.rule[1].def_count == 2 && strcmp(g_pga.rule[1].def[0], "a | b") == 0);
  CHECK(g_pga.rule[2].def_count == 2 && strcmp(g_pga.rule[2].def[0], "g(a|b)") == 0);
  CHECK(strcmp(g_pga.rule[3].spec, "t:x") == 0);
  CHECK(strcmp(g_pga.rule[4].def[0], "x := 1") == 0);
  CHECK(g_pga.line_count == 3 && strcmp(g_pga.line[2].text, "say t:x") == 0);
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(leaf_is(&g_pga, &g_asm, 0, 0, "<a|b>"));
  CHECK(leaf_is(&g_pga, &g_asm, 2, 0, "say y"));

  /* a specification never splits a word; "$" is not a word character */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, "mov : MOV\n$ : go\nmovx mov\n$r0\n"));
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(leaf_is(&g_pga, &g_asm, 0, 0, "movx MOV"));
  CHECK(leaf_is(&g_pga, &g_asm, 1, 0, "gor0"));

  /* L2R rewrites the leftmost site first, R2L the one that ends last */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, "a b : X\nb c : Y\na b c\n"));
  CHECK(parse_into(&g_pgb, CANOR_ELITE_R2L, "a b : X\nb c : Y\na b c\n"));
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(canor_elite_assemble(&g_profile, NULL, &g_pgb, 64, &g_asm2) == CANOR_OK);
  CHECK(leaf_is(&g_pga, &g_asm, 0, 0, "X c"));
  CHECK(leaf_is(&g_pgb, &g_asm2, 0, 0, "a Y"));

  /* an empty definition deletes: a no-op, not an error */
  CHECK(parse_into(&g_pga, CANOR_ELITE_L2R, "nop :\nnop\n"));
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_pga, 64, &g_asm) == CANOR_OK);
  CHECK(g_asm.leaf[0][0].flags == CANOR_ELITE_S_EMPTY && g_asm.branch[0].candidate);
}

static void test_parse_diagnostics(void) {
  char src[1024];
  size_t k, n = 0;
  /* reported with its line number; the good lines around it still count */
  CHECK(canor_elite_program_init(&g_pga, CANOR_ELITE_L2R) == CANOR_OK);
  CHECK(canor_elite_program_parse(&g_pga, "ok : yes\n: nothing\nok\n", 22) ==
        CANOR_ERR_ARGS);
  CHECK(g_pga.diag_count == 1 && g_pga.diag_total == 1);
  CHECK(g_pga.diag[0].line == 2 && g_pga.diag[0].kind == CANOR_ELITE_PARSE_EMPTY_SPEC);
  CHECK(g_pga.rule_count == 1 && g_pga.line_count == 1);
  /* too long */
  CHECK(canor_elite_program_init(&g_pga, CANOR_ELITE_L2R) == CANOR_OK);
  for (k = 0; k < CANOR_ELITE_TEXT_MAX + 10; ++k) src[n++] = 'q';
  src[n++] = '\n';
  CHECK(canor_elite_program_parse(&g_pga, src, n) == CANOR_ERR_RANGE);
  CHECK(g_pga.diag[0].kind == CANOR_ELITE_PARSE_TOO_LONG && g_pga.line_count == 0);
  /* more problems than stored: every one is still counted */
  CHECK(canor_elite_program_init(&g_pga, CANOR_ELITE_L2R) == CANOR_OK);
  n = 0;
  for (k = 0; k < CANOR_ELITE_PARSE_DIAG_MAX + 4; ++k) {
    memcpy(src + n, ": x\n", 4);
    n += 4;
  }
  CHECK(canor_elite_program_parse(&g_pga, src, n) == CANOR_ERR_ARGS);
  CHECK(g_pga.diag_count == CANOR_ELITE_PARSE_DIAG_MAX);
  CHECK(g_pga.diag_total == CANOR_ELITE_PARSE_DIAG_MAX + 4);
  /* capacity */
  CHECK(canor_elite_program_init(&g_pga, CANOR_ELITE_L2R) == CANOR_OK);
  n = 0;
  for (k = 0; k <= CANOR_ELITE_INSN_MAX; ++k) {
    memcpy(src + n, "nop\n", 4);
    n += 4;
  }
  CHECK(canor_elite_program_parse(&g_pga, src, n) == CANOR_ERR_FULL);
  CHECK(g_pga.line_count == CANOR_ELITE_INSN_MAX &&
        g_pga.diag[0].kind == CANOR_ELITE_PARSE_FULL &&
        g_pga.diag[0].line == CANOR_ELITE_INSN_MAX + 1);
  /* builders */
  CHECK(canor_elite_program_define(&g_pga, "", "x", 0) == CANOR_ERR_ARGS);
  CHECK(canor_elite_program_add_line(&g_pga, "", 0) == CANOR_ERR_ARGS);
  CHECK(canor_elite_program_parse(NULL, "x", 1) == CANOR_ERR_ARGS);
}

static void print_worked_example(void) {
  static const char src[] = "$ : mov | jmp | cmp\n"
                            "$ r0, r1\n"
                            "$ r2\n"
                            "store r0, r1\n";
  size_t len;
  CHECK(parse_into(&g_program, CANOR_ELITE_L2R, src));
  CHECK(canor_elite_assemble(&g_profile, &g_accord, &g_program, 64, &g_asm) ==
        CANOR_OK);
  CHECK(canor_elite_record_json(&g_profile, &g_accord, &g_program, &g_asm, g_json,
                                sizeof g_json, &len) == CANOR_OK);
  printf("%s\n", g_json);
  /* and an accord that fails, to show the unanimity error log */
  g_work = g_accord;
  sign_cert(&g_work, SIG_AGENT, SIG_USER, CAT_ANTI_CHEAT,
            CANOR_ELITE_VERDICT_REJECTED);
  g_work.approval[SIG_APP].present = false;
  (void)outcome_of(&g_profile, &g_work);
  CHECK(canor_elite_report_json(&g_work, &g_report, g_json, sizeof g_json, &len) ==
        CANOR_OK);
  printf("%s\n", g_json);
}

int main(int argc, char **argv) {
  fix_profile(&g_profile, true);
  fix_basis(&g_basis);

  test_gram_metric();
  test_encoding();
  test_builders();
  test_unanimous_acceptance(); /* builds g_accord, used below */
  test_bilateral_is_the_two_party_case();
  test_one_missing_consent();
  test_one_rejection_rejects();
  test_out_of_bound_and_collusion();
  test_cheating_claims();
  test_roster_change_requires_reconsent();
  test_unset_profile_is_never_agreed();
  test_diagnostic_bound();
  test_assembler_branches();
  test_assembler_checks();
  test_tools_and_selection();
  test_side_effect_free();
  test_json_buffers();
  test_freeform_source();
  test_parse_diagnostics();

  if (argc > 1 && strcmp(argv[1], "--record") == 0) print_worked_example();

  fprintf(stderr, "canor_test_elite: %d checks, %d failures\n", g_checks,
          g_failures);
  return g_failures ? 1 : 0;
}
