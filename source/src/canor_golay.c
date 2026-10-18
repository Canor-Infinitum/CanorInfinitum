/* The binary Golay code, M24, and the ternary-phase telemetry that hangs off
 * them. See canor_golay.h for what each part is for.
 *
 * The decode table is built by EXHAUSTIVE enumeration of every error pattern
 * of weight <= 3 rather than from a clever syndrome identity. It costs a few
 * milliseconds once, and it makes the perfect-code property a MEASUREMENT: if
 * every one of the 2^11 syndromes is hit exactly once by the 2048 patterns,
 * the code is perfect, and if it is not, the construction is wrong and the
 * build refuses. A closed-form decoder would be faster and could be subtly
 * wrong without ever saying so.
 */

#include "canor_golay.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* --- construction ---------------------------------------------------------- */

static uint32_t g_cw[GOLAY_CODEWORDS];
static uint32_t g_oct[GOLAY_OCTADS];
static int g_noct;
/* uint16_t, not uint8_t: this indexes a 2048-entry table, and an 8-bit index
 * silently wraps at 256 - which made the construction fail its own perfect-code
 * check for a reason that had nothing to do with the code. */
static uint16_t g_leader23[2048];  /* syndrome -> index into g_err23 */
static uint32_t g_err23[2048];     /* coset leaders of the perfect code */
static int g_nerr;
static bool g_ready;

int golay_weight(uint32_t w) {
  int n = 0;
  while (w) { n += (int)(w & 1u); w >>= 1; }
  return n;
}
int golay_distance(uint32_t a, uint32_t b) { return golay_weight(a ^ b); }

/* Cyclic [23,12,7] with g(x) = x^11+x^10+x^6+x^5+x^4+x^2+1, then an overall
 * parity bit at coordinate 23 to extend it. */
#define GPOLY ((1u << 11) | (1u << 10) | (1u << 6) | (1u << 5) | (1u << 4) | (1u << 2) | 1u)

uint32_t golay_encode(uint32_t data12) {
  uint32_t poly = 0;
  int k;
  for (k = 0; k < GOLAY_K; ++k)
    if (data12 & (1u << k)) poly ^= GPOLY << k;
  poly &= 0x7fffffu;
  if (golay_weight(poly) & 1) poly |= (1u << 23);
  return poly;
}

/* Parity-check by remainder mod g(x): the syndrome of the 23-bit part. */
static uint32_t syn23(uint32_t w23) {
  uint32_t r = w23 & 0x7fffffu;
  int i;
  for (i = 22; i >= 11; --i)
    if (r & (1u << i)) r ^= GPOLY << (i - 11);
  return r & 0x7ffu;                 /* 11 bits */
}

uint32_t golay_syndrome(uint32_t word24) {
  /* 11 bits from the cyclic check, plus the overall parity bit: 12 in all,
   * which is what the extended code carries. */
  uint32_t s = syn23(word24);
  if (golay_weight(word24 & 0xffffffu) & 1) s |= (1u << 11);
  return s;
}

bool golay_init(void) {
  uint32_t m;
  int a, b, c, i;
  long hits[2048];

  if (g_ready) return true;

  for (m = 0; m < GOLAY_CODEWORDS; ++m) g_cw[m] = golay_encode(m);

  g_noct = 0;
  for (m = 0; m < GOLAY_CODEWORDS; ++m)
    if (golay_weight(g_cw[m]) == 8) {
      if (g_noct >= GOLAY_OCTADS) return false;      /* too many: wrong code */
      g_oct[g_noct++] = g_cw[m];
    }
  if (g_noct != GOLAY_OCTADS) return false;

  /* Every error pattern of weight <= 3 on 23 bits, and its syndrome. The
   * perfect-code property is exactly the claim that this map is a bijection
   * onto the 2048 syndromes. */
  memset(hits, 0, sizeof hits);
  g_nerr = 0;
  for (a = -1; a < GOLAY_N23; ++a)
    for (b = a; b < GOLAY_N23; ++b)
      for (c = b; c < GOLAY_N23; ++c) {
        uint32_t e = 0;
        uint32_t s;
        if (a >= 0) e |= 1u << a;
        if (b > a && b >= 0) e |= 1u << b;
        if (c > b && c >= 0) e |= 1u << c;
        if (golay_weight(e) > 3) continue;
        s = syn23(e);
        if (hits[s]) {
          /* Two distinct low-weight patterns sharing a syndrome would mean the
           * code is not perfect - i.e. this is not the Golay code. */
          if (g_err23[g_leader23[s]] != e) return false;
          continue;
        }
        hits[s] = 1;
        g_err23[g_nerr] = e;
        g_leader23[s] = (uint16_t)g_nerr;
        ++g_nerr;
      }
  for (i = 0; i < 2048; ++i) if (!hits[i]) return false;   /* not perfect */
  if (g_nerr != 2048) return false;

  g_ready = true;
  return true;
}

