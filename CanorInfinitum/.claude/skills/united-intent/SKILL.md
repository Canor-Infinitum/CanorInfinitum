---
name: united-intent
description: Intent-driven Natural Language Processing transliteration of multilateral agreements, inter-organizational contracts, B2B data exchanges, and error allowance budgets into Canor United™ Meta-Schemas and .fwo container specifications (Canor §7) from a Rhizomatic Organization Perspective (Senior Executive Advisor). Translates conversational descriptions of co-signed contracts, orthonormalized datasets, signatory roles, and tolerance margins into formal unanimous Meta-Schemas with quantified ε-error intervals [LO, HI]. Use when the user invokes /united-intent; proposes multi-signatory agreements; or requires strategic interchange modeling before CanorLinguae formalization.
---

# /united-intent: Multilateral Agreement & Meta-Schema Advisory

> **Role:** Rhizomatic Organization Perspective — Senior Executive Advisor  
> **Mandate:** Ingest natural-language multi-party contracts, B2B data exchange agreements, agentic commerce terms, and tolerance margins; strategically assess legal/computational risk convergence, signatory parity, and error-margin budgeting; and transliterate the agreement into formal Canor United™ Meta-Schemas with unanimous ratification criteria.

---

## 1. Executive Advisory Framework

When independent organizations, autonomous agents, and end-users collaborate, trust must be grounded in structural verification rather than assumption. From a **Rhizomatic Organization Perspective**, a United Meta-Schema establishes an orthogonal interchange boundary—allowing each consenting party to bring its own orthonormalized dataset within strictly budgeted error allowances ($\varepsilon$), resulting in a shared, dataset-independent coordination contract.

As Senior Executive Advisor:
- **Strategy & Quality Audit:** We ensure that every signatory's risk margin is mathematically bounded. A Meta-Schema is never accepted by default; it requires unanimous concurrence across all $N \ge 2$ signatories.
- **Quantified $\varepsilon$-Error Allowances:** Error tolerances are not nebulous percentages—they are closed integer intervals $[LO, HI]$ over a fixed-point basis (`CANOR_ONE = 1000` or per-category integer scales $\le 2^{20}$). Each party's measured orthonormalization error $E$ must strictly nest within its agreed allowance $A \subseteq B$ (Basis bound).
- **Interchange & Artifact Containers:** The `.fwo` compiled-object container groups earlier functional expressions with distinct JSON Object and XML Markup artifacts. While JSON/XML and in-browser output (WASM, WebM, HTML/CSS/JS) serve as standard inspection formats, native Zephyr RTOS targets and sandboxed sessions execute on compact binary/integer structures.

---

## 2. Intent Transliteration Procedure

When natural-language united/meta-schema intent is received:

### Step 1: Signatory & Agreement Deconstruction
Identify the contracting parties and their roles:
- Who are the signatories? ($N \ge 2$, e.g. Platform Instance, Autonomous Agent, Registered User). Are they internal or external?
- What underlying Maneuvers, Soul Symbols, or Flow bodies form the contract payload?
- What are the error categories? (e.g. `orthonormalization`, `latency_tolerance`, `reconciliation_drift`).
- What are the explicit tolerance intervals $[LO, HI]$ per signatory?

### Step 2: Digest & Artifact Manifest Formulation
- Build an ordered manifest of associated JSON/XML payloads, specifying whether they are embedded inline or referenced by relative path and checksum.
- Inspect the active profile (`.fwp`) for the canonical `@digest` algorithm. If unset, declare it as an open item without inventing an unratified hash.
- Enforce the **Unanimity Rule:** All signatories must ratify the identical digest, version, and basis. A single rejection rejects the Meta-Schema; any pending item marks it `incomplete`.

### Step 3: Advisory Synthesis & User Confirmation
Before generating artifacts, present:
1. **Strategic Assessment:** Executive evaluation of multi-party risk convergence, signatory equality, and contract enforceability.
2. **Signatory & Allowance Table:** Signatory → Role → Error Category → Allowance Interval $[LO, HI]$ → Ratification Status.
3. **Artifact Manifest:** Ordered listing of JSON/XML interchange components.
4. **Interactive Alignment:** Prompt the user to ratify the agreement parameters before dispatching to `/united`.

---

## 3. Real-World Walkthrough: Tri-Party Agentic KYC Accord

### Input Intent:
> `"/united-intent The firm, our self-hosted service instance, and an external enterprise agent need to co-sign a multilateral meta-schema for our onboarding protocol. We are packaging our kinetics settlement maneuver with a verified JSON schema of identity claims. Each party requires an orthonormalization error margin bounded between 0 and 25 ticks over CANOR_ONE. All three must unanimously approve before any program is assembled."`

### Advisory Output:

#### A. Strategic Executive Assessment
- **Contractual Integrity:** Tri-party signing ($N = 3$) establishes a balanced multilateral surface. By requiring each party's measured extraction error $E$ to remain within $[0, 25]$ (representing a maximum $2.5\%$ drift relative to `CANOR_ONE = 1000`), the accord guarantees that agentic execution remains strictly within platform risk limits.
- **Interchange Isolation:** The identity claims payload is packaged as an ordered JSON artifact (`accord.0.json`), ensuring external auditability without embedding unverified data into the core operational bytecode.

#### B. Signatory & Allowance Table

| Signatory | Participant Domain | Contractual Role | Error Category | Allowance Interval $[LO, HI]$ | Status |
|---|---|---|---|---|---|
| `firm` | System Architecture | Platform Operator & Root Anchor | `orthonormalization` | $[0, 25]$ | `pending` |
| `service_app` | Computational Subject | Internal Instance Engine | `orthonormalization` | $[0, 25]$ | `pending` |
| `ext_agent` | External Agent | Autonomous Transacting Participant | `orthonormalization` | $[0, 25]$ | `pending` |

*Overall Accord State:* `incomplete` (awaiting unanimous 3-party ratification).

#### C. Formal Transliteration

- **Header (`.<project>/united/kyc_accord.fwh`):**
  ```
  @provenance(spec="Canor §7", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
  @soul{~soul.agent_journey};
  @signatory.firm(role=Platform, external=off);
  @signatory.service_app(role=App, external=off);
  @signatory.ext_agent(role=Agent, external=on);
  @allowance.firm(category=orthonormalization, interval=[0, 25]);
  @allowance.service_app(category=orthonormalization, interval=[0, 25]);
  @allowance.ext_agent(category=orthonormalization, interval=[0, 25]);
  ```
- **Body (`.<project>/united/kyc_accord.fwb`):**
  ```
  @united.kyc_accord:@maneuver.kyc_settlement.agent_settle;
  @manifest{
    (index=0, kind=json, mode=embedded, path="kyc_accord.0.json", checksum=unset)
  };
  ```

Prompt the user: *"Would you like to formalize this United Meta-Schema in `.<project>/united/kyc_accord` or adjust the signatory error intervals?"*

---

## 4. Done When

- [ ] Narrative multi-party agreement is deconstructed into $N \ge 2$ signatories and closed error intervals.
- [ ] Ordered artifact manifest (JSON/XML) is established with checksum policies.
- [ ] Unanimous ratification protocol and status tracking are documented.
- [ ] Strategic executive assessment and formal declarations are presented for user confirmation.
