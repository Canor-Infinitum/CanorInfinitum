/* canor_demo_provenance.c - provenance: every emission is integrity-checkable
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* provenance: every emission is integrity-checkable                          */
/* ========================================================================== */
/* One rule, applied without exception: anything that leaves memory gets a
 * stamp. An artifact read back later and used to draw a conclusion is exactly
 * where silent corruption does its damage, because a corrupted file that still
 * parses looks like data.
 *
 * The stamp lands in `<artifact>.golay` rather than inside the artifact, so
 * the artifact stays byte-identical to what the algorithm produced and stays
 * valid JSON where it is JSON. */

bool canor_demo_prov_write_sidecar(const char *path) {
  FILE *f;
  long n;
  char *buf;
  char stamp[PROV_STAMP_CHARS];
  char side[600];

  f = fopen(path, "rb");
  if (!f) return false;
  fseek(f, 0, SEEK_END);
  n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return false; }
  buf = (char *)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return false; }
  if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
    fclose(f); free(buf); return false;
  }
  fclose(f);

  if (!prov_stamp(buf, (size_t)n, stamp, (int)sizeof stamp)) { free(buf); return false; }
  free(buf);

  snprintf(side, sizeof side, "%s.golay", path);
  f = fopen(side, "w");
  if (!f) return false;
  fprintf(f, "# extended Golay [24,12,8] provenance stamp\n");
  fprintf(f, "# 64-bit FNV-1a digest, six 12-bit chunks, each Golay-encoded.\n");
  fprintf(f, "# Corrects up to 3 bit errors per 24-bit block; detects 4.\n");
  fprintf(f, "# Detects accidental corruption. NOT tamper-evidence - see\n");
  fprintf(f, "# canor_golay.h; anyone who edits the artifact can recompute this.\n");
  fprintf(f, "bytes %ld\n", n);
  fprintf(f, "golay %s\n", stamp);
  fclose(f);
  printf("  stamped %s\n", side);
  return true;
}

int canor_demo_cmd_verify(const char *path) {
  FILE *f;
  long n;
  char *buf;
  char side[600], line[256], stamp[PROV_STAMP_CHARS];
  prov_result r;
  long declared = -1;
  bool have = false;

  snprintf(side, sizeof side, "%s.golay", path);
  f = fopen(side, "r");
  if (!f) {
    fprintf(stderr, "follow: no stamp at %s\n", side);
    return 2;
  }
  while (fgets(line, sizeof line, f)) {
    if (line[0] == '#') continue;
    if (!strncmp(line, "bytes ", 6)) declared = atol(line + 6);
    else if (!strncmp(line, "golay ", 6)) {
      int k = 0;
      const char *q = line + 6;
      while (*q && *q != '\n' && *q != '\r' && k + 1 < (int)sizeof stamp) stamp[k++] = *q++;
      stamp[k] = 0;
      have = (k == PROV_BLOCKS * 6);
    }
  }
  fclose(f);
  if (!have) { fprintf(stderr, "follow: %s has no readable stamp line\n", side); return 2; }

  f = fopen(path, "rb");
  if (!f) { fprintf(stderr, "follow: cannot read %s\n", path); return 2; }
  fseek(f, 0, SEEK_END);
  n = ftell(f);
  fseek(f, 0, SEEK_SET);
  buf = (char *)malloc((size_t)(n > 0 ? n : 1));
  if (!buf) { fclose(f); return 2; }
  if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
    fclose(f); free(buf);
    fprintf(stderr, "follow: short read on %s\n", path);
    return 2;
  }
  fclose(f);

  prov_verify(buf, (size_t)n, stamp, &r);
  free(buf);

  printf("artifact   %s\n", path);
  printf("stamp      %s\n", side);
  printf("bytes      %ld%s\n", n,
         (declared >= 0 && declared != n) ? "   <-- SIZE DISAGREES WITH THE STAMP" : "");
  if (r.stamp_unreadable) {
    printf("stamp      UNREADABLE - a block carried 4+ errors, past correcting\n");
    printf("           Refused rather than guessed: a guess here would produce a\n");
    printf("           confident wrong digest.\n");
    return 1;
  }
  if (r.stamp_repaired)
    printf("stamp      repaired %d bit(s) in transit; the record survived\n",
           r.bits_corrected);
  printf("digest     stamp %016llx   payload %016llx\n",
         (unsigned long long)r.expected, (unsigned long long)r.actual);
  printf("result     %s\n", r.ok ? "INTACT" : "CORRUPTED - the payload does not match");
  return r.ok ? 0 : 1;
}
