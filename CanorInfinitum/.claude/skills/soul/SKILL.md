---
name: soul
description: Create a new Canor Soul™ Symbol, the fifth CanorLinguae interoperability language (Canor §5). A Soul Symbol is the realification or complexification of Strategy + Flow, Strategy + MetaFlow + Action, or Flow + Action. It is produced by a traversal Order over an immutable Flow, emitting an n+1 count stream mapped to canonical residues. Use when the user invokes /soul, or asks to create a Soul Symbol, run a Flow traversal, or compute a count stream.
---

# /soul: Create a new Soul™ Symbol

Composes existing workspace artifacts into an executable Soul Symbol. Computes and verifies its count stream.

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

## Definition (Canor §5)

- **Compositions.** A Soul Symbol is the realification or complexification of one of:
  - Strategy + Flow
  - Strategy + MetaFlow + Action
  - Flow + Action
- **Traversal inputs:** an immutable Flow of length `n`, plus either a direction or an explicit **Order**.
- **Order** is a bijection on `Index(n)`: length `n`, containing each of `0 … n-1` exactly once.
  - When it's omitted, the direction decides: L→R is the identity, R→L is its reverse.
  - Middle-left-out, middle-right-out and other traversals are explicit permutations.
- **Count stream,** per registered trifecta:
  - The initial vector counts each of the trifecta's three names in the Flow.
  - Each consumed position **decrements** every component whose name matches. All other components are unchanged.
  - **Exactly `n + 1` vectors** come out: the initial vector plus one after each consumed position.
- **Action** maps each vector's components to **canonical residues** before grayscale conversion. The grayscale conversion is profile-defined.
- Atoms outside every trifecta go to the configured auxiliary-channel policy.
- **The Flow is never rewritten** by the traversal.
- **Representations.** Here Reals means count-stream steps and Bases means IMAG units.

  | Representation | Reals / Bases |
  |---|---|
  | Elementary | (Real, Base) |
  | Unique | (Real, Bases) |
  | Composite | (Reals, Base) |
  | Unknown | (Reals, Bases) |

- **Category = use.**

  | Category | Use |
  |---|---|
  | Storage | stored in a database or written to a `.fwb` (the default for generated symbols) |
  | Learning | used by a learning system |
  | Operation | used at runtime / I/O |

- **Enumerated space:** Strategy × Flow × MetaFlow = 90 × 7,776 × 24 = 16,796,160 Soul Symbols.
- **Reference implementation:** `canor_soul.h`. The `one` basis element goes to the auxiliary channel. Grayscale conversion and realification/complexification are profile-defined (`unset`).

## Inputs

| Input | Default |
|---|---|
| Symbol name (a `REGISTERED_SOUL_SYMBOL_TOKEN`, i.e. a `NAME`) | none, so ask |
| Composition: which Strategy / Flow / MetaFlow / Action | ask, then offer the workspace's existing artifacts |
| Direction or explicit Order | L→R |
| Realification or complexification | `unset` (profile-defined) |
| Representation | ask: Elementary, Unique, Composite or Unknown |
| Category | `Storage` |
| Grayscale conversion | `unset` (open item) |

## Procedure

1. **Resolve references.** Each named artifact must exist in `.<project>/`. A missing one is `CANOR_ERR_NOT_FOUND`.
2. **Validate the Order.** It must have length `n` and be a permutation of `0 … n-1`. Otherwise report `CANOR_ERR_ARGS` and name the index that's missing or repeated.
3. **Compute the count stream** for every trifecta, with exact integers. Show it as a table: step, consumed position, atom, vector, residue vector.
4. **Verify by property:**

   | Property | On failure |
   |---|---|
   | Rows per trifecta = `n + 1` | `CANOR_ERR_ARGS` (internal; report it) |
   | No component goes below 0 | `CANOR_ERR_RANGE` |
   | Final vector = 0 for every trifecta whose atoms were all consumed | report |
   | Every residue lies in `[0, N)` | `CANOR_ERR_RANGE` |
   | Flow input is byte-identical before and after | report |
   | Auxiliary atoms have a policy | `CANOR_ERR_UNDEFINED` (open item) |

5. **Write the header** `.<project>/soul/<name>.fwh`: provenance, the symbol declaration, its composition, its representation, its category and its Order.
6. **Write the body** `.<project>/soul/<name>.fwb`: the invocation, and the stream. For an enumerated space, write the index spec instead (see the shared rules).

### Example

Flow `i, j, k, i`, trifecta `(i, j, k)`, L→R, Action `mod 4`:

| Step | Consumed | Vector | Residues |
|---|---|---|---|
| 0 | none | [2, 1, 1] | [2, 1, 1] |
| 1 | 0 `i` | [1, 1, 1] | [1, 1, 1] |
| 2 | 1 `j` | [1, 0, 1] | [1, 0, 1] |
| 3 | 2 `k` | [1, 0, 0] | [1, 0, 0] |
| 4 | 3 `i` | [0, 0, 0] | [0, 0, 0] |

That's 5 = n + 1 vectors.

Uses the proposed default grammar semantics (`~` = Soul):

```
~soul.demo(flow=@flow.demo, action=%action.parity, order=L2R, form=unset, representation=Composite, category=Storage);
```

## Done when

- [ ] The workspace exists. `.fwh` and `.fwb` are written, and provenance is recorded.
- [ ] The Order is validated. The count-stream table has `n + 1` rows per trifecta.
- [ ] Every residue is canonical. Grayscale conversion is marked as profile-defined unless the profile sets it.

**Next:** `/maneuver` to bind this symbol to an equation or expression.
