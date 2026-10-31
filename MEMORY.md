# MEMORY.md: Decision Log

Append-only. Format: `YYYY-MM-DD — Decision — Why — Source`.

## Project decisions
- 2026-10-18 — **Documentation scaffold created** (root files + README/AGENTS/CONTEXT per subfolder, cross-linked). — Initial setup.
- 2026-10-18 — **Folder roles come from `Canor_Working-Draft_v1.pdf`** (QGTC/QGCT 1–4, Projects A–C). — User correction.
- 2026-10-18 — **Standardized on canonical `canor_*` naming** across all headers, sources, and tests. — User decision.
- 2026-10-18 — **Locales:** Follow™ EN (primary), Redko™ RU, Avec™ FR, Fengshui™ ZH-Hant, Canor™ LA. — Draft, "LOCALES – UN-Alignment".
- 2026-10-18 — Copilot instructions are mirrored to `.github/copilot-instructions.md`. — User decision.

- 2026-10-18 — **Build system:** Both CMake (host/desktop builds) and `west` (Zephyr RTOS targets) will be supported.
- 2026-10-18 — **Rename-map approved:** All legacy modules officially adopt the `canor_*` mapping in `include/` and `src/`.
- 2026-10-18 — **Nine interoperability languages ordered sequence confirmed:**
  1. Flow™
  2. MetaFlow
  3. Action™
  4. Soul™
  5. Maneuver™
  6. United™ (Interchange language)
  7. Elite™ (Branching assembler)
  8. Carmen™ Interlingua (Interoperability with complementary languages)
  9. Aether™ (Intermediate representation & timing-library linker)
