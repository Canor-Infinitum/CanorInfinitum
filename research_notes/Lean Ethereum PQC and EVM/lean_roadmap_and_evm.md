# Lean Ethereum Roadmap and Proposed Changes to the EVM (state as of 21 October 2026)

Status labels used throughout:
- **[SHIPPED]**: live on mainnet.
- **[DECIDED]**: Scheduled for Inclusion (SFI) in a named fork, or a stated EF commitment.
- **[PROPOSED]**: has an EIP or formal forum/blog proposal but is not scheduled. Includes CFI items.
- **[DECLINED]**: Declined for Inclusion (DFI) in a named fork.
- **[SPECULATIVE/RESEARCH]**: research posts, roadmap "strawman" items, and statements by individuals.

Source-quality note: primary sources were fetched for the EF blog (Lean Ethereum, 31 Jul 2025; realtime proving, 10 Jul 2025; Glamsterdam testnet, 17 Sep 2026; Protocol priorities, 7 Sep 2026), eips.ethereum.org (EIP-8081, EIP-7864), ethereum-magicians (RISC-V, 20 Apr 2025), ethresear.ch (Extremely Lean Chain, 6 Jul 2026), zkevm.ethereum.foundation (EIP-8025, 14 May 2026), strawmap.org, and leanroadmap.org. Items sourced only from crypto press are marked "(secondary)".

---

## 1. What is Lean Ethereum (lean consensus, lean data, lean execution), who proposed it, how does it relate to Beam Chain, what is the timeline, and is post-quantum an explicit pillar?

### Takeaway
"Lean Ethereum" is Justin Drake's (EF) 31 July 2025 vision for rebuilding all three L1 sublayers: lean consensus ("beacon chain 2.0", the renamed Beam Chain), lean data ("blobs 2.0") and lean execution ("EVM 2.0", a minimal SNARK-friendly ISA, possibly RISC-V). Hash-based post-quantum (PQ) cryptography is the unifying foundation. As of October 2026 it is an EF-endorsed multi-fork "strawmap", not shipped code. The EF Protocol cluster has committed to a **December 2029** target for a quantum-resistant L1 (fork L\*, five forks after Glamsterdam), with a "minimum viable PQ" safeguard at fork J\*.

