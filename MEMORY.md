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
- 2026-10-18 — **5th Trifecta:** Defined as `(x, y, z)` bringing IMAG to 16 basis elements `[one=0, (i,j,k)=1..3, (m,n,o)=4..6, (r,s,t)=7..9, (e,f,g)=10..12, (x,y,z)=13..15]`.
- 2026-10-18 — **Quaternion Sub-algebras:** Profile configurations allow 4 quaternion sub-algebras (or modular/disjoint Cayley-Dickson products) without rigid global refusal.
- 2026-10-18 — **Grammar extracted:** `CanorLinguae/grammar/atoms.ebnf` extracted directly from Appendix A of Working Draft v1.
- 2026-10-18 — **ARM Architecture Support for Zephyr:** Explicitly added ARM (Cortex-M / AArch32 via `arm-zephyr-eabi`) alongside RISC-V for Zephyr RTOS targets.
- 2026-10-18 — **ARM architectures for Zephyr allowed & enabled:** ARM Cortex-M (ARMv6-M, ARMv7-M, ARMv8-M: Cortex-M0+/M3/M4/M7/M23/M33/M55) and AArch32/AArch64 are officially supported Zephyr RTOS targets, powered by the installed `arm-zephyr-eabi` SDK toolchain alongside RISC-V and POSIX native boards.
- 2026-10-20 — **Elite assembler module `canor_elite`** (`source/include/canor_elite.h`, `source/src/canor_elite.c`, `source/tests/canor_test_elite.c`): side-effect-free, branch-retaining pattern assembler plus the multilateral Diffeomorphic Basis accord. Self-contained (needs only `canor_core.h` and `CANOR_ONE`). — User request. — WD v1 §7, §6; Z-Notation Draft §7, §8.
- 2026-10-20 — **United meta-schema validity is multilateral and unanimous** (Z-draft `BilateralUnitedValidity`, `#approvals = 2`, becomes the N = 2 case): every signatory (N ≥ 2; User / App / Agent, internal or external) (1) ratifies the preset basis, (2) submits per error category an allowance A and the measured orthonormalization error E of its own meta-schema extraction, (3) is certified by every *other* signatory per category (E ⊆ A ⊆ basis bound B), (4) approves the same object digest, version, basis, provenance and margin. One rejection rejects; one pending item leaves it incomplete; an unset profile algorithm leaves it undefined. Never accepted by default. — User decision. — `canor_elite_accord_evaluate`.
- 2026-10-20 — **Elite declared readings (confirm or replace):** (a) Z's equal `acceptedErrorMargin` is the basis bound vector B[c], identical for every approval by construction, while each A[s][c] ⊆ B[c]; (b) margins are closed integer intervals over a per-category scale ≤ 2^20 (replaces Z's ℝ); (c) built-in metric `gram-linf` = L∞ distance of the Gram matrix from the identity, exact enclosure [floor, ceil]; (d) ratify and approve statements bind the whole signatory roster, so adding a party voids earlier consents; (e) certificates sign the exact numbers and the evidence digest, and certifying an out-of-bound or contradicted submission is itself diagnosed; (f) branch order is mixed-radix with the first-traversed site most significant (L2R / R2L traversal identity). — Declared reading.
- 2026-10-20 — **Elite source is freeform and line-based**, and Elite alone is not read through `atoms.ebnf` (resolves §7's `$` against the proposed `$` = variable sigil). Each LF/CRLF line is either a rule `spec : def | def | …` (whatever precedes the first `:` is redefined as whatever follows it; any text either side) or a program line. `|` separates definitions outside `()`/`[]`/`{}`; `\:` and `\|` are the only escapes (TeX such as `\alpha` passes through). — User decision. — `canor_elite_program_parse`.
- 2026-10-20 — **Freeform declared readings (confirm or replace):** a respecified spec ACCUMULATES definitions in source order (not last-wins: an earlier definition is never silently dropped); definitions may name specs (nested replacement, ≤ `CANOR_ELITE_DEPTH_MAX` = 8 rewrites per line; cycles end as `depth-limit` diagnostics); a spec matches only where it does not split a word, leftmost-longest under L2R, rightmost-longest under R2L; ≤ `CANOR_ELITE_EXPANSION_MAX` = 32 interpretations per line, beyond that `expansion_limited` and the traversal is incomplete; interpretation order is lexicographic over rewrite paths. — Declared reading.
- 2026-10-21 — **Opt-in PQC profile `pqc-slh-dsa`** described in `CanorLinguae/elite/TRUST.md` (SHA3-512, SLH-DSA-SHAKE-256s, hybrid McEliece + ML-KEM-1024); not a project default; no core change. — User decision (rescoped from the 2026-10-20 trust-model session; the core changes made that day were reverted).
- 2026-10-21 — **INI variable classes PINNED** (SCHEMA.md "INI variables"): `__name__` Global (internal), `_name_` Scoped (internal/project), `__name` Limited (project), `_name` Custom (any), `name` Public; strict lowercase names for the underscore classes, case-blind compare, untranslated names, Globals equal across locale files, `;` comments. — User decision (approved as shown). — SCHEMA.md.

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
| Elite Assembler & Diffeomorphic Basis | `canor_elite.h/.c` | `CanorLinguae` (Elite §7) |
| Store & Indexing | `canor_index.h/.c` | `Canor/CanorRepositorium` |
| Strategy, Excitation & ORE | `canor_strategy`, `canor_excite`, `canor_ore` | `CanorInstinctivus` |
| Golay Coding | `canor_golay.h/.c` | `Canor/CanorInfinitum` |
| Executable Reference Demo | `examples/canor_demo.c` | `CarmenInstinctus` |
| Canonical Macros | `CANOR_*` | — |
| Unified Status Codes | `canor_status` | `Canor/` |

## Core Implementation Invariants
- **No floating point.** Exactness is the design basis, and the draft mandates integer + interval arithmetic.
- **IMAG = 5 trifectas (16, sedenions).** 13 elements has no Cayley–Dickson closure, so 4 trifectas is refused at init. The table is computed, not hand-written.
- **Hypervectors are packed 64-bit words** (1024 bits), with integer similarity in `[0, CANOR_ONE]`.
- **FlowValue bit encoding is pinned:** `00 NegZero, 01 NegOne, 10 PosZero, 11 PosOne`.
- **The conformance check must actually decode.** The lossless path runs over RGB strings. A FLOW id is a non-invertible hash.
- **Factoradic topology is never materialized.** The Lehmer code is the address (6–96 levels).
- **Unified status code family:** `canor_status` provides single, consistent return diagnostics.

## Open items & Tracking
- [x] Build system: Both CMake and `west` (Zephyr) confirmed.
- [x] Module placements confirmed and approved.
- [ ] CanorInstinctivus draft elaboration (pending further theoretical notes from draft `[...]`).
- [x] Interoperability language sequence: 1. Flow ... 8. Carmen, 9. Aether confirmed.
- [ ] Elite: digest and signature algorithms, trust roots, revocation, signature lifecycle (profile-defined; `canor_elite` calls the profile's pure digest/verify functions and reports UNDEFINED while unset).
- [ ] Elite: error-category taxonomy (anti-cheat, anti-malware / anti-virus, …) and each category's metric, scale and bound; metrics other than `gram-linf` are measured outside the core.
- [ ] Elite: certification rule. Current: recompute, then E_recomputed ⊆ E_claimed ⊆ A ⊆ B. Alternative: a tolerance band.
- [ ] Elite: candidate-ordering schema beyond the declared lexicographic order, and the Aether §9 latency/jitter field per branch traversal (needs `canor_timing`).
- [x] Elite surface syntax and nested replacement: freeform line-based rules (2026-10-20).
