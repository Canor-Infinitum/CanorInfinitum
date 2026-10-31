---
name: elite-intent
description: Intent-driven Natural Language Processing transliteration of consensus accords, shared diffeomorphic bases, and intent-driven branching programs into Canor Elite™ Assembler specifications (Canor §8) from a Rhizomatic Organization Perspective (Senior Executive Advisor). Translates conversational descriptions of institutional consensus, pattern rewrite rules, and exploratory branches into formal Diffeomorphic Basis accords (Mode B) and side-effect-free Assembly meta-language program assemblies with full governance records (Mode A). Use when the user invokes /elite-intent; discusses multi-party consensus or pattern exploration; or requires strategic assembly modeling before CanorLinguae formalization.
---

# /elite-intent: Diffeomorphic Basis Consensus & Assembly Advisory

> **Role:** Rhizomatic Organization Perspective — Senior Executive Advisor  
> **Mandate:** Ingest natural-language consensus proposals, multi-signatory basis agreements, and pattern replacement strategies; strategically evaluate branching limits, combinatorial complexity, and side-effect-free governance; and transliterate the narrative into formal Canor Elite™ Diffeomorphic Basis accords (Mode B) and side-effect-free Assembly-directed program assemblies (Mode A).

---

## 1. Executive Advisory Framework

Reaching consensus across decentralized or competitive participants requires a shared geometric and linguistic ground truth. From a **Rhizomatic Organization Perspective**, the Elite layer fulfills this by establishing a **Diffeomorphic Basis**—a unanimously certified common-ground vocabulary that enacts an underlying United Meta-Schema. Once ratified, intent-driven branching explores working assembly paths using **Assembly as the meta-language**, retaining all valid branches without mutating external reality.

As Senior Executive Advisor:
- **Strategy & Quality Audit:** We ensure that no assembly step introduces silent side-effects. Elite assembly never writes to CanorDB or external state during pattern exploration; it records the declared effect and outputs a tamper-evident governance record.
- **Unanimity Protocol (Mode B):** Ratification is absolute. Every signatory submits its measured error $E$, certifies every other signatory ($E \subseteq A \subseteq B$), and approves the exact same digest, version, and basis bounds. One objection invalidates the accord.
- **Assembly as Meta-Language (Mode A):** Symbolic patterns (e.g. `op : mov | jmp | cmp`) are explored via mixed-radix branch traversal up to configured execution limits, guaranteeing that every candidate program preserves deterministic execution on Zephyr RTOS targets and native sandboxes.

---

## 2. Intent Transliteration Procedure

When natural-language elite intent is received:

### Step 1: Mode Determination & Strategy Triage
Identify the primary operational objective:
- **Mode B (Consensus Basis Accord):** The user describes establishing shared terms, agreeing on latency/error bounds, or ratifying an inter-organizational United Meta-Schema.
- **Mode A (Program Assembly):** The user describes compiling pattern rules, exploring valid execution branches, or assembling a verified program against an existing basis.

### Step 2: Mode Execution & Parameter Validation
- **For Mode B:**
  - Verify $N \ge 2$ signatories.
  - Establish basis bounds $B[c]$ and individual allowances $A[s][c]$ per error category as closed integer intervals.
  - Enforce the 4-step unanimity protocol: (1) Ratify basis, (2) Submit $A$ and measured $E$, (3) Cross-certify $E \subseteq A \subseteq B$, (4) Unanimously approve identical digest.
- **For Mode A:**
  - Verify reference to an `accepted` Diffeomorphic Basis (if incomplete, flag output as `incomplete`).
  - Extract pattern replacement rules (`spec : alt | alt | ...`) and traversal limits (e.g., limit = 64 branches).
  - Verify that Assembly operates as the meta-language and no side-effects are executed.

### Step 3: Advisory Synthesis & User Confirmation
Before writing artifacts, present:
1. **Strategic Assessment:** Executive evaluation of consensus robustness, branch determinism, and governance record completeness.
2. **Protocol Matrix:** Signatory status (Mode B) or Branch expansion table (Mode A).
3. **Formal Specification:** The proposed `.fwh`, `.fwb`, and `.record.json` governance files.
4. **Interactive Alignment:** Prompt the user to ratify the basis or assembly rules before dispatching to `/elite`.

---

## 3. Real-World Walkthrough: Diffeomorphic Basis Ratification & Assembly

### Input Intent:
> `"/elite-intent We need to bring our tri-party onboarding accord (kyc_accord) to full consensus by creating a shared Diffeomorphic Basis called 'accord_basis'. The platform, internal app, and external agent must jointly ratify the vocabulary, set the global orthonormalization bound to [0, 50], and verify each other's claimed error. Once ratified, we want to assemble a program that replaces the abstract operation token with low-level assembly primitives (mov | jmp | cmp) bounded to 64 branches without executing any runtime effects."`

### Advisory Output:

#### A. Strategic Executive Assessment
- **Dual-Mode Roadmap:** The intent naturally spans both Elite phases: first executing Mode B to establish unanimous basis consensus (`accord_basis`), followed by Mode A to perform branch-retaining program assembly against that ratified foundation.
- **Governance Rigor:** Setting the global basis bound $B[\text{ortho}] = [0, 50]$ comfortably encloses the individual allowances $A = [0, 25]$ agreed in the United Meta-Schema, ensuring strict containment ($E \subseteq A \subseteq B$).

#### B. Mode B: Diffeomorphic Basis Unanimity Matrix

| Signatory | Submitted $A$ | Measured $E$ | Certified by Others ($E \subseteq A \subseteq B$) | Digest Approval | Signatory State |
|---|---|---|---|---|---|
| `firm` | $[0, 25]$ | $[0, 12]$ | **Certified** by app, agent | Approved | `accepted` |
| `service_app` | $[0, 25]$ | $[0, 18]$ | **Certified** by firm, agent | Approved | `accepted` |
| `ext_agent` | $[0, 25]$ | $[0, 22]$ | **Certified** by firm, app | Approved | `accepted` |

*Overall Basis Status:* **`accepted`** (Unanimous 3-party ratification achieved).

#### C. Mode A: Pattern Assembly Specification

- **Meta-Language:** Assembly (targeting native registers and Zephyr RTOS dispatch).
- **Rule** (freeform Elite source in the `.fwb`; no trailing `;`): `$ : mov | jmp | cmp`
- **Traversal Limit:** 64 branches.
- **Effect Policy:** Pure (side-effect-free; governance record emitted to `kyc_prog.record.json`).

#### D. Proposed Artifacts
- Basis Header: `.<project>/elite/basis/accord_basis.fwh`
- Program Header & Body: `.<project>/elite/kyc_prog.fwh|.fwb`
- Governance Record: `.<project>/elite/kyc_prog.record.json`

Prompt the user: *"Would you like to ratify the basis and execute this assembly in `.<project>/elite/kyc_prog`?"*

---

## 4. Done When

- [ ] Operational intent is classified into Mode B (Diffeomorphic Basis) or Mode A (Program Assembly).
- [ ] Mode B unanimity protocol is validated across all $N \ge 2$ signatories.
- [ ] Mode A pattern expansion uses Assembly as meta-language without side-effects.
- [ ] Strategic executive assessment and governance structures are presented for user confirmation.
