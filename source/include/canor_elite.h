#ifndef CANOR_ELITE_H
#define CANOR_ELITE_H

/* Elite(TM) Programming & Assembler, and the multilateral Diffeomorphic Basis.
 *
 * Canor WD v1 §7 Elite Programming & Assembler, §6 United Interchange;
 * CanorLinguae Z-Notation Draft §7 (UnitedApproval, BilateralUnitedValidity),
 * §8 (Elite Assembly and Governance), §12 (Provenance, Contracts, Security),
 * §13 (Open Design Decisions).
 *
 * TWO HALVES, ONE CONTRACT
 * ------------------------
 * §7 gives Elite two jobs that share a single object:
 *
 *   "For a bilateral United schema, the agreed Elite vocabulary serves as the
 *    common diffeomorphic basis ... Both signatories can bind their signatures
 *    to the same canonical schema digest, Elite basis/version, provenance
 *    record, and error margin, then assemble locally against that contract."
 *
 * So the BASIS (agreed vocabulary + agreed error envelope) is negotiated once
 * by every signatory - the ACCORD half - and each signatory then ASSEMBLES
 * against it locally - the ASSEMBLER half. The accord is data plus a pure
 * evaluator; the assembler is a pure function from (profile, accord, program)
 * to a branch trace and governance record. Neither half performs any effect.
 *
 * MULTILATERAL, NOT ONLY BILATERAL
 * --------------------------------
 * The Z draft's BilateralUnitedValidity fixes #approvals = 2. The accord here
 * generalizes it to N >= 2 signatories (User / App / Agent, internal or
 * external), with bilateral as the N = 2 case, under a UNANIMITY rule:
 *
 *   1. The basis is ratified by EVERY signatory before anything else counts
 *      ("unanimously agreed upon from beforehand").
 *   2. Every signatory submits, PER CATEGORY of error metric, its allowance
 *      A[s][c] and the measured error E[s][c] of orthonormalizing its own
 *      meta-schema extraction, with the declared method (agent, QML, ...).
 *   3. Every OTHER signatory certifies every such submission: E within A,
 *      A within the basis bound B[c]. No signatory certifies itself.
 *   4. Every signatory approves the same United object digest, object
 *      version, basis digest, provenance and accepted error margin.
 *   The meta-schema is ACCEPTED only when all of 1-4 hold for all parties.
 *   One rejection anywhere rejects it; one pending item leaves it incomplete.
 *
 * The Z draft also requires a.acceptedErrorMargin = b.acceptedErrorMargin for
 * the two approvals. That is kept exactly, and reconciled with per-signatory
 * allowances: the margin each approval signs is the basis's bound vector
 * B[0..C-1], identical for everyone by construction, while each signatory's own
 * allowance A[s][c] sits inside it and is what the others certify.
 *
 * NO REALS. The Z draft types acceptedErrorMargin as R. Here every margin,
 * allowance and measurement is a closed integer interval [lo, hi] in units of
 * 1/scale, where each category declares its own positive scale. That is exact
 * rational arithmetic (AGENTS.md invariant 1), and LO/HI enclosures are how a
 * non-integer measurement is carried without rounding it away.
 *
 * WHAT THE CORE DOES NOT DECIDE
 * -----------------------------
 * Digest algorithms, signature algorithms, trust roots and revocation are
 * profile-defined (WD v1 §6; Z §7, §12; SCHEMA.md "digest algorithm TODO").
 * The core therefore never computes a digest or checks a signature itself: it
 * builds the CANONICAL BYTES of every statement (so all parties sign the same
 * bytes on every host) and calls the profile's digest/verify functions. When
 * the profile leaves one unset, every check that needs it is UNDEFINED and the
 * accord cannot be accepted - nobody is "agreed" by default.
 *
 * ANTI-CHEAT AND ANTI-MALWARE SURFACES
 * ------------------------------------
 * - Certification is reproducible: canor_elite_gram_linf recomputes the
 *   orthonormality error bit-identically on every host, so a certifier checks
 *   rather than trusts (canor_elite_certify_check).
 * - Signatures cover the exact numbers: a changed submission invalidates every
 *   certificate over it.
 * - A certificate that CERTIFIES an out-of-bound submission is itself
 *   diagnosed (collusion surface), and self-certification is refused.
 * - At assembly, a token outside the agreed vocabulary, a wrong arity, or an
 *   effect without an active authorizing contract is diagnosed on its branch.
 *
 * DIAGNOSE, NEVER PRUNE (AGENTS.md invariant 3). Every branch is kept up to the
 * traversal limit; every failed check is a diagnostic with the full bitmask of
 * reasons, not just the first.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- capacities. #ifndef-overridable; the caller places every value. --- */

