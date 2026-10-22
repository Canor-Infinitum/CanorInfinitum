# SEMANTICS.md: Default Canor Grammar Semantics

**Status: proposal.** The defaults live in [default.fwp](../../default.fwp) and the eight files under [settings/](../../settings/). Every setting can be overridden. The `.fwp` and `.fws` files are themselves written in [atoms.ebnf](atoms.ebnf).

`atoms.ebnf` departs from WD v1 Appendix A in two places (MEMORY.md, 2026-10-20):
- `:: TEMPLATE` is optional. Without a template there is no `::`.
- `.` is a header character, so headers can be namespaced: `@lexis.whitespace`.

## The shape of a definition

```
SIGIL HEADER [ :: TEMPLATE ] [ GROUP ] [ : VALUE ] ;
```

```canor
&Test;
@lexis.whitespace:keep;
@sigil.amp(glyph="&", role=type);
&Test::_polymorphic:"Uses the generic template polymorphic.";
```

| Part | Default meaning |
|---|---|
| Sigil (`ATOM_PATH`) | The role: `&` type, `$` variable, `%` modulation, `#` index, `@` registry, `?` candidate, `!` effect, `^` upper, `_` generic, `~` Soul, `/` resource, `\` TeX. |
| Header | The name (dotted segments that follow [Names](#names)), then optional tags separated by spaces: locale `en ru fr zh-Hant la`, version `v2`, `pure`/`effect`, `sum`/`nosum`, `soul-NAME`. |
| `:: TEMPLATE` | Optional. It names the definition this one instantiates. A sigil prefix (`_polymorphic`) selects a user definition by sigil and name, and a bare name (`Tensor`) is a built-in. Suffixes: `^n`/`_n` give rank, `?` partial, `!` effectful. |
| Group | `( )` symmetric (order-free), `[ ]` antisymmetric (a swap flips the sign), `{ }` unknown symmetry (order as written), `< \| >` bra-ket. Parameters override the template's. |
| `: VALUE` | The body. `:=` defines, `=` is equality (≡ mod N under `Modulation`), `<=`/`>=` are bounds. It overrides the template's body. |

## Examples

```canor
_polymorphic::Template(T="&Any;");
&Template_Selection::Template(Param1="", Param2="");
&Type_Definition.Variable_Selection::&Template_Selection(Param1="_T:&T;", Param2="Generics"):Variable_Selection \in T := "Lorem ipsum dolor sit amet...";
&ijk::Trifecta[i, j, k]:{i*j = k, j*i = -k, i*i = -1};
&RGBA::MetaFlow[R, G, B, A];
%Z4::Modulation(N=4):x + 4 = x;
~Q soul-ijk::Soul(flow="&ijk;", action="%Z4;", order=identity);
&g::Tensor^0_2:g_{ab} = g_{ba};
&__basis__::Atom;
___slot__::Var;
&Riemann::Tensor^1_3:R^{a}_{bcd} := \partial_{c} \Gamma^{a}_{bd} - \partial_{d} \Gamma^{a}_{bc} + \Gamma^{a}_{ce} \Gamma^{e}_{bd} - \Gamma^{a}_{de} \Gamma^{e}_{bc};
&Overlap::Tensor:<\psi|\phi> := \delta;
$latency.nom::Interval(lo=10, hi=20);
\hbar::Glyph(tex="hbar", codepoint=0x210F);
&Test v2 la::_polymorphic;
!db.commit effect::Carmen(lang="c11", contract="canordb-write");
```

`"_T:&T;"` is a quoted fragment that binds the generic `T` with the bound `&T`. The last line parses, but it is **denied by default**: no contract authorizes the effect, so it returns `CANOR_ERR_UNDEFINED` and its branch is kept.

## Rules ([canor_lexis.fws](../../settings/canor_lexis.fws))

| Setting | Meaning |
|---|---|
| `whitespace:keep` | Whitespace between tokens is insignificant and is kept as lexical metadata (Canor §2 Flow). |
| `path_header:bare_sigil` | The path is the bare sigil and the header is what follows it. A path with scripts (`&T_{i} name`) needs a space before the header. |
| `scripts(brace=required)` | `_{` opens a subscript, so `T_i` is one name and `T_{i}` is T sub i. |
| `hyphen` | `-` appears only in header tags (`zh-Hant`, `soul-ijk`). Everywhere else it is minus: `x-1` is x − 1. |
| `binding:kvp` | `KEY=` is a binding. Use `:=` to define a bare name. |
| `equation` | An equation splits at the first depth-0 `:=`, or else at the first `<=` `>=` `=`. |
| `braket` | `<` is a braket if `<a\|b>` completes. Otherwise it is `<=` or `<`. |
| `group_item` | A group item or value takes the first alternative that consumes all of it: kvp, group, typed atom, symbolic atom. |
| `root` | Where a sigil is expected, the first character is the sigil. A first `_` is always the generic sigil, except in `_{` and `^{`, which open a left script. |
| `symbol` | A name resolves to a parameter, then a header Soul symbol (Canor §7 United), then a glyph. Otherwise it stays a free symbol. |
| `number` | Binary, decimal, or hex integers. No decimal point: use exact `/` or fixed point over `CANOR_ONE`. |
| `fragment` | `"SIGIL…;"` strings are parsed as a reference, then as a bound, then as a definition. If none fits, the string stays text and a diagnostic is emitted. |
| `identity` | Sigil, header, and version identify a definition, compared case-insensitively, so `&Test` and `_Test` are different but `&Test` and `&test` collide. A later `.fws` overrides an earlier one, and the override is recorded in provenance. |
| `limits` | Each name segment (between dots) is at most 32 characters (`CANOR_IDENT_MAX`), group depth 16, tensor rank 4. Exceeding one returns `CANOR_ERR_FULL`. |

Effects are denied unless a contract allows them ([canor_contracts.fws](../../settings/canor_contracts.fws)). The diagnostic codes are listed in [canor_diagnostics.fws](../../settings/canor_diagnostics.fws).

## Names

Every name follows the workspace-wide convention in [SCHEMA.md](../../SCHEMA.md#names-workspace-wide--pinned) ([canor_names.fws](../../settings/canor_names.fws)). That covers header segments, template names, keys, and `.ini` variables.

| Written | Sigil | Name | Class |
|---|---|---|---|
| `&Test` | `&` | `Test` | Public |
| `&__basis__` | `&` | `__basis__` | Global |
| `@_x_` | `@` | `_x_` | Scoped |
| `_polymorphic` | `_` | `polymorphic` | Public |
| `___slot__` | `_` | `__slot__` | Global |
| `__x__` | `_` | `_x__` | invalid, `CANOR_ERR_ARGS` |

A template name is as long as the naming rule allows, and sigil suffixes start where the name ends. So `Tensor_2` is the name `Tensor_2`, and a (0,2)-tensor is written `Tensor^0_2`.

## Overriding

Add a `local.fwp` with `@extends(by="default.fwp");` and list more `.fws` files. Later entries win.
