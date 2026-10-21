/* canor_demo_config.c - configuration
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* configuration                                                              */
/* ========================================================================== */

static void cfg_defaults(canor_demo_config_t *c) {
  memset(c, 0, sizeof *c);
  fb_rat_init(&c->frame_cr); fb_rat_init(&c->frame_ci);
  fb_rat_init(&c->frame_sr); fb_rat_init(&c->frame_si);
  /* The classic view: centre -1/2, span 3 by 5/2. */
  fb_rat_set_i64(&c->frame_cr, -1, 2);
  fb_rat_set_i64(&c->frame_ci, 0, 1);
  fb_rat_set_i64(&c->frame_sr, 3, 1);
  fb_rat_set_i64(&c->frame_si, 5, 2);
  c->budget = 1u << 20;
  c->default_strategy[0] = 0;
  c->points = 4096;
  snprintf(c->letters[0], sizeof c->letters[0], "enruwyk");
  snprintf(c->letters[1], sizeof c->letters[1], "tihlcgbv");
  snprintf(c->letters[2], sizeof c->letters[2], "aosdmfpjxqz");
  snprintf(c->strat_name[0], sizeof c->strat_name[0], "example");
  canor_strategy_default(&c->strat[0], c->strat_name[0], 1);
  snprintf(c->strat_d[0][0], 128, "-sin1 +tan2 +sin3 -sin4");
  snprintf(c->strat_d[0][1], 128, "+cos1*sin2 -cos3*tan4");
  snprintf(c->strat_d[0][2], 128, "+cos1 -sin2*tan3/sin4");
  c->nstrat = 1;
  canor_flows_default(&c->flows);
  {
    static const char *const mn[CANOR_DEMO_AXIS_N] =
      { "General", "Specific", "Distinctive", "Arbitrary" };
    static const char *const xn[CANOR_DEMO_AXIS_N] =
      { "Elementary", "Unique", "Composite", "Unknown" };
    static const int ch[CANOR_DEMO_AXIS_N] = { 0, 1, 2, -1 };
    static const char un[CANOR_DEMO_AXIS_N] = { 'i', 'j', 'k', 0 };
    static const int dm[CANOR_DEMO_AXIS_N] = { 1, 1, 2, -1 };
    static const bool dom[CANOR_DEMO_AXIS_N] = { false, true, true, false };
    int i;
    for (i = 0; i < CANOR_DEMO_AXIS_N; ++i) {
      snprintf(c->meta[i].name, sizeof c->meta[i].name, "%s", mn[i]);
      c->meta[i].channel = ch[i];
      c->meta[i].unit = un[i];
      c->meta[i].kclass = (canor_kclass)i;
      snprintf(c->mix[i].name, sizeof c->mix[i].name, "%s", xn[i]);
      c->mix[i].dims = dm[i];
      c->mix[i].dominant = dom[i];
    }
  }
  c->set.fuel = 12;
  c->set.levels = (int)CANOR_LEVELS_MIN;
  c->set.hv_tables = 12;
  c->set.hv_bits = 14;
  c->set.grid_w = 64;
  c->set.grid_h = 32;
  snprintf(c->path, sizeof c->path, "(built-in defaults)");
}

static BiasMode bias_of(const char *s) {
  if (!strcmp(s, "right"))    return RightBiased;
  if (!strcmp(s, "neutral"))  return NeutralBiased;
  if (!strcmp(s, "strategy")) return StrategyBiased;
  return LeftBiased;
}
const char *canor_demo_bias_name(BiasMode b) {
  return b == RightBiased ? "right" : b == NeutralBiased ? "neutral"
       : b == StrategyBiased ? "strategy" : "left";
}

/* The per-operator operators block:
 *     "operators": {
 *       "sin": { "a": "1",  "q": "1", "s0": "1" },
 *       "cos": { "a": "0",  "q": "1", "s0": "1" },
 *       "tan": { "a": "-1", "q": "1", "s0": "2" }
 *     }
 * An operator that is named but incompletely specified inherits the defaults
 * rather than being half-configured; an operator that is not named at all
 * keeps a = q = s0 = 1. Both are stated rather than silent, because "sin was
 * never configured" and "sin was configured to the default" are the same
 * numbers and different facts. */
