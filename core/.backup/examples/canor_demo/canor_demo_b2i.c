/* canor_demo_b2i.c - b2i - base 2i, and a digit filter over the Gaussian lattice
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* b2i - base 2i, and a digit filter over the Gaussian lattice               */
/* ========================================================================== */
/* Base 2i is an exact positional system for the Gaussian integers: digits
 * {0,1,2,3}, no sign anywhere, and one representation per point. Both halves
 * of that are load-bearing here.
 *
 * NO SIGN, because the axes run in base -4 and a negative base alternates on
 * its own. A point in the third quadrant is a string of 0..3 exactly like one
 * in the first, so a digit filter is not secretly a sign filter.
 *
 * UNIQUE, which is what makes a DIGIT filter mean anything about a NUMBER. In
 * a redundant system the same lattice point would pass under one spelling and
 * fail under another, and the filter would be selecting spellings rather than
 * points. Uniqueness is what promotes "this string is a palindrome" into a
 * property of the Gaussian integer itself - so it is checked below by
 * round-tripping every point in the window rather than cited.
 */

int canor_demo_cmd_b2i(const canor_demo_config_t *c, const char *arg, const char *out) {
  canor_b2i v;
  canor_b2i_filter flt;
  char buf[B2I_MAX + 8];
  int64_t re, im;
  int i, R = 12, pass = 0, tot = 0, rt_ok = 1, uniq_ok = 1;
  FILE *f;
  static const struct { int64_t a, b; const char *want; } KNOWN[] = {
    { 0,  0, "0"     },
    { 1,  0, "1"     },
    { 2,  0, "2"     },
    { 1,  1, "11.2"  },   /* Knuth's worked example */
    { 0,  2, "10"    },
    { -4, 0, "100"   },
    { 16, 0, "10000" }
  };
  (void)c;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  printf("Base 2i - quater-imaginary, digits {0,1,2,3}\n\n");

  printf("  known values, checked rather than cited\n");
  for (i = 0; i < (int)(sizeof KNOWN / sizeof KNOWN[0]); ++i) {
    char lab[64];
    if (!canor_b2i_encode(KNOWN[i].a, KNOWN[i].b, &v)) { canor_demo_check(0, "encode"); continue; }
    canor_b2i_format(&v, buf, sizeof buf);
    snprintf(lab, sizeof lab, "%lld%+lldi -> %s", (long long)KNOWN[i].a,
             (long long)KNOWN[i].b, KNOWN[i].want);
    printf("    %-24s got %s\n", lab, buf);
    canor_demo_check(!strcmp(buf, KNOWN[i].want), lab);
  }

  /* --- round trip and uniqueness over a window ----------------------------- */
  {
    static char seen[64 * 64][B2I_MAX + 8];
    int nseen = 0;
    for (re = -R; re <= R; ++re)
      for (im = -R; im <= R; ++im) {
        int64_t r2, i2;
        int q;
        ++tot;
        if (!canor_b2i_encode(re, im, &v)) { rt_ok = 0; continue; }
        if (!canor_b2i_decode(&v, &r2, &i2)) { rt_ok = 0; continue; }
        if (r2 != re || i2 != im) rt_ok = 0;
        canor_b2i_format(&v, buf, sizeof buf);
        /* UNIQUENESS THE HARD WAY: no two distinct points may share a string.
         * Quadratic in the window, which is affordable at this size and is the
         * only check that actually establishes the property. */
        if (nseen < 64 * 64) {
          for (q = 0; q < nseen; ++q)
            if (!strcmp(seen[q], buf)) { uniq_ok = 0; break; }
          snprintf(seen[nseen++], B2I_MAX + 8, "%s", buf);
        }
      }
    printf("\n  window [-%d,%d]^2 : %d points\n", R, R, tot);
    canor_demo_check(rt_ok, "every point round-trips exactly through base 2i");
    canor_demo_check(uniq_ok, "no two distinct points share a representation");
  }

  /* --- the filter ----------------------------------------------------------- */
  canor_b2i_filter_init(&flt);
  if (arg && *arg) {
    /* A tiny spec: digits allowed, then optional flags. "013 pal noflac" style
     * is overkill here; the useful knobs are the digit set and the flags. */
    const char *q = arg;
    uint8_t mask = 0;
    for (; *q && *q >= '0' && *q <= '3'; ++q) mask |= (uint8_t)(1u << (*q - '0'));
    if (mask) flt.allow = mask;
    if (strstr(arg, "pal")) flt.palindrome = true;
    if (strstr(arg, "int")) flt.no_frac = true;
  } else {
    flt.allow = 1 | 2;          /* digits 0 and 1 only */
    flt.no_frac = true;
  }
  printf("\n  filter: digits {");
  for (i = 0; i < 4; ++i) if ((flt.allow >> i) & 1) printf("%d", i);
  printf("}%s%s\n", flt.no_frac ? ", integer only" : "",
         flt.palindrome ? ", palindromic" : "");

  f = fopen(out, "w");
  if (!f) return 2;
  fputs("<!doctype html>\n<meta charset=\"utf-8\">\n<title>base 2i</title>\n"
        "<style>body{background:#0d1117;color:#c9d1d9;font:14px/1.6 "
        "system-ui,sans-serif;margin:0;padding:32px}.w{max-width:980px;"
        "margin:0 auto}h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;"
        "margin:26px 0 8px;color:#e6edf3}.sub{color:#8b949e;font-size:13px;"
        "max-width:72ch}svg{background:#010409;border:1px solid #30363d;"
        "border-radius:6px}</style>\n<div class=\"w\">\n"
        "<h1>Base 2i &mdash; the Gaussian lattice, filtered by digits</h1>\n", f);
  fprintf(f, "<p class=\"sub\">Digits {0,1,2,3}, base 2i. Every Gaussian "
             "integer has exactly one representation and no sign is ever "
             "needed &mdash; the axes run in base &minus;4, which alternates on "
             "its own. Uniqueness is what makes a filter on DIGITS a filter on "
             "NUMBERS: in a redundant system the same point would pass under "
             "one spelling and fail under another.</p>\n");
  fprintf(f, "<svg viewBox=\"0 0 %d %d\" width=\"100%%\">\n",
          (2 * R + 1) * 22 + 40, (2 * R + 1) * 22 + 40);
  for (re = -R; re <= R; ++re)
    for (im = -R; im <= R; ++im) {
      int ok;
      double x = 20.0 + (double)(re + R) * 22.0;
      double y = 20.0 + (double)(R - im) * 22.0;
      if (!canor_b2i_encode(re, im, &v)) continue;
      ok = canor_b2i_pass(&v, &flt);
      if (ok) ++pass;
      canor_b2i_format(&v, buf, sizeof buf);
      fprintf(f, "<rect x=\"%.0f\" y=\"%.0f\" width=\"20\" height=\"20\" "
                 "fill=\"%s\" fill-opacity=\"%s\" stroke=\"#21262d\">"
                 "<title>%lld%+lldi = %s</title></rect>\n",
              x, y, ok ? "#3fb950" : "#30363d", ok ? "0.85" : "0.25",
              (long long)re, (long long)im, buf);
    }
  fprintf(f, "</svg>\n");
  fprintf(f, "<p class=\"sub\">%d of %d lattice points pass. Green passes; the "
             "origin is at the centre and the imaginary axis runs up. The "
             "pattern is a statement about digit strings, so its symmetry - or "
             "lack of it - is a fact about base 2i and not about the "
             "window.</p>\n", pass, tot);
  fputs("</div>\n", f);
  fclose(f);

  printf("  %d of %d points pass\n", pass, tot);
  printf("\n  wrote %s\n", out);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
