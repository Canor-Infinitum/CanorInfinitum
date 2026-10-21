/* `follow` - one binary for every Strategy/Flow operation, configured by JSON.
 *
 * This replaces the pile of one-shot demo executables that accumulated while
 * the algebra was being built (mandel, mandel2, mandel3, kdemo, stratdemo,
 * detax, ...). Those each hard-coded their Strategy and their four Flows, so
 * trying a different differential family meant editing C and recompiling. The
 * families were already text-driven - canor_family_parse reads "-sin1 +tan2" - so
 * the only thing standing between the code and a configuration file was a file.
 *
 * Everything selectable now lives in follow.json:
 *
 *   strategies[]  name, version, bias, and the three differential families
 *                 di/dj/dk in the compact text form canor_family_parse accepts.
 *   flows[]       name and word over the imaginary units.
 *   settings      fuel, levels, grid size, hypervector shape.
 *
 * Nothing here is hard-coded except the defaults used when a key is absent, and
 * those defaults are the four baseline Flows and the worked example from the
 * brief - the same values the deleted demos used, so behaviour is unchanged
 * when the config is unchanged.
 *
 * THE JSON READER is purpose-built, as in repl.c. A dependency would be a poor
 * trade for the shape read here: a flat object of arrays of flat objects, no
 * escapes beyond the ones the writer of a config would type, no streaming. It
 * refuses what it cannot read rather than guessing, which is the property that
 * matters - a silently half-parsed Strategy would produce plausible numbers.
 *
 * The demo is split by feature: each command lives in canor_demo_<command>.c, the
 * shared types and helpers are declared in canor_demo.h, and this file holds
 * only the usage text, option parsing and command dispatch.
 */

#include "canor_demo.h"

static int usage(void) {
  printf(
    "follow - Strategy and Flow operations over the FollowLang core\n"
    "\n"
    "  follow [options] <command> [arguments]\n"
    "\n"
    "options\n"
    "  --config FILE     configuration to load (default follow.json, then\n"
    "                    ../follow.json; built-in defaults if neither exists)\n"
    "  --strategy NAME   which configured Strategy to use (default: the first)\n"
    "  --fuel N          override settings.fuel\n"
    "  --levels N        override settings.levels\n"
    "  --grid WxH        override the grid\n"
    "  --centre RE,IM    frame centre, as exact rationals (\"-3/4,0\")\n"
    "  --span RE,IM      frame span, as exact rationals (\"1/1000,1/1000\")\n"
    "  --zoom N          divide the configured span by N, keeping the centre\n"
    "  --budget LIMBS    arbitrary-precision ceiling (0 = unlimited)\n"
    "\n"
    "commands\n"
    "  config            what was loaded, and from where\n"
    "  strategies        configured Strategies and their differential families\n"
    "  flows             configured Flows and what each word folds to\n"
    "  fold WORD         fold an arbitrary word over i, j, k\n"
    "  diff [FLOW]       di/dj/dk of every Flow, or one, under the Strategy\n"
    "  rgb STRING        project an RGB string onto Flow/MetaFlow/Mixture\n"
    "  kapprox           the description-length ladder across the level range\n"
    "  mandel            fifth-schema Mandelbrot benchmark over the frame\n"
    "  bigcheck          arbitrary-precision arithmetic acceptance suite\n"
    "  hp4               HP^4 atlas: 5 quaternionic Mandelbrot charts\n"
    "  selmer            Selmer-Lie differentials as an atlas of TST trees\n"
    "  tensor            H^(x)m algebras, chart rotations, voxel zooms\n"
    "  scales            scale-relative operators, ordinals, duality\n"
    "  ricci             discrete flow with surgery, ordinal termination\n"
    "  sed2hp4           sedenions vs H(x)H over one skeleton, and HP^4\n"
    "  tvoxel            a 5xStrategy through H^(x)5, voxelized and zoomed\n"
    "  volume            3D/4D quaternionic escape field, raymarched\n"
    "  scene             volume + PG(3,2) + expr graph, two scale sliders\n"
    "  mfbuild           TST+BBST datasets across Flow/MetaFlow/MultiFlow\n"
    "  b2i [SPEC]        base 2i over the Gaussian lattice, with a filter\n"
    "  surreal           epsilon as a value, and the band crossing\n"
    "  agg               declared equivalence, singularity detector\n"
    "  interval          quantum values: tagged {lo, val, hi}\n"
    "  fw [FILE]         parse and check a FollowLang Master-Strategy\n"
    "  spacetime         OrigoDelta clouds animated by PRE/PERI/POST, PFISD\n"
    "  hp4               HP^4 atlas: 5 quaternionic Mandelbrot charts\n"
    "  selmer            Selmer-Lie differentials as an atlas of TST trees\n"
    "\n"
    "Configuration is read, never written. Every Strategy field is optional and\n"
    "falls back to the worked example from the brief.\n");
  return 0;
}

