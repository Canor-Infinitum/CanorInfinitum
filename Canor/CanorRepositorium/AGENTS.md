# AGENTS.md: CanorRepositorium: Project B (Storage, Learning & Runtime)

Inherits **all** non-negotiable rules from [root AGENTS.md](../../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules
- **Role**: Project B: All of CanorInstinctivus-Storage, -Learning, and -Runtime. FlowStore (hash id -> RGB content string), word/flow indexing, and hypervector dictionary storage.
- **Local Rules**: Every insert must verify capacity and return CANOR_ERR_FULL if exhausted. Lookups return CANOR_ERR_NOT_FOUND, never null dereference.
- **Naming**: Strictly \canor_*\. Legacy prefixes (\c_*\, \i_*\) are forbidden in all new/migrated code.
- **Arithmetic**: Pure integer / interval arithmetic. Zero floats or doubles.
- **Status Codes**: Return unified \canor_status\ with explicit diagnostics for undefined cases.

## Relative Links
- [Root AGENTS.md](../../AGENTS.md)
- [Root README.md](../../README.md)
- [Root SCHEMA.md](../../SCHEMA.md)
- [Local Context](CONTEXT.md)
