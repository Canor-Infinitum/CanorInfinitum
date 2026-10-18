# CLAUDE.md

Claude Code loads this file automatically. Shared context is imported below. Keep this file short.

@CONTEXT.md
@SCHEMA.md
@AGENTS.md

## How to work here
- **The primary spec is `D:\Workspace\Canor_Working-Draft_v1.pdf`.** Cite its section numbers (Flow §1 … Aether §9).
- **Plan before editing** anything marked PINNED in `SCHEMA.md`. Propose the change and wait for approval.
- **Port, then rename.** When migrating from `.old`, keep the rationale comments, but rename every `fc_*`/`vi_*` identifier to `canor_*` using the map in MEMORY.md.
- **Verify by property.** Examples: Flow count stream has n+1 vectors, residues are canonical, quaternion `i*j=k`/`j*i=-k`, octonion non-associativity, sedenion zero divisors, United digest stability.
- After any change to the C core, run build + tests + the lint grep from AGENTS.md, then report the result.

## Locales
English (Follow™) is primary. Don't translate docs unless asked. When asked, add `README.<locale>.md` siblings (`ru`, `fr`, `zh-Hant`, `la`).

## Memory
Append durable decisions to [MEMORY.md](MEMORY.md).

## Slash-command hints
- `/review`: also check the AGENTS.md invariants and `canor_*` naming.
- TODO: project commands under `.claude/commands/`.
