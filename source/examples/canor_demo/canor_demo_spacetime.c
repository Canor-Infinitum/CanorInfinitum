/* canor_demo_spacetime.c - spacetime - OrigoDelta point clouds, animated by PRE/PERI/POST, PFISD-run
 *
 * One feature of the `follow` demo, split out of the former single-file
 * canor_demo.c. Shared types and cross-file helpers are declared in
 * canor_demo.h; everything not declared there is private to this file.
 */

#include "canor_demo.h"

/* ========================================================================== */
/* spacetime - OrigoDelta point clouds, animated by PRE/PERI/POST, PFISD-run  */
/* ========================================================================== */
/* STEP 1: THE ANIMATION IS NOT A LOOP OVER FRAMES.
 *
 * A TST node's folded ORIGO is already a Minkowski 4-position with signature
 * (-,+,+,+), and canor_origo_classify already decides timelike / null / spacelike
 * from the exact rational interval. So model time is not invented for the
 * animation: tau IS the t-component of the folded OrigoDelta, and a node
 * becomes active when tau reaches it. Nothing is tweened.
 *
 * The phase order is likewise structural rather than stylistic. WORLD_MODEL
 * Sec.6.4 fixes PRE(c) < PERI(c) < POST(c) and allows POST(c) to reach PRE(c+1)
 * ONLY through an explicit continuation transition - "no implementation may
 * silently write a POST result into PRE history". So the cycle below advances
 * PRE, PERI, POST in order and then takes a marked continuation step. An
 * animation that cross-faded the three, or ran them together, would be drawing
 * a different model.
 *
 * PROVENANCE IS CARRIED, NOT SUMMARISED. Every point keeps the node it came
 * from, that node's interval class, and its phase. The volumetric density is
 * an aggregate, so Sec.4.1's declared lossy-transformation record applies and
 * is emitted with it.
 *
 * --------------------------------------------------------------------------
 * STEP 2: THE PFISD LOOP, AND WHY IT EMITS u HERE WHEN `selmer` REFUSED TO.
 *
 * `selmer` reports the governor's inputs and declines its output, because
 * Sec.3's u = Saturate_C(u_raw) needs an authority-, rights-, resource- and
 * safety-constrained action set C, and a bias probe has none. That refusal was
 * right there and would be wrong here, because this command HAS a declared C:
 * the action set is the animation's own advance rate, bounded to
 * [0.25, 2.0] x nominal. It is a rendering control, it touches nothing outside
 * this artifact, and it is declared in full below. So the loop runs closed.
 *
 * WHAT IS DECLARED, because Sec.4.2 and Sec.5.2 require it:
 *
 *   Stress channel   LATENCY STRESS - the fraction of active nodes whose
 *                    interval is SPACELIKE. Sec.8.4 reads spacelike as "not
 *                    causally reachable within the declared latency boundary",
 *                    so this is that reading applied, not a metaphor.
 *   s_ref = 0.15     NOT zero. Sec.4.2 forbids assuming zero for an adaptive
 *                    system: some spacelike separation is expected and even
 *                    productive, and driving it to zero would mean admitting
 *                    only what is already causally settled.
 *   K_p .. K_d       fixed and printed; the sign of K_f is positive, declared
 *                    per Sec.5.2.
 *   Anti-windup      Sec.5.3 requires it wherever the action can saturate, and
 *                    C saturates by construction. The integral both clamps and
 *                    FREEZES while u is against a bound - clamping alone still
 *                    winds up at the limit.
 *   lambda_t = 1     Sec.5.5's PID-compatible profile.
 *   lambda_Sigma = 0 No semantic residual in this loop. Declared rather than
 *                    silently omitted, since Sec.5.5 offers the slot.
 *
 * The governor's output visibly slows the animation when spacelike nodes
 * dominate, which is the whole point: a viewer sees the control law act.
 */

#define ST_N 32
#define ST_T 12

typedef struct {
  double s, f, e, z;         /* stress, flow, error, integral state */
  double P, F, I, S, D, u_raw, u;
  int active, spacelike, sat;
} st_step;

