# AGENTS.md: CanorInfinitum: Project C (Hardware, Software & Technology)

Inherits **all** non-negotiable rules from the [root AGENTS.md](../../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules

- **Role:** Project C encompasses hardware integration, foundational software libraries, and embedded technology integration for CanorInstinctivus. It provides arbitrary-precision integer arithmetic, factoradic/Lehmer indexing, prime factor algorithms, surreal numbers, Golay error-correction coding, and Zephyr RTOS bare-metal drivers.
- **Local Rules:** Exactness is absolute. Integer programming with closed interval arithmetic. Arithmetic overflow must return `CANOR_ERR_RANGE` rather than wrapping silently or truncating.
- **Naming Standards:** Strictly `canor_*`. Legacy prefixes (`fc_*`, `vi_*`) are forbidden in all new and migrated code.
- **Arithmetic:** Pure integer and interval arithmetic. Floating-point types (`float`, `double`) are prohibited.
- **Status Codes:** Return unified `canor_status` with explicit diagnostics for undefined cases.

## Relative Links

- [Root AGENTS.md](../../AGENTS.md)
- [Root README.md](../../README.md)
- [Root SCHEMA.md](../../SCHEMA.md)
- [Local Context](CONTEXT.md)
