# AGENTS.md: CanorLinguae: QGTC (Language Suite)

Inherits **all** non-negotiable rules from the [root AGENTS.md](../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules

- **Role:** CanorLinguae (QGTC) is a reactive, multi-metaprogramming language suite combining quantum-symbolic and physical models. It covers the ten languages of [docs/Canor.md](../docs/Canor.md): Strategy, Flow, MetaFlow, Action, Soul, Maneuver, United, Elite, Follower / Carmen Interlingua, and Aether IR.
- **Local Rules:** Strict compliance with `atoms.ebnf` and the Canor.md sections §1–§10. Pipe-dot framing (`| ... .`) for embedded complementary code blocks. Rate-limited ops/sec timing with $3 \times 3 \times 3$ latency and jitter fields.
- **Naming Standards:** Strictly `canor_*`. Legacy prefixes (`fc_*`, `vi_*`) are forbidden in all new and migrated code.
- **Arithmetic:** Pure integer and interval arithmetic. Floating-point types (`float`, `double`) are prohibited.
- **Status Codes:** Return unified `canor_status` with explicit diagnostics for undefined cases.

## Relative Links

- [Root AGENTS.md](../AGENTS.md)
- [Root README.md](../README.md)
- [Root SCHEMA.md](../SCHEMA.md)
- [Local Context](CONTEXT.md)
