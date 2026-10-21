#ifndef CANOR_DEMO_H
#define CANOR_DEMO_H

/* `follow` demo: shared declarations.
 *
 * The demo was one 12,873-line translation unit. It is now one file per
 * feature (canor_demo_<feature>.c) plus the dispatcher in canor_demo.c. This
 * header holds exactly what crosses a file boundary: the configuration and
 * point-cloud types, and the helpers one command borrows from another. Every
 * name here carries the canor_demo_ prefix; anything without it is private to
 * the file that defines it.
 *
 * Examples sit outside the library lint scope (AGENTS.md): the demo uses
 * double and the heap for rendering, which the core never does.
 */

#include "canor_core.h"
#include "canor_big.h"
#include "canor_alg.h"
#include "canor_dim.h"
#include "canor_poly.h"
#include "canor_prime.h"
#include "canor_sym.h"
#include "canor_tensor.h"
#include "canor_scale.h"
#include "canor_b2i.h"
#include "canor_surreal.h"
#include "canor_agg.h"
#include "canor_qval.h"
#include "canor_fw.h"
#include "canor_flow.h"
#include "canor_strategy.h"
#include "canor_ore.h"
#include "canor_excite.h"
#include "canor_golay.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ---- minimal JSON scanner ---------------------------------------------- */

typedef struct { const char *p, *end; char err[384]; } canor_demo_jscan_t;

/* ---- configuration ----------------------------------------------------- */

#define CANOR_DEMO_CFG_STRAT_MAX 32

typedef struct {
  int fuel, levels, hv_tables, hv_bits, grid_w, grid_h;
} canor_demo_settings_t;

/* --- the two four-valued axes ----------------------------------------------
 * MetaFlow lives at the RGB level, Flow at the IMAG level, and Mixture is the
 * pair of them. All three are four-valued, and in each the FOURTH SLOT IS A
 * REFUSAL - Arbitrary and Unknown are not a fourth kind of answer, they are
 * the absence of one. That is why a MetaFlow entry carries a channel and the
 * refusal carries "none": the first three name a dominance, the fourth names
 * the case where nothing dominates.
 *
 * The names are configurable but the ARITY IS NOT. A three-valued MetaFlow
 * axis would have nowhere to put an undecided string and would have to invent
 * an answer; a five-valued one would not correspond to the Kapprox ladder,
 * which has exactly four classes. So the loader requires exactly four and says
 * so rather than accepting a list it cannot use. */
#define CANOR_DEMO_AXIS_N 4

typedef struct {
  char name[24];
  int channel;          /* 0=R 1=G 2=B, -1 = the refusal slot */
  char unit;            /* the imaginary unit this channel projects to */
  canor_kclass kclass;
} canor_demo_metaflow_def_t;

typedef struct {
  char name[24];
  int dims;             /* required distinct channels, -1 = the refusal slot */
  bool dominant;        /* whether a strict dominance is also required */
} canor_demo_mixture_def_t;

typedef struct {
  canor_strategy strat[CANOR_DEMO_CFG_STRAT_MAX];
  /* STRATEGY.name is a `const char *`, so the config must own the storage it
   * points at. Keeping the names here rather than in the caller's frame is not
   * a style choice: a name held in a parse-time local dangles the moment that
   * function returns, and the symptom is a Strategy that prints as empty and
   * can never be selected by name. */
  char strat_name[CANOR_DEMO_CFG_STRAT_MAX][32];
  /* Ore operator parameters, one per Strategy. `use_ore` records whether the
   * Strategy asked for them; a Strategy that did not keeps the hash-derived
   * defaults, so adding this could not change any existing result. */
  /* One Ore operator per abstract operator: [strategy][sin|cos|tan]. Sharing a
   * single (a,q,s0) across all three made them three readings of one sequence,
   * which is a far smaller family than section 13 describes. */
  canor_ore strat_ore[CANOR_DEMO_CFG_STRAT_MAX][3];
  int strat_use_ore[CANOR_DEMO_CFG_STRAT_MAX];
  /* All fifteen differentials, by name. Trifecta t holds names 3t..3t+2, so
   * the tuple order below IS the trifecta assignment and reordering it would
   * silently move a family into a different chart. */
  char strat_d[CANOR_DEMO_CFG_STRAT_MAX][CANOR_TRIFECTAS * 3][128];
  int nstrat;
  canor_flow_registry flows;
  canor_demo_metaflow_def_t meta[CANOR_DEMO_AXIS_N];
  canor_demo_mixture_def_t mix[CANOR_DEMO_AXIS_N];
  canor_demo_settings_t set;
  /* The frame: centre and span, exact rationals. A zoom is a smaller span
   * around the same or a different centre, and nothing about it is
   * approximate - which is the only reason a zoom means anything here. */
  fb_rat frame_cr, frame_ci, frame_sr, frame_si;
  size_t budget;
  /* Which Strategy is selected when --strategy is absent. A name, not an
   * index, so reordering the array cannot silently change the default. */
  char default_strategy[32];
  /* Letter-to-channel map. Partitioned by frequency mass, not by alphabet, so
   * a typical English word carries no built-in channel preference. */
  char letters[3][32];
  long points;
  char path[512];
  bool from_file;
} canor_demo_config_t;