const uint32_t *golay_codewords(void) { return g_ready ? g_cw : NULL; }
const uint32_t *golay_octads(void) { return g_ready ? g_oct : NULL; }

uint32_t golay_snap23(uint32_t word23, int *moved) {
  uint32_t e;
  if (!g_ready && !golay_init()) { if (moved) *moved = -1; return word23; }
  word23 &= 0x7fffffu;
  e = g_err23[g_leader23[syn23(word23)]];
  if (moved) *moved = golay_weight(e);
  return word23 ^ e;
}

bool golay_decode24(uint32_t word24, uint32_t *codeword, int *moved) {
  int best = 99, bestn = 0, i;
  uint32_t bestcw = 0;
  if (!g_ready && !golay_init()) return false;
  word24 &= 0xffffffu;
  /* Exhaustive over 4096 codewords. At distance 4 the nearest is not unique,
   * and reporting that as a tie is the extended code's actual guarantee -
   * corrects 3, DETECTS 4 - so a unique-nearest search is the honest test. */
  for (i = 0; i < GOLAY_CODEWORDS; ++i) {
    int d = golay_distance(word24, g_cw[i]);
    if (d < best) { best = d; bestcw = g_cw[i]; bestn = 1; }
    else if (d == best) ++bestn;
  }
  if (best > 3 || bestn != 1) return false;      /* detected, not correctable */
  if (codeword) *codeword = bestcw;
  if (moved) *moved = best;
  return true;
}

/* --- M24 ------------------------------------------------------------------- */

/* alpha: x -> x+1 mod 23, fixing 23. beta: x -> 2x. gamma: x -> -1/x.
 * delta: x -> x^3/9 on quadratic residues, 9x^3 on non-residues - the unique
 * member of that family which preserves the code. */
static uint8_t g_perm[4][24];
static bool g_perm_ready;

static int inv23m(int a) {
  int i;
  for (i = 1; i < 23; ++i) if ((a * i) % 23 == 1) return i;
  return 0;
}
static int is_qr23(int a) {
  int i;
  if (a == 0) return 0;
  for (i = 1; i < 23; ++i) if ((i * i) % 23 == a) return 1;
  return 0;
}

static void m24_build(void) {
  int x;
  if (g_perm_ready) return;
  for (x = 0; x < 23; ++x) g_perm[M24_ALPHA][x] = (uint8_t)((x + 1) % 23);
  g_perm[M24_ALPHA][23] = 23;
  for (x = 0; x < 23; ++x) g_perm[M24_BETA][x] = (uint8_t)((2 * x) % 23);
  g_perm[M24_BETA][23] = 23;
  for (x = 1; x < 23; ++x) g_perm[M24_GAMMA][x] = (uint8_t)((23 - inv23m(x)) % 23);
  g_perm[M24_GAMMA][0] = 23;
  g_perm[M24_GAMMA][23] = 0;
  for (x = 1; x < 23; ++x) {
    int cube = (x * x % 23) * x % 23;
    g_perm[M24_DELTA][x] = (uint8_t)(is_qr23(x) ? cube * 18 % 23 : cube * 9 % 23);
  }
  g_perm[M24_DELTA][0] = 0;
  g_perm[M24_DELTA][23] = 23;
  g_perm_ready = true;
}

