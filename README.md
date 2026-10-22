# Canor Infinitum

**A Pure-C, Integer-Only Systems Architecture for Quantum-Symbolic Computing and Communication**  
*Working Draft v1.0.0 (Locales: Follow™ · Redko™ · Avec™ · Fengshui™ · Canor™)*

**Canor Infinitum** is a deterministic, zero-allocation implementation of the **CanorLinguae** language suite, the **CanorDatabasis (CanorDB)** relational invariant store, and the **CarmenInstinctus** physical modeling layer, combined in **CanorInstinctivus**.

Designed for mission-critical embedded systems and high-assurance distributed environments, the framework executes natively on **Zephyr RTOS** (ARM Cortex-M, RISC-V) and in sandboxed native sessions (x86_64, AArch64). It employs low-level **Assembly as its meta-language** and provides plug-and-play bindings for **Free Pascal (FPC/Lazarus)**, **SWI-Prolog & C**, and **Rust FFI**. In-browser export (WASM, WebM, JSON/XML/HTML/CSS/JS) is available as an optional visualization and auditing layer.

## Subsystems: Language, Database, Model Layer, and Integration

| # | Subsystem | Architectural Role | Description |
|---|---|---|---|
| 1 | [CanorLinguae/](CanorLinguae/README.md) | **Language Suite (QGTC)** | A reactive, quantum-symbolic metaprogramming suite providing nine layered interoperability languages. |
| 2 | [CanorDatabasis/](CanorDatabasis/README.md) | **Database Layer (QGCT)** | The persistent invariant store isomorphic to CanorLinguae, maintaining communication contexts, state spaces, and tensor structures. |
| 3 | [CarmenInstinctus/](CarmenInstinctus/README.md) | **Physical Model Layer** | Plug-and-play discrete relativity and multiform Unruh-Lorentzian frame models for physical simulation. |
| 4 | [CanorInstinctivus/](CanorInstinctivus/README.md) | **Applied Integration** | Combines the language suite, database, and physical models into concrete operational workflows. |

## Subprojects: Lingua, Repositorium, Infinitum ([Canor/](Canor/README.md))

| | Project | Focus Area | Description |
|---|---|---|---|
| A | [Canor/CanorLingua/](Canor/CanorLingua/README.md) | Natural Language Processing | Lexical parsing, tokenization, and AST processing for the quantum-linguistic architecture. |
| B | [Canor/CanorRepositorium/](Canor/CanorRepositorium/README.md) | Storage & Machine Learning | FlowStore hash-to-content indexing, hypervector dictionaries, and learning runtime loops. |
| C | [Canor/CanorInfinitum/](Canor/CanorInfinitum/README.md) | Hardware & Core Algorithms | Unbounded integer arithmetic, factoradic/Lehmer indices, Golay coding, and bare-metal Zephyr drivers. |

## Locales: UN Alignment

| | Mark | Target Language | Status |
|---|---|---|---|
| I | **Follow™** | English | **Primary specification locale** |
| II | **Redko™** | Russian | Supported |
| III | **Avec™** | French | Supported |
| IV | **Fengshui™** | Traditional Chinese | Supported |
| V | **Canor™** | Latin | Supported |

## Toolchains and Build

See [TOOLS.md](TOOLS.md) and [WORKFLOW.md](WORKFLOW.md).

```sh
# Host build and test suite
cmake -S source -B build -G Ninja && cmake --build build && ctest --test-dir build --output-on-failure
```

## Documentation Index

| File | Purpose |
|---|---|
| [AGENTS.md](AGENTS.md) | Universal engineering rules and invariants for AI coding agents |
| [CLAUDE.md](CLAUDE.md) | Claude Code-specific tool and project workflows |
| [INSTRUCTIONS.md](INSTRUCTIONS.md) | GitHub Copilot instructions (mirrored to [.github/copilot-instructions.md](.github/copilot-instructions.md)) |
| [SKILL.md](SKILL.md) | Developer skill for implementing or porting a `canor_*` C module |
| [MEMORY.md](MEMORY.md) | Append-only architectural decision log and canonical module registry |
| [DESIGN.md](DESIGN.md) | Detailed system architecture and effect boundary models |
| [TOOLS.md](TOOLS.md) | Toolchain requirements and verification commands |
| [PROMPTS.md](PROMPTS.md) | Reusable prompt templates for development tasks |
| [WORKFLOW.md](WORKFLOW.md) | Branching strategy, build-test cycles, CI targets, and git synchronization |
| [SCHEMA.md](SCHEMA.md) | Pinned types, encodings, file extensions, and interchange formats |
| [CONTEXT.md](CONTEXT.md) | Domain glossary, UN locale tags, and project status |

## Specifications

The authoritative specification is this repository's Markdown documentation: [SCHEMA.md](SCHEMA.md), [AGENTS.md](AGENTS.md), [MEMORY.md](MEMORY.md), and [CONTEXT.md](CONTEXT.md). Section numbering (§0–§9) follows Canor Working Draft v1.

## License

Licensed under the [GNU Affero General Public License v3.0 or later](LICENSE).