/* THE FIFTEEN DIFFERENTIAL NAMES, in trifecta order:
 *
 *   T0 di dj dk    T1 dm dn do    T2 dr ds dt    T3 de df dg    T4 dp dq du
 *
 * Only di/dj/dk used to be configurable, so a Strategy declaring five
 * trifectas got one family set from its config and four from the built-in
 * default - it was fully-fledged in arity and not in content. Naming all
 * fifteen closes that: what a Strategy declares is what it evaluates.
 *
 * An undeclared name still falls back to the default family rather than
 * failing, because a Strategy that declares three trifectas and names nine
 * differentials is a legitimate partial declaration. What it does NOT do is
 * pretend a default was a declaration - `strategies` prints which is which. */
#define CANOR_DEMO_CFG_DNAMES (CANOR_TRIFECTAS * 3)

extern const char *const canor_demo_cfg_dname[CANOR_DEMO_CFG_DNAMES];

/* ---- extended and ringed imaginary bases ------------------------------- */

#define CANOR_DEMO_EXT_UNITS_MAX 64

typedef struct {
  char name[24];
  uint8_t unit[CANOR_DEMO_EXT_UNITS_MAX];
  int32_t ring[CANOR_DEMO_EXT_UNITS_MAX];
  int len;
  int has_ring;
} canor_demo_ext_flow_t;

typedef struct { uint8_t unit; int sign; int32_t ring; } canor_demo_ext_fold_t;

/* ---- point cloud - RGBA-XYZ -------------------------------------------- */

typedef struct {
  int r, g, b, a;
  int32_t x, y, z;
} canor_demo_cloud_point_t;

/* ---- quality - the deterministic structural metric --------------------- */

typedef struct {
  int terms, factors, ops_distinct, args_distinct;
  /* Since sin_S, cos_S and tan_S carry independent Ore parameters, three
   * operators with three different periods span more structure than three
   * readings of one sequence. That is a property of the Strategy the term
   * count cannot see, so it enters the score separately. */
  int periods_distinct;        /* of 3 */
  int period_lcm;              /* the combined repeat length of the triple */
  int codes_distinct;          /* of 64 */
  long evaluated;
  double explicit_score, induced_score, quality;
} canor_demo_qual_t;

/* ---- volume - fixed-point format shared with scene --------------------- */

#define CANOR_DEMO_VOL_FP 24                /* fixed-point fractional bits */

#define CANOR_DEMO_VOL_ONE (1LL << CANOR_DEMO_VOL_FP)

/* ---- shared check harness (canor_demo_util.c) -------------------- */

extern int canor_demo_check_fail, canor_demo_check_ran;

/* ---- helpers and commands, by defining file ------------------------ */

/* canor_demo_util.c */
double canor_demo_rat_d(canor_rational r);
void canor_demo_check(int ok, const char *what);
void canor_demo_b64_out(FILE *f, const unsigned char *p, size_t n);
void canor_demo_rat_show(canor_rational r, char *buf, int cap);

