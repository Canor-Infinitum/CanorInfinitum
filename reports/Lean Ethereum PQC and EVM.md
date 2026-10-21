# Ethereum Stakes Its Quantum Future on Hashes

As of 21 October 2026, Ethereum mainnet runs no post-quantum (PQ) cryptography, and no named fork schedules any. Validators still sign with BLS and accounts with ECDSA. The plan of record is a self-imposed Ethereum Foundation (EF) deadline: a quantum-resistant L1 by **December 2029**, with outside experts reassessing quantum progress in January 2027. On the consensus layer, devnets have settled on **hash-based generalized XMSS (leanSig)**. Its signatures are aggregated by recursive, hash-based STARK proofs in **leanVM**, which compress n signatures of 1.2–2.5 KB each into one proof of about 200–320 KiB. The cost is linear proving work and stateful keys. The hash underneath is still unsettled: the EF dropped Poseidon on 13 August 2026 in favour of SHA or BLAKE, and BLAKE2s is an explicit placeholder. The execution layer gets signature agility through account abstraction rather than a forced cutover:

- **EIP-8141 Frame Transactions** is scheduled for Hegotá (2027).
- Two key-revocation EIPs are under consideration for Hegotá.
- An ML-DSA precompile was declined for Hegotá.
- The EF's proposed long-term path is hash-based leanSPHINCS signatures aggregated in the mempool (EIP-8288).

Across all of these designs, Ethereum follows one rule for key binding: a verifier never trusts a key that travels with a message. It looks the key up in a registry entry that was bound once, with a proof of possession. The PQ registry proposal adds versioned rebinding, authorized by the validator's withdrawal authority.

For the canor_elite accord this gives three lessons:

1. **Key-swap gap.** Bind a key commitment wherever attribution is decided. Profile-level binding works, but evaluators running different profiles can reach different verdicts.
2. **Signature storage.** Profile-level Merkle batch signing cuts the 13.8 MB worst case about 15× without touching the core.
3. **General Public projects.** Open participation needs Ethereum's pattern: an open registry feeding small quorums with a fixed member list per decision and a cost to join. All-pairs certification does not scale.

*Reading conventions.* Status tags:

| Tag | Meaning |
|---|---|
| **[Shipped]** | Live on Ethereum mainnet. |
| **[Decided]** | Scheduled for Inclusion (SFI) in a named fork's meta-EIP, or a published EF self-commitment. Not an All Core Devs fork decision. |
| **[Devnet]** | Implemented in leanSpec, the pq-devnets or client code. Decided for the lean R&D track only, not for mainnet. |
| **[Proposed]** | Draft EIP, ethresear.ch design, Considered-for-Inclusion (CFI) item, or roadmap placeholder. |
| **[Declined]** | Declined for Inclusion (DFI) in a named fork. |
| **[Research]** | Papers, benchmarks, forum replies, individual statements. |

Paragraphs that open with **Inference.** are my analysis. Everything else is sourced. Facts about the accord come from the repository's own files.

## Nothing post-quantum ships on mainnet before 2027

The mainnet baseline is unchanged classical cryptography. Pectra (7 May 2025) and Fusaka (3 December 2025) shipped with no post-quantum items. Glamsterdam's headline features are ePBS and block-level access lists, and it carries no PQ items either. It forked Sepolia on 6 October 2026. Its mainnet date is not set, with early December 2026 floated ([EF Blog](https://blog.ethereum.org/2026/09/17/glamsterdam-testnet-announcement)). Hegotá's scheduled scope is FOCIL plus **EIP-8141 Frame Transactions**. Its two PQ-relevant candidates were both **declined** ([EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)):

- EIP-8355 (ML-DSA precompiles);
- EIP-8025 (optional execution proofs).

