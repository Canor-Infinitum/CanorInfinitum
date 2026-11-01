/* The three axes, computed deterministically over semantic_v1a.json.
 *
 * No model anywhere. Every answer is derived from the RGB string, so the whole
 * run is a pure function of the dictionary - which is the point: it establishes
 * what the algebra says BEFORE anyone asks anything external, and an elicited
 * answer set can then be compared against it.
 *
 * The three axes, all computable:
 *
 *   Flow      RGB -> IMAG by R->i, G->j, B->k, folded under Hamilton's rules.
 *             The resulting unit names a baseline: one=Ordinary, i=Peaceful,
 *             j=Formful, k=Stormful.
 *   MetaFlow  which RGB channel strictly dominates; none is Arbitrary.
 *   Mixture   (decomposable, dominant), decomposable being dim >= 2.
 *
 * THREE COMPOSITION QUESTIONS, AND THEY ARE NOT THE SAME KIND OF QUESTION
 * ----------------------------------------------------------------------
 * Composition here is concatenation: the metaflow of "AB" is the metaflow of A
 * followed by that of B.
 *
 *   A. Is Flow(A ++ B) the product Flow(A) . Flow(B)?
 *      This is a THEOREM, not a measurement. Folding is a left-to-right
 *      product and the group is associative, so it must hold at 100%. It is
 *      run anyway, because it is the check that would catch an error in the
 *      projection, the fold, or the unit-to-baseline naming - and those are
 *      exactly the places a subtle mistake would hide.
 *
 *   B. Is MetaFlow(A ++ B) predictable from MetaFlow(A) and MetaFlow(B)?
 *      GENUINELY UNCERTAIN. Channel dominance of a concatenation is a property
 *      of the summed counts, and two strings can each be R-dominant while
 *      their concatenation is not. There is no reason this should hold, and
 *      measuring it says how much compositional structure the MetaFlow axis
 *      actually carries.
 *
 *   C. Same question for Mixture. Also uncertain, and additionally
 *      interesting because decomposability is monotone under concatenation
 *      while dominance is not - so the two coordinates of the Mixture should
 *      behave differently, and the measurement can show that.
 *
 * A predictor for B and C needs a rule. The one used is the obvious one: the
 * majority of the two inputs, ties going to the left operand - which is the
 * same left-bias the cancelling sums use, chosen for consistency rather than
 * because it is likely to be right.
 */

#include "vi.h"
#include "vi_strategy.h"
#include "fc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXC 256

typedef enum { F_ORD, F_PEA, F_FOR, F_STO, F_NONE } flow_id;
typedef enum { M_GEN, M_SPE, M_DIS, M_ARB } meta_id;
typedef enum { X_ELE, X_UNI, X_COM, X_UNK } mix_id;

static const char *const fname[5] = { "Ordinary", "Peaceful", "Formful", "Stormful", "?" };
static const char *const mname[4] = { "General", "Specific", "Distinctive", "Arbitrary" };
static const char *const xname[4] = { "Elementary", "Unique", "Composite", "Unknown" };

static uint8_t flow_unit[4];          /* baseline -> basis index, folded */

static char g_name[MAXC][32];
static char g_rgb[MAXC][80];
static int g_n;

static void counts(const char *s, int *r, int *g, int *b) {
  *r = *g = *b = 0;
  for (; *s; ++s) { if (*s == 'R') ++*r; else if (*s == 'G') ++*g; else if (*s == 'B') ++*b; }
}

static meta_id meta_of_rgb(const char *s) {
  int r, g, b;
  counts(s, &r, &g, &b);
  if (r > g && r > b) return M_GEN;
  if (g > r && g > b) return M_DIS;
  if (b > r && b > g) return M_SPE;
  return M_ARB;
}

static mix_id mix_of_rgb(const char *s) {
  int r, g, b;
  counts(s, &r, &g, &b);
  int dec = (b >= 2);
  int dom = (r > g && r > b) || (g > r && g > b) || (b > r && b > g);
  if (!dec) return dom ? X_ELE : X_UNI;
  return dom ? X_COM : X_UNK;
}

/* RGB -> IMAG -> fold. R->i, G->j, B->k, which is the inverse of the
 * projection the v1-beta build used. */