static bool cfg_operators(canor_demo_jscan_t *j, char oa[3][32], char oq[3][32],
                          char os0[3][32], int *seen) {
  char key[64];
  if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected operators object"); return false; }
  if (canor_demo_jlit(j, '}')) return true;
  do {
    int k = -1;
    if (!canor_demo_jstr(j, key, sizeof key)) return false;
    if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
    if (!strcmp(key, "sin")) k = 0;
    else if (!strcmp(key, "cos")) k = 1;
    else if (!strcmp(key, "tan")) k = 2;
    if (k < 0) {
      snprintf(j->err, sizeof j->err,
               "operators: '%s' is not an abstract operator (sin, cos, tan)", key);
      return false;
    }
    if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected an operator object"); return false; }
    if (!canor_demo_jlit(j, '}')) {
      do {
        char k2[64];
        if (!canor_demo_jstr(j, k2, sizeof k2)) return false;
        if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
        /* The Ore names, with the older letters kept as aliases so configs
         * already written keep working. */
        if      (!strcmp(k2, "tau") || !strcmp(k2, "a"))
          { if (!canor_demo_jstr(j, oa[k], 32)) return false; }
        else if (!strcmp(k2, "sigma") || !strcmp(k2, "q"))
          { if (!canor_demo_jstr(j, oq[k], 32)) return false; }
        else if (!strcmp(k2, "delta_amp") || !strcmp(k2, "s0"))
          { if (!canor_demo_jstr(j, os0[k], 32)) return false; }
        else if (!strcmp(k2, "kind")) {
          char kind[16];
          if (!canor_demo_jstr(j, kind, sizeof kind)) return false;
          if (strcmp(kind, "ore")) {
            snprintf(j->err, sizeof j->err,
                     "operators.%s.kind must be \"ore\"", key);
            return false;
          }
        }
        else if (!canor_demo_jskip(j)) return false;
      } while (canor_demo_jlit(j, ','));
      if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in operator"); return false; }
    }
    *seen |= (1 << k);
  } while (canor_demo_jlit(j, ','));
  if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in operators"); return false; }
  return true;
}

const char *const canor_demo_cfg_dname[CANOR_DEMO_CFG_DNAMES] = {
  "di", "dj", "dk",   "dm", "dn", "do",   "dr", "ds", "dt",
  "de", "df", "dg",   "dp", "dq", "du"
};

static int cfg_dindex(const char *key) {
  int i;
  for (i = 0; i < CANOR_DEMO_CFG_DNAMES; ++i) if (!strcmp(key, canor_demo_cfg_dname[i])) return i;
  return -1;
}

