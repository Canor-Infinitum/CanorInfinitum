---
name: maneuver
description: Create a new Canor Maneuver™ Group, the sixth CanorLinguae interoperability language (Canor §6). A Maneuver Group attaches Strategies, Flows, MetaFlows, Soul Symbols and/or Actions to functional or symbolic equations or expressions, with declared input/output domains, arities, one-to-one / one-to-many / many-to-one junctions, and a shared effect policy. Use when the user invokes /maneuver, or asks to create a Maneuver or a Maneuver Group.
---

# /maneuver: Create a new Maneuver™ Group

Creates a group of bindings inside the project workspace. Checks every arity and port, and keeps the group pure unless a contract authorizes an effect.

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

## Definition (Canor §6)

- **Two-sided binding.**
  - One side is an executable **Soul™ Symbol**, or another earlier form: Strategy, Flow, MetaFlow, Action.
  - The other side is a functional or symbolic **equation or expression**: braces, brackets, parentheses, bra-ket, infix operators, invariants, variables, constants.
- **Each binding declares** input and output domains, arities, and a junction: one-to-one, one-to-many or many-to-one.
- **Assembly checks** the declared arities and connected ports, and reports each mismatch as a diagnostic. Assembly **never mutates** CanorDB or external state.
- **Pure Maneuvers are the default.** A runtime may read or update CanorDB only when the active `.fwp` and `.fws` contracts authorize it.
- **Equality is extensional.** Under the same profile and input domain, two Maneuvers are equal when their observable results, errors and authorized state transitions agree.
- **A Maneuver Group** collects related bindings under one shared effect policy.
- **Default Maneuver:** a Soul Symbol bound one-to-one to its Action, which maps the Soul's count stream to canonical residues mod `o`. When `o = 0` the Maneuver stays `CANOR_ERR_UNDEFINED`. Assignment defaults to `unassigned`, and effects to none.
- **Enumerated space:** Soul × Action = 16,796,160 × 24,389,000 = 409,641,546,240,000 Maneuvers.
- **Grouping symbols:**
  - `( )` symmetric
  - `[ ]` antisymmetric
  - `{ }` unknown symmetry
  - `< | >` bra-ket, for tensors only

## Inputs

| Input | Default |
|---|---|
| Group name (`NAME`) | none, so ask |
| Per binding: source artifact, expression, input domain, output domain, arity, junction | none, so ask |
| Group effect policy | `pure` |
| Authorizing contract (only when the group has effects) | none, which means the effect is not authorized |
| Status | `unassigned`. Others: `assigned`, `tasked`, `scheduled`. |

## Procedure

1. **Resolve** every source artifact in `.<project>/`. A missing one is `CANOR_ERR_NOT_FOUND`.
2. **Parse** each expression as a `SYMBOLIC_ATOM`. Report the first failure as `CANOR_ERR_ARGS` and any unclosed group as `CANOR_ERR_NOT_CLOSED`.
3. **Validate by property:**

   | Property | On failure |
   |---|---|
   | Arity matches the junction shape (1→1, 1→k, k→1) | diagnostic, kept on the binding |
   | Every port is connected to a declared domain | `CANOR_ERR_NOT_FOUND` |
   | Every constant is named | Flag it. Name it before writing. |
   | An effectful group names an authorizing `.fws` contract | `CANOR_ERR_UNDEFINED`. Never assume authorization. |
   | Bra-ket is used only for tensors | `CANOR_ERR_ARGS` |

4. **Show the binding table:** source, expression, in → out, arity, junction, effect, diagnostics.
5. **Write the header** `.<project>/maneuver/<group>.fwh`: provenance, domains, junctions, effect policy, status.
6. **Write the body** `.<project>/maneuver/<group>.fwb`: the bindings.
7. **For effects,** append the contract to a `.fws` only after the user approves it. Show the added lines.

### Example

Uses the proposed default grammar semantics. The active profile overrides it.

`.<project>/maneuver/kinetics.fwh`

```
@provenance(spec="Canor §6", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
@maneuver.kinetics(effect=pure, status=unassigned);
@junction.kinetics.demo(in=N, out=N, arity=1, shape=one_to_one);
```

`.<project>/maneuver/kinetics.fwb`

```
@maneuver.kinetics.demo(~soul.demo):E := (1/2)*m*v^2;
```

Here `(1/2)` is exact only if the profile defines exact rational division. Otherwise it's `CANOR_ERR_UNDEFINED`. This is a deliberate example of a diagnostic that stays in the report.

## Done when

- [ ] The workspace exists. `.fwh` and `.fwb` are written, and provenance is recorded.
- [ ] Every binding has domains, arity, junction and effect.
- [ ] Every arity and port mismatch is reported. None was pruned.
- [ ] No effect is treated as authorized without a named contract.

**Next:** `/united` to define the Meta-Schema and its interchange.
