---
name: follower
description: Create a new Canor Follower™ Interlingua, the ninth CanorLinguae interoperability language (Canor §9). It declares the project's Complementary Languages in the .fwp, binds their versions, compiler, assembler, runtime, environment, constraints and contracts in a .fws, and compiles every earlier assembly (Flow, MetaFlow, Action, Soul, Maneuver, United, Elite) into one compiled object and/or executable binary. Use when the user invokes /follower, or asks for a Follower Interlingua, an interlingua, a compilation setup, or a Complementary Language plug-in.
---

# /follower: Create a new Follower™ Interlingua

Declares and binds the project's Complementary Languages, checks the toolchains, and produces a compilation plan for every earlier artifact. On failure it hands an error report to `/aether`.

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

## Definition (Canor §9)

- **Role.** The main interoperability language of the suite. It gives plug-and-play linkage between CanorLinguae and the Complementary Languages the profile declares.
- **`.fwp`** declares each Complementary Language and its embedded code-block form.
- **`.fws`** binds each language's versions, compiler, assembler, runtime, environment, constraints and contracts.
- **Dispatch has runtime effects.** A block may read or update CanorDB **only** when its `.fwp`, `.fws` and active contracts authorize it. Branch identity, declared inputs and outputs, errors and permitted state transitions stay visible to assembly and replay.
- **Pipe-dot framing** marks embedded code regions. The enclosing United™, JSON or XML representation handles its own escaping and markup. United, JSON or XML may carry or describe the blocks.
- **Complementary Languages** commonly bound:
  - **Free Pascal / Lazarus:** algorithmic and declarative bindings.
  - **SWI-Prolog & C:** logical resolution and symbolic reasoning over the pure-C core.
  - **Rust FFI:** memory-safe systems bindings.

  Bindings live under `CanorLinguae/`, never in the C core.
- **Compilation** unites every earlier assembly into one compiled object (`.fwo`) and/or executable binary.
- **Not yet formalized by the spec:** the dispatch relation, and rollback/commit for failed CanorDB transitions. Always list both as open items.

## Inputs

| Input | Default |
|---|---|
| Interlingua name (`NAME`) | the project name |
| Artifacts to compile | every `.fwh` / `.fwb` in `.<project>/`, listed for confirmation |
| Complementary Languages | `none`. Common choices: Free Pascal / Lazarus, SWI-Prolog, Rust, C. |
| Per language: block form, version, compiler, assembler, runtime, environment, constraints, contracts | ask; any field left unanswered is an open item |
| Effect policy per block | `pure` |
| Output | compiled object (`.fwo`), executable binary, or both |

## Procedure

1. **Inventory** the workspace. List each earlier artifact with its language and its status, taken from its own report: clean, diagnosed or incomplete.
2. **Check toolchains** where it's cheap, read-only, and doesn't install anything:
   - `fpc -iV`
   - `swipl --version`
   - `cargo --version`
   - `gcc --version` or `clang --version`

   A missing tool is a diagnostic. Ask before installing anything.
3. **Validate by property:**

   | Property | On failure |
   |---|---|
   | Every artifact to compile exists | `CANOR_ERR_NOT_FOUND` |
   | Every declared language has a complete `.fws` binding | `CANOR_ERR_UNDEFINED` per missing field |
   | Each pipe-dot block opens and closes | `CANOR_ERR_NOT_CLOSED` |
   | Each effectful block names an authorizing contract | `CANOR_ERR_UNDEFINED`. Never assume authorization. |
   | Elite input comes from an `accepted` basis | compile, but mark the output `incomplete` |

4. **Show the compilation plan:** inputs → per-language blocks → outputs, with an effect column.
5. **Append to the profile** (after the user approves, showing the added lines):
   - the `.fwp`: each `@complementary` declaration, replacing `@complementary:none`;
   - a new `.<project>/follower.fws`: the bindings. Register it in `@settings{…}`.
6. **Write** the plan to `.<project>/follower/<name>.fwh` (declarations, provenance) and `<name>.fwb` (the compilation units, in order).
7. **Compile** only when the user asks, and only with tools that are present. On success, report the `.fwo` / binary path; don't commit it. On failure, write `.<project>/follower/<name>.errors.json`, then offer `/aether` to read it.

### Example

Uses the proposed default grammar semantics. The active profile overrides it.

`.fwp` addition

```
@complementary{(lang=swipl, block="| ... ."), (lang=rust, block="| ... .")};
@settings{".<project>/default.fws", ".<project>/follower.fws"};
```

`.<project>/follower.fws`

```
@bind.swipl(version=unset, runtime=swipl, environment=host, constraints=none, contract=pure);
@bind.rust(version=unset, compiler=cargo, environment=host, constraints=none, contract=pure);
@dispatch:unset;
@rollback:unset;
```

## Done when

- [ ] The workspace exists. `.fwp` / `.fws` additions are shown and approved. `.fwh` and `.fwb` are written, and provenance is recorded.
- [ ] Every Complementary Language is declared and bound, or its gaps are listed.
- [ ] Every effect is tied to a contract. Dispatch and rollback/commit are listed as open items.
- [ ] The output is either a compiled object / binary, or an error report handed to `/aether`.

**Next:** `/aether` for the IR log, timing and ε-error mitigation.
