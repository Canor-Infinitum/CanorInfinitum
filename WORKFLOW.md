# WORKFLOW.md

## Branching
- `main` must always build and pass.
- Feature branches: `feat/<folder>-<topic>`. Fixes: `fix/<topic>`.
- TODO: commit convention (e.g. Conventional Commits).

## Git sync (Git Bash)
```bash
cp INSTRUCTIONS.md .github/copilot-instructions.md   # keep the Copilot mirror in sync (fix ../ links if edited)
git add -A && git commit -m "<msg>"
git pull --rebase origin main
git push origin <branch>
```

## Change loop
1. Pick a task. Read the folder's `AGENTS.md` and `CONTEXT.md`.
2. Implement it (see [SKILL.md](SKILL.md)).
3. Build: `-Wall -Wextra -Werror -pedantic`.
4. Test: unit, property, and conformance round-trip.
5. Lint: no float/malloc and no `fc_`/`vi_` identifiers.
6. Update `MEMORY.md` if you made a decision.
7. Open a PR. Review against the AGENTS.md invariants.

## CI matrix (TODO)
| Target | Toolchain | Notes |
|---|---|---|
| x86_64 Linux / Windows / macOS | GCC, Clang, MSVC | sanitizers on Linux |
| aarch64 | cross-GCC + QEMU | |
| riscv32/64 | Zephyr SDK | `west build`, `twister` |
| wasm32 | Emscripten | optional |
| Carmen bindings | fpc, swipl, cargo | smoke tests |
| Docs | link checker | every relative link resolves |

**Determinism gate:** identical test vectors must produce byte-identical output on every target.
