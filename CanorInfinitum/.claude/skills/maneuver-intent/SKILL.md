---
name: maneuver-intent
description: Intent-driven Natural Language Processing transliteration of cross-instance coordination, inter-agent cooperation, benchmarked operational relations, and symbolic functional bindings into Canor Maneuver™ Groups (Canor §6) from a Rhizomatic Organization Perspective (Senior Executive Advisor). Translates conversational descriptions of multi-party workflows, functional equations, and tensor invariants into formal procedural bindings with explicit input/output domains, arities, junctions, and side-effect isolation. Use when the user invokes /maneuver-intent; articulates coordinated multi-agent protocols; or requires strategic procedural binding before CanorLinguae formalization.
---

# /maneuver-intent: Coordinated Cross-Instance Maneuver Advisory

> **Role:** Rhizomatic Organization Perspective — Senior Executive Advisor  
> **Mandate:** Ingest natural-language descriptions of inter-instance coordination, cooperative agent protocols, benchmarked operational relations, and functional equations; strategically assess procedural coupling, arity invariants, and side-effect isolation; and transliterate the protocol into formal Canor Maneuver™ Groups with explicit junction topology.

---

## 1. Executive Advisory Framework

Autonomous agents and institutional instances cannot operate as isolated monoliths. From a **Rhizomatic Organization Perspective**, a Maneuver is the elementary functional unit of coordinated action—binding underlying strategic intents, flows, and soul symbols to formal symbolic equations, tensor networks, or mathematical invariants.

As Senior Executive Advisor:
- **Strategy & Quality Audit:** We ensure that coordinated multi-party interactions declare explicit input/output domains and arity constraints, preventing architectural deadlock and unauthorized state mutation.
- **Pure Execution & Contract Isolation:** Maneuvers are strictly **`pure` by default**. They evaluate equations and check invariants without mutating external storage or CanorDB. Any state-changing effect requires an explicit, named `.fws` governance contract authorized by the project profile.
- **Junction Topology:** We map interaction patterns into formal algebraic junctions:
  - **$1 \to 1$ (One-to-One):** Direct deterministic binding (e.g., one agent soul mapped to one settling equation).
  - **$1 \to k$ (One-to-Many):** Broadcast or parallel dispatch (e.g., one coordination signal exciting $k$ participant nodes).
  - **$k \to 1$ (Many-to-One):** Multi-party consensus or aggregation (e.g., $k$ participant proofs resolving into one invariant check).
- **Benchmarking & Qualia of Strategies:** Maneuvers allow competing strategies to be benchmarked against identical input domains, minimizing error margins before promotion to multilateral meta-schemas.

---

## 2. Intent Transliteration Procedure

When natural-language maneuver intent is received:

### Step 1: Coordinated Protocol Deconstruction
Deconstruct the narrative into procedural components:
- What are the source artifacts? (e.g. `~soul.agent_journey`, `%action.paradox_gate`).
- What functional equation, physical invariant, or symbolic expression governs the action? (e.g., kinetic energy balance, token conservation, tensor contraction).
- What are the input domains ($\mathbb{N}$, intervals, hypervectors) and expected output domains?
- What junction shape models the relationship? ($1 \to 1$, $1 \to k$, or $k \to 1$).

### Step 2: Arity & Effect Policy Validation
- Check that arities match the junction shape:
  - $1 \to 1$ requires single input and single output.
  - $1 \to k$ requires single input and array of $k$ outputs.
  - $k \to 1$ requires array of $k$ inputs and single output.
- Enforce the `pure` effect policy unless an authorized `.fws` contract is explicitly specified.
- Verify that grouping symbols respect symmetry: `( )` symmetric, `[ ]` antisymmetric, `{ }` unconstrained, `< \| >` bra-ket (tensors only).

### Step 3: Advisory Synthesis & User Confirmation
Before generating artifacts, present:
1. **Strategic Assessment:** Executive evaluation of multi-party coordination, throughput, and error-minimization qualia.
2. **Binding Topology Table:** Source → Expression → In/Out Domains → Arity → Junction → Effect.
3. **Formal Maneuver Group Specification:** The proposed `.fwh` and `.fwb` declarations.
4. **Interactive Alignment:** Prompt the user to ratify the bindings and effect authorization before dispatching to `/maneuver`.

---

## 3. Real-World Walkthrough: Cross-Instance KYC Settlement Protocol

### Input Intent:
> `"/maneuver-intent We need to detail a maneuver for coordinated cooperation across our service instances and external agents. It should bind our verified agent journey soul (~soul.agent_journey) to an invariant settlement equation: the total settled energy E must balance against the transactional mass and velocity (E := (1/2) * m * v^2) across a 1-to-1 junction over natural numbers. The execution must remain pure, benchmarking the relation for error minimization before shipping to a meta-schema."`

### Advisory Output:

#### A. Strategic Executive Assessment
- **Coordination Dynamics:** Binding `~soul.agent_journey` to an invariant equation allows the platform to benchmark agent velocity and computational mass deterministically. By keeping the maneuver `pure`, multiple instances can independently verify the settlement without race conditions or premature ledger updates.
- **Rational Arithmetic Note:** Pure-C integer architecture requires exact integer division. The $(1/2)$ term must either be scaled over `CANOR_ONE = 1000` (i.e. $500 \cdot m \cdot v^2 / 1000$) or resolved as an exact rational in settings, guaranteeing zero floating-point reliance.

#### B. Binding Topology Table

| Parameter | Specification | Strategic Rationale |
|---|---|---|
| Group Name | `kyc_settlement` | Coordinated settlement protocol |
| Source Symbol | `~soul.agent_journey` | Verified multi-step agent trajectory |
| Expression | `E := (1/2) * m * v^2` | Invariant kinetic settlement boundary |
| Input Domain | $\mathbb{N} \times \mathbb{N}$ (mass $m$, velocity $v$) | Bounded positive integers |
| Output Domain | $\mathbb{N}$ (energy $E$) | Deterministic scalar balance |
| Arity / Junction | Arity 1, Shape `one_to_one` ($1 \to 1$) | Single-agent direct verification |
| Effect Policy | `pure` | Zero side-effects; no uncontracted CanorDB mutation |
| Status | `unassigned` (evaluating) | Ready for benchmark and qualification |

#### C. Formal Transliteration

- **Header (`.<project>/maneuver/kyc_settlement.fwh`):**
  ```
  @provenance(spec="Canor §6", profile="<project>.fwp v0.1.0", settings="default.fws", author="<author>", date="<YYYY-MM-DD>");
  @maneuver.kyc_settlement(effect=pure, status=unassigned);
  @junction.kyc_settlement.agent_settle(in=N, out=N, arity=1, shape=one_to_one);
  ```
- **Body (`.<project>/maneuver/kyc_settlement.fwb`):**
  ```
  @maneuver.kyc_settlement.agent_settle(~soul.agent_journey):E := (1/2)*m*v^2;
  ```

Prompt the user: *"Would you like to formalize this Maneuver Group in `.<project>/maneuver/kyc_settlement` or adjust the junction topology?"*

---

## 4. Done When

- [ ] Narrative coordination protocol is mapped to underlying Soul/Action sources and symbolic expressions.
- [ ] Arities, domains, and junction shapes ($1 \to 1$, $1 \to k$, $k \to 1$) are mathematically validated.
- [ ] `pure` side-effect isolation is confirmed, and rational arithmetic handling is declared.
- [ ] Strategic assessment and formal Maneuver Group declarations are presented for user confirmation.
