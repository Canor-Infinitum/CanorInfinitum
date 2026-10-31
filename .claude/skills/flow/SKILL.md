---
name: flow
description: Create a new Canor Flow™, the second CanorLinguae interoperability language (Canor §2). A Flow is an immutable, ordered string of imaginary-base atoms whose defining trifectas are declared in a .fwh header and whose invocations are in a .fwb body. Use when the user invokes /flow, or asks to create, declare or register a Flow, a Flow string, a trifecta or a Strategy's imaginary bases.
---

# /flow: Create a new Flow™

Creates a Flow inside the project workspace, checks it by property, and shows the initial count vector of each trifecta that a Soul™ traversal will start from.

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

## Definition (Canor §1, §2)

- **AtomUsage** = (atom name, ring index ∈ ℕ). An omitted index means `0`. Explicit indices (`i0, j1, k0`) select indexed instances, and range over `0 … ∞`.
- **The ring index is categorical.** It selects an indexed instance of an atom name. Matching is by atom name. The index is never folded into arithmetic.
- **Flow** = a length `n` plus a total function over `Index(n) = {0 … n-1}`. The index set is empty when `n = 0`. The Flow is **immutable and ordered** throughout evaluation.
- **Duplicates are distinct positions.** Each occurrence counts. Never deduplicate.
- **Trifecta** = an ordered triple of atom names, used as imaginary bases with column arithmetic. The five pinned IMAG trifectas are Hamilton's quaternionic trifectas: `a*b = c`, `b*c = a`, `c*a = b`, `b*a = -c` (e.g. `i*j = k`, `l*m = n`, `e*f = g`). A triple of other atom names **does not get undeclared algebraic laws**; its laws apply only when declared. The default IMAG basis has 5 trifectas: 16 elements with sedenion closure. The Cayley-Dickson product (XOR of the units' Cayley-Dickson coordinates) is the IMAG product, not a Flow's value.
- Atoms that appear in the Flow string are registered there, including atoms that belong to no trifecta. Their handling (the auxiliary channel) is a settings choice.
- **Commas** are the default delimiter. Whitespace is optional and is kept as lexical metadata, not discarded.
- **Strategies** define the differential imaginary bases (`di … dg`) that a Flow's trifectas instantiate.
  - Each differential `d_u` is an Ore extension `K[x_u][d_u; σ, δ]`, with `d_u·f = σ(f)·d_u + δ(f)`.
  - **Order** `k` is the power `d_u^k`. The profile default `τ = σ = 1` has period 6, so Orders `1 … 6` run one period.
  - That gives 15 × 6 = 90 Strategies.
- **Enumerated Flow space:** the 15 atoms, with trifecta order fixed and each trifecta independently over its 6 permutations. That's `6^5 = 7,776` Flows, ranked mixed-radix 6 with trifecta 0 most significant.
  - Enumerated Flows contain each atom once, because each is a permutation. A Flow value in general may repeat atoms.
- **Value type:** `canor_soul_flow_t` (`canor_soul.h`). Enumeration and ranking: `canor_enum.h`.

## Inputs

Ask only for what's missing. Batch the questions into one `AskUserQuestion` call where possible.

| Input | Default |
|---|---|
| Flow name (`NAME`) | none, so ask |
| Atom string, e.g. `i, j0, k, i1, l` | none, so ask |
| Trifectas to register | the pinned IMAG trifectas the string uses |
| Declared algebraic laws | none (stays undeclared) |
| Auxiliary-atom policy | `unset` (open item) |

## Procedure

1. **Normalize.** List every position: index, atom name, ring index (an omitted index is written as `0`), and its trifecta (or `auxiliary`). Keep the original string verbatim next to the table, so the whitespace metadata survives.
2. **Validate by property.** Every check passes or produces a diagnostic. Never silently correct the input.

   | Property | On failure |
   |---|---|
   | `n` = number of delimited atoms, duplicates kept | `CANOR_ERR_ARGS` |
   | Each name matches `NAME = NAME_START { NAME_CONTINUE }` | `CANOR_ERR_ARGS` |
   | Each ring index is a nonnegative integer | `CANOR_ERR_RANGE` |
   | Each trifecta has exactly three names | `CANOR_ERR_ARGS` |
   | Every referenced trifecta is declared (pinned or in a `.fwh`) | `CANOR_ERR_NOT_FOUND` |
   | Atoms outside every trifecta have a policy | `CANOR_ERR_UNDEFINED` (open item) |

   A name may sit in more than one trifecta component. Say so, because consuming it decrements every matching component (§5).
3. **Compute the initial count vector** for each trifecta: the number of occurrences of each of its three names. Soul™ emits this vector first.
4. **Write the header** `.<project>/flow/<name>.fwh`: provenance, trifecta declarations, declared laws, auxiliary policy.
5. **Write the body** `.<project>/flow/<name>.fwb`: the Flow invocation, verbatim.
6. **Register** the header in the profile only if the profile lists sources explicitly. Otherwise leave the `.fwp` unchanged.

### Example

The examples use the proposed default grammar semantics: the sigil is the path, the header is the name, `@` means registry. If the active profile overrides them, follow the profile instead. Any form that `atoms.ebnf` doesn't settle is an open item, not an invention.

`.<project>/flow/demo.fwh`

```
@provenance(spec="Canor §2", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
@trifecta.ijk(i, j, k);
@trifecta.lmn(l, m, n);
@law.ijk{i*j = k, j*i = -k};
@auxiliary:unset;
```

`.<project>/flow/demo.fwb`

```
@flow.demo:"i, j0, k, i1, l";
```

| # | Atom | Ring | Trifecta |
|---|---|---|---|
| 0 | i | 0 | ijk |
| 1 | j | 0 | ijk |
| 2 | k | 0 | ijk |
| 3 | i | 1 | ijk |
| 4 | l | 0 | lmn |

Initial vectors: `ijk → [2, 1, 1]`, `lmn → [1, 0, 0]`.

## Done when

- [ ] The workspace exists. `.fwh` and `.fwb` are written, and provenance is recorded.
- [ ] The normalized table and the verbatim source string are both shown.
- [ ] Each trifecta's initial count vector is shown.
- [ ] Every diagnostic and open item is listed. Nothing was dropped.

**Next:** `/metaflow` for channels, `/action` for a modulation, `/soul` to traverse.
