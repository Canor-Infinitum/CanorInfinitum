# Copilot Instructions

> Mirrored to `.github/copilot-instructions.md`, which Copilot reads automatically. Edit this file, then re-sync (see WORKFLOW.md).

This repository is pure C11 and integer-only, implementing the Canor™ suite. All modules use the `canor_*` prefix.

## Always
- Use `int32_t`/`int64_t`/`uint64_t`, exact integers, and closed `LO/HI` integer intervals. Express ratios as fixed-point over `CANOR_ONE` (1000).
- Return `canor_status`. Return `CANOR_ERR_ARGS` on NULL. Return explicit diagnostics for undefined cases.
- Check capacity before writing: `if (n >= CANOR_X_MAX) return CANOR_ERR_FULL;`
- Name things `canor_<module>_<verb>()`, `canor_<name>_t`, and `CANOR_*`.
- Add a test case for each new public function.

## Never
- `float`, `double`, `<math.h>`, `malloc`/`free` in core code.
- Non-standard or non-canor prefixes in new code.
- Mutating CanorDB during assembly.
- Reordering FlowValue (`NegZero=0, NegOne=1, PosZero=2, PosOne=3`) or hard-coding the sedenion table.
- Suggesting edits under `.old/`.

## Context
Glossary: [CONTEXT.md](CONTEXT.md). Types: [SCHEMA.md](SCHEMA.md). Full rules: [AGENTS.md](AGENTS.md).
