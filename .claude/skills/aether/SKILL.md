---
name: aether
description: Canor Aether™ Intermediate Representation & Library Linker, the tenth CanorLinguae interoperability language (Canor §10). It is the integer-only binary-tree IR, plus the strict timing library and ε-error mitigation tool that runs through all assembly and compilation, with PRE/PERI/POST × MIN/NOM/MAX × LOW/VALUE/HIGH latency and jitter fields, ops/sec flowrate, UPDATE_RATE and ERROR_RATE. It starts with a Q&A that asks the User / App / Agent their intent (minimize error, read the error log, project status, build an IR) before acting. Use when the user invokes /aether, or asks about the IR, the IR log, the timing library, ε-error, or error mitigation.
---

# /aether: Aether™ Intermediate Representation

Reads, explains and reduces the project's errors through the Intermediate Representation log, and builds integer-only IR trees and timing fields.

## Prerequisite: Canor™ workspace

Do this before anything else. Every artifact this skill writes goes into a workspace.

1. **Find the workspace.** It's a top-level folder named `.<project>` that holds `<project>.fwp`. Skip tool folders (`.git`, `.github`, `.claude`, `.agentic`, `.vscode`, `.idea`).
   - **Exactly one found:** use it, and name it in one line.
   - **Several found:** ask which one.
   - **None found:** ask the user to name the project, then create it (step 2).
2. **Create the workspace** (only when none exists, or the user asks for a new one):
   - The name must be a grammar `NAME`: it starts with a letter or `_`, then letters, digits, `_`, `-`. Otherwise report `CANOR_ERR_ARGS` and ask again.
   - Create `.<project>/`, then the profile `.<project>/<project>.fwp`, then `.<project>/default.fws`.
   - Add more Canor Settings as separately named `.fws` files (`custom.fws`, `database.fws`, …). List every one in the profile's `@settings{…}`.
3. **Never overwrite** an existing `.fwp` or `.fws`. Append the new declaration and show the user the added lines.
4. **Keep compiled output out of version control.** `.fwo` objects and executables are build products.

Starter profile, `.<project>/<project>.fwp`. Paths are relative to the repository root. `unset` means the profile hasn't chosen a value yet, which is a valid state, not an error.

```
@project(name="<project>", version="0.1.0", license="AGPL-3.0-or-later", locale=en);
@provenance(source="Canor_Working-Draft_v1", author="<author>", date="<YYYY-MM-DD>");
@grammar(path="CanorLinguae/grammar/atoms.ebnf", productions=60, override=off);
@settings{".<project>/default.fws"};
@digest(algorithm=unset, covers={grammar, settings});
@complementary:none;
```

Starter settings, `.<project>/default.fws`. By default nothing has effects: no CanorDB access, no runtime dispatch.

```
@contract.default(effect=none, canordb=none, runtime=none);
@contract.provenance{profile, settings, version, digest};
```

If the repository already has a profile style (e.g. a `default.fwp` with `settings/canor_*.fws`), follow it instead of the starter.

### Canor™ files

| Extension | Definition |
|---|---|
| `.ini` | Protocol-wide settings |
| `.fwp` | Project-wide profile |
| `.fws` | Project-wide settings |
| `.fwh` | Source header |
| `.fwb` | Source body |
| `.fwo` | Compiled object |

Each language keeps its sources in `.<project>/<language>/`: declarations in `<name>.fwh`, invocations and bodies in `<name>.fwb`.

**`.ini` keys** follow the pinned INI variable classes. Use them when reading a locale `.ini`; never invent a key:

| Form | Class |
|---|---|
| `__name__` | Global |
| `_name_` | Scoped |
| `__name` | Limited |
| `_name` | Custom |
| `name` | Public |

- Any other underscore pattern is `CANOR_ERR_ARGS`.
- Keys compare case-insensitively.
- Names are never translated.
- Comments are whole lines starting with `;`.

### Keyword map