static bool cfg_one_strategy(canor_demo_jscan_t *j, canor_demo_config_t *c) {
  char key[64], name[32] = "unnamed", bias[16] = "left";
  char dtext[CANOR_DEMO_CFG_DNAMES][128];
  int di_i;
  char ops[16] = "";
  long ntrif = 1;                 /* 1 unless the Strategy says otherwise */
  char oa[3][32] = { "1", "1", "1" };
  char oq[3][32] = { "1", "1", "1" };
  char os0[3][32] = { "1", "1", "1" };
  int opseen = 0;
  long version = 1;
  int idx = c->nstrat;
  canor_strategy *s;

  for (di_i = 0; di_i < CANOR_DEMO_CFG_DNAMES; ++di_i) dtext[di_i][0] = 0;
  if (idx >= CANOR_DEMO_CFG_STRAT_MAX) { canor_demo_jfail(j, "too many strategies"); return false; }
  if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected strategy object"); return false; }
  if (!canor_demo_jlit(j, '}')) {
    do {
      if (!canor_demo_jstr(j, key, sizeof key)) return false;
      if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
      if      (!strcmp(key, "name"))    { if (!canor_demo_jstr(j, name, sizeof name)) return false; }
      else if (!strcmp(key, "bias"))    { if (!canor_demo_jstr(j, bias, sizeof bias)) return false; }
      else if (!strcmp(key, "version")) { if (!canor_demo_jnum(j, &version)) return false; }
      else if (cfg_dindex(key) >= 0) {
        if (!canor_demo_jstr(j, dtext[cfg_dindex(key)], 128)) return false;
      }
      /* FULLY-FLEDGED IS A DECLARATION, NOT A DEFAULT. A Strategy carries one
       * trifecta unless it says it carries more, and canor_dtc refuses an
       * undeclared one rather than evaluating it - so five families' worth of
       * structure has to be asked for. Without this key no Strategy could ever
       * be fully-fledged, which is what the HP^4 atlas exposed: four of five
       * charts reported nothing because four of five trifectas were, correctly,
       * undeclared. */
      else if (!strcmp(key, "trifectas")) {
        if (!canor_demo_jnum(j, &ntrif)) return false;
        if (ntrif < 1 || ntrif > CANOR_TRIFECTAS) {
          snprintf(j->err, sizeof j->err,
                   "strategy '%s': trifectas must be 1..%d", name, CANOR_TRIFECTAS);
          return false;
        }
      }
      else if (!strcmp(key, "operators")) {
        /* Two forms. The string form ("ore" / "hash") applies one setting to
         * all three operators; the object form gives each its own. */
        canor_demo_jws(j);
        if (j->p < j->end && *j->p == '"') {
          if (!canor_demo_jstr(j, ops, sizeof ops)) return false;
        } else if (!cfg_operators(j, oa, oq, os0, &opseen)) {
          return false;
        } else {
          snprintf(ops, sizeof ops, "ore");
        }
      }
      /* The old per-strategy keys still work and set all three, so existing
       * configs keep their exact behaviour. */
      else if (!strcmp(key, "ore_tau") || !strcmp(key, "ore_a"))
                                       { if (!canor_demo_jstr(j, oa[0], 32)) return false;
                                         snprintf(oa[1], 32, "%s", oa[0]);
                                         snprintf(oa[2], 32, "%s", oa[0]); }
      else if (!strcmp(key, "ore_sigma") || !strcmp(key, "ore_q"))
                                       { if (!canor_demo_jstr(j, oq[0], 32)) return false;
                                         snprintf(oq[1], 32, "%s", oq[0]);
                                         snprintf(oq[2], 32, "%s", oq[0]); }
      else if (!strcmp(key, "ore_delta_amp") || !strcmp(key, "ore_s0"))
                                       { if (!canor_demo_jstr(j, os0[0], 32)) return false;
                                         snprintf(os0[1], 32, "%s", os0[0]);
                                         snprintf(os0[2], 32, "%s", os0[0]); }
      else if (!canor_demo_jskip(j)) return false;
    } while (canor_demo_jlit(j, ','));
    if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in strategy"); return false; }
  }

  s = &c->strat[idx];
  snprintf(c->strat_name[idx], sizeof c->strat_name[idx], "%s", name);
  /* Start from the worked example, then override only what was given: a
   * config that names a Strategy but omits dk gets the example's dk rather
   * than an empty family that silently evaluates to a zero. */
  canor_strategy_default(s, c->strat_name[idx], (uint32_t)version);
  s->bias = bias_of(bias);
  s->ntrifecta = (int)ntrif;
  for (di_i = 0; di_i < CANOR_DEMO_CFG_DNAMES; ++di_i) {
    int t = di_i / 3, k = di_i % 3;
    if (!dtext[di_i][0]) {
      /* Undeclared: keep whatever canor_strategy_default installed, and record it
       * as the empty string so `strategies` can show it as a default rather
       * than reprinting it as though it had been asked for. */
      c->strat_d[idx][di_i][0] = 0;
      continue;
    }
    snprintf(c->strat_d[idx][di_i], 128, "%.127s", dtext[di_i]);
    if (!canor_family_parse(&s->d[t][k], canor_demo_cfg_dname[di_i], c->strat_d[idx][di_i])) {
      snprintf(j->err, sizeof j->err, "strategy '%s': cannot parse %s",
               name, canor_demo_cfg_dname[di_i]);
      return false;
    }
  }
  c->strat_use_ore[idx] = 0;
  if (ops[0] && !strcmp(ops, "ore")) {
    int k;
    for (k = 0; k < 3; ++k)
      if (!canor_ore_init(&c->strat_ore[idx][k], oa[k], oq[k], os0[k])) {
        snprintf(j->err, sizeof j->err,
                 "strategy '%s': operator %s has tau/sigma/delta_amp that are not exact "
                 "rationals (\"1\", \"-2\", \"3/2\")", name,
                 k == 0 ? "sin" : k == 1 ? "cos" : "tan");
        return false;
      }
    c->strat_use_ore[idx] = 1;
  } else if (ops[0] && strcmp(ops, "hash")) {
    snprintf(j->err, sizeof j->err,
             "strategy '%s': operators must be \"ore\" or \"hash\"", name);
    return false;
  }
  ++c->nstrat;
  return true;
}