- 2026-10-20 — **Locale marks clarified:** Follow™ is the English locale mark of the whole protocol / project name. Follower™ is the English locale mark of the Carmen™ Interlingua ("Follower Interlingua" = Carmen™ Interlingua). — User clarification.
- 2026-10-20 — **`/canor-infinitum` skill added** (`.claude/skills/canor-infinitum/SKILL.md`): MOT pipeline phase templates A–G keyed on explicit term mentions. — User request.
- 2026-10-20 — **Nine per-language skills added** (`.claude/skills/{flow,metaflow,action,soul,maneuver,united,elite,follower,aether}/SKILL.md`), one per interoperability language, each grounded in Working Draft v1 §1–§9. `/follower` = Carmen™ Interlingua (§8). `/aether` opens with an intent Q&A (minimize error / read error log / project status / build IR). Default artifact location `CanorLinguae/<language>/`. Profile-defined items (digest algorithm, grayscale conversion, dispatch, rollback/commit, rate-score rule) stay open items, never invented. — User request.
- 2026-10-20 — **Language skills made production grade:** every `/flow` … `/aether` SKILL.md now opens with the shared *Canor™ workspace* prerequisite: a `.<project>/` folder holding `<project>.fwp` plus named `*.fws` settings (`default.fws`, …), the Canor™ file table and the keyword-to-profile map. Artifacts move from `CanorLinguae/<language>/` to `.<project>/<language>/<name>.fwh|.fwb`. Examples use the proposed default grammar semantics (pending review) and carry no locale-mark asides. — User request.
- 2026-10-20 — **`/canor-infinitum` redesigned as the MOT hub (supersedes the phase-template entry above):** it frames, records and routes; artifacts are built by `/flow` … `/aether`. Four modes (Orient / Frame / Walk / Status), one phase record per phase (perspective, participants, signatories, category, completion, assignment, concurrency, integer budgets over `CANOR_ONE`, diagnostics, open items, next skill). F/G artifacts stay internal to one signatory. No locale-mark asides. — User request.
- 2026-10-18 — **5th Trifecta:** Defined as `(x, y, z)` bringing IMAG to 16 basis elements `[one=0, (i,j,k)=1..3, (m,n,o)=4..6, (r,s,t)=7..9, (e,f,g)=10..12, (x,y,z)=13..15]`.
- 2026-10-18 — **Quaternion Sub-algebras:** Profile configurations allow 4 quaternion sub-algebras (or modular/disjoint Cayley-Dickson products) without rigid global refusal.
- 2026-10-18 — **Grammar extracted:** `CanorLinguae/grammar/atoms.ebnf` extracted directly from Appendix A of Working Draft v1.
- 2026-10-18 — **ARM Architecture Support for Zephyr:** Explicitly added ARM (Cortex-M / AArch32 via `arm-zephyr-eabi`) alongside RISC-V for Zephyr RTOS targets.
- 2026-10-18 — **ARM architectures for Zephyr allowed & enabled:** ARM Cortex-M (ARMv6-M, ARMv7-M, ARMv8-M: Cortex-M0+/M3/M4/M7/M23/M33/M55) and AArch32/AArch64 are officially supported Zephyr RTOS targets, powered by the installed `arm-zephyr-eabi` SDK toolchain alongside RISC-V and POSIX native boards.
- 2026-10-20 — **IMAG naming re-pinned (supersedes the 2026-10-18 `(x,y,z)` entry):** `[one=0, (i,j,k)=1..3, (l,m,n)=4..6, (o,p,q)=7..9, (r,s,t)=10..12, (e,f,g)=13..15]`; differentials `di … dg` in the same order. Indices and the Cayley-Dickson table are unchanged; only names moved. — User approval (PINNED change). — `canor_imag_name`, SCHEMA.md.
- 2026-10-20 — **Flow space = 6^5 = 7,776:** all 15 atoms, trifecta order fixed, each trifecta independently over its order-6 permutohedron (3! hexagon). Rank = mixed radix 6, trifecta 0 most significant. — User decision. — `canor_enum.h`.
- 2026-10-20 — **Strategy space = 15 differentials × Order 1..6 = 90.** Order = lazy-evolutionary nesting depth of the Cayley-Dickson / Ore form. — User decision.
- 2026-10-20 — **MetaFlow = 4! = 24** permutations of `R,G,B,A`; Soul = Strategy × Flow × MetaFlow = 16,796,160; Action = `m ≣ n (mod o)`, m,n,o ∈ 0..289 = 24,389,000 (84,100 with o = 0 kept as UNDEFINED diagnostics); Maneuver = Soul × Action = 409,641,546,240,000.
- 2026-10-20 — **Soul, Action and Maneuver are index-addressed, never materialized.** `.fwb` holds radix spec, counts, first/last K samples, gap records naming the unrank function, and digests. — User decision. — `tools/canor_gen.c`.
- 2026-10-20 — **Timing library:** `canor_field3_t` = PRE/PERI/POST × MIN/NOM/MAX × LOW/VALUE/HIGH; property type fixed at compile time (`CANOR_TIMING_INTERVAL` 0 → int64, 27 scalars; 1 → LO/HI, 54). Latency = per-sample ops/sec; jitter = |Δ| between consecutive samples; LOW/VALUE/HIGH = ±1-tick quantization enclosure. — Declared reading. — `canor_timing.h`.
- 2026-10-20 — **SWAR library:** narrow lanes 2..64 bit as macros on `uint64_t`; wide lanes 128..4096 bit as `uint64_t` limbs with factoradic conversion (`canor_swar.h`). 1024-bit = hypervector width.
- 2026-10-20 — **Build:** `source/CMakeLists.txt` (GCC/Clang, `-Wall -Wextra -Werror -pedantic`), tests under `source/tests/`, generator under `source/tools/`. Generated `.fwh/.fwb` go to `build/` (gitignored).
- 2026-10-20 — **Reference demo split by feature:** `source/examples/canor_demo.c` (12,873 lines) is now the folder `source/examples/canor_demo/`: `canor_demo.h` (shared types and cross-file helpers, all `canor_demo_`-prefixed), one `canor_demo_<feature>.c` per command, `canor_demo_util.c` for shared helpers, and `canor_demo.c` for usage and dispatch. The `examples/canor_demo.c` row of the module mapping now means this folder. Behaviour unchanged: 59 command runs byte-compared (stdout, exit code, emitted files) against the single-file build. Build: `gcc -std=gnu11 -Wall -Wextra -Isource/include source/examples/canor_demo/*.c source/src/*.c -lm`. — Maintainability. — User request.
- 2026-10-20 — **Elite assembler module `canor_elite`** (`source/include/canor_elite.h`, `source/src/canor_elite.c`, `source/tests/canor_test_elite.c`): side-effect-free, branch-retaining pattern assembler plus the multilateral Diffeomorphic Basis accord. Self-contained (needs only `canor_core.h` and `CANOR_ONE`). — User request. — WD v1 §7, §6; Z-Notation Draft §7, §8.
- 2026-10-20 — **United meta-schema validity is multilateral and unanimous** (Z-draft `BilateralUnitedValidity`, `#approvals = 2`, becomes the N = 2 case): every signatory (N ≥ 2; User / App / Agent, internal or external) (1) ratifies the preset basis, (2) submits per error category an allowance A and the measured orthonormalization error E of its own meta-schema extraction, (3) is certified by every *other* signatory per category (E ⊆ A ⊆ basis bound B), (4) approves the same object digest, version, basis, provenance and margin. One rejection rejects; one pending item leaves it incomplete; an unset profile algorithm leaves it undefined. Never accepted by default. — User decision. — `canor_elite_accord_evaluate`.
- 2026-10-20 — **Elite declared readings (confirm or replace):** (a) Z's equal `acceptedErrorMargin` is the basis bound vector B[c], identical for every approval by construction, while each A[s][c] ⊆ B[c]; (b) margins are closed integer intervals over a per-category scale ≤ 2^20 (replaces Z's ℝ); (c) built-in metric `gram-linf` = L∞ distance of the Gram matrix from the identity, exact enclosure [floor, ceil]; (d) ratify and approve statements bind the whole signatory roster, so adding a party voids earlier consents; (e) certificates sign the exact numbers and the evidence digest, and certifying an out-of-bound or contradicted submission is itself diagnosed; (f) branch order is mixed-radix with the first-traversed site most significant (L2R / R2L traversal identity). — Declared reading.
- 2026-10-20 — **Elite source is freeform and line-based**, and Elite alone is not read through `atoms.ebnf` (resolves §7's `$` against the proposed `$` = variable sigil). Each LF/CRLF line is either a rule `spec : def | def | …` (whatever precedes the first `:` is redefined as whatever follows it; any text either side) or a program line. `|` separates definitions outside `()`/`[]`/`{}`; `\:` and `\|` are the only escapes (TeX such as `\alpha` passes through). — User decision. — `canor_elite_program_parse`.
- 2026-10-20 — **Freeform declared readings (confirm or replace):** a respecified spec ACCUMULATES definitions in source order (not last-wins: an earlier definition is never silently dropped); definitions may name specs (nested replacement, ≤ `CANOR_ELITE_DEPTH_MAX` = 8 rewrites per line; cycles end as `depth-limit` diagnostics); a spec matches only where it does not split a word, leftmost-longest under L2R, rightmost-longest under R2L; ≤ `CANOR_ELITE_EXPANSION_MAX` = 32 interpretations per line, beyond that `expansion_limited` and the traversal is incomplete; interpretation order is lexicographic over rewrite paths. — Declared reading.
- 2026-10-21 — **Flow values carry AtomUsage `(atom, ring)`; every occurrence counts; the ring index is categorical** (selects an indexed instance, matched by atom name, never arithmetic). The 7,776 enumerated Flows happen to contain each atom once because each is a permutation; the value type (`canor_soul_flow_t`) admits repeats. The Cayley-Dickson fold (index XOR) is the IMAG product, not a Flow's value, and is no longer reported. — User correction. — `canor_soul.h`.
- 2026-10-21 — **Soul value = the §4 count stream** (n + 1 vectors per trifecta, each consumed position decrements its matching components; L→R default; `one` goes to the auxiliary channel). Grayscale conversion and realification/complexification stay profile-defined (`unset`). — /soul skill. — `canor_soul.h`.
- 2026-10-21 — **Representations (supersedes the 2026-10-20 open item):** Elementary = (Real, Base), Unique = (Real, Bases), Composite = (Reals, Base), Unknown = (Reals, Bases); Reals = count-stream steps, Bases = IMAG units. Per Soul/Maneuver: 240 / 16 / 15 / 1. — User decision; CanorDB §3.2. — `canor_soul.h`.
- 2026-10-21 — **Strategy = Ore extension of its differential's variable (supersedes "nesting depth"):** `d_u` adjoined as `K[x_u][d_u; σ, δ]`, `d_u·f = σ(f)·d_u + δ(f)`; Order k = the power `d_u^k`, evaluated exactly by `canor_ore` (C, S, T). Profile default τ = σ = 1 (period 6, so Orders 1..6 run one period). — User decision. — `canor_enum_strategy_ore`.
- 2026-10-21 — **MetaFlow invocation = complex-fractal string, slot for slot:** R Reals = what remains of `[0, b^k]`, G Gaps = removed lengths `b^(k-1-j)` × `(b-1)^j`, B Bases = scaling base b and imaginary base `e_u`, A Auxiliaries = `z -> z**2 + c`, c ∈ span{1, e_u}; depth k = Strategy order, `e_u` = its differential, slot order = the MetaFlow permutation. Default b = 3 (trifecta arity), profile-overridable (`CANOR_ENUM_FRACTAL_BASE`). — User decision. — `canor_enum_invocation_unrank`.
- 2026-10-21 — **Soul category = use:** Storage (symbol stored in a database or written to a `.fwb`), Learning (used by a learning system), Operation (used at runtime / I/O). Generated symbols are Storage. — User decision.
- 2026-10-21 — **Maneuver = Soul Symbol bound one-to-one to its Action**, which maps the Soul's count stream to canonical residues mod o; o = 0 stays UNDEFINED. Assignment default `unassigned`; effects none. — /maneuver skill.
- 2026-10-21 — **Generator digests are profile-defined (`unset`)**; the United digest is specified in another session. Output layout `<out>/<language>/<language>.fwh|.fwb`. — User decision.
- 2026-10-21 — **Opt-in PQC profile `pqc-slh-dsa`** described in `CanorLinguae/elite/TRUST.md` (SHA3-512, SLH-DSA-SHAKE-256s, hybrid McEliece + ML-KEM-1024); not a project default; no core change. — User decision (rescoped from the 2026-10-20 trust-model session; the core changes made that day were reverted).
- 2026-10-21 — **INI variable classes PINNED** (SCHEMA.md "INI variables"): `__name__` Global (internal), `_name_` Scoped (internal/project), `__name` Limited (project), `_name` Custom (any), `name` Public; strict lowercase names for the underscore classes, case-blind compare, untranslated names, Globals equal across locale files, `;` comments. — User decision (approved as shown). — SCHEMA.md.
- 2026-10-22 — **The repository's Markdown is the spec authority; the PDF drafts are legacy.** CLAUDE.md, CONTEXT.md, README.md, SKILL.md and PROMPTS.md no longer point agents at PDFs outside the repo. §-labels stay as citations. — User direction.
- 2026-10-22 — **Legacy modules ported to invariants 1–2 (integer only, fixed capacity).** The library has no heap and no floating point. The improved AGENTS.md lint is empty for `source/include` and `source/src`, and gcc + clang build and pass ctest. Changes:
  - **`canor_big`:** heap `fb_*` is replaced by caller-placed `canor_big_t` / `canor_big_rat_t` with `CANOR_BIG_LIMBS_MAX` = 128 × 32-bit limbs (4096 bits). An operation that needs more is CANOR_ERR_FULL and leaves its destination unchanged. Adds `isqrt` / `iroot`.
  - **`canor_poly`:** Mittag-Leffler computes 1/Γ from the declared A&S 6.1.34 26-coefficient integer table, quantized to 64 significant bits. `t^α` uses an exact integer root, with the argument as floor(t^α·10⁶)/10⁶.
  - **`canor_prime`:** the sieve uses static tables sized by `CANOR_PRIME_LIMIT`, and a limit above it is refused rather than clamped. `canor_residual_L` takes θ, φ as `canor_rational` and reports Q20 `value_*_q20`. The Arbitrary-class order uses a Q30 integer exp2.
  - **`canor_sym`:** series and axis values are int64 milli. The log axes are now base 2 instead of base 10 (a declared scale change).
  - **`canor_golay`:** `vt_*` is replaced by `canor_golay_tele_*` on `canor_timing` samples (CANOR_ERR_FULL at `CANOR_TIMING_SAMPLES_MAX`, never dropped). The 1.96σ forecast is an outward-rounded integer interval. The M24 search uses a caller workspace (`canor_golay_m24_ws_t`: 2^24-bit visited bitset plus a queue with a capacity).
  - **`canor_index`:** `canor_tree` / `canor_ann` are caller-placed structs (`CANOR_TREE_WORDS_MAX`, `CANOR_ANN_TABLES/BITS/N_MAX`).
  - **`canor_flow`:** RGBString is a value type (`CANOR_RGB_MAX`). Dictionary, WordFlowMap and FlowStore use fixed arrays (`CANOR_DICT_MAX`, `CANOR_LABEL_MAX`, `CANOR_WORDFLOW_MAX`, `CANOR_FLOWSTORE_MAX`), and the `*_free` functions are gone.
  - **`canor_fw`:** `fw_read_file` reads into a caller buffer.
  - **Tests:** `canor_test_big`, `canor_test_containers`.
  - **Known break:** `source/examples/canor_demo/` no longer compiles because it still uses `fb_*`, `vt_*` and the old signatures; examples are tracked separately. — User approval (fix requested). — `canor_big.h`, `canor_golay.h`, `canor_index.h`, `canor_flow.h`.
- 2026-10-22 — **`canor_demo` ported to the fixed-capacity APIs** (fixes the break above). It builds with the 2026-10-20 line and shows only the 5 warnings that were already there. All 51 commands give the same exit codes as the pre-port build, no check fails, and 45 produce byte-identical stdout. Changes:
  - **Budget:** `--budget` / `budget_limbs` is now a per-value limb cap ≤ `CANOR_BIG_LIMBS_MAX` (0 = the maximum; a larger value is refused). The `peakKB` column is gone.
  - **Capacity for deep orbits:** at the default 128 limbs, `mandel` fuel ≥ 10 and deep `describe` orbits report as undetermined. Build with `-DCANOR_BIG_LIMBS_MAX=1024` to reproduce the old output exactly; it runs on the default stack.
  - **Expected differences:**
    - `golay` telemetry is printed in integer milli-units;
    - `poly`'s E_{1/2} closed-form check went from 1.4e-5 to 2.3e-7 (its assertion tightened from 5e-5 to 1e-6);
    - the Arbitrary-class order moved by ≤ 0.004% (integer `exp2`);
    - `scene`'s α = 1/2 curve moved by ≤ 1e-5.
  - **Library fix found by the comparison:** the Mittag-Leffler stop test in `src/canor_poly.c` is now exact (the term truncates to zero in Q20) instead of a ±1-bit width estimate. This restores E_{1/4} at t = 4 (0.377376).
  - — User request. — `source/examples/canor_demo/`.
- 2026-10-25 — **`docs/Canor.md` is the project documentation and specification.** It heads the authority list. A PINNED entry in SCHEMA.md stands until an entry here changes it. It was copied from the main checkout with formatting-only fixes (headings, a stray footnote marker, broken bold). — User direction, 2026-10-24.
- 2026-10-25 — **Ten languages, Strategy first (supersedes the 2026-10-18 nine-language sequence):** 1 Strategy™, 2 Flow™, 3 MetaFlow™, 4 Action™, 5 Soul™, 6 Maneuver™, 7 United™, 8 Elite™, 9 Follower™ / Carmen™ Interlingua, 10 Aether™ IR. — Canor.md.
- 2026-10-25 — **Section labels follow the 1–10 order** in every tracked file and skill, cited as `Canor §N Name` (SCHEMA.md "Section labels"). Old Working Draft v1 label §n (n = 1…9) is now §(n+1), and the old §0 is Strategy §1. Z-Notation draft and CanorDB section numbers are unchanged. **Entries above this one keep their original labels.** — User decision.
- 2026-10-25 — **IMAG re-pinned: every trifecta is one of Hamilton's quaternionic trifectas (PINNED change).** Names and indices are unchanged. The product is Cayley–Dickson doubling over pinned coordinates: one 0, i 1, j 2, k 3, l 4, m 8, n 12, o 5, p 10, q 15, r 6, s 11, t 13, e 7, f 9, g 14. These are the lexicographically first spread of PG(3,2), and all 56 spreads lie in one GL(4,2) orbit, so the choice is a relabelling. (i,j,k) with l, o, r, e spans the octonions. Before this, only (i,j,k) closed (l·m was +i), because since the 2026-10-18 port the trifectas sat on consecutive Cayley–Dickson indices. Adds `canor_imag_cd_index` / `canor_imag_from_cd_index`; `canor_excite` works in coordinates. New tests: Hamilton rules per trifecta, octonion non-associativity, sedenion zero divisors. — User approval. — `canor_core.c`, SCHEMA.md "IMAG basis".
- 2026-10-25 — **`*.error` files are Aether diagnostic output names, not a pinned extension.** — User decision.
- 2026-10-25 — **Follower code blocks use the Canor.md pipe-dot framing `| ... .`**, replacing the skills' `|.lang ... .|`. The block's language comes from its `lang=` attribute. — User decision.
- 2026-10-25 — **The core keeps the integer parts; the rest is profile-defined.** In the core: Hilbert ordering, quadtrees, mipmaps and checksums (Soul) and their like. Profile-defined: diffusion transformers, MCMC, and agent- or LLM-guided rewriting. Agent and MCMC outputs enter as recorded inputs with seed and provenance. Reaching a configured limit is reported, never silently cut. — User decision. — AGENTS.md invariant 8, DESIGN.md.

## Canonical Module Mapping (Approved)
| Module Name | Target Header / Source | Destination |
|---|---|---|
| Core Type Tower | `canor_core.h/.c` | `Canor/` |
| Big Integers | `canor_big.h/.c` | `Canor/CanorInfinitum` |
| Prime & Factoradic | `canor_prime.h/.c` | `Canor/CanorInfinitum` |
| Hypersurreal Numbers | `canor_surreal.h/.c` | `CanorDatabasis` (hypercomplex hypersurreals) |
| Q-Values & Rationals | `canor_qval.h/.c` | `Canor/CanorInfinitum` |
| Basis-to-Integer | `canor_b2i.h/.c` | `Canor/CanorInfinitum` |
| Pure Tensors & Networks | `canor_tensor.h/.c` | `CanorDatabasis` (pure tensors) |
| Dimensions, Scales & Aggregates | `canor_dim`, `canor_scale`, `canor_agg`, `canor_fw` | `CanorLinguae` (Aether IR) |
| Symbols, Polynomials & Algebra | `canor_sym`, `canor_poly`, `canor_alg` | `CanorDatabasis` (Symbolicals) |
| Flow, RGB & Hypervectors | `canor_flow.h/.c` | `CanorLinguae` |
| Elite Assembler & Diffeomorphic Basis | `canor_elite.h/.c` | `CanorLinguae` (Elite §8) |
| Store & Indexing | `canor_index.h/.c` | `Canor/CanorRepositorium` |
| Strategy, Excitation & ORE | `canor_strategy`, `canor_excite`, `canor_ore` | `CanorInstinctivus` |
| Golay Coding | `canor_golay.h/.c` | `Canor/CanorInfinitum` |
| Executable Reference Demo | `examples/canor_demo.c` | `CarmenInstinctus` |
| Canonical Macros | `CANOR_*` | — |
| Unified Status Codes | `canor_status` | `Canor/` |

## Core Implementation Invariants
- **No floating point.** Exactness is the design basis, and the draft mandates integer + interval arithmetic.
- **IMAG = 5 trifectas (16, sedenions), each one of Hamilton's quaternionic trifectas.** 13 elements has no Cayley–Dickson closure, so 4 trifectas is refused at init. The table is computed from the pinned 16-entry coordinate map, not hand-written.
- **Hypervectors are packed 64-bit words** (1024 bits), with integer similarity in `[0, CANOR_ONE]`.
- **FlowValue bit encoding is pinned:** `00 NegZero, 01 NegOne, 10 PosZero, 11 PosOne`.
- **The conformance check must actually decode.** The lossless path runs over RGB strings. A FLOW id is a non-invertible hash.
- **Factoradic topology is never materialized.** The Lehmer code is the address (6–96 levels).
- **Unified status code family:** `canor_status` provides single, consistent return diagnostics.

## Open items & Tracking
- [x] Build system: Both CMake and `west` (Zephyr) confirmed.
- [x] Module placements confirmed and approved.
- [ ] CanorInstinctivus draft elaboration (pending further theoretical notes from draft `[...]`).
- [x] Language sequence: 1. Strategy … 10. Aether (2026-10-25, Canor.md; supersedes 1. Flow … 9. Aether).
- [x] Representations, MetaFlow complex-fractal excitation, Strategy ↔ Ore binding, Soul category: decided 2026-10-21 (entries above).
- [ ] Digest algorithm: profile-defined; being specified in the United/Elite session. Generator writes `unset`.
- [ ] Soul grayscale conversion and realification/complexification form: profile-defined, `unset`.
- [x] Legacy modules violated invariants 1–2: ported 2026-10-22 (entry above).
- [x] `source/examples/canor_demo/` moved to the ported APIs (2026-10-22, entry above).
- [ ] Legacy non-`canor_*` identifiers remain in `canor_golay` (`golay_*`, `m24_*`, `prov_*`) and `canor_fw` (`fw_*`); renaming needs its own pass.
- [ ] Elite: digest and signature algorithms, trust roots, revocation, signature lifecycle (profile-defined; `canor_elite` calls the profile's pure digest/verify functions and reports UNDEFINED while unset).
- [ ] Elite: error-category taxonomy (anti-cheat, anti-malware / anti-virus, …) and each category's metric, scale and bound; metrics other than `gram-linf` are measured outside the core.
- [ ] Elite: certification rule. Current: recompute, then E_recomputed ⊆ E_claimed ⊆ A ⊆ B. Alternative: a tolerance band.
- [ ] Elite: candidate-ordering schema beyond the declared lexicographic order, and the Aether §10 latency/jitter field per branch traversal (needs `canor_timing`). Canor.md §8 puts the interval timing field in Elite's header.
- [x] Elite surface syntax and nested replacement: freeform line-based rules (2026-10-20).
- [x] Action source spelling: `L = R (mod N)` inside a `%` modulation is the congruence form (2026-10-22, user confirmed).
- [x] `/elite` skill aligned with the freeform Elite syntax: `.fwh` = Canor header (provenance, basis, tooling, direction, limit), `.fwb` = pure Elite source, since Elite has no comment syntax (2026-10-22).
- [x] Language skills (`/flow` … `/aether`) regenerated from one shared template (2026-10-22). Kept: the decided facts merged in by other sessions (representations, categories, Ore Strategies, MetaFlow fractal string, enumeration sizes, `canor_field3_t`, complementary languages) plus the pinned `.ini` key classes. Dropped: platform asides (Zephyr / sandbox / WASM paragraphs). Restored: the user's original `/aether` intent questions. The skills are the language templates for the `*-intent` rewrite.
- [ ] Timing prerequisite scope: Canor.md requires the Aether timing suite in "all instantiated code blocks". Is that CanorLinguae `.fwh`/`.fwb` blocks only, or the C core too? Also, `CANOR_TIMING_INTERVAL` defaults to 0 (27 scalars), but Canor.md describes LO/HI interval fields.
- [ ] Soul permutations MLO and MRO (Canor.md §5): meaning not yet defined.
- [ ] Ether™ (Canor.md §10 "Ether™ / Aether™ IR"): its relation to Aether™ is not yet recorded.
- [ ] Flow scripts (Canor.md §2): `.` as a subscript and `'` as a superscript marker collide with atoms.ebnf, where both are SELECTOR. Whitespace as lexical metadata has no grammar rule yet.
- [ ] `/strategy` skill: Strategy §1 has no skill yet; `/canor-infinitum` routes Strategy through `/flow`.
- [ ] `canor_demo` outputs that print IMAG products, groupings or spreads change with the 2026-10-25 IMAG re-pin. Checked 2026-10-31 against the ported demo: `hp4` is unchanged, `excite` output changes, and `sed2hp4` fails 1 of 6 checks because it XORs basis indices instead of `canor_imag_cd_index` coordinates. The core suite passes; the demo needs the coordinate fix.
