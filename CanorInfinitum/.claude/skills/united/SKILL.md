---
name: united
description: Create a new Canor United™ Interchange / Meta-Schema, the seventh CanorLinguae interoperability language (Canor §7). It is the .fwo compiled-object model: a United body with numeric and logical arithmetic over earlier artifacts, plus distinct JSON Object and/or XML Markup artifacts (code blocks or nested JSON), embedded or referenced by path and checksum, under an ordered manifest and a canonical digest. For a Meta-Schema it also records the signatories and their ε-error allowances. Use when the user invokes /united, or asks for a United object, an interchange, a Meta-Schema, or a .fwo.
---

# /united: Create a new United™ Interchange / Meta-Schema

Builds the source form of a United object inside the project workspace: header, body, ordered manifest and artifacts. Validates every artifact, and computes the canonical digest only when the profile defines an algorithm.

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

## Definition (Canor §7, 7.A JSON, 7.B XML)

- **United** is the standard interchange language, and the container for compiled `.fwo` objects. It's the common object boundary for symbols, generated schemas, and declared Complementary Language blocks.
- **Header** may declare Soul™ symbols for direct use in the body.
- **Body** groups the earlier languages with numeric and arithmetic-logic operations. §7 gives a Flow string combined with `xor` under `(mod 4)` as a legal body.
- **Artifacts.** JSON Object and XML Markup are **distinct content**, not alternate serializations of the body. Each artifact is either:
  - embedded as ASCII-first text, **or**
  - referenced by a project-relative path plus its expected checksum.
- **XML** may hold **ordered child artifacts**: JSON Object strings, code blocks, Canor™-specific files. The profile declares and validates permitted content and nesting.
- **Assembly** resolves each referenced path and verifies its checksum before accepting the object.
- **Canonical digest** covers, in order: the body, the ordered artifact manifest, the embedded payloads, and the reference checksums. Escaping, path resolution, the checksum algorithm, the digest encoding and XML child interpretation are all **profile-defined**.
- **Meta-Schema.** A meta-schema is valid only by **unanimous multilateral agreement**: N ≥ 2 signatories (User / App / Agent, internal or external). Each one records an ε-error allowance per error category, and all approve the same digest, version, basis, provenance and margin.
  - One rejection rejects the meta-schema.
  - One pending item leaves it incomplete.
  - An unset algorithm leaves it `CANOR_ERR_UNDEFINED`.
  - It is never accepted by default.

## Inputs

| Input | Default |
|---|---|
| Object name (`NAME`) | none, so ask |
| Header: the Soul symbols to declare | the workspace's Soul symbols that the body uses |
| Body: earlier artifacts + arithmetic / logic | none, so ask |
| Artifacts: kind (JSON/XML), embedded or referenced, XML children in order | none |
| Checksum / digest algorithm | the profile's `@digest`. If it's `unset`, it stays an open item. |
| Meta-Schema: signatories, roles, error categories, allowances `[LO, HI]` | asked only for a Meta-Schema |

## Procedure

1. **Read `@digest`** from the `.fwp`.
   - If it's `unset`, say so. Don't pick an algorithm silently.
   - A project may opt into a published profile instead. For example, `pqc-slh-dsa` (`CanorLinguae/elite/TRUST.md`) uses SHA3-512. It applies only when that project's `.fwp` opts in; it's never a default.
   - If the user chooses one, append the change to the `.fwp` after they approve it, and record it as a profile decision.
2. **Build the ordered manifest.** Columns: index, kind, embedded / referenced, path, checksum. The order is significant to the digest.
3. **Validate by property:**

   | Property | On failure |
   |---|---|
   | Every body reference resolves in `.<project>/` | `CANOR_ERR_NOT_FOUND` |
   | Embedded JSON parses | `CANOR_ERR_ARGS` |
   | Embedded XML is well-formed, with children in declared order | `CANOR_ERR_ARGS` / `CANOR_ERR_NOT_CLOSED` |
   | Each referenced path exists under the repository root | `CANOR_ERR_NOT_FOUND` |
   | Each checksum matches (when an algorithm is set) | diagnostic. Never accept silently. |
   | Nesting is within the profile's allowance | `CANOR_ERR_UNDEFINED` (open item) |
   | Meta-Schema: ≥ 2 signatories, each allowance a closed integer interval | `CANOR_ERR_ARGS` / `CANOR_ERR_RANGE` |

4. **Digest.** Compute it only under a declared algorithm. Otherwise list the exact digest inputs in order, so it can be computed later.
5. **Write** to `.<project>/united/`:
   - `<name>.fwh`: provenance, Soul declarations, signatories, allowances.
   - `<name>.fwb`: body plus manifest.
   - The artifacts beside them: `<name>.<n>.json` / `<name>.<n>.xml`.
6. **Don't commit** the compiled `<name>.fwo`. It's a build product.
7. **Approval status.** For a Meta-Schema, report each signatory as `pending`, `approved` or `rejected`, and the overall result as `accepted`, `rejected`, `incomplete` or `undefined`.

### Example

Uses the proposed default grammar semantics. The active profile overrides it.

`.<project>/united/accord.fwh`

```
@provenance(spec="Canor §7", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
@soul{~soul.demo};
@signatory.user(role=User, external=off);
@signatory.app(role=App, external=off);
@allowance.user(category=orthonormalization, interval=[0, 25]);
@allowance.app(category=orthonormalization, interval=[0, 25]);
```

`.<project>/united/accord.fwb`

```
@united.accord:"iiiijk" xor ~soul.demo (mod 4);
@manifest{
  (index=0, kind=json, mode=embedded, path="accord.0.json", checksum=unset),
  (index=1, kind=xml, mode=referenced, path=".<project>/united/accord.1.xml", checksum=unset)
};
```

## Done when

- [ ] The workspace exists. `.fwh`, `.fwb` and the artifacts are written, and provenance is recorded.
- [ ] The ordered manifest is shown. Every artifact is validated or diagnosed.
- [ ] The digest is computed under a declared algorithm, or its ordered inputs are listed as an open item.
- [ ] For a Meta-Schema: signatories, allowances, and per-signatory and overall status are shown. Nothing is accepted by default.

**Next:** `/elite` to agree a Diffeomorphic Basis and assemble.
