# SCHEMA.md: Types, Encodings, Interchange

> Entries marked **PINNED** are specification. Changing one requires an entry in MEMORY.md.

## File extensions (draft §9) — **PINNED**
| Ext | Definition |
|---|---|
| `.ini` | Canor™ settings |
| `.fwp` | CanorLinguae project / profile |
| `.fws` | CanorLinguae settings |
| `.fwh` | CanorLinguae source header |
| `.fwb` | CanorLinguae source body |
| `.fwo` | CanorLinguae compiled object (United™ container) |

## INI variables (`.ini`) — **PINNED**
Each locale has its own `.ini` (e.g. `locales/Canor.ini`, locale `la`). Apart from `[Section]` headers, every line is a `key=value` pair, a comment or blank. A key's class comes from its underscores:

| Form | Class | Use |
|---|---|---|
| `__name__` | Global | Internal use only |
| `_name_` | Scoped | Internal / project use only |
| `__name` | Limited-usage | Project use only |
| `_name` | Custom-usage | Any custom use |
| `name` | Public | Public / general use |

- **Name.** `name` starts with a letter and ends with a letter or digit; inner characters are letters, digits or `_`. The four underscore classes use lowercase ASCII only; Public keys may use upper case.
- **Classification.** Count the underscores before and after `name`: 2/2 Global, 1/1 Scoped, 2/0 Limited, 1/0 Custom, 0/0 Public. Any other form (`__x_`, `___x`, `_x__`, `x_`) is diagnosed (`CANOR_ERR_ARGS`), never ignored.
- **Case.** Keys compare case-insensitively, because INI parsers differ. Two keys in one section that differ only in case are diagnosed.
- **Locales.** Names are never translated. A Global key must have the same value, in the same section, in every locale's `.ini`; a mismatch is diagnosed. The other classes may differ per locale.
- **Comments.** Whole lines starting with `;`. `#` and comments after a value aren't portable and are not used.
- The existing keys (`CanonicalSection`, `Locale`, `Scope`, `Projects`, `SupportedByDefault`) are Public keys.

## Status codes (unified)
`CANOR_OK 0, CANOR_ERR_ARGS -1, CANOR_ERR_RANGE -2, CANOR_ERR_FULL -3, CANOR_ERR_NOT_CLOSED -4, CANOR_ERR_UNDEFINED -5, CANOR_ERR_NOT_FOUND -6`
(Unified error and diagnostics family.)

## Flow / MetaFlow
- **AtomUsage** = (atom name, ring index ∈ ℕ). An omitted index means 0.
- **Flow** = length n plus a total function over `Index(n) = {0..n-1}`. Immutable. Duplicates are distinct positions.
- **Trifecta** = an ordered triple of atom names.
- **Order** = a bijection on `Index(n)`. The default is the identity (L→R). Its reverse is R→L.
- **Count stream** = n + 1 vectors per trifecta.
- **MetaFlow** ≤ 4 atoms. Default labels **PINNED**: `R` Reals, `G` Gaps, `B` Bases, `A` Auxiliaries.

## Timing fields (Aether §9)
`canor_field3_t[3][3][3]`, indexed by `PRE/PERI/POST × MIN/NOM/MAX × LOW/VALUE/HIGH`. Components are integers or closed `LO/HI` integer intervals. The flowrate metric is ops/sec. Every field carries `UPDATE_RATE` and `ERROR_RATE`.

## Aether IR
An integer-only binary tree: atoms (an integer value) and cells (ordered left/right). Required storage is a multidimensional integer array. Sparse matrices and HaPPY mapping are future work.

## Type tower (`canor_core.h`)
`BASIS, NUM, NUMBER, INT, TOKEN, VALUE, REFERENCE, FUNCTIONAL, OBSERVABLE, EXPR, OP, OPERATOR, DATA, REAL, INTERVAL, IMAG, IMAGINARY, BASUM, BASE, MODULATION, CALCULUS, TENSORIAL, TENSOR, UNIT, ATOM, CHANGE, FORCE, EXPRESSION, BBST, TST, NODE, LEAF`

## Capacities (defaults, `#ifndef`-overridable)
| Macro | Default | Meaning |
|---|---|---|
| `CANOR_LEVELS_MIN/MAX` | 6 / 96 | Factoradic levels (Lehmer bytes) |
| `CANOR_NUMBER_DIGITS` | 12 | Rich factoradic digits |
| `CANOR_IMAG_TRIFECTAS` | 5 | Default trifectas: 5 (16 basis elements, sedenion closure). Profile-configurable (e.g., 4 quaternion sub-algebras). |
| `CANOR_BASE_MAX` | 8 | |
| `CANOR_TENSORIAL_MAX` | 4 | |
| `CANOR_TENSOR_RANK_MAX` | 4 | |
| `CANOR_UNIT_MAX` | 2 | |
| `CANOR_IDENT_MAX` | 32 | |
| `CANOR_BBST_MAX` | 64 | |
| `CANOR_HV_BITS` | 1024 | Hypervector bits (16 × `uint64_t`) |
| `CANOR_ONE` | 1000 | Fixed-point one |

## IMAG basis — **PINNED**
`[one=0, (i,j,k)=1..3, (m,n,o)=4..6, (r,s,t)=7..9, (e,f,g)=10..12, (x,y,z)=13..15]` (5 trifectas = 16 basis elements, sedenions).
*Sub-algebra Profiles*: Quaternion sub-algebras (e.g. 4 trifectas with modular or disjoint quaternion products) are supported as profile configurations.

## FlowValue bit encoding — **PINNED**
| Bits | Value |
|---|---|
| `00` | NegZero |
| `01` | NegOne |
| `10` | PosZero |
| `11` | PosOne |

## CanorDB structures (draft)
1. Hypercomplex hypersurreal numbers. 2. Pure tensors (tensor-network contractions, HaPPY codes). 3. Aether Propositum structures. 4. Symbolicals (expressions and equations). X. Canorum (Canor-specific structures and protocols).

## Grammar
`atoms.ebnf` (60 productions, Appendix A of Working Draft v1) is located at [CanorLinguae/grammar/atoms.ebnf](CanorLinguae/grammar/atoms.ebnf).

## Interchange: United™
A `.fwo` carries distinct JSON and XML artifacts, embedded or referenced by path plus checksum. The canonical digest covers the body, ordered manifest, payloads, and checksums. TODO: digest algorithm (profile-defined).