On 7 September 2026 the EF Protocol cluster published its priorities. It commits to an L1 that is quantum-resistant across execution, consensus and data by **December 2029**. It plans for Q-day "as early as 2030", and treats the deadline as non-negotiable at least until a **January 2027** review ([EF Blog](https://blog.ethereum.org/2026/09/07/protocol-priorities)). The work is mapped onto placeholder forks:

| Fork | Contents |
|---|---|
| I\* | PQ public-key registry |
| J\* | "MV-PQ" safeguard: PQ heartbeat, leanDA sampling, leanSPHINCS transactions |
| K\* | Mandatory execution proofs |
| L\* | Full PQ readiness, including PQ attestations |

Reaching L\* by December 2029 needs one fork roughly every 7.2 months ([The Block](https://www.theblock.co/news/ecosystems/2026-09-08-ethereum-foundation-quantum-resistance-2029-413716)).

| Item | Status | Date | Source |
|---|---|---|---|
| EIP-7702 EOA code delegation (ECDSA key stays in control) | Shipped | Pectra, 2025-05-07 | [EIP-7600](https://eips.ethereum.org/EIPS/eip-7600) |
| leanSig generalized XMSS signatures | Devnet | pq-devnet-1, Dec 2025 onward | [leanroadmap.org](https://leanroadmap.org/) |
| leanVM recursive aggregation, single block-level proof | Devnet | devnet-4 Mar 2026; devnet-5 interop 2026-05-29 | [LambdaClass](https://blog.lambdaclass.com/ethlambda-devnet-5-and-beyond/) |
| EF PQ Security team (lead Thomas Coratger) | Decided (organisational) | Jan 2026 | [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/) |
| Poseidon abandoned for L1 in favour of SHA/BLAKE | Decided (EF direction) | 2026-08-13 | [CryptoSlate](https://cryptoslate.com/ethereum-abandons-its-8-year-cryptography-bet-after-proof-systems-flip-the-tradeoff/) |
| Quantum-resistant L1 by Dec 2029 | Decided (EF self-commitment) | 2026-09-07 | [EF Blog](https://blog.ethereum.org/2026/09/07/protocol-priorities) |
| EIP-8141 Frame Transactions (native AA, scheme byte) | Decided (SFI Hegotá; spec Draft) | Created 2026-01-29; SFI dated 27 Aug to 10 Sep 2026 depending on source; Hegotá projected mid/late 2027 | [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081); [AppliedPQC](https://github.com/AppliedPQC/pqc-research/issues/35) |
| EIP-8298 SETCODEFROM + EIP-8151 restricted ecrecover | Proposed (CFI Hegotá) | 2026-06-11; 2026-02-09 | [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081) |
| EIP-8355 ML-DSA precompiles; EIP-8025 optional proofs | Declined (Hegotá) | 2026 | [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081) |
| PQ validator key registry design | Proposed (fork I\*) | 2026-06-01 | [ethresear.ch](https://ethresear.ch/t/exploring-the-design-space-for-a-post-quantum-public-key-registry-for-ethereum-validators/25040) |
| EIP-8310 keystore for stateful keys | Proposed | 2026-06-19 | [EIP-8310](https://eips.ethereum.org/EIPS/eip-8310) |
| EIP-8288 in-mempool leanSPHINCS aggregation | Proposed | 2026-06-03 | [EIP-8288](https://eips.ethereum.org/EIPS/eip-8288) |
| PQ-ready deposit contract (EIPs PR #12235) | Proposed | 2026-08-24 | [crypto.news](https://crypto.news/ethereum-proposes-post-quantum-deposit-contract/) |
| EIP-8365 / EIP-8433 retire 0x00 BLS-credential validators | Proposed | 2026-07-18; 2026-09-28 | [EIP-8365](https://eips.ethereum.org/EIPS/eip-8365); [EIP-8433](https://eips.ethereum.org/EIPS/eip-8433) |
| MV-PQ safeguard at J\* | Proposed (roadmap) | Strawmap update 2026-08-19 | [CryptoSlate](https://cryptoslate.com/ethereums-2029-quantum-deadline-is-starting-to-reshape-what-gets-built-next/) |
| Quantum-emergency hard fork | Research | 2024-03-09 | [ethresear.ch](https://ethresear.ch/t/how-to-hard-fork-to-save-most-users-funds-in-a-quantum-emergency/18901) |

Three sources disagree on the milestone map:

- **ethereum.org** (updated 8 September 2026) puts PQ signature-verification precompiles at J\*. It adds an M\* fork for full PQ aggregation and PQ blobs ([ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)).
- **pq.ethereum.org** says J\* verification uses a "vector math precompile" rather than one precompile per scheme ([pq.ethereum.org](https://pq.ethereum.org/)).
- **An earlier Drake strawmap** put production leanVM in 2027 and the consensus, data and execution deployments in 2028 ([CryptoSlate](https://cryptoslate.com/ethereum-abandons-its-8-year-cryptography-bet-after-proof-systems-flip-the-tradeoff/)).

The EF's 7 September post is the newest primary statement. It says the fork order after J\* will be "finalized once research matures".

## Hash-based XMSS beat lattices on conservatism and aggregation cost

leanSig is a **generalized XMSS** by Drake, Khovratovich, Kudinov and Wagner: a Merkle tree of Winternitz-style one-time signatures. Its security proof avoids random oracles and states explicit standard-model requirements on the hash. That matters because the same hash must then be proven inside an aggregation circuit ([ePrint 2025/055](https://eprint.iacr.org/2025/055)). Its encoding is **Target-Sum Winternitz**, which fixes the sum of chain positions so that the verifier's worst-case hash count equals its average ([Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)).

The draft production parameter set is LOG_LIFETIME = 32 (2^32 leaves), 46 chains, base 8 and target sum 200 ([EIP-8310](https://eips.ethereum.org/EIPS/eip-8310)) **[Proposed]**. Signatures have shrunk with each devnet **[Devnet]**:

| When | Signature | Public key | Source |
|---|---|---|---|
| devnet-2, Jan 2026 | **3,112 B** | — | [LambdaClass](https://blog.lambdaclass.com/ethlambda-building-a-post-quantum-ethereum-client-with-the-help-of-shared-tooling/) |
| devnet-4/5 | **2,536 B** | 52 B | [ethlambda PR #539](https://github.com/lambdaclass/ethlambda/pull/539) |
| leanVM `main`, Jul–Sep 2026 | **1,208 B** | **32 B** | [ethlambda PR #539](https://github.com/lambdaclass/ethlambda/pull/539) |

The last row uses random-oracle-model parameters with 42 chains. Standard-model parameters cost about 16% of aggregation throughput and 15% of proof size ([Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)). The security target is NIST Level 1: 128-bit classical and 64-bit quantum. On an M4 Max, signing takes about 0.53 ms and verification about 0.19 ms ([LambdaClass](https://blog.lambdaclass.com/ethereum-signature-schemes-explained-ecdsa-bls-xmss-and-post-quantum-leansig-with-rust-code-examples/)).

The case against the alternatives is quantitative ([Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)):

- **ML-DSA-65** carries a 3,309-B signature and a 1,952-B key, about 90 MB per slot unaggregated.
- **SLH-DSA** signatures run 7.9–49.9 KB. That is above the roughly 4 KiB bound implied by the paper's bandwidth model: a 32 KiB signature with a 2^12 committee would force an aggregator to receive 2^30 bits in one second.
- **Falcon with LaBRADOR** compresses 10,000 signatures to 74 KB, but verifying takes 2.65 s, against about 106 ms for the hash route.

The hash has moved twice. leanSig used Poseidon1 over KoalaBear ([leanSig](https://github.com/leanEthereum/leanSig)). Then, on **13 August 2026**, Justin Drake announced "The Ethereum Foundation is abandoning Poseidon for L1, pivoting to SHA or BLAKE" ([X](https://x.com/drakefjustin/status/2087905684180418733)). No practical break forced the change. The reason was that binary-field SNARKs (Binius, Flock) now prove standard hashes cheaply. On one M4 Max core, Flock reaches about 82,000 BLAKE3 compressions, 42,000 SHA-256 compressions or 30,000 Keccak permutations per second ([CryptoSlate](https://cryptoslate.com/ethereum-abandons-its-8-year-cryptography-bet-after-proof-systems-flip-the-tradeoff/)). Clients followed in September: ethlambda PR #606 swapped Poseidon over KoalaBear for **BLAKE2s over binary fields** and kept 1,208-B signatures ([ethlambda PR #606](https://github.com/lambdaclass/ethlambda/pull/606)). The leanVM README still calls BLAKE2s a **placeholder**, with SHA2, SHA3 and BLAKE3 under consideration ([leanVM](https://github.com/leanEthereum/leanMultisig)).

XMSS is stateful, and Ethereum handles that by tying the one-time leaf to the slot. "Never reuse a leaf" then becomes "never sign twice in a slot", which slashing already punishes ([Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)). EIP-8310 adds operational rules ([EIP-8310](https://eips.ethereum.org/EIPS/eip-8310)) **[Proposed]**:

- commit the state advance before releasing a signature;
- give each signer a disjoint leaf range;
- treat an import with empty state as a hard error;
- detect a stale restore;
- refuse to sign when the key is exhausted.

NIST SP 800-208 still calls stateful hash signatures "not suitable for general use" ([Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)), and leanVM also benchmarks stateless **leanSPHINCS** as a fallback ([leanVM](https://github.com/leanEthereum/leanMultisig)) **[Research]**.

The execution layer has not committed to a scheme. pq.ethereum.org lists Falcon, Dilithium and SPHINCS+ under evaluation and favours agility ([pq.ethereum.org](https://pq.ethereum.org/)). EIP-8288 nonetheless accepts only hash-based leanSPHINCS as a signature dependency ([EIP-8288](https://eips.ethereum.org/EIPS/eip-8288)). Lattice precompile drafts remain unscheduled ([EIP-8051](https://eips.ethereum.org/EIPS/eip-8051); [EIP-8052](https://eips.ethereum.org/EIPS/eip-8052)):

- EIP-8051: ML-DSA-44 at 4,500 gas;
- EIP-8052: Falcon-512 at 3,000 gas.

Without a precompile, a tuned SLH-DSA-Keccak-128-24 contract verifies in about 94,000 gas. That just fits EIP-8141's 100,000-gas mempool cap, while pure-Solidity Falcon costs 650,000 to 1.9M gas ([AppliedPQC](https://github.com/AppliedPQC/pqc-research/issues/35)).

**Inference.** Ethereum splits its scheme choice on whether a global clock exists:

- **Validators** sign once per slot, so a slot-indexed stateful XMSS is safe and small.
- **Accounts and transactions** have no such clock, so the EL leans to stateless SPHINCS-family signatures.

The hash choice is now driven by conservatism plus provability, not by circuit-friendliness. That leaves SHA3 a live candidate even though BLAKE currently leads.

## Aggregation trades a 96-byte signature for a 200-KiB proof

BLS made aggregation nearly free: one 96-byte aggregate plus a participation bitfield per committee, about 10 KiB of signature data per block ([Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx)). The PQ path rebuilds aggregation as a proof system. The devnets show it step by step ([leanroadmap.org](https://leanroadmap.org/); [LambdaClass](https://blog.lambdaclass.com/ethlambda-devnet-5-and-beyond/)):

| Devnet | Aggregation |
|---|---|
| devnet-1 (Dec 2025) | Signatures simply concatenated |
| devnet-2 (Jan 2026) | leanMultisig proofs |
| devnet-3 (Feb 2026) | A separate aggregator role |
| devnet-4 | Recursive aggregation, but one proof per attestation, so blocks "could weigh several MBs" |
| **devnet-5** (interop 29 May 2026) | One **multi-message block proof** "no larger than one of the old per-attestation proofs", cutting maximal blocks to about 200 KB |

The October 2026 leanVM benchmarks on an M4 Max ([leanVM](https://github.com/leanEthereum/leanMultisig)) **[Research]**:

| Workload | Proof | Proving time | Throughput | Verify | Peak RAM |
|---|---|---|---|---|---|
| 1,290 XMSS signatures | **318.4 KiB** | 0.947 s | 1,362 signatures/s | **5.4 ms** | 15.5 GiB |
| Recursive 2→1 merge | 201.5 KiB | 0.291 s | — | — | — |

A client measured p50 aggregation of 201 ms for 126 signers, and p50 block-proof verification of 43.7–135 ms depending on the path ([ethlambda PR #606](https://github.com/lambdaclass/ethlambda/pull/606)). The EF's targets are 1,000 signatures/s, 200 ms recursion and 256 KB proofs that propagate in under a second ([HackMD](https://hackmd.io/@tcoratger/ryS1ElrWbx)).

| Regime | Status | Aggregate stored / sent | Verifier work | Producer work |
|---|---|---|---|---|
| BLS12-381 today | Shipped | O(1): 96 B + O(n)-bit bitfield | O(n) public-key additions + one pairing check | O(1) point addition, by anyone, in any order |
| PQ, unaggregated | Devnet-1 | **O(n·σ)**: 34–70 MB per slot at ~28k votes | O(n) verifications at ~0.19 ms each | none |
| PQ, leanVM recursive STARK | Devnet-4/5 | **O(polylog n)**, measured flat at 0.2–0.4 MB, + O(n)-bit bitfield | **O(polylog n)**: 5.4 ms in benchmark, 43.7–135 ms p50 in a client | **O(n)** proving, ~0.7 ms per signature; ~0.29 s per 2→1 merge |
| EL in-mempool aggregation (EIP-8288) | Proposed | One recursive STARK per block, 128–512 kB | One proof check against a fixed `AGGREGATED_VK` | Per-interval recursion; ~256 kB fixed overhead per interval |

The table's asymptotic classes are my inference from the cited measurements. The numbers come from the [Survey](https://hackmd.io/@goatresearch/H1G2tOCwGx), [leanVM](https://github.com/leanEthereum/leanMultisig) and [EIP-8288](https://eips.ethereum.org/EIPS/eip-8288). The bitfield is a hard requirement, not a detail: the proof must cover "exactly that subset" of signers because the consensus layer "must know exactly who participated" ([ethresear.ch](https://ethresear.ch/t/exploring-the-design-space-for-a-post-quantum-public-key-registry-for-ethereum-validators/25040)).

The execution-layer version is EIP-8288 (Buterin and Coratger, Draft, 3 June 2026) **[Proposed]**. It strips individual leanSPHINCS signatures in the mempool and replaces them with one block-level recursive STARK. A transaction carries only 96-byte `(scheme, data_hash, vk_hash)` triples. Each costs 3,000 gas plus 30,000 `recursive_stark_gas`, against 150,000–200,000 gas to verify on-chain ([EIP-8288](https://eips.ethereum.org/EIPS/eip-8288)).

### Buterin argues for "EIP-8288 maximalism" (19 October 2026)

On 19 October 2026 Vitalik Buterin posted twice on X, recasting EIP-8288 from an aggregation mechanism into a general rule for cryptography on Ethereum **[Research]**. Both are individual statements, read directly on x.com on 2026-10-21; the times below are as displayed there.

- **Post 1 (4:56 AM).** He was replying to @lex_node, who had argued that lattice signatures could still arrive through a precompile once they are judged secure against AI-assisted analysis. His answer: the ideal route for everyone's cryptography is EIP-8288. Any signature is made locally, wrapped in a STARK and included through the STARK aggregator. Nothing goes on-chain as calldata, so Ethereum never has to add another precompile ([X](https://x.com/VitalikButerin/status/2112015118418473358)).
- **Post 2 (6:04 AM), quoting post 1.** Precompiles and the EVM are the wrong home for most cryptography, because their inputs must be posted on-chain as calldata. Block space tops out in practice at about 8 MB/s, even with PeerDAS and blocks-in-blobs. Self-contained heavy computation should instead be proven client-side with STARKs, which he says are getting fast even on phones, and pushed through EIP-8288's recursive aggregation. The chain then records only that the computation happened and its result. He calls this stance "EIP-8288 maximalism": anyone can use any cryptography, and no new precompiles are needed ([X](https://x.com/VitalikButerin/status/2112032009413439704)).

Secondary coverage ties the exchange to two other events. One is a16z crypto's 16 October announcement of Aerie, a lattice-based scheme for aggregating Falcon signatures. The other is Eli Ben-Sasson's advice to allow three to five years of AI-assisted scrutiny before trusting lattice schemes with large sums ([Cryptotimes](https://www.cryptotimes.io/2026/10/19/vitalik-buterin-says-eip-8288-could-solve-ethereums-cryptography-bottleneck/); [KuCoin](https://www.kucoin.com/news/flash/ethereum-cryptography-debate-hashes-vs-lattices-as-vitalik-proposes-eip-8288)).

**Inference.** If Ethereum's most PQ-committed voice wants the protocol to verify only a reference plus a proof, with every scheme living outside it, that strengthens the case for a consent protocol to keep cryptography out of its core, which the accord's profile-defined design already does.

What aggregation loses is just as concrete. BLS merges are point additions that anyone can do in microseconds. PQ merges are recursive proofs needing about 0.3 s and over 12 GiB of RAM, so aggregator hardware becomes consensus-critical, and keys must be guarded as state.

**Inference.** Prover cost stays linear. At about 0.7 ms per signature, ~28,000 votes a slot need roughly 20 M4-Max-seconds of proving, which has to be sharded across subnet aggregators. Aggregation only pays above a break-even point. A ~320 KiB proof equals about 270 leanSig signatures of 1,208 B, but only about 11 SLH-DSA-256s signatures. Rogue-key attacks, which forced BLS to require proofs of possession, largely disappear, because the circuit verifies each signature against its registered key. No source states this explicitly.

## Ethereum never trusts a key that travels with a message

### Validators today: one write-once key behind a proof of possession

A validator's identity is its 48-byte BLS public key, fixed by the first processed deposit **[Shipped]**. That deposit carries a BLS signature over (public key, withdrawal credentials, amount) under a fork-agnostic deposit domain. The new key enters the registry only if `is_valid_deposit_signature` passes. Later deposits to the same key are top-ups that check nothing and change nothing ([consensus-specs](https://github.com/ethereum/consensus-specs/blob/master/specs/electra/beacon-chain.md)). The signature doubles as a **proof of possession** against rogue-key attacks ([eth2book](https://eth2book.info/latest/part2/building_blocks/signatures/)).

Verifiers never read a key from an attestation. They rebuild the committee from (slot, committee index), filter it by `aggregation_bits`, and **fetch public keys from the beacon state**. Signing roots mix in a domain that separates message type, fork version and chain ([eth2book](https://eth2book.info/latest/part2/building_blocks/signatures/)).

A second authority is the withdrawal credential. An 0x01 or 0x02 credential holds an execution address that can trigger exits, withdrawals and consolidations, but only when `source_address == withdrawal_credentials[12:]` ([consensus-specs](https://raw.githubusercontent.com/ethereum/consensus-specs/refs/heads/master/specs/electra/beacon-chain.md)). It cannot sign duties or change the signing key.

No operation changes `Validator.pubkey`. Rotation today means depositing a new validator and consolidating the old one into it, or exiting and re-depositing. This is my reading of the spec; no source states it verbatim.

### The PQ registry: withdrawal authority vouches, versions rotate

The leading design is an ethresear.ch post of 1 June 2026 by Coratger, Wambsgans, Ladislaus, Thiery and Drake **[Proposed]**. It registers XMSS keys in a **registry fork first** (I\*) and switches signatures "several forks later". The pieces ([ethresear.ch](https://ethresear.ch/t/exploring-the-design-space-for-a-post-quantum-public-key-registry-for-ethereum-validators/25040)):

- **Operation.** A new `PostQuantumRegistration` carries a 52-byte key: a 32-byte Merkle root plus a 20-byte public parameter.
- **Authorization.** For 0x01 and 0x02 credentials, an L1 signature from the withdrawal address. For 0x00, a signature from the active BLS key.
- **Possession.** One valid XMSS signature over the payload serves as the proof of possession.
- **Rotation.** A version or sequence field, so "bindings are not permanent".
- **Activation.** Windows aligned to 65,536-slot boundaries.
- **Hash agility.** Either a hash-function identifier per entry, or a commitment to two or three hashes up front.

Forum replies pushed further **[Research]**:

- One reply asks for an explicit "authority ended at time T" record, because expiry by exhaustion does not cover compromise.
- Another proposes monotonic "generations", where revocation advances the generation without installing a key.
- A third proposes committing to hashed keys now and choosing the scheme later. A counter-reply warns that hashed BLS commitments must never be used to sign.

A draft PQ-ready deposit contract (24 August 2026) adds a **scheme identifier per deposit** (Scheme 0 = BLS) and lets a later fork irreversibly close Scheme 0 ([crypto.news](https://crypto.news/ethereum-proposes-post-quantum-deposit-contract/)) **[Proposed]**. About 9,200 validators still have 0x00 credentials, whose only authority is a quantum-recoverable BLS key. EIP-8365 would stop new ones, and EIP-8433 would force-exit the rest ([EIP-8365](https://eips.ethereum.org/EIPS/eip-8365); [EIP-8433](https://eips.ethereum.org/EIPS/eip-8433)) **[Proposed]**.

### Accounts: the account code is the signature profile

On the execution layer, the "profile" that picks a signature scheme is per-account, and it sits in a different place in each design:

- **EIP-7702 [Shipped]** lets an EOA run contract code, but its secp256k1 key keeps full authority. It gives agility of code, not of keys ([EIP-7702](https://eips.ethereum.org/EIPS/eip-7702)). EIP-8151's motivation spells out the danger: after an EOA moves to PQ authorization through 7702, whoever holds the old ECDSA key can still drain it through `ecrecover`-based permits ([EIP-8151](https://eips.ethereum.org/EIPS/eip-8151)).
- **EIP-8141 [Decided, SFI Hegotá]** carries a `signatures` list of `[scheme, signer, msg, signature]`. It validates SECP256K1 and P256 natively, and an `ARBITRARY` scheme passes raw bytes to account code through `SIGDATACOPY`. The spec calls this "a native off-ramp" from ECDSA, and keeps protocol-validated signature bytes unreadable by EVM code "to allow future aggregation schemes" ([EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)).
- **EIP-8130 [Proposed]** (Base) registers "actors" in a keystore. Each actor has an authenticator contract, a scope bitmask, an optional expiry and an optional policy. Epoch and sequence counters cancel outstanding signatures ([EIP-8130](https://eips.ethereum.org/EIPS/eip-8130)).
- **EIP-8164 [Proposed]** installs an ML-DSA-44 key as account code and permanently rejects ECDSA afterwards. Because the public key sits inside the signed authorization tuple, "front-running cannot substitute it" ([EIP-8164](https://eips.ethereum.org/EIPS/eip-8164)).
- **EIP-7932 and EIP-8202 [Proposed]** hash an algorithm or scheme identifier into the address: `keccak(algorithm_id || public_key)` in EIP-7932, `keccak256(0x01 || merkle_root)` in EIP-8202. EIP-7932 warns that "allowing more ways to derive addresses for one account may reduce that account's security" ([EIP-7932](https://eips.ethereum.org/EIPS/eip-7932); [EIP-8202](https://eips.ethereum.org/EIPS/eip-8202)).

Retiring the old key takes two more EIPs, both CFI for Hegotá **[Proposed]**:

- **EIP-8298 SETCODEFROM**, together with EIP-3607, "retires the account's key, not the account" ([EIP-8298](https://eips.ethereum.org/EIPS/eip-8298)).
- **EIP-8151** makes `ecrecover` return nothing for accounts that have real code ([EIP-8151](https://eips.ethereum.org/EIPS/eip-8151)).

EIP-8164 also states the consistency requirement bluntly: "All clients must agree on accept/reject decisions … or the chain could split" ([EIP-8164](https://eips.ethereum.org/EIPS/eip-8164)).

**Inference.** Ethereum blocks key substitution in three ways:

- **Identity is an index.** It points into a registry that is written once, after a proof of possession.
- **The key, or a hash of the scheme and key, sits inside the signed bytes.** That covers the EIP-8164 tuple and EIP-7932/8202 address derivation.
- **Rebinding is versioned and authorized** by a separate authority.

The weakest point of the PQ registry is that its authorizing signature is ECDSA or BLS, both quantum-vulnerable. That is why registration is staged first, at I\*. Adding a new key without retiring the old one leaves a backdoor; that is the 7702 lesson.

## Emergency plans hinge on binding PQ keys before Q-day

The only concrete emergency procedure is still Vitalik Buterin's post of 9 March 2024 **[Research]**. It would ([ethresear.ch](https://ethresear.ch/t/how-to-hard-fork-to-save-most-users-funds-in-a-quantum-emergency/18901)):

1. roll back blocks after large-scale theft is evident;
2. disable EOA transactions;
3. require smart-wallet transactions;
4. let owners recover with a STARK proof that they know the hash preimage (for example a BIP-32 seed) behind their key.

Keys generated from raw randomness with exposed public keys are not recoverable. Critics in the thread noted that a fork cannot tell owner from attacker, and that attacks may go unnoticed.

Since then the EF has added two layers of preparation:

- **Organisation.** A PQ Security team formed in January 2026, and ethereum.org says users need take no action now, because wallets will guide migration "after Hegotá and EIP-8141" ([ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)).
- **Protocol.** A planned **MV-PQ** safeguard at J\*, added to the strawmap on 19 August 2026. It is meant to keep the chain running through Q-day with reduced guarantees, which are still being defined ([EF Blog](https://blog.ethereum.org/2026/09/07/protocol-priorities); [CryptoSlate](https://cryptoslate.com/ethereums-2029-quantum-deadline-is-starting-to-reshape-what-gets-built-next/)).

The EF estimates about 0.1% of ETH sits in quantum-vulnerable dormant accounts, against about 5% for Bitcoin. Whether to freeze them has "no community consensus" ([pq.ethereum.org](https://pq.ethereum.org/)). ethereum.org cites two external drivers ([ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)):

- Google Quantum AI estimates about 1,200 logical qubits could break 256-bit elliptic-curve cryptography.
- NIST expects to deprecate ECDSA by 2030 and disallow it by 2035.

**Inference.** The emergency plan's preconditions are partly on the roadmap:

- **Step 3 (smart-wallet transactions)** comes with EIP-8141 native account abstraction.
- **A cleaner kill switch than "disable EOA transactions"** would come from EIP-8298 plus EIP-8151.

The STARK preimage-recovery transaction still has no EIP, so a real emergency would force it to be built under pressure. The common thread is timing. Every migration path needs a quantum-safe key bound by a still-trustworthy classical authority **before** Q-day: the I\* registry, the EIP-8164 authorization, and pre-committed hashed keys. Key-derivation provenance (whether a seed is hash-derived) decides who can still recover afterwards.

## Open registries feed small, snapshotted quorums

Ethereum's validator set is permissionless, but its quorums are not ad hoc. Gasper finalizes when **two-thirds of total staked ether** link successive checkpoints ([ethereum.org](https://ethereum.org/developers/docs/consensus-mechanisms/pos/gasper/)) **[Shipped]**. Three mechanisms keep that threshold meaningful:

- **Churn limits.** Entry and exit are capped at 128–256 ETH per epoch ([consensus-specs](https://github.com/ethereum/consensus-specs/blob/master/specs/electra/beacon-chain.md)).
- **Inactivity leak.** If finality stalls for more than four epochs, absent validators' stake drains until they hold less than a third ([ethereum.org](https://ethereum.org/developers/docs/consensus-mechanisms/pos/rewards-and-penalties/)).
- **Slashing.** Attributable double-signing is penalized, with a 36-day exit ([ethereum.org](https://ethereum.org/developers/docs/consensus-mechanisms/pos/rewards-and-penalties/)).

Lean consensus is moving to faster finality with higher thresholds and smaller signer sets **[Research]**:

- **Minimmit** needs n ≥ 5f+1, notarizes at more than 40% and finalizes at more than 80%. It assumes a known n and a fixed leader schedule ([Dankrad Feist](https://dankradfeist.de/tempo/2025/12/31/minimmit-simple-fast-consensus.html)).
- **The devnet-5 "PQ heartbeat"** uses committee-based voting under the Goldfish fork-choice rule ([leanroadmap.org](https://leanroadmap.org/)).
- **Vitalik Buterin estimates** 256–1,024 signatures per slot, where STARK aggregation may not even be needed ([X](https://x.com/VitalikButerin/status/2027075026378543132)).
- **Orbit SSF** caps the active set near 32,000. Large validators are always active, and a **slowly** rotating periphery of small ones stops adversaries from accumulating weight across fast-changing committees ([ethresear.ch](https://ethresear.ch/t/orbit-ssf-solo-staking-friendly-validator-set-management-for-ssf/19928)).
- **"Extremely Lean"** goes further: it shrinks a validator to a 5-byte deposit index with daily STARK-proven participation. A second phase re-anonymizes keys daily ([Unchained](https://unchainedcrypto.com/vitalik-buterin-proposes-extremely-lean-ethereum-that-cuts-validator-state-by-95-using-zk-proofs/)).

**Inference.** Ethereum separates **who may vote**, an open registry of about a million keys priced by stake, from **who votes now**, a bounded committee whose membership is fixed for each period. A threshold needs a fixed denominator. A Sybil cost makes the threshold mean something. Slow churn stops weight from being accumulated. Attributable signatures make equivocation punishable. And a liveness valve, the inactivity leak, lets a stalled quorum recover. Without any one of these, a quorum stops being a safety guarantee.

## What this means for the canor_elite accord

These facts come from the repository:

- **The roster leaves out the key.** The accord's canonical roster encodes each signatory's name, role, external flag and **profile digest**, but not its `key` ([canor_elite.c](../source/src/canor_elite.c), `canor_elite_enc_roster`).
- **Verify trusts the struct's key.** The profile's `verify` callback receives that signatory record, key included, straight from the accord struct ([canor_elite.h](../source/include/canor_elite.h)).
- **Roster changes void consents.** Adding a signatory resets every ratification and approval ([canor_elite.c](../source/src/canor_elite.c), `canor_elite_accord_add_signatory`).
- **Signature count.** There are S ratifications, S(S−1)·C certifications and S approvals. At the caps (S = C = 8) that is **464 signatures**. With 29,792-byte SLH-DSA-SHAKE-256s signatures this comes to **≈13.8 MB** ([TRUST.md](../CanorLinguae/elite/TRUST.md)).
- **The profile closes the gap today.** The opt-in `pqc-slh-dsa` profile closes the key-swap gap with no core change: its `verify` checks `who->key` against the root key declared in the `.fwp` behind `who->profile` ([TRUST.md](../CanorLinguae/elite/TRUST.md)).
- **No syntax for root keys yet.** The `.fwp` syntax for declaring a root key does not exist yet.
- **Core options need approval.** Binding keys into the core roster, and bundling certifications into one signature per certifier, were both reverted on 2026-10-21. They need explicit approval before they return.

### Close the key-swap gap where attribution is decided

**Inference.** The accord currently does what Ethereum's consensus layer is built to avoid: it verifies against a key that travels with the record rather than one fixed by signed bytes. Profile-level binding corrects this the way Ethereum's execution layer does, with the account (here the profile) defining verification. Core-level binding would correct it the way the consensus layer does. Both are algorithm-neutral, and they differ in who enforces the binding:

| Property | Profile level (TRUST.md as written) | Core level (opaque `key` blob added to the roster encoding) |
|---|---|---|
| Ethereum analog | Account code / keystore (EIP-8141, EIP-8130) | Write-once registry key with PoP; key inside the signed tuple (EIP-8164) |
| Core change | None | Every ratify/approve byte string changes |
| Algorithm neutrality | Yes | Yes: the core encodes bytes and interprets none |
| Enforced by | Each evaluator's `verify` | Every party's signature over the roster |
| Evaluators running different profiles | Can disagree: one checks, one does not (the "chain split" risk EIP-8164 warns of) | Agree: a swapped key breaks all N ratifications and approvals |
| Prerequisites | `.fwp` root-key syntax; evaluator holds the `.fwp` bytes | None |
| Key attestation | Indirect, through the profile digest | Direct: every party signs every party's key, which is stronger than Ethereum's single-authority registration |
| Root-key rotation | Changes the `.fwp` digest, so stale signatures stop verifying and all parties re-sign | Same, via the roster |
| Status here | Current scope | Reverted 2026-10-21; needs explicit approval |

Within the profile route, Ethereum's experience points to four refinements:

- **Bind the algorithm with the key.** This follows the scheme identifier per deposit, `algorithm_id || public_key`, and the hash identifier per registry entry. A key's meaning is fixed only together with its scheme, and NIST-conformant and Ethereum-variant parameter sets are treated as distinct identifiers. The `.fwp` digest covers this once the syntax exists.
- **Make working-key endorsements monotonic.** Give them a generation number, and let a revocation advance it without installing a key (the "generations" reply on the registry thread). Endorsing a new working key without retiring the old one recreates the EIP-7702 backdoor.
- **Fail closed when the `.fwp` declares no root key.** TRUST.md already maps unknown keys to UNDEFINED, and that should cover this case too.
- **Close the split risk for General Public work.** Where evaluators may run different profiles, the profile-level route can still split verdicts. The clean fix is one mandated profile per project, or core binding if approved.

### Batch signatures at the profile before reaching for proofs

**Inference.** Θ(S²·C·σ) has two factors to attack: the S²·C count of signed statements, and σ. Ethereum attacks both: the devnet-5 multi-message proof and EIP-8288 cut the count, and leanSig's Level 1 parameters cut σ. Proofs only pay at scale. At the accord's caps, Merkle batching inside the profile gets most of the benefit with no core change. A party signs one SLH-DSA signature over a Merkle root of the SHA3-512 digests of all statements it signs in one session. Each statement's 64-byte blob then references the batch, and the profile's store holds the path. `verify` already receives the exact canonical statement bytes and the blob, so it can recompute the leaf, walk the path and check the single signature. The figures below are my arithmetic, at S = C = 8 with 64-byte path nodes.

| Option | Level | Signatures | Bytes | Reduction | Ethereum analog |
|---|---|---|---|---|---|
| Current: one SLH-DSA-SHAKE-256s per statement | — | 464 | 13.82 MB | 1× | devnet-1 concatenation |
| SLH-DSA-SHAKE-128s (7,856 B per [AppliedPQC](https://github.com/AppliedPQC/pqc-research/issues/35)) instead of 256s | Profile | 464 | 3.65 MB | 3.8× | leanSig targets NIST Level 1 |
| Merkle batch per (certifier, subject) | Profile | 72 | 2.23 MB | 6.2× | Signing a Merkle root of an object (SSZ signing roots) |
| Merkle batch per signing session (ratify, certifications, approve) | Profile | 24 | 0.89 MB | 15.6× | One multi-message block proof (devnet-5) |
| Per-session batch with 128s | Profile | 24 | 0.36 MB | 38× | Both of the above |
| One bundled certification statement per certifier | Core (reverted) | 24 | 0.72 MB | 19.3× | Pre-aggregated committee attestation (EIP-8243) |
| STARK aggregation of all 464 | Profile, research | 1 proof | ~0.33 MB | ~42× | leanVM, but SLH-DSA-SHAKE-256s inside leanVM is not benchmarked |

The asymptotics move as follows:

| Option | Storage class |
|---|---|
| Current | Θ(S²·C·σ) |
| Per-subject batching | Θ(S²·σ + S²·C·h·log C) |
| Per-session batching | Θ(S·σ + S²·C·h·log(S·C)), with h = 64 B |

Because σ/h ≈ 465, the signature term dominates at any realistic accord size.

Batching has four caveats:

- **Domain tag.** The batch root needs its own domain tag, so a root can never be confused with a statement. This is Ethereum's domain-separation lesson.
- **Re-signing.** Changing any one submission forces a re-signed batch. The accord already voids certificates over a changed submission, so this costs little.
- **Pending subjects.** A certifier who is still waiting on one subject must either batch per subject or encode PENDING leaves.
- **No stateful schemes.** Do not chase leanSig-sized signatures with a stateful scheme. The accord has no slot clock to index leaves, so it would need EIP-8310's commit-before-sign discipline for a handful of signatures. NIST SP 800-208's warning applies directly.

### Give General Public projects a registry, a snapshot and a price of entry

**Inference.** The unanimity accord is the n-of-n corner of Ethereum's design space. One hostile or absent party vetoes everything, which suits a contract but not a public project. Open participation also breaks the storage model. At 1,000 participants and 8 categories, all-pairs certification is about 8 million statements, roughly 238 GB of SLH-DSA-256s signatures. Sampled certification with 16 certifiers per subject is about 3.9 GB unbatched, or about 89 MB with per-session batching. That is the scale where STARK aggregation becomes worth its cost.

Ethereum's working pattern suggests five rules:

1. **Keep the registry outside the accord.** It can be open and large, but each decision snapshots a bounded roster, which fits the fixed-capacity invariant. The existing rule that any roster change voids consents already implements "the snapshot is the denominator". New registrants join the next round, as Ethereum's activation windows and churn limits do.
2. **Price identity.** Without stake, a profile-defined admission rule (endorsement by existing members, or external attestation) must make Sybil identities costly. Without one, any threshold below unanimity is meaningless.
3. **Replace all-pairs certification with committees.** Draw k certifiers per subject by a deterministic integer shuffle seeded from the basis digest, giving Θ(S·k·C) statements. Keep a per-item participation record, because attribution survives aggregation only if it is designed in.
4. **Pair the threshold with accountability and liveness.**
   - **Threshold:** 2f+1 of 3f+1 tolerates f Byzantine parties. Minimmit-style 80% buys one-round finality.
   - **Accountability:** the core can diagnose equivocation, such as two signed approvals by one party over different object digests, which fits "diagnose, never prune".
   - **Liveness:** an inactivity rule can drop persistent non-participants from the next snapshot.
5. **Use one profile for everyone, and plan for public verifiability.** Public, unencrypted participation needs integrity, not confidentiality, so the hybrid KEM is irrelevant there. Hash-based signatures made today stay valid after Q-day. Mandating one PQ profile per General Public project removes both the evaluator-split risk and the weakest-link classical signer.

## Conclusion

Ethereum's hardest post-quantum problem has turned out to be **binding**, not the signature algorithm. The scheme is settled for consensus and narrowing for accounts, and aggregation is a solved engineering problem. Yet the first protocol step on the roadmap is a key registry, because every later migration depends on a quantum-safe key bound by an authority that is still trustworthy. The canor_elite accord faces the same ordering. Its key-swap gap matters more than its 13.8 MB worst case, since a batched or aggregated signature over an unbound key is still forgeable consent. The profile route closes the gap for any evaluator that runs it. Only a binding inside the bytes every party signs makes all evaluators agree.

The second insight is that Ethereum never asks its whole membership to sign everything. It separates an open, slowly changing registry from bounded per-period quorums, and reserves heavy cryptographic machinery (recursive STARKs) for the scale where linear signing breaks. Applied here, unanimity accords and General Public projects can share one core: the accord's roster becomes the per-decision snapshot. Merkle batching covers accord-scale storage. Aggregation enters only when participation reaches the thousands, and by then Ethereum's own hash choice (SHA3 remains a candidate) should be settled.