### Cited Findings
**Origin and definition**
- [SPECULATIVE/RESEARCH, 12 Nov 2024] Justin Drake presented the **Beam Chain** at Devcon SEA (Bangkok) on 12 Nov 2024. It was a redesign of the consensus layer only, "leaving the EVM and the blob data layer untouched". Its stated goals were attester-proposer separation, faster slots, lowering the stake from 32 ETH to 1 ETH, SNARK-based "zero-knowledge consensus", and quantum-resistant cryptography. Péter Szilágyi warned against changing too much at once (secondary) — [Cointelegraph](https://cointelegraph.com/news/ethereum-beam-chain-consensus-redesign); [The Defiant](https://thedefiant.io/news/blockchains/justin-drake-proposes-beam-chain-overhaul-for-ethereum); [Unchained](https://unchainedcrypto.com/ethereum-researcher-justin-drake-proposes-beam-chain-redesign-but-not-all-ogs-are-onboard/)
- [SPECULATIVE/RESEARCH, 31 Jul 2025] Justin Drake's EF blog post "Lean Ethereum" (self-described as "a Drake take", i.e. personal vision) defines three L1 sublayers:
  - **Lean consensus** ("beacon chain 2.0"): finality in seconds, with hash-based aggregate signatures replacing BLS.
  - **Lean data** ("blobs 2.0"): PQ blobs and granular blob sizing, with hash-based DAS commitments replacing KZG.
  - **Lean execution** ("EVM 2.0"): a minimal, SNARK-friendly ISA that keeps EVM compatibility, possibly RISC-V, with real-time zkVMs replacing EVM re-execution.

  It states "Beam Chain" was formerly the branding for lean consensus. North stars: a gigagas/s L1 (~10K TPS) and a teragas/s L2 (~10M TPS) over a ~10-year horizon. Contacts: leanroadmap.org and lean@ethereum.org — [EF Blog: Lean Ethereum](https://blog.ethereum.org/2025/07/31/lean-ethereum)
- Drake's proposal makes a single hash function the core primitive across all L1 sublayers (secondary characterization) — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/)
- leanroadmap.org (lean consensus, snapshot ~May 2026) lists these pillars:
  - Lean Consensus
  - Lean Cryptography: "hash-based signatures for SNARKs and PQ security using one building block"
  - Lean Governance: bundling upgrades into fewer releases
  - Lean Craft: minimalism, modularity, formal verification

  Research tracks and progress:
  | Track | Progress |
  |---|---|
  | Poseidon cryptanalysis | 50% |
  | Hash-based multi-signatures (Winternitz/XMSS, lead Benedikt Wagner) | 70% |
  | PQ signature aggregation with zkVMs (lead Thomas Coratger) | 50% |
  | Lean 4 formal verification of FRI/STIR/WHIR | 40% |
  | P2P (4-second blocks, stake 32 ETH to 1 ETH) | 30% |
  | Attester-proposer separation | 20% |
  | 3-slot finality | 50% |

  — [leanroadmap.org](https://leanroadmap.org)

**Specs, clients and devnets (lean consensus)** [SPECULATIVE/RESEARCH: implementation in progress]
- Reference repos: leanSpec (consensus specs), leanSig (PQ signature reference implementation), leanMultisig (PQ aggregation), leanMetrics.
- Clients: Ream (Rust), Zeam (Zig), Qlean-mini (C++), Lantern (C), ethlambda (Rust, LambdaClass), gean (Go), Peam (Rust), Grandine, and a Lighthouse fork.
- Devnets:
  | Devnet | Date | Scope |
  |---|---|---|
  | pq-devnet-0 | Oct 2025 | no PQ sigs, 4 s slots, 3SF-mini |
  | pq-devnet-1 | Dec 2025 | leanSig integrated |
  | pq-devnet-2 | Jan 2026 | leanMultisig aggregation |
  | pq-devnet-3 | Feb 2026 | separate aggregator role |
  | pq-devnet-4 | target Mar 2026, "Active" | recursive PQ aggregation using leanVM |
  | pq-devnet-5 | planned | block-level aggregation proof; replace 3SF-mini with a "PQ heartbeat" under Goldfish fork-choice |

  — [leanroadmap.org](https://leanroadmap.org)
- More than 10 client teams run weekly PQ interop devnets as of mid-2026 (secondary) — [Coinpaprika](https://coinpaprika.com/news/ethereum-foundation-sets-2029-target-l1-quantum-upgrade/)
- The November 2025 lean consensus plan specified:
  - leanVM over the KoalaBear field, with POSEIDON_16/POSEIDON_24 precompiles.
  - A target of 1,000 signature verifications per second.
  - leanXMSS-style signatures of about 3,000 bytes, versus 96 bytes for BLS.

  (secondary summary of [hackmd.io/@tcoratger/ryS1ElrWbx](https://hackmd.io/@tcoratger/ryS1ElrWbx)) — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/)

**The 2026 "strawmap" and fork sequence**
- strawmap.org is maintained by "EF Architecture" (first names given: Justin, Thomas, Toni, Vitalik) and published by EF Protocol.
  - It originated at a **January 2026** EF workshop and promises "at least quarterly updates".
  - It outlines **seven forks by 2029** at a cadence of about 6 months. Only Glamsterdam and Hegotá have final names; I\*, J\*, (K\*) and L\* are placeholders, and L\* is tied to the larger lean consensus fork.
  - North stars:
    - "fast L1": inclusion and finality in seconds.
    - "gigagas L1": 1 gigagas/s via zkEVMs and real-time proving.
    - "teragas L2": 1 GB/s via DAS.
    - "post quantum L1": "centuries-long cryptographic security, via hash-based schemes".
    - "private L1": L1 shielded transfers.
  - It calls itself "not a prediction", a "strawman" to be treated "with healthy skepticism".

  — [strawmap.org](https://strawmap.org)
- [SPECULATIVE/RESEARCH, 4 Jul 2026] Vitalik Buterin promoted an updated Lean roadmap on X (with strawmap.org), described as a three-to-four-year overhaul. Press says it addressed all key feedback except tokenomics (secondary) — [The Defiant](https://thedefiant.io/news/blockchains/vitalik-buterin-outlines-lean-ethereum-roadmap-a-three-to-four-year-protocol-overhaul); [CryptoTimes](https://www.cryptotimes.io/2026/07/05/vitalik-buterin-unveils-lean-ethereum-roadmap-for-next-era/)
- [SPECULATIVE/RESEARCH, 6 Jul 2026] Vitalik's ethresear.ch post "The Extremely Lean Chain" is a design proposal, not an EIP.
  - Phase 1 cuts per-validator beacon state from about 121 bytes to **6 bytes**. The pubkey is replaced by a 5-byte deposit-tree index, plus a 1-byte effective balance.
  - Each validator submits a daily STARK proving its participation and new balance, and these proofs are aggregated on-chain.
  - Phase 2 cuts the state to 1 byte and adds daily re-anonymized validator keys using ZK-STARKs.
  - It cites "recursive-STARK-based quantum-resistant signature aggregation" and leanWOTS signatures. No calendar dates are given.

  Press figures conflict: 48→6 bytes, 114→6 bytes, "87%", "95%". The ~121-byte figure is the sum of the fields listed in the post — [ethresear.ch](https://ethresear.ch/t/the-extremely-lean-chain/25369); [The Block](https://www.theblock.co/post/407319/vitalik-buterin-extremely-lean-ethereum-shrinking-chain-to-near-zero-state-zk-proofs)

**Post-quantum as an explicit pillar, and the 2029 deadline**
- [DECIDED: EF self-commitment, 25 Mar 2026] The EF launched pq.ethereum.org, a post-quantum hub, with core PQ milestones targeted around 2029. Sources date it 24 or 25 March (secondary) — [Coinpaprika](https://coinpaprika.com/news/ethereum-foundation-sets-2029-target-l1-quantum-upgrade/); [CoinAlertNews](https://coinalertnews.com/news/2026/03/24/ethereum-post-quantum-security-roadmap-2029)
- [DECIDED: EF self-commitment, 7 Sep 2026] The EF Protocol cluster's "Current and Emerging Priorities" post:
  - **Target and planning assumptions:**
    - L1 quantum-resistant across execution, consensus and data by **December 2029**, a "self-imposed deadline".
    - It plans for Q-day "as early as 2030", which it calls deliberately aggressive.
    - The deadline is treated as non-negotiable at least until **January 2027**, when outside experts will reassess quantum progress.
  - **Fork mapping:**
    | Fork | Contents |
    |---|---|
    | I\* | PQ public-key registry; "decoupled consensus" is the leading headliner candidate; design of the state-trie migration begins |
    | J\* | "MV-PQ" temporary safeguard: PQ heartbeat (consensus) + leanDA sampling (data) + leanSPHINCS transactions (execution) |
    | K\* | mandatory execution proofs (current ordering) |
    | L\* | full PQ readiness, including PQ attestations |
  - **Still open:**
    - Swapping the PQ-attestation and mandatory-proof milestones is "in review".
    - The fork order after J\* will be "finalized once research matures".
    - The reduced guarantees of MV-PQ are still being defined.

  — [EF Blog: Protocol priorities](https://blog.ethereum.org/2026/09/07/protocol-priorities)
- MV-PQ at J\* was added in the **19 Aug 2026** strawmap update. Meeting L\* by Dec 2029 needs an average fork cadence of about 7.2 months (secondary) — [The Block](https://www.theblock.co/news/ecosystems/2026-09-08-ethereum-foundation-quantum-resistance-2029-413716); [CryptoSlate](https://cryptoslate.com/ethereums-2029-quantum-deadline-is-starting-to-reshape-what-gets-built-next/)
- One press summary gives preliminary production targets of leanVM in 2027 and consensus, data and execution in 2028. This conflicts with the EF's Dec 2029 framing and is probably stale (secondary) — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/)

### Inferences
- Beam Chain (Nov 2024) is **superseded by name**, not by substance: it became "lean consensus" inside the wider Lean Ethereum program (Jul 2025), which added lean data and lean execution.
- PQ moved from one motivation among several (2024–25) to the organizing deadline of the whole roadmap (Mar–Sep 2026). The Dec 2029 date is an EF internal target, not an All Core Devs (ACD) fork decision. No fork from I\* onward has EIPs scheduled.
- The lean consensus fork is effectively L\*, the fifth fork after Glamsterdam. On the EF's own cadence math, that lands in 2029 at the earliest.

### Gaps
- I did not fetch pq.ethereum.org or Vitalik's 4 Jul 2026 X post directly. Their exact wording comes from press.
- The strawmap page as scraped did not itemize the lean execution and lean data contents, and its latest revision date was not visible.
- The exact date the strawmap first went public (vs. the January 2026 workshop) is unclear.
- Whether K\* exists as a separately named slot in the strawmap is inferred from the EF post's mention of "K\*".

---

## 2. What has Vitalik proposed for the EVM itself (RISC-V, EOF, EVM simplification)? What is decided vs. still debated as of 2026?

### Takeaway
Nothing that replaces the EVM is scheduled in any fork. Vitalik's RISC-V replacement is a long-term proposal:
- 20 Apr 2025: an ethereum-magicians post.
- 1 Mar 2026: an X post adding a three-stage path, with the binary state tree (EIP-7864) as the nearer-term step.

The proposal is contested (Offchain Labs/Arbitrum argue for WASM; prover engineers dispute the gains). EOF was pulled from Fusaka in April 2025 and does not appear in Glamsterdam or Hegotá. Ongoing EVM work is incremental: small opcodes, gas repricing, and SELFDESTRUCT/refund cleanup.

### Cited Findings
**RISC-V proposal**
- [SPECULATIVE/RESEARCH, 20 Apr 2025] Vitalik's post "Long-term L1 execution layer proposal: replace the EVM with RISC-V":
  - **What changes:** contracts would be RISC-V. Accounts, calls and storage stay the same, and opcodes like SLOAD/SSTORE/CALL become syscalls. Solidity and Vyper would gain RISC-V backends.
  - **Why:** a 100x prover-efficiency gain overall needs at least 50x on EVM proving. Gains "over 100x" are cited only in limited cases, and he expects real-world gains to be smaller.
  - **Transition options:**
    - A dual VM.
    - EVM contracts as thin wrappers around an EVM interpreter written in RISC-V.
    - "Enshrined VM interpreters", with the EVM first and others (e.g. Move) possible later.
  - **State tree:** replace the keccak hexary MPT with a binary tree using a prover-friendly hash such as Poseidon. The post cites about 2M Poseidon hashes/s vs about 15K keccak hashes/s on a laptop, and proposes dropping bloom filters.

  — [Ethereum Magicians](https://ethereum-magicians.org/t/long-term-l1-execution-layer-proposal-replace-the-evm-with-risc-v/23617)
- Immediate objections (20–21 Apr 2025), all in the same thread — [Ethereum Magicians](https://ethereum-magicians.org/t/long-term-l1-execution-layer-proposal-replace-the-evm-with-risc-v/23617):
  - **benaadams:** u256 operations rely on AVX2/AVX-512, so decomposing to RISC-V could hurt block building and validation.
  - **levs57** ("Please don't"): the proving-gain assumptions are flawed, precompiles would still be needed, RISC-V zkVMs are not production-ready, and an EOF-like ISA with static jumps would fit better.
  - **SirSpudlington:** proposed Cairo or a 256-bit-register ISA instead.
  - **pkieltyka:** proposed Stylus/WASM.
  - **eigmax:** argued the MPT cost and the ISA question are separate issues.
  - **Vitalik's reply:** ZK-EVMs already compile EVM implementations to RISC-V, so exposing RISC-V removes a layer.
- [SPECULATIVE/RESEARCH, Nov 2025] Offchain Labs (Arbitrum) published a rebuttal arguing WebAssembly is the better long-term contract format. Its point is that the "delivery ISA" and the "proving ISA" need not be the same (secondary) — [The Block](https://www.theblock.co/post/391681/vitalik-buterin-lays-out-a-two-part-plan-to-overhaul-ethereums-execution-layer-from-the-ground-up)
- [SPECULATIVE/RESEARCH, 1 Mar 2026] Vitalik's X post set out a two-part execution-layer plan:
  - **Part 1:** binary state tree EIP-7864.
  - **Part 2:** eventually replace the EVM with RISC-V in three stages:
    1. RISC-V for precompiles only.
    2. Users may deploy RISC-V contracts.
    3. The EVM is retired by turning it into a smart contract running on the new VM.
  - He says the state tree plus the VM make up more than 80% of the proving bottleneck.
  - Press reports that the VM transition lacks broad developer consensus (secondary) — [The Block](https://www.theblock.co/post/391681/vitalik-buterin-lays-out-a-two-part-plan-to-overhaul-ethereums-execution-layer-from-the-ground-up); [PANews](https://panews.io/articles/019cb265-5c5f-7134-b342-a2f5a506c487)
- [PROPOSED] **EIP-7864** "Ethereum state using a unified binary tree":
  - Status: Draft, created 2025-01-20. Authors include Vitalik Buterin, Guillaume Ballet, Dankrad Feist and Ignacio Hagopian.
  - Design: a binary tree with 31-byte stems grouping 256 values, no RLP and no extension nodes. It depends only on hashes, not elliptic curves.
  - Migration: the MPT is frozen and migration is deferred to EIP-7748.
  - Costs: code-chunk access gets more expensive, and in-EVM proofs of historical state would break.

  — [EIP-7864](https://eips.ethereum.org/EIPS/eip-7864)
- EIP-7864 is **not listed** in the Hegotá meta-EIP (no SFI, CFI or DFI) — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081). The EF says "major state trie migration design begins" at I\* — [EF Blog: Protocol priorities](https://blog.ethereum.org/2026/09/07/protocol-priorities)
- The strawmap's gigagas north star is "via zkEVMs and real-time proving". The scraped page lists no RISC-V item — [strawmap.org](https://strawmap.org)
- A community ethresear.ch post "Lean Execution" (user conalloreilly, 6 Jul 2026) frames lean execution as "minimal SNARK-friendly high-throughput execution capacity". It discusses native rollups via an EXECUTE precompile and a "verkle-like" vector-commitment state. It does not mention RISC-V. It is not EF policy — [ethresear.ch](https://ethresear.ch/t/lean-execution-a-holistic-approach-to-secure-efficient-adaptive-and-resourceful-execution-throughput-to-scale-the-world-computer/25374)

**EOF (EVM Object Format)**
- [DECLINED for Fusaka, Apr 2025] Core developers removed EOF from Fusaka (recorded in an EIP-7607 PR and announced by Tim Beiko) to protect the timeline and because of the unresolved debate over EOF variants. It was left open to be re-pitched for Glamsterdam (secondary) — [The Defiant](https://thedefiant.io/news/blockchains/ethereum-removes-evm-object-format-fusaka-upgrade-eyes-glamsterdam-b97edac0); [CryptoSlate](https://cryptoslate.com/ethereum-drops-eof-from-fusaka-upgrade-after-community-pushback/); [Blockworks](https://blockworks.com/news/ethereum-consensus-evm-upgrade-fusaka-devs)
- EOF is **absent** from the final Glamsterdam EIP list in the 17 Sep 2026 EF announcement — [EF Blog: Glamsterdam testnet](https://blog.ethereum.org/2026/09/17/glamsterdam-testnet-announcement). It is also absent from Hegotá's SFI, CFI and DFI lists — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
- [DECIDED for Glamsterdam] **EIP-8024** "Backward compatible SWAPN, DUPN, EXCHANGE" ships deeper stack-access opcodes in legacy EVM — [EF Blog: Glamsterdam testnet](https://blog.ethereum.org/2026/09/17/glamsterdam-testnet-announcement)

**Incremental EVM changes actually scheduled**
- [SHIPPED, Fusaka, 3 Dec 2025] EIP-7939 CLZ (count leading zeros) opcode and EIP-7951 secp256r1 (P-256) precompile — [Alchemy](https://www.alchemy.com/blog/ethereum-fusaka-upgrade-dev-guide-to-12-eips); [EF Blog: Fusaka mainnet](https://blog.ethereum.org/2025/11/06/fusaka-mainnet-announcement)
- [DECIDED, Glamsterdam] EVM changes: EIP-7708 (ETH transfers emit a log), EIP-7843 (SLOTNUM opcode), EIP-7954 (larger maximum contract size), EIP-7997 (deterministic factory contract), EIP-8024 (SWAPN/DUPN/EXCHANGE) and EIP-8246 (remove SELFDESTRUCT burn).
- [DECIDED, Glamsterdam] Gas repricing: EIP-2780, EIP-7778 (block gas accounting without refunds), EIP-7976, EIP-7981, EIP-8037 (state-creation gas increase) and EIP-8038 (state-access gas update).

  — [EF Blog: Glamsterdam testnet](https://blog.ethereum.org/2026/09/17/glamsterdam-testnet-announcement)
- [PROPOSED, Hegotá CFI] Includes:
  - EIP-3298 (remove storage-clear refund)
  - EIP-4758 (deactivate SELFDESTRUCT)
  - EIP-5920 (PAY opcode)
  - EIP-7668 (remove bloom filters)
  - EIP-7709 (BLOCKHASH from storage)
  - EIP-7979 (call/return opcodes)
  - EIP-8151 (account-code-restricted ecrecover)
  - EIP-8163 (reserve EXTENSION 0xae opcode)
  - EIP-8298 (SETCODEFROM)
  - EIP-8360 (TCREATE)

  — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
- [DECLINED, Hegotá DFI] EIP-2488 (deprecate CALLCODE), EIP-7666 (EVM-ify the identity precompile), EIP-7807 (SSZ execution blocks), EIP-7923 (page-based memory costing), **EIP-8200 "EVMification"**, EIP-8219 (checked arithmetic opcodes) — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)

### Inferences
- **Decided:** incremental legacy-EVM evolution (Fusaka and Glamsterdam opcodes and repricings). EOF as a container format is effectively shelved: it has not been in any fork list since April 2025. EIP-8024 and the CFI'd EIP-7979 look like EOF features retrofitted piecemeal onto legacy EVM. That reading is based on the EIP titles; I did not verify it against EIP text.
- **Debated:** the RISC-V (or WASM, or custom-ISA) replacement. It has no EIP and no fork slot. The concrete precursor, EIP-7864, is still Draft, with its hash undecided and its migration unscheduled. The EF post puts state-trie migration design at I\* at the earliest, so a VM swap would come after that, beyond 2028.
- The repricing wave in Glamsterdam (state growth, calldata, access lists) fits the "price by proving cost and resources" logic of the lean execution vision, even though it is not branded "lean".

### Gaps
- I did not fetch the original 1 Mar 2026 X post. The three-stage RISC-V detail comes from The Block's write-up.
- I could not confirm the current EIP-7607 or EOF-EIP status labels (e.g. Stagnant or Withdrawn), or whether EOF was formally re-pitched and declined for Glamsterdam.
- The content of EIP-8200 "EVMification" was not read, so I am not characterizing it beyond its title.
- No source found on whether the strawmap assigns RISC-V or "lean execution ISA" work to a specific fork.

---

## 3. What is the state of zkEVM / real-time proving and the plan for L1 to verify zk proofs of execution? How does this interact with PQ (hash-based STARKs vs pairing SNARKs)?

### Takeaway
Real-time proving (99% of blocks in ≤10 s on ≤10 kW hardware) was declared achieved by December 2025. EF focus then shifted to provable soundness (≥100-bit by May 2026, 128-bit and <300 KiB proofs by end-2026). The opt-in step, EIP-8025 "Optional Execution Proofs", was **Declined for Inclusion in Hegotá**. Under the current EF ordering, **mandatory execution proofs** sit at K\* (possibly L\*), roughly 2028–29. PQ requirements push L1 toward hash-based proof systems.

### Cited Findings
- [DECIDED: EF target-setting, 10 Jul 2025] "Shipping an L1 zkEVM #1: Realtime Proving" sets these targets:
  - Latency of ≤10 s, given 12 s slots and ~1.5 s propagation, for at least 99% of mainnet blocks.
  - Hardware at ≤10 kW, with press citing a ~$100k capex cap.
  - 128-bit security long-term, with a 100-bit minimum for the initial months.
  - Proofs under 300 KiB, as summarized by press.

  — [EF Blog: Realtime proving](https://blog.ethereum.org/2025/07/10/realtime-proving); [The Block](https://www.theblock.co/post/362170/ethereum-zkevm-zk-proofs)
- [SHIPPED off-chain / benchmark milestone, 18 Dec 2025] "zkEVM Security Foundations" reports:
  - Ecosystem proving latency fell from 16 minutes to 16 seconds, and cost fell 45x.
  - zkVMs now prove 99% of blocks in under 10 s on target hardware (measured on Ethproofs).
  - New milestones: soundcalc integration (Feb 2026), ≥100-bit provable security by May 2026, 128-bit with <300 KiB proofs by end-2026.
  - Many STARK zkEVMs relied on unproven conjectures (proximity gaps) that recent research has challenged.

  — [EF Blog: zkEVM security foundations](https://blog.ethereum.org/2025/12/18/zkevm-security-foundations); [CryptoSlate](https://cryptoslate.com/ethereum-foundation-refocuses-to-security-over-speed-sets-strict-128-bit-rule-for-2026/)
- [SPECULATIVE: vendor claim] SP1 Hypercube claims to be the first general-purpose hash-based zkVM free of proximity-gap conjectures. This is unverified (secondary) — [CryptoSlate](https://cryptoslate.com/ethereum-foundation-refocuses-to-security-over-speed-sets-strict-128-bit-rule-for-2026/)
- [PROPOSED → DECLINED for Hegotá] **EIP-8025 "Optional Execution Proofs"**, described in an EF zkEVM team post (Ladislaus von Daniels, 14 May 2026):
  - **What it changes:** consensus-layer only. Opt-in provers gossip zkEVM proofs, and opt-in "zkAttesters" attest based on verified proofs.
  - **Optional phase:** re-execution still governs and proofs are a "supplementary check". Mandatory proofs are "a separate decision for a later EIP and a later fork".
  - **Plans at the time:** a Lighthouse+Prysm multi-client devnet and an EF on-prem GPU prover cluster. ePBS would widen the proving window.

  — [zkevm.ethereum.foundation](https://zkevm.ethereum.foundation/blog/eip-8025-optional-execution-proofs-hegota); [EIP-8025](https://eips.ethereum.org/EIPS/eip-8025)
- The Hegotá meta-EIP lists **EIP-8025 under Declined for Inclusion**, alongside EIP-8200 (EVMification) and EIP-7807 (SSZ execution blocks) — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
- A secondary source describes a working "3-of-5" multi-proof threshold for zkAttesters. This is a working assumption, not a parameter — [Base58Labs / secondary](https://base58labs.com/research/eip-8025-and-the-unbundling-of-ethereum-block-verification-optional-execution-proofs-as-an-architectural-safety-valve-1)
- [DECIDED as EF plan, not ACD] Mandatory execution proofs are at K\* under the current ordering. A swap with PQ attestations, which would push mandatory proofs to L\*, is "in review" — [EF Blog: Protocol priorities](https://blog.ethereum.org/2026/09/07/protocol-priorities). Developers have not approved that swap (secondary) — [CryptoSlate](https://cryptoslate.com/ethereums-2029-quantum-deadline-is-starting-to-reshape-what-gets-built-next/)
- The strawmap's "gigagas L1" (1 gigagas/s, 10K TPS) is to be reached "via zkEVMs and real-time proving" — [strawmap.org](https://strawmap.org)
- The PQ framing: the Lean Ethereum vision makes hash-based cryptography the foundation. It replaces BLS with hash-based aggregate signatures and KZG with hash-based DAS commitments — [EF Blog: Lean Ethereum](https://blog.ethereum.org/2025/07/31/lean-ethereum). The strawmap's PQ north star is "via hash-based schemes" — [strawmap.org](https://strawmap.org)
- Signature aggregation for lean consensus uses recursive STARKs in leanVM (pq-devnet-4) — [leanroadmap.org](https://leanroadmap.org); [ethresear.ch](https://ethresear.ch/t/the-extremely-lean-chain/25369)

### Inferences
- The L1 verification path is staged:
  1. Off-chain benchmarks ([SHIPPED] on Ethproofs).
  2. Optional proofs (EIP-8025, declined for Hegotá, so at I\* or later if revived).
  3. Mandatory proofs (K\* or L\*, ~2028–29).

  The Hegotá decline pushes even the opt-in phase past 2027.
- On PQ:
  - Pairing-based SNARK wrappers (e.g. Groth16/PLONK over BN254 or BLS12-381) and KZG are not quantum-safe. Any proof that L1 verifies natively after the PQ deadline would have to be hash-based (STARK/FRI/WHIR-family).
  - This is why EF security work targets FRI/STIR/WHIR soundness (formal verification in Lean 4) and why proof size (<300 KiB) is a constraint: hash-based proofs are larger than pairing proofs.
  - This is my inference from the cited goals. I found no single EF document stating "no pairing wrappers" verbatim.

### Gaps
- No source found confirming whether zkVM teams actually met the May 2026 100-bit milestone, or the current share of teams at 128-bit.
- The date and reason for EIP-8025's decline for Hegotá were not found. It was being proposed on 14 May 2026 and is DFI by Oct 2026.
- The exact proof-system requirements for mandatory proofs (e.g. multi-proof k-of-n, whether pairing wrappers are allowed) are not specified in any source I found.

---

## 4. Which hash functions are proposed for the lean/zk era (Poseidon2 vs Keccak/SHA3/SHA-2/BLAKE), and why? Status and security concerns.

### Takeaway
The lean plan through mid-2026 leaned on Poseidon-family hashes over the KoalaBear field: leanVM POSEIDON_16/24 precompiles, and Poseidon2 as a candidate for the binary tree. The EF's Poseidon Cryptanalysis Initiative (Nov 2024 to Dec 2026) found issues in Poseidon2 serious enough to discuss more rounds or a revert to Poseidon1, though no practical break of recommended full-round parameters. On **13 Aug 2026** Justin Drake announced the EF is **abandoning Poseidon for L1 in favor of SHA or BLAKE**, citing binary-field SNARKs that make standard hashes cheap to prove. No specific function or parameter set has been fixed, and clients have not agreed.

### Cited Findings
- [SPECULATIVE/RESEARCH, 20 Apr 2025] Vitalik's RISC-V post proposed a binary tree with a prover-friendly hash "such as Poseidon" (≈2M Poseidon/s vs ≈15K keccak/s on a laptop) — [Ethereum Magicians](https://ethereum-magicians.org/t/long-term-l1-execution-layer-proposal-replace-the-evm-with-risc-v/23617)
- [PROPOSED] EIP-7864 (Draft) does **not** fix a hash. Its reference implementation uses BLAKE3 "to ease experimentation", Keccak and Poseidon2 are listed as candidates, and it warns not to assume BLAKE3 is final — [EIP-7864](https://eips.ethereum.org/EIPS/eip-7864)
- [SPECULATIVE/RESEARCH, 1 Mar 2026] Vitalik named BLAKE3 or a Poseidon variant for the tree, the latter "still requires more security review". Changing the hash could add a 3x to 100x proving-efficiency gain (secondary) — [The Block](https://www.theblock.co/post/391681/vitalik-buterin-lays-out-a-two-part-plan-to-overhaul-ethereums-execution-layer-from-the-ground-up)
- **Poseidon Cryptanalysis Initiative:**
  - **Who and when:** launched November 2024 by George Kadianakis, Dmitry Khovratovich and Antonio Sanso, with an external advisory board, and scheduled to run to December 2026.
  - **Phase 2:** focuses on Poseidon1 over KoalaBear.

  — [poseidon-initiative.info](https://www.poseidon-initiative.info/); [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/). leanroadmap.org (~May 2026) shows the track at 50% with "$66k earned" — [leanroadmap.org](https://leanroadmap.org)
- Initiative results and budgets:
  - Announced budget ceilings total about $1.36M, including a $992K Poseidon1 collision prize. These are ceilings, not confirmed disbursements.
  - The first collision-prize tier ($32K) was claimed **6 Apr 2026** with a three-element partial collision against the full-round Poseidon1 compression function.
  - CICO solutions were verified in April, May and July 2026, and zero-test records reached 12 partial rounds by 27 Jul 2026.
  - The EF **paused the Poseidon1 Collision Prize on 1 Aug 2026**.
  - Grassi, Koschatko and Rechberger found Poseidon and Poseidon2 secure against the algebraic attacks they analyzed. Gröbner-basis cost estimates remain imprecise.

  (secondary, detailed) — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/)
- [SPECULATIVE: individual statement, Feb 2026] Vitalik said the Poseidon2 cryptanalysis program had found issues needing either extra rounds or a revert to Poseidon1. He explained his resistance to a Poseidon precompile on the EL (Solana, Starknet and Stellar ship one). Original post not located (secondary) — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/)
- [DECIDED as EF research direction, not ACD; 13 Aug 2026] Justin Drake on X:
  - **The announcement:** the EF is abandoning Poseidon as the L1 hash candidate and moving to SHA or BLAKE. He called it the end of an "eight-year, eight-figure effort".
  - **Why:** not a practical break, but "hash-friendly SNARKs" over binary extension fields (Binius, EUROCRYPT 2025; Binius64; Succinct's Flock prototype, June 2026, eprint 2026/1329) that make standard hashes cheap to prove.
  - **What is not settled:** coverage varies between SHA-2/SHA-256, BLAKE2s and BLAKE3, and no function or parameters have been named. SHA3/Keccak is not presented as the candidate. Nothing is deployed, and clients have not agreed.

  — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/) (cites [x.com/drakefjustin/status/2087905684180418733](https://x.com/drakefjustin/status/2087905684180418733)); [CryptoBriefing](https://cryptobriefing.com/ethereum-foundation-drops-poseidon-sha-blake/)
- Flock's authors do not recommend it for production. SNARK.fast throughput claims (1.8M BLAKE3 compressions/s) have no published methodology (secondary, caveat) — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/)
- The Nov 2025 lean consensus plan chose KoalaBear partly for efficient Poseidon2 S-boxes. After the pivot, the hash for leanSig/leanVM is "not yet specified" (secondary) — [PostQuantum.com](https://postquantum.com/security-pqc/ethereum-roadmap-drops-poseidon/)
- No EIP proposes a Poseidon precompile in Glamsterdam or Hegotá. EIP-5988 (Poseidon precompile) exists but appears in neither fork list — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081); [EF Blog: Glamsterdam testnet](https://blog.ethereum.org/2026/09/17/glamsterdam-testnet-announcement); [EIP-5988 discussion](https://ethereum-magicians.org/t/eip-5988-add-poseidon-hash-function-precompile/11772)

### Inferences
- The trade-off: Poseidon-family hashes are 100x+ cheaper inside prime-field STARKs, but are young and algebraically structured. SHA-2/BLAKE (and Keccak) have decades of cryptanalysis but were expensive to prove until binary-field proof systems matured. The August 2026 pivot trades proving cost for conservative security, consistent with the "centuries-long security" north star.
- Knock-on effects to watch:
  - The EIP-7864 hash choice (BLAKE3 is now favored over Poseidon2).
  - The leanSig/leanXMSS hash, which needs re-parameterization away from Poseidon2 over KoalaBear.
  - The leanVM field choice: KoalaBear vs binary fields.
- Keccak is unlikely to be the new L1 commitment hash. It remains the EVM's KECCAK256 opcode for compatibility, but no 2026 source proposes it as the lean-era tree or signature hash.

### Gaps
- I did not fetch Drake's 13 Aug 2026 X post or Vitalik's Feb 2026 statement directly. Both come via PostQuantum.com and press.
- No official EF blog post on the Poseidon pivot was found. The final choice among SHA-256, BLAKE2s and BLAKE3, and the treatment of leanSig, are unknown.
- I found no confirmed public attack paper breaking full-round recommended Poseidon2 parameters.

---

## 5. Which network upgrades shipped or are scheduled for 2025–2027 (Pectra, Fusaka, Glamsterdam, Hegotá, later), and which carry lean or post-quantum items?

### Takeaway
- **Shipped:** Pectra (7 May 2025) and Fusaka (3 Dec 2025). Neither carries lean or PQ items.
- **Glamsterdam:** headliners ePBS and BALs, plus a large gas-repricing set. On Sepolia since 6 Oct 2026, with mainnet unscheduled (aimed at about Nov–Dec 2026). No lean or PQ items.
- **Hegotá** (2027): FOCIL plus Frame Transactions (EIP-8141) scheduled. The PQ-relevant ML-DSA precompile and optional execution proofs were declined.
- **PQ and lean items start at I\*** (PQ key registry), J\* (MV-PQ) and K\*/L\* (mandatory proofs, PQ attestations, lean consensus), targeting December 2029.

### Cited Findings
| Fork | Status | Date | Key contents | Lean / PQ items |
|---|---|---|---|---|
| **Pectra** | [SHIPPED] | 7 May 2025, 10:05:11 UTC, epoch 364032 | EIP-7702 (EOA code delegation), among others | None found |
| **Fusaka** | [SHIPPED] | 3 Dec 2025, 21:49:11 UTC, slot 13,164,544 | PeerDAS; EIP-7939 CLZ; EIP-7951 secp256r1 precompile; EOF removed Apr 2025 | None. PeerDAS is DAS but still KZG-based (inference) |
| **Glamsterdam** | [DECIDED], on testnet | Sepolia 6 Oct 2026 13:53:36 UTC; Hoodi and mainnet TBD | Headliners EIP-7732 ePBS and EIP-7928 BALs; repricing and EVM EIPs (see §2); EIP-7688, 8045, 8061, 8282 | None |
| **Hegotá** | [DECIDED] scope (SFI) | 2027; no dates in meta-EIP | SFI: EIP-7805 FOCIL, EIP-8141 Frame Transaction; about 20 CFI | PQ-relevant items DFI: EIP-8355 ML-DSA precompiles, EIP-8025 optional proofs, EIP-8182 private transfers |
| **I\*** | [SPECULATIVE] strawmap | ~2027–28 | Decoupled consensus (candidate); state-trie migration design | PQ public-key registry |
| **J\*** | [SPECULATIVE] strawmap | ~2028 | MV-PQ | PQ heartbeat, leanDA sampling, leanSPHINCS txs |
| **K\*** | [SPECULATIVE] | ~2028–29 | Mandatory execution proofs (may swap with L\*) | zk-verified execution |
| **L\*** | [SPECULATIVE] | Target Dec 2029 | Lean consensus fork; full PQ | PQ attestations |

Sources by row:
- **Pectra:** the EF announcement gives epoch 364032 on 7 May 2025 at 10:05:11 UTC; ethereum.org confirms the activation — [EF Blog: Pectra mainnet](https://blog.ethereum.org/2025/04/23/pectra-mainnet); [ethereum.org Pectra](https://ethereum.org/roadmap/pectra/)
- **Fusaka:** the activation slot and time come from the EF announcement, and the EIP list from Alchemy's guide (secondary). One Forklog piece says 17 Dec, which looks like an error — [EF Blog: Fusaka mainnet](https://blog.ethereum.org/2025/11/06/fusaka-mainnet-announcement); [Alchemy](https://www.alchemy.com/blog/ethereum-fusaka-upgrade-dev-guide-to-12-eips)
- **Glamsterdam:**
  - EIP list and Sepolia time: [EF Blog: Glamsterdam testnet](https://blog.ethereum.org/2026/09/17/glamsterdam-testnet-announcement)
  - The EF's 7 Sep post implies a December 2026 mainnet target: [EF Blog: Protocol priorities](https://blog.ethereum.org/2026/09/07/protocol-priorities)
  - EIPsInsight shows a "projected" 4 Nov 2026: [EIPsInsight](https://eipsinsight.com/upgrade/schedule)
  - Press reports Hoodi on 26 Oct 2026 and a mainnet aim of early December (secondary): [Bitbase](https://www.bitbase.com/news/ethereums-glamsterdam-upgrade-in-final-stretch-here-are-key-dates-to-watch)
  - Glamsterdam missed the earlier H1-2026 expectation: [The Block](https://www.theblock.co/post/391681/vitalik-buterin-lays-out-a-two-part-plan-to-overhaul-ethereums-execution-layer-from-the-ground-up)
- **Hegotá:**
  - Scope lists: [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
  - Headliners FOCIL and EIP-8141: [EF Blog: Protocol priorities](https://blog.ethereum.org/2026/09/07/protocol-priorities)
  - EIPsInsight projects 19 May 2027 (projected only): [EIPsInsight](https://eipsinsight.com/upgrade/schedule)
  - EIP-8141 moved to SFI on ACD #244, 27 Aug 2026 (secondary): [CryptoDaily](https://cryptodaily.co.uk/2026/08/hegota-focil-frame-transactions-l1-privacy)
- **I\* to L\*:** [EF Blog: Protocol priorities](https://blog.ethereum.org/2026/09/07/protocol-priorities); [strawmap.org](https://strawmap.org)

Additional findings:
- The EIP-8081 meta-EIP (Draft; authors Beiko, Stokes, Dietrichs, Nixo, Jayanthi) has a blank activation table. Hegotá is not yet client-scheduled — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
- The EF notes that SFI, CFI and PFI "do not substitute for implementation evidence" — [EF Blog: Protocol priorities](https://blog.ethereum.org/2026/09/07/protocol-priorities)
- The EIP-8025 blog said ePBS (Glamsterdam) would give provers a more generous window. Glamsterdam's ePBS is therefore an enabler for later zk-proof verification, though not itself a lean item — [zkevm.ethereum.foundation](https://zkevm.ethereum.foundation/blog/eip-8025-optional-execution-proofs-hegota)

### Inferences
- No fork shipped or scheduled through Hegotá (2027) carries a PQ cryptography change or a lean-consensus component. The first PQ protocol item in the EF plan is the I\* PQ key registry. If the cadence holds, I\* is roughly 2027–28 (inference from the 6–7-month cadence).
- EIP-8141 Frame Transactions (native account abstraction, SFI for Hegotá) is the nearest execution-layer enabler for PQ accounts. It would let accounts validate with non-ECDSA signatures. This is my inference from the EIP's role; I did not read the EIP-8141 text. The related CFI EIP-8151 ("Account Code Restricted ecRecover") was described in press as a "post-quantum path for ecrecover" — [Etherspot](https://etherspot.io/blog/eip-8141-advances-toward-hegota-inclusion-a-composable-take-on-native-aa-glamsterdam-repricing-tests-compatibility-a-post-quantum-path-for-ecrecover/)
- Declining EIP-8355 (ML-DSA precompiles) for Hegotá suggests the EF and ACD favor hash-based signatures (leanSPHINCS, leanXMSS) over lattice-based standards at L1. That is an inference: I found no stated rationale for the decline.

### Gaps
- I found no confirmed Glamsterdam mainnet date (sources conflict: projected 4 Nov 2026 vs early December 2026) and no confirmed Hoodi date from a primary source.
- Fusaka's Blob-Parameter-Only (BPO) follow-up forks and their dates were not verified in this pass.
- The Hegotá date is projected only (19 May 2027 per EIPsInsight). No ACD-confirmed date was found.
- The full Pectra EIP list was not re-verified here beyond EIP-7702.