/* canor_demo_json.c */
void canor_demo_jfail(canor_demo_jscan_t *j, const char *what);
void canor_demo_jws(canor_demo_jscan_t *j);
bool canor_demo_jlit(canor_demo_jscan_t *j, char c);
bool canor_demo_jstr(canor_demo_jscan_t *j, char *out, int cap);
bool canor_demo_jnum(canor_demo_jscan_t *j, long *out);
bool canor_demo_jskip(canor_demo_jscan_t *j);

/* canor_demo_config.c */
const char *canor_demo_bias_name(BiasMode b);
const char *canor_demo_kclass_name(canor_kclass k);
bool canor_demo_cfg_load(canor_demo_config_t *c, const char *path, char *err, int errcap);

/* canor_demo_inspect.c */
int canor_demo_strat_find(const canor_demo_config_t *c, const char *name);
int canor_demo_cmd_config(const canor_demo_config_t *c);
const char *canor_demo_cfg_dshow(const canor_demo_config_t *c, int i, int n, char *buf,
                                 size_t cap);
int canor_demo_cmd_strategies(const canor_demo_config_t *c, int sel);
int canor_demo_cmd_flows(const canor_demo_config_t *c);
int canor_demo_cmd_diff(const canor_demo_config_t *c, int sel, const char *only);
int canor_demo_cmd_kapprox(const canor_demo_config_t *c);
int canor_demo_meta_of(const canor_demo_config_t *c, const int cnt[3]);
int canor_demo_mix_of(const canor_demo_config_t *c, const int cnt[3]);
int canor_demo_cmd_rgb(const canor_demo_config_t *c, const char *str);
int canor_demo_cmd_metaflows(const canor_demo_config_t *c);

/* canor_demo_mandel.c */
int canor_demo_escape_exact(const fb_rat *cr, const fb_rat *ci, int fuel, int *steps, int *bits);
int canor_demo_cmd_describe(const canor_demo_config_t *c);
int canor_demo_cmd_mandel(const canor_demo_config_t *c);

/* canor_demo_bigcheck.c */
int canor_demo_cmd_bigcheck(void);

/* canor_demo_cloud.c */
int canor_demo_cloud_word(const canor_demo_config_t *c, const canor_demo_cloud_point_t *p,
                          char *word, int cap, int cnt[3]);
bool canor_demo_cloud_get(const canor_demo_config_t *c, int sel, const char *path, long want,
                          canor_demo_cloud_point_t **pts, long *n);
int canor_demo_cmd_cloud(const canor_demo_config_t *c, int sel, const char *path, long want,
                         const char *emit);
int canor_demo_cmd_bias(const canor_demo_config_t *c, int sel, const char *path, long want);

/* canor_demo_beta.c */
int canor_demo_cmd_beta(const canor_demo_config_t *c, const char *path);

/* canor_demo_tree.c */
int canor_demo_letter_channel(const canor_demo_config_t *c, char ch);
int canor_demo_cmd_tree(const canor_demo_config_t *c, int sel, const char *path, const char *emit);

/* canor_demo_fold.c */
bool canor_demo_ext_fold_word(const canor_demo_ext_flow_t *f, canor_demo_ext_fold_t *out);
bool canor_demo_ext_parse(canor_demo_ext_flow_t *f, const char *name, const char *text);
int canor_demo_cmd_fold(const canor_demo_config_t *c, const char *word, const char *rings);

/* canor_demo_survey.c */
int canor_demo_cmd_survey(const canor_demo_config_t *c, int sel, const char *path, long want,
                          int nsurvey, int amin);

/* canor_demo_questions.c */
int canor_demo_cmd_questions(const canor_demo_config_t *c, int sel, const char *path,
                             const char *emit, long limit);

/* canor_demo_quality.c */
void canor_demo_qual_measure_ore(const canor_demo_config_t *c, int sel, canor_demo_qual_t *q);
void canor_demo_qual_measure(const canor_demo_config_t *c, const canor_strategy *s,
                             const canor_demo_cloud_point_t *pts, long n, int periods_distinct,
                             int period_lcm, canor_demo_qual_t *q);

/* canor_demo_ensemble.c */
int canor_demo_cmd_ensemble(const canor_demo_config_t *c, const char *path, long want);

/* canor_demo_qualia.c */
int canor_demo_cmd_spreads(void);
int canor_demo_cmd_qualia(const canor_demo_config_t *c, int sel, const char *grouping);

