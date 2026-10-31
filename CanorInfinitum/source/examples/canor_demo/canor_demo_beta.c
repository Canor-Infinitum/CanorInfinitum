/* canor_demo_beta.c - beta - verify the deterministic core against v1-beta.json
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* beta - verify the deterministic core against v1-beta.json                  */
/* ========================================================================== */
/* v1-beta.json is a frozen artifact: 370k words, each with an RGB string, built
 * by an earlier pipeline. Running the current core over it is the check that
 * the algebra still says what it said when the artifact was written - which is
 * the only kind of regression test a semantic layer can have.
 *
 * ARBITRARY LENGTH. The Flow registry caps a word at 32 letters, and dictionary
 * RGB strings run longer. Rather than truncate - which would silently answer a
 * different question - the fold here goes straight through canor_imag_mul, one
 * letter at a time. That is the same product the registry uses; only the
 * storage limit is bypassed.
 *
 * THE COMPOSITION CHECK IS A THEOREM, NOT A MEASUREMENT. Folding is a
 * left-to-right product in an associative group, so Flow(A ++ B) must equal
 * Flow(A) . Flow(B) at exactly 100%. It is run anyway, because it is what would
 * catch an error in the projection, the fold, or the unit-to-Flow naming - and
 * those are precisely where a subtle mistake would otherwise sit unnoticed. A
 * result of 99.9% here would mean the core is broken, not that the data is
 * interesting. */

typedef struct { uint8_t unit; int sign; } foldres;

static bool beta_fold(const canor_demo_config_t *c, const char *rgb, foldres *out, int cnt[3]) {
  canor_imag acc = 0;                  /* e0, the identity */
  int sign = 1;
  const char *p;
  cnt[0] = cnt[1] = cnt[2] = 0;
  for (p = rgb; *p; ++p) {
    int chan = (*p == 'R') ? 0 : (*p == 'G') ? 1 : (*p == 'B') ? 2 : -1;
    int m, s2;
    canor_imag u, prod;
    if (chan < 0) return false;
    ++cnt[chan];
    for (m = 0; m < CANOR_DEMO_AXIS_N; ++m) if (c->meta[m].channel == chan) break;
    if (m == CANOR_DEMO_AXIS_N || !c->meta[m].unit) return false;
    u = (canor_imag)(c->meta[m].unit == 'i' ? 1 :
                  c->meta[m].unit == 'j' ? 2 :
                  c->meta[m].unit == 'k' ? 3 : 0);
    if (canor_imag_mul(acc, u, &prod, &s2) != CANOR_OK) return false;
    acc = prod;
    sign *= s2;
  }
  out->unit = (uint8_t)acc;
  out->sign = sign;
  return true;
}

