---
name: elite
description: Canor Elite™ Programming & Branching Assembler, the eighth CanorLinguae interoperability language (Canor §8). Two modes. (B) Create a new multilateral Diffeomorphic Basis / Quantum-Linguistics: a unanimously ratified common-ground basis that enacts a United Meta-Schema. (A) Assemble an Elite Programming program against that basis by side-effect-free, branch-retaining pattern assembly with a governance record, using Assembly as the meta-language. Use when the user invokes /elite, or asks to assemble an Elite program or agree a Diffeomorphic Basis or Quantum-Linguistics.
---

# /elite: Assemble Elite™ Programming, or create a Diffeomorphic Basis

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

## Definition (Canor §8)

- **Elite** is the procedural invocation layer. It can invoke Soul™ symbols, Maneuver™ expressions, United™ meta-schemas, Flow™ strings, Action™ modularities and earlier forms.
- **Pattern replacement using Assembly as the meta-language.** A specification maps to alternative definitions. The §8 example `$ : mov | jmp | cmp` maps `$` to `mov`, `jmp` or `cmp`. The assembler explores multiple interpretation paths (intent-driven branching) toward a working program assertion. The source syntax is described in the next section.
- **Tooling** guides the branching and is declared and profile-selected. Tool kinds are agent, language model, stochastic, procedural and other. The core never calls a tool; a tool's output enters only as a score over `CANOR_ONE`. Each tool's identity, version, configuration and branch outputs are recorded.
- **Every branch is retained** up to the configured traversal limit, including operator branches and L/R traversal identities. Undefined cases are diagnosed, never pruned.
- **Side-effect-free.** Assembly never mutates CanorDB or external state. A stateful Maneuver Group runs only at declared runtime, under `.fwp` / `.fws` authorization. Assembly **records** the declared effect without performing it.
- **Diffeomorphic Basis.** For a multilateral United schema, the agreed Elite vocabulary is the common diffeomorphic basis (quantum linguistics).
  - Every signatory binds to the **same** canonical schema digest, Elite basis and version, provenance record and error margin.
  - Each signatory then assembles locally against that contract.
- **Governance record.** When the traversal limit is reached, assembly outputs a record:
  - profile and settings versions;
  - active contract IDs;
  - the agreed basis;
  - the branch trace;
  - configured vs. consumed counts;
  - the termination reason;
  - complete / incomplete status.

  Persisting the record is a separate governed action.

## Elite source syntax

Elite source is **freeform and line-based**, and it is **not** parsed with `atoms.ebnf`. That's why `$`, and every other glyph, is free for Elite use even though the grammar settings give `$` another meaning. The reference implementation is `canor_elite_program_parse`.

Lines end in LF or CRLF. Each line is trimmed, and blank lines are skipped. Every other line is exactly one of:

| Line | Form | Meaning |
|---|---|---|
| **Rule** | `spec : def \| def \| …` | Everything before the **first** unescaped `:` is the specification. Everything after it is a list of definitions. Either side may hold any text. `spec :` with nothing after it defines the empty text, i.e. a deletion. |
| **Program line** | anything without an unescaped `:` | One line to assemble. |

- **Separators.** A vertical bar (`|`) separates definitions, but not inside `()`, `[]` or `{}`.
- **Escapes.** Only two:
  - backslash-colon (`\:`) is a literal colon;
  - backslash-bar (`\|`) is a literal bar.

  So a bra-ket inside Elite source is written `<a\|b>`. TeX commands such as `\alpha` pass through unchanged.
- **No comments.** A comment-like line is still parsed as a rule or a program line. That's why provenance goes in the `.fwh` header, never in the Elite source.
- **Redefinition accumulates.** A spec redefined on a later line *adds* definitions, in source order. An earlier definition is never dropped, and each one is another candidate interpretation.
- **Nested replacement.** A definition may contain other specs. Rewriting happens one site at a time, up to `CANOR_ELITE_DEPTH_MAX = 8` rewrites per line. A deeper rewrite (every cycle ends up here) leaves a *diagnosed* interpretation, not a dropped one.
- **Matching.** A spec matches only where it doesn't split a word. Word characters are letters, digits, `_` and any non-ASCII byte. So `mov` doesn't match inside `movx`, but `$` does match in `$r0`.
  - **L2R** rewrites the leftmost-longest site first.
  - **R2L** rewrites the rightmost-longest site first, i.e. the site that ends last.
