# Post-Quantum Cryptography at Ethereum's Execution Layer: Accounts, Transactions, EVM Precompiles and Quantum-Emergency Plans (state as of 2026-10-21)

Status tags: **DECIDED/SHIPPED** = on mainnet, Final, or Scheduled for Inclusion (SFI) in a fork meta-EIP. **PROPOSED (EIP-n)** = Draft/CFI/PFI EIP or ERC, or Declined. **RESEARCH** = forum post, roadmap item or paper with no EIP. "Secondary" marks claims taken from aggregators or third-party analyses rather than the EIP or author.

## Q1. Account abstraction as signature agility: ERC-4337, EIP-7702, native AA (EIP-7701, RIP-7560, EIP-8141, EIP-8130)

### Takeaway
Ethereum's chosen route to PQ accounts is *signature agility through account abstraction*, not one forced cutover. EIP-7702 (shipped in Pectra, 2025-05-07) lets an EOA run smart-account code, but its ECDSA key remains in control. ERC-4337 smart accounts can already verify any scheme in contract code. The protocol-level vehicle is **EIP-8141 Frame Transactions** (created 2026-01-29). It is **Scheduled for Inclusion in Hegotá** alongside FOCIL, with Hegotá activation projected for mid/late 2027. EIP-7701 is withdrawn (superseded by EIP-8141). RIP-7560 remains a draft for rollups. Base's EIP-8130 "Keystore Accounts" competes with or complements EIP-8141.

