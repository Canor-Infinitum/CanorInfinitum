# AGENTS.md: Canor: Projects Umbrella

Inherits **all** non-negotiable rules from [root AGENTS.md](../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules
- **Role**: Umbrella for projects: CanorLingua (A), CanorRepositorium (B), and CanorInfinitum (C). Hosts core type tower and shared definitions.
- **Local Rules**: Owns the canonical C ABI boundaries. Children must interact through well-defined canor_* contracts. No float, no hidden malloc.
- **Naming**: Strictly \canor_*\. Legacy prefixes (\c_*\, \i_*\) are forbidden in all new/migrated code.
- **Arithmetic**: Pure integer / interval arithmetic. Zero floats or doubles.
- **Status Codes**: Return unified \canor_status\ with explicit diagnostics for undefined cases.

## Relative Links
- [Root AGENTS.md](../AGENTS.md)
- [Root README.md](../README.md)
- [Root SCHEMA.md](../SCHEMA.md)
- [Local Context](CONTEXT.md)
