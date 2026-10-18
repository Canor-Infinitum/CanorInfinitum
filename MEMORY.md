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
