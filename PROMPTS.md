# PROMPTS.md: Reusable Prompt Templates

Replace the `{placeholders}`.

## Port and rename a legacy module
```
Port .old/follow_core-verifiable_intelligence/{include,src}/{legacy}.{h,c} to {folder}/ as canor_{module}.
Rename every fc_*/vi_*/FC_*/VI_* identifier per MEMORY.md rename map. Keep rationale comments.
Follow AGENTS.md invariants. Add property tests. Update {folder}/CONTEXT.md. Build + test + lint, report.
```

## Implement a CanorLinguae language feature
```
Implement {Flow|MetaFlow|Action|Soul|Maneuver|United|Elite|Carmen|Aether} feature {X}
per Canor_Working-Draft_v1.pdf §{n}. Integer-only; diagnose undefined cases; assembly is side-effect-free.
```

## Write a conformance test
```
Write a test for canor_{function} asserting: {property, e.g. count stream length == n+1}.
Cite the draft section. No floats, fixed-capacity buffers only.
```

## Invariant review
```
Review {files} against AGENTS.md invariants 1–9 and canor_* naming. List violations with file:line and a fix.
Do not change code.
```

## Add a complementary-language binding (Carmen Interlingua)
```
Expose canor_{functions} to {FPC|SWI-Prolog|Rust|WASM} in CanorLinguae/carmen/{lang}/.
C ABI only. Map canor_status to the target's error idiom. Declare the block in the .fwp/.fws profile.
```

## Localize a document
```
Translate {file} into {ru (Redko™)|fr (Avec™)|zh-Hant (Fengshui™)|la (Canor™)} as {file}.{locale}.md.
Keep code, identifiers and links unchanged. English is authoritative.
```