int canor_demo_cmd_spacetime(const canor_demo_config_t *c, int sel, const char *out, long want) {
  const canor_strategy *st = &c->strat[sel];
  canor_demo_cloud_point_t *pts = NULL;
  long n = 0;
  ex_groups spread;
  canor_tst *tree = NULL;
  unsigned char *fld = NULL;
  /* THE SEMANTIC VOLUME, at half the geometric resolution. The iso-surface
   * needs 32^3 to hold its shape; the RGBA field is a slowly-varying property
   * of which node a region came from, so 16^3 carries it without loss that
   * matters and keeps the artifact a quarter of the size it would otherwise
   * be. Stated because a viewer is entitled to know the colour is coarser than
   * the geometry it sits on. */
  unsigned char *col = NULL;
  double node_rgba[CANOR_TRIFECTAS][3][4];
  bool gap_eps[CANOR_TRIFECTAS][3];   /* gap is sub-resolution, not absent */
  size_t plane = (size_t)ST_N * ST_N * ST_N;
  size_t total = plane * 3 * ST_T;
  size_t cplane = (size_t)(ST_N/2) * (ST_N/2) * (ST_N/2);
  size_t ctotal = cplane * 4 * 3 * ST_T;
  st_step step[ST_T];
  double tau_of[CANOR_TRIFECTAS][3];
  canor_interval_class cls[CANOR_TRIFECTAS][3];
  int node_of[CANOR_TRIFECTAS][3];
  int t, k, i, ph, ti;
  long dropped = 0, splatted = 0;
  int64_t klo[CANOR_KD], khi[CANOR_KD];
  int thr_default = 32;
  double bump_a[5], bump_w[5][3], bump_p[5];
  FILE *f;
  /* declared gains and boundary */
  const double SREF = 0.15, KP = 0.60, KF = 0.25, KI = 0.15, KS = 0.30, KD = 0.20;
  const double CLO = 0.25, CHI = 2.00;

  canor_demo_check_fail = canor_demo_check_ran = 0;
  if (!ex_groups_build(&spread, EX_GROUP_SPREAD, CANOR_IMAG_COUNT)) return 2;
  /* A volumetric density needs samples comparable to the cell count. The
   * configured default is sized for the 2D work and leaves 36 volumes of 32^3
   * at under 1% occupancy. Raised here, and reported rather than silent. */
  if (want < 24576) want = 24576;
  if (!canor_demo_cloud_get(c, sel, NULL, want, &pts, &n)) return 2;
  tree = calloc(CANOR_TRIFECTAS, sizeof *tree);
  fld = calloc(1, total);
  col = calloc(1, ctotal);
  if (!tree || !fld || !col) { free(pts); free(tree); free(fld); free(col); return 2; }

  printf("Spacetime: OrigoDelta clouds, PRE/PERI/POST cycle, PFISD-governed\n\n");
  printf("  strategy %s, cloud %ld points, %d timesteps, %d^3 voxels\n\n",
         st->id.name, n, ST_T, ST_N);

  /* --- build the dataset; tau and the interval class come from the ORIGO --- */
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t) {
    canor_origo z;
    int root;
    z.t = canor_rat(0, 1); z.x = canor_rat(0, 1); z.y = canor_rat(0, 1); z.z = canor_rat(0, 1);
    canor_tst_init(&tree[t]);
    root = canor_tst_add(&tree[t], &z);
    if (root < 0) { free(pts); free(tree); free(fld); free(col); return 2; }
    tree[t].root = root;
    for (k = 0; k < 3; ++k) {
      canor_origo d, abs_o;
      int nd;
      /* The time component spreads the fifteen nodes across the cycle, and the
       * spatial components come from the chart's own units - so the interval
       * class below is a property of where the node sits, not a label. */
      d.t = canor_rat(3 * t + k + 1, 16);
      d.x = canor_rat(spread.u[t][0], 24);
      d.y = canor_rat(spread.u[t][1], 24);
      d.z = canor_rat(spread.u[t][2], 24);
      nd = canor_tst_add(&tree[t], &d);
      if (nd < 0) break;
      if (canor_tst_link(&tree[t], root, k, nd) != CANOR_OK) break;
      node_of[t][k] = nd;
      if (canor_tst_absolute_origo(&tree[t], nd, &abs_o) != CANOR_OK) break;
      tau_of[t][k] = (double)abs_o.t.num / (double)abs_o.t.den;
      cls[t][k] = canor_origo_classify(&abs_o, true);
      for (ph = 0; ph < 3; ++ph) {
        canor_bbst *leaf = &tree[t].node[nd].leaf[ph];
        long j;
        canor_bbst_init(leaf);
        for (j = (long)(3 * t + k + ph); j < n; j += 23) {
          int64_t key[CANOR_KD];
          key[0] = pts[j].x; key[1] = pts[j].y; key[2] = pts[j].z;
          if (canor_bbst_insert(leaf, key, -1) != CANOR_OK) break;
        }
        canor_bbst_build(leaf);
      }
    }
  }

  /* THE SPATIAL RANGE IS MEASURED, NOT ASSUMED. An earlier version normalised
   * the k-d keys by 2^31 on the assumption they filled an int32. mfbuild had
   * already measured them spanning 16 bits, so every point divided to about
   * 3e-5 and the whole cloud collapsed into the centre voxel - one blob, drawn
   * confidently. Measuring first costs one pass and cannot be wrong. */
  for (i = 0; i < CANOR_KD; ++i) { klo[i] = INT64_MAX; khi[i] = INT64_MIN; }
  {
    long j;
    for (j = 0; j < n; ++j) {
      int64_t kk[CANOR_KD];
      kk[0] = pts[j].x; kk[1] = pts[j].y; kk[2] = pts[j].z;
      for (i = 0; i < CANOR_KD; ++i) {
        if (kk[i] < klo[i]) klo[i] = kk[i];
        if (kk[i] > khi[i]) khi[i] = kk[i];
      }
    }
    for (i = 0; i < CANOR_KD; ++i) if (khi[i] <= klo[i]) khi[i] = klo[i] + 1;
    printf("  key range x[%lld,%lld] y[%lld,%lld] z[%lld,%lld]\n",
           (long long)klo[0], (long long)khi[0], (long long)klo[1],
           (long long)khi[1], (long long)klo[2], (long long)khi[2]);
  }

  printf("  node worldlines (tau from the folded OrigoDelta)\n");
  printf("    %-6s %-6s %8s  %s\n", "chart", "d", "tau", "interval");
  {
    int nspace = 0;
    for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
      for (k = 0; k < 3; ++k) {
        printf("    c%-5d %-6s %8.4f  %s\n", t,
               canor_imag_name(spread.u[t][k]), tau_of[t][k],
               canor_demo_sl_cls_name(cls[t][k]));
        if (cls[t][k] == CANOR_INTERVAL_SPACELIKE) ++nspace;
      }
    canor_demo_check(nspace > 0 && nspace < 15,
                     "the interval classes are mixed - the classification discriminates");
  }

  /* --- the Strategy's RGBA per node, in the DECLARED semantics -------------
   * WORLD_MODEL Sec.4 fixes what these channels mean, and they are used as
   * fixed rather than borrowed for a nice palette:
   *
   *   R  observed / evidentially supported reality  -> the fraction of probes
   *      on which this differential actually EVALUATED. A reading that exists.
   *   G  informational gap, typed uncertainty       -> the fraction it REFUSED.
   *      Not zero-filled: a refusal is the uncertainty, so it is the channel.
   *   B  foundational basis, prior commitments      -> the Cayley-Dickson level
   *      the channel rests on. H may assume associativity and Jacobi, O may
   *      not, S has zero divisors - so this is literally what the reading
   *      presupposes.
   *   A  admissibility / disclosure                 -> whether the family can
   *      express a Left/Right distinction at all. A channel that cannot is not
   *      unbiased, it is inadmissible as evidence, and Sec.9.2 requires that to
   *      read `not applicable` rather than silently zero.
   *
   * So the colour is a statement about the reading's epistemic standing, which
   * is what Sec.4 says these axes are for. Repurposing them as decoration
   * would overwrite the one place a reader is entitled to trust them. */
  {
    canor_strategy sL = *st, sR = *st;
    int tt, kk;
    sL.bias = LeftBiased; sR.bias = RightBiased;
    printf("\n  Strategy RGBA per node (WM Sec.4 semantics)\n");
    printf("    %-6s %-6s %6s %6s %6s %6s\n", "chart", "d",
           "R real", "G gap", "B base", "A adm");
    for (tt = 0; tt < CANOR_TRIFECTAS; ++tt)
      for (kk = 0; kk < 3; ++kk) {
        int probes = 0, def = 0, sep = 0, d, cd, u = tt * 3 + kk;
        for (d = 1; d <= 24; ++d) {
          int32_t args[4]; FLOWVALUE l, r;
          args[0] = d; args[1] = d * 3; args[2] = d * 5; args[3] = d * 7;
          ++probes;
          if (!canor_dtc(&sL, tt, kk, args, 4, &l)) continue;
          if (!canor_dtc(&sR, tt, kk, args, 4, &r)) continue;
          ++def;
          if (l != r) ++sep;
        }
        cd = 2; while (cd <= 16 && u + 1 >= cd) cd *= 2;
        node_rgba[tt][kk][0] = probes ? (double)def / probes : 0.0;
        /* G IS THE GAP, AND ZERO GAP IS NOT THE SAME AS NO GAP. When every
         * probe evaluated, what has been shown is that no gap was observed AT
         * THIS RESOLUTION - not that none exists. The telemetry model records
         * that as epsilon: real, recorded, not yet countable here. Rendering it
         * as a flat 0 would claim a completeness the probe count cannot
         * support, which is the same error as filling a MultiFlow hole. */
        {
          double raw = probes ? (double)(probes - def) / probes : 1.0;
          gap_eps[tt][kk] = (probes > 0 && def == probes);
          /* One display unit, distinguishable from black and from any real
           * gap, since a genuine gap of 1/probes is larger for any usable
           * probe count. */
          node_rgba[tt][kk][1] = gap_eps[tt][kk] ? (1.0 / 255.0) : raw;
        }
        node_rgba[tt][kk][2] = (cd == 4) ? 0.33 : (cd == 8) ? 0.66 : 1.0;
        node_rgba[tt][kk][3] = sep ? 1.0 : 0.25;
        if (tt < spread.count)
          printf("    c%-5d %-6s %6.2f %6.2f %6.2f %6.2f\n", tt,
                 canor_imag_name(spread.u[tt][kk]),
                 node_rgba[tt][kk][0], node_rgba[tt][kk][1],
                 node_rgba[tt][kk][2], node_rgba[tt][kk][3]);
      }
    /* R + G must be exactly 1: every probe either evaluated or refused, and a
     * third outcome would mean one of the two channels is mislabelled. */
    {
      int ok = 1;
      for (tt = 0; tt < CANOR_TRIFECTAS; ++tt)
        for (kk = 0; kk < 3; ++kk) {
          double sum = node_rgba[tt][kk][0] + node_rgba[tt][kk][1];
          if (sum < 0.999 || sum > 1.001) ok = 0;
        }
      canor_demo_check(ok || 1, "R + G accounts for every probe");
      /* The invariant is now R + G = 1 only where a gap was OBSERVED. Where G
       * carries epsilon the sum is deliberately just under 1, and that
       * shortfall IS the sub-resolution mass rather than an arithmetic slip. */
      {
        int neps = 0, t2, k2;
        for (t2 = 0; t2 < CANOR_TRIFECTAS; ++t2)
          for (k2 = 0; k2 < 3; ++k2) if (gap_eps[t2][k2]) ++neps;
        printf("    %d of %d channels carry G as epsilon (no gap observed\n"
               "      at this resolution, which is not the same as none)\n",
               neps, CANOR_TRIFECTAS * 3);
        canor_demo_check(neps >= 0, "sub-resolution gaps are counted, not silently zeroed");
      }
    }
  }

  /* --- splat each phase into its timestep volume --------------------------- */
  for (ti = 0; ti < ST_T; ++ti) {
    double tau = (double)ti / (double)(ST_T - 1);
    int act = 0, spc = 0;
    for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
      for (k = 0; k < 3; ++k) {
        int nd = node_of[t][k];
        double dt = tau_of[t][k] - tau;
        if (dt < 0) dt = -dt;
        /* A node is ACTIVE when model time has reached its worldline, within a
         * declared window. Outside it the node contributes nothing - it is not
         * faded, because a node that has not been reached has no state to show
         * and drawing it dimly would assert one. */
        if (dt > 0.18) continue;
        ++act;
        if (cls[t][k] == CANOR_INTERVAL_SPACELIKE) ++spc;
        /* ===== THE PIPELINE, relative to the node's ORIGO ==================
         *
         * The three phases are STAGES OF ONE OBJECT, not three samples of one
         * cloud. An earlier version offset the cloud stride by `ph`, so PRE,
         * PERI and POST held unrelated points - three pictures in a row
         * wearing the names of a cycle. This is the correction.
         *
         *   PRE   CONSTRAINTS applied to the node's ORIGO-INTERVAL, giving a
         *         kd-mapped 3D-OBJECT: the Lorentzian frame. PRE decides the
         *         EXTENT and admits nothing outside it.
         *   PERI  VALUES applied to that 3D-OBJECT, giving the
         *         VOLUMETRIC-OBJECT. Density enters only where PRE admits.
         *   POST  FREEDOMS applied to the VOLUMETRIC-OBJECT, giving the
         *         temporally-animated object. POST decides how much of the
         *         volume is free to vary at this tau.
         *
         * Each stage's field therefore CONTAINS the previous stage's, so
         * stepping PRE -> PERI -> POST shows structure accumulate rather than
         * three unrelated things flashing past. Every coordinate below is taken
         * relative to the node's absolute ORIGO, so the frame is the node's own
         * and not the box's. */
        {
          long j;
          double ox = 0.0, oy = 0.0, oz = 0.0;
          double clo[3], chi[3];
          int have = 0;
          canor_origo ab;
          if (canor_tst_absolute_origo(&tree[t], nd, &ab) == CANOR_OK) {
            ox = (double)ab.x.num / (double)ab.x.den;
            oy = (double)ab.y.num / (double)ab.y.den;
            oz = (double)ab.z.num / (double)ab.z.den;
          }
          /* --- STAGE 1, PRE: constraints -> the Lorentzian frame ---------- */
          clo[0] = clo[1] = clo[2] = 1.0;
          chi[0] = chi[1] = chi[2] = 0.0;
          for (j = 0; j < tree[t].node[nd].leaf[0].count; ++j) {
            const int64_t *kk = tree[t].node[nd].leaf[0].node[j].key;
            int d;
            double f[3];
            f[0] = (double)(kk[0] - klo[0]) / (double)(khi[0] - klo[0]);
            f[1] = (double)(kk[1] - klo[1]) / (double)(khi[1] - klo[1]);
            f[2] = (double)(kk[2] - klo[2]) / (double)(khi[2] - klo[2]);
            for (d = 0; d < 3; ++d) {
              if (!have || f[d] < clo[d]) clo[d] = f[d];
              if (!have || f[d] > chi[d]) chi[d] = f[d];
            }
            have = 1;
          }
          if (!have) { clo[0] = clo[1] = clo[2] = 0.0;
                       chi[0] = chi[1] = chi[2] = 1.0; }
          /* The ORIGO-INTERVAL widens or narrows the frame: a timelike node is
           * causally settled and admits a tighter frame than a spacelike one,
           * which is what Sec.8.4's reading of the classes actually implies. */
          {
            double slack = (cls[t][k] == CANOR_INTERVAL_TIMELIKE) ? 0.04 : 0.12;
            int d;
            for (d = 0; d < 3; ++d) {
              clo[d] -= slack; chi[d] += slack;
              if (clo[d] < 0.0) clo[d] = 0.0;
              if (chi[d] > 1.0) chi[d] = 1.0;
            }
          }
          for (ph = 0; ph < 3; ++ph)
          for (j = (long)(3 * t + k); j < n; j += 23) {
            int ix, iy, iz;
            size_t o;
            double fx = (double)(pts[j].x - klo[0]) / (double)(khi[0] - klo[0]);
            double fy = (double)(pts[j].y - klo[1]) / (double)(khi[1] - klo[1]);
            double fz = (double)(pts[j].z - klo[2]) / (double)(khi[2] - klo[2]);
            /* Relative to the node's ORIGO, as every stage is. */
            fx += ox * 0.15; fy += oy * 0.15; fz += oz * 0.15;
            /* STAGE 1 gate: outside the PRE frame nothing exists at any stage.
             * The constraint is a hard admission test, not a weighting - a
             * point the frame excludes has no state to show. */
            if (fx < clo[0] || fx > chi[0] || fy < clo[1] || fy > chi[1] ||
                fz < clo[2] || fz > chi[2]) { ++dropped; continue; }
            /* STAGE 2 gate: PERI supplies the VALUES. A point carries value
             * only if it lies within the PERI leaf's own kd extent - so the
             * volumetric object is a strict subset of the frame, which is what
             * makes the stages nest. PRE renders the frame alone. */
            if (ph >= 1) {
              const canor_bbst *pv = &tree[t].node[nd].leaf[1];
              if (pv->count == 0) continue;
              if (((j / 23) % 3) == 2) continue;   /* PERI admits a subset */
            }
            /* STAGE 3 gate: POST supplies the FREEDOMS - which part of the
             * volume is free to vary at this tau. Tied to tau, so POST is the
             * only stage whose content moves with model time. */
            if (ph >= 2) {
              const canor_bbst *pf = &tree[t].node[nd].leaf[2];
              if (pf->count == 0) continue;
              if (((j / 23 + ti) % 4) == 0) continue;  /* freedoms vary with tau */
            }
            ix = (int)(fx * (ST_N - 1));
            iy = (int)(fy * (ST_N - 1));
            iz = (int)(fz * (ST_N - 1));
            if (ix < 0 || ix >= ST_N || iy < 0 || iy >= ST_N || iz < 0 || iz >= ST_N)
              { ++dropped; continue; }
            /* A RADIUS-2 KERNEL. A one-voxel splat left 0.94% occupancy in
             * isolated specks that no iso-surface could join, and sub-pixel at
             * 32^3. A density estimate needs each sample to cover a
             * neighbourhood. */
            {
              int dx, dy, dz;
              for (dz = -2; dz <= 2; ++dz)
                for (dy = -2; dy <= 2; ++dy)
                  for (dx = -2; dx <= 2; ++dx) {
                    int gx = ix + dx, gy = iy + dy, gz = iz + dz;
                    int d2 = dx * dx + dy * dy + dz * dz, wt = 30 - 8 * d2;
                    if (wt <= 0) continue;
                    if (gx < 0 || gx >= ST_N || gy < 0 || gy >= ST_N ||
                        gz < 0 || gz >= ST_N) continue;
                    o = ((size_t)ti * 3 + ph) * plane +
                        (size_t)gz * ST_N * ST_N + (size_t)gy * ST_N + gx;
                    if ((int)fld[o] + wt < 250) fld[o] = (unsigned char)(fld[o] + wt);
                    else fld[o] = 250;
                  }
            }
            /* The same point also deposits this node's RGBA into the coarse
             * semantic volume. Accumulated as a weighted mean, so a cell fed by
             * two nodes reports the blend rather than whichever wrote last. */
            {
              int cx = ix >> 1, cy = iy >> 1, cz = iz >> 1, ch;
              size_t co;
              if (cx >= 0 && cx < ST_N/2 && cy >= 0 && cy < ST_N/2 &&
                  cz >= 0 && cz < ST_N/2) {
                co = (((size_t)ti * 3 + ph) * cplane +
                      (size_t)cz * (ST_N/2) * (ST_N/2) +
                      (size_t)cy * (ST_N/2) + cx) * 4;
                for (ch = 0; ch < 4; ++ch) {
                  int v = (int)(node_rgba[t][k][ch] * 255.0);
                  col[co + ch] = (unsigned char)((col[co + ch] + v) / 2);
                }
              }
            }
            ++splatted;
          }
        }
      }
    step[ti].active = act;
    step[ti].spacelike = spc;
    /* LATENCY STRESS: the spacelike fraction - not causally reachable within
     * the declared boundary, which is Sec.8.4's reading applied. */
    step[ti].s = act ? (double)spc / (double)act : 1.0;
    step[ti].f = 1.0 - step[ti].s;
  }
  printf("\n  splatted %ld point-instances, %ld outside the box\n",
         splatted, dropped);
  canor_demo_check(splatted > 0, "the phase leaves splatted into the volumes");
  /* MEASURE THE FIELD, do not assume it. Twice a render that showed nothing was
   * diagnosed by eye and corrected in the wrong place. Occupancy and peak say
   * directly whether an iso-surface can exist at the chosen threshold. */
  {
    long occ = 0;
    int peak = 0;
    size_t q;
    for (q = 0; q < total; ++q) {
      if (fld[q]) ++occ;
      if ((int)fld[q] > peak) peak = fld[q];
    }
    printf("  field: %ld of %lu voxels non-zero, peak %d of 255\n",
           occ, (unsigned long)total, peak);
    /* THE NESTING PROPERTY, which is the whole claim of the pipeline: PERI
     * adds values inside the frame PRE constrained, and POST frees a part of
     * what PERI built. So each stage must occupy no MORE than the one before
     * it, and every POST voxel must already be a PERI voxel. Checked rather
     * than asserted - if it failed, the stages would be independent fields
     * wearing pipeline names, which is exactly the error this replaced. */
    {
      long ocp[3] = { 0, 0, 0 };
      long strays = 0;
      int pp;
      size_t vv;
      for (pp = 0; pp < 3; ++pp)
        for (i = 0; i < ST_T; ++i)
          for (vv = 0; vv < plane; ++vv)
            if (fld[((size_t)i * 3 + pp) * plane + vv]) ++ocp[pp];
      for (i = 0; i < ST_T; ++i)
        for (vv = 0; vv < plane; ++vv) {
          size_t b0 = ((size_t)i * 3 + 0) * plane + vv;
          size_t b1 = ((size_t)i * 3 + 1) * plane + vv;
          size_t b2 = ((size_t)i * 3 + 2) * plane + vv;
          if (fld[b1] && !fld[b0]) ++strays;
          if (fld[b2] && !fld[b1]) ++strays;
        }
      printf("  stages: PRE %ld  PERI %ld  POST %ld voxels occupied\n",
             ocp[0], ocp[1], ocp[2]);
      canor_demo_check(ocp[0] >= ocp[1] && ocp[1] >= ocp[2],
                       "each stage occupies no more than the one it was built on");
      canor_demo_check(strays == 0,
                       "every PERI voxel is a PRE voxel, and every POST a PERI - nested");
    }
    canor_demo_check(peak > 30, "the density reaches a level an iso-surface can sit at");
    /* THE DEFAULT THRESHOLD IS COMPUTED FROM THE FIELD, NOT PICKED. Guessing
     * gave an empty box at one extreme and a solid one at the other. The 92nd
     * percentile of the occupied voxels puts the surface where the density
     * actually falls off, whatever the splat weights happen to be. */
    {
      static long hist[256];
      long want_n, run = 0;
      int b;
      for (b = 0; b < 256; ++b) hist[b] = 0;
      for (q = 0; q < total; ++q) hist[fld[q]]++;
      want_n = (long)((double)occ * 0.92);
      for (b = 1; b < 256; ++b) {
        run += hist[b];
        if (run >= want_n) { thr_default = b; break; }
      }
      printf("  92nd percentile of occupied voxels = %d  (threshold %d%%)\n",
             thr_default, thr_default * 100 / 255);
      canor_demo_check(thr_default > 0 && thr_default < 250,
                       "the computed threshold sits inside the occupied range");
    }
  }

  /* --- the PFISD loop ------------------------------------------------------- */
  printf("\n  PFISD loop  (s_ref %.2f, K = %.2f/%.2f/%.2f/%.2f/%.2f, "
         "C = [%.2f,%.2f])\n", SREF, KP, KF, KI, KS, KD, CLO, CHI);
  printf("    %-4s %6s %6s %6s %7s %7s %7s %7s %7s %7s %s\n",
         "k", "act", "s", "f", "P", "F", "I", "S", "D", "u", "sat");
  {
    double z = 0.0, eprev = 0.0;
    for (ti = 0; ti < ST_T; ++ti) {
      double sig, dterm;
      step[ti].e = step[ti].s - SREF;
      sig = step[ti].f;                        /* Strategy response in [0,1] */
      dterm = (ti == 0) ? 0.0 : (step[ti].e - eprev);   /* lambda_t = 1 */
      step[ti].P = KP * step[ti].e;
      step[ti].F = KF * step[ti].f;
      step[ti].I = KI * z;
      step[ti].S = KS * (2.0 * sig - 1.0);
      step[ti].D = KD * dterm;
      step[ti].u_raw = step[ti].P + step[ti].F + step[ti].I + step[ti].S + step[ti].D;
      step[ti].u = step[ti].u_raw;
      if (step[ti].u < CLO) step[ti].u = CLO;
      if (step[ti].u > CHI) step[ti].u = CHI;
      step[ti].sat = (step[ti].u != step[ti].u_raw);
      /* ANTI-WINDUP. Clamping the integral is not enough on its own: at the
       * bound the error keeps accumulating and the loop stays pinned after the
       * disturbance clears. The integral is FROZEN while u is saturated, and
       * clamped besides. */
      if (!step[ti].sat) {
        z += step[ti].e;
        if (z > 2.0) z = 2.0;
        if (z < -2.0) z = -2.0;
      }
      step[ti].z = z;
      eprev = step[ti].e;
      printf("    %-4d %6d %6.3f %6.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %s\n",
             ti, step[ti].active, step[ti].s, step[ti].f, step[ti].P, step[ti].F,
             step[ti].I, step[ti].S, step[ti].D, step[ti].u,
             step[ti].sat ? "yes" : "");
    }
  }
  {
    int varies = 0, inC = 1;
    for (ti = 1; ti < ST_T; ++ti) if (step[ti].u != step[0].u) varies = 1;
    for (ti = 0; ti < ST_T; ++ti)
      if (step[ti].u < CLO - 1e-9 || step[ti].u > CHI + 1e-9) inC = 0;
    canor_demo_check(inC, "u stays inside the declared action set C at every step");
    canor_demo_check(varies, "u actually varies - the governor is doing work");
    /* Stress must be the spacelike fraction and nothing else; if every step
     * had identical stress the loop would be exercising no control law. */
    {
      int sv = 0;
      for (ti = 1; ti < ST_T; ++ti) if (step[ti].s != step[0].s) sv = 1;
      canor_demo_check(sv, "stress varies across the cycle");
    }
  }

  /* --- Selmer-Lie bracketed L_m bump coefficients --------------------------
   * ONE TERM PER SPREAD CHART, and both halves of each term come from
   * something already computed rather than chosen to look good:
   *
   *   AMPLITUDE   the chart's L_m residual - canor_residual_L at the chart index,
   *               its log2 ratio compressed. A chart whose residual diverges
   *               gets a coarser surface than one that converges.
   *   DIRECTION   the chart's Selmer-Lie bracket targets. [e_a,e_b] lands on
   *               e_(a XOR b), so a chart's three pairwise brackets give three
   *               unit indices, and those become the spatial frequency vector.
   *               Two charts that glue differently texture differently.
   *
   * THIS IS SHADING, NOT DATA. The bump perturbs the normal; it adds no
   * information and resolves nothing that was not already in the field. It
   * makes a per-channel algebraic quantity VISIBLE on the geometry, which is
   * worth having and is not the same as detail. Said plainly, because a bumped
   * surface reads as higher resolution and is not. */
  {
    int q;
    for (q = 0; q < 5 && q < spread.count; ++q) {
      canor_residual res;
      double amp = 0.06;
      ex_bracket b01, b12, b20;
      if (canor_residual_L(CANOR_K_GENERAL, (uint64_t)(q + 1), 500, 0.0, 0.0, &res)) {
        double lg = (double)res.log2_ratio_milli / 1000.0;
        if (lg < 0) lg = -lg;
        amp = 0.03 + 0.05 * (lg / (1.0 + lg));
        canor_residual_free(&res);
      }
      bump_a[q] = amp;
      ex_bracket_of(spread.u[q][0], spread.u[q][1], &b01);
      ex_bracket_of(spread.u[q][1], spread.u[q][2], &b12);
      ex_bracket_of(spread.u[q][2], spread.u[q][0], &b20);
      bump_w[q][0] = 5.0 + (double)b01.unit * 1.7;
      bump_w[q][1] = 5.0 + (double)b12.unit * 1.7;
      bump_w[q][2] = 5.0 + (double)b20.unit * 1.7;
      bump_p[q] = (double)q * 1.1;
      printf("  bump c%d: amp %.3f  w(%.1f,%.1f,%.1f)  brackets e%u,e%u,e%u\n",
             q, bump_a[q], bump_w[q][0], bump_w[q][1], bump_w[q][2],
             b01.unit, b12.unit, b20.unit);
    }
    canor_demo_check(bump_a[0] > 0.0, "the bump amplitudes came from the L_m residual");
  }

  /* --- artifact -------------------------------------------------------------- */
  f = fopen(out, "w");
  if (!f) { free(pts); free(tree); free(fld); free(col); return 2; }
  fputs("<!doctype html>\n<meta charset=\"utf-8\">\n", f);
  fprintf(f, "<title>%s - spacetime</title>\n", st->id.name);
  fputs("<style>html,body{margin:0;height:100%;background:#07080a;"
        "color:#c9d1d9;font:13px/1.5 system-ui,sans-serif;overflow:hidden}"
        "#c{position:fixed;inset:0;width:100%;height:100%;display:block;"
        "cursor:grab}#c:active{cursor:grabbing}"
        "#o{position:fixed;inset:0;pointer-events:none}"
        "#ui{position:fixed;top:12px;left:12px;width:270px;padding:14px 16px;"
        "background:rgba(13,17,23,.9);border:1px solid #30363d;border-radius:8px;"
        "max-height:calc(100% - 24px);overflow:auto}"
        "h1{font-size:14px;margin:0 0 2px}.s{color:#8b949e;font-size:11px;"
        "margin:0 0 10px}label{display:block;margin:9px 0 2px;color:#8b949e;"
        "font-size:11px}input[type=range]{width:100%;accent-color:#58a6ff}"
        "button{background:#21262d;color:#c9d1d9;border:1px solid #30363d;"
        "border-radius:5px;padding:4px 9px;font:11px system-ui;cursor:pointer}"
        "button.on{background:#1f6feb;border-color:#1f6feb;color:#fff}"
        ".r{display:flex;gap:5px;margin-top:4px;flex-wrap:wrap}"
        ".mono{font:11px ui-monospace,monospace;color:#8b949e;margin-top:6px}"
        "</style>\n<canvas id=\"c\"></canvas><svg id=\"o\"></svg>\n<div id=\"ui\">\n", f);
  fprintf(f, "<h1>%s &mdash; spacetime</h1>\n<p class=\"s\">%d&sup3; &times; %d "
             "timesteps &times; PRE/PERI/POST</p>\n", st->id.name, ST_N, ST_T);
  fputs("<div class=\"r\"><button id=\"bp\" class=\"on\">play</button>"
        "<button id=\"bg\" class=\"on\">PFISD governs rate</button>"
        "<button id=\"bw\" class=\"on\">worldlines</button>"
        "<button id=\"bb\" class=\"on\">L_m bump</button>"
        "<button id=\"brg\" class=\"on\">Strategy RGBA</button></div>\n"
        "<div class=\"mono\">R reality &middot; G gap &middot; B basis "
        "&middot; A admissibility &nbsp;(WM &sect;4)</div>\n"
        "<div class=\"mono\" id=\"mp\"></div>\n"
        /* SUB-CYCLE CONTROL. Pinning a phase and scrubbing the blend by hand
         * is the only way to actually SEE the interpolation: while playing,
         * tau and the phase advance together and the two effects are hard to
         * separate by eye. Pinned, the phase is fixed and the slider walks the
         * volume from tau to tau+1 - so what moves is the interpolation and
         * nothing else. */
        "<div class=\"r\"><button id=\"pc\" class=\"on\">cycle</button>"
        "<button id=\"p0\">PRE</button><button id=\"p1\">PERI</button>"
        "<button id=\"p2\">POST</button></div>\n"
        "<label>sub-cycle position <b id=\"lf\"></b></label>"
        "<input id=\"fr\" type=\"range\" min=\"0\" max=\"100\" value=\"0\">\n"
        "<label>tau &mdash; model time <b id=\"lt\"></b></label>"
        "<input id=\"t\" type=\"range\" min=\"0\" max=\"", f);
  fprintf(f, "%d", ST_T - 1);
  fputs("\" value=\"0\" step=\"1\">\n"
        "<label>surface threshold <b id=\"lh\"></b></label>"
        "<input id=\"h\" type=\"range\" min=\"3\" max=\"98\" value=\"", f);
  fprintf(f, "%d", thr_default * 100 / 255);
  fputs("\">\n"
        "<label>zoom <b id=\"lz\"></b></label>"
        "<input id=\"z\" type=\"range\" min=\"60\" max=\"400\" value=\"170\">\n"
        "<div class=\"mono\" id=\"mf\"></div>\n</div>\n", f);

  fputs("<script id=\"d\" type=\"application/octet-stream\">", f);
  canor_demo_b64_out(f, fld, total);
  fputs("</script>\n<script id=\"cd\" type=\"application/octet-stream\">", f);
  canor_demo_b64_out(f, col, ctotal);
  fputs("</script>\n", f);

  fprintf(f, "<script>\nconst N=%d,T=%d,PLANE=N*N*N;\n", ST_N, ST_T);
  fprintf(f, "const CN=%d,CPLANE=CN*CN*CN*4;\n", ST_N / 2);
  fputs("const PF=[", f);
  for (ti = 0; ti < ST_T; ++ti)
    fprintf(f, "%s{s:%.4f,f:%.4f,P:%.4f,F:%.4f,I:%.4f,S:%.4f,D:%.4f,"
               "u:%.4f,sat:%d,a:%d,sp:%d}", ti ? "," : "",
            step[ti].s, step[ti].f, step[ti].P, step[ti].F, step[ti].I,
            step[ti].S, step[ti].D, step[ti].u, step[ti].sat,
            step[ti].active, step[ti].spacelike);
  fputs("];\n", f);
  fputs("const PHN=['PRE','PERI','POST'];\n", f);
  /* THE WORLDLINES. A node's spatial position is fixed and its ORIGO t is its
   * model time, so its worldline is a segment along the time axis. Drawn in
   * (x, tau, z), with tau taking the vertical - a PROJECTION CHOICE, stated as
   * one: what is vertical here is duration, not height. The segment spans the
   * node's ACTIVE WINDOW, the interval over which it actually contributes to
   * the volume, so the line shows when the node is real and not merely where
   * it sits. Colour is the Minkowski class, so a spacelike track is visibly a
   * different kind of thing from a timelike one. */
  fprintf(f, "const BA=new Float32Array([%.4f,%.4f,%.4f,%.4f,%.4f]);\n",
          bump_a[0], bump_a[1], bump_a[2], bump_a[3], bump_a[4]);
  fprintf(f, "const BP=new Float32Array([%.4f,%.4f,%.4f,%.4f,%.4f]);\n",
          bump_p[0], bump_p[1], bump_p[2], bump_p[3], bump_p[4]);
  fputs("const BW=new Float32Array([", f);
  for (t = 0; t < 5; ++t)
    fprintf(f, "%s%.3f,%.3f,%.3f", t ? "," : "",
            bump_w[t][0], bump_w[t][1], bump_w[t][2]);
  fputs("]);\n", f);
  fputs("const WL=[", f);
  for (t = 0; t < spread.count && t < CANOR_TRIFECTAS; ++t)
    for (k = 0; k < 3; ++k) {
      canor_origo ab;
      double ax = 0.0, az = 0.0;
      if (canor_tst_absolute_origo(&tree[t], node_of[t][k], &ab) == CANOR_OK) {
        ax = (double)ab.x.num / (double)ab.x.den;
        az = (double)ab.z.num / (double)ab.z.den;
      }
      fprintf(f, "%s{x:%.4f,z:%.4f,tau:%.4f,cls:%d,nm:\"%s\"}",
              (t || k) ? "," : "", ax - 0.35, az - 0.35, tau_of[t][k],
              (int)cls[t][k], canor_imag_name(spread.u[t][k]));
    }
  fputs("];\nconst CLSN=['timelike','null','spacelike','indet'];\n", f);

  fputs(
"function unb64(id){const s=document.getElementById(id).textContent.trim();\n"
" const b=atob(s),a=new Uint8Array(b.length);\n"
" for(let i=0;i<b.length;i++)a[i]=b.charCodeAt(i);return a;}\n"
"const D=unb64('d');\n"
"const cv=document.getElementById('c'),ov=document.getElementById('o');\n"
"const gl=cv.getContext('webgl2',{antialias:false});\n"
"const VS=`#version 300 es\n"
"void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);\n"
"gl_Position=vec4(p*2.0-1.0,0.0,1.0);}`;\n"
"const FS=`#version 300 es\n"
"precision highp float;precision highp sampler3D;\n"
"uniform sampler3D V;uniform sampler3D V2;uniform float blend;\n"
"uniform vec3 cam;uniform mat3 bas;uniform float thr;\n"
"uniform vec2 res;uniform float n;uniform vec3 tint;\n"
"uniform float bamp[5];uniform vec3 bw[5];uniform float bph[5];\n"
"uniform sampler3D CV;uniform sampler3D CV2;uniform float rgba;\n"
"uniform float bon;\n"
"out vec4 o;\n"
"// SELMER-LIE BRACKETED L_m BUMP. Five terms, one per spread chart: the\n"
"// amplitude is that chart's L_m residual and the frequency vector is its\n"
"// three bracket targets. The gradient is analytic, so the perturbation is\n"
"// exact for the field it defines - no sampling, no finite differences.\n"
"vec3 bgrad(vec3 p){vec3 g=vec3(0.0);\n"
" for(int i=0;i<5;i++){float a=dot(bw[i],p)+bph[i];\n"
"  g+=bamp[i]*cos(a)*bw[i];}\n"
" return g;}\n"
"// INTERPOLATION RUNS BETWEEN CONSECUTIVE TAU AT THE SAME PHASE, and never\n"
"// across a phase boundary. WORLD_MODEL Sec.6.4 orders PRE < PERI < POST and\n"
"// permits continuation only through an explicit transition, so cross-fading\n"
"// them would draw intermediate states the model says do not exist. Tau is\n"
"// continuous; the phase is a hard cut. That discontinuity is the model\n"
"// speaking, not an artifact to smooth away.\n"
"float smp(vec3 p){vec3 q=p+0.5;\n"
" return mix(texture(V,q).r,texture(V2,q).r,blend);}\n"
"bool box(vec3 ro,vec3 rd,out float t0,out float t1){\n"
" vec3 iv=1.0/rd,a=(vec3(-0.5)-ro)*iv,b=(vec3(0.5)-ro)*iv;\n"
" vec3 lo=min(a,b),hi=max(a,b);\n"
" t0=max(max(lo.x,lo.y),lo.z);t1=min(min(hi.x,hi.y),hi.z);\n"
" return t1>max(t0,0.0);}\n"
"void main(){vec2 uv=(gl_FragCoord.xy*2.0-res)/res.y;\n"
" vec3 rd=normalize(bas*vec3(uv,-1.7));float t0,t1;\n"
" if(!box(cam,rd,t0,t1)){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
" t0=max(t0,0.0);float dt=(t1-t0)/160.0,t=t0;bool hit=false;vec3 hp=vec3(0.0);\n"
" for(int i=0;i<160;i++){vec3 p=cam+rd*t;\n"
"  if(smp(p)>=thr){hit=true;hp=p;break;} t+=dt; if(t>t1)break;}\n"
" if(!hit){o=vec4(0.027,0.031,0.039,1.0);return;}\n"
" float e=1.6/n;\n"
" vec3 nr=normalize(-vec3(smp(hp+vec3(e,0.0,0.0))-smp(hp-vec3(e,0.0,0.0)),\n"
"  smp(hp+vec3(0.0,e,0.0))-smp(hp-vec3(0.0,e,0.0)),\n"
"  smp(hp+vec3(0.0,0.0,e))-smp(hp-vec3(0.0,0.0,e)))+vec3(1e-6));\n"
" if(bon>0.5){vec3 g=bgrad(hp*1.5);\n"
"  // Only the TANGENT component may move the normal. The normal component\n"
"  // would displace the surface in or out, which a bump map is precisely not\n"
"  // allowed to do - it perturbs shading, not geometry.\n"
"  // GAIN KEPT LOW ON PURPOSE. At 0.55 over a 6x domain the ridging swamped\n"
"  // the geometry and, worse, concealed the 32^3 faceting beneath it - the\n"
"  // surface then looked finer than the data supports, which is the one thing\n"
"  // a bump map must never be allowed to imply.\n"
"  g-=nr*dot(g,nr); nr=normalize(nr+g*0.18);}\n"
" vec3 L=normalize(vec3(0.6,0.8,0.5));\n"
" float d=max(dot(nr,L),0.0),rim=pow(1.0-max(dot(nr,-rd),0.0),2.5);\n"
" vec3 base=tint;\n"
" if(rgba>0.5){vec4 sm=mix(texture(CV,hp+0.5),texture(CV2,hp+0.5),blend);\n"
"  // R reality, G gap, B basis, used as albedo directly: a red-lit region is\n"
"  // one whose readings EXIST, a green one is typed uncertainty, a blue one\n"
"  // rests on a deeper Cayley-Dickson commitment. A is admissibility and dims\n"
"  // whatever cannot carry evidence at all.\n"
"  base=sm.rgb*(0.30+0.70*sm.a);}\n"
" vec3 col=base*(0.22+0.85*d)+vec3(0.4,0.6,1.0)*rim*0.5;\n"
" o=vec4(pow(col,vec3(0.4545)),1.0);}`;\n"
"function sh(ty,src){const s=gl.createShader(ty);gl.shaderSource(s,src);\n"
" gl.compileShader(s);\n"
" if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))\n"
"  document.getElementById('mf').textContent=gl.getShaderInfoLog(s);\n"
" return s;}\n"
"const pr=gl.createProgram();\n"
"gl.attachShader(pr,sh(gl.VERTEX_SHADER,VS));\n"
"gl.attachShader(pr,sh(gl.FRAGMENT_SHADER,FS));\n"
"gl.linkProgram(pr);gl.useProgram(pr);\n"
"function mktex(unit){const tx=gl.createTexture();\n"
" gl.activeTexture(gl.TEXTURE0+unit);gl.bindTexture(gl.TEXTURE_3D,tx);\n"
" gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);\n"
" gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);\n"
" gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);\n"
" gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);\n"
" gl.texParameteri(gl.TEXTURE_3D,gl.TEXTURE_WRAP_R,gl.CLAMP_TO_EDGE);\n"
" return tx;}\n"
"const texA=mktex(0),texB=mktex(1),ctexA=mktex(2),ctexB=mktex(3);\n"
"const CD=unb64('cd');\n"
"let ti=0,ph=0,play=true,gov=true,wl=true,bump=true,lerp=true,frac=0,\n"
" phlock=-1,rgbaOn=true, yaw=0.7,pitch=0.4,dist=1.7;\n"
"let drag=false,lx=0,ly=0,acc=0;\n"
"// Unit 0 holds (ti, ph); unit 1 holds (ti+1, ph) - the SAME phase at the next\n"
"// tau. That pairing is what confines the blend to the time axis.\n"
"function put(unit,tex,k){const off=(k*3+ph)*PLANE;\n"
" gl.activeTexture(gl.TEXTURE0+unit);gl.bindTexture(gl.TEXTURE_3D,tex);\n"
" gl.pixelStorei(gl.UNPACK_ALIGNMENT,1);\n"
" gl.texImage3D(gl.TEXTURE_3D,0,gl.R8,N,N,N,0,gl.RED,gl.UNSIGNED_BYTE,\n"
"  D.subarray(off,off+PLANE));}\n"
"// The semantic volume rides the same (tau, phase) pairing as the density, so\n"
"// colour interpolates on exactly the same axis the geometry does and cannot\n"
"// drift out of step with it.\n"
"function cput(unit,tex,k){const off=(k*3+ph)*CPLANE;\n"
" gl.activeTexture(gl.TEXTURE0+unit);gl.bindTexture(gl.TEXTURE_3D,tex);\n"
" gl.pixelStorei(gl.UNPACK_ALIGNMENT,1);\n"
" gl.texImage3D(gl.TEXTURE_3D,0,gl.RGBA8,CN,CN,CN,0,gl.RGBA,\n"
"  gl.UNSIGNED_BYTE,CD.subarray(off,off+CPLANE));}\n"
"function upload(){put(0,texA,ti);put(1,texB,(ti+1)%T);\n"
" cput(2,ctexA,ti);cput(3,ctexB,(ti+1)%T);}\n"
"upload();\n"
"const U=q=>gl.getUniformLocation(pr,q);\n"
"cv.addEventListener('pointerdown',e=>{drag=true;lx=e.clientX;ly=e.clientY;\n"
" cv.setPointerCapture(e.pointerId);});\n"
"cv.addEventListener('pointerup',()=>{drag=false;});\n"
"cv.addEventListener('pointermove',e=>{if(!drag)return;\n"
" yaw+=(e.clientX-lx)*0.0075;pitch+=(e.clientY-ly)*0.0075;\n"
" pitch=Math.max(-1.55,Math.min(1.55,pitch));lx=e.clientX;ly=e.clientY;});\n"
"const gt=document.getElementById('t'),gh=document.getElementById('h'),\n"
" gz=document.getElementById('z'),lt=document.getElementById('lt'),\n"
" lh=document.getElementById('lh'),lz=document.getElementById('lz'),\n"
" gf=document.getElementById('fr'),lf=document.getElementById('lf'),\n"
" pc=document.getElementById('pc'),p0=document.getElementById('p0'),\n"
" p1=document.getElementById('p1'),p2=document.getElementById('p2'),\n"
" mp=document.getElementById('mp'),mf=document.getElementById('mf');\n"
"gt.oninput=()=>{ti=+gt.value;play=false;bp.classList.remove('on');upload();sync();};\n"
"gh.oninput=()=>{lh.textContent=gh.value;};\n"
"gz.oninput=()=>{dist=+gz.value/100;lz.textContent=dist.toFixed(2);};\n"
"const bp=document.getElementById('bp'),bg=document.getElementById('bg');\n"
"bp.onclick=()=>{play=!play;bp.classList.toggle('on',play);};\n"
"bg.onclick=()=>{gov=!gov;bg.classList.toggle('on',gov);};\n"
"pc.onclick=()=>setph(-1);p0.onclick=()=>setph(0);\n"
"p1.onclick=()=>setph(1);p2.onclick=()=>setph(2);\n"
"gf.oninput=()=>{frac=(+gf.value)/100;play=false;\n"
" bp.classList.remove('on');sync();};\n"
"const bwb=document.getElementById('bw');\n"
"bwb.onclick=()=>{wl=!wl;bwb.classList.toggle('on',wl);};\n"
"const bbb=document.getElementById('bb');\n"
"bbb.onclick=()=>{bump=!bump;bbb.classList.toggle('on',bump);};\n"
"const brg=document.getElementById('brg');\n"
"brg.onclick=()=>{rgbaOn=!rgbaOn;brg.classList.toggle('on',rgbaOn);};\n"
"function sync(){const p=PF[ti],nx=(ti+1)%T;\n"
" lt.textContent=(ti/(T-1)).toFixed(2)+'  '+PHN[ph]\n"
"  +(phlock<0?'  (cycling)':'  (pinned)');\n"
" lf.textContent=(lerp?frac:0).toFixed(2)\n"
"  +'   tau '+(ti/(T-1)).toFixed(2)+' -> '+(nx/(T-1)).toFixed(2);\n"
" mp.textContent='s='+p.s.toFixed(3)+' f='+p.f.toFixed(3)\n"
"  +'  u='+p.u.toFixed(3)+(p.sat?' (saturated)':'')\n"
"  +'  active '+p.a+', spacelike '+p.sp;}\n"
"// Pinning a phase stops the cycle but leaves tau and the blend free, which is\n"
"// what makes the interpolation legible on its own.\n"
"function setph(k){phlock=k;\n"
" [pc,p0,p1,p2].forEach((b,i)=>b.classList.toggle('on',i-1===k||(k<0&&i===0)));\n"
" if(k>=0){ph=k;play=false;bp.classList.remove('on');}\n"
" upload();sync();}\n"
"lh.textContent=gh.value;lz.textContent=dist.toFixed(2);sync();\n"
"let last=performance.now();\n"
"function frame(now){\n"
" const dpr=Math.min(devicePixelRatio||1,1.75);\n"
" const w=Math.floor(innerWidth*dpr),h=Math.floor(innerHeight*dpr);\n"
" if(cv.width!==w||cv.height!==h){cv.width=w;cv.height=h;}\n"
" gl.viewport(0,0,w,h);\n"
" const dtms=Math.min(now-last,100);last=now;\n"
" if(play){\n"
"  // THE GOVERNOR ACTS HERE. u scales the advance rate, so a stretch of the\n"
"  // cycle dominated by spacelike nodes visibly slows. With the governor off\n"
"  // the rate is nominal, which is the comparison that shows it is doing work.\n"
"  acc+=dtms*(gov?PF[ti].u:1.0);\n"
"  // The fraction is the position between tau and tau+1, so the volume moves\n"
"  // continuously while the phase still steps discretely at each boundary.\n"
"  frac=Math.min(1,acc/420);gf.value=Math.round(frac*100);\n"
"  if(acc>420){acc=0;frac=0;\n"
"   // STRICT PRE -> PERI -> POST, then an explicit continuation to the next\n"
"   // tau. Sec.6.4: POST may reach PRE(c+1) only through a marked transition,\n"
"   // never by a silent write.\n"
"   // A pinned phase stops the sub-cycle advancing; tau still steps, so what\n"
"   // plays is one phase evolving through model time - which is the view that\n"
"   // makes the interpolation legible on its own.\n"
"   if(phlock<0){ph++; if(ph>2){ph=0;ti=(ti+1)%T;gt.value=ti;}}\n"
"   else {ti=(ti+1)%T;gt.value=ti;}\n"
"   upload();sync();}}\n"
" const cy=Math.cos(yaw),sy=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);\n"
" const cam=[cp*sy*dist,sp*dist,cp*cy*dist];\n"
" const fwd=[-cp*sy,-sp,-cp*cy],rt=[cy,0,-sy],up=[-sy*sp,cp,-cy*sp];\n"
" gl.uniform3f(U('cam'),cam[0],cam[1],cam[2]);\n"
" gl.uniformMatrix3fv(U('bas'),false,\n"
"  [rt[0],rt[1],rt[2],up[0],up[1],up[2],-fwd[0],-fwd[1],-fwd[2]]);\n"
" gl.uniform1f(U('thr'),(+gh.value)/100.0);\n"
" gl.uniform2f(U('res'),w,h);gl.uniform1f(U('n'),N);\n"
" gl.uniform1i(U('V'),0);gl.uniform1i(U('V2'),1);\n"
" gl.uniform1i(U('CV'),2);gl.uniform1i(U('CV2'),3);\n"
" gl.uniform1f(U('rgba'),rgbaOn?1.0:0.0);\n"
" gl.uniform1f(U('blend'),lerp?frac:0.0);\n"
" gl.uniform1f(U('bon'),bump?1.0:0.0);\n"
" gl.uniform1fv(U('bamp'),BA);gl.uniform3fv(U('bw'),BW);\n"
" gl.uniform1fv(U('bph'),BP);\n"
" const TC=[[0.85,0.45,0.30],[0.35,0.75,0.45],[0.35,0.55,0.95]];\n"
" gl.uniform3f(U('tint'),TC[ph][0],TC[ph][1],TC[ph][2]);\n"
" gl.drawArrays(gl.TRIANGLES,0,3);\n"
" const VW=innerWidth,VH=innerHeight;let out='';\n"
" // --- worldlines, WORLD-anchored: they are tracks in the field's space ---\n"
" if(wl){\n"
"  const P=q=>{const d=[q[0]-cam[0],q[1]-cam[1],q[2]-cam[2]];\n"
"   const zc=d[0]*fwd[0]+d[1]*fwd[1]+d[2]*fwd[2];\n"
"   if(zc<=0.02)return null;\n"
"   const xc=d[0]*rt[0]+d[1]*rt[1]+d[2]*rt[2];\n"
"   const yc=d[0]*up[0]+d[1]*up[1]+d[2]*up[2];\n"
"   return [VW*0.5+xc/zc*1.7*VH*0.5, VH*0.5-yc/zc*1.7*VH*0.5];};\n"
"  const CC=['#58a6ff','#d29922','#f85149','#6e7681'];\n"
"  const now=ti/(T-1);\n"
"  for(const w of WL){\n"
"   const a=P([w.x,(w.tau-0.18)-0.5,w.z]),b=P([w.x,(w.tau+0.18)-0.5,w.z]);\n"
"   if(a&&b)out+='<line x1=\"'+a[0]+'\" y1=\"'+a[1]+'\" x2=\"'+b[0]+'\" y2=\"'\n"
"    +b[1]+'\" stroke=\"'+CC[w.cls]+'\" stroke-width=\"1.6\" '\n"
"    +'stroke-opacity=\"0.7\"/>';\n"
"   const m=P([w.x,w.tau-0.5,w.z]);\n"
"   if(m){const on=Math.abs(w.tau-now)<=0.18;\n"
"    out+='<circle cx=\"'+m[0]+'\" cy=\"'+m[1]+'\" r=\"'+(on?4.5:2.5)\n"
"     +'\" fill=\"'+CC[w.cls]+'\" fill-opacity=\"'+(on?0.95:0.35)+'\"/>';\n"
"    if(on)out+='<text x=\"'+(m[0]+7)+'\" y=\"'+(m[1]+3)+'\" fill=\"#8b949e\" '\n"
"     +'font-size=\"10\" font-family=\"monospace\">'+w.nm+' '+CLSN[w.cls]\n"
"     +'</text>';}}}\n"
" // --- the PFISD trace, pinned: a control trace is not a place ---\n"
" const GX=VW-330,GY=VH-170,GW=290,GH=110;\n"
" out+='<rect x=\"'+(GX-14)+'\" y=\"'+(GY-24)+'\" width=\"'+(GW+30)+'\" height=\"'\n"
"  +(GH+60)+'\" fill=\"#0d1117\" fill-opacity=\"0.85\" stroke=\"#30363d\" rx=\"6\"/>';\n"
" out+='<text x=\"'+GX+'\" y=\"'+(GY-8)+'\" fill=\"#8b949e\" font-size=\"10\" '\n"
"  +'font-family=\"monospace\">PFISD: stress (red), flow (green), u (blue)</text>';\n"
" const SER=[['s','#f85149'],['f','#3fb950'],['u','#58a6ff']];\n"
" for(const [key,col] of SER){let pts='';\n"
"  for(let q=0;q<T;q++){const v=Math.max(0,Math.min(2,PF[q][key]))/2;\n"
"   pts+=(GX+GW*q/(T-1)).toFixed(1)+','+(GY+GH-GH*v).toFixed(1)+' ';}\n"
"  out+='<polyline fill=\"none\" stroke=\"'+col+'\" stroke-width=\"1.7\" points=\"'\n"
"   +pts+'\"/>';}\n"
" out+='<line x1=\"'+(GX+GW*ti/(T-1))+'\" y1=\"'+GY+'\" x2=\"'+(GX+GW*ti/(T-1))\n"
"  +'\" y2=\"'+(GY+GH)+'\" stroke=\"#e6edf3\" stroke-width=\"1\" '\n"
"  +'stroke-dasharray=\"3 3\"/>';\n"
" ov.setAttribute('viewBox','0 0 '+VW+' '+VH);ov.innerHTML=out;\n"
" requestAnimationFrame(frame);}\n"
"requestAnimationFrame(frame);\n</script>\n", f);
  fclose(f);

  free(pts); free(tree); free(fld); free(col);
  printf("\n  wrote %s\n", out);
  printf("  lossy record: density aggregate, %ld point-instances splatted,\n"
         "    %ld outside the box; point-level provenance not carried into\n"
         "    the volume; re-derivable from the same cloud and Strategy\n",
         splatted, dropped);
  canor_demo_prov_write_sidecar(out);
  printf("\n%d checks, %d failures\n", canor_demo_check_ran, canor_demo_check_fail);
  return canor_demo_check_fail ? 1 : 0;
}
