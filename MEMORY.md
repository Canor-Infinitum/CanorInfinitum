# MEMORY.md: Decision Log

Append-only. Format: `YYYY-MM-DD — Decision — Why — Source`.

## Project decisions
- 2026-10-18 — Documentation scaffold created (12 root files + README/AGENTS/CONTEXT per subfolder, all cross-linked). — Initial setup.
- 2026-10-18 — **Folder roles come from `Canor_Working-Draft_v1.pdf`** (QGTC/QGCT 1–4, Projects A–C). — User correction.
- 2026-10-18 — **All `fc_*` and `vi_*` modules are renamed to `canor_*`.** `fc` is legacy naming. `vi` is being retired. — User decision.
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
  8. Carmen™ Interlingua (Interoperability to complementary languages)
  9. Aether™ (Intermediate representation & timing-library linker)

## Legacy → canor rename map (Approved)
| Legacy | New | Destination |
|---|---|---|
| `fc.h/.c` (type tower) | `canor_core.h/.c` | `Canor/` |
| `fc_big` | `canor_big` | `Canor/CanorInfinitum` |
| `fc_prime` | `canor_prime` | `Canor/CanorInfinitum` |
| `fc_surreal` | `canor_surreal` | `CanorDatabasis` (hypercomplex hypersurreals) |
| `fc_qval` | `canor_qval` | `Canor/CanorInfinitum` |
| `fc_b2i` | `canor_b2i` | `Canor/CanorInfinitum` |
| `fc_tensor` | `canor_tensor` | `CanorDatabasis` (pure tensors) |
| `fc_dim`, `fc_scale`, `fc_agg`, `fc_fw` | `canor_dim`, `canor_scale`, `canor_agg`, `canor_fw` | `CanorLinguae` (Aether IR) |
| `fc_sym`, `fc_poly`, `fc_alg` | `canor_sym`, `canor_poly`, `canor_alg` | `CanorDatabasis` (Symbolicals) |
| `vi.h/.c` (Flow, RGB, hypervector, FlowStore) | `canor_flow.h/.c` | `CanorLinguae` |
| `vi_index` | `canor_index` | `Canor/CanorRepositorium` |
| `vi_strategy`, `vi_excite`, `vi_ore` | `canor_strategy`, `canor_excite`, `canor_ore` | `CanorInstinctivus` |
| `vi_golay` | `canor_golay` | `Canor/CanorInfinitum` |
| `examples/follow.c` | `examples/canor_demo.c` | `CarmenInstinctus` |
| `FC_*` / `VI_*` macros | `CANOR_*` | — |
| `fc_status` + `VI_OK…VI_FULL` | single `canor_status` | `Canor/` |

## Inherited from legacy (still valid after rename)
- **No floating point.** Exactness is the design basis, and the draft mandates integer + interval arithmetic.
- **IMAG = 5 trifectas (16, sedenions).** 13 elements has no Cayley–Dickson closure, so 4 trifectas is refused at init. The table is computed, not hand-written.
- **Hypervectors are packed 64-bit words** (1024 bits), with integer similarity in `[0, CANOR_ONE]`.
- **FlowValue bit encoding is pinned:** `00 NegZero, 01 NegOne, 10 PosZero, 11 PosOne`.
- **The conformance check must actually decode.** The lossless path runs over RGB strings. A FLOW id is a non-invertible hash.
- **Factoradic topology is never materialized.** The Lehmer code is the address (6–96 levels).
- **Two status families merge into one** `canor_status` (resolves the `fc_status` vs `VI_*` sign mismatch).

## Open items & Tracking
- [x] Build system: Both CMake and `west` (Zephyr) confirmed.
- [x] Rename-map placements confirmed and approved.
- [ ] CanorInstinctivus draft elaboration (pending further theoretical notes from draft `[...]`).
- [x] Interoperability language sequence: 1. Flow ... 8. Carmen, 9. Aether confirmed.
