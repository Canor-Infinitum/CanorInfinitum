#!/usr/bin/env python3
"""Language defaults for one project.

Writes the mandatory project_name.cap, project_name.cas, main.cah, and main.cab.
Further .cap, .cas, .cah, and .cab files are named in that toolchain. Compilation to .cao is not done here.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "flow"))
sys.path.insert(0, str(Path(__file__).resolve().parent / "strategy"))

import strategy_gen
import symb_gen
import place

DEFAULT_TEMPLATES = ["diff", "d", "partial", "del", "nabla"]
TEMPLATE_DECLS = "".join(
    f"\\{word}:{{}};\n" for word in ("Profile", "Settings", "Strategy", "Header", "Body", "Flow", "MetaFlow")
) + "\n"
SOURCE_SUFFIXES = {".cap", ".cas", ".cah", ".cab"}
GENERATED = (
    "strategy.cas",
    "flows.cah",
    "flows.cab",
    "metaflows.cah",
    "metaflows.cab",
    "action.cas",
    "soul.cah",
    "soul.cab",
)
LANGUAGES = ("strategy", "flow", "metaflow", "action", "soul")


def check_name(name: str) -> str:
    if not name or not (name[0].isalpha() or name[0] == "_"):
        raise ValueError(f"name {name!r} must start with a letter or underscore")
    for ch in name:
        if not (ch.isalnum() or ch == "_"):
            raise ValueError(f"name {name!r} must be letters, digits, or underscore")
    return name


EMPTY_KIND = {".cap": "Profile", ".cas": "Settings", ".cah": "Header", ".cab": "Body"}


def empty_source(path: str) -> str:
    source = Path(path)
    check_name(source.stem)
    kind = EMPTY_KIND[source.suffix]
    return f"&{source.stem}::{kind}:{{}};\n"


def check_source(path: str) -> str:
    suffix = Path(path).suffix
    if suffix not in SOURCE_SUFFIXES:
        raise ValueError(f"{path!r} must end with .cap, .cas, .cah, or .cab")
    return path.replace("\\", "/")


def language_settings() -> str:
    flow_bases = ",".join(symb_gen.FLOW_BASES)
    both = flow_bases + ",R,G,B,A"
    templates = ",".join(DEFAULT_TEMPLATES)
    return (
        "    &Strategy::Settings:{\n"
        f'        $bases:"{both}";\n'
        f'        $templates:"{templates}";\n'
        "    };\n"
        "    &Flow::Settings:{\n"
        f'        $bases:"{flow_bases}";\n'
        "        $order:1;\n"
        "        $order_min:1;\n"
        "        $order_max:288;\n"
        '        $direction:"LTR";\n'
        "        $count:1;\n"
        "    };\n"
        "    &MetaFlow::Settings:{\n"
        '        $bases:"R,G,B,A";\n'
        "        $order:1;\n"
        "        $order_min:1;\n"
        "        $order_max:288;\n"
        "        $count:1;\n"
        "    };\n"
        "    &Action::Settings:{\n"
        "        $min:1;\n"
        "        $max:288;\n"
        "    };\n"
        "    &Soul::Settings:{\n"
        "        $mip:64;\n"
        "        $scale_min:1;\n"
        "        $scale_nom:1;\n"
        "        $scale_max:255;\n"
        "    };\n"
    )


def render_cap(name: str, extra: list[str]) -> str:
    del extra
    return (
        TEMPLATE_DECLS
        + f"&{name}::Profile:{{\n"
        + f'    $manifesto:"{name}";\n'
        + language_settings()
        + "};\n"
    )


def render_cas(name: str, extra: list[str]) -> str:
    lines = [
        f"&{name}::Settings:{{",
        f'    $profile:"{name}.cap";',
        '    $header:"main.cah";',
        '    $body:"main.cab";',
        f'    $object:"{name}.cao";',
    ]
    for path in GENERATED:
        lines.append(f'    $file:"{path}";')
    for path in extra:
        lines.append(f'    $file:"{path}";')
    lines.append("};")
    return TEMPLATE_DECLS + "\n".join(lines) + "\n"


def render_cah(name: str) -> str:
    return f"&{name}.main::Header:{{\n    &master.strategy::Strategy;\n}};\n"


def render_cab(name: str) -> str:
    inner = "\n".join("    " + line if line else "" for line in render_strategy("master.strategy").splitlines())
    return f"&{name}.main::Body:{{\n{inner}\n}};\n"


def render_strategy(name: str) -> str:
    bases = strategy_gen.select_bases(False, False, None)
    return strategy_gen.render(bases, name, list(DEFAULT_TEMPLATES))


def render_words(bases: list[str], order: int, name: str, kind: str) -> tuple[str, str]:
    prog = symb_gen.ring_expr.compile_symbolic("0,0,0,0,0,0,0,0")
    count = symb_gen.cardinality(len(bases), order)
    header = symb_gen.header_text(bases, order, "LTR", 0, count, name, kind, prog.source)
    body = "".join(line + "\n" for line in symb_gen.iter_atoms(bases, order, 0, count, "LTR", name, kind, prog))
    return header, body


def strategy_files(extra_strategies: list[str]) -> list[tuple[str, str]]:
    planned = [("strategy.cas", None)]
    seen = {"strategy.cas"}
    for raw in extra_strategies:
        stem = check_name(raw)
        filename = stem + ".cas"
        if filename in seen:
            continue
        seen.add(filename)
        planned.append((filename, stem))
    return planned


def files(name: str, extra: list[str], extra_strategies: list[str] | None = None) -> dict[str, str]:
    global GENERATED
    planned = strategy_files(extra_strategies or [])
    GENERATED = tuple(filename for filename, _atom in planned) + (
        "flows.cah",
        "flows.cab",
        "metaflows.cah",
        "metaflows.cab",
        "action.cas",
        "soul.cah",
        "soul.cab",
    )
    flows_h, flows_b = render_words(list(symb_gen.FLOW_BASES), 3, "flow", "Flow")
    meta_h, meta_b = render_words(list(symb_gen.METAFLOW_BASES), 4, "metaflow", "MetaFlow")
    out = {
        f"{name}.cap": render_cap(name, extra),
        f"{name}.cas": render_cas(name, extra),
        "main.cah": render_cah(name),
        "main.cab": render_cab(name),
        "flows.cah": flows_h,
        "flows.cab": flows_b,
        "metaflows.cah": meta_h,
        "metaflows.cab": meta_b,
    }
    for filename, atom in planned:
        out[filename] = render_strategy(name if atom is None else atom)
    return out


def write_language(folder: Path, language: str, name: str) -> None:
    folder.mkdir(parents=True, exist_ok=True)
    language = language.lower()
    if language == "strategy":
        (folder / "strategy.cas").write_text(render_strategy(name), encoding="utf-8", newline="\n")
    elif language == "flow":
        header, body = render_words(list(symb_gen.FLOW_BASES), 3, "flow", "Flow")
        (folder / "flows.cah").write_text(header, encoding="utf-8", newline="\n")
        (folder / "flows.cab").write_text(body, encoding="utf-8", newline="\n")
    elif language == "metaflow":
        header, body = render_words(list(symb_gen.METAFLOW_BASES), 4, "metaflow", "MetaFlow")
        (folder / "metaflows.cah").write_text(header, encoding="utf-8", newline="\n")
        (folder / "metaflows.cab").write_text(body, encoding="utf-8", newline="\n")
    elif language == "action":
        sys.path.insert(0, str(Path(__file__).resolve().parent / "action"))
        import action_gen

        (folder / "action.cas").write_text(action_gen.render("default"), encoding="utf-8", newline="\n")
    elif language == "soul":
        sys.path.insert(0, str(Path(__file__).resolve().parent / "soul"))
        import soul_gen

        soul_gen.write_soul(folder)
    else:
        raise ValueError(f"language {language!r} must be one of {', '.join(LANGUAGES)}")


def self_test() -> None:
    got = files("project", ["notes.cah"])
    assert got["project.cap"].startswith(
        "\\Profile:{};\n"
        "\\Settings:{};\n"
        "\\Strategy:{};\n"
        "\\Header:{};\n"
        "\\Body:{};\n"
        "\\Flow:{};\n"
        "\\MetaFlow:{};\n"
        "\n"
        "&project::Profile:{\n"
        '    $manifesto:"project";\n'
    )
    assert '&Soul::Settings:{' in got["project.cap"]
    assert "$mip:64;" in got["project.cap"]
    assert "$scale_nom:1;" in got["project.cap"]
    assert "$order_max:288;" in got["project.cap"]
    assert '$object:"project.cao";' in got["project.cas"]
    assert '$file:"notes.cah";' in got["project.cas"]
    assert got["main.cah"] == "&project.main::Header:{\n    &master.strategy::Strategy;\n};\n"
    assert got["main.cab"].startswith("&project.main::Body:{\n    &master.strategy::Strategy(bases=\"i,j,k,l,m,n,o,p,q,r,s,t,e,f,g,R,G,B,A\", templates=\"diff,d,partial,del,nabla\"):{")
    assert "    $d.i::diff:(" in got["main.cab"]
    assert got["main.cab"].rstrip().endswith("};")
    assert "%fn_1::(...);" in got["strategy.cas"]
    assert "%fn_95::(...);" in got["strategy.cas"]
    more = files("project", [], ["alt"])
    assert '$file:"alt.cas";' in more["project.cas"]
    assert more["alt.cas"].startswith('&alt::Strategy(bases="')
    assert "%fn_95::(...);" in more["alt.cas"]
    assert got["strategy.cas"].startswith('&project::Strategy(bases="i,j,k,l,m,n,o,p,q,r,s,t,e,f,g,R,G,B,A", templates="diff,d,partial,del,nabla"):{')
    assert got["flows.cah"].startswith("@flow::Flow.3.0.3375:{")
    flow_lines = got["flows.cab"].splitlines()
    assert len(flow_lines) == 3375
    assert flow_lines[0] == "@flow::Flow.3.0.3375:{ i, i, i };"
    assert flow_lines[-1] == "@flow::Flow.3.3374.3375:{ g, g, g };"
    assert got["metaflows.cah"].startswith("@metaflow::MetaFlow.4.0.256:{")
    meta_lines = got["metaflows.cab"].splitlines()
    assert len(meta_lines) == 256
    assert meta_lines[0] == "@metaflow::MetaFlow.4.0.256:{ R, R, R, R };"
    assert meta_lines[-1] == "@metaflow::MetaFlow.4.255.256:{ A, A, A, A };"
    assert empty_source("notes.cah") == "&notes::Header:{};\n"
    assert empty_source("extra.cap") == "&extra::Profile:{};\n"
    assert empty_source("extra.cas") == "&extra::Settings:{};\n"
    assert empty_source("extra.cab") == "&extra::Body:{};\n"


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Write the mandatory project profile, settings, header, and body.")
    p.add_argument("--default", action="store_true", help="write the default project files")
    p.add_argument("--name", default="project", help="project name used for the .cap, .cas, and future .cao")
    p.add_argument("--dir", default=None, help="directory for the generated files; default is the shared place rule")
    p.add_argument("--strategy", action="append", default=[], help="with --default, another NAME.cas strategy")
    p.add_argument("--file", action="append", default=[], help="write an empty .cap, .cas, .cah, or .cab")
    p.add_argument("--language", action="append", default=[], help="strategy, flow, metaflow, action, or soul")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args(argv)
    if args.self_test:
        self_test()
        print("lang_gen self-test ok")
        return 0
    if not args.default and not args.file and not args.language:
        args.default = True
    name = check_name(args.name)
    extra = [check_source(path) for path in args.file]
    folder = place.place_dir(place.family_dir(__file__), name, args.dir)
    if args.default:
        for filename, text in files(name, extra, args.strategy).items():
            (folder / filename).write_text(text, encoding="utf-8", newline="\n")
        for language in LANGUAGES:
            write_language(folder, language, name)
    for language in args.language:
        if not args.default:
            write_language(folder, language, name)
    for path in extra:
        target = folder / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(empty_source(path), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ValueError as exc:
        print(f"lang_gen: {exc}", file=sys.stderr)
        raise SystemExit(1)