static int flow_of_rgb(const char *s, uint8_t *unit_out, int *sign_out) {
  fc_imag acc = 0;
  int sign = 1, started = 0;
  for (; *s; ++s) {
    fc_imag u = (*s == 'R') ? 1 : (*s == 'G') ? 2 : (*s == 'B') ? 3 : 0;
    if (!u) continue;
    if (!started) { acc = u; started = 1; continue; }
    fc_imag r;
    int sg;
    if (fc_imag_mul(acc, u, &r, &sg) != FC_OK) return F_NONE;
    acc = r;
    sign *= sg;
  }
  if (!started) return F_NONE;
  if (unit_out) *unit_out = acc;
  if (sign_out) *sign_out = sign;
  for (int i = 0; i < 4; ++i) if (flow_unit[i] == acc) return i;
  return F_NONE;
}

static int load(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) { printf("cannot open %s\n", path); return 0; }
  static char buf[1 << 18];
  size_t got = fread(buf, 1, sizeof buf - 1, f);
  buf[got] = '\0';
  fclose(f);
  const char *p = buf;
  while (*p && g_n < MAXC) {
    const char *ks = strchr(p, '"');       if (!ks) break;
    const char *ke = strchr(ks + 1, '"');  if (!ke) break;
    const char *co = strchr(ke + 1, ':');  if (!co) break;
    const char *vs = strchr(co + 1, '"');  if (!vs) break;
    const char *ve = strchr(vs + 1, '"');  if (!ve) break;
    size_t kl = (size_t)(ke - ks - 1), vl = (size_t)(ve - vs - 1);
    if (kl < 32 && vl < 80) {
      memcpy(g_name[g_n], ks + 1, kl); g_name[g_n][kl] = '\0';
      memcpy(g_rgb[g_n], vs + 1, vl);  g_rgb[g_n][vl] = '\0';
      ++g_n;
    }
    p = ve + 1;
  }
  return g_n > 0;
}