### Cited Findings
**EIP-7702 (Set Code for EOAs): DECIDED/SHIPPED**
- EIP-7702 is "Final", created 2024-05-07. It uses tx type `0x04` (`SET_CODE_TX_TYPE`). The authorization tuple is `[chain_id, address, nonce, y_parity, r, s]`, signed over `keccak(MAGIC=0x05 || rlp([chain_id, address, nonce]))`. The delegation indicator `0xef0100 || address` is written to the EOA's code. Gas: `PER_AUTH_BASE_COST` 12,500 and `PER_EMPTY_ACCOUNT_COST` 25,000, with a refund if the account is non-empty. — [EIP-7702](https://eips.ethereum.org/EIPS/eip-7702)
- Pectra (EIP-7600, Final) includes EIP-7702, EIP-7623 (calldata cost increase) and EIP-2537 (BLS12-381 precompile). Mainnet activation was epoch 364032, Unix 1746612311, which is 2025-05-07 10:05 UTC (date converted from the timestamp). Sepolia activated 2025-03-05 and Hoodi 2025-03-26. — [EIP-7600 Pectra meta](https://eips.ethereum.org/EIPS/eip-7600)
- EIP-7702's text does not discuss revoking the ECDSA key or post-quantum security. Delegating to the zero address clears the delegation, but the original secp256k1 key keeps full authority. — [EIP-7702](https://eips.ethereum.org/EIPS/eip-7702)
- EIP-7702 is therefore agility of *code*, not of *keys*. EIP-8151's motivation says an EOA that migrates to PQ authorization via 7702 can still be drained by whoever holds the old ECDSA key, through contracts that rely on `ecrecover` (e.g. ERC-2612 `permit`). — [EIP-8151](https://eips.ethereum.org/EIPS/eip-8151)

**ERC-4337 (smart accounts, EntryPoint, alt-mempool): DECIDED/SHIPPED at the application layer (no consensus change)**
- ERC-4337 smart accounts do their own signature validation, so an account can adopt any scheme in contract code. The main limits are gas cost and bundler simulation rules. One limit noted: the "Bundler still uses ECDSA", i.e. the outer transaction that carries UserOps is ECDSA-signed (secondary, 2026-09 survey). — [AppliedPQC issue #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- A 2024-03 ethresear.ch reply (user "pldd") reports Lamport-signature smart-contract wallets already built on ERC-4337, including a Chrome-plugin wallet. — [ethresear.ch quantum-emergency thread](https://ethresear.ch/t/how-to-hard-fork-to-save-most-users-funds-in-a-quantum-emergency/18901)
- EntryPoint v0.7 is the widely agreed singleton at `0x0000000071727De22E5E9d8BAf0edAc6f37da032` (secondary). — [eco.com 4337 vs 7702 vs native AA](https://eco.com/support/en/articles/15254047-erc-4337-vs-erc-7702-vs-native-aa-2026-standards-compared)
- EntryPoint v0.9 fixed a griefing vector in which a malicious actor could make valid, signed UserOps revert while the sender still paid gas. — [ERC-4337 team Substack](https://erc4337.substack.com/p/improving-useroperation-execution)
- v0.9 adoption lags. A Kaia forum thread reports infrastructure providers still prioritising v0.6/v0.7. — [Kaia dev forum](https://devforum.kaia.io/t/erc-4337-entrypoint-v0-9/9090)
- Safe's 4337 module documents an overhead of about 12.1k gas with fee payment and about 10.3k without. These are module-specific figures, and the docs note that actual overhead is higher. — [@safe-global/safe-4337 (npm)](https://www.npmjs.org/package/@safe-global/safe-4337)

**EIP-7701 (Native AA): PROPOSED → Withdrawn**
- EIP-7701 status is "Withdrawn", reason "Superseded by EIP-8141". Authors: Buterin, Weiss, Forshtat, Tirosh, Nacson. Created 2024-05-01. It had an `AA_ENTRY_POINT` at `address(0x7701)` with a 15,000 gas base, role constants `0xA0`–`0xA4`, and `CURRENT_ROLE`/`ACCEPT_ROLE`/`TXPARAM*` opcodes. — [EIP-7701](https://eips.ethereum.org/EIPS/eip-7701)

**RIP-7560 (native AA for rollups): PROPOSED (RIP, draft)**
- Created 2023-09-01 and still draft as of 2026 (secondary). It defines an `AA_TX_TYPE` with separate validation, execution and post-op gas limits. No rollup was found that has shipped it. — [eco.com RIP-7560](https://eco.com/support/en/articles/11564615-what-is-rip-7560-native-account-abstraction-for-l2s)
- Etherspot describes RIP-7560 as backward-compatible with ERC-4337. — [Etherspot blog](https://etherspot.io/blog/ethereums-trustless-manifesto-metamask-launches-multichain-accounts-rip-7560-explained-x402-protocol-challenges-aa-limitations-and-eip-7702-infra-workshop-at-devconnect-by-etherspot/)

**EIP-8141 (Frame Transaction): DECIDED (SFI in Hegotá); spec still Draft**
- Created 2026-01-29, Standards Track Core, Draft. Authors: Vitalik Buterin, lightclient, Felix Lange, Yoav Weiss, Alex Forshtat, Dror Tirosh, Shahaf Nacson, Derek Chiang, Toni Wahrstätter, Stavros Vlachakis. It requires EIP-7702, 7825, 7976, 8037 and others. — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- Tx type `0x06`, payload `[chain_id, nonce, sender, frames, signatures, fees, blob_versioned_hashes]`, `MAX_FRAMES = 64`. Frame modes are `DEFAULT` (0), `VERIFY` (1, STATICCALL-like; only `APPROVE` can change context) and `SENDER` (2). Approval scopes are NONE/PAYMENT/EXECUTION/EXECUTION_AND_PAYMENT, and there is an atomic-batch flag. — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- New opcodes: `APPROVE` (0xaa), `TXPARAM` (0xb0), `FRAMEDATALOAD`/`FRAMEDATACOPY` (0xb1/0xb2), `FRAMEPARAM` (0xb3), `SIGPARAM` (0xb4) and `SIGDATACOPY` (0xb5). — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- **How agility works:** a `signatures` list carries `[scheme, signer, msg, signature]`. Protocol-validated schemes are `SECP256K1` (0x1, 65 B, low-s enforced, 2,800 gas) and `P256` (0x2, 128 B, 6,700 gas). `ARBITRARY` (0x0, 100 gas) carries bespoke bytes that account code reads via `SIGDATACOPY` and verifies itself. The spec calls this "a native off-ramp" from ECDSA to PQ: future PQ schemes are added as new scheme values, and protocol-validated signature bytes are kept unreadable by EVM code "to allow future aggregation schemes". — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- **EOAs:** accounts with no code and no 7702 delegation get *default code*. In `VERIFY` mode it requires a `SECP256K1` signature by the target, so existing EOAs get batching, sponsorship and ERC-20 gas without changing code. The spec omits 7702-style authorization lists "because they rely on ECDSA". — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- **Gas (provisional "benchmark targets"):** `FRAME_TX_INTRINSIC_COST` 12,000, `FRAME_TX_PER_FRAME_COST` 475, `TX_VALUE_COST` 6,000 per value-carrying frame to a non-sender. Calldata is 4 gas/token with a 16 gas/token floor (EIP-7976). — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- **Mempool:** `MAX_VERIFY_GAS = 100,000` and `MAX_VERIFY_STATE_GAS = 500,000`. Recognised validation prefixes are `[self_verify]`, `[deploy, self_verify]`, `[only_verify, pay]` and `[deploy, only_verify, pay]`. Validation bans TIMESTAMP (except in the expiry verifier), BALANCE, SSTORE, CREATE* and others, and SLOAD may read only sender storage. A non-canonical paymaster may have at most 1 pending tx. — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- **Hegotá status:** the Hegotá meta (EIP-8081, Draft, created 2025-11-11) lists **SFI: EIP-7805 (FOCIL) and EIP-8141**. Its activation table is still empty. — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
- **Path to SFI:** ACDE #244 reached "consensus toward inclusion rather than a formally ratified decision", with Ethrex, Geth, Nethermind and Besu in support. EL preferences were due 2026-09-09 (Etherspot, 2026-09-02). — [Etherspot digest](https://etherspot.io/blog/eip-8141-advances-toward-hegota-inclusion-a-composable-take-on-native-aa-glamsterdam-repricing-tests-compatibility-a-post-quantum-path-for-ecrecover/)
- **Confirmation (secondary):** a survey says the headliner was confirmed at ACDE #245 on 2026-09-10. — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **Conflicting date (secondary):** COINOTAG reports a formal commitment on Aug 27. — [COINOTAG](https://en.coinotag.com/ethereum-eip-8141-frame-transactions-hegota-2027)
- **"Must-ship":** an EF Protocol-cluster priorities document (via Cointelegraph, reported 2026-09-08) calls FOCIL and Frame Transactions "must-ship". If either slips, the fork slips rather than the feature being cut. — [Cryptonomist](https://en.cryptonomist.ch/2026/09/08/ethereum-hegota-upgrade/)
- **Stale page:** ethereum.org's quantum-resistance page (last updated 2026-09-08) still says EIP-8141 is "being considered" for Hegotá. Treat EIP-8081 as the more current source. — [ethereum.org quantum resistance](https://ethereum.org/roadmap/security/quantum-resistance/)
- **Hegotá timing:** scoping targeted completion by Devcon (2026-11-03), and client implementation may start late Q4 2026. Projected activation is 2027-05-19 (EIPsInsight) or 2027-06-16 (Forkcast), both planning assumptions; the survey's own estimate is July–December 2027 (secondary). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **Glamsterdam context:** proposed Sepolia 2026-09-28, Hoodi 2026-10-26, mainnet tentatively early December 2026. — [Etherspot digest](https://etherspot.io/blog/eip-8141-advances-toward-hegota-inclusion-a-composable-take-on-native-aa-glamsterdam-repricing-tests-compatibility-a-post-quantum-path-for-ecrecover/)
- **Fusaka** went live 2025-12-03. — [Cryptonomist](https://en.cryptonomist.ch/2026/09/08/ethereum-hegota-upgrade/)

**EIP-8130 (Keystore Accounts, Base/Coinbase): PROPOSED (EIP-8130), not listed in the Hegotá meta**
- Created 2025-10-14 by Chris Hunter. Accounts register *actors* in an on-chain Keystore. Each actor has an authenticator contract, a scope bitmask, an optional expiry and an optional policy. The EOA's own secp256k1 key is an implicit admin actor. The canonical authenticators are k1, p256, passkey and delegate. — [EIP-8130](https://eips.ethereum.org/EIPS/eip-8130)
- New schemes are added as new authenticator contracts, and accounts adopt them by authorising a new actor "without migration or address changes". The initial set contains no PQ scheme, but it is "expected to grow as signature algorithms mature (e.g., post-quantum)". PQ public keys arrive in signature data rather than storage, so large keys do not increase per-actor storage reads. — [EIP-8130](https://eips.ethereum.org/EIPS/eip-8130)
- **Rotation:** `AuthorizeActor`/`RevokeActor` batches, with an epoch and sequence counter to cancel outstanding signatures. Gas: 22,100 per initial actor slot, 2,100 for an `actor_config` read, and 32,000 + 200/byte for create. — [EIP-8130](https://eips.ethereum.org/EIPS/eip-8130)
- EIP-8130 is championed by Base and parts of the L2 ecosystem. As of 2026-09-02 it was being reconciled with EIP-8141: a shared tx type is preferred, with account interoperability as the fallback. — [Etherspot digest](https://etherspot.io/blog/eip-8141-advances-toward-hegota-inclusion-a-composable-take-on-native-aa-glamsterdam-repricing-tests-compatibility-a-post-quantum-path-for-ecrecover/)
- EIP-7851 (Code-Controlled EOA Delegation) is Declined for Hegotá. — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)

**Vitalik's roadmap framing: RESEARCH**
- Vitalik's quantum roadmap post (X, 2026-02-26) names four vulnerable components: CL BLS, KZG data availability, EOA ECDSA signatures and app-layer pairing ZK proofs. For EOAs it proposes native AA via frame transactions so accounts can choose quantum-safe schemes. — [vitalik.eth on X](https://x.com/VitalikButerin/status/2027075026378543132); [Cryptotimes, 2026-02-27](https://www.cryptotimes.io/2026/02/27/vitalik-buterin-outlines-ethereums-quantum-resistance-roadmap/)
- pq.ethereum.org (last updated 2026-06-27) describes the EL approach as "gradual, opt-in migration rather than a single forced switch". Smart accounts can upgrade their verification logic "without protocol-level intervention". — [pq.ethereum.org](https://pq.ethereum.org/)

### Inferences
- The per-account "profile" that chooses a signature scheme lives in different places in each design. Under ERC-4337 and EIP-7702 it is the account's code (with 7702 the ECDSA key cannot be retired). Under EIP-8141 it is the account's `VERIFY`-frame code plus the scheme byte in the `signatures` list. Under EIP-8130 it is the Keystore actor set (authenticator plus scope). Under EIP-8164 it is the code-prefix byte (`0xef0101`…`0xef01ff`). Under EIP-8202 it is `scheme_id` hashed into the address.
- For a PQ account, EIP-8141's practical constraint is the 100,000-gas `MAX_VERIFY_GAS` public-mempool cap. Only very cheap PQ verifiers (hash-based Keccak variants, or a precompile) fit; pure-Solidity Falcon/ML-DSA does not (see Q2). Until a PQ precompile or EIP-8288 aggregation ships, PQ frame-tx accounts would need private or alternative mempools or very gas-tuned hash-based schemes.
- Native PQ account migration on L1 cannot start before Hegotá activates (projected mid/late 2027). Before then, PQ accounts are possible only as ERC-4337 or 7702-delegated contracts that keep an ECDSA dependency (the bundler EOA, or the un-revocable 7702 key).

### Gaps
- No authoritative gas-overhead benchmark for ERC-4337 EntryPoint v0.8/v0.9 per UserOp was found. ERC-4337's formal EIP status (Draft/Final) as of 2026-10 was not confirmed.
- The exact date of formal SFI for EIP-8141 is unclear: Aug 27 (COINOTAG), ACDE #244 (Etherspot) and ACDE #245 on 2026-09-10 (AppliedPQC). The EIP-8081 SFI listing is confirmed but carries no date.
- The final outcome of the EIP-8130/EIP-8141 reconciliation (shared tx type or not) was not found.
- The official RIP repository was not checked for RIP-7560's current status (secondary source only).

## Q2. PQ verification in the EVM: precompiles, opcodes and PQ transaction types

### Takeaway
As of 2026-10, **no PQ signature precompile is scheduled for any fork**. Several Drafts exist: EIP-7619 (Falcon-512, 2023), EIP-7885 (NTT), EIP-7932 (secondary-signature-algorithm registry and `sigrecover`), EIP-8051 (ML-DSA-44, 4,500 gas), EIP-8052 (Falcon-512, 3,000 gas) and EIP-8355 (ML-DSA, *Declined* for Hegotá). EF roadmap milestone J* covers "PQ signature precompiles" via a *vector math precompile*, not one precompile per scheme. For the EL, the team says it is evaluating Falcon, Dilithium and SPHINCS+ and favours agility. Long-term the direction is hash-based (leanSPHINCS) signatures plus STARK aggregation (EIP-8288). PQ transaction-type proposals are EIP-8164 (ML-DSA-44 native key) and EIP-8202 (Merkle tree of one-time ECDSA keys).

### Cited Findings
**Precompiles and opcodes**
- **EIP-8051 "Precompile for ML-DSA signature verification": PROPOSED (EIP-8051), Draft.** Created 2025-10-15 by Renaud Dubois and Simon Masson (ZKNox); requires EIP-7932. Two precompiles: `VERIFY_MLDSA` at 0x12 (SHAKE256 per FIPS 204) and `VERIFY_MLDSA_ETH` at 0x13 (Keccak-based PRNG, `t1` in NTT domain), each **4,500 gas**. Gas is estimated by linear pricing against keccak256, with no measured benchmarks; an EIP-7932 `GAS_PENALTY` of about 3,000 is "subject to benchmarking". — [EIP-8051](https://eips.ethereum.org/EIPS/eip-8051)
  - Covers ML-DSA-44 only ("NIST level II … 128 bits"). Signature 2,420 B, message 32 B. The expanded public key is 20,512 B (A_hat 16,384 + tr 32 + t1-NTT 4,096) versus 1,312 B standard.
  - Rationale: NIST standardisation, moderate sizes, ZK-friendly parameters, and a simpler signer than FN-DSA. The security section is minimal ("tbd" on seed-to-key derivation).
- **EIP-8052 "Precompile for Falcon support": PROPOSED (EIP-8052), Draft.** Created 2025-10-17. Authors: Dubois, Masson, Antonio Sanso, Marius van der Wijden, Kevaundray Wedderburn, Zhenfei Zhang, Nicolas Consigny. Requires EIP-7932. Falcon-512 (n=512, q=12289). — [EIP-8052](https://eips.ethereum.org/EIPS/eip-8052)
  - Three functions: `FALCON_HASH_TO_POINT_SHAKE256` (NIST-compliant, 1,000 gas), `FALCON_HASH_TO_POINT_KECCAKPRNG` (EVM/ZK-friendly, 1,000 gas) and `FALCON_CORE` (2,000 gas), so **3,000 gas** per verification. Gas is interpolated from SUPERCOP benchmarks. Addresses are "TBD by ACD".
  - Sizes: public key 896 B, signature 666 B (40 B salt + 626 B compressed).
- **EIP-7619 (Falcon-512 generic verifier): PROPOSED (EIP-7619), Draft** (2023-07-03). About 1,465 gas base + 6/word (secondary). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **EIP-7885 (NTT operations precompile): PROPOSED (EIP-7885), Draft** (2025-02-12). ETHFALCON was measured at 479,341 gas on an OP-Stack testnet using it (secondary). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **EIP-7932 "Secondary Signature Algorithms": PROPOSED (EIP-7932), Draft.** Created 2025-04-12 by James Kempton. — [EIP-7932](https://eips.ethereum.org/EIPS/eip-7932)
  - Defines an `algorithm_registry` keyed by a type byte (<127; `0x7F` invalid) and a `sigrecover` precompile at 0x12 (base 3,000 gas) that returns the 20-byte signer address.
  - The EIP itself specifies only secp256k1 (0x00); each new algorithm needs its own EIP.
  - Precompile address conflict: EIP-8051 also claims 0x12. Both are drafts.
- **EIP-8355 (ML-DSA Verification Precompiles): PROPOSED → Declined for Inclusion in Hegotá.** — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
  - It was graded C-tier in the EF Hegotá review (secondary). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- EIP-8051, 8052, 7932, 8164 and 8202 do not appear in any Hegotá list (SFI/CFI/PFI/DFI). — [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
- **EF milestone J\*: RESEARCH (roadmap).** "PQ signature precompiles" are an EL item, and "verification uses a vector math precompile". The EL order is J* → PQ transactions → M* (PQ signature aggregation, PQ blobs). There are no per-milestone dates, the roadmap lives at strawmap.org and is "subject to change". — [pq.ethereum.org](https://pq.ethereum.org/)
  - ethereum.org lists J* as "post-quantum signature verification precompiles". — [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)
  - Ordering conflict: the pq.ethereum.org roadmap line puts PQ transactions between J* and M*, but its milestone table lists them under "Longer term" (secondary). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **Favoured schemes: RESEARCH.** For the EL, pq.ethereum.org lists "Falcon, Dilithium, and SPHINCS+" as candidates under evaluation and favours cryptographic agility. Its stated risks of committing early are cryptanalysis, DoS from large keys, immature libraries and no safe rollback. The CL has chosen hash-based leanXMSS (Rust: leanSig). — [pq.ethereum.org](https://pq.ethereum.org/)
  - Vitalik's 2026-02-26 roadmap calls the hash-function choice possibly "Ethereum's last hash function". — [Cryptotimes](https://www.cryptotimes.io/2026/02/27/vitalik-buterin-outlines-ethereums-quantum-resistance-roadmap/)
  - EIP-8288 (Buterin and Coratger) defines only hash-based **leanSPHINCS** as a valid signature dependency, with leanSTARK for proofs. This is a strong signal that the EF's long-term EL favourite is hash-based plus STARK-aggregated rather than lattice. — [EIP-8288](https://eips.ethereum.org/EIPS/eip-8288)
- **Pure-EVM verification cost (secondary):** ECDSA verification is "about 3000 gas" and quantum-resistant alternatives "could require around 200,000 gas". — [CryptoPotato on Vitalik's roadmap](https://cryptopotato.com/vitalik-buterin-unveils-ethereums-comprehensive-quantum-resistance-roadmap/)
  - EIP-8288 cites 150,000–200,000 gas for on-chain signature verification. — [EIP-8288](https://eips.ethereum.org/EIPS/eip-8288)
- **Measured contract costs (secondary, 2026-09 survey):**
  - Pure-contract SLH-DSA-Keccak-128-24 costs about 94,000 gas, which fits EIP-8141's 100k `MAX_VERIFY_GAS` with roughly 6k headroom. SPHINCS− C13 costs 127,000 (over the cap).
  - Pure-contract Falcon costs 650,345 to 1.9M gas. NIST reference Falcon costs 3.9M–7M and ETHFALCON 1.5M–1.9M. One independent measurement puts Falcon at about 6.6M and ML-DSA at about 13.5M.
  - Source: [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **Precompile pricing discrepancy (secondary):** ML-DSA verification is priced at 4,500 gas in EIP-8051 but 50,000 in EIP-8164. The survey flags underpricing as a DoS risk. — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **Standards status:** NIST finalised FIPS 203/204/205 (ML-KEM, ML-DSA, SLH-DSA) in Aug 2024. — [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)
  - FIPS 206 (FN-DSA/Falcon) was submitted for approval on 2025-08-28 and is expected late 2026 or early 2027; NIST has committed to no date (secondary). EIP-8052 therefore specifies Falcon before FIPS 206 parameters are final. — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)

**PQ transaction types**
- **EIP-8164 "Native Key Delegation for EOAs": PROPOSED (EIP-8164), Draft.** Created 2026-02-17 by Gregory Markou and James Prestwich. — [EIP-8164](https://eips.ethereum.org/EIPS/eip-8164)
  - An ECDSA-signed tuple in a type `0x06` tx's `native_key_authorization_list` sets account code to `0xef0101 || ML-DSA-44 pubkey` (1,315 B).
  - After that, ECDSA txs and 7702 tuples from the account are permanently rejected. The account sends ML-DSA-44-signed txs (2,420 B signature) with `ML_DSA_44_VERIFY_COST` of 50,000 intrinsic gas.
  - Prefixes `0xef0102`–`0xef01ff` are reserved for future schemes.
  - Note: its type byte `0x06` collides with EIP-8141's `FRAME_TX_TYPE = 0x06`. Both are drafts.
- **EIP-8202 "Scheme-Agile Transactions": PROPOSED (EIP-8202), Draft.** Created 2026-03-22 by Giulio Rebuffo and Ben Adams. — [EIP-8202](https://eips.ethereum.org/EIPS/eip-8202)
  - Type `0x05` with `authorizations` `[role_id, scheme_id, witness]`.
  - Scheme 0x01 ("ephemeral secp256k1"): the address is `keccak256(0x01 || merkle_root)[12:]` over 2^20 one-time keys, and the key used is indexed by nonce. The witness is 737 B. There is a 5,000 gas surcharge plus about 43,008 gas extra calldata ((737−65)×64).
- **ecrecover PQ upgrade: RESEARCH.** Circle's Mira Belenkiy posted "Proposed PQ upgrade for ecrecover" on ethresear.ch: `ecrecover` would read PQ signatures carried in EIP-8141 frame txs, triggered by a sentinel value in the usual signature fields. — [Etherspot digest, 2026-09-02](https://etherspot.io/blog/eip-8141-advances-toward-hegota-inclusion-a-composable-take-on-native-aa-glamsterdam-repricing-tests-compatibility-a-post-quantum-path-for-ecrecover/)
  - Reviewers flagged a downgrade risk (the classical path stays live unless there is a fail-closed cutoff).
  - Others preferred a new opcode, and an EF researcher preferred deprecation via EIP-8151.

### Inferences
- PQ precompiles (J*) are not in Glamsterdam (Dec 2026) or Hegotá (2027). EIP-8355 is declined, and 8051/8052 are not even PFI. So the earliest L1 PQ precompile is the fork after Hegotá, i.e. 2028 or later, unless scope changes. In the meantime, PQ verification on L1 means pure-EVM contracts (about 94k gas for tuned SLH-DSA-Keccak up to millions for lattice schemes) or L2 precompiles.
- The EF line (vector math precompile plus hash-based leanSPHINCS plus STARK aggregation) suggests the long-term EL design avoids enshrining per-scheme lattice precompiles. ZKNox/Dubois-led drafts push Falcon and ML-DSA precompiles for nearer-term wallet use (e.g. hardware wallets that prefer NIST lattice schemes).
- Keccak/"ETH" variants (EIP-8051 `VERIFY_MLDSA_ETH`, EIP-8052 `KECCAKPRNG`) trade NIST conformance for EVM and ZK efficiency. A profile system would need to treat NIST-conformant and Ethereum-variant parameter sets as distinct algorithm identifiers.

### Gaps
- No EIP for SLH-DSA (FIPS 205) or leanSPHINCS *verification precompiles* was found. The leanSPHINCS formal spec and parameter set (signature size) were not retrieved.
- No EIP number was found for the J* "vector math precompile" (EVM-MAX/SIMD-style). It may be EIP-7885 (NTT) or an unnumbered strawmap item.
- The Magicians discussion and any benchmark revisions for EIP-8051/8052 after 2025-10 were not retrieved, and the status of EIP-7619 beyond Draft was not confirmed from the primary page.
- The ecrecover-upgrade proposal has no EIP number and its ethresear.ch post date was not retrieved.

## Q3. Address-to-key binding, key rotation and key-substitution protection

### Takeaway
An EOA address is the last 20 bytes of keccak256 of the secp256k1 public key. Never-used EOAs hide the key behind a hash, but every EOA that has sent a transaction has exposed its public key on-chain. Under 7702 the ECDSA key can never be retired. Smart and native-AA accounts decouple the address from the key and allow rotation. Concrete revocation designs exist: **EIP-8298 SETCODEFROM** plus EIP-3607 retire the ECDSA *transaction* authority, and **EIP-8151** retires the ECDSA *`ecrecover`* authority (permits). Both are CFI for Hegotá. Replay and substitution protection relies on chain_id, nonce, magic-byte domain separation, low-s or canonical encodings, and algorithm-ID-prefixed address derivation.

### Cited Findings
**Binding and public-key exposure**
- Once an account sends a transaction its public key is exposed on-chain. Accounts that have only received ETH expose just the address (a hash of the public key). — [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)
- pq.ethereum.org repeats that EOA keys are exposed after the first transaction. — [pq.ethereum.org](https://pq.ethereum.org/)

**Address derivation in new proposals**
- EIP-7932: `keccak(algorithm_id || public_key)[12:]`, with a compatibility path keeping legacy secp256k1 addresses. A zero byte is inserted for 63-byte keys to avoid collisions with legacy addresses. The EIP warns that "allowing more ways to derive addresses for one account may reduce that account's security". — [EIP-7932](https://eips.ethereum.org/EIPS/eip-7932)
- EIP-8202: `keccak256(0x01 || merkle_root)[12:]`. Hashing `scheme_id` into addresses separates address spaces across schemes, and security rests on Keccak-256 preimage resistance (about 128-bit under Grover). — [EIP-8202](https://eips.ethereum.org/EIPS/eip-8202)
- EIP-8141: accounts are "unlinked from their ECDSA keys, allowing native key rotation". Its Rationale floats a future 20-byte alias address pointing to a public key stored in state, which is not part of the EIP. — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)

**Rotation mechanisms**
- **EIP-8164:** rotation tuple `[chain_id, new_pubkey, nonce, authority, signature]`, signed by the currently installed ML-DSA key. It is applied atomically before execution and can be submitted by any party (sponsorship). There is no on-chain recovery: losing the ML-DSA key loses the account. — [EIP-8164](https://eips.ethereum.org/EIPS/eip-8164)
- **EIP-8130:** actor add/revoke batches; epoch and sequence counters invalidate outstanding signatures. — [EIP-8130](https://eips.ethereum.org/EIPS/eip-8130)
- **EIP-8202:** rotation by nonce within a 2^20-leaf tree. After about 1,048,576 txs the user must move to a new address. — [EIP-8202](https://eips.ethereum.org/EIPS/eip-8202)
- **Related research (secondary):** ethresear.ch "ephemeral ECDSA rotation" (#24273, 2026-03-04, updated 2026-05-20 to WOTS+C/FORS+C) and frame-transaction rotation (#24526, 2026-03-28). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)

**Revocation of the legacy ECDSA key**
- **EIP-8298 "SETCODEFROM": PROPOSED (EIP-8298), CFI for Hegotá.** Created 2026-06-11 by Liyi Guo, Ben Adams, Carlos Perez and Nicolas Consigny. — [EIP-8298](https://eips.ethereum.org/EIPS/eip-8298); [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
  - Opcode `0xA7` copies another account's code hash into the current account, replacing a 7702 delegation indicator with real code.
  - EIP-3607 then makes ECDSA-signed txs from the account invalid, and 7702 can no longer redelegate it. In the EIP's words, it "retires the account's key, not the account".
  - Gas: 12,200 cold / 9,300 warm when the code hash is updated.
  - Banned in EIP-8141 validation prefixes.
  - Caveat: `permit`-style `ecrecover` authorisations still accept the old key unless EIP-8151 also ships.
- **EIP-8151 "Account Code Restricted ecRecover": PROPOSED (EIP-8151), CFI for Hegotá.** Created 2026-02-09 by Liyi Guo and Nicolas Consigny. — [EIP-8151](https://eips.ethereum.org/EIPS/eip-8151); [EIP-8081](https://eips.ethereum.org/EIPS/eip-8081)
  - `ecRecover` returns the address only if the recovered account's code is empty or exactly a 7702 delegation indicator; otherwise it returns empty output, the same as a failed recovery.
  - Gas: 3,000 base, rising to 6,100 cold or 3,200 warm on success. Fixed-gas `staticcall(3000,…)` callers may break.
  - Risks: `ecrecover` becomes state-dependent (L2 fault proofs and off-chain verifiers diverge), and pure-Solidity ECDSA verifiers bypass it.
- The survey grades the "revocation pair" EIP-8298 + EIP-8151 as A-tier in the EF Hegotá review, with EIP-8164 as the alternative tx-side route (secondary). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)

**Replay, malleability and substitution protection**
- **EIP-7702:**
  - MAGIC `0x05` domain separation.
  - chain_id `0` makes an authorization valid on every chain ("When universal deployment is preferred, simply set chain ID to 0").
  - `s ≤ secp256k1n/2` (EIP-2).
  - Wallets should not expose a generic "sign authorization" interface ("There is no safe way to provide this interface").
  - A poorly implemented delegate "can allow a malicious actor to take near complete control" of the EOA.
  - Initialization can be front-run (sign initial calldata).
  - Storage collisions on redelegation (use ERC-7201 layouts).
  - `tx.origin` semantics change.
  - Source: [EIP-7702](https://eips.ethereum.org/EIPS/eip-7702)
- **EIP-8164:**
  - Magic `0x07` for authorizations and the `0x06` prefix for tx payloads. ECDSA-mode payloads omit `sender`, so they are distinguishable from ML-DSA mode.
  - The public key is inside the signed tuple, so front-running cannot substitute it.
  - Rotation messages include `authority` against cross-account replay. `chain_id=0` permits cross-chain replay.
  - ML-DSA-44 requires canonical encodings, norm-bound checks and rejection of duplicate or out-of-order hint indices. "All clients must agree on accept/reject decisions … or the chain could split."
  - Source: [EIP-8164](https://eips.ethereum.org/EIPS/eip-8164)
- **EIP-8141:**
  - The canonical signature hash elides the signature bytes of empty-`msg` entries.
  - An explicit `msg` must be 32 bytes and not all zeros.
  - secp256k1 low-s is enforced. `chain_id` and `nonce` are in the payload.
  - Source: [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- **EIP-7932:** each registered algorithm spec must include a security analysis and address malleability. — [EIP-7932](https://eips.ethereum.org/EIPS/eip-7932)

### Inferences
- A complete "PQ migration" of a legacy EOA needs four things: (a) a PQ verifier (8141 `VERIFY` code or 8164 native key); (b) retirement of ECDSA tx authority (8298 + 3607, or 8164's permanent switch); (c) retirement of ECDSA off-chain-signature authority (8151); (d) a quantum-safe binding of new keys signed *before* Q-day. Without (c), EIP-2612 permits and other `ecrecover`-based approvals remain a quantum backdoor.
- EIP-712 typed-data domain separators (chainId, verifyingContract) and ERC-1271 contract-signature validation are the app-layer counterparts. Neither was re-fetched for these notes. No PQ-specific amendment to EIP-712 or ERC-1271 was found; such signatures would inherit agility from the account's ERC-1271 implementation.
- Chain-agnostic authorizations (`chain_id = 0` in 7702 and 8164) are attractive for multi-chain smart-account rollout but enlarge the replay surface. A profile system should treat chain-binding as an explicit, recorded choice.

### Gaps
- No primary source was found on how EIP-8141 accounts protect against *scheme downgrade* when several schemes are accepted, beyond reviewer comments on the ecrecover proposal and EIP-8202's downgrade note.
- Whether EIP-8298 and EIP-8151 move from CFI to SFI in Hegotá was not determined (scoping runs to about 2026-11-03).
- The EIP-712 and ERC-1271 texts were not fetched in this session.

## Q4. Quantum-emergency plans, EF PQ team and timelines

### Takeaway
The canonical emergency plan is still Vitalik's 2024-03-09 ethresear.ch post. It would roll back the chain to before the theft, disable EOA transactions, require smart-wallet transactions, and let users recover via a STARK proof of knowledge of the hash preimage or BIP-32 seed behind their key. It is **RESEARCH** with no EIP. Since January 2026 the EF has had a dedicated Post-Quantum team (lead Thomas Coratger) and pq.ethereum.org. An EF Protocol-cluster document sets **December 2029** for full PQ security across EL, CL and DA, "non-negotiable" until a January 2027 review. A reduced-guarantee "MV-PQ" milestone using leanSPHINCS transactions to survive Q-day reportedly appeared on the roadmap on 2026-08-19 (secondary).

### Cited Findings
**Vitalik's 2024 emergency plan: RESEARCH**
- Posted by vbuterin on 2024-03-09 as "How to hard-fork to save most users' funds in a quantum emergency". — [ethresear.ch](https://ethresear.ch/t/how-to-hard-fork-to-save-most-users-funds-in-a-quantum-emergency/18901)
  - Steps: (1) revert all blocks after the first block where large-scale theft is clearly underway; (2) disable traditional EOA transactions; (3) add a smart-contract-wallet tx type (possibly RIP-7560) if not already present; (4) add a tx type or opcode accepting a STARK proof of knowledge of a private preimage `x`, a hash-function ID from an approved list, and that the derived address matches. If valid, the account's code switches to new validation code.
  - Proofs could be batched as a STARK-of-STARKs. The infrastructure "could in principle start to be built tomorrow".
  - Never-used accounts are already safe. Keys derived via hash chains (BIP-32, brainwallets) are recoverable, which is "most users' private keys". Keys from raw randomness with exposed public keys are not covered.
  - Thread objections: corepool says a fork cannot distinguish owner from attacker; domothy says attacks may go unnoticed, so deploy now with a kill switch; others question the qubit-scale estimates.
- **Follow-on research (secondary):** Kiraz & Kardas, ePrint 2026/352 (rev. 2026-09-01), "hidden-key proofs" with an SP1 prototype. — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)

**EF organisation and roadmap: DECIDED (organisational) / RESEARCH (technical)**
- The EF formed a Post-Quantum Security team in January 2026, led by Thomas Coratger, and tracks it at pq.ethereum.org.
  - [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)
  - [CoinDesk, 2026-01-24](https://www.coindesk.com/tech/2026/01/24/ethereum-foundation-makes-post-quantum-security-a-top-priority-as-new-team-forms)
  - [The Quantum Insider, 2026-01-26](https://thequantuminsider.com/2026/01/26/ethereum-foundation-elevates-post-quantum-security-to-top-strategic-priority/)
- pq.ethereum.org (updated 2026-06-27): the team sits in the EF Protocol cluster.
  - The EL work streams include "emergency preparedness", with no details.
  - L1 protocol upgrades "could be completed by 2029", and full EL migration takes additional years.
  - Most roadmaps place a CRQC in the early-to-mid 2030s. — [pq.ethereum.org](https://pq.ethereum.org/)
- The Protocol Snarkification team (Alex Hicks) runs a $20M, three-year zkEVM Formal Verification Project covering SNARK primitives used in PQ signature aggregation. — [pq.ethereum.org](https://pq.ethereum.org/)
- **ethereum.org milestone list (as of the page update 2026-09-08):** I* (PQ validator key registry), J* (PQ signature verification precompiles), L* (PQ attestations, leanVM), M* (full PQ signature aggregation, PQ blobs). Core PQ infrastructure is targeted for about 2029.
  - More than 10 client teams were in weekly interop devnets as of April 2026, there is a $1M Poseidon Prize, and the 2nd PQ Research Retreat was held in Cambridge, UK, 9–12 Oct 2026.
  - Users need take no action now. Wallets will guide migration "after Hegotá and EIP-8141".
  - Source: [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)
- **December 2029 target:** an EF Protocol-cluster priorities document (reported by Cointelegraph, relayed by Cryptonomist on 2026-09-08) sets "full quantum resistance" across EL, CL and DA by December 2029. It is "non-negotiable" until a January 2027 reassessment with outside specialists.
  - Reaching it from Glamsterdam needs about one fork every 7.2 months.
  - The document reviewed 62 EIPs: 15 A-tier, 8 candidates, 7 below cutoff and 28 declined.
  - Source: [Cryptonomist](https://en.cryptonomist.ch/2026/09/08/ethereum-hegota-upgrade/) (secondary; primary document not linked)
- **MV-PQ milestone (secondary):** reportedly added to the Foundation roadmap on 2026-08-19 as part of J*. It would use leanSPHINCS transactions and is "designed to keep Ethereum operating through Q-day with reduced guarantees". The survey assumes Q-day is possible from 2030. — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- **Dormant funds:** the EF estimates about 0.1% of ETH sits in quantum-vulnerable dormant accounts, versus about 5% for Bitcoin, which makes "do nothing" more viable. Freezing is "an open governance question with no community consensus". — [pq.ethereum.org](https://pq.ethereum.org/); [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)
- **External drivers** (as cited by ethereum.org):
  - Google Quantum AI (Mar 2026) estimated that breaking 256-bit ECC could need about 1,200 logical qubits, and set a 2029 internal migration deadline.
  - NIST anticipates deprecating ECDSA by 2030 and disallowing it by 2035.
  - Source: [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)

### Inferences
- The 2024 plan's preconditions are now partly on the roadmap: native AA (EIP-8141, SFI) gives step 3, and EIP-8298/8151 give a cleaner ECDSA kill-switch than "disable EOA txs". Step 4 (a STARK preimage-recovery tx type) still has no EIP. So an emergency today would require building that piece under pressure.
- BIP-32-derived wallets are recoverable by preimage proof. Keys generated from raw randomness, and any key whose seed is not hash-derived, are not. Profiles should record key-derivation provenance (seed lineage) because it determines emergency recoverability.

### Gaps
- No EIP or spec for the STARK "proof of seed/preimage" recovery transaction was found.
- The MV-PQ milestone wording comes only from a secondary survey. strawmap.org was not fetched to confirm its date and content.
- The EF Protocol-cluster priorities document behind the December 2029 / January 2027 claims was not located as a primary source.
- No 2025–2026 Vitalik post was found that updates the *emergency-fork* procedure itself (as opposed to the migration roadmap).

## Q5. Bandwidth and storage cost of PQ signatures, and mitigations

### Takeaway
PQ signatures are 10x to 100x+ larger than ECDSA's 65 B: Falcon-512 is 666 B, ML-DSA-44 2,420 B, hash-based leanXMSS/leanSPHINCS about 2–5 kB, and SLH-DSA-128s 7,856 B. At current calldata floor pricing (EIP-7976, in review: 64 gas/byte) that bandwidth costs tens of thousands to hundreds of thousands of gas per tx before verification. The main mitigation is **recursive-STARK aggregation in the mempool** (Vitalik, 2026-01-12), specified as **EIP-8288** (Buterin and Coratger, 2026-06-03, Draft). Individual signatures are stripped and replaced by one block-level recursive STARK, at a constant overhead of about 128–512 kB per proof per tick.

### Cited Findings
**Signature and key sizes**
- Falcon-512: signature 666 B, public key 896 B. — [EIP-8052](https://eips.ethereum.org/EIPS/eip-8052)
- ML-DSA-44: signature 2,420 B, public key 1,312 B (expanded 20,512 B in EIP-8051). — [EIP-8051](https://eips.ethereum.org/EIPS/eip-8051)
- leanXMSS: about 3,000 B, versus about 96 B for BLS. — [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)
- "Hash-based signatures are about 2–3 kB each". — [EIP-8288](https://eips.ethereum.org/EIPS/eip-8288)
- Secondary sizes:
  - SLH-DSA-128s 7,856 B and 128f 17,088 B.
  - SPHINCS− (EVM-tuned) 3,704–3,856 B.
  - leanSPHINCS example 4,924 B.
  - WOTS+C 468 B and FORS+C 2,448 B.
  - Source: [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)

**Calldata pricing**
- EIP-8141 prices calldata at 4 gas/token with a 16 gas/token floor (EIP-7976). — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)
- EIP-8202 computes its 737 B witness as an extra (737−65)×64 = 43,008 gas. — [EIP-8202](https://eips.ethereum.org/EIPS/eip-8202)
- EIP-7976 ("Increase Calldata Floor Cost", In Review, 2025-06-18) sets 64 gas/byte for zero and non-zero bytes. EIP-7623 (Pectra, Final) set 10/40. The survey notes PQ txs are about 1.6x costlier than a Dec 2025 SoK assumed at 40 gas/byte (secondary). — [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)

**Recursive-STARK mempool: RESEARCH**
- Vitalik, ethresear.ch, 2026-01-12: PQ rules out elliptic-curve SNARKs, and "STARKs are expensive, taking up 128 kB even in highly size-optimized implementations". — [ethresear.ch](https://ethresear.ch/t/recursive-stark-based-bandwidth-efficient-mempool/23838)
  - Each node, every tick (e.g. 500 ms), produces a recursive STARK over all still-valid objects and forwards objects *stripped of signatures*.
  - Overhead is constant at about 2 MB/s with 8 peers and 500 ms ticks, independent of tx count.
  - Use cases: EL PQ signature aggregation (privacy protocols), CL aggregation and blob roots.
  - No proving-time benchmarks are given.

**EIP-8288 "In-mempool signature and proof aggregation": PROPOSED (EIP-8288), Draft**
- Authors Vitalik Buterin and Thomas Coratger. Created 2026-06-03. Requires EIP-8141. — [EIP-8288](https://eips.ethereum.org/EIPS/eip-8288)
  - Adds frame mode `DEP_VERIFY_FRAME_MODE = 4`. Each dependency triple `(scheme, data_hash, vk_hash)` takes 96 B.
  - Schemes: leanSPHINCS `0x10` and leanSTARK `0x11` (only leanSPHINCS currently valid).
  - Block validity requires a `recursive_stark` header field checked against a fixed `AGGREGATED_VK`.
  - Mempool wrappers carry either individual proofs (mode 0) or a recursive STARK (mode 1), with `AGGREGATION_INTERVAL = 1000 ms`. FOCIL is extended with a `recursive_stark` field.
  - STARKs are over 128 kB, up to 512 kB for fast client-side proving. The fixed builder overhead is about 256 kB per interval.
  - Gas: 3,000 per leanSPHINCS triple and 30,000 per leanSTARK triple, plus `recursive_stark_gas` of 30,000 per triple, compared with 150k–200k for on-chain verification.
  - Limits: 256 dependencies per frame, and per tx at most 16 leanSPHINCS and 1 leanSTARK.
  - Hash: BLAKE2s in current tooling, with BLAKE3 the leading replacement. Proofs use the KoalaBear field with a 2^24 trace limit.
- One outlet reports EIP-8288 was merged into the EIPs repo on 2026-09-09. Unconfirmed; the EIP page shows only the 2026-06-03 creation date. — [Crypto Briefing](https://cryptobriefing.com/eip-8288-recursive-stark-mempools-ethereum/)

**leanVM benchmarks (secondary, from the README; "not (yet) production ready")**
- On an M4 Max, 245 SPHINCS signatures gave a 320.1 KiB proof in about 1 s at 12.5 GiB peak memory.
- 900 XMSS signatures gave a 294.9 KiB proof, and a recursive 2-to-1 merge takes about 0.29 s.
- The survey estimates about 39 s for one machine to prove a full 200M-gas block of transfers.
- Source: [AppliedPQC #35](https://github.com/AppliedPQC/pqc-research/issues/35)
- ethereum.org says leanVM aggregation compresses signature data about 250x. — [ethereum.org](https://ethereum.org/roadmap/security/quantum-resistance/)

**Other approaches: RESEARCH**
- Folding-based PQ aggregation: Coratger & Setty. — [ethresear.ch folding](https://ethresear.ch/t/post-quantum-signature-aggregation-a-folding-approach/23639)
- Falcon aggregation for PQ mempools, cited in an arXiv paper. — [arXiv 2609.24689](https://arxiv.org/pdf/2609.24689)
- EIP-8141 deliberately hides protocol-validated signature bytes from the EVM to keep future aggregation possible. — [EIP-8141](https://eips.ethereum.org/EIPS/eip-8141)

### Inferences
- At 64 gas/byte the calldata alone, before verification, costs about:

  | Signature | Calldata gas |
  |---|---|
  | Falcon-512 (666 B) | ~42.6k |
  | ML-DSA-44 (2,420 B) | ~155k |
  | 3 kB hash-based | ~192k |
  | SLH-DSA-128s | ~503k |

  These are my arithmetic, assuming the EIP-7976 floor applies. This is why EIP-8288 aims to strip signatures from the chain entirely, and why compact Falcon remains attractive to precompile advocates despite FIPS 206 not being final.
- EIP-8288 makes the EL signature scheme effectively *protocol-chosen* (leanSPHINCS) for aggregated txs, while EIP-8141 `ARBITRARY` keeps *account-chosen* schemes for non-aggregated txs. This yields a two-tier model: cheap aggregated hash-based signatures versus expensive bespoke per-account schemes.

### Gaps
- No primary measurement of PQ-tx mempool propagation latency or of the end-to-end bandwidth impact on today's devp2p was found.
- leanSPHINCS's final parameterisation and signature size were not obtained from a primary spec.
- Whether EIP-8288 is on any fork's PFI/CFI list is unknown. It does not appear in the EIP-8081 Hegotá meta lists retrieved.
