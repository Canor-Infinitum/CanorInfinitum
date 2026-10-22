#ifndef CANOR_GOLAY_H
#define CANOR_GOLAY_H

/* The binary Golay code, M24, and what they are actually good for here.
 *
 * TWO CODES, AND THE DIFFERENCE MATTERS.
 *
 *   [23,12,7]  the PERFECT code. Every one of the 2^23 words lies within
 *              distance 3 of exactly one codeword, and the count is exact:
 *              C(23,0)+C(23,1)+C(23,2)+C(23,3) = 1+23+253+1771 = 2048 = 2^11,
 *              which is precisely the number of syndromes. Nothing is left
 *              over and nothing is double-covered. That is what makes
 *              "snap to the nearest codeword" a BOUNDED operation rather than
 *              a heuristic - the snap moves a word by at most 3 bits, always,
 *              with no input on which it can do worse.
 *
 *   [24,12,8]  the EXTENDED code. Not perfect: 2325 words of weight <= 3
 *              against 4096 syndromes, so it corrects 3 and DETECTS 4 rather
 *              than correcting everything. What it gains is M24 - its
 *              automorphism group - and the 759 octads.
 *
 * Both are here, labelled, because using the extended code and claiming
 * perfection is the easy mistake and it would silently weaken every bound
 * built on top.
 *
 * WHAT THIS IS USED FOR IN THIS SYSTEM
 *
 *   BOUNDED SMOOTHING. golay_snap maps any 23-bit window onto its nearest
 *   codeword. The guarantee is hard: at most 3 bits move. A smoother with a
 *   proven displacement bound is worth more here than one that is usually
 *   gentler, because the bound composes - a pipeline of snaps cannot drift
 *   further than the sum of its bounds, and an unbounded smoother's error
 *   cannot be reasoned about at all.
 *
 *   SIMILARITY. Two words that decode to the same codeword are within
 *   distance 6 of each other (3 + 3, triangle inequality). So the codeword is
 *   a locality-sensitive bucket with a PROVEN radius, not an empirical one.
 *   The 12-bit syndrome is the complementary handle: equal syndromes mean the
 *   difference is a codeword, hence distance 0 or >= 8 - never in between.
 *
 *   TELEMETRY. Phase classifications quantised into a 23-bit window, snapped,
 *   and compared. See the telemetry section below.
 *
 * WHAT IT IS NOT USED FOR: see the note on McEliece at the bottom of this
 * header before building anything cryptographic on it.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canor_core.h"
#include "canor_timing.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GOLAY_N23 23
#define GOLAY_N24 24
#define GOLAY_K 12
#define GOLAY_CODEWORDS 4096
#define GOLAY_OCTADS 759

/* Builds both codes and the decode table. Idempotent; returns false only if
 * the construction fails its own checks, which would mean the generator
 * polynomial or the extension is wrong - so a caller that ignores this is
 * ignoring the one signal that the code is not the Golay code. */
bool golay_init(void);

/* The 4096 codewords of the extended [24,12,8] code, as 24-bit masks. */
const uint32_t *golay_codewords(void);
/* The 759 weight-8 codewords - the octads. */
const uint32_t *golay_octads(void);

uint32_t golay_encode(uint32_t data12);          /* -> 24-bit codeword */
int golay_weight(uint32_t w);
int golay_distance(uint32_t a, uint32_t b);

/* Nearest codeword of the PERFECT [23,12,7] code, and the number of bits it
 * moved. `moved` is always <= 3; a value of 4 would mean the perfect-code
 * property had failed and is reported rather than silently accepted. */
uint32_t golay_snap23(uint32_t word23, int *moved);

/* Decode the extended code. Returns false when the word is at distance 4 from
 * the code - detected but not correctable. Refusing there is the point of the
 * extended code; "correcting" it would be inventing a codeword. */
bool golay_decode24(uint32_t word24, uint32_t *codeword, int *moved);

/* The 12-bit syndrome. Equal syndromes => the difference is a codeword, so the
 * two words are at distance 0 or >= 8, never between. */
uint32_t golay_syndrome(uint32_t word24);

