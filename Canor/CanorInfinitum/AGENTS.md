# AGENTS.md: CanorInfinitum: Project C (Hardware, Software & Technology)

Inherits **all** non-negotiable rules from [root AGENTS.md](../../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules
- **Role**: Project C: All of CanorInstinctivus-Hardware, -Software, and -Technology. Exact unbounded arithmetic, big integers, factoradic/Lehmer, primes, surreals, Golay coding, and Zephyr / bare-metal integration.
- **Local Rules**: Exactness is absolute. Integer programming with interval arithmetic. Arithmetic overflow must return CANOR_ERR_RANGE, never wrap silently or truncate.
- **Naming**: Strictly \canor_*\. Legacy prefixes (\fc_*\, \vi_*\) are forbidden in all new/migrated code.
- **Arithmetic**: Pure integer / interval arithmetic. Zero floats or doubles.
- **Status Codes**: Return unified \canor_status\ with explicit diagnostics for undefined cases.

## Relative Links
- [Root AGENTS.md](../../AGENTS.md)
- [Root README.md](../../README.md)
- [Root SCHEMA.md](../../SCHEMA.md)
- [Local Context](CONTEXT.md)