static bool cfg_one_flow(canor_demo_jscan_t *j, canor_demo_config_t *c) {
  char key[64], name[24] = "", word[CANOR_FLOW_WORD_MAX] = "";
  if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected flow object"); return false; }
  if (!canor_demo_jlit(j, '}')) {
    do {
      if (!canor_demo_jstr(j, key, sizeof key)) return false;
      if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
      if      (!strcmp(key, "name")) { if (!canor_demo_jstr(j, name, sizeof name)) return false; }
      else if (!strcmp(key, "word")) { if (!canor_demo_jstr(j, word, sizeof word)) return false; }
      else if (!canor_demo_jskip(j)) return false;
    } while (canor_demo_jlit(j, ','));
    if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in flow"); return false; }
  }
  if (!name[0] || !word[0]) { canor_demo_jfail(j, "flow needs name and word"); return false; }
  if (!canor_flows_add(&c->flows, name, word)) {
    snprintf(j->err, sizeof j->err, "flow '%s': bad word '%s'", name, word);
    return false;
  }
  return true;
}

static bool kclass_of(const char *s, canor_kclass *out) {
  if (!strcmp(s, "general"))     { *out = CANOR_K_GENERAL;     return true; }
  if (!strcmp(s, "specific"))    { *out = CANOR_K_SPECIFIC;    return true; }
  if (!strcmp(s, "distinctive")) { *out = CANOR_K_DISTINCTIVE; return true; }
  if (!strcmp(s, "arbitrary"))   { *out = CANOR_K_ARBITRARY;   return true; }
  return false;
}
const char *canor_demo_kclass_name(canor_kclass k) {
  static const char *const n[4] = { "2n+1", "2(n!)+1", "(2n+1)!", "(2(n!)+1)!" };
  return n[(int)k & 3];
}

/* The MetaFlow axis. Exactly four entries, the last of which must be the
 * refusal - a config that gives four dominances and no refusal has nowhere to
 * put a balanced string, and would have to answer one anyway. */