| Keywords | Relation to the profile |
|---|---|
| Strategy, Flow, MetaFlow, (Meta)Flow | Strategies define differential imaginary bases (`di … dg`, each an Ore extension), in a source header or a source body. A **Flow** is any string of atomic trifectas (imaginary-base atoms): its defining trifecta is declared in a `.fwh`, its invocations are in a `.fwb`. A **MetaFlow** is any string of at most four independent channels (default `R, G, B, A`), stored the same way. |
| Soul, Action | A **Soul Symbol** is the realification or complexification of Strategy + Flow, Strategy + MetaFlow + Action, or Flow + Action, into one of the representations Elementary, Unique, Composite or Unknown. An **Action** is a modulation: a `(mod N)` equation, written `L = R (mod N)`. |
| Maneuver | Strategies, Flows, MetaFlows, Soul Symbols and/or Actions, attached to a functional or symbolic equation or a symbolic expression. |
| United, Interchange, Meta-Schema | All of the above, defined in a Meta-Schema, plus numeric and/or logical arithmetic between the parts. Often datamined by machine learning, which yields an ε-error metric or allowance set by multilateral agreement. |
| Elite, Diffeomorphic Basis, Quantum-Linguistics | One such multilateral agreement: a shared common-ground basis (the Diffeomorphic Basis or Quantum-Linguistics) that enacts the United Meta-Schema. On completion it's assembled into a program, with Assembly as the meta-language, optionally with more United Meta-Schemas. |
| Follower, Interlingua | The project's Complementary Languages join the interoperability languages here. All earlier assemblies are compiled into one compiled object and/or executable binary. |
| Aether, Intermediate Representation, IR, Timing Library, ε-error, Error Mitigation | Across all assembly and compilation, a strict timing library (`canor_field3_t` fields) and ε-error mitigation tool runs to minimize errors and raise fault tolerance. |

### Rules every Canor skill keeps

- **Integer only.** Exact integers, closed `[LO, HI]` integer intervals, and fixed-point values over `CANOR_ONE = 1000`. Never use floating point.
- **Diagnose, never prune.** A problem gets a status code and stays in the report. Nothing is silently fixed or dropped.

  | Code | Value | Meaning |
  |---|---|---|
  | `CANOR_OK` | 0 | Accepted |
  | `CANOR_ERR_ARGS` | -1 | Malformed input or syntax |
  | `CANOR_ERR_RANGE` | -2 | Value out of range |
  | `CANOR_ERR_FULL` | -3 | Capacity exceeded |
  | `CANOR_ERR_NOT_CLOSED` | -4 | Unclosed group or interval |
  | `CANOR_ERR_UNDEFINED` | -5 | Undefined or profile-unset case |
  | `CANOR_ERR_NOT_FOUND` | -6 | Missing reference |

- **Assembly is side-effect-free.** Reading or writing CanorDB, or dispatching to a runtime, needs a contract in the `.fwp`/`.fws` that authorizes it. Without one, record the declared effect and don't perform it.
- **Name constants.** No unexplained magic numbers. The assembler and compiler track every named constant.
- **Pinned encodings are frozen:**
  - IMAG basis `[one=0, (i,j,k)=1..3, (l,m,n)=4..6, (o,p,q)=7..9, (r,s,t)=10..12, (e,f,g)=13..15]`, with differentials `di … dg` in the same order. Every trifecta `(a,b,c)` is one of Hamilton's quaternionic trifectas: `a·b = c`, `b·c = a`, `c·a = b`.
  - FlowValue `NegZero=0, NegOne=1, PosZero=2, PosOne=3`.
  - MetaFlow labels `R, G, B, A`.
- **Large spaces are index-addressed, never materialized.**

  | Space | Size |
  |---|---|
  | Strategy | 15 differentials × Orders 1..6 = 90 |
  | Flow | 6^5 = 7,776 |
  | MetaFlow | 4! = 24 |
  | Soul | 16,796,160 |
  | Action | 24,389,000 |
  | Maneuver | 409,641,546,240,000 |

  When someone asks for an enumeration, give the radix spec, the counts, the first and last samples, and gap records, not every row.
- **Don't invent profile-defined values.** Leave the digest algorithm, the grayscale conversion, the dispatch rules and the like as explicit **open items**.
- **Record provenance** in every `.fwh`: spec section, profile and settings versions, author, date.

## Definition (Canor §10)

- **Role.** CanorLinguae's intermediate representation, and its timing-library linkage layer. A strict timing library and ε-error mitigation tool runs through all assembly and compilation, to minimize errors and raise fault tolerance.
- **Base value: an integer-only binary tree.**
  - **Atoms** carry an integer value.
  - **Cells** keep an ordered left and right branch.
  - There are no floating-point, string or other non-integer scalar nodes.
  - Paths and branch relationships stay addressable, for deterministic replay.