const uint8_t *m24_generator(m24_gen g) {
  m24_build();
  return ((int)g >= 0 && (int)g < 4) ? g_perm[(int)g] : NULL;
}
const char *m24_generator_name(m24_gen g) {
  return g == M24_ALPHA ? "alpha (x+1)" :
         g == M24_BETA ? "beta (2x)" :
         g == M24_GAMMA ? "gamma (-1/x)" : "delta (x^3/9 | 9x^3)";
}

uint32_t m24_apply(const uint8_t *perm, uint32_t word) {
  uint32_t out = 0;
  int i;
  for (i = 0; i < 24; ++i) if (word & (1u << i)) out |= 1u << perm[i];
  return out;
}

bool m24_preserves(const uint8_t *perm) {
  int i, j;
  if (!g_ready && !golay_init()) return false;
  for (i = 0; i < GOLAY_CODEWORDS; ++i) {
    uint32_t img = m24_apply(perm, g_cw[i]);
    int found = 0;
    /* The image must be a codeword. Linear-scan; called rarely. */
    for (j = 0; j < GOLAY_CODEWORDS; ++j) if (g_cw[j] == img) { found = 1; break; }
    if (!found) return false;
  }
  return true;
}


/* --- the M24 quantizer ------------------------------------------------------
 * Orbit computation by breadth-first search under the four generators. Forward
 * generators only: the group is finite, so repeated application reaches every
 * inverse and adding them would only duplicate work.
 *
 * The visited set is a 2^24 byte array, 16 MB, allocated per call and freed.
 * That is deliberate rather than cached: the orbit of a single word is the
 * only thing most callers need, and a persistent 16 MB table would be the
 * largest allocation in the whole core for the sake of a function that runs
 * a handful of times.
 */

static long m24_orbit_bfs(uint32_t start, uint32_t *canonical,
                          unsigned char *vis, uint32_t *queue, long cap) {
  long head = 0, tail = 0, count = 0;
  uint32_t best = start;
  int g;
  const uint8_t *perm[4];
  for (g = 0; g < 4; ++g) perm[g] = m24_generator((m24_gen)g);

  start &= 0xffffffu;
  vis[start] = 1;
  queue[tail++] = start;
  ++count;
  while (head < tail) {
    uint32_t cur = queue[head++];
    if (cur < best) best = cur;
    for (g = 0; g < 4; ++g) {
      uint32_t nxt = m24_apply(perm[g], cur);
      if (!vis[nxt]) {
        vis[nxt] = 1;
        if (tail >= cap) return -1;
        queue[tail++] = nxt;
        ++count;
      }
    }
  }
  if (canonical) *canonical = best;
  return count;
}

uint32_t m24_canonical(uint32_t word24, long *size) {
  unsigned char *vis;
  uint32_t *queue, best = 0;
  long n;
  const long CAP = 1000000;
  vis = (unsigned char *)calloc((size_t)1 << 24, 1);
  queue = (uint32_t *)malloc((size_t)CAP * sizeof *queue);
  if (!vis || !queue) {
    free(vis); free(queue);
    if (size) *size = 0;
    return 0;                     /* refuse, rather than pass the input back */
  }
  n = m24_orbit_bfs(word24, &best, vis, queue, CAP);
  free(vis);
  free(queue);
  if (size) *size = n;
  return n > 0 ? best : 0;
}