/* --- M24 ------------------------------------------------------------------
 * The four verified generators on 24 points, 0-based. Each preserves the
 * extended code; together they are 5-transitive, and a 5-transitive subgroup
 * of Aut(Golay) is M24 itself. */
typedef enum { M24_ALPHA = 0, M24_BETA = 1, M24_GAMMA = 2, M24_DELTA = 3 } m24_gen;
const uint8_t *m24_generator(m24_gen g);
const char *m24_generator_name(m24_gen g);
/* Applies a permutation to a 24-bit word. */
uint32_t m24_apply(const uint8_t *perm, uint32_t word);
/* Does this permutation preserve the code? The membership test for M24. */
bool m24_preserves(const uint8_t *perm);

/* --- the M24 quantizer ----------------------------------------------------
 * Quantisation by group orbit: two descriptors that M24 maps to one another
 * are the SAME under the symmetry, so collapsing each orbit to a canonical
 * representative is a lossless quotient with respect to that symmetry and a
 * lossy one with respect to nothing else. The compression at each weight is
 * therefore not tunable - it is whatever the group says.
 *
 * The behaviour is fixed in advance by 5-transitivity. M24 is exactly
 * 5-transitive, so on subsets of size 1..5 it is transitive and every such
 * subset collapses to ONE bucket; at size 6 transitivity runs out and the
 * orbits must split. A quantizer that reported more than one bucket below
 * weight 6, or only one at weight 6, would be wrong - which is what makes
 * this checkable rather than merely plausible.
 *
 * The canonical representative is the numerically smallest word in the orbit,
 * so it depends on the labelling but not on the search order. */

/* The orbit search's working storage, placed by the caller (AGENTS.md
 * invariant 2: no heap in the core). `visited` is a bitset over all 2^24
 * words - CANOR_GOLAY_M24_VISITED_WORDS uint64_t, 2 MB - and is cleared by
 * every call. `queue` holds the BFS frontier; CANOR_GOLAY_M24_QUEUE_MAX is
 * enough for every orbit the census can meet and is the recommended size. */
#define CANOR_GOLAY_M24_WORDS ((uint32_t)1 << GOLAY_N24)
#define CANOR_GOLAY_M24_VISITED_WORDS (CANOR_GOLAY_M24_WORDS / 64u)
#ifndef CANOR_GOLAY_M24_QUEUE_MAX
#define CANOR_GOLAY_M24_QUEUE_MAX 1000000
#endif
typedef struct {
  uint64_t *visited;          /* CANOR_GOLAY_M24_VISITED_WORDS entries */
  uint32_t *queue;
  size_t queue_cap;
} canor_golay_m24_ws_t;

/* Canonical representative of word's M24 orbit, with the orbit size.
 * CANOR_ERR_ARGS for a missing workspace; CANOR_ERR_FULL when the orbit
 * outgrows ws->queue_cap. On any error *canonical and *size are left unset -
 * a quantizer that silently passed its input through would look like an
 * identity bucket. */
canor_status m24_canonical(uint32_t word24, const canor_golay_m24_ws_t *ws,
                           uint32_t *canonical, long *size);

#define M24_MAX_ORBITS 16
typedef struct {
  int weight;
  int norbits;
  long size[M24_MAX_ORBITS];
  uint32_t rep[M24_MAX_ORBITS];
  long total;
} m24_orbit_census;

/* Decomposes all C(24,w) subsets of the given weight into M24 orbits.
 * CANOR_ERR_FULL when there are more than M24_MAX_ORBITS orbits or one
 * outgrows the queue; the census then holds what was found so far. */
canor_status m24_orbits_of_weight(int weight, const canor_golay_m24_ws_t *ws,
                                  m24_orbit_census *out);


