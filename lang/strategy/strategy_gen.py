#!/usr/bin/env python3
"""Strategy generator.

One .cas atom. Differential children store unevaluated function chains.
--flow is the default alphabet. --metaflow selects R,G,B,A.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import place

FLOW_BASES = list("ijklmnopqrstefg")
METAFLOW_BASES = ["R", "G", "B", "A"]


def parse_bases(text: str) -> list[str]:
    raw = text.replace("{", " ").replace("}", " ").replace(",", " ")
    bases = raw.split()
    if not bases:
        raise ValueError("basis alphabet is empty")
    for b in bases:
        if len(b) != 1 or not b.isalpha():
            raise ValueError(f"basis {b!r} must be a single letter")
    if len(set(bases)) != len(bases):
        raise ValueError("basis alphabet has duplicates")
    return bases


def check_name(name: str) -> str:
    if not name or not (name[0].isalpha() or name[0] == "_"):
        raise ValueError(f"name {name!r} must start with a letter or underscore")
    for ch in name:
        if not (ch.isalnum() or ch == "_"):
            raise ValueError(f"name {name!r} must be letters, digits, or underscore")
    return name


def parse_templates(text: str) -> list[str]:
    names = [part.strip() for part in text.split(",")]
    names = [part for part in names if part]
    if not names:
        raise ValueError("template list is empty")
    for name in names:
        check_name(name)
    if len(set(names)) != len(names):
        raise ValueError("template list has duplicates")
    return names


def render(bases: list[str], name: str, templates: list[str] | None = None) -> str:
    if templates is None:
        templates = ["diff"]
    lines = [f'&{name}::Strategy(bases="{",".join(bases)}", templates="{",".join(templates)}"):{{']
    bodies: list[str] = []
    fn = 1
    for basis in bases:
        for template in templates:
            # skew is the Ore extension K[x][d; sigma, delta]: tau, sigma, delta_amp.
            sig = "(tau, sigma, delta_amp)" if template == "skew" else "(...)"
            lines.append(f"    $d.{basis}::{template}:(")
            lines.append(f"        %fn_{fn}::{sig};")
            lines.append("    );")
            bodies.append(": ore" if template == "skew" else "")
            fn += 1
    lines.append("};")
    lines.append("")
    for i, mark in enumerate(bodies, start=1):
        sig = "(tau, sigma, delta_amp)" if mark else "(...)"
        lines.append(f"%fn_{i}::{sig}{mark};")
    return "\n".join(lines) + "\n"


def self_test() -> None:
    got = render(parse_bases("i,j,k"), "default")
    assert got == (
        '&default::Strategy(bases="i,j,k", templates="diff"):{\n'
        "    $d.i::diff:(\n"
        "        %fn_1::(...);\n"
        "    );\n"
        "    $d.j::diff:(\n"
        "        %fn_2::(...);\n"
        "    );\n"
        "    $d.k::diff:(\n"
        "        %fn_3::(...);\n"
        "    );\n"
        "};\n"
        "\n"
        "%fn_1::(...);\n"
        "%fn_2::(...);\n"
        "%fn_3::(...);\n"
    )
    skew = render(parse_bases("i,j,k"), "skewtrig", ["skew"])
    assert "$d.i::skew:(" in skew and "$d.j::skew:(" in skew and "$d.k::skew:(" in skew
    assert "%fn_1::(tau, sigma, delta_amp): ore;" in skew
    paired = render(["R"], "default", parse_templates("diff,d"))
    assert paired == (
        '&default::Strategy(bases="R", templates="diff,d"):{\n'
        "    $d.R::diff:(\n"
        "        %fn_1::(...);\n"
        "    );\n"
        "    $d.R::d:(\n"
        "        %fn_2::(...);\n"
        "    );\n"
        "};\n"
        "\n"
        "%fn_1::(...);\n"
        "%fn_2::(...);\n"
    )
    meta = render(METAFLOW_BASES, "default")
    assert "$d.R::diff:(" in meta
    assert "%fn_1::(...);" in meta
    assert "yield" not in meta
    both = select_bases(False, False, None)
    assert both[:3] == ["i", "j", "k"] and both[-4:] == ["R", "G", "B", "A"]
    assert select_bases(True, False, None) == list(FLOW_BASES)
    assert select_bases(False, True, None) == list(METAFLOW_BASES)
    assert select_bases(True, True, "i,j") == ["i", "j"]


def select_bases(flow: bool, meta: bool, bases: str | None) -> list[str]:
    if bases is not None:
        return parse_bases(bases)
    if flow and not meta:
        return list(FLOW_BASES)
    if meta and not flow:
        return list(METAFLOW_BASES)
    return list(FLOW_BASES) + list(METAFLOW_BASES)


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Generate a Strategy .cas atom.")
    p.add_argument("--flow", action="store_true", help="Flow alphabet only, unless --bases is set")
    p.add_argument("--metaflow", action="store_true", help="MetaFlow alphabet only, unless --bases is set")
    p.add_argument("--bases", default=None, help="comma-separated imaginary bases")
    p.add_argument("--templates", default="diff", help="comma-separated template headers, one use per basis")
    p.add_argument("--name", default="default", help="strategy header word")
    p.add_argument("--dir", default=None, help="output directory; default is the shared place rule")
    p.add_argument("--stdout", action="store_true", help="print the .cas text to stdout")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args(argv)
    if args.self_test:
        self_test()
        print("strategy_gen self-test ok")
        return 0
    bases = select_bases(args.flow, args.metaflow, args.bases)
    templates = parse_templates(args.templates)
    name = check_name(args.name)
    text = render(bases, name, templates)
    folder = place.place_dir(place.family_dir(__file__), name, args.dir)
    (folder / f"{name}.cas").write_text(text, encoding="utf-8", newline="\n")
    if args.stdout:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ValueError as exc:
        print(f"strategy_gen: {exc}", file=sys.stderr)
        raise SystemExit(1)