static bool cfg_metaflows(canor_demo_jscan_t *j, canor_demo_config_t *c) {
  int n = 0;
  if (!canor_demo_jlit(j, '[')) { canor_demo_jfail(j, "metaflows must be an array"); return false; }
  if (canor_demo_jlit(j, ']')) { canor_demo_jfail(j, "metaflows cannot be empty"); return false; }
  do {
    char key[64], name[24] = "", chan[8] = "", kc[24] = "", un[8] = "";
    canor_demo_metaflow_def_t d;
    if (n >= CANOR_DEMO_AXIS_N) { canor_demo_jfail(j, "metaflows takes exactly four entries"); return false; }
    if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected metaflow object"); return false; }
    if (!canor_demo_jlit(j, '}')) {
      do {
        if (!canor_demo_jstr(j, key, sizeof key)) return false;
        if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
        if      (!strcmp(key, "name"))    { if (!canor_demo_jstr(j, name, sizeof name)) return false; }
        else if (!strcmp(key, "channel")) { if (!canor_demo_jstr(j, chan, sizeof chan)) return false; }
        else if (!strcmp(key, "kclass"))  { if (!canor_demo_jstr(j, kc, sizeof kc)) return false; }
        else if (!strcmp(key, "unit"))    { if (!canor_demo_jstr(j, un, sizeof un)) return false; }
        else if (!canor_demo_jskip(j)) return false;
      } while (canor_demo_jlit(j, ','));
      if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in metaflow"); return false; }
    }
    if (!name[0]) { canor_demo_jfail(j, "metaflow needs a name"); return false; }
    memset(&d, 0, sizeof d);
    snprintf(d.name, sizeof d.name, "%s", name);
    if (!chan[0] || !strcmp(chan, "none")) d.channel = -1;
    else if (!strcmp(chan, "R")) d.channel = 0;
    else if (!strcmp(chan, "G")) d.channel = 1;
    else if (!strcmp(chan, "B")) d.channel = 2;
    else {
      snprintf(j->err, sizeof j->err,
               "metaflow '%s': channel must be R, G, B or none", name);
      return false;
    }
    /* The channel-to-unit projection. It lives HERE, next to the channel,
     * because it is the same fact: a MetaFlow entry says what R means at the
     * RGB level and what it becomes at the IMAG level, and splitting those
     * across two config sections would let them disagree. The refusal slot has
     * no unit, which is right - nothing projects to nothing. */
    d.unit = 0;
    if (un[0]) {
      if ((un[0] != 'i' && un[0] != 'j' && un[0] != 'k' && un[0] != '1') || un[1]) {
        snprintf(j->err, sizeof j->err,
                 "metaflow '%s': unit must be i, j, k or 1", name);
        return false;
      }
      d.unit = un[0];
    } else if (d.channel >= 0) {
      d.unit = "ijk"[d.channel];
    }
    d.kclass = (canor_kclass)n;
    if (kc[0] && !kclass_of(kc, &d.kclass)) {
      snprintf(j->err, sizeof j->err, "metaflow '%s': unknown kclass '%s'", name, kc);
      return false;
    }
    c->meta[n++] = d;
  } while (canor_demo_jlit(j, ','));
  if (!canor_demo_jlit(j, ']')) { canor_demo_jfail(j, "expected ']'"); return false; }
  if (n != CANOR_DEMO_AXIS_N) {
    snprintf(j->err, sizeof j->err,
             "metaflows needs exactly %d entries, got %d - the axis is "
             "four-valued and the fourth slot is the refusal", CANOR_DEMO_AXIS_N, n);
    return false;
  }
  {
    /* Distinct units, or the projection is not injective and two different RGB
     * strings fold to the same word for a reason that has nothing to do with
     * the algebra. This is the check that catches a config typo which would
     * otherwise look like a surprising result. */
    int p, q;
    for (p = 0; p < CANOR_DEMO_AXIS_N; ++p)
      for (q = p + 1; q < CANOR_DEMO_AXIS_N; ++q)
        if (c->meta[p].unit && c->meta[p].unit == c->meta[q].unit) {
          snprintf(j->err, sizeof j->err,
                   "metaflows '%s' and '%s' both project to '%c'; the "
                   "channel-to-unit map must be injective",
                   c->meta[p].name, c->meta[q].name, c->meta[p].unit);
          return false;
        }
  }
  if (c->meta[CANOR_DEMO_AXIS_N - 1].channel != -1) {
    snprintf(j->err, sizeof j->err,
             "the last metaflow ('%s') must have channel \"none\": it is the "
             "slot a string with no dominant channel lands in",
             c->meta[CANOR_DEMO_AXIS_N - 1].name);
    return false;
  }
  return true;
}