/* canor_demo_excite.c */
int canor_demo_cmd_excite(const canor_demo_config_t *c, int sel);

/* canor_demo_graph.c */
int canor_demo_cmd_graph(const canor_demo_config_t *c, int sel, const char *emit, long want);

/* canor_demo_spec.c */
int canor_demo_cmd_spec(const canor_demo_config_t *c, int sel, const char *emit, long want);

/* canor_demo_symgraph.c */
int canor_demo_cmd_symgraph(void);

/* canor_demo_primes.c */
int canor_demo_cmd_primes(void);

/* canor_demo_poly.c */
int canor_demo_cmd_poly(void);

/* canor_demo_units.c */
int canor_demo_cmd_units(void);

/* canor_demo_alg.c */
int canor_demo_cmd_alg(void);

/* canor_demo_pgview.c */
int canor_demo_pg_weight(int v);
int canor_demo_cmd_pgview(const canor_demo_config_t *c, const char *emit);

/* canor_demo_mfview.c */
int canor_demo_cmd_mfview(const canor_demo_config_t *c, int sel, const char *emit);

/* canor_demo_hp4.c */
int canor_demo_cmd_hp4(const canor_demo_config_t *c, int sel, const char *emit);

/* canor_demo_selmer.c */
const char *canor_demo_sl_cls_name(canor_interval_class k);
int canor_demo_cmd_selmer(const canor_demo_config_t *c, int sel, const char *path, long want,
                          const char *out);

/* canor_demo_tensor.c */
int canor_demo_cmd_tensor(const canor_demo_config_t *c, int sel, const char *out);

/* canor_demo_scales.c */
int canor_demo_cmd_scales(const canor_demo_config_t *c, int sel, const char *out);

/* canor_demo_ricci.c */
int canor_demo_cmd_ricci(const canor_demo_config_t *c, int sel, const char *out);

/* canor_demo_sed2hp4.c */
int canor_demo_cmd_sed2hp4(const canor_demo_config_t *c, int sel, const char *out);

/* canor_demo_tvoxel.c */
int canor_demo_cmd_tvoxel(const canor_demo_config_t *c, int sel, const char *out);

/* canor_demo_volume.c */
int64_t canor_demo_vfp(double x);
int canor_demo_vol_escape(int64_t a, int64_t b, int64_t c2, int64_t d, int64_t ca, int64_t cb,
                          int64_t cc, int64_t cd, int fuel);
int canor_demo_vol_strategy_c(const canor_strategy *st, int64_t out[4]);
int canor_demo_cmd_volume(const canor_demo_config_t *c, int sel, const char *out, int fuel);

/* canor_demo_scene.c */
int canor_demo_cmd_scene(const canor_demo_config_t *c, int sel, const char *out, int fuel);

/* canor_demo_mfbuild.c */
int canor_demo_cmd_mfbuild(const canor_demo_config_t *c, int sel, const char *out, long want);

/* canor_demo_b2i.c */
int canor_demo_cmd_b2i(const canor_demo_config_t *c, const char *arg, const char *out);

/* canor_demo_spacetime.c */
int canor_demo_cmd_spacetime(const canor_demo_config_t *c, int sel, const char *out, long want);

/* canor_demo_surreal.c */
int canor_demo_cmd_surreal(const canor_demo_config_t *c, const char *out);

/* canor_demo_agg.c */
int canor_demo_cmd_agg(const canor_demo_config_t *c, const char *out);

/* canor_demo_interval.c */
int canor_demo_cmd_interval(const canor_demo_config_t *c, const char *out);

/* canor_demo_fw.c */
int canor_demo_cmd_fw(const canor_demo_config_t *cfg, const char *path, const char *out);

/* canor_demo_provenance.c */
bool canor_demo_prov_write_sidecar(const char *path);
int canor_demo_cmd_verify(const char *path);

/* canor_demo_golay.c */
int canor_demo_cmd_golay(void);

/* canor_demo_ore.c */
int canor_demo_cmd_ore(const canor_demo_config_t *c, int sel);

/* canor_demo_determinism.c */
int canor_demo_cmd_determinism(const canor_demo_config_t *c);

#endif /* CANOR_DEMO_H */