int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : "../semantic_v1a.json";
  if (fc_imag_init() != FC_OK) { printf("unit algebra does not close\n"); return 1; }

  vi_flow_registry reg;
  vi_flows_default(&reg);
  for (int i = 0; i < 4; ++i) {
    uint8_t u;
    int sg;
    vi_flow_fold(&reg, i, &u, &sg);
    flow_unit[i] = u;
  }

  if (!load(path)) return 1;
  printf("deterministic axes over %s: %d concepts, no model\n\n", path, g_n);

  /* --- per-concept, and the joint occupancy ------------------------------ */
  int cell[4][4][4];
  memset(cell, 0, sizeof cell);
  int fh[5] = { 0 }, mh[4] = { 0 }, xh[4] = { 0 };

  printf("  %-14s %-34s %-9s %-12s %s\n", "concept", "rgb", "Flow", "MetaFlow", "Mixture");
  for (int i = 0; i < g_n; ++i) {
    uint8_t u;
    int sg;
    int fl = flow_of_rgb(g_rgb[i], &u, &sg);
    meta_id me = meta_of_rgb(g_rgb[i]);
    mix_id mx = mix_of_rgb(g_rgb[i]);
    ++fh[fl < 0 ? 4 : fl];
    ++mh[me];
    ++xh[mx];
    if (fl >= 0 && fl < 4) ++cell[fl][me][mx];
    if (i < 8 || !strcmp(g_name[i], "quaternion") || !strcmp(g_name[i], "ether"))
      printf("  %-14s %-34s %-9s %-12s %s\n", g_name[i], g_rgb[i],
             fl < 4 ? fname[fl] : "?", mname[me], xname[mx]);
  }
  printf("  ... (%d more)\n", g_n - 8);

  printf("\n  Flow     ");
  for (int i = 0; i < 4; ++i) printf(" %s=%d", fname[i], fh[i]);
  printf("\n  MetaFlow ");
  for (int i = 0; i < 4; ++i) printf(" %s=%d", mname[i], mh[i]);
  printf("\n  Mixture  ");
  for (int i = 0; i < 4; ++i) printf(" %s=%d", xname[i], xh[i]);
  int occupied = 0;
  for (int a = 0; a < 4; ++a) for (int b = 0; b < 4; ++b) for (int c = 0; c < 4; ++c)
    if (cell[a][b][c]) ++occupied;
  printf("\n  joint    %d of 64 cells occupied\n", occupied);

  /* --- composition = concatenation --------------------------------------- */
  int a_hit = 0, a_tot = 0;
  int b_hit = 0, b_tot = 0;
  int c_hit = 0, c_tot = 0;
  int b_marg[4] = { 0 }, c_marg[4] = { 0 };
  int sign_flip = 0, noncommuting = 0;

  static char cat[192];
  for (int i = 0; i < g_n; ++i) {
    for (int j = 0; j < g_n; ++j) {
      if (strlen(g_rgb[i]) + strlen(g_rgb[j]) >= sizeof cat) continue;
      snprintf(cat, sizeof cat, "%s%s", g_rgb[i], g_rgb[j]);

      uint8_t u;
      int sg;
      int f_cat = flow_of_rgb(cat, &u, &sg);
      int f_i = flow_of_rgb(g_rgb[i], NULL, NULL);
      int f_j = flow_of_rgb(g_rgb[j], NULL, NULL);
      if (f_cat < 0 || f_i < 0 || f_j < 0) continue;

      /* A: the theorem. */
      fc_imag pu;
      int ps;
      if (fc_imag_mul(flow_unit[f_i], flow_unit[f_j], &pu, &ps) == FC_OK) {
        int pred = F_NONE;
        for (int k = 0; k < 4; ++k) if (flow_unit[k] == pu) pred = k;
        ++a_tot;
        if (pred == f_cat) ++a_hit;
      }

      /* Sign, which the four names cannot express: does reversing the operands
       * flip it? Counted separately, because it is the part of the algebra the
       * naming discards. */
      if (i < j) {
        snprintf(cat, sizeof cat, "%s%s", g_rgb[j], g_rgb[i]);
        uint8_t u2;
        int sg2;
        int f_rev = flow_of_rgb(cat, &u2, &sg2);
        if (f_rev >= 0) {
          ++noncommuting;
          if (u2 == u && sg2 != sg) ++sign_flip;
        }
        snprintf(cat, sizeof cat, "%s%s", g_rgb[i], g_rgb[j]);
      }

      /* B and C: genuinely uncertain. Predictor is the majority of the two
       * inputs, ties to the left - the same left-bias the cancelling sums use. */
      meta_id mi = meta_of_rgb(g_rgb[i]), mj = meta_of_rgb(g_rgb[j]);
      meta_id m_cat = meta_of_rgb(cat);
      meta_id m_pred = (mi == mj) ? mi : mi;
      ++b_tot;
      if (m_pred == m_cat) ++b_hit;
      ++b_marg[m_cat];

      mix_id xi = mix_of_rgb(g_rgb[i]), xj = mix_of_rgb(g_rgb[j]);
      mix_id x_cat = mix_of_rgb(cat);
      mix_id x_pred = (xi == xj) ? xi : xi;
      ++c_tot;
      if (x_pred == x_cat) ++c_hit;
      ++c_marg[x_cat];
      (void)mj; (void)xj;
    }
  }

  int bm = 0, cm = 0;
  for (int i = 1; i < 4; ++i) { if (b_marg[i] > b_marg[bm]) bm = i; if (c_marg[i] > c_marg[cm]) cm = i; }

  printf("\nCOMPOSITION = concatenation, %d ordered pairs\n", a_tot);
  printf("\n  A  Flow(A++B) == Flow(A).Flow(B)        %6.2f%%   (%d of %d)\n",
         100.0 * a_hit / (a_tot ? a_tot : 1), a_hit, a_tot);
  printf("     THEOREM: folding is a left-to-right product and the group is\n"
         "     associative, so anything below 100%% is a bug in the projection,\n"
         "     the fold, or the unit-to-baseline naming - not a finding.\n");

  printf("\n  B  MetaFlow(A++B) from its parts        %6.2f%%   (%d of %d)\n",
         100.0 * b_hit / (b_tot ? b_tot : 1), b_hit, b_tot);
  printf("     marginal %.2f%% always answering %s, chance 25%%\n",
         100.0 * b_marg[bm] / (b_tot ? b_tot : 1), mname[bm]);

  printf("\n  C  Mixture(A++B) from its parts         %6.2f%%   (%d of %d)\n",
         100.0 * c_hit / (c_tot ? c_tot : 1), c_hit, c_tot);
  printf("     marginal %.2f%% always answering %s, chance 25%%\n",
         100.0 * c_marg[cm] / (c_tot ? c_tot : 1), xname[cm]);

  printf("\n  sign     %d of %d unordered pairs flip sign when reversed\n",
         sign_flip, noncommuting);
  printf("           the four baseline names cannot express sign, so this is\n"
         "           structure the naming discards - and the amount of it is\n"
         "           the ceiling on what any four-option elicitation can recover\n");

  return (a_hit == a_tot) ? 0 : 1;
}
