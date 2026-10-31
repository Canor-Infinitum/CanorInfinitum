# Post-Quantum Signatures and Signature Aggregation in Ethereum Lean Consensus (leanSig, leanMultisig/leanVM, XMSS variants), as of 2026-10-21

Status labels used throughout:
- **DECIDED**: implemented in leanSpec or the pq-devnets, or publicly announced by the Ethereum Foundation (EF) as the chosen direction. Note that **nothing is decided for Ethereum mainnet**: no hard fork or accepted EIP deploys any of this. "DECIDED" means decided for the lean-consensus R&D track and devnets.
- **PROPOSED**: draft EIPs, roadmap targets, planned devnets, or parameter sets not yet in a running devnet.
- **RESEARCH**: papers, benchmarks, alternatives under study, and unreviewed claims.

Research date: 2026-10-21. Fetched pages were read on that date. Claims that come only from a search-result snippet, not a fetched page, are labelled "(snippet)".

## Q1. What is leanSig (scheme, parameters, sizes, costs, hash), why hash-based, and what is decided?

### Takeaway
leanSig is a **generalized XMSS** (Merkle tree of Winternitz-style one-time signatures using **Target-Sum / "top-level" incomparable encodings**) by Drake, Khovratovich, Kudinov and Wagner. It is the DECIDED signature for every pq-devnet since pq-devnet-1 (Dec 2025). Signatures shrank across devnets: 3,112 B (devnet-2), 2,536 B (devnet-4/5), then **1,208 B with 32-B public keys** on leanVM `main` (Jul–Sep 2026). The hash moved from Poseidon2 to Poseidon1 over KoalaBear, and then, after the EF's 2026-08-13 "goodbye Poseidon" pivot, to **BLAKE2s over binary fields**. BLAKE2s is labelled a placeholder: SHA2, SHA3 and BLAKE3 are still under consideration. Hash-based was chosen for conservative (hash-only, standard-model) security and because it fits a hash-based SNARK aggregation stack. Lattice and SLH-DSA alternatives lose on size, bandwidth or aggregate-verification cost.