/* --- provenance stamps ----------------------------------------------------
 * Everything this system writes out is an artifact someone will later read
 * back and draw a conclusion from. A conclusion drawn from a silently
 * corrupted file is worse than no conclusion, because nothing about it looks
 * wrong. So every emission carries a stamp.
 *
 * THE SHAPE. A 64-bit digest of the payload, split into six 12-bit chunks,
 * each Golay-encoded to 24 bits, rendered as hex. 144 bits in all.
 *
 * WHY ENCODE THE DIGEST RATHER THAN THE FILE. Golay costs 100% overhead - 12
 * parity bits per 12 data bits - which is absurd for bulk data and exactly
 * right for a 64-bit digest. What it buys is that the INTEGRITY RECORD is
 * itself error-correctable: up to 3 bit-flips per 24-bit block are repaired,
 * so a damaged stamp does not raise a false alarm about an intact file. A
 * plain checksum has the opposite failure mode - corrupt the checksum and the
 * good file is condemned.
 *
 * SIDECAR, NOT IN-BAND. The stamp goes in `<artifact>.golay`, not inside the
 * artifact. In-band would mean the digest cannot cover itself, which forces
 * either a placeholder dance or a format that is no longer valid JSON. A
 * sidecar keeps the artifact byte-identical to what the algorithm produced.
 *
 * WHAT THIS IS NOT. The digest is FNV-1a, which is not cryptographic. This
 * detects accidental corruption - a truncated write, a bad disk, a mangled
 * transfer - and nothing else. It is not tamper-evidence: anyone who edits the
 * file can recompute the stamp. Tamper-evidence needs a MAC or a signature and
 * a key to go with it, which is a different mechanism with different
 * obligations, and calling this one "integrity" without saying so would invite
 * exactly the wrong reliance. */

#define PROV_BLOCKS 6
#define PROV_STAMP_CHARS (PROV_BLOCKS * 6 + 1)   /* 6 hex digits per 24-bit block */

typedef struct {
  bool ok;               /* payload digest matches the stamp */
  bool stamp_repaired;   /* the stamp itself needed correcting */
  int bits_corrected;    /* total bit-flips repaired inside the stamp */
  bool stamp_unreadable; /* a block was past correcting: 4+ errors */
  uint64_t expected, actual;
} prov_result;

uint64_t prov_digest(const void *data, size_t n);
/* Writes PROV_STAMP_CHARS bytes including the terminator. */
bool prov_stamp(const void *data, size_t n, char *out, int cap);
/* Verifies, repairing the stamp in memory if it needs it. */
bool prov_verify(const void *data, size_t n, const char *stamp, prov_result *r);

/* --- temporal telemetry ---------------------------------------------------
 * The ternary-phase model of temporalis.pas, on the canor_timing library
 * (Canor WD v1 §9 Aether; SCHEMA.md "Timing fields"):
 *
 *   PRE   attention arrives until commitment. Measures the USER.
 *   PERI  commitment until dispatch. Motor, and the most stable.
 *   POST  dispatch until settled. Measures the MACHINE.
 *
 * Averaging the three destroys the signal because they fail for different
 * reasons, so they stay separate: each phase is a fixed-capacity series of
 * canor_timing_sample_t (ops over ticks), CANOR_TIMING_SAMPLES_MAX deep.
 * A full series REFUSES the next sample with CANOR_ERR_FULL - it never drops
 * the oldest silently (AGENTS.md invariants 2-3). The caller resets.
 *
 * INTEGER ONLY (invariant 1). The old Welford/double statistics are replaced
 * by exact sums: means and the jitter coefficient are fixed point over
 * CANOR_ONE, and the 1.96-sigma forecast is a closed integer interval whose
 * bounds are rounded OUTWARD, so it encloses the real-number forecast.
 *
 * Jitter is the COEFFICIENT OF VARIATION, stddev/mean, not raw stddev - an
 * operation that is slow but consistent has a low CV despite a high mean,
 * while one that usually is fast and occasionally stalls has a high CV, and
 * only the second is a problem. (canor_golay_tele_measure gives the op/sec
 * LATENCY/JITTER field3 view of the same samples.)
 *
 * WHAT GOLAY ADDS, precisely. It is an error-correcting code, not an
 * estimator. It gives a BOUNDED quantiser: a window of 23 "was this POST
 * sample slow" bits snapped to its nearest codeword moves by at most 3, so two
 * runs whose windows snap together are provably within 6. */

