# Canor-Follow-Redko-Avec-Fengshui

**Canor Infinitum: Quantum Gravity Turing-Computing & Communication Theory (QGTC/QGCT)**
*Follow™ · Redko™ · Avec™ · Fengshui™ · Canor™* (Working Draft v1.0.0)

A pure-C, integer-only implementation of the **CanorLinguae** language suite, its database **CanorDatabasis / CanorDB**, the **CarmenInstinctus** plug-and-play QG/ToE model layer, and their combination in **CanorInstinctivus**. It targets Zephyr RTOS and sandboxed native sessions, uses Assembly as the meta-language, and provides complementary-language bindings (FPC/Lazarus, SWI-Prolog & C, Rust FFI). In-browser output (WASM, WebM, JSON/XML/HTML/CSS/JS) is optional.

## QGTC/QGCT: Language, Database, Instinct, Instinctual
| # | Folder | Role |
|---|---|---|
| 1 | [CanorLinguae/](CanorLinguae/README.md) | **QGTC**: a reactive, quantum symbolic and physical, multi-meta-programming language suite |
| 2 | [CanorDatabasis/](CanorDatabasis/README.md) | **QGCT** (CanorDB): isomorphic to QGTC. Repository of all Quantum Gravity Communicational Contexts and Invariants |
| 3 | [CarmenInstinctus/](CarmenInstinctus/README.md) | **QG/ToE plug-and-play**: Discrete Quantum Relativity Theory Extension (Q-RT) with multiform Unruh-Lorentzian frames |
| 4 | [CanorInstinctivus/](CanorInstinctivus/README.md) | **Canor-specific theory and practice**: combines 1–3 with Canor™ theories and practices |

## Projects: Lingua, Repositorium, Infinitum ([Canor/](Canor/README.md))
| | Folder | Role |
|---|---|---|
| A | [Canor/CanorLingua/](Canor/CanorLingua/README.md) | All quantum-linguistic work: natural language processing |
| B | [Canor/CanorRepositorium/](Canor/CanorRepositorium/README.md) | All CanorInstinctivus storage, learning, and runtime |
| C | [Canor/CanorInfinitum/](Canor/CanorInfinitum/README.md) | All CanorInstinctivus hardware, software, and technology |

## Locales: UN alignment
| | Mark | Locale |
|---|---|---|
| I | Follow™ | English (**primary**) |
| II | Redko™ | Russian |
| III | Avec™ | French |
| IV | Fengshui™ | Traditional Chinese |
| V | Canor™ | Latin |

## Toolchains and build
See [TOOLS.md](TOOLS.md) and [WORKFLOW.md](WORKFLOW.md).
```sh
# TODO: replace once the build system is chosen (CMake / west / make)
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

## Documentation index
| File | Purpose |
|---|---|
| [AGENTS.md](AGENTS.md) | Rules for any AI coding agent |
| [CLAUDE.md](CLAUDE.md) | Claude Code-specific instructions |
| [INSTRUCTIONS.md](INSTRUCTIONS.md) | GitHub Copilot instructions (mirrored to [.github/copilot-instructions.md](.github/copilot-instructions.md)) |
| [SKILL.md](SKILL.md) | Agent skill: add or port a `canor_*` module |
| [MEMORY.md](MEMORY.md) | Decision log and open questions |
| [DESIGN.md](DESIGN.md) | Architecture |
| [TOOLS.md](TOOLS.md) | Toolchain setup |
| [PROMPTS.md](PROMPTS.md) | Reusable prompt templates |
| [WORKFLOW.md](WORKFLOW.md) | Branching, build, test, CI, and git sync |
| [SCHEMA.md](SCHEMA.md) | Types, encodings, file extensions, and interchange |
| [CONTEXT.md](CONTEXT.md) | Glossary, locales, and source documents |

## Sources
- `D:\Workspace\Canor_Working-Draft_v1.pdf`: **primary** specification
- `D:\Workspace\CanorLinguae_Conceptual_Design_Draft_with_panels.pdf`
- `D:\Workspace\CanorLinguae_Z_Notation_Specification_Draft.pdf`
- `D:\Workspace\.old\follow_core-verifiable_intelligence\`: legacy C (`fc_*`/`vi_*`), being migrated to `canor_*`. Read-only.

## License
GNU AGPL v3.0-or-later. TODO: add `LICENSE`.