static bool cfg_mixtures(canor_demo_jscan_t *j, canor_demo_config_t *c) {
  int n = 0;
  if (!canor_demo_jlit(j, '[')) { canor_demo_jfail(j, "mixtures must be an array"); return false; }
  if (canor_demo_jlit(j, ']')) { canor_demo_jfail(j, "mixtures cannot be empty"); return false; }
  do {
    char key[64], name[24] = "";
    canor_demo_mixture_def_t d;
    long dims = -1;
    bool dominant = false, saw_dims = false;
    if (n >= CANOR_DEMO_AXIS_N) { canor_demo_jfail(j, "mixtures takes exactly four entries"); return false; }
    if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected mixture object"); return false; }
    if (!canor_demo_jlit(j, '}')) {
      do {
        if (!canor_demo_jstr(j, key, sizeof key)) return false;
        if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
        if (!strcmp(key, "name")) { if (!canor_demo_jstr(j, name, sizeof name)) return false; }
        else if (!strcmp(key, "dims")) {
          if (!canor_demo_jnum(j, &dims)) return false;
          saw_dims = true;
        } else if (!strcmp(key, "dominant")) {
          canor_demo_jws(j);
          if (!strncmp(j->p, "true", 4)) { dominant = true; j->p += 4; }
          else if (!strncmp(j->p, "false", 5)) { dominant = false; j->p += 5; }
          else { canor_demo_jfail(j, "dominant must be true or false"); return false; }
        } else if (!canor_demo_jskip(j)) return false;
      } while (canor_demo_jlit(j, ','));
      if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in mixture"); return false; }
    }
    if (!name[0]) { canor_demo_jfail(j, "mixture needs a name"); return false; }
    memset(&d, 0, sizeof d);
    snprintf(d.name, sizeof d.name, "%s", name);
    d.dims = saw_dims ? (int)dims : -1;
    d.dominant = dominant;
    c->mix[n++] = d;
  } while (canor_demo_jlit(j, ','));
  if (!canor_demo_jlit(j, ']')) { canor_demo_jfail(j, "expected ']'"); return false; }
  if (n != CANOR_DEMO_AXIS_N) {
    snprintf(j->err, sizeof j->err,
             "mixtures needs exactly %d entries, got %d", CANOR_DEMO_AXIS_N, n);
    return false;
  }
  return true;
}

/* The letter-to-channel map: three strings, one per channel. A letter that
 * appears in two channels, or in none, would make the projection ambiguous or
 * lossy without saying so, and both are refused. */
static bool cfg_letters(canor_demo_jscan_t *j, canor_demo_config_t *c) {
  char key[64], val[32];
  int i, k;
  if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected letters object"); return false; }
  if (canor_demo_jlit(j, '}')) return true;
  do {
    int tgt = -1;
    if (!canor_demo_jstr(j, key, sizeof key)) return false;
    if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
    if (!strcmp(key, "R")) tgt = 0;
    else if (!strcmp(key, "G")) tgt = 1;
    else if (!strcmp(key, "B")) tgt = 2;
    if (tgt < 0) { if (!canor_demo_jskip(j)) return false; continue; }
    if (!canor_demo_jstr(j, val, sizeof val)) return false;
    snprintf(c->letters[tgt], sizeof c->letters[tgt], "%s", val);
  } while (canor_demo_jlit(j, ','));
  if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in letters"); return false; }
  for (i = 0; i < 3; ++i)
    for (k = i + 1; k < 3; ++k) {
      const char *a;
      for (a = c->letters[i]; *a; ++a) {
        const char *b;
        for (b = c->letters[k]; *b; ++b)
          if (*a == *b) {
            snprintf(j->err, sizeof j->err,
                     "letter '%c' is in two channels; the map must be a partition", *a);
            return false;
          }
      }
    }
  return true;
}