#define CANOR_GOLAY_TELE_PHASES CANOR_TIMING_AXIS
/* Per-sample duration ceiling: keeps every exact sum inside int64. */
#define CANOR_GOLAY_TELE_TICKS_MAX UINT32_MAX
/* 1.96 sigma as an exact ratio, and the confidence prior n/(n + 8). */
#define CANOR_GOLAY_TELE_Z_NUM 196
#define CANOR_GOLAY_TELE_Z_DEN 100
#define CANOR_GOLAY_TELE_PRIOR_N 8

typedef struct {
  canor_timing_sample_t s[CANOR_GOLAY_TELE_PHASES][CANOR_TIMING_SAMPLES_MAX];
  size_t n[CANOR_GOLAY_TELE_PHASES];
  uint64_t sum_ticks[CANOR_GOLAY_TELE_PHASES];
  uint32_t window;        /* 23-bit rolling history of "was this sample slow" */
  int window_n;
} canor_golay_tele_t;

void canor_golay_tele_reset(canor_golay_tele_t *t);
/* One sample of `ops` operations over `ticks` ticks. CANOR_ERR_ARGS for a bad
 * phase or ops == 0, CANOR_ERR_RANGE for ticks beyond
 * CANOR_GOLAY_TELE_TICKS_MAX, CANOR_ERR_FULL when the phase series is full. */
canor_status canor_golay_tele_observe(canor_golay_tele_t *t, canor_timing_phase ph,
                                      uint64_t ops, uint64_t ticks);
/* LATENCY and JITTER fields (ops/sec) for the three phases, via
 * canor_timing_measure. */
canor_status canor_golay_tele_measure(const canor_golay_tele_t *t, uint64_t hz,
                                      canor_timing_metric_t *out);

/* All durations in milli-ticks (ticks . CANOR_ONE). */
typedef struct {
  int64_t per_phase_milli[CANOR_GOLAY_TELE_PHASES];  /* mean duration, truncated */
  int64_t jitter_milli[CANOR_GOLAY_TELE_PHASES];     /* CV over CANOR_ONE; 0 when n < 2 */
  int64_t expected_milli;                            /* sum of the phase means */
  /* [lo, hi] encloses expected +- 1.96 sigma (phases independent); outward
   * rounding, lo clamped at 0 - a negative latency is not a forecast. */
  int64_t lo_milli, hi_milli;
  int64_t confidence_milli;  /* n/(n+8) . 1/(1+Jtotal), over CANOR_ONE */
  int64_t n;                 /* fewest samples in any phase */
} canor_golay_forecast_t;

/* CANOR_ERR_FULL when an exact intermediate outgrows canor_big. */
canor_status canor_golay_tele_forecast(const canor_golay_tele_t *t, canor_golay_forecast_t *out);

/* The Golay-bounded part: snap the rolling window and report how far it moved.
 * Two telemetries whose snapped windows are equal are within distance 6. */
uint32_t canor_golay_tele_window_snap(const canor_golay_tele_t *t, int *moved);
bool canor_golay_tele_similar(const canor_golay_tele_t *a, const canor_golay_tele_t *b,
                              int *distance);

/* --- ON McELIECE ----------------------------------------------------------
 * Do not build a cryptosystem on this code.
 *
 * McEliece's security rests on the private code being hard to distinguish from
 * a random one. The Golay code is a single, fixed, famous object - there is no
 * family to hide in and nothing to keep secret about which code was chosen. The
 * only secret left would be the permutation, and the permutation group is M24
 * with 244,823,040 elements, which is exhaustible in seconds. The scheme would
 * be broken by enumeration before any structural attack was needed.
 *
 * This is not a parameter-size caveat that a bigger Golay would fix; there is
 * no bigger Golay. Code-based cryptography wants binary GOPPA codes, where the
 * key is a choice from a large family and the parameters are n around 3488 to
 * 6960 - a different construction entirely, not a scaling of this one.
 *
 * What this code IS legitimately good for on the security side is INTEGRITY:
 * detecting and correcting corruption in a telemetry record or an artifact
 * checksum. That is a real use and it makes no secrecy claim. */

#ifdef __cplusplus
}
#endif
#endif /* CANOR_GOLAY_H */
