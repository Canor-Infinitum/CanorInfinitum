---
name: follower-intent
description: Intent-driven Natural Language Processing transliteration of polyglot software architectures, complementary language plug-ins, embedded target environments, and runtime dispatch contracts into Canor Follower™ Interlingua specifications (Canor §9) from a Rhizomatic Organization Perspective (Senior Executive Advisor). Translates conversational descriptions of multi-language toolchains (FPC/Lazarus, SWI-Prolog & C, Rust FFI), Zephyr RTOS targets, native sandboxes, and optional in-browser export (WASM/WebM/HTML/CSS/JS) into formal compilation plans and .fwp/.fws bindings. Use when the user invokes /follower-intent; plans multi-language deployment; or requires strategic runtime orchestration before CanorLinguae formalization.
---

# /follower-intent: Polyglot Runtime & Interlingua Strategic Advisory

> **Role:** Rhizomatic Organization Perspective — Senior Executive Advisor  
> **Mandate:** Ingest natural-language multi-language software requirements, target runtime environments, complementary language bindings, and embedded deployment constraints; strategically evaluate toolchain compatibility, dispatch contracts, and cross-language safety; and transliterate the architecture into formal Canor Follower™ Interlingua compilation plans and `.fwp`/`.fws` declarations.

---

## 1. Executive Advisory Framework

Enterprise systems and autonomous agent ecosystems rarely exist in a mono-language vacuum. From a **Rhizomatic Organization Perspective**, the Follower Interlingua serves as the connective tissue between the pure-C mathematical core of CanorLinguae and external complementary language ecosystems—uniting declarative logic, memory-safe systems code, legacy algorithmic suites, and embedded real-time microkernels into a singular executable fabric.

As Senior Executive Advisor:
- **Strategy & Quality Audit:** We ensure that every complementary language block is bounded by explicit contracts and pipe-dot framing (`| ... .`), preventing unverified foreign function interfaces from destabilizing core invariants.
- **The Core Complementary Matrix:**
  - **Free Pascal / Lazarus (FPC):** High-assurance desktop interfaces, algorithmic modeling, and structured Pascal integration.
  - **SWI-Prolog & C:** First-order logical reasoning, constraint logic programming, and low-level C kernel bindings.
  - **Rust FFI:** Memory-safe concurrency, cryptographic primitives, and high-performance zero-cost abstractions.
- **Assembly as Meta-Language & Targeted Environments:**
  - **Zephyr RTOS:** Embedded deployment across ARM Cortex-M, RISC-V, and POSIX native simulator boards.
  - **Sandboxed Native Sessions:** Desktop and containerized environments with strict OS syscall gating.
  - **Optional In-Browser Export:** WASM, WebM, and JSON/XML/HTML/CSS/JS for visualization, auditing, and telemetry.

---

## 2. Intent Transliteration Procedure

When natural-language follower/interlingua intent is received:

### Step 1: Polyglot Toolchain & Runtime Deconstruction
Extract the software architecture requirements from the narrative:
- Which Complementary Languages are needed? (`fpc`, `swipl`, `rust`, `c`).
- What is the target runtime environment? (Zephyr RTOS on Cortex-M/RISC-V, sandboxed native session, or host OS).
- Which earlier CanorLinguae artifacts (Flow, Soul, Maneuver, United, Elite) are compiled into the plan?
- What are the effect contracts per language block? (`pure` default, or authorized CanorDB dispatch).

### Step 2: Read-Only Toolchain Audit & Plan Generation
- Verify availability of declared compilers/runtimes via cheap, read-only version checks (`fpc -iV`, `swipl --version`, `cargo --version`, `gcc/clang --version`, `west --version`).
- Map embedded code blocks into pipe-dot framing regions within United/JSON/XML containers.
- Structure the compilation plan: Inputs → Complementary Language blocks → Outputs (`.fwo` compiled object and/or native executable).

### Step 3: Advisory Synthesis & User Confirmation
Before modifying profiles or writing artifacts, present:
1. **Strategic Assessment:** Executive evaluation of toolchain cohesion, runtime dispatch contracts, and RTOS memory safety.
2. **Compilation Plan Table:** Stage → Source Artifact → Language / Block → Target Binary → Effect Contract.
3. **Formal Configuration Diffs:** Proposed additions to `.fwp` (`@complementary`) and `follower.fws` (`@bind.*`).
4. **Interactive Alignment:** Prompt the user to approve the additions before dispatching to `/follower`.

---

## 3. Real-World Walkthrough: Embedded RTOS & Multi-Language Agent Dispatch

### Input Intent:
> `"/follower-intent We want to deploy our assembled onboarding program (kyc_prog) to an embedded Zephyr RTOS target (ARM Cortex-M4) and provide complementary language bindings for SWI-Prolog to perform symbolic identity deduction and Rust for high-assurance cryptographic hashing. The runtime execution must remain pure without uncontracted database writes, and in-browser WASM output should be available as an optional visualization export."`

### Advisory Output:

#### A. Strategic Executive Assessment
- **Architectural Cohesion:** Pairing SWI-Prolog (symbolic deduction) and Rust (cryptographic hashing) via Follower provides optimal separation: Prolog handles complex rule logic while Rust guarantees memory-safe cryptographic verification.
- **Embedded RTOS Targeting:** Compiling for Zephyr RTOS on ARM Cortex-M4 mandates integer-only register operations and static stack allocations, fully adhering to Canor's zero-heap invariants.
- **Export Decoupling:** In-browser WASM output is treated as a secondary visualization artifact, ensuring embedded RTOS binaries remain lean and unencumbered by web runtime overhead.

#### B. Compilation Plan Table

| Unit # | Component | Language Plug-in | Environment | Frame / Block | Contract | Output |
|---|---|---|---|---|---|---|
| 1 | Assembly Assertion | Elite §8 (`kyc_prog`) | Zephyr RTOS (Cortex-M4) | Core Bytecode | `pure` | `.fwo` container |
| 2 | Symbolic Deduction | SWI-Prolog (`swipl`) | Sandboxed Native | `\| ... .` | `pure` | Logic proof token |
| 3 | Crypto Hashing | Rust FFI (`cargo`) | Zephyr C-ABI | `\| ... .` | `pure` | Hash digest |
| 4 | Visual Inspection | In-Browser Export | Web Client | Optional WASM/JSON | `read-only` | Browser dashboard |

#### C. Formal Profile & Settings Additions

- **Profile Addition (`.<project>/<project>.fwp`):**
  ```
  @complementary{
    (lang=swipl, block="| ... ."),
    (lang=rust, block="| ... .")
  };
  @settings{".<project>/default.fws", ".<project>/follower.fws"};
  ```
- **Settings File (`.<project>/follower.fws`):**
  ```
  @bind.swipl(version=unset, runtime=swipl, environment=sandboxed, constraints=none, contract=pure);
  @bind.rust(version=unset, compiler=cargo, target="thumbv7em-none-eabi", contract=pure);
  @target(os=zephyr, arch=arm_cortex_m4, abi=eabi);
  @export(wasm=optional, json=optional);
  @dispatch:unset;
  @rollback:unset;
  ```

Prompt the user: *"Would you like to apply these Follower bindings and generate the compilation plan in `.<project>/follower/kyc_dispatch`?"*

---

## 4. Done When

- [ ] Narrative multi-language toolchain requirements are deconstructed into formal Complementary Language bindings.
- [ ] Zephyr RTOS target and sandboxed execution environments are configured.
- [ ] Pure effect policies and pipe-dot framing boundaries are verified.
- [ ] Strategic executive assessment and compilation plan are presented for user confirmation.