/* Frame values are read as strings, not numbers, because they are rationals
 * and the point of the exercise is that they stay exact: "-3/4", "1/1000000",
 * "-0.3125". A JSON number would be a double the moment anything parsed it. */
static bool cfg_frame(canor_demo_jscan_t *j, canor_demo_config_t *c) {
  char key[64], val[256];
  if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected frame object"); return false; }
  if (canor_demo_jlit(j, '}')) return true;
  do {
    fb_rat *tgt = NULL;
    if (!canor_demo_jstr(j, key, sizeof key)) return false;
    if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
    if      (!strcmp(key, "centre_re") || !strcmp(key, "center_re")) tgt = &c->frame_cr;
    else if (!strcmp(key, "centre_im") || !strcmp(key, "center_im")) tgt = &c->frame_ci;
    else if (!strcmp(key, "span_re")) tgt = &c->frame_sr;
    else if (!strcmp(key, "span_im")) tgt = &c->frame_si;
    if (!tgt) { if (!canor_demo_jskip(j)) return false; continue; }
    if (!canor_demo_jstr(j, val, sizeof val)) return false;
    if (!fb_rat_parse(tgt, val)) {
      snprintf(j->err, sizeof j->err, "frame.%s: '%s' is not a rational", key, val);
      return false;
    }
  } while (canor_demo_jlit(j, ','));
  if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in frame"); return false; }
  if (fb_rat_cmp_i64(&c->frame_sr, 0) <= 0 || fb_rat_cmp_i64(&c->frame_si, 0) <= 0) {
    snprintf(j->err, sizeof j->err, "frame span must be positive");
    return false;
  }
  return true;
}

static bool cfg_settings(canor_demo_jscan_t *j, canor_demo_config_t *c) {
  char key[64];
  long v;
  if (!canor_demo_jlit(j, '{')) { canor_demo_jfail(j, "expected settings object"); return false; }
  if (canor_demo_jlit(j, '}')) return true;
  do {
    if (!canor_demo_jstr(j, key, sizeof key)) return false;
    if (!canor_demo_jlit(j, ':')) { canor_demo_jfail(j, "expected ':'"); return false; }
    if      (!strcmp(key, "fuel"))      { if (!canor_demo_jnum(j, &v)) return false; c->set.fuel = (int)v; }
    else if (!strcmp(key, "levels"))    { if (!canor_demo_jnum(j, &v)) return false; c->set.levels = (int)v; }
    else if (!strcmp(key, "hv_tables")) { if (!canor_demo_jnum(j, &v)) return false; c->set.hv_tables = (int)v; }
    else if (!strcmp(key, "hv_bits"))   { if (!canor_demo_jnum(j, &v)) return false; c->set.hv_bits = (int)v; }
    else if (!strcmp(key, "grid_w"))    { if (!canor_demo_jnum(j, &v)) return false; c->set.grid_w = (int)v; }
    else if (!strcmp(key, "grid_h"))    { if (!canor_demo_jnum(j, &v)) return false; c->set.grid_h = (int)v; }
    else if (!strcmp(key, "budget_limbs")) { if (!canor_demo_jnum(j, &v)) return false; c->budget = (size_t)v; }
    else if (!strcmp(key, "points")) { if (!canor_demo_jnum(j, &v)) return false; c->points = v; }
    else if (!strcmp(key, "default_strategy")) {
      if (!canor_demo_jstr(j, c->default_strategy, sizeof c->default_strategy)) return false;
    }
    else if (!canor_demo_jskip(j)) return false;
  } while (canor_demo_jlit(j, ','));
  if (!canor_demo_jlit(j, '}')) { canor_demo_jfail(j, "expected '}' in settings"); return false; }
  return true;
}

/* Returns false with `err` set. A missing file is NOT an error - the built-in
 * defaults stand and the caller is told which was used - but a malformed one
 * is, because a config that half-loaded is worse than no config. */
