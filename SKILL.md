---
name: canor-module
description: Add a new integer-only pure-C canor_* module to the Canor tree while keeping the project invariants.
---

# Skill: Add or port a `canor_*` module

## When to use
- Creating a new `canor_*` module.
- Implementing or migrating a module into `canor_*`.

## Steps
1. **Locate the destination** using the folder tables in [README.md](README.md). Read that folder's `AGENTS.md` and `CONTEXT.md`.
2. **Read the spec section** in `Canor_Working-Draft_v1.pdf`. For a port, also read both legacy `.h` and `.c`.
3. **Rename** using the [rename map](MEMORY.md#legacy--canor-rename-map). That covers file names, functions, types, macros, and status codes.
4. **Header first.** Create `include/canor_<module>.h` with an include guard, `extern "C"`, `<stdint.h>`/`<stddef.h>`/`<stdbool.h>` only, `#ifndef CANOR_*_MAX` capacities, and a block comment citing the spec and any C substitution.
5. **Implementation.** Create `src/canor_<module>.c`. Validate arguments, check capacity, and return `canor_status`. Diagnose undefined cases; don't prune them.
6. **Tests.** Add property tests (round-trip, n+1 count stream, canonical residues, capacity refusal, algebraic identities).
7. **Lint:**
   ```sh
   rg -n "float|double|math\.h|malloc|free\(" include/ src/   # must be empty
   ```
8. **Build and test** per [WORKFLOW.md](WORKFLOW.md).
9. **Record** decisions in [MEMORY.md](MEMORY.md). Update the folder's `CONTEXT.md` legacy table.

## Done when
- [ ] Builds with `-Wall -Wextra -Werror -pedantic`
- [ ] Tests pass and the lint grep is empty
- [ ] Folder `CONTEXT.md` marks the module as ported