#ifndef CANOR_ELITE_NAME_MAX
#define CANOR_ELITE_NAME_MAX 32 /* bytes per name, including the NUL */
#endif
#ifndef CANOR_ELITE_BLOB_MAX
#define CANOR_ELITE_BLOB_MAX 64 /* digest / key / signature bytes (512 bits) */
#endif
#ifndef CANOR_ELITE_SIGNATORY_MAX
#define CANOR_ELITE_SIGNATORY_MAX 8
#endif
#ifndef CANOR_ELITE_CATEGORY_MAX
#define CANOR_ELITE_CATEGORY_MAX 8
#endif
#ifndef CANOR_ELITE_VOCAB_MAX
#define CANOR_ELITE_VOCAB_MAX 32
#endif
#ifndef CANOR_ELITE_SETTINGS_MAX
#define CANOR_ELITE_SETTINGS_MAX 8
#endif
#ifndef CANOR_ELITE_CONTRACT_MAX
#define CANOR_ELITE_CONTRACT_MAX 8
#endif
#ifndef CANOR_ELITE_TOOL_MAX
#define CANOR_ELITE_TOOL_MAX 4
#endif
#ifndef CANOR_ELITE_RULE_MAX
#define CANOR_ELITE_RULE_MAX 16
#endif
#ifndef CANOR_ELITE_ALT_MAX
#define CANOR_ELITE_ALT_MAX 8
#endif
#ifndef CANOR_ELITE_INSN_MAX
#define CANOR_ELITE_INSN_MAX 16
#endif
#ifndef CANOR_ELITE_OPERAND_MAX
#define CANOR_ELITE_OPERAND_MAX 4
#endif
#ifndef CANOR_ELITE_BRANCH_MAX
#define CANOR_ELITE_BRANCH_MAX 256
#endif
/* Freeform Elite source (see "the assembler" below). */
#ifndef CANOR_ELITE_TEXT_MAX
#define CANOR_ELITE_TEXT_MAX 128 /* spec, definition or line, with the NUL */
#endif
#ifndef CANOR_ELITE_DEPTH_MAX
#define CANOR_ELITE_DEPTH_MAX 8 /* nested rewrites per line */
#endif
#ifndef CANOR_ELITE_EXPANSION_MAX
#define CANOR_ELITE_EXPANSION_MAX 32 /* interpretations retained per line */
#endif
#ifndef CANOR_ELITE_PARSE_DIAG_MAX
#define CANOR_ELITE_PARSE_DIAG_MAX 16
#endif
/* One component per IMAG basis element: 5 trifectas + one = 16 (SCHEMA.md). */
#ifndef CANOR_ELITE_DIM_MAX
#define CANOR_ELITE_DIM_MAX 16
#endif
#ifndef CANOR_ELITE_VECTOR_MAX
#define CANOR_ELITE_VECTOR_MAX CANOR_ELITE_DIM_MAX
#endif
/* Gram-metric input bounds, chosen so the metric is exact in int64 with no
 * wrap: |component| <= 2^24 and dim <= 16 keep every inner product below 2^53,
 * and scales <= 2^20 keep the remainder product r * cscale below 2^60.
 * Anything larger is CANOR_ERR_RANGE, never a rounded result. */
#ifndef CANOR_ELITE_COMPONENT_MAX
#define CANOR_ELITE_COMPONENT_MAX (INT64_C(1) << 24)
#endif
#ifndef CANOR_ELITE_SCALE_MAX
#define CANOR_ELITE_SCALE_MAX (INT64_C(1) << 20)
#endif
/* The accord evaluator emits AT MOST ONE diagnostic per checked item (carrying
 * every failed flag), so this bound is exact:
 *   certificates + submissions  S*(S-1)*C + S*C = S*S*C
 *   ratifications, approvals, duplicate signatories   3*S
 *   vocabulary words, categories                       V + C
 *   accord-level items (CANOR_ELITE_ITEM_*)            ITEM_COUNT */
#define CANOR_ELITE_ITEM_COUNT 8
#ifndef CANOR_ELITE_DIAG_MAX
#define CANOR_ELITE_DIAG_MAX                                                   \
  (CANOR_ELITE_SIGNATORY_MAX * CANOR_ELITE_SIGNATORY_MAX *                     \
       CANOR_ELITE_CATEGORY_MAX +                                              \
   3 * CANOR_ELITE_SIGNATORY_MAX + CANOR_ELITE_VOCAB_MAX +                     \
   CANOR_ELITE_CATEGORY_MAX + CANOR_ELITE_ITEM_COUNT)
#endif

/* Canonical-encoding sizes (see "Canonical bytes" below), so callers can size
 * buffers at compile time. A string or blob costs a u16 length prefix. */
#define CANOR_ELITE_ENC_STR (2 + CANOR_ELITE_NAME_MAX)
#define CANOR_ELITE_ENC_BLOB (2 + CANOR_ELITE_BLOB_MAX)
#define CANOR_ELITE_ENC_INT 8
#define CANOR_ELITE_BASIS_ENCODED_MAX                                          \
  (2 * CANOR_ELITE_ENC_STR + CANOR_ELITE_ENC_INT + 2 +                         \
   CANOR_ELITE_VOCAB_MAX * (2 * CANOR_ELITE_ENC_STR + 2) +                     \
   CANOR_ELITE_CATEGORY_MAX * (2 * CANOR_ELITE_ENC_STR + 3 * CANOR_ELITE_ENC_INT))