- **Interpretations.** Each line keeps at most `CANOR_ELITE_EXPANSION_MAX = 32` interpretations, ordered lexicographically by rewrite path. Beyond that, the record sets `expansion_limited`: the total becomes a lower bound and the traversal is incomplete.
- **After rewriting.** Once no spec matches, a line reads `head operand, operand, …` for the basis checks. Top-level commas separate the operands.
- **Capacities** (overridable via `#ifndef`):

  | Macro | Default | Limits |
  |---|---|---|
  | `CANOR_ELITE_RULE_MAX` | 16 | rules |
  | `CANOR_ELITE_ALT_MAX` | 8 | definitions per spec |
  | `CANOR_ELITE_INSN_MAX` | 16 | program lines |
  | `CANOR_ELITE_TEXT_MAX` | 128 | bytes per spec, definition or line, including the NUL |
  | `CANOR_ELITE_BRANCH_MAX` | 256 | branches |

### Parse diagnostics

Each one is recorded with its line number. Parsing continues past it.

| Kind | Status | When |
|---|---|---|
| `EMPTY_SPEC` | `CANOR_ERR_ARGS` | `:` with nothing before it |
| `TOO_LONG` | `CANOR_ERR_RANGE` | a spec, definition or line is longer than `CANOR_ELITE_TEXT_MAX - 1` |
| `FULL` | `CANOR_ERR_FULL` | the rule, definition or line capacity is exceeded |
| `MALFORMED` | `CANOR_ERR_ARGS` | e.g. a NUL byte inside a line |

## Step 0: Choose the mode

Unless the request already says, ask with `AskUserQuestion`:

- **A. Assemble** an Elite Programming / Quantum-Linguistics program.
- **B. Create** a new multilateral Diffeomorphic Basis.

Mode A needs a basis. If none exists in `.<project>/elite/basis/`, offer to run mode B first.

## Mode B: Create a Diffeomorphic Basis

1. **Collect:**
   - basis name;
   - signatories (N ≥ 2, User / App / Agent, internal or external);
   - the United Meta-Schema(s) to enact (from `/united`) and their digests;
   - the Elite vocabulary;
   - the basis version;
   - error categories, each with a basis bound `B[c]` and each signatory's allowance `A[s][c] ⊆ B[c]` as closed integer intervals.
   - Optionally: latency metrics, jitter coefficients, error-rate allowances and an update-rate budget (Aether timing fields).
2. **Apply the unanimity protocol.** Every signatory:
   1. ratifies the basis;
   2. submits, per category, its allowance `A` and its measured orthonormalization error `E`;
   3. is certified per category by every **other** signatory (`E ⊆ A ⊆ B`);
   4. approves the same digest, version, basis, provenance and margin.
3. **Evaluate:**

   | Outcome | When |
   |---|---|
   | `accepted` | every step above holds for every signatory |
   | `rejected` | any rejection |
   | `incomplete` | any pending ratification, certificate or approval |
   | `undefined` (`CANOR_ERR_UNDEFINED`) | the profile's digest or signature algorithm is `unset` |

   Never accept by default. Adding or removing a signatory voids earlier ratifications and approvals.

   The digest and signature algorithms come from the profile. A project may opt into `pqc-slh-dsa` (`CanorLinguae/elite/TRUST.md`); otherwise they stay `unset`, and the outcome is `undefined`.
4. **Write** `.<project>/elite/basis/<name>.fwh`:
   - provenance;
   - vocabulary;
   - version;
   - schema digest(s);
   - categories with `B[c]`;
   - per-signatory `A`, `E`, certificates and approvals, each with its status.

## Mode A: Assemble a program