bool canor_demo_cfg_load(canor_demo_config_t *c, const char *path, char *err, int errcap) {
  FILE *f;
  long len;
  char *buf;
  canor_demo_jscan_t j;
  char key[64];
  bool ok = true;

  cfg_defaults(c);
  err[0] = 0;
  f = fopen(path, "rb");
  if (!f) return true;

  fseek(f, 0, SEEK_END);
  len = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (len <= 0 || len > (1L << 22)) {
    fclose(f);
    snprintf(err, errcap, "%s: implausible size %ld", path, len);
    return false;
  }
  buf = (char *)malloc((size_t)len + 1);
  if (!buf) { fclose(f); snprintf(err, errcap, "out of memory"); return false; }
  if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
    fclose(f); free(buf);
    snprintf(err, errcap, "%s: short read", path);
    return false;
  }
  fclose(f);
  buf[len] = 0;

  memset(&j, 0, sizeof j);
  j.p = buf;
  j.end = buf + len;

  /* Config wins outright over defaults for any section it declares, so a
   * config listing two Flows gets two Flows and not two plus the four
   * baselines. Sections it omits keep their defaults. */
  if (!canor_demo_jlit(&j, '{')) { canor_demo_jfail(&j, "expected object at top level"); ok = false; }
  else if (!canor_demo_jlit(&j, '}')) {
    do {
      if (!canor_demo_jstr(&j, key, sizeof key)) { ok = false; break; }
      if (!canor_demo_jlit(&j, ':')) { canor_demo_jfail(&j, "expected ':'"); ok = false; break; }
      if (!strcmp(key, "strategies")) {
        c->nstrat = 0;
        if (!canor_demo_jlit(&j, '[')) { canor_demo_jfail(&j, "strategies must be an array"); ok = false; break; }
        if (!canor_demo_jlit(&j, ']')) {
          do { if (!cfg_one_strategy(&j, c)) { ok = false; break; } } while (canor_demo_jlit(&j, ','));
          if (ok && !canor_demo_jlit(&j, ']')) { canor_demo_jfail(&j, "expected ']'"); ok = false; }
        }
      } else if (!strcmp(key, "flows")) {
        c->flows.count = 0;
        if (!canor_demo_jlit(&j, '[')) { canor_demo_jfail(&j, "flows must be an array"); ok = false; break; }
        if (!canor_demo_jlit(&j, ']')) {
          do { if (!cfg_one_flow(&j, c)) { ok = false; break; } } while (canor_demo_jlit(&j, ','));
          if (ok && !canor_demo_jlit(&j, ']')) { canor_demo_jfail(&j, "expected ']'"); ok = false; }
        }
      } else if (!strcmp(key, "settings")) {
        if (!cfg_settings(&j, c)) { ok = false; break; }
      } else if (!strcmp(key, "metaflows")) {
        if (!cfg_metaflows(&j, c)) { ok = false; break; }
      } else if (!strcmp(key, "mixtures")) {
        if (!cfg_mixtures(&j, c)) { ok = false; break; }
      } else if (!strcmp(key, "letters")) {
        if (!cfg_letters(&j, c)) { ok = false; break; }
      } else if (!strcmp(key, "frame")) {
        if (!cfg_frame(&j, c)) { ok = false; break; }
      } else if (!canor_demo_jskip(&j)) { ok = false; break; }
      if (!ok) break;
    } while (canor_demo_jlit(&j, ','));
    if (ok && !canor_demo_jlit(&j, '}')) { canor_demo_jfail(&j, "expected '}' at top level"); ok = false; }
  }

  free(buf);
  if (!ok) {
    snprintf(err, errcap, "%s: %s", path, j.err[0] ? j.err : "parse error");
    return false;
  }
  if (c->nstrat == 0) { snprintf(err, errcap, "%s: no strategies", path); return false; }
  if (c->flows.count == 0) { snprintf(err, errcap, "%s: no flows", path); return false; }
  snprintf(c->path, sizeof c->path, "%s", path);
  c->from_file = true;
  return true;
}