#define CANOR_ELITE_STATEMENT_MAX                                              \
  (8 * CANOR_ELITE_ENC_STR + 4 * CANOR_ELITE_ENC_BLOB +                        \
   8 * CANOR_ELITE_ENC_INT + 4 +                                               \
   CANOR_ELITE_SIGNATORY_MAX * (CANOR_ELITE_ENC_STR + CANOR_ELITE_ENC_BLOB + 2) + \
   CANOR_ELITE_CATEGORY_MAX *                                                  \
       (2 * CANOR_ELITE_ENC_STR + 3 * CANOR_ELITE_ENC_INT))
#define CANOR_ELITE_VECTORS_ENCODED_MAX                                        \
  (CANOR_ELITE_ENC_STR + 2 + CANOR_ELITE_ENC_INT +                             \
   CANOR_ELITE_VECTOR_MAX * CANOR_ELITE_DIM_MAX * CANOR_ELITE_ENC_INT)

/* "No index" marker in diagnostics (signatory, peer, category, site). */
#define CANOR_ELITE_NONE UINT8_C(0xFF)
/* "No branch selected" marker. */
#define CANOR_ELITE_NO_BRANCH (-1)

/* --- basic values --- */

/* Closed integer interval [lo, hi], lo <= hi, in units of 1/scale. */
typedef struct {
  int64_t lo, hi;
} canor_elite_interval_t;

/* Opaque bytes: a digest, a public key / trust-root reference, or a
 * signature. len == 0 means UNSET, and every check that needs it is UNDEFINED. */
typedef struct {
  uint8_t len;
  uint8_t bytes[CANOR_ELITE_BLOB_MAX];
} canor_elite_blob_t;

/* MOT phase D participants. */
typedef enum {
  CANOR_ELITE_ROLE_USER = 0,
  CANOR_ELITE_ROLE_APP = 1,
  CANOR_ELITE_ROLE_AGENT = 2
} canor_elite_role_t;

typedef struct {
  char name[CANOR_ELITE_NAME_MAX];
  canor_elite_role_t role;
  bool external;              /* internal or external part */
  canor_elite_blob_t key;     /* verification key / trust-root reference */
  canor_elite_blob_t profile; /* digest of this signatory's .fwp + .fws */
} canor_elite_signatory_t;

/* --- profile: versions, contracts, and the profile-defined cryptography --- */

typedef enum {
  CANOR_ELITE_SIG_VALID = 0,
  CANOR_ELITE_SIG_INVALID = 1,
  CANOR_ELITE_SIG_UNDEFINED = 2 /* unknown key, revoked root, unsupported... */
} canor_elite_sig_t;

/* Profile-defined functions. Both MUST be pure: no CanorDB or external effect
 * (AGENTS.md invariant 4). NULL means the profile leaves it unset. */
typedef canor_status (*canor_elite_digest_fn)(void *ctx, const uint8_t *msg,
                                              size_t len,
                                              canor_elite_blob_t *out);
typedef canor_elite_sig_t (*canor_elite_verify_fn)(
    void *ctx, const canor_elite_signatory_t *who, const uint8_t *msg,
    size_t len, const canor_elite_blob_t *signature);

typedef struct {
  char name[CANOR_ELITE_NAME_MAX];
  uint32_t version;
} canor_elite_setting_t;

typedef struct {
  char name[CANOR_ELITE_NAME_MAX]; /* .fwp profile id */
  uint32_t version;
  canor_elite_setting_t settings[CANOR_ELITE_SETTINGS_MAX]; /* active .fws */
  uint8_t settings_count;
  char contract[CANOR_ELITE_CONTRACT_MAX][CANOR_ELITE_NAME_MAX]; /* active */
  uint8_t contract_count;
  char digest_algorithm[CANOR_ELITE_NAME_MAX];    /* "" = unset */
  char signature_algorithm[CANOR_ELITE_NAME_MAX]; /* "" = unset */
  canor_elite_digest_fn digest;
  canor_elite_verify_fn verify;
  void *crypto_ctx;
} canor_elite_profile_t;

/* --- the Diffeomorphic Basis: agreed vocabulary + agreed error envelope --- */

typedef struct {
  char token[CANOR_ELITE_NAME_MAX];
  uint8_t arity;                      /* declared operand count */
  bool effect;                        /* '!' effectful: needs a contract */
  char contract[CANOR_ELITE_NAME_MAX]; /* authorizing contract id, "" = none */
} canor_elite_word_t;

/* Built-in error metric: L-infinity distance of the Gram matrix from the
 * identity (canor_elite_gram_linf). Any other metric id is profile-defined and
 * measured outside the core; certify_check then reports UNDEFINED. */
#define CANOR_ELITE_METRIC_GRAM_LINF "gram-linf"

typedef struct {
  char name[CANOR_ELITE_NAME_MAX];   /* e.g. "anti-cheat", "anti-malware" */
  char metric[CANOR_ELITE_NAME_MAX]; /* error-metric id */
  int64_t scale;                     /* > 0; values are exact v/scale */
  canor_elite_interval_t bound;      /* B[c], 0 <= lo <= hi */
} canor_elite_category_t;

