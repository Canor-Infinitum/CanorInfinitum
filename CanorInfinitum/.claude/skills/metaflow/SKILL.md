---
name: metaflow
description: Create a new Canor MetaFlow, the third CanorLinguae interoperability language (Canor §3). A MetaFlow is a Flow-like string of at most four independent, indexed or labeled channels, with pinned default labels R Reals, G Gaps, B Bases, A Auxiliaries, declared in a .fwh and invoked in a .fwb. Use when the user invokes /metaflow, or asks to create a MetaFlow, channels, or R/G/B/A slots.
---

# /metaflow: Create a new MetaFlow

Creates a MetaFlow of up to four channels inside the project workspace and checks it against the four-slot bound and the pinned labels.

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

## Definition (Canor §3)

- Analogous in type and use to a Flow™ (§2): ordered, immutable, and duplicates count.
- **At most four atoms** in sequence. Each one is an independent channel and may be **indexed** or **labeled**.
- **Default labels (pinned):** `R` Reals, `G` Gaps, `B` Bases, `A` Auxiliaries, in that order.
- A project may supply other labels or interpretations **within the four-slot bound**. Project settings decide how later layers (Soul, Maneuver) read each slot.
- **Enumerated space:** the `4! = 24` permutations of `R, G, B, A`. The slot order is the MetaFlow permutation.
- **Invocation is a complex-fractal string,** slot for slot. All of it is exact integer arithmetic:

  | Slot | Meaning |
  |---|---|
  | `R` Reals | what remains of `[0, b^k]` |
  | `G` Gaps | the removed lengths `b^(k-1-j) × (b-1)^j` |
  | `B` Bases | the scaling base `b` and the imaginary base `e_u` |
  | `A` Auxiliaries | the generating map `z -> z**2 + c`, with `c ∈ span{1, e_u}` |

  - Depth `k` is the Strategy's Order, and `e_u` is its differential.
  - The default base is `b = 3` (the trifecta arity), which the profile can override with `CANOR_ENUM_FRACTAL_BASE`.
  - Unrank with `canor_enum_invocation_unrank`.

## Inputs

| Input | Default |
|---|---|
| MetaFlow name (`NAME`) | none, so ask |
| Channel atoms (1 to 4) | `R, G, B, A` |
| Per channel: label or ring index | label from the default sequence |
| Channel interpretations | `unset` (settings-defined, open item) |
| Flow this MetaFlow annotates | none |

## Procedure

1. **Build the channel table:** position, atom, ring index (omitted → `0`), label, interpretation.
2. **Validate by property:**

   | Property | On failure |
   |---|---|
   | Channel count ≤ 4 | `CANOR_ERR_FULL`. Report all channels; never truncate. |
   | Default labels are exactly `R, G, B, A`, in order | `CANOR_ERR_ARGS` |
   | Custom labels don't redefine the pinned defaults | Stop. A redefinition needs explicit approval and a decision-log entry. |
   | Each ring index is a nonnegative integer | `CANOR_ERR_RANGE` |
   | A referenced Flow exists | `CANOR_ERR_NOT_FOUND` |

3. **Write the header** `.<project>/metaflow/<name>.fwh`: provenance, labels, interpretations. Custom interpretations go into a `.fws` as settings, not into the source.
4. **Write the body** `.<project>/metaflow/<name>.fwb`: the MetaFlow invocation.
5. **Never modify** the Flow it annotates. Only reference it.

### Example

Uses the proposed default grammar semantics. The active profile overrides it.

`.<project>/metaflow/rgba.fwh`

```
@provenance(spec="Canor §3", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
@channel.labels{R, G, B, A};
@channel.interpretation(R=unset, G=unset, B=unset, A=unset);
```

`.<project>/metaflow/rgba.fwb`

```
@metaflow.rgba(flow=@flow.demo):"R, G, B, A";
```

## Done when

- [ ] The workspace exists. `.fwh` and `.fwb` are written, and provenance is recorded.
- [ ] The channel table is shown: at most 4 channels, labels checked against the pinned defaults.
- [ ] Interpretations are recorded as settings or listed as open items.

**Next:** `/soul` (Strategy + MetaFlow + Action), or `/action`.