- **Required storage:** a multidimensional integer array. Nested dimensions and indices encode both the values and the topology.
- **Future extensions,** not available yet: sparse matrices, Unimodular Sparse Matrix Field conformance, HaPPY-style holographic mapping.
- **Which events get timing fields:** every signaling Maneuver™, Elite™ branch traversal, Follower™ runtime occurrence and CanorDB mutation has a latency field and a jitter field.
- **Timing field layout (`canor_field3_t`, `canor_timing.h`).** Each field is `3 × 3 × 3`: `PRE/PERI/POST × MIN/NOM/MAX × LOW/VALUE/HIGH`.
  - The property type is fixed at compile time: `CANOR_TIMING_INTERVAL` 0 gives 27 integers, 1 gives 54 interval bounds.
  - Latency is ops/sec per sample. Jitter is the absolute difference between consecutive samples. `LOW/VALUE/HIGH` is the ±1-tick quantization enclosure.
  - Components are integers, or closed `[LO, HI]` integer intervals.
  - The flowrate metric is **ops/sec**.
  - Every field carries `UPDATE_RATE` and `ERROR_RATE`.
  - The rate-score rule and interval aggregation are settings-dependent (open item).
- **Goal:** ε-error minimization within ω-number (Chaitin) maximization. That means raising `UPDATE_RATE` while lowering `ERROR_RATE`.

## Step 1: Ask for intent

Before doing anything else, greet the User / App / Agent **confidently and compassionately** and ask what they want. Use `AskUserQuestion` with these four options; adapt the wording but keep the substance:

1. **Minimize error.** "Do you wish to minimize error? I can read the Intermediate Representation log for you."
2. **Read the error log.** "Do you wish to read the error log? I can show the Intermediate Representation log to you."
3. **Project status.** "Do you wish to know the status of the project? All is representable in the Intermediate Representation log."
4. **Build an IR or timing field.** "Do you wish to represent an artifact as an Aether tree, or record a latency or jitter field?"

"Other" covers anything else, such as linking a timing library or inspecting one branch. If the request already states the intent, confirm it in one line and continue; don't ask again.

## Step 2: Gather the IR log

There's no single log file. Gather the sources below, and say plainly which were found and which were missing:

| Source | Where |
|---|---|
| Per-artifact diagnostics | `.<project>/**/*.fwh`, `*.fwb` |
| Elite governance records | `.<project>/elite/*.record.json` |
| Follower compile errors | `.<project>/follower/*.errors.json` |
| United manifest and digest diagnostics | `.<project>/united/*.fwb` |
| Profile open items (`unset` values) | `.<project>/*.fwp`, `*.fws` |
| Build, test and lint output | the repository's documented build, test and lint commands. Run them only for intents 1 and 3, and say that you are running them. |
| Version-control state | `git status`, `git log -10` |

## Step 3: Act on the intent

- **1. Minimize error.**
  - Rank diagnostics by layer (Flow → Follower), then by status code.
  - For each one, say what raises `ERROR_RATE` and propose the smallest fix.
  - Apply a fix only after the user approves it.
  - Never change a pinned encoding without explicit approval.
- **2. Read the error log.**
  - Show every entry as a table: source, layer, code (`CANOR_*`), message, location.
  - Keep every entry. Nothing is pruned or summarized away.
- **3. Project status.** For each language 1–9, give:
  - the artifacts present;
  - clean / diagnosed / incomplete counts;
  - open items.

  Then add the build, test and lint results, and the profile's `unset` values.
- **4. Build an IR or timing field.**
  - **Tree:** encode the artifact as an integer binary tree of atoms and cells. Give its multidimensional-array form, then verify the round trip: array → tree → array, identical.
  - **Timing field:** fill the `3 × 3 × 3` grid with integers or `[LO, HI]` intervals, plus `UPDATE_RATE`, `ERROR_RATE` and ops/sec. Check every interval satisfies `LO ≤ HI`; otherwise report `CANOR_ERR_RANGE`.
  - **Fractions:** if a value would need a fraction, scale it over `CANOR_ONE = 1000` and say that you did.
  - **Write** the output to `.<project>/aether/<name>.fwh` (declarations, provenance) and `<name>.fwb` (the array and fields).

### Example (intent 4)

The tree `cell(atom 3, cell(atom 1, atom 4))` has this array form: rows of `[kind, value, left, right]`, where `kind` is 0 for an atom and 1 for a cell, and `-1` marks no child.

```
[[1, 0, 1, 2],
 [0, 3, -1, -1],
 [1, 0, 3, 4],
 [0, 1, -1, -1],
 [0, 4, -1, -1]]
```

## Done when

- [ ] The workspace exists. The intent was asked or confirmed, and then answered.
- [ ] Every log source is reported as found or missing. No entry was dropped.
- [ ] All values are integers. Future extensions and missing schemas are marked as open items.
- [ ] Any file written records its provenance.