typedef struct {
  char name[CANOR_ELITE_NAME_MAX];
  uint32_t version;
  canor_elite_blob_t digest; /* profile digest of canor_elite_basis_encode */
  canor_elite_word_t vocab[CANOR_ELITE_VOCAB_MAX];
  uint8_t vocab_count;
  canor_elite_category_t category[CANOR_ELITE_CATEGORY_MAX];
  uint8_t category_count;
} canor_elite_basis_t;

/* --- the accord: who agreed to what --- */

typedef enum {
  CANOR_ELITE_VERDICT_PENDING = 0,
  CANOR_ELITE_VERDICT_CERTIFIED = 1,
  CANOR_ELITE_VERDICT_REJECTED = 2
} canor_elite_verdict_t;

typedef struct {
  bool present;
  canor_elite_interval_t allowance; /* A[s][c] */
  canor_elite_interval_t measured;  /* E[s][c], an enclosure */
  char method[CANOR_ELITE_NAME_MAX]; /* declared tool: agent, qml, ... */
  canor_elite_blob_t evidence;      /* digest of the published extraction */
} canor_elite_submission_t;

typedef struct {
  canor_elite_verdict_t verdict;
  canor_elite_blob_t signature;
} canor_elite_cert_t;

typedef struct {
  bool present;
  canor_elite_blob_t signature;
} canor_elite_seal_t;

typedef struct {
  canor_elite_basis_t basis;
  canor_elite_blob_t object_digest; /* United meta-schema canonical digest */
  uint32_t object_version;
  canor_elite_blob_t provenance;    /* provenance record digest */
  canor_elite_signatory_t signatory[CANOR_ELITE_SIGNATORY_MAX];
  uint8_t signatory_count;
  canor_elite_seal_t ratification[CANOR_ELITE_SIGNATORY_MAX];
  canor_elite_submission_t submission[CANOR_ELITE_SIGNATORY_MAX]
                                     [CANOR_ELITE_CATEGORY_MAX];
  /* cert[certifier][subject][category]; the diagonal is never used. */
  canor_elite_cert_t cert[CANOR_ELITE_SIGNATORY_MAX][CANOR_ELITE_SIGNATORY_MAX]
                         [CANOR_ELITE_CATEGORY_MAX];
  canor_elite_seal_t approval[CANOR_ELITE_SIGNATORY_MAX];
} canor_elite_accord_t;

/* Overall outcome. Precedence: REJECTED > UNDEFINED > INCOMPLETE > ACCEPTED. */
typedef enum {
  CANOR_ELITE_ACCORD_ACCEPTED = 0,
  CANOR_ELITE_ACCORD_INCOMPLETE = 1,
  CANOR_ELITE_ACCORD_UNDEFINED = 2,
  CANOR_ELITE_ACCORD_REJECTED = 3
} canor_elite_outcome_t;

typedef enum {
  CANOR_ELITE_STAGE_ACCORD = 0, /* index = CANOR_ELITE_ITEM_* */
  CANOR_ELITE_STAGE_VOCAB = 1,  /* index = vocabulary word */
  CANOR_ELITE_STAGE_CATEGORY = 2, /* index = category */
  CANOR_ELITE_STAGE_RATIFY = 3, /* subject = signatory */
  CANOR_ELITE_STAGE_SUBMIT = 4, /* subject, index = category */
  CANOR_ELITE_STAGE_CERTIFY = 5, /* peer = certifier, subject, index */
  CANOR_ELITE_STAGE_APPROVE = 6  /* subject */
} canor_elite_stage_t;

/* Accord-level items (diagnostic index when stage == ACCORD). */
#define CANOR_ELITE_ITEM_SIGNATORIES 0   /* count >= 2, names distinct */
#define CANOR_ELITE_ITEM_DIGEST_ALG 1    /* profile digest algorithm */
#define CANOR_ELITE_ITEM_SIGNATURE_ALG 2 /* profile signature algorithm */
#define CANOR_ELITE_ITEM_OBJECT_DIGEST 3 /* United object digest */
#define CANOR_ELITE_ITEM_PROVENANCE 4    /* provenance record digest */
#define CANOR_ELITE_ITEM_BASIS_DIGEST 5  /* basis digest present and correct */
#define CANOR_ELITE_ITEM_VOCAB 6         /* agreed vocabulary non-empty */
#define CANOR_ELITE_ITEM_CATEGORIES 7    /* error envelope non-empty */

/* Accord diagnostic flags (a diagnostic carries ALL that apply). */
#define CANOR_ELITE_F_PENDING (1u << 0)      /* not yet given */
#define CANOR_ELITE_F_SIG_INVALID (1u << 1)  /* signature does not verify */
#define CANOR_ELITE_F_SIG_UNDEFINED (1u << 2) /* cannot be verified */
#define CANOR_ELITE_F_REJECTED (1u << 3)     /* explicit rejection */
#define CANOR_ELITE_F_OUT_OF_BOUND (1u << 4) /* allowance outside B[c] */
#define CANOR_ELITE_F_OVER_ALLOWANCE (1u << 5) /* measured outside A[s][c] */
#define CANOR_ELITE_F_MALFORMED (1u << 6)    /* lo > hi, scale <= 0, ... */
#define CANOR_ELITE_F_INCONSISTENT (1u << 7) /* claim contradicts evidence */
#define CANOR_ELITE_F_DUPLICATE (1u << 8)    /* repeated name */
#define CANOR_ELITE_F_UNSET (1u << 9)        /* profile-defined item unset */
#define CANOR_ELITE_F_DIGEST_MISMATCH (1u << 10) /* digest != content */
#define CANOR_ELITE_F_TOO_FEW (1u << 11)     /* fewer than two signatories */

