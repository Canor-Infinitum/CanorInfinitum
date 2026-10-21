# TRUST.md: Opt-in PQC Profile for the Elite Accord

> **Scope: one opt-in profile, nothing more.** This document describes a single, optional post-quantum (PQC) profile for the `canor_elite` accord. Working id: `pqc-slh-dsa`, a placeholder that can be renamed.
>
> - Only a project whose `.fwp` opts into it uses it.
> - It is **not** a project default.
> - It does **not** change `canor_elite`'s core behavior or any other project.
> - The core stays algorithm-neutral: it calls whatever digest and verify functions the active profile supplies. With none set, the accord stays `undefined`, never accepted by default.
>
> **History.** An earlier version of this file (2026-10-20) was written as an accord-wide trust model and folded into the core without the scope being asked. On 2026-10-21 the user rescoped it to this profile and asked for every core change to be reverted. See §6.
>
> **Authority.** The repository's `.md` files:
> - [SCHEMA.md](../../SCHEMA.md): `.fwp` = project / profile; the United digest algorithm is profile-defined.
> - [AGENTS.md](../../AGENTS.md): invariants 1, 4, 6 and 8.
> - The `united` and `elite` skills: `@digest`; "Don't invent profile-defined values".
>
> The external references at the end are only for the standing of the standards.

## 1. What the profile sets

