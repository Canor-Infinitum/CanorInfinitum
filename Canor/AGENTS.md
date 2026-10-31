# AGENTS.md: Canor: Projects Umbrella

Inherits **all** non-negotiable rules from the [root AGENTS.md](../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules

- **Role:** Umbrella for projects: CanorLingua (Project A), CanorRepositorium (Project B), and CanorInfinitum (Project C). It houses the core type hierarchy and shared definitions.
- **Local Rules:** Owns canonical C ABI boundaries. Subprojects interact exclusively through well-defined `canor_*` contracts. Floating-point math and dynamic memory allocation (`malloc`, `free`) are prohibited.
- **Naming Standards:** Strictly `canor_*`. Legacy prefixes (`fc_*`, `vi_*`) are forbidden in all new and migrated code.
- **Arithmetic:** Pure integer and interval arithmetic. Floating-point types (`float`, `double`) are prohibited.
- **Status Codes:** Return unified `canor_status` with explicit diagnostics for undefined cases.

## Relative Links

- [Root AGENTS.md](../AGENTS.md)
- [Root README.md](../README.md)
- [Root SCHEMA.md](../SCHEMA.md)
- [Local Context](CONTEXT.md)