typedef struct {
  uint8_t stage;     /* canor_elite_stage_t */
  int8_t status;     /* canor_status */
  uint8_t subject;   /* signatory, or CANOR_ELITE_NONE */
  uint8_t peer;      /* certifier, or CANOR_ELITE_NONE */
  uint8_t index;     /* category or vocabulary index, or CANOR_ELITE_NONE */
  uint16_t flags;
} canor_elite_diag_t;

typedef struct {
  canor_elite_outcome_t outcome;
  uint16_t checked;                 /* items examined */
  uint16_t passed;                  /* items with no diagnostic */
  uint16_t diag_count;
  canor_elite_diag_t diag[CANOR_ELITE_DIAG_MAX];
} canor_elite_report_t;

/* --- the assembler (§7): freeform pattern replacement, branches retained ---
 *
 * FREEFORM RULES (user decision, 2026-10-20). Elite source is line-based and
 * is NOT read through atoms.ebnf; that is what frees "$" and every other glyph
 * for Elite use. Each line, ended by LF or CRLF, is one of:
 *
 *   spec : def | def | ...   a RULE. Whatever precedes the first ':' is the
 *                            specification; whatever follows it, the
 *                            definition(s). Any text at all, either side.
 *   anything else            a PROGRAM LINE, to be assembled.
 *
 * - Any specification may be redefined on any later line. Every definition is
 *   kept, in source order, as one more candidate interpretation (§7 "multiple
 *   interpretation paths"; AGENTS.md invariant 3: an earlier definition is
 *   never silently dropped).
 * - '|' separates definitions, except inside (), [] or {}. "\|" and "\:" are a
 *   literal '|' and ':' (so a bra-ket is written <a\|b>). There are no other
 *   escapes: TeX commands such as \alpha pass through unchanged.
 * - Definitions may contain specifications: replacement is NESTED, one site at
 *   a time, up to CANOR_ELITE_DEPTH_MAX rewrites per line. Going deeper (every
 *   cycle does) leaves a diagnosed interpretation, not a dropped one.
 * - A specification matches where it does not split a word (letters, digits,
 *   '_', any non-ASCII byte): "mov" does not match inside "movx", "$" does
 *   match in "$r0". L2R takes the leftmost-longest site, R2L the rightmost-
 *   longest (the site that ends last).
 * - Once no specification matches, the line reads `head operand, operand, ...`
 *   for the basis checks; top-level commas separate operands.
 */

/* Left/right traversal identity: L2R is the identity order, R2L its reversal
 * (as in Soul). It picks the site rewritten first within a line, and which
 * line is the most significant digit of the branch index. */
typedef enum {
  CANOR_ELITE_L2R = 0,
  CANOR_ELITE_R2L = 1
} canor_elite_direction_t;

/* "$ : mov | jmp | cmp", accumulated over every line that defines "$". */
typedef struct {
  char spec[CANOR_ELITE_TEXT_MAX];
  char def[CANOR_ELITE_ALT_MAX][CANOR_ELITE_TEXT_MAX];
  uint16_t def_line[CANOR_ELITE_ALT_MAX]; /* source line; 0 = builder */
  uint8_t def_count;
} canor_elite_rule_t;

typedef struct {
  char text[CANOR_ELITE_TEXT_MAX];
  uint16_t source_line; /* 0 = builder */
} canor_elite_line_t;

typedef enum {
  CANOR_ELITE_PARSE_EMPTY_SPEC = 0, /* ':' with nothing before it */
  CANOR_ELITE_PARSE_TOO_LONG = 1,   /* longer than CANOR_ELITE_TEXT_MAX - 1 */
  CANOR_ELITE_PARSE_FULL = 2,       /* rule, definition or line capacity */
  CANOR_ELITE_PARSE_MALFORMED = 3   /* e.g. a NUL byte inside a line */
} canor_elite_parse_kind_t;

typedef struct {
  uint16_t line;
  int8_t status; /* canor_status */
  uint8_t kind;  /* canor_elite_parse_kind_t */
} canor_elite_parse_diag_t;

typedef struct {
  canor_elite_rule_t rule[CANOR_ELITE_RULE_MAX];
  uint8_t rule_count;
  canor_elite_line_t line[CANOR_ELITE_INSN_MAX];
  uint8_t line_count;
  canor_elite_direction_t direction;
  canor_elite_parse_diag_t diag[CANOR_ELITE_PARSE_DIAG_MAX];
  uint16_t diag_count; /* stored */
  uint16_t diag_total; /* found; exceeds diag_count only past the capacity */
} canor_elite_program_t;

/* Declared tooling (§7): recorded for reproducibility; the core never calls
 * an external tool. A tool's outputs enter as scores (canor_elite_score). */
