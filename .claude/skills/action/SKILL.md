---
name: action
description: Create a new Canor Action™ modulation, the fourth CanorLinguae interoperability language (Canor §4). An Action is a typed (mod N) equation, L ≡ R (mod N), over grammar expressions, with canonical residues and an explicit diagnostic for every undefined case. Use when the user invokes /action, or asks to create an Action, a modulation, an Action Modulation, or a modular constraint.
---

# /action: Create a new Action™ Modulation

Creates a `(mod N)` equation inside the project workspace and proves its residues on sample assignments, with exact integer arithmetic.

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

## Definition (Canor §4)

- A **typed modular constraint** over an expression from the language grammar, ending in a modulus marker. The §4 primitive over `m, n, o` with modulus `4` is the reference example.
- Variables take values in ℕ. The **modulus `N` is a positive integer**.
- Operators come from the grammar's `OPERATOR` set. Multiplication and division spellings stay separate source operators where configured.
- **Relation:** `L ≡ R (mod N)` holds exactly when `residue_N(L) = residue_N(R)`. Here `residue_N(z)` is the unique `r` with `0 ≤ r < N` and `z ≡ r (mod N)`. Canonical residues are never negative.
- **Evaluation** (forms, division, mixed arithmetic) is profile-selected. A missing or undefined evaluation is an **explicit diagnostic**, never a silently removed branch.
- **Enumerated space:** `m = n (mod o)` with `m, n, o ∈ 0 … 289`, giving 24,389,000 Actions. The 84,100 with `o = 0` are kept as `CANOR_ERR_UNDEFINED` diagnostics. They're never dropped, and never become a division trap.

## Inputs

| Input | Default |
|---|---|
| Action name (`NAME`) | none, so ask |
| Left expression `L`, right expression `R` | none, so ask |
| Modulus `N` | none, so ask |
| Variables and their ℕ domains | all of ℕ |
| Evaluation profile (division, mixed arithmetic) | `unset` (open item) |
| Trifecta or count vector it maps (for `/soul`) | none |

## Procedure

1. **Parse** `L` and `R` as `EXPRESSION` (atoms.ebnf). Report the first token that fails.
2. **Validate by property:**

   | Property | On failure |
   |---|---|
   | `N` is an integer and `N > 0` | `N = 0`: `CANOR_ERR_UNDEFINED` (kept as a diagnostic). `N < 0`: `CANOR_ERR_RANGE`. |
   | Every variable has an ℕ domain | `CANOR_ERR_ARGS` |
   | Every constant is named | Flag it. Name it before writing. |
   | Division is exact, or the profile defines it | `CANOR_ERR_UNDEFINED` |
   | No floating-point literal | `CANOR_ERR_ARGS` |

3. **Prove on samples.** For each assignment the user gives (or small ones you choose, labeled as samples):
   - compute `L`, `R`, `residue_N(L)` and `residue_N(R)` with exact integers;
   - show that negative intermediates still reduce into `[0, N)`;
   - report holds or fails.
4. **Write the header** `.<project>/action/<name>.fwh`: provenance, variables and domains, named constants, modulus, evaluation profile.
5. **Write the body** `.<project>/action/<name>.fwb`: the modulation.

### Example

Uses the proposed default grammar semantics (`%` = modulation). The source form of `L ≡ R (mod N)` is `L = R (mod N)` inside a `%` modulation: inside a modulation, `=` together with the `(mod N)` marker means congruence.

`.<project>/action/parity.fwh`

```
@provenance(spec="Canor §4", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
@domain(m=N, n=N, o=N);
@const.modulus:4;
```

`.<project>/action/parity.fwb`

```
%action.parity:4*m = 4*n + 3*o (mod modulus);
```

| m | n | o | L | R | L mod 4 | R mod 4 | Holds |
|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | yes |
| 1 | 0 | 0 | 4 | 0 | 0 | 0 | yes |
| 0 | 0 | 1 | 0 | 3 | 0 | 3 | no |

## Done when

- [ ] The workspace exists. `.fwh` and `.fwb` are written, and provenance is recorded.
- [ ] The constraint is shown as `L ≡ R (mod N)`, with typed variables and named constants.
- [ ] Sample residues are computed, and every one satisfies `0 ≤ r < N`.
- [ ] Every undefined case is a listed diagnostic.

**Next:** `/soul` to compose this Action with a Flow or MetaFlow.