bool m24_orbits_of_weight(int weight, m24_orbit_census *out) {
  unsigned char *vis;
  uint32_t *queue;
  const long CAP = 1000000;
  uint32_t w;
  bool ok = true;

  if (!out || weight < 0 || weight > 24) return false;
  memset(out, 0, sizeof *out);
  out->weight = weight;

  vis = (unsigned char *)calloc((size_t)1 << 24, 1);
  queue = (uint32_t *)malloc((size_t)CAP * sizeof *queue);
  if (!vis || !queue) { free(vis); free(queue); return false; }

  /* Sweep every word of the requested weight. A word already visited belongs
   * to an orbit that has been counted, so each orbit is discovered exactly
   * once, from its numerically smallest unvisited member. */
  for (w = 0; w < (1u << 24); ++w) {
    long n;
    uint32_t rep = 0;
    if (golay_weight(w) != weight) continue;
    if (vis[w]) continue;
    if (out->norbits >= M24_MAX_ORBITS) { ok = false; break; }
    n = m24_orbit_bfs(w, &rep, vis, queue, CAP);
    if (n < 0) { ok = false; break; }
    out->size[out->norbits] = n;
    out->rep[out->norbits] = rep;
    ++out->norbits;
    out->total += n;
  }
  free(vis);
  free(queue);
  return ok;
}


/* --- provenance stamps ------------------------------------------------------ */

uint64_t prov_digest(const void *data, size_t n) {
  /* FNV-1a, 64-bit. Deterministic across machines, no floating point, and
   * already the hashing style the rest of the core uses. Not cryptographic -
   * see the header. */
  const unsigned char *p = (const unsigned char *)data;
  uint64_t h = 1469598103934665603ull;
  size_t i;
  for (i = 0; i < n; ++i) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ull;
  }
  return h;
}

bool prov_stamp(const void *data, size_t n, char *out, int cap) {
  uint64_t h;
  int b;
  if (!out || cap < PROV_STAMP_CHARS) return false;
  if (!golay_init()) return false;
  h = prov_digest(data, n);
  for (b = 0; b < PROV_BLOCKS; ++b) {
    /* Six 12-bit chunks hold 72 bits; the digest is 64, so the last chunk is
     * padded with zeros. Padding rather than truncating: a truncated digest
     * would quietly weaken the check by 8 bits. */
    uint32_t chunk = (uint32_t)((h >> (b * 12)) & 0xfffu);
    uint32_t code = golay_encode(chunk);
    int k;
    for (k = 0; k < 6; ++k) {
      int nib = (int)((code >> (20 - 4 * k)) & 0xfu);
      out[b * 6 + k] = (char)(nib < 10 ? '0' + nib : 'a' + nib - 10);
    }
  }
  out[PROV_BLOCKS * 6] = 0;
  return true;
}

static int hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool prov_verify(const void *data, size_t n, const char *stamp, prov_result *r) {
  uint64_t recovered = 0;
  int b;
  if (!r) return false;
  memset(r, 0, sizeof *r);
  if (!stamp || !golay_init()) { r->stamp_unreadable = true; return false; }

  for (b = 0; b < PROV_BLOCKS; ++b) {
    uint32_t word = 0, code = 0;
    int k, moved = 0;
    for (k = 0; k < 6; ++k) {
      int v = hexval(stamp[b * 6 + k]);
      if (v < 0) { r->stamp_unreadable = true; return false; }
      word = (word << 4) | (uint32_t)v;
    }
    if (!golay_decode24(word, &code, &moved)) {
      /* 4 or more errors in this block: detected, and refused rather than
       * guessed at. A guess here would produce a confident wrong digest. */
      r->stamp_unreadable = true;
      return false;
    }
    if (moved > 0) { r->stamp_repaired = true; r->bits_corrected += moved; }
    /* The low 12 bits of a codeword are the data chunk, since golay_encode
     * XORs shifted copies of g(x) and g(x) has constant term 1. Recover the
     * chunk by re-encoding candidates is unnecessary: the systematic part is
     * recoverable by dividing out, but a 4096-entry reverse scan is simpler
     * and cannot be subtly wrong. */
    {
      uint32_t d;
      int found = 0;
      for (d = 0; d < GOLAY_CODEWORDS; ++d)
        if (golay_encode(d) == code) { recovered |= ((uint64_t)d) << (b * 12); found = 1; break; }
      if (!found) { r->stamp_unreadable = true; return false; }
    }
  }
  r->expected = recovered & 0xffffffffffffffffull;
  r->actual = prov_digest(data, n);
  /* Only 64 of the 72 recovered bits are digest; the top 8 are the pad. */
  r->ok = ((recovered & 0xffffffffffffffffull) == r->actual);
  return r->ok;
}