typedef enum {
  CANOR_ELITE_TOOL_AGENT = 0,
  CANOR_ELITE_TOOL_LANGUAGE_MODEL = 1,
  CANOR_ELITE_TOOL_STOCHASTIC = 2,
  CANOR_ELITE_TOOL_PROCEDURAL = 3,
  CANOR_ELITE_TOOL_OTHER = 4
} canor_elite_tool_kind_t;

typedef struct {
  char name[CANOR_ELITE_NAME_MAX];
  char version[CANOR_ELITE_NAME_MAX];
  canor_elite_tool_kind_t kind;
  canor_elite_blob_t config; /* digest of the tool configuration */
  uint64_t seed;             /* recorded seed for stochastic tools */
} canor_elite_tool_t;

/* Per-line interpretation flags. */
#define CANOR_ELITE_S_NOT_IN_BASIS (1u << 0) /* head outside agreed vocab */
#define CANOR_ELITE_S_ARITY (1u << 1)     /* operand count != declared arity */
#define CANOR_ELITE_S_EFFECT_UNAUTHORIZED (1u << 2) /* no active contract */
#define CANOR_ELITE_S_EFFECT_RECORDED (1u << 3) /* authorized: recorded only */
#define CANOR_ELITE_S_DEPTH (1u << 4)    /* past CANOR_ELITE_DEPTH_MAX: cycles */
#define CANOR_ELITE_S_TOO_LONG (1u << 5) /* a rewrite would exceed TEXT_MAX */
#define CANOR_ELITE_S_UNCHECKED (1u << 6) /* no basis bound: raw, permissive */
#define CANOR_ELITE_S_EMPTY (1u << 7)    /* rewrote to nothing: a no-op */
/* Flags that make a branch DIAGNOSED rather than a candidate. */
#define CANOR_ELITE_S_ERRORS                                                   \
  (CANOR_ELITE_S_NOT_IN_BASIS | CANOR_ELITE_S_ARITY |                          \
   CANOR_ELITE_S_EFFECT_UNAUTHORIZED | CANOR_ELITE_S_DEPTH |                   \
   CANOR_ELITE_S_TOO_LONG)

/* One interpretation of one program line: the definition chosen at each
 * rewrite. Its text is not stored; canor_elite_leaf_text replays the path. */
typedef struct {
  uint8_t path[CANOR_ELITE_DEPTH_MAX];
  uint8_t depth;
  uint8_t flags; /* CANOR_ELITE_S_* */
} canor_elite_leaf_t;

typedef struct {
  uint64_t id;                          /* mixed-radix rank */
  uint8_t choice[CANOR_ELITE_INSN_MAX]; /* interpretation (leaf) per line */
  uint8_t site_flags[CANOR_ELITE_INSN_MAX];
  uint8_t flags;                        /* union of site_flags */
  bool candidate;                       /* no error flag */
  bool scored;
  int32_t score;                        /* tool output, over CANOR_ONE */
} canor_elite_branch_t;

typedef enum {
  CANOR_ELITE_TERM_EXHAUSTED = 0, /* every interpretation traversed */
  CANOR_ELITE_TERM_LIMIT = 1      /* configured traversal limit reached */
} canor_elite_termination_t;

/* Governance / provenance record (§7, Z §8). An OUTPUT of assembly; persisting
 * it is a separately governed action, so nothing here writes anywhere. */
typedef struct {
  bool basis_bound;
  canor_elite_outcome_t basis_outcome; /* the accord's verdict at assembly */
  uint64_t configured;      /* traversal limit */
  uint64_t consumed;        /* branches traversed and retained */
  uint64_t total;           /* product of retained interpretations per line */
  bool total_overflow;      /* product exceeds 2^64 - 1 */
  bool expansion_limited;   /* a line has more than EXPANSION_MAX: total is a
                               lower bound, and the traversal is incomplete */
  canor_elite_termination_t termination;
  bool complete;            /* termination == EXHAUSTED */
  uint64_t candidates;
  uint64_t diagnosed;
  uint64_t effects_declared; /* authorized effects recorded, never performed */
  int64_t selected;          /* branch position, or CANOR_ELITE_NO_BRANCH */
} canor_elite_record_t;

typedef struct {
  canor_elite_leaf_t leaf[CANOR_ELITE_INSN_MAX][CANOR_ELITE_EXPANSION_MAX];
  uint8_t leaf_count[CANOR_ELITE_INSN_MAX];
  bool leaf_more[CANOR_ELITE_INSN_MAX]; /* more interpretations than stored */
  canor_elite_branch_t branch[CANOR_ELITE_BRANCH_MAX];
  uint16_t branch_count;
  canor_elite_tool_t tool[CANOR_ELITE_TOOL_MAX];
  uint8_t tool_count;
  canor_elite_record_t record;
} canor_elite_assembly_t;

/* ===========================================================================
 * Builders. Each validates its arguments, checks capacity first, and returns
 * CANOR_ERR_ARGS (NULL, empty or duplicate name), CANOR_ERR_RANGE (name too
 * long, bad interval/scale), or CANOR_ERR_FULL.
 * ========================================================================= */

