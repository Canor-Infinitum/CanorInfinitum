# Copilot Instructions

> Mirrored to `.github/copilot-instructions.md`, which GitHub Copilot reads automatically. Always edit this file directly, then re-sync to the `.github/` mirror (see [WORKFLOW.md](WORKFLOW.md)).

This repository contains a pure C11, integer-only implementation of the Canor™ architecture. All modules strictly adhere to the canonical `canor_*` naming convention.

## Mandatory Implementation Guidelines

- **Exact Arithmetic:** Use fixed-width integer types (`int32_t`, `int64_t`, `uint64_t`), exact integer values, and closed `[LO, HI]` integer intervals. Express ratios and fractional scores as fixed-point integers over `CANOR_ONE` (`1000`).
- **Unified Diagnostics:** Return unified status codes (`canor_status`). Check for NULL pointers and return `CANOR_ERR_ARGS`. Emit explicit diagnostics for undefined or unhandled cases rather than silent fallbacks.
- **Capacity Bounds:** Verify container boundaries before inserting elements: `if (n >= CANOR_X_MAX) return CANOR_ERR_FULL;`.
- **Standardized Naming:** Name public functions as `canor_<module>_<verb>()`, types as `canor_<name>_t`, and preprocessor constants as `CANOR_*`.
- **Automated Testing:** Provide automated unit tests for every newly introduced public function.

## Prohibited Patterns

- Do not use floating-point types (`float`, `double`), the standard math library (`<math.h>`), or dynamic memory allocation (`malloc`, `free`) in the core library.
- Do not introduce identifiers with non-standard or non-canor prefixes into the C codebase.
- Do not execute database mutations or side-effects during assembly or compilation.
- Do not reorder spec-pinned constants (such as FlowValue bit encodings) or hardcode Cayley–Dickson sedenion tables.
- Do not propose modifications to historical reference files under `.old/`.

## Context References

- Architectural Glossary: [CONTEXT.md](CONTEXT.md)
- Type Specifications & Encodings: [SCHEMA.md](SCHEMA.md)
- Complete Engineering Invariants: [AGENTS.md](AGENTS.md)