/* --- temporal telemetry ----------------------------------------------------- */

void vt_reset(vt_op *op) {
  int i;
  memset(op, 0, sizeof *op);
  for (i = 0; i < 3; ++i) { op->p[i].min = 0.0; op->p[i].max = 0.0; }
}

void vt_observe(vt_op *op, vt_phase ph, double ms) {
  vt_stats *s = &op->p[(int)ph];
  double d, d2;
  if (!op || (int)ph < 0 || (int)ph > 2) return;
  ++s->count;
  d = ms - s->mean;
  s->mean += d / (double)s->count;
  d2 = ms - s->mean;
  s->m2 += d * d2;                        /* Welford: no history retained */
  if (s->count == 1) { s->min = s->max = ms; }
  else { if (ms < s->min) s->min = ms; if (ms > s->max) s->max = ms; }
  s->last = ms;

  /* Roll the 23-bit window on the POST phase only: post measures the machine,
   * and a window mixing user deliberation with machine response would snap to
   * a codeword that meant nothing in particular. */
  if (ph == VT_POST) {
    int slow = (s->count > 1 && ms > s->mean) ? 1 : 0;
    op->window = ((op->window << 1) | (uint32_t)slow) & 0x7fffffu;
    if (op->window_n < GOLAY_N23) ++op->window_n;
  }
}

double vt_mean(const vt_op *op, vt_phase ph) { return op->p[(int)ph].mean; }

double vt_stddev(const vt_op *op, vt_phase ph) {
  const vt_stats *s = &op->p[(int)ph];
  if (s->count < 2) return 0.0;
  return sqrt(s->m2 / (double)s->count);
}

double vt_jitter(const vt_op *op, vt_phase ph) {
  const vt_stats *s = &op->p[(int)ph];
  if (s->count < 2 || s->mean <= 0.0) return 0.0;
  return vt_stddev(op, ph) / s->mean;
}

void vt_forecast_of(const vt_op *op, vt_forecast *out) {
  double total = 0.0, var = 0.0, jt = 0.0, sd;
  int64_t nmin = -1;
  int i;
  memset(out, 0, sizeof *out);
  for (i = 0; i < 3; ++i) {
    double m = op->p[i].mean, j = vt_jitter(op, (vt_phase)i);
    double sdi = vt_stddev(op, (vt_phase)i);
    out->per_phase[i] = m;
    out->jitter[i] = j;
    total += m;
    var += sdi * sdi;                    /* phases treated as independent */
    jt += j;
    if (nmin < 0 || op->p[i].count < nmin) nmin = op->p[i].count;
  }
  sd = sqrt(var);
  out->expected_ms = total;
  out->lo_ms = total - 1.96 * sd;
  if (out->lo_ms < 0.0) out->lo_ms = 0.0;   /* a negative latency is not a forecast */
  out->hi_ms = total + 1.96 * sd;
  out->n = nmin < 0 ? 0 : nmin;
  out->confidence = ((double)out->n / ((double)out->n + 8.0)) * (1.0 / (1.0 + jt));
}

uint32_t vt_window_snap(const vt_op *op, int *moved) {
  return golay_snap23(op->window, moved);
}

bool vt_similar(const vt_op *a, const vt_op *b, int *distance) {
  int ma = 0, mb = 0, d;
  uint32_t sa = golay_snap23(a->window, &ma);
  uint32_t sb = golay_snap23(b->window, &mb);
  d = golay_distance(a->window, b->window);
  if (distance) *distance = d;
  /* Equal snapped windows put the raw windows within 3+3 by the triangle
   * inequality - a similarity claim that carries its own radius. */
  return sa == sb;
}
