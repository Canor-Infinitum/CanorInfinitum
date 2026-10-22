# CONTEXT.md: Project Glossary and Specification Context

## Project Overview

**Canor Infinitum** is a deterministic, integer-only systems implementation of Quantum Gravity Turing-Computing & Communication Theory (QGTC/QGCT). Written in pure C11, it scales seamlessly from resource-constrained Zephyr microcontrollers to sandboxed native host sessions and client web browsers.

## Locales: UN Alignment (Security Council & Member States)

| | Mark | Language | Locale Tag | Status |
|---|---|---|---|---|
| I | **Follow™** | English | `en` | **Primary specification locale** |
| II | **Redko™** | Russian | `ru` | Supported |
| III | **Avec™** | French | `fr` | Supported |
| IV | **Fengshui™** | Traditional Chinese | `zh-Hant` | Supported |
| V | **Canor™** | Latin | `la` | Supported |

## Domain Glossary

| Term | Architectural Definition |
|---|---|
| **QGTC** | **Quantum Gravity Turing-Computing:** The formal computational paradigm realized by the CanorLinguae language suite. |
| **QGCT** | **Quantum Gravity Communication Theory:** The formal relational paradigm realized by CanorDatabasis (CanorDB), mathematically isomorphic to QGTC. |
| **CanorLinguae** | The layered nine-language interoperability suite: Flow, MetaFlow, Action, Soul, Maneuver, United, Elite, Carmen Interlingua, and Aether IR. |
| **CanorDatabasis / CanorDB** | The persistent database repository storing communication contexts, invariant boundaries, and structured tensor states. |
| **CarmenInstinctus** | The physical model layer implementing discrete quantum relativity (Q-RT) and multiform Unruh-Lorentzian frames. |
| **CanorInstinctivus** | The unified framework combining CanorLinguae, CanorDB, and CarmenInstinctus into concrete computational applications. |
| **CanorLingua / Repositorium / Infinitum** | Subprojects A, B, and C: Natural language processing / storage and learning runtime / hardware integration and core algorithms. |
| **Flow™** | An immutable, ordered sequence of imaginary-base atoms. |
| **Follow™** | The English locale mark of the overall protocol / project name. |
| **Trifecta** | An ordered triple of atom names (e.g. `i,j,k`) forming a Hamilton-style imaginary basis. |
| **Soul™ Symbol** | An executable state representation produced by traversing a Flow under modular Action constraints. |
| **United™** | The standard interchange language and `.fwo` compiled-object container. |
| **Elite™** | A side-effect-free, branch-retaining pattern assembler using Assembly as its meta-language. |
| **Carmen™ Interlingua** | The primary interoperability layer providing bindings to complementary languages (English locale mark: **Follower™**). |
| **Aether™** | The integer-only binary-tree Intermediate Representation (IR) and real-time timing library linker. |
| **Q-RT** | Discrete Quantum Relativity Theory Extension. |
| **DQG** | Diffeomorphic Quantum Gravity. |
| **ops/sec, UPDATE_RATE, ERROR_RATE** | Real-time performance metrics targeting ε-error minimization within ω-number (Chaitin) throughput maximization. |
| **Lehmer Code** | Factoradic permutation address encoding permutation rank without dynamic memory allocation. |

## Authoritative Specifications

- **Primary Authority:** This repository's Markdown documentation ([SCHEMA.md](SCHEMA.md), [AGENTS.md](AGENTS.md), [MEMORY.md](MEMORY.md), and this file). Language section labels (§1–§9) follow Canor Working Draft v1.
- **Historical Reference:** PDF design drafts (Working Draft v1, Conceptual Design, Z-Notation). All binding architectural decisions are recorded in [MEMORY.md](MEMORY.md).

## Implementation Status

The C11 core library builds with CMake (`-Wall -Wextra -Werror -pedantic`), and its ctest suite passes on the host. The legacy modules were ported to integer-only, fixed-capacity form on 2026-10-22. Open work, tracked in [MEMORY.md](MEMORY.md): moving `canor_demo` to the ported APIs, renaming the remaining non-`canor_*` identifiers in `canor_golay` and `canor_fw`, the profile-defined digest and signature algorithms, complementary-language bindings, and Zephyr RTOS board targets.
