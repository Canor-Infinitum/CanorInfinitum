# AGENTS.md

Instructions for any AI coding agent working in this repository. Subfolders have their own `AGENTS.md`. Those add local rules and never relax these.

## Non-negotiable invariants
1. **Integer only.** Use no `float`, `double`, or `<math.h>`. The draft mandates "Integer Programming with Interval Arithmetic … throughout all of Canor™". Use exact integers, closed `LO/HI` integer intervals, and fixed-point scores over `CANOR_ONE` (1000). Results must be bit-identical across hosts.
2. **Fixed capacities.** Every container has an `#ifndef`-overridable `CANOR_*_MAX`. The caller places all values. Use no hidden `malloc` in the core. Every insert checks capacity and returns `CANOR_ERR_FULL`.
3. **Diagnose, never prune.** Return `canor_status` codes. Undefined cases produce **explicit diagnostics**, never silently dropped branches. Use no `abort()`/`exit()`.
4. **Assembly is side-effect-free.** Assembly/Elite never mutates CanorDB or external state. Effects happen only at declared runtime, when the `.fwp` profile and `.fws` contracts authorize them.
5. **Flow semantics.** Flow values are immutable and ordered. Duplicate atoms count separately. An omitted ring index means 0. An `Order` is a permutation of `Index(n)`. Count streams emit **n + 1** vectors. Residues are canonical (`0 ≤ r < N`).
6. **No magic numbers.** Name constants and flag them, so the assembler/compiler can track them.
7. **Spec-pinned encodings are frozen** (see [SCHEMA.md](SCHEMA.md)): FlowValue `NegZero=0, NegOne=1, PosZero=2, PosOne=3`, IMAG = 5 trifectas (16, sedenions), and the MetaFlow labels `R,G,B,A`.
8. **Reproducibility.** Record profile and settings versions and provenance with every artifact.
9. **License:** AGPL-3.0-or-later.

## Naming: everything is `canor_*`
- Headers `include/canor_<module>.h`, sources `src/canor_<module>.c`, functions `canor_<module>_<verb>()`, types `canor_<name>_t`, macros `CANOR_*`.
- Only canonical `canor_*` identifiers may appear in codebase files. See [MEMORY.md](MEMORY.md#canonical-module-mapping-approved).
- C11, `-Wall -Wextra -Werror -pedantic`, `<stdint.h>` types, `extern "C"` guards.
- Block comments explain *why*, citing the draft by section (e.g. `Canor WD v1 §4 Soul`).

## Commands
```sh
configure: cmake -S source -B build -G Ninja -DCMAKE_C_COMPILER=gcc
build:   cmake --build build
test:    ctest --test-dir build --output-on-failure
fwh/fwb: build/canor_gen --out build/fw          # <out>/<language>/<language>.fwh|.fwb, digests profile-defined
zephyr:  west build -b <board> app
lint:    test "$(rg --files source/include source/src | wc -l)" -gt 0 && rg -n "float|double|math\.h|malloc" source/include source/src   # must be empty; scope is the library (examples/ tracked separately)
```

## Do not
- Edit `D:\Workspace\.old\` (read-only reference).
- Add dependencies to the C core. Complementary-language bindings belong under `CanorLinguae/`.
- Commit generated binaries, `.fwo`, WASM, or WebM.

## See also
[CONTEXT.md](CONTEXT.md) · [SCHEMA.md](SCHEMA.md) · [WORKFLOW.md](WORKFLOW.md) · [MEMORY.md](MEMORY.md)