canor_status canor_elite_profile_init(canor_elite_profile_t *p, const char *name,
                                      uint32_t version);
canor_status canor_elite_profile_add_setting(canor_elite_profile_t *p,
                                             const char *name, uint32_t version);
canor_status canor_elite_profile_add_contract(canor_elite_profile_t *p,
                                              const char *contract);

canor_status canor_elite_basis_init(canor_elite_basis_t *b, const char *name,
                                    uint32_t version);
canor_status canor_elite_basis_add_word(canor_elite_basis_t *b, const char *token,
                                        uint8_t arity, bool effect,
                                        const char *contract);
canor_status canor_elite_basis_add_category(canor_elite_basis_t *b,
                                            const char *name, const char *metric,
                                            int64_t scale,
                                            canor_elite_interval_t bound);

canor_status canor_elite_accord_init(canor_elite_accord_t *a,
                                     const canor_elite_basis_t *basis,
                                     const canor_elite_blob_t *object_digest,
                                     uint32_t object_version,
                                     const canor_elite_blob_t *provenance);
/* The ratify and approve statements bind the whole roster: consent is given
 * among exactly these parties. Adding a signatory therefore resets every
 * ratification and approval to pending - everyone re-consents. */
canor_status canor_elite_accord_add_signatory(canor_elite_accord_t *a,
                                              const char *name,
                                              canor_elite_role_t role,
                                              bool external,
                                              const canor_elite_blob_t *key,
                                              const canor_elite_blob_t *profile,
                                              uint8_t *index_out);
canor_status canor_elite_accord_ratify(canor_elite_accord_t *a, uint8_t s,
                                       const canor_elite_blob_t *signature);
/* Re-submitting resets every certificate over (s, c) to PENDING: they signed
 * the old numbers. */
canor_status canor_elite_accord_submit(canor_elite_accord_t *a, uint8_t s,
                                       uint8_t c,
                                       canor_elite_interval_t allowance,
                                       canor_elite_interval_t measured,
                                       const char *method,
                                       const canor_elite_blob_t *evidence);
/* certifier != subject, else CANOR_ERR_ARGS. */
canor_status canor_elite_accord_certify(canor_elite_accord_t *a,
                                        uint8_t certifier, uint8_t subject,
                                        uint8_t c, canor_elite_verdict_t verdict,
                                        const canor_elite_blob_t *signature);
canor_status canor_elite_accord_approve(canor_elite_accord_t *a, uint8_t s,
                                        const canor_elite_blob_t *signature);

canor_status canor_elite_program_init(canor_elite_program_t *p,
                                      canor_elite_direction_t direction);
/* One definition for a specification (text taken exactly, no trimming);
 * defining an existing specification again appends to its definitions. */
canor_status canor_elite_program_define(canor_elite_program_t *p,
                                        const char *spec, const char *def,
                                        uint16_t source_line);
/* Convenience: `symbol : alt[0] | ... | alt[count-1]`, all or nothing. */
canor_status canor_elite_program_add_rule(canor_elite_program_t *p,
                                          const char *symbol,
                                          const char *const *alt, uint8_t count);
canor_status canor_elite_program_add_line(canor_elite_program_t *p,
                                          const char *text,
                                          uint16_t source_line);
/* Convenience: the program line "head op0, op1, ...". */
canor_status canor_elite_program_add_insn(canor_elite_program_t *p,
                                          const char *head,
                                          const char *const *operand,
                                          uint8_t count);
/* Freeform Elite source (len bytes, LF or CRLF lines), appended to p. Every
 * problem is a diagnostic in p->diag with its line number, and parsing goes
 * on past it: the return value is CANOR_OK or the first diagnostic's status. */
canor_status canor_elite_program_parse(canor_elite_program_t *p,
                                       const char *text, size_t len);
/* The text of interpretation `leaf` of program line `line`, replayed from its
 * path into out[CANOR_ELITE_TEXT_MAX]. For a TOO_LONG leaf it is the last
 * text that fitted. CANOR_ERR_RANGE if the leaf does not fit the program. */
canor_status canor_elite_leaf_text(const canor_elite_program_t *p, uint8_t line,
                                   const canor_elite_leaf_t *leaf, char *out);

/* ===========================================================================
 * Canonical bytes. Every party signs exactly these, on every host: fields are
 * length-prefixed (u16 big-endian) or fixed 8-byte big-endian two's
 * complement, after a versioned domain-separation tag. Returns CANOR_ERR_FULL
 * when cap is too small (*len then holds the bytes required).
 * ========================================================================= */

canor_status canor_elite_basis_encode(const canor_elite_basis_t *b, uint8_t *buf,
                                      size_t cap, size_t *len);
canor_status canor_elite_statement_ratify(const canor_elite_accord_t *a,
                                          uint8_t s, uint8_t *buf, size_t cap,
                                          size_t *len);
canor_status canor_elite_statement_cert(const canor_elite_accord_t *a,
                                        uint8_t certifier, uint8_t subject,
                                        uint8_t c, canor_elite_verdict_t verdict,
                                        uint8_t *buf, size_t cap, size_t *len);