int canor_demo_cmd_beta(const canor_demo_config_t *c, const char *path) {
  FILE *f;
  long len;
  char *buf;
  canor_demo_jscan_t j;
  long entries = 0, bad = 0, maxlen = 0, total_len = 0;
  long joint[CANOR_DEMO_AXIS_N][8];
  long comp_ok = 0, comp_bad = 0, comp_run = 0;
  char prev_rgb[512];
  foldres prev_fr;
  bool have_prev = false;
  int mi, fi;

  f = fopen(path, "rb");
  if (!f) { fprintf(stderr, "follow: cannot open %s\n", path); return 2; }
  fseek(f, 0, SEEK_END);
  len = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (len <= 0) { fclose(f); fprintf(stderr, "follow: %s is empty\n", path); return 2; }
  buf = (char *)malloc((size_t)len + 1);
  if (!buf) { fclose(f); fprintf(stderr, "follow: out of memory\n"); return 2; }
  if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
    fclose(f); free(buf);
    fprintf(stderr, "follow: short read on %s\n", path);
    return 2;
  }
  fclose(f);
  buf[len] = 0;

  memset(joint, 0, sizeof joint);
  memset(&j, 0, sizeof j);
  j.p = buf; j.end = buf + len;

  if (!canor_demo_jlit(&j, '{')) {
    free(buf);
    fprintf(stderr, "follow: %s is not a JSON object\n", path);
    return 2;
  }
  if (!canor_demo_jlit(&j, '}')) {
    do {
      char word[256], rgb[512];
      foldres fr;
      int cnt[3], slen = 0;
      if (!canor_demo_jstr(&j, word, sizeof word)) break;
      if (!canor_demo_jlit(&j, ':')) break;
      if (!canor_demo_jstr(&j, rgb, sizeof rgb)) break;
      ++entries;
      while (rgb[slen]) ++slen;
      total_len += slen;
      if (slen > maxlen) maxlen = slen;
      if (!beta_fold(c, rgb, &fr, cnt)) { ++bad; continue; }
      mi = canor_demo_meta_of(c, cnt);
      fi = (fr.unit < (uint8_t)c->flows.count) ? (int)fr.unit : c->flows.count;
      joint[mi][fi]++;

      /* Composition, against the previous entry. Every 64th pair, which is
       * enough to be a population and keeps a 370k-entry run quick. */
      if (have_prev && (entries % 64) == 0) {
        char cat[1024];
        foldres cfr;
        int ccnt[3];
        int a = 0, b = 0;
        while (prev_rgb[a] && a < 500) { cat[a] = prev_rgb[a]; ++a; }
        while (rgb[b] && a + b < 1000) { cat[a + b] = rgb[b]; ++b; }
        cat[a + b] = 0;
        ++comp_run;
        if (beta_fold(c, cat, &cfr, ccnt)) {
          canor_imag prod;
          int s2;
          if (canor_imag_mul(prev_fr.unit, fr.unit, &prod, &s2) == CANOR_OK &&
              prod == cfr.unit && prev_fr.sign * fr.sign * s2 == cfr.sign)
            ++comp_ok;
          else
            ++comp_bad;
        } else ++comp_bad;
      }
      { int k = 0; while (rgb[k] && k < 500) { prev_rgb[k] = rgb[k]; ++k; } prev_rgb[k] = 0; }
      prev_fr = fr;
      have_prev = true;
    } while (canor_demo_jlit(&j, ','));
  }
  free(buf);

  printf("v1-beta verification: %s\n\n", path);
  printf("  entries              %ld\n", entries);
  printf("  unreadable RGB       %ld\n", bad);
  printf("  longest RGB string   %ld letters%s\n", maxlen,
         maxlen >= CANOR_FLOW_WORD_MAX ? "  (past the registry cap; folded directly)" : "");
  printf("  mean length          %.2f\n\n", entries ? (double)total_len / entries : 0.0);

  printf("  composition Flow(A ++ B) = Flow(A) . Flow(B)\n");
  printf("    checked            %ld\n", comp_run);
  printf("    holds              %ld\n", comp_ok);
  printf("    fails              %ld   %s\n", comp_bad,
         comp_bad ? "<-- THE CORE IS BROKEN" : "(a theorem; anything else is a defect)");
  printf("\njoint distribution, MetaFlow x Flow\n\n");
  printf("  %-14s", "");
  for (fi = 0; fi < c->flows.count; ++fi) printf(" %10s", c->flows.flow[fi].name);
  printf(" %10s %7s\n", "total", "share");
  for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) {
    long row = 0;
    printf("  %-14s", c->meta[mi].name);
    for (fi = 0; fi < c->flows.count; ++fi) { printf(" %10ld", joint[mi][fi]); row += joint[mi][fi]; }
    printf(" %10ld %6.1f%%\n", row, entries ? 100.0 * row / entries : 0.0);
  }
  printf("  %-14s", "total");
  for (fi = 0; fi < c->flows.count; ++fi) {
    long col = 0;
    for (mi = 0; mi < CANOR_DEMO_AXIS_N; ++mi) col += joint[mi][fi];
    printf(" %10ld", col);
  }
  printf("\n");
  return comp_bad ? 1 : 0;
}