| Profile value | Choice | Reported id |
|---|---|---|
| Digest | **SHA3-512** (FIPS 202), 64 bytes | `sha3-512` |
| Signatures | **SLH-DSA-SHAKE-256s** only (FIPS 205) | `slh-dsa-shake-256s` |
| Key exchange (confidentiality only) | **Hybrid** `mceliece8192128` + **ML-KEM-1024** (FIPS 203), SHA3-512 combiner. A runtime effect under the `.fws` contracts, never in the core | `mceliece8192128+ml-kem-1024` |
| Trust roots | **Per-party roots**, declared in each party's own `.fwp` (§3) | — |
| Time (inside the profile's verify only) | int64 Unix seconds (UTC), supplied by the caller | — |
| Out of scope | RCC (Riemann-Collatz Cipher) and SYM variational meta-schemas | — |

All of these are integer and bitwise algorithms, consistent with AGENTS.md invariant 1. SHA3-512 and SLH-DSA-SHAKE share the Keccak-f[1600] permutation, so one Keccak core serves both.

**Opting in.** A project's `.fwp` declares `@digest(algorithm=sha3-512, covers={grammar, settings});` using the existing syntax. The `.fwp` vocabulary has **no** declarations yet for a signature algorithm, key exchange or trust roots. How to declare them is an open item (§5), and this profile doesn't invent that syntax.

## 2. How it fits the existing core, unchanged

The profile sets the `canor_elite_profile_t` fields the core already has. It needs no change to the core.

| Core field | This profile |
|---|---|
| `digest_algorithm`, `digest` | `"sha3-512"`, a SHA3-512 function |
| `signature_algorithm`, `verify` | `"slh-dsa-shake-256s"`, the profile's verifier |
| `crypto_ctx` | the profile's store: detached signatures, endorsements, revocation lists, evaluation time |

**The 64-byte blob** (`CANOR_ELITE_BLOB_MAX`, unchanged):
- A signatory's `key` is the party's SLH-DSA-SHAKE-256s public key, which is exactly 64 B.
- A signature blob holds the **SHA3-512 reference** of the detached 29,792-byte signature. The profile's `verify` looks the bytes up in its store:
  - bytes missing → `UNDEFINED`;
  - reference mismatch → `INVALID`;
  - otherwise, the SLH-DSA verdict.
- The core only passes the blob through.

**Storage cost (core unchanged, one signature per statement):**

| Accord size | Signatures | Detached bytes |
|---|---|---|
| 2 parties × 4 categories | 12 | ≈ 357 KB |
| 4 × 4 | 56 | ≈ 1.67 MB |
| 8 × 8 (the caps) | 464 | ≈ 13.8 MB |

That cost is the price of choosing this profile. It lands on the profile's store, not on the accord struct.

## 3. Trust roots and revocation (inside the profile)

- **Roots in the profile.** Each party's `.fwp` declares its own root public key. The core's signed roster already binds each signatory's `profile` digest (its `.fwp` + `.fws`). So the profile's `verify` checks that `who->key` matches the key declared in the profile behind `who->profile`. A swapped key then fails verification, with no core change.
- **Working keys (optional).** A root may endorse a working key with a validity window. The endorsement is a profile artifact with its own domain tag (e.g. `canor.pqc.endorse.v1`), not a `canor_elite` statement.
- **Revocation lists.** These are root-signed profile artifacts: a sequence number, then entries of (key, effective time, `superseded` | `compromised`). A `compromised` entry voids every signature by that key; a `superseded` entry voids only signatures dated at or after its effective time.
- **Results.** Revoked keys, keys outside their window, and unknown keys all return `CANOR_ELITE_SIG_UNDEFINED`. That is the core's existing "unknown key, revoked root" outcome, so the accord is `undefined` until the party re-signs.
- **Reproducibility (invariant 8).** The caller supplies the evaluation time and the revocation lists through `crypto_ctx`. The profile records them with each evaluation.

## 4. Key exchange: hybrid McEliece + ML-KEM-1024

- **Role.** Confidentiality and key transport for meta-schema exchanges, at runtime and only when the `.fws` contracts authorize it (AGENTS.md invariant 4). McEliece never signs.
- **Combiner.** `K = SHA3-512("canor.pqc.kem.v1" || ss_mceliece || ss_mlkem || ct_mceliece || ct_mlkem || SHA3-512(pk_mceliece) || SHA3-512(pk_mlkem))`.
- **Size.** An `mceliece8192128` public key is 1,357,824 B. It never enters the accord.
- **Known risk, accepted for this profile.** BSI advised on 1 Oct 2026 against Classic McEliece for new developments, while still accepting hybrid use. ISO standardized it in June 2026; NIST chose HQC instead. Revisit after BSI's early-2027 revision and NIST's FIPS 207.

## 5. Open items (this profile only)

- [ ] `.fwp` syntax for declaring the signature algorithm, the key exchange and a party's root key (only `@digest` exists today).
- [ ] Where the detached signatures, endorsements and revocation lists are stored and retained (e.g. United referenced artifacts in a `.fwo`).
- [ ] How fresh a revocation list must be, and what happens when none is supplied (proposed: `UNDEFINED`).
- [ ] McEliece variant (plain, `f`, `pc`, `pcf`) and the transport protocol.
- [ ] SLH-DSA mode. Proposed: pure SLH-DSA with an empty context string and hedged signing.

## 6. Not part of this profile (reverted from the core, 2026-10-21)

The 2026-10-20 version proposed core protocol changes, and the parent session implemented them. On 2026-10-21 the user rescoped this work to one profile and chose to **revert all of them**. They are listed only so they aren't re-added without explicit approval:

- v2 statements (`ratify.v2`, `certs.v2`, `approve.v2`, `basis.v2`) and the core statements `withdraw.v1`, `endorse.v1`, `revoke.v1`;
- one bundled certify signature per certifier;
- signatory keys bound into the core roster;
- statement times, evaluation time and `accepted_until`;
- consent withdrawal, consent expiry and `consent_ttl`;
- `CANOR_ELITE_SIG_REVOKED`, and the REVOKED / EXPIRED / WITHDRAWN flags;
- PQC algorithm ids and sizes in `canor_elite.h`;
- the "trust model adopted" entries in MEMORY.md.

## External references (standards status, checked 2026-10-20)

- NIST: "NIST Advances 9 Candidates to the 3rd Round" (14 May 2026): https://csrc.nist.gov/News/2026/nist-advances-9-candidates-to-the-3rd-round-of-pqc
- NIST IR 8610: https://nvlpubs.nist.gov/nistpubs/ir/2026/NIST.IR.8610.pdf
- BSI TR-02102-1 (Classic McEliece note): https://www.bsi.bund.de/SharedDocs/Downloads/EN/BSI/Publications/TechGuidelines/TG02102/BSI-TR-02102-1.html
- heise, "BSI Concerned About McEliece": https://www.heise.de/en/news/Post-Quantum-Crypto-BSI-Concerned-About-McEliece-11478462.html
- ISO/IEC 18033-2 Amd 2:2026 coverage: https://thequantuminsider.com/2026/07/15/classic-mceliece-iso-standard-post-quantum-cryptography/
- FIPS 202, 203, 205: https://csrc.nist.gov/publications/fips