### Cited Findings
**Papers (RESEARCH → basis of DECIDED design)**
- [RESEARCH, 2025-01-14 → rev. 2025-09-12] "Hash-Based Multi-Signatures for Post-Quantum Ethereum" by Justin Drake (EF), Dmitry Khovratovich (EF), Mikhail Kudinov (TU Eindhoven) and Benedikt Wagner (EF). ePrint 2025/055, published in IACR Communications in Cryptology (CiC) 2025, DOI 10.62056/aey7qjp10. It proposes XMSS variants as PQ replacements for BLS multi-signatures, in a unified framework that minimises security loss. Signatures are aggregated with hash-based succinct arguments. The security proof **avoids random oracles** and states explicit standard-model hash requirements, removing the conflict between treating the hash as a random oracle and needing it as an explicit circuit for aggregation. — [ePrint 2025/055](https://eprint.iacr.org/2025/055)
- [RESEARCH, received 2025-07-21, revised 2025-09-12] "Technical Note: LeanSig for Post-Quantum Ethereum" (same four authors), ePrint 2025/1332. It gives a new instantiation of the 2025/055 framework using a novel **incomparable encoding** that improves the trade-off between signature size and verification hashing, building on Khovratovich–Kudinov–Wagner (Crypto 2025, ePrint 2025/889). — [ePrint 2025/1332](https://eprint.iacr.org/2025/1332)
- [RESEARCH, 2025] In the paper's preliminary benchmarks, signatures are **1.01–4.17 KiB** and verification takes **22–543 µs** with SHA-3. Poseidon2 is about **10× slower** natively. Security targets are classical k_C = 128 and quantum k_Q = 64 (NIST Level 1). — [Survey, HackMD, dated 2026-08-26](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [RESEARCH, 2025] The paper's bandwidth model: a 32 KiB signature with a committee of 2^12 would require the aggregator to receive 2^30 bits within 1 s of a 4 s slot, which is infeasible. Signatures under about 4 KiB soften this. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)

**Encoding and parameters**
- [DECIDED for devnets] The encoding is **Target-Sum Winternitz (TSW)**. It fixes the sum of chain positions at T, so the verifier's worst-case hash count equals its average case. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [PROPOSED/"production set", EIP created 2026-06-19] The leanSig production parameter set is **LOG_LIFETIME = 32 (2^32 = 4,294,967,296 leaves), DIMENSION v = 46 chains, BASE w = 8, TARGET_SUM = 200**, matching leanSig's `SIGAbortingTargetSumLifetime32Dim46Base8` instantiation. — [EIP-8310 (Draft)](https://eips.ethereum.org/EIPS/eip-8310); [Ethereum Magicians EIP-8310 thread (snippet)](https://ethereum-magicians.org/t/eip-8310-post-quantum-keystore-for-stateful-keys/28853)
- [DECIDED for devnet-5] The devnet-5 standard-model parameters are **v = 46, w = 3** (read as 3 bits per chunk, i.e. base 8, consistent with EIP-8310). Signature is about **2,524 B** and public key **52 B**. The leanVM main branch uses **ROM parameters**: v = 42, signature **1,208 B**, public key **32 B**. Standard-model parameters cost about 16% of aggregation throughput and about 15% of proof size relative to ROM parameters. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [DECIDED for devnet-1, snippet] gean's pq-devnet-1 instantiation uses LOG_LIFETIME = 32 (2^32 signatures per key), citing leanSpec@050fa4a. — [gean DeepWiki](https://deepwiki.com/geanlabs/gean)
- [DECIDED for devnets] Public key is "8 elements for the root and 5 for the randomiser". Thirteen 4-byte KoalaBear elements give 52 B. — [leanroadmap.org](https://leanroadmap.org/)

**Size history across devnets**
- [DECIDED, devnet-2, Jan 2026] XMSS signatures were **3,112 B** each. — [LambdaClass ethlambda tooling blog (snippet)](https://blog.lambdaclass.com/ethlambda-building-a-post-quantum-ethereum-client-with-the-help-of-shared-tooling/)
- [DECIDED, devnet-4] Canonical SSZ signature is **2,536 B**. Deserializers also accept legacy 3,112-B buffers when the trailing bytes are zero. — [bitminetech/c-leanvm (snippet)](https://github.com/bitminetech/c-leanvm)
- [PROPOSED → adopted in clients, 2026-07-24 to 2026-09-18] ethlambda PR #539 tracks leanVM `main` at `a5909d1`. leanVM "internalized XMSS" into its own `xmss` crate, dropping the external leanSig dependency. `SIGNATURE_SSZ_LEN` goes from **2,536 to 1,208 B** and `PUB_KEY_SSZ_LEN` from **52 to 32 B**. The PR was closed unmerged on 2026-09-18, "Superseded by the new leanVM version using binary fields: #606". — [ethlambda PR #539](https://github.com/lambdaclass/ethlambda/pull/539)
- [DECIDED in ethlambda, opened 2026-09-03, ready 2026-09-25, merged with no date shown] ethlambda PR #606 "bump to new leanVM version" changes the hash from **Poseidon over KoalaBear to BLAKE2s over binary fields**, with public key 32 B and signature 1,208 B ("stayed at 32 and 1208 through a later bump"). It also says a key's bottom subtree covers 512 slots, for a key range of about 2^18. — [ethlambda PR #606](https://github.com/lambdaclass/ethlambda/pull/606)

**Signing and verification costs**
- [RESEARCH benchmark, 2026-02-09] On an M4 Max, signing takes about **0.53 ms** against a target under 0.5 ms, and verification about **0.19 ms** against a target under 0.5 ms. Signature is about 3 kB ("~2.5 KB" in the diagram), with an "8-year" key lifetime. — [LambdaClass, "Ethereum Signature Schemes Explained" (2026-02-09)](https://blog.lambdaclass.com/ethereum-signature-schemes-explained-ecdsa-bls-xmss-and-post-quantum-leansig-with-rust-code-examples/)
- [RESEARCH benchmark, page state ~May 2026] On a single M1 core, signing runs at **107% of target** and verification at **39% of target**. Key generation is measured on a 10-core M1 with an 8-year key lifetime. The absolute values do not render on the page. — [leanroadmap.org](https://leanroadmap.org/)
- [RESEARCH benchmark, Oct 2026 README] Verifying one leanXMSS signature costs about **201 BLAKE2s compressions**. At 485,765 compressions/s proven, that is equivalent to about 2,416.7 leanXMSS/s. — [leanEthereum/leanVM README](https://github.com/leanEthereum/leanMultisig)

**Hash function trajectory**
- [DECIDED for leanSig repo] leanSig instantiations use **Poseidon1**, not Poseidon2 (`instantiations_poseidon`, `instantiations_poseidon_top_level`). Parameters come from b-wagn/hashsig-parameters and b-wagn/hypercube-hashsig-parameters. The repo departs from the paper in leanVM-friendly ways ("overwrite" sponge, capacity-then-rate layout, reordered inputs). It is "not audited and not meant to be used in production". — [leanEthereum/leanSig README](https://github.com/leanEthereum/leanSig)
- [DECIDED for devnets up to 5] leanVM used the Poseidon permutation (width 16, 4+20+4 rounds, first-generation Poseidon) over KoalaBear for the tweakable hash, Merkle commitments, Fiat-Shamir and in-circuit verification. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [DECIDED as EF direction, 2026-08-13] Justin Drake announced: "The Ethereum Foundation is abandoning Poseidon for L1, pivoting to SHA or BLAKE." This was driven by binary-field SNARKs (Binius, Flock) that prove traditional hashes cheaply. There was no break in Poseidon and no mainnet change. — [Justin Drake on X](https://x.com/drakefjustin/status/2087905684180418733); [CryptoSlate, 2026-08-14](https://cryptoslate.com/ethereum-abandons-its-8-year-cryptography-bet-after-proof-systems-flip-the-tradeoff/)
- [RESEARCH, posted 2026-07-29] Flock (arXiv 2607.27491) reports, on one M4 Max core, about 82,000 BLAKE3 compressions/s, 42,000 SHA-256 compressions/s and 30,000 Keccak permutations/s. Ten cores exceed 660,000 BLAKE3 compressions/s. SHA-256 proving is more than 9× faster than Binius64. — [CryptoSlate](https://cryptoslate.com/ethereum-abandons-its-8-year-cryptography-bet-after-proof-systems-flip-the-tradeoff/)
- [PROPOSED, Oct 2026] The leanVM README calls BLAKE2s a **placeholder**, with SHA2, SHA3 and BLAKE3 under consideration. The final L1 hash is not fixed. EIP-8310 likewise leaves the hash field open ("Poseidon1 over KoalaBear still being evaluated" at its 2026-06-19 creation). — [leanVM README](https://github.com/leanEthereum/leanMultisig); [EIP-8310](https://eips.ethereum.org/EIPS/eip-8310)

**Why hash-based rather than lattice or SLH-DSA**
- [RESEARCH] Per the paper: conservative, hash-only assumptions with explicit standard-model requirements, and aggregation by hash-based succinct arguments. — [ePrint 2025/055](https://eprint.iacr.org/2025/055)
- [RESEARCH, survey 2026-08] **ML-DSA-65** (FIPS 204) has a 3,309-B signature and a 1,952-B public key, about **90 MB per slot** unaggregated for ~28k votes. Hash-based at 1.2–2.5 KB gives 34–70 MB per slot. **SLH-DSA** (FIPS 205) signatures are 7.9–49.9 KB, above the ~4 KiB bandwidth bound. **Falcon + LaBRADOR** compresses 10,000 Falcon-512 signatures to **74 KB**, but verification takes **2.65 s** and is hard to parallelise, against about **106 ms** for the hash route. Other lattice aggregation schemes: Squirrel (4,096 sigs → 771 KB), Chipmunk (8,192 sigs → <200 KB), Lemur (ePrint 2026/1161, claims 2^20 signers in 380 KB, unreviewed). — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [RESEARCH, 2026] Aerie (ePrint 2026/2501, Centelles) studies efficient aggregation of Falcon signatures. It benchmarks against leanVM's XMSS throughput and notes lean consensus targets 1,000 sigs/s (snippet). — [Aerie, ePrint 2026/2501](https://eprint.iacr.org/2026/2501.pdf)
- [RESEARCH] LambdaClass justifies hash-based signatures only as "SNARK-friendly". — [LambdaClass blog](https://blog.lambdaclass.com/ethereum-signature-schemes-explained-ecdsa-bls-xmss-and-post-quantum-leansig-with-rust-code-examples/)
- [RESEARCH] The Project Eleven PQ Crypto Registry lists leanSig with research status, "No benchmarks yet", and no audited implementation (snippet). — [Project Eleven registry](https://registry.projecteleven.com/algorithms/leansig)

### Inferences
- The 1,208-B size is consistent with v = 42 chain values plus a 32-level authentication path at 16-B (128-bit) nodes plus a short randomiser (42×16 + 32×16 = 1,184, plus 24). This is arithmetic only, not confirmed by a source. The 2,536-B devnet-4 size fits 32-B (8 KoalaBear element) nodes: 46×32 + 32×32 = 2,496, plus about 40 B.
- The 1,208-B / 32-B format appeared on leanVM `main` before the binary-field switch (PR #539, July 2026, predates PR #606). The size drop therefore comes mainly from ROM-model parameters and the internalized `xmss` crate, not from BLAKE2s itself.
- The 2,524 B (survey) vs 2,536 B (c-leanvm, ethlambda) gap of 12 B most likely reflects raw vs SSZ-encoded framing (offsets).
- Overall status: generalized XMSS with TSW encoding is DECIDED for the lean track. The exact parameters (standard-model vs ROM, v = 42 vs 46), the hash (BLAKE2s vs SHA2/SHA3/BLAKE3), and the field remain PROPOSED.

### Gaps
- The primary PDFs (ePrint 2025/055 Table 3, 2025/1332) returned HTTP 403. Exact per-parameter-set KiB tables, keygen times and verification hash counts for each instantiation were not retrieved from the primary source.
- No source gives the exact parameter set (v, w, T, node size, LOG_LIFETIME) of the binary-field/BLAKE2s `xmss` crate. The crate page returned 404.
- The date and rationale of the Poseidon2 → Poseidon1 switch in leanSig were not found.
- leanroadmap.org's absolute benchmark values do not render. Only "% of target" is visible.

## Q2. Stateful-signature handling: key lifetime, exhaustion, and state-reuse prevention

### Takeaway
Keys are **synchronized** (epoch/slot-indexed) XMSS. The production set has 2^32 leaves (LOG_LIFETIME = 32). The one-time leaf index is a deterministic function of the slot or epoch, so "never reuse a leaf" and "never double-sign a slot" are the same invariant, which slashing already enforces. The Draft EIP-8310 (created 2026-06-19) mandates commit-before-sign persistence, disjoint leaf ranges, no import resets, stale-restore detection, and hard refusal on exhaustion. Keys are activated over a sub-range (an "8-year" lifetime is quoted) and advanced incrementally.

### Cited Findings
- [DECIDED for devnets] Each public key is the root of a height-32 Merkle tree, a lifetime of 2^32 slots. The slot number serves as the one-time key index, and each validator signs at most once per slot, so statefulness is tied to protocol semantics. Double-signing a slot is already slashable. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [DECIDED for leanSig API] Each secret key and epoch pair **must not be used twice** in `sign`. Keys must be prepared via `sk.advance_preparation()`. Users sign in epoch order and advance once half the prepared interval has passed. — [leanSig README](https://github.com/leanEthereum/leanSig)
- [RESEARCH/benchmark context] An "8-year key lifetime" is quoted for leanSig key generation. — [leanroadmap.org](https://leanroadmap.org/); [LambdaClass blog, 2026-02-09](https://blog.lambdaclass.com/ethereum-signature-schemes-explained-ecdsa-bls-xmss-and-post-quantum-leansig-with-rust-code-examples/)
- [DECIDED in ethlambda, Sep 2026] For the binary-field build, "a key's bottom subtree covers 512 slots for a key range of about 2^18". Key warming was moved off the actor thread (#622). — [ethlambda PR #606](https://github.com/lambdaclass/ethlambda/pull/606)
- [PROPOSED, EIP-8310 Draft, created 2026-06-19] "Post-Quantum Keystore for Stateful Keys". Authors are Anshal Shukla, Benedikt Wagner, Gajinder Singh, Guillaume Ballet, Justin Drake, Kolby Moroz Liebl, Parthasarathy Ramanujam, Shariq Naiyer, Thomas Coratger and Unnawut Leepaisalsuwanna. It requires EIP-2335 and EIP-3076 and defines keystore version 5, whose encrypted secret is an XMSS seed (`leanxmss`). Its rules:
  - Commit-before-sign: "Releasing a signature before the state advance is durable is a violation."
  - `state.authoritative` MUST be false.
  - Leaf ranges must be disjoint, and one range is never given to two signers.
  - Importing a consumable key with fresh or empty state MUST be a hard error.
  - A high-water regression MUST be detected, and signing refused until it is reconciled.
  - In synchronized mode, the EIP-3076 slashing-protection DB is the authoritative state. Key and history move together atomically.
  - On exhaustion the signer MUST refuse: no wrap-around and no borrowing from another range. An exhausted key can no longer sign, and capacity monitoring is an operational requirement.
  - Synchronized mode uses `leaf = f(epoch)` or `f(slot)`. Counter mode uses a separate durable store.
  — [EIP-8310](https://eips.ethereum.org/EIPS/eip-8310)
- [RESEARCH] NIST SP 800-208 calls XMSS/LMS "not suitable for general use" because of state failure, for example a backup restore that rewinds the index. The survey flags state reuse as an operational risk for validators. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [RESEARCH, Oct 2026 README] leanVM now also benchmarks **leanSPHINCS** aggregation (a stateless hash-based family): 850 sigs, proof 325.8 KiB, 1.0 s proving, 850 sigs/s, verify 5.113 ms. — [leanVM README](https://github.com/leanEthereum/leanMultisig)
- [RESEARCH] A September 2026 arXiv paper argues any XMSS-style hierarchy must specify how rollback, duplicate leaf use and crash-recovery divergence are avoided, and how epoch rotation retires leaves (snippet). — [arXiv 2609.24689](https://arxiv.org/pdf/2609.24689)

### Inferences
- With 4-s slots, 8 years ≈ 63.1M slots ≈ 2^26 active epochs, a sub-range of the 2^32 tree (2^32 × 4 s ≈ 544 years). The "8-year" figure is therefore most likely the activation window used for keygen benchmarks, not the tree capacity. This is arithmetic, not confirmed.
- Because the leaf index is the slot, missing slots wastes leaves harmlessly, and exhaustion is calendar-driven. Rotation would be a re-registration of a new public key (a new XMSS root) before the activation range ends. No spec text for an in-protocol rotation was found.
- leanSPHINCS benchmarking signals that a stateless fallback is being kept open (RESEARCH). Nothing indicates it has been chosen.

### Gaps
- No leanSpec text was found on in-protocol key rotation or re-registration at exhaustion, or on how activation-range extension is signalled on-chain.
- No source confirmed the exact activation window used in devnets.

## Q3. leanMultisig / leanVM aggregation: design, proof size, prover and verifier time, complexity, recursion

### Takeaway
Aggregation uses **leanVM** (formerly leanMultisig), a minimal hash-based zkVM. It proves verification of many XMSS signatures inside a STARK-style proof (WHIR PCS) and supports **recursive 2→1 merging**. Current numbers (Oct 2026 README, M4 Max) are 1,290 sigs proven in **0.947 s (1,362 sigs/s)**, proof **318.4 KiB**, verify **5.387 ms**, and recursion 2→1 in **0.291 s** giving **201.5 KiB**. The stack moved from KoalaBear/Poseidon to a 192-bit binary field with BLAKE2s in Sep 2026. Devnet-5 (interop 2026-05-29) carries **one ~200 KB block-level proof** instead of per-attestation proofs that made blocks several MB. Bandwidth drops from O(n·σ) (34–70 MB per slot unaggregated) to a near-constant ~0.2–0.4 MB proof plus an O(n) participation bitfield.

### Cited Findings
**Architecture**
- [DECIDED for devnets ≤5, KoalaBear era] Field: KoalaBear p = 2^31 − 2^24 + 1 with a degree-5 extension (q ≈ 2^155). PCS: WHIR, plus SuperSpartan-style sumcheck and Logup/GKR. ISA: 4 base instructions (ADD/MUL/DEREF/JUMP) plus 2 precompiles. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [DECIDED on leanVM main, Oct 2026] Self-described as a "minimal hash-based zkVM, for post-quantum Ethereum". Field: **192-bit binary field** (degree-3 tower over a 64-bit field). KoalaBear lives on in a separate branch. Hash: **BLAKE2s (placeholder)**. Proof system: WHIR (Ligerito), Flock hash proving, Binius64 ring switching and M3 arithmetisation. Security: "128-bit ROM (64-bit QROM) soundness", "no proximity gap conjecture", "a traditional hash function". The project is not production-ready, has no zero-knowledge yet, and formal verification is in progress. The ISA is leanISA, with a planned migration to RISC-V rv64im. — [leanVM README](https://github.com/leanEthereum/leanMultisig)

**Benchmarks**
- [RESEARCH benchmark, Oct 2026 README; M4 Max, 12P+4E cores, 48 GB, no GPU] — [leanVM README](https://github.com/leanEthereum/leanMultisig)

| Workload | Proof | Prove | Throughput | Verify | Peak RAM |
|---|---|---|---|---|---|
| leanXMSS aggregation, 1,290 sigs (2^20.35 cycles) | 318.4 KiB | 0.947 s | 1,362 sigs/s | 5.387 ms | 15.46 GiB |
| leanSPHINCS aggregation, 850 sigs | 325.8 KiB | 1.0 s | 850 sigs/s | 5.113 ms | 16.1 GiB |
| Recursion 2→1 (leaves of 900 leanXMSS) | 201.5 KiB | 0.291 s | n/a | 6.271 ms | 14.75 GiB |
| 16 DA blobs | 324.0 KiB | 0.986 s | 2.029 MiB/s | 6.573 ms | 12.5 GiB |
| 262,144 BLAKE2s compressions | n/a | 539.7 ms | 485,765 compressions/s | 2.0 ms | n/a |

- [RESEARCH benchmark, KoalaBear era, survey 2026-08]
  - Leaf proof: **377 KiB** at rate 1/2.
  - Three-level tree for **5,519 signers**: **128 KiB** at rate 1/16, root proof **0.635 s**, recursive merge about **0.3 s per child proof**.
  - Main branch: 327 KiB and **1,426 sigs/s**. devnet-5 leaf: **1,193 sigs/s**.
  - 10,000 signatures: 7.0 s (main) and 8.4 s (devnet-5), against a target of 10,000 in 1 s.
  - Aggregate verifier time not confirmed.
  — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [RESEARCH, earlier 2026] Earlier throughput figures were much lower:
  - Ream Labs devnet-2 notes: **45 XMSS/s** on the devnet2 branch vs about **500 XMSS/s** on main (snippet). — [Ream devnet-2 metrics](https://hackmd.io/@reamlabs/BJvG5qmP-g)
  - LambdaClass: about **400 aggregations/s** (snippet). — [LambdaClass ethlambda tooling blog](https://blog.lambdaclass.com/ethlambda-building-a-post-quantum-ethereum-client-with-the-help-of-shared-tooling/)
  - LambdaClass diagram: aggregate "~100 KB (constant size!)". — [LambdaClass blog, 2026-02-09](https://blog.lambdaclass.com/ethereum-signature-schemes-explained-ecdsa-bls-xmss-and-post-quantum-leansig-with-rust-code-examples/)
- [RESEARCH, page state ~May 2026] XMSS aggregated per second reaches **97% of target** (M4 Max, `main`), 82% (`lean-vm-simple`) and 38% (i9-12900H). Aggregate size is at **313–391% of target** (efficient branch). Absolute values do not render. — [leanroadmap.org](https://leanroadmap.org/)
- [PROPOSED, EF 2026 plan] Targets: **200 ms** for 2-to-1 recursion, **1,000 sigs/s**, and **256 KB** proofs propagating in under 1 s. — [Survey citing "Lean Consensus: 2026 Plan"](https://hackmd.io/@goatresearch/H1G2tOCwGx); [Lean Consensus 2026 Plan (HackMD)](https://hackmd.io/@tcoratger/ryS1ElrWbx)

**Client-measured timings (DECIDED in ethlambda, Sep 2026, binary-field leanVM)**
- Devnet aggregation (n = 126): p50 **201 ms**, p95 350 ms, max 493 ms.
- Type-2 block-proof verify: p50 **135 ms**, max 270 ms. Other commits report about 50 ms, and p50 **43.7 ms** through the "pubkey-less path".
- Aggregating 3 sigs went from 27–83 s (queued) to 201–232 ms p50.
— [ethlambda PR #606](https://github.com/lambdaclass/ethlambda/pull/606); [ethlambda PR #539](https://github.com/lambdaclass/ethlambda/pull/539)

**Recursion and devnet evolution (DECIDED)**
- Recursion runs along the signer dimension. One program verifies some signatures directly and checks sub-proofs in-circuit. The bytecode is self-referential, compiled to a fixed point. Signers may overlap across sources, which enables sharded p2p aggregation. Duplicates are capped at MAX_XMSS_DUPLICATES = 2^15. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- Devnet evolution, 2026-06-16:
  - Devnet-2 added real signature aggregation.
  - Devnet-3 introduced an aggregator role with one level of aggregation.
  - Devnet-4 added a second, recursive level in both aggregators and proposers. Blocks carried one proof per attestation (≤16 attestations × ~200 KB), so "a block could weigh several MBs".
  - **Devnet-5** gossips a **single block proof**: a multi-message aggregate "no larger than one of the old per-attestation proofs", cutting maximal blocks "from several MBs to around 200KB".
  — [LambdaClass, "ethlambda: devnet 5 and beyond"](https://blog.lambdaclass.com/ethlambda-devnet-5-and-beyond/)

**Bandwidth arithmetic**
- [RESEARCH, survey 2026-08] About 903,000 active validators (Aug 2026) cast ~28k votes per slot. Unaggregated, hash-based sigs at 1.2–2.5 KB give **34–70 MB per slot**. Proof-based aggregates are 128–377 KiB, about an order of magnitude more than today's ~10 KiB of BLS signature data per block. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)

### Inferences
- **Complexity.** Unaggregated cost is O(n·σ) with σ ≈ 1.2–2.5 KB. SNARK aggregation makes the proof O(polylog n) in theory (WHIR/FRI-style). In practice it is roughly constant at 0.2–0.4 MB across 900–5,519 signers, and recursion keeps it bounded (201.5 KiB after 2→1). Verification is O(polylog n) hashing (~5–6 ms in-bench, 43–135 ms p50 in a client). A participation bitfield of O(n) bits, about n/8 bytes, is still needed to bind the signer set. **Prover cost stays linear**: about 0.7 ms of M4-Max time per signature at 1,362 sigs/s, so ~28k votes per slot is about 21 M4-Max-seconds of proving per slot, spread across subnet aggregators and recursion.
- **Recursion.** Aggregating aggregates is DECIDED (devnet-4 onward, block-level in devnet-5). Per-merge cost (~0.29–0.3 s) is the binding latency term within a 4-s slot.
- The ethlambda "pubkey-less path" (p50 43.7 ms) suggests the verifier need not ingest all n public keys, likely by committing to the validator registry. This is unconfirmed.

### Gaps
- No independent, binary-field devnet-wide measurement of aggregate size and verify time at full validator scale (only ≤1,024 validators in devnet-5, 126 in an ethlambda test).
- The exact public-input format (bitfield vs. registry commitment) for the devnet-5 block proof was not retrieved from leanSpec.

## Q4. Comparison with today's BLS aggregation, and what is lost

### Takeaway
BLS gives a ~96-B aggregate (plus a ~1 bit per validator bitfield) with trivially cheap, non-interactive, linear merging. The PQ route replaces this with ~200–330 KiB STARK-style proofs that cost ~0.3 s per recursive merge, plus 1.2–2.5 KB individual signatures and stateful keys. Rogue-key concerns largely vanish because each signature is verified individually inside the proof (inference).

### Cited Findings
- [Current mainnet] BLS signature **96 B**, public key **48 B**. A beacon block carries 64 aggregates × 96 B ≈ 6 KiB plus ~440-bit bitfields per committee (~3.5 KiB), roughly **10 KiB** per block. The sync committee is a 512-validator BLS aggregate. There were ~903,000 validators staking ~42.4M ETH in Aug 2026. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [Conflict] LambdaClass gives the BLS signature as "48 bytes, in compressed form", which is the min-signature convention. Ethereum uses min-pk (48-B public key, 96-B signature), so the survey's figures are the Ethereum ones. — [LambdaClass blog](https://blog.lambdaclass.com/ethereum-signature-schemes-explained-ecdsa-bls-xmss-and-post-quantum-leansig-with-rust-code-examples/) vs [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [RESEARCH] The proof-based route is 128–377 KiB per aggregate, about **10×+** today's ~10 KiB of signature data. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)
- [DECIDED, devnet-5] A block-level proof of ~200 KB replaces per-attestation proofs. — [LambdaClass devnet-5 post](https://blog.lambdaclass.com/ethlambda-devnet-5-and-beyond/)

### Inferences
- **What is lost**:
  - Algebraic, homomorphic aggregation. BLS merges are point additions (µs, any order, by anyone). PQ merges need a recursive proof (~0.29 s, GB-scale RAM), so aggregator hardware and latency become consensus-critical.
  - About 2,000–3,500× larger aggregate objects (≈200–330 KiB vs 96 B).
  - 12–26× larger individual signatures on gossip (1,208–2,536 B vs 96 B).
  - Statelessness of signing keys (XMSS state must be guarded, per EIP-8310).
  - Simple aggregate verification (a pairing check vs a STARK verifier).
- **What is gained or neutralised**: BLS same-message aggregation needs proof-of-possession against rogue-key attacks. In SNARK aggregation every signature is checked individually against its registered key inside the circuit, so rogue-key attacks do not apply in the BLS sense. No source states this explicitly. Overlapping signer sets are explicitly supported (MAX_XMSS_DUPLICATES = 2^15), whereas BLS overlaps need multiplicity tracking.
- Asymptotically both keep an O(n)-bit participation bitfield. BLS keeps an O(1) 96-B signature, while the PQ proof is O(polylog n) and practically constant at ~0.2–0.3 MB.

### Gaps
- No primary EF or leanSpec document was found that explicitly enumerates the "lost properties" or discusses rogue-key handling for leanSig aggregation.

## Q5. Benchmarks, devnets and client implementations: status

### Takeaway
Eight-plus clients (Ream, Zeam, Qlean-mini, Lantern, Lighthouse fork, ethlambda, gean, Peam, plus Grandine in devnets) have progressed through pq-devnet-0 to pq-devnet-5. Devnet-5 interop was reached on 2026-05-29 with one block-level recursive proof. Devnet-6 is PROPOSED to change consensus (Goldfish/RLMD-GHOST plus a finality gadget). In Sep 2026 clients began migrating to the binary-field/BLAKE2s leanVM. The EF strawmap puts production leanVM in 2027, CL deployment in 2028 and the L1 upgrade around 2029, all PROPOSED and non-binding.

### Cited Findings
**Devnets**
- [DECIDED/completed]

| Devnet | Status | Content |
|---|---|---|
| pq-devnet-0 | completed Oct 2025 | Ream/Zeam/Qlean interop, no PQ sigs, 4-s slots, QUIC, Gossipsub v1.0, modified 3SF-mini |
| pq-devnet-1 | completed Dec 2025 | leanSig sign/verify integrated, aggregation by concatenation |
| pq-devnet-2 | completed Jan 2026 | leanMultisig aggregation |
| pq-devnet-3 | completed Feb 2026 | separate aggregator role |
| pq-devnet-4 | target Mar 2026 | recursive aggregation via leanVM, one aggregate per message |
| pq-devnet-5 | planned on the roadmap page | one block-level proof over multiple attestations, "PQ heartbeat" under Goldfish |

— [leanroadmap.org](https://leanroadmap.org/)
- [DECIDED, 2026-04-29, snippet] At PQ Interop call #37, devnet-4 had been relaunched with two subnets. Finality was lost after slot 110, both Ream instances crashed, and qlean/lantern timed out on requests from zeam. — [ethereum/pm #2035](https://github.com/ethereum/pm/issues/2035)
- [DECIDED, devnet-3] A Ream metrics run used 13 validators across ethlambda, Qlean, Ream, Zeam and others (snippet). — [Ream devnet-3 metrics](https://hackmd.io/@reamlabs/devnet-3-metrics)
- [DECIDED, 2026-05-29] devnet-5 interop was reached with 8 clients and 1,024 validators per devnet. — [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx); [EthDaily (snippet)](https://ethdaily.io/lean-ethereum-reaches-devnet5-interop)
- [PROPOSED, 2026-06-16] Devnet-6 would replace LMD-GHOST and 3SF-mini with Goldfish, RLMD-GHOST and a trailing finality gadget. Devnet-7 would connect CL to EL, add balances, deposits and dynamic validator sets. Later: FOCIL, ethp2p, ePBS and APS. — [LambdaClass devnet-5 post](https://blog.lambdaclass.com/ethlambda-devnet-5-and-beyond/)
- [Status, Sep 2026, snippet] The ethean client lists pq-devnet-4 as its default and pq-devnet-5 as a prepared path. — [ethean-labs/ethean](https://github.com/ethean-labs/ethean)

**Clients**
- Ream (Rust, ReamLabs), Zeam (Zig), Qlean-mini (C++, qdrvm), Lantern (C, Pier Two), a Lighthouse fork (Rust), ethlambda (Rust, LambdaClass), gean (Go, geanlabs) and Peam (Rust). Grandine appears in devnet-1. — [leanroadmap.org](https://leanroadmap.org/)
- [DECIDED in ethlambda, Sep 2026] Migration to binary-field leanVM (BLAKE2s) via PR #606, with a 3-node local devnet test (22 blocks, finalized slot 28). — [ethlambda PR #606](https://github.com/lambdaclass/ethlambda/pull/606)

**Repositories**
- leanSig: Apache-2.0, unaudited, Rust ≥1.87, ~211 commits. XMSS is now internalized into leanVM's `xmss` crate for client use. — [leanSig README](https://github.com/leanEthereum/leanSig); [ethlambda PR #539](https://github.com/lambdaclass/ethlambda/pull/539)
- leanMultisig is now titled leanVM. — [leanVM repo](https://github.com/leanEthereum/leanMultisig)

**Timeline**
- [PROPOSED, 2026-08-13] Production-grade leanVM around 2027, then CL/DA/EL deployments in 2028, per Drake's preliminary strawmap. The official PQ roadmap's non-binding target is about 2029 for core infrastructure. Nothing is deployed to mainnet. — [CryptoSlate](https://cryptoslate.com/ethereum-abandons-its-8-year-cryptography-bet-after-proof-systems-flip-the-tradeoff/); [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)

### Inferences
- As of October 2026 the live interop target is devnet-5 (KoalaBear/Poseidon-era formats, 2,536-B signatures). Clients are mid-migration to the binary-field/BLAKE2s leanVM (1,208-B signatures, 32-B keys), which PR #539 notes "needs ecosystem coordination" and is not drop-in. A new devnet (likely devnet-6 or a re-genesis) would be needed to adopt it network-wide.

### Gaps
- No primary source confirms devnet-6 launch, or any devnet running binary-field leanVM across multiple clients, as of 2026-10-21.
- leanroadmap.org benchmark absolute values and its post-May-2026 state were not rendered.
- The Zeam, Qlean and Ream repositories were not individually checked for Sep–Oct 2026 status.
