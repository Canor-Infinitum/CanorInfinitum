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

## Status codes (unified)
`CANOR_OK 0, CANOR_ERR_ARGS -1, CANOR_ERR_RANGE -2, CANOR_ERR_FULL -3, CANOR_ERR_NOT_CLOSED -4, CANOR_ERR_UNDEFINED -5, CANOR_ERR_NOT_FOUND -6`
(Replaces `fc_status` and `VI_OK/VI_ERR/VI_NOT_FOUND/VI_FULL`.)

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

## Type tower (from legacy `fc.h`, now `canor_core.h`)
`BASIS, NUM, NUMBER, INT, TOKEN, VALUE, REFERENCE, FUNCTIONAL, OBSERVABLE, EXPR, OP, OPERATOR, DATA, REAL, INTERVAL, IMAG, IMAGINARY, BASUM, BASE, MODULATION, CALCULUS, TENSORIAL, TENSOR, UNIT, ATOM, CHANGE, FORCE, EXPRESSION, BBST, TST, NODE, LEAF`

## Capacities (defaults, `#ifndef`-overridable)
| Macro | Default | Meaning |
|---|---|---|
| `CANOR_LEVELS_MIN/MAX` | 6 / 96 | Factoradic levels (Lehmer bytes) |
| `CANOR_NUMBER_DIGITS` | 12 | Rich factoradic digits |
| `CANOR_IMAG_TRIFECTAS` | 5 | → 16 basis elements (**PINNED**: 4 is refused) |
| `CANOR_BASE_MAX` | 8 | |
| `CANOR_TENSORIAL_MAX` | 4 | |
| `CANOR_TENSOR_RANK_MAX` | 4 | |
| `CANOR_UNIT_MAX` | 2 | |
| `CANOR_IDENT_MAX` | 32 | |
| `CANOR_BBST_MAX` | 64 | |
| `CANOR_HV_BITS` | 1024 | Hypervector bits (16 × `uint64_t`) |
| `CANOR_ONE` | 1000 | Fixed-point one |

## IMAG basis — **PINNED**
`[one=0, (i,j,k)=1..3, (m,n,o)=4..6, (r,s,t)=7..9, (e,f,g)=10..12, (5th trifecta)=13..15]`. TODO: name the 5th trifecta.

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
`atoms.ebnf`, 60 productions, in draft Appendix A. TODO: copy it to `CanorLinguae/grammar/atoms.ebnf`.

## Interchange: United™
A `.fwo` carries distinct JSON and XML artifacts, embedded or referenced by path plus checksum. The canonical digest covers the body, ordered manifest, payloads, and checksums. TODO: digest algorithm (profile-defined).