canor_status canor_elite_statement_approve(const canor_elite_accord_t *a,
                                           uint8_t s, uint8_t *buf, size_t cap,
                                           size_t *len);
/* Published meta-schema extraction vectors; a submission's `evidence` is the
 * profile digest of exactly these bytes. */
canor_status canor_elite_vectors_encode(const int32_t *v, uint8_t count,
                                        uint8_t dim, int64_t vscale,
                                        uint8_t *buf, size_t cap, size_t *len);

/* ===========================================================================
 * Error metric and certification.
 * ========================================================================= */

/* Orthonormality error of `count` vectors of `dim` components, each component
 * x meaning x / vscale. With G[a][b] = <v_a, v_b> (exact int64),
 *
 *   D = max over a, b of | G[a][b] - (a == b ? vscale^2 : 0) |
 *
 * is reported in units of 1 / cscale as the exact enclosure
 *   [floor(D * cscale / vscale^2), ceil(D * cscale / vscale^2)].
 * CANOR_ERR_RANGE on out-of-bound components, scales, or overflow. */
canor_status canor_elite_gram_linf(const int32_t *v, uint8_t count, uint8_t dim,
                                   int64_t vscale, int64_t cscale,
                                   canor_elite_interval_t *out);

/* What a certifier should sign for submission (subject, c), given the
 * subject's published, orthonormalized vectors. Checks, collecting every
 * failure in *flags:
 *   digest(vectors) == evidence            else DIGEST_MISMATCH
 *   recomputed error within claimed E      else INCONSISTENT
 *   E within allowance A                   else OVER_ALLOWANCE
 *   A within basis bound B                 else OUT_OF_BOUND
 * Returns CANOR_ERR_UNDEFINED (verdict PENDING, flag UNSET) when the profile
 * has no digest function or the category metric is not built in: the
 * certifier must then measure with the profile-defined method itself. */
canor_status canor_elite_certify_check(const canor_elite_profile_t *p,
                                       const canor_elite_accord_t *a,
                                       uint8_t subject, uint8_t c,
                                       const int32_t *v, uint8_t count,
                                       uint8_t dim, int64_t vscale,
                                       canor_elite_verdict_t *verdict,
                                       uint16_t *flags);

/* Pure evaluation of the whole accord. report may be NULL (outcome only). */
canor_status canor_elite_accord_evaluate(const canor_elite_profile_t *p,
                                         const canor_elite_accord_t *a,
                                         canor_elite_report_t *report,
                                         canor_elite_outcome_t *outcome);

/* ===========================================================================
 * Assembly. Side-effect-free: reads its const inputs, writes only *out.
 * accord may be NULL (no basis bound: tokens are raw and UNCHECKED).
 * limit must be in 1..CANOR_ELITE_BRANCH_MAX (else CANOR_ERR_RANGE / _FULL).
 * ========================================================================= */

canor_status canor_elite_assemble(const canor_elite_profile_t *p,
                                  const canor_elite_accord_t *accord,
                                  const canor_elite_program_t *program,
                                  uint32_t limit, canor_elite_assembly_t *out);

canor_status canor_elite_tool_declare(canor_elite_assembly_t *as,
                                      const char *name, const char *version,
                                      canor_elite_tool_kind_t kind,
                                      const canor_elite_blob_t *config,
                                      uint64_t seed);
/* Record a tool's score for branch position `pos`. Scores rank, never prune. */
canor_status canor_elite_score(canor_elite_assembly_t *as, uint16_t pos,
                               int32_t score);
/* Built-in reproducible stochastic tool: score = splitmix64(seed ^ id) mod
 * (CANOR_ONE + 1) for every branch. Same seed, same scores, on every host. */
canor_status canor_elite_score_seeded(canor_elite_assembly_t *as,
                                      uint64_t seed);
/* Re-select: highest score among candidates, ties to the lowest branch id;
 * unscored candidates rank after scored ones, in branch order. */
canor_status canor_elite_select(canor_elite_assembly_t *as);

/* Deterministic JSON governance record (the elite skill's <name>.record.json;
 * United 6.A form). Names pass through byte-for-byte apart from JSON escapes.
 * accord may be NULL. CANOR_ERR_FULL if cap is too small (*len then holds the
 * bytes required); the output is always NUL-terminated when cap > 0. */
canor_status canor_elite_record_json(const canor_elite_profile_t *p,
                                     const canor_elite_accord_t *accord,
                                     const canor_elite_program_t *program,
                                     const canor_elite_assembly_t *as, char *buf,
                                     size_t cap, size_t *len);

/* The accord report as JSON: the unanimity error log, one entry per
 * diagnostic, none dropped. Same buffer contract as canor_elite_record_json. */
canor_status canor_elite_report_json(const canor_elite_accord_t *a,
                                     const canor_elite_report_t *r, char *buf,
                                     size_t cap, size_t *len);

/* Stable identifiers for records and logs. */
const char *canor_elite_status_name(canor_status s);
const char *canor_elite_outcome_name(canor_elite_outcome_t o);

#ifdef __cplusplus
}
#endif

#endif /* CANOR_ELITE_H */
