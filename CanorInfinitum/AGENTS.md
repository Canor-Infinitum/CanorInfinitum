# AGENTS.md: Universal Coding Standards

Instructions for any AI coding agent working within this repository. Subfolders contain localized `AGENTS.md` files that supplement these guidelines with directory-specific context, but never relax these core standards.

## Non-Negotiable Invariants

1. **Pure Integer Arithmetic:** Do not use `float`, `double`, or the standard math library (`<math.h>`). The specification mandates integer arithmetic with closed interval bounds throughout all components. Calculations must use exact integers, closed `[LO, HI]` integer intervals, and fixed-point representations over `CANOR_ONE` (`1000`). Results must be bit-identical across all host and target architectures.
2. **Static Capacity Management:** Every container must declare a compile-time capacity limit overridable via `#ifndef CANOR_*_MAX`. The caller supplies all memory storage. Dynamic heap allocation (`malloc`, `free`) is strictly prohibited in the core library. Every insertion must verify available capacity and return `CANOR_ERR_FULL` upon overflow.
3. **Explicit Diagnostics (diagnose, never prune):** Functions must return unified status codes (`canor_status`). Undefined or invalid inputs must emit explicit diagnostics, never silently dropped branches, silent fallbacks or process terminations (`abort()`, `exit()`).
4. **Side-Effect-Free Assembly:** Assembly and compilation phases (Elite, Maneuver verification) must never mutate CanorDB or external state. State mutations occur exclusively at runtime when authorized by `.fwp` profile and `.fws` settings contracts.
5. **Formal Flow Semantics:** Flow values are immutable and ordered. Duplicate atoms represent distinct sequential positions and are never folded. An omitted ring index defaults to `0`. An `Order` is an exact permutation bijection over `Index(n)`. Count-stream traversals emit exactly $n + 1$ vectors. Modulations yield canonical non-negative residues ($0 \le r < N$).
6. **No Unnamed Constants:** All numerical constants must be explicitly named so the assembler and compiler can track their semantics.
7. **Frozen Encodings:** Pinned encodings are specification invariants (see [SCHEMA.md](SCHEMA.md)): FlowValue (`NegZero=0, NegOne=1, PosZero=2, PosOne=3`), IMAG 5-trifecta sedenions (16 basis elements), each trifecta one of Hamilton's quaternionic trifectas, and MetaFlow channel labels (`R, G, B, A`).
8. **Reproducibility & Provenance:** Record profile versions, settings versions, author attribution, and date provenance with every artifact, including every source header (`.fwh`). Agent, LLM and MCMC outputs enter as recorded inputs with their seed and provenance.
9. **Licensing:** Licensed under the GNU AGPL v3.0 or later.

## Identifier Naming Conventions

- All identifiers within the codebase follow the canonical `canor_*` naming convention (see [MEMORY.md](MEMORY.md#canonical-module-mapping-approved)):
  - Header files: `include/canor_<module>.h`
  - Source files: `src/canor_<module>.c`
  - Public functions: `canor_<module>_<verb>()`
  - Type definitions: `canor_<name>_t`
  - Preprocessor macros and constants: `CANOR_*`
- Code must adhere to C11 standards and compile cleanly under `-Wall -Wextra -Werror -pedantic`. Use fixed-width types from `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, and provide `extern "C"` inclusion guards.
- Block comments must explain implementation rationale, citing the spec by section label (e.g. `Canor §5 Soul`; see SCHEMA.md "Section labels").

## Canonical Verification Commands

```sh
configure: cmake -S source -B build -G Ninja -DCMAKE_C_COMPILER=gcc
build:     cmake --build build
test:      ctest --test-dir build --output-on-failure
generate:  build/canor_gen --out build/fw
zephyr:    west build -b <board> app
lint:      sh source/tools/canor_lint.sh
```

The lint passes (exit 0) only when no line matches. It flags heap calls (`malloc`, `calloc`, `realloc`, `free`), floating-point types, `sqrt`, and `<math.h>`. It blanks comments and string literals first (keeping line numbers), so prose may discuss them, while code after a comment or a line starting with `*` is still checked. It exits 2 if it finds no library files. Its scope is the library: `examples/` is tracked separately.

## Prohibited Actions

- Do not edit reference files under `.old/` (maintained strictly as read-only historical context).
- Do not introduce external dependencies into the pure-C core. Complementary-language wrappers belong under `CanorLinguae/`.
- Do not commit generated binary artifacts, `.fwo` files, WASM modules, or WebM video streams to version control.

## Reference Documents

[docs/Canor.md](docs/Canor.md) · [CONTEXT.md](CONTEXT.md) · [SCHEMA.md](SCHEMA.md) · [WORKFLOW.md](WORKFLOW.md) · [MEMORY.md](MEMORY.md)