int main(int argc, char **argv) {
  canor_demo_config_t cfg;
  char err[512];
  const char *path = NULL, *want = NULL, *cmd = NULL;
  const char *arg1 = NULL, *arg2 = NULL, *centre = NULL, *span = NULL, *zoom = NULL;
  const char *grid = NULL, *cloud_path = NULL, *emit = NULL;
  int fuel = 0, levels = 0, sel = 0, i;
  long budget = -1, points = 0;
  int nsurvey = 0, amin = 1;

  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--config") && i + 1 < argc) path = argv[++i];
    else if (!strcmp(argv[i], "--strategy") && i + 1 < argc) want = argv[++i];
    else if (!strcmp(argv[i], "--fuel") && i + 1 < argc) fuel = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--levels") && i + 1 < argc) levels = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--grid") && i + 1 < argc) grid = argv[++i];
    else if (!strcmp(argv[i], "--centre") && i + 1 < argc) centre = argv[++i];
    else if (!strcmp(argv[i], "--center") && i + 1 < argc) centre = argv[++i];
    else if (!strcmp(argv[i], "--span") && i + 1 < argc) span = argv[++i];
    else if (!strcmp(argv[i], "--zoom") && i + 1 < argc) zoom = argv[++i];
    else if (!strcmp(argv[i], "--budget") && i + 1 < argc) budget = atol(argv[++i]);
    else if (!strcmp(argv[i], "--cloud") && i + 1 < argc) cloud_path = argv[++i];
    else if (!strcmp(argv[i], "--emit") && i + 1 < argc) emit = argv[++i];
    else if (!strcmp(argv[i], "--points") && i + 1 < argc) points = atol(argv[++i]);
    else if (!strcmp(argv[i], "--surveys") && i + 1 < argc) nsurvey = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--amin") && i + 1 < argc) amin = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) return usage();
    else if (argv[i][0] == '-') {
      fprintf(stderr, "follow: unknown option %s\n", argv[i]);
      return 2;
    }
    else if (!cmd) cmd = argv[i];
    else if (!arg1) arg1 = argv[i];
    else if (!arg2) arg2 = argv[i];
    else { fprintf(stderr, "follow: too many arguments\n"); return 2; }
  }
  if (!cmd) return usage();
  /* bigcheck exercises the arithmetic itself and must not depend on a config
   * being present or parseable - if the config is what broke, this is the
   * command that has to still run. */
  if (!strcmp(cmd, "bigcheck")) return canor_demo_cmd_bigcheck();

  /* Search order, so the tool works both from the package root and from the
   * repository root without the caller having to say which. */
  if (path) {
    if (!canor_demo_cfg_load(&cfg, path, err, sizeof err)) {
      fprintf(stderr, "follow: %s\n", err);
      return 2;
    }
    if (!cfg.from_file) {
      fprintf(stderr, "follow: %s: no such file\n", path);
      return 2;
    }
  } else {
    if (!canor_demo_cfg_load(&cfg, "follow.json", err, sizeof err)) {
      fprintf(stderr, "follow: %s\n", err);
      return 2;
    }
    if (!cfg.from_file && !canor_demo_cfg_load(&cfg, "../follow.json", err, sizeof err)) {
      fprintf(stderr, "follow: %s\n", err);
      return 2;
    }
  }
  if (fuel > 0) cfg.set.fuel = fuel;
  if (levels > 0) cfg.set.levels = levels;
  if (budget >= 0) cfg.budget = (size_t)budget;
  if (grid) {
    int gw = 0, gh = 0;
    if (sscanf(grid, "%dx%d", &gw, &gh) != 2 || gw < 1 || gh < 1) {
      fprintf(stderr, "follow: --grid wants WxH, got '%s'\n", grid);
      return 2;
    }
    cfg.set.grid_w = gw; cfg.set.grid_h = gh;
  }
  /* Frame overrides. Each half is an exact rational; a pair is "re,im". */
  {
    const char *pairs[2] = { centre, span };
    fb_rat *dst[2][2] = { { &cfg.frame_cr, &cfg.frame_ci },
                          { &cfg.frame_sr, &cfg.frame_si } };
    int k;
    for (k = 0; k < 2; ++k) {
      char buf[256];
      const char *comma;
      if (!pairs[k]) continue;
      comma = strchr(pairs[k], ',');
      if (!comma || (size_t)(comma - pairs[k]) >= sizeof buf) {
        fprintf(stderr, "follow: expected RE,IM, got '%s'\n", pairs[k]);
        return 2;
      }
      memcpy(buf, pairs[k], (size_t)(comma - pairs[k]));
      buf[comma - pairs[k]] = 0;
      if (!fb_rat_parse(dst[k][0], buf) || !fb_rat_parse(dst[k][1], comma + 1)) {
        fprintf(stderr, "follow: '%s' is not a pair of rationals\n", pairs[k]);
        return 2;
      }
    }
  }
  if (zoom) {
    /* Dividing the span by an exact integer keeps the frame exact, so a zoom
     * of 10^12 is a real 10^12 and not a rounded one. */
    fb_rat z;
    bool ok;
    fb_rat_init(&z);
    ok = fb_rat_parse(&z, zoom) && fb_rat_cmp_i64(&z, 0) > 0;
    if (ok) {
      fb_rat inv;
      fb_rat_init(&inv);
      ok = fb_copy(&inv.num, &z.den) && fb_copy(&inv.den, &z.num) &&
           fb_rat_normalize(&inv) &&
           fb_rat_mul(&cfg.frame_sr, &cfg.frame_sr, &inv) &&
           fb_rat_mul(&cfg.frame_si, &cfg.frame_si, &inv);
      fb_rat_free(&inv);
    }
    fb_rat_free(&z);
    if (!ok) {
      fprintf(stderr, "follow: --zoom wants a positive rational, got '%s'\n", zoom);
      return 2;
    }
  }
  if (fb_rat_cmp_i64(&cfg.frame_sr, 0) <= 0 ||
      fb_rat_cmp_i64(&cfg.frame_si, 0) <= 0) {
    fprintf(stderr, "follow: frame span must be positive\n");
    return 2;
  }
  fb_budget_set(cfg.budget);

  if (!want && cfg.default_strategy[0]) want = cfg.default_strategy;
  if (want) {
    sel = canor_demo_strat_find(&cfg, want);
    if (sel < 0) {
      /* A default naming a Strategy that is not present is a config error, not
       * something to fall back from. Quietly using a different Strategy than
       * the one named would make every downstream number wrong and plausible. */
      fprintf(stderr, "follow: no strategy named '%s' in %s\n", want, cfg.path);
      return 2;
    }
  }

  /* The imaginary tables back every fold; a command that folds without them is
   * reporting a product that was never computed. */
  /* Bind Ore parameters and swap the abstract operators in. Done once, here,
   * so every command downstream - diff, cloud, bias, tree - runs whichever
   * operators the Strategy declared without any of them knowing about it. */
  for (i = 0; i < cfg.nstrat; ++i)
    if (cfg.strat_use_ore[i]) {
      int k;
      for (k = 0; k < 3; ++k)
        if (!canor_ore_bind_op(cfg.strat[i].id.name, (canor_absop)k,
                            &cfg.strat_ore[i][k])) {
          fprintf(stderr, "follow: too many Ore strategies to bind\n");
          return 2;
        }
      cfg.strat[i].ops = canor_ore_absop;
    }

  if (canor_imag_init() != CANOR_OK) {
    fprintf(stderr, "follow: the imaginary tables do not close\n");
    return 2;
  }

  if (!strcmp(cmd, "config"))     return canor_demo_cmd_config(&cfg);
  if (!strcmp(cmd, "strategies")) return canor_demo_cmd_strategies(&cfg, sel);
  if (!strcmp(cmd, "flows"))      return canor_demo_cmd_flows(&cfg);
  if (!strcmp(cmd, "metaflows"))  return canor_demo_cmd_metaflows(&cfg);
  if (!strcmp(cmd, "kapprox"))    return canor_demo_cmd_kapprox(&cfg);
  if (!strcmp(cmd, "mandel"))     return canor_demo_cmd_mandel(&cfg);
  if (!strcmp(cmd, "describe"))   return canor_demo_cmd_describe(&cfg);
  if (!strcmp(cmd, "determinism")) return canor_demo_cmd_determinism(&cfg);
  if (!strcmp(cmd, "ore"))        return canor_demo_cmd_ore(&cfg, sel);
  if (!strcmp(cmd, "excite"))     return canor_demo_cmd_excite(&cfg, sel);
  if (!strcmp(cmd, "golay"))      return canor_demo_cmd_golay();
  if (!strcmp(cmd, "symgraph"))   return canor_demo_cmd_symgraph();
  if (!strcmp(cmd, "primes"))     return canor_demo_cmd_primes();
  if (!strcmp(cmd, "poly"))       return canor_demo_cmd_poly();
  if (!strcmp(cmd, "units"))      return canor_demo_cmd_units();
  if (!strcmp(cmd, "alg"))        return canor_demo_cmd_alg();
  if (!strcmp(cmd, "verify")) {
    if (!arg1) { fprintf(stderr, "follow: verify needs a file\n"); return 2; }
    return canor_demo_cmd_verify(arg1);
  }
  if (!strcmp(cmd, "spreads"))    return canor_demo_cmd_spreads();
  if (!strcmp(cmd, "pgview"))     return canor_demo_cmd_pgview(&cfg, emit);
  if (!strcmp(cmd, "mfview"))     return canor_demo_cmd_mfview(&cfg, sel, emit);
  if (!strcmp(cmd, "hp4"))        return canor_demo_cmd_hp4(&cfg, sel, emit);
  if (!strcmp(cmd, "fw"))
    return canor_demo_cmd_fw(&cfg, arg1, emit ? emit : "fw.html");
  if (!strcmp(cmd, "interval"))
    return canor_demo_cmd_interval(&cfg, emit ? emit : "interval.html");
  if (!strcmp(cmd, "agg"))
    return canor_demo_cmd_agg(&cfg, emit ? emit : "agg.html");
  if (!strcmp(cmd, "surreal"))
    return canor_demo_cmd_surreal(&cfg, emit ? emit : "surreal.html");
  if (!strcmp(cmd, "spacetime"))
    return canor_demo_cmd_spacetime(&cfg, sel, emit ? emit : "spacetime.html",
                                    points > 0 ? points : cfg.points);
  if (!strcmp(cmd, "b2i"))
    return canor_demo_cmd_b2i(&cfg, arg1, emit ? emit : "b2i.html");
  if (!strcmp(cmd, "mfbuild"))
    return canor_demo_cmd_mfbuild(&cfg, sel, emit ? emit : "mfbuild.html",
                                  points > 0 ? points : cfg.points);
  if (!strcmp(cmd, "scene"))
    return canor_demo_cmd_scene(&cfg, sel, emit ? emit : "scene.html", fuel);
  if (!strcmp(cmd, "volume"))
    return canor_demo_cmd_volume(&cfg, sel, emit ? emit : "volume.html", fuel);
  if (!strcmp(cmd, "tvoxel"))
    return canor_demo_cmd_tvoxel(&cfg, sel, emit ? emit : "tvoxel.html");
  if (!strcmp(cmd, "sed2hp4"))
    return canor_demo_cmd_sed2hp4(&cfg, sel, emit ? emit : "sed2hp4.html");
  if (!strcmp(cmd, "ricci"))
    return canor_demo_cmd_ricci(&cfg, sel, emit ? emit : "ricci.html");
  if (!strcmp(cmd, "scales"))
    return canor_demo_cmd_scales(&cfg, sel, emit ? emit : "scales.html");
  if (!strcmp(cmd, "tensor"))
    return canor_demo_cmd_tensor(&cfg, sel, emit ? emit : "tensor.html");
  if (!strcmp(cmd, "selmer"))
    return canor_demo_cmd_selmer(&cfg, sel, cloud_path, points > 0 ? points : cfg.points,
                                 emit ? emit : "selmer.html");
  if (!strcmp(cmd, "graph"))
    return canor_demo_cmd_graph(&cfg, sel, emit, points > 0 ? points : cfg.points);
  if (!strcmp(cmd, "spec"))
    return canor_demo_cmd_spec(&cfg, sel, emit, points > 0 ? points : cfg.points);
  if (!strcmp(cmd, "qualia"))     return canor_demo_cmd_qualia(&cfg, sel, arg1);
  if (!strcmp(cmd, "ensemble"))
    return canor_demo_cmd_ensemble(&cfg, cloud_path, points > 0 ? points : cfg.points);
  if (!strcmp(cmd, "questions"))
    return canor_demo_cmd_questions(&cfg, sel, arg1 ? arg1 : "../words_dictionary.json",
                                    emit, points);
  if (!strcmp(cmd, "survey"))
    return canor_demo_cmd_survey(&cfg, sel, cloud_path ? cloud_path : arg1,
                                 points > 0 ? points : cfg.points,
                                 nsurvey > 0 ? nsurvey : 4, amin);
  if (!strcmp(cmd, "tree"))
    return canor_demo_cmd_tree(&cfg, sel, arg1 ? arg1 : "../words_dictionary.json", emit);
  if (!strcmp(cmd, "beta"))
    return canor_demo_cmd_beta(&cfg, arg1 ? arg1 : "../v1-beta.json");
  if (!strcmp(cmd, "cloud"))
    return canor_demo_cmd_cloud(&cfg, sel, cloud_path, points > 0 ? points : cfg.points, emit);
  if (!strcmp(cmd, "bias"))
    return canor_demo_cmd_bias(&cfg, sel, cloud_path, points > 0 ? points : cfg.points);
  if (!strcmp(cmd, "diff"))       return canor_demo_cmd_diff(&cfg, sel, arg1);
  if (!strcmp(cmd, "fold")) {
    if (!arg1) { fprintf(stderr, "follow: fold needs a word\n"); return 2; }
    return canor_demo_cmd_fold(&cfg, arg1, arg2);
  }
  if (!strcmp(cmd, "rgb")) {
    if (!arg1) { fprintf(stderr, "follow: rgb needs a string\n"); return 2; }
    return canor_demo_cmd_rgb(&cfg, arg1);
  }

  fprintf(stderr, "follow: unknown command '%s'\n", cmd);
  return 2;
}

