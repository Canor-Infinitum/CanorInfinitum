# AGENTS.md: CanorRepositorium: Project B (Storage, Learning & Runtime)

Inherits **all** non-negotiable rules from the [root AGENTS.md](../../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules

- **Role:** Project B implements the storage, machine learning, and runtime execution infrastructure for CanorInstinctivus. This includes FlowStore (mapping cryptographic hash identifiers to RGB content strings), lexicographical word/flow indexing, and hypervector dictionary storage.
- **Local Rules:** Every insertion must verify capacity bounds and return `CANOR_ERR_FULL` if capacity is exhausted. Lookups must return `CANOR_ERR_NOT_FOUND` on missing keys rather than null pointer dereferences.
- **Naming Standards:** Strictly `canor_*`. Legacy prefixes (`fc_*`, `vi_*`) are forbidden in all new and migrated code.
- **Arithmetic:** Pure integer and interval arithmetic. Floating-point types (`float`, `double`) are prohibited.
- **Status Codes:** Return unified `canor_status` with explicit diagnostics for undefined cases.

## Relative Links

- [Root AGENTS.md](../../AGENTS.md)
- [Root README.md](../../README.md)
- [Root SCHEMA.md](../../SCHEMA.md)
- [Local Context](CONTEXT.md)