1. **Collect:**
   - program name;
   - the basis (mode B output);
   - the Elite source: rule lines `spec : def | def | …` and program lines (see **Elite source syntax**);
   - the direction, `L2R` (default) or `R2L`;
   - the earlier artifacts to invoke;
   - the declared tooling (name, version, kind, configuration digest, and seed for stochastic tools);
   - the traversal limit.
2. **Check the basis.** If its status isn't `accepted`, still assemble, but mark the output `incomplete`. Name the basis status in the record.
3. **Parse** the source. Report every parse diagnostic with its line number. A rule a diagnostic hits contributes nothing, but the diagnostic stays in the report.
4. **Expand.**
   - For each program line, list its interpretations: the rewrite path, the resulting text, and its flags.
   - A **branch** picks one interpretation per line.
   - Its id is a mixed-radix rank. The first line in the traversal direction is the most significant digit.
   - Traverse up to the limit, and show the table: branch id, choice per line, flags, `candidate` or `diagnosed`.
5. **Validate by property.** Flags mark a branch; they never remove it.

   | Flag | Meaning | Branch is |
   |---|---|---|
   | `NOT_IN_BASIS` | the head is outside the agreed vocabulary | diagnosed |
   | `ARITY` | operand count ≠ the declared arity | diagnosed |
   | `EFFECT_UNAUTHORIZED` | an effect has no active contract | diagnosed |
   | `DEPTH` | more than `CANOR_ELITE_DEPTH_MAX` rewrites (cycles) | diagnosed |
   | `TOO_LONG` | a rewrite would exceed `CANOR_ELITE_TEXT_MAX` | diagnosed |
   | `EFFECT_RECORDED` | an authorized effect, recorded and not performed | candidate |
   | `UNCHECKED` | no basis bound: raw syntax, permissive | candidate |
   | `EMPTY` | the line rewrote to nothing: a no-op | candidate |

   Also check:
   - every invoked earlier artifact exists (`CANOR_ERR_NOT_FOUND`);
   - no effect is performed during assembly (if one would be, stop: that violates the side-effect-free rule).
6. **Termination.**
   - `exhausted`: every branch was traversed, and the record is `complete`.
   - `limit`: the configured traversal limit was reached, and the record is incomplete.

   Either way, `expansion_limited` also marks the record incomplete.

7. **Emit the governance record** with every field listed in the Definition. Include the counts: configured, consumed, total, candidates, diagnosed, effects declared. If a field has no schema yet (candidate ordering beyond lexicographic, tool determinism, resource accounting, signature lifecycle), list it as an open item.
8. **Write** to `.<project>/elite/`:
   - `<name>.fwh`: a Canor header, parsed with `atoms.ebnf`. It holds provenance, the basis reference, tooling, direction and limit.
   - `<name>.fwb`: the **Elite source**, rules and program lines only, exactly as `canor_elite_program_parse` reads it. Put no provenance or comments here: any line would parse as a rule or a program line.
   - `<name>.record.json`: the governance record.

   Write nothing to CanorDB.

### Example

`.<project>/elite/demo.fwh` (Canor header, proposed default grammar semantics):

```
@provenance(spec="Canor §8", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
@elite.demo(basis=@basis.accord, direction=L2R, limit=64, tooling=none);
```

`.<project>/elite/demo.fwb` (Elite source, §8):

```
$ : mov | jmp | cmp
$ r0, r1
```

Basis vocabulary for this example: `mov` and `cmp` with arity 2, `jmp` with arity 1. That gives three branches:

| Branch | Line 2 interpretation | Flags | Status |
|---|---|---|---|
| 0 | `mov r0, r1` | none | candidate |
| 1 | `jmp r0, r1` | `ARITY` | diagnosed (kept) |
| 2 | `cmp r0, r1` | none | candidate |

Termination: `exhausted`, `complete`. Totals: 3 consumed, 2 candidates, 1 diagnosed.

## Done when

- [ ] The workspace exists, the mode is stated, the files are written, and provenance is recorded.
- [ ] **B:** each signatory has a status and the overall outcome is shown. Nothing is accepted by default.
- [ ] **A:** every branch is kept or diagnosed, the governance record is complete or its gaps are listed, and no effect was performed.

**Next:** `/follower` to compile with the project's Complementary Languages.
