# CLAUDE.md

Claude Code loads this file automatically. Shared context is imported below. Keep this file short.

@CONTEXT.md
@SCHEMA.md
@AGENTS.md

## How to work here
- **The working spec is this repository's Markdown:** docs/Canor.md (the project and its ten languages), SCHEMA.md (types, encodings, PINNED), AGENTS.md (invariants), MEMORY.md (decisions), CONTEXT.md (glossary). Cite the section labels (Strategy §1, Flow §2 … Aether §10). The PDF drafts are legacy. Don't look for them outside the repository.
- **Plan before editing** anything marked PINNED in `SCHEMA.md`. Propose the change and wait for approval.
- **Canonical Naming.** Ensure every header, function, and structure strictly follows the `canor_*` naming standard per MEMORY.md.
- **Verify by property.** Examples: Flow count stream has n+1 vectors, residues are canonical, every trifecta obeys Hamilton's rules (`i*j=k`/`j*i=-k`, likewise `l*m=n` … `e*f=g`), octonion non-associativity, sedenion zero divisors, United digest stability.
- After any change to the C core, run build + tests + the lint grep from AGENTS.md, then report the result.

## Locales
English (Follow™) is primary. Don't translate docs unless asked. When asked, add `README.<locale>.md` siblings (`ru`, `fr`, `zh-Hant`, `la`).

## Memory
Append durable decisions to [MEMORY.md](MEMORY.md).

## Slash-command hints
- `/review`: also check the AGENTS.md invariants and `canor_*` naming.
- TODO: project commands under `.claude/commands/`.
