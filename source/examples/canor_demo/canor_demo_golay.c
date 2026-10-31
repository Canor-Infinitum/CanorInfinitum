/* canor_demo_golay.c - golay - the code, M24, and the telemetry bounds
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* golay - the code, M24, and the telemetry bounds                            */
/* ========================================================================== */

static int gchecks, gfails;
static void gcheck(int cond, const char *what) {
  ++gchecks;
  if (!cond) { ++gfails; printf("  FAIL  %s\n", what); }
}

int canor_demo_cmd_golay(void) {
  const uint32_t *cw, *oct;
  int i, j, moved;
  uint32_t w, snapped;

  printf("Golay code, M24, and the bounds they provide\n\n");
  if (!golay_init()) {
    fprintf(stderr, "follow: the Golay construction failed its own checks\n");
    return 2;
  }
  cw = golay_codewords();
  oct = golay_octads();

  printf("  [24,12,8] extended code   %d codewords\n", GOLAY_CODEWORDS);
  printf("  octads (weight 8)         %d\n", GOLAY_OCTADS);
  {
    long wd[25];
    memset(wd, 0, sizeof wd);
    for (i = 0; i < GOLAY_CODEWORDS; ++i) wd[golay_weight(cw[i])]++;
    printf("  weight distribution      ");
    for (i = 0; i <= 24; ++i) if (wd[i]) printf(" %d:%ld", i, wd[i]);
    printf("\n");
    gcheck(wd[0] == 1 && wd[8] == 759 && wd[12] == 2576 && wd[16] == 759 &&
           wd[24] == 1, "weight distribution is 1/759/2576/759/1");
  }

  /* Minimum distance is 8 - checked, because a construction that quietly
   * produced a weaker code would still print 4096 codewords. */
  {
    int dmin = 99;
    for (i = 1; i < GOLAY_CODEWORDS; ++i) {
      int d = golay_weight(cw[i]);
      if (d && d < dmin) dmin = d;
    }
    printf("  minimum distance          %d\n", dmin);
    gcheck(dmin == 8, "minimum distance of the extended code is 8");
  }

  /* The perfect-code property of [23,12,7], which is what every bound below
   * actually rests on. golay_init refuses to complete unless it holds, so
   * reaching here is already most of the proof; this restates the counting. */
  printf("\n  [23,12,7] perfect code\n");
  printf("    C(23,0..3) = 1+23+253+1771 = %d\n", 1 + 23 + 253 + 1771);
  printf("    syndromes  = 2^11         = %d\n", 2048);
  gcheck(1 + 23 + 253 + 1771 == 2048, "the sphere-packing bound is met exactly");

  /* Snapping never moves more than 3 bits. Exhaustive over a large sample
   * rather than a spot check: the bound is the product, so it is worth the
   * loop. */
  {
    int worst = 0, ok = 1;
    for (w = 0; w < 200000u; ++w) {
      snapped = golay_snap23(w, &moved);
      if (moved < 0 || moved > 3) { ok = 0; break; }
      if (moved > worst) worst = moved;
      if (golay_distance(w & 0x7fffffu, snapped) != moved) { ok = 0; break; }
    }
    printf("\n  bounded smoothing\n");
    printf("    200000 words snapped, worst displacement %d bits\n", worst);
    gcheck(ok, "every snap moves at most 3 bits, and moved == actual distance");
    gcheck(worst == 3, "and 3 is attained, so the bound is tight not merely safe");
  }

  /* Similarity with a proven radius. */
  {
    int same = 0, viol = 0;
    for (i = 0; i < 4000; ++i) {
      uint32_t a = (uint32_t)(i * 2654435761u) & 0x7fffffu;
      uint32_t b = a ^ (uint32_t)((i * 40503u) & 0x1fu);   /* small perturbation */
      int ma, mb;
      if (golay_snap23(a, &ma) == golay_snap23(b, &mb)) {
        ++same;
        if (golay_distance(a, b) > 6) ++viol;
      }
    }
    printf("\n  similarity\n");
    printf("    %d of 4000 perturbed pairs snap together\n", same);
    gcheck(viol == 0, "pairs snapping together are within distance 6, always");
  }

  /* Equal syndrome => difference is a codeword => distance 0 or >= 8. */
  {
    int viol = 0;
    for (i = 0; i < 3000; ++i) {
      uint32_t a = (uint32_t)(i * 2246822519u) & 0xffffffu;
      uint32_t b = a ^ cw[(i * 7919) % GOLAY_CODEWORDS];
      int d = golay_distance(a, b);
      if (golay_syndrome(a) != golay_syndrome(b)) { ++viol; continue; }
      if (d != 0 && d < 8) ++viol;
    }
    gcheck(viol == 0, "equal syndromes mean distance 0 or >= 8, never between");
  }

  /* The extended code detects weight-4 error rather than mis-correcting it. */
  {
    int detected = 0, tried = 0;
    for (i = 0; i < 500; ++i) {
      uint32_t base = cw[(i * 131) % GOLAY_CODEWORDS];
      uint32_t e = 0;
      int bits = 0, k = i;
      while (bits < 4) {
        uint32_t bit = 1u << (k % 24);
        k = k * 31 + 7;
        if (e & bit) continue;
        e |= bit;
        ++bits;
      }
      ++tried;
      if (!golay_decode24(base ^ e, NULL, NULL)) ++detected;
    }
    printf("\n  extended code at weight 4\n");
    printf("    %d of %d refused rather than mis-corrected\n", detected, tried);
    gcheck(detected == tried, "weight-4 error is DETECTED, never silently corrected");
  }

  /* M24: the four generators, each verified against the code. */
  printf("\n  M24 generators, each tested against the code\n");
  for (i = 0; i < 4; ++i) {
    const uint8_t *g = m24_generator((m24_gen)i);
    int ok = m24_preserves(g);
    printf("    %-22s preserves the code: %s\n",
           m24_generator_name((m24_gen)i), ok ? "YES" : "NO");
    gcheck(ok, "generator preserves the extended Golay code");
  }
  /* M24 permutes the octads among themselves - a stronger statement than
   * preserving the code, and the one the Excitation metrics rely on. */
  {
    int ok = 1;
    const uint8_t *g = m24_generator(M24_DELTA);
    for (i = 0; i < GOLAY_OCTADS && ok; ++i) {
      uint32_t img = m24_apply(g, oct[i]);
      int found = 0;
      for (j = 0; j < GOLAY_OCTADS; ++j) if (oct[j] == img) { found = 1; break; }
      if (!found) ok = 0;
    }
    gcheck(ok, "delta permutes the 759 octads among themselves");
  }

  /* Telemetry: the ternary-phase model with a Golay-bounded window. Samples
   * are one operation over a whole number of millisecond ticks; every
   * reported figure is exact integer arithmetic in milli-units. */
  {
    static canor_golay_tele_t a, b;
    canor_golay_forecast_t f;
    int st = CANOR_OK;
    canor_golay_tele_reset(&a);
    canor_golay_tele_reset(&b);
    for (i = 0; i < 60 && st == CANOR_OK; ++i) {
      uint64_t base = 80u + (uint64_t)((i * 37) % 11);
      st |= canor_golay_tele_observe(&a, CANOR_TIMING_PRE, 1, 200u + (uint64_t)((i * 13) % 90));
      st |= canor_golay_tele_observe(&a, CANOR_TIMING_PERI, 1, 45u + (uint64_t)((i * 7) % 5));
      st |= canor_golay_tele_observe(&a, CANOR_TIMING_POST, 1, base + ((i % 9 == 0) ? 400u : 0u));
      st |= canor_golay_tele_observe(&b, CANOR_TIMING_PRE, 1, 205u + (uint64_t)((i * 13) % 90));
      st |= canor_golay_tele_observe(&b, CANOR_TIMING_PERI, 1, 46u + (uint64_t)((i * 7) % 5));
      st |= canor_golay_tele_observe(&b, CANOR_TIMING_POST, 1, base + ((i % 9 == 0) ? 380u : 0u));
    }
    gcheck(st == CANOR_OK, "60 samples per phase fit the fixed series");
    gcheck(canor_golay_tele_forecast(&a, &f) == CANOR_OK, "the forecast is computed exactly");
    printf("\n  temporal telemetry (ternary phases, exact sums, CV jitter)\n");
    printf("    pre   mean %4lld.%03lld ms   jitter %lld/1000\n",
           (long long)(f.per_phase_milli[0] / 1000), (long long)(f.per_phase_milli[0] % 1000),
           (long long)f.jitter_milli[0]);
    printf("    peri  mean %4lld.%03lld ms   jitter %lld/1000\n",
           (long long)(f.per_phase_milli[1] / 1000), (long long)(f.per_phase_milli[1] % 1000),
           (long long)f.jitter_milli[1]);
    printf("    post  mean %4lld.%03lld ms   jitter %lld/1000\n",
           (long long)(f.per_phase_milli[2] / 1000), (long long)(f.per_phase_milli[2] % 1000),
           (long long)f.jitter_milli[2]);
    printf("    forecast %lld.%03lld ms  [%lld.%03lld, %lld.%03lld]  confidence %lld/1000  n=%lld\n",
           (long long)(f.expected_milli / 1000), (long long)(f.expected_milli % 1000),
           (long long)(f.lo_milli / 1000), (long long)(f.lo_milli % 1000),
           (long long)(f.hi_milli / 1000), (long long)(f.hi_milli % 1000),
           (long long)f.confidence_milli, (long long)f.n);
    gcheck(f.jitter_milli[2] > f.jitter_milli[1],
           "the stalling phase has higher CV than the steady one");
    gcheck(f.lo_milli >= 0 && f.lo_milli <= f.expected_milli && f.expected_milli <= f.hi_milli,
           "the forecast is a closed interval around the mean, clamped at zero");
    gcheck(f.confidence_milli > 0 && f.confidence_milli < 1000, "confidence is a proper fraction");
    {
      int d = 0;
      bool sim = canor_golay_tele_similar(&a, &b, &d);
      canor_golay_tele_window_snap(&a, &moved);
      printf("    windows: raw distance %d, snap moved %d, similar %s\n",
             d, moved, sim ? "yes" : "no");
      gcheck(!sim || d <= 6, "similar windows are within the proven radius of 6");
    }
  }

  /* Provenance stamps: the round trip, the repair, and the refusal. All three
   * matter. A stamp that only ever confirmed intact files would not show that
   * it can repair itself, nor that it declines to guess when it cannot. */
  {
    const char *payload = "RGBA-XYZ 12 34 56 78";
    char st[PROV_STAMP_CHARS];
    prov_result pr;
    size_t plen = strlen(payload);
    int q;

    gcheck(prov_stamp(payload, plen, st, (int)sizeof st), "a stamp is produced");
    gcheck(prov_verify(payload, plen, st, &pr) && pr.ok, "an intact payload verifies");
    printf("\n  provenance\n    stamp %s   (%d bits over a 64-bit digest)\n",
           st, PROV_BLOCKS * 24);

    gcheck(!prov_verify("RGBA-XYZ 12 34 56 79", plen, st, &pr) && !pr.ok,
           "a one-character payload change is detected");

    for (q = 3; q <= 4; ++q) {
      char dmg[PROV_STAMP_CHARS];
      unsigned v = 0;
      int k;
      memcpy(dmg, st, sizeof dmg);
      for (k = 0; k < 6; ++k) {
        int hv = dmg[k] >= 'a' ? dmg[k] - 'a' + 10 : dmg[k] - '0';
        v = (v << 4) | (unsigned)hv;
      }
      v ^= (q == 3) ? 7u : 15u;          /* 3 or 4 bit errors in block 0 */
      for (k = 0; k < 6; ++k) {
        int nib = (int)((v >> (20 - 4 * k)) & 0xfu);
        dmg[k] = (char)(nib < 10 ? '0' + nib : 'a' + nib - 10);
      }
      prov_verify(payload, plen, dmg, &pr);
      if (q == 3)
        gcheck(pr.ok && pr.stamp_repaired && pr.bits_corrected == 3,
               "3 bit errors in the stamp are repaired; the file still passes");
      else
        gcheck(pr.stamp_unreadable,
               "4 bit errors in the stamp are REFUSED, never guessed at");
    }
  }

  printf("\n  NOTE ON McELIECE. Not on this code. The Golay code is a single\n"
         "  fixed object with no family to hide in, and the only secret left\n"
         "  would be the permutation - M24 has 244,823,040 elements, which is\n"
         "  exhaustible in seconds. That is not a parameter-size caveat; there\n"
         "  is no larger Golay. Code-based crypto wants binary Goppa codes at\n"
         "  n around 3488-6960, a different construction entirely. This code\n"
         "  is sound for INTEGRITY, which makes no secrecy claim.\n");

  printf("\n%d checks, %d failures\n", gchecks, gfails);
  return gfails ? 1 : 0;
}
