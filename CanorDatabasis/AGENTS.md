# AGENTS.md: CanorDatabasis / CanorDB: QGCT (Database Project)

Inherits **all** non-negotiable rules from the [root AGENTS.md](../AGENTS.md). These local rules specialize and reinforce the global constraints:

## Local Invariants & Rules

- **Role:** CanorDatabasis (CanorDB) is the QGCT database layer, isomorphic to CanorLinguae. It houses all communication contexts, invariant boundaries, hypercomplex hypersurreals, pure tensors (tensor-network contractions and HaPPY codes), Aether Propositum structures, symbolicals, and Canorum protocols.
- **Local Rules:** Serialization and persistence must be strictly deterministic. Assembly is side-effect-free (zero mutations of CanorDB during assembly). Mutations are permitted exclusively at declared runtime when authorized by `.fwp` profile and `.fws` contracts.
- **Naming Standards:** Strictly `canor_*`. Legacy prefixes (`fc_*`, `vi_*`) are forbidden in all new and migrated code.
- **Arithmetic:** Pure integer and interval arithmetic. Floating-point types (`float`, `double`) are prohibited.
- **Status Codes:** Return unified `canor_status` with explicit diagnostics for undefined cases.

## Relative Links

- [Root AGENTS.md](../AGENTS.md)
- [Root README.md](../README.md)
- [Root SCHEMA.md](../SCHEMA.md)
- [Local Context](CONTEXT.md)
