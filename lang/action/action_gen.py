#!/usr/bin/env python3
"""Action modulation generator.

Default atom: &default::Action(m, n, o):"m = n (mod o)";
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import place


def check_name(name: str) -> str:
    if not name or not (name[0].isalpha() or name[0] == "_"):
        raise ValueError(f"name {name!r} must start with a letter or underscore")
    for ch in name:
        if not (ch.isalnum() or ch == "_"):
            raise ValueError(f"name {name!r} must be letters, digits, or underscore")
    return name


def check_param(label: str, value: int | None) -> int | None:
    if value is None:
        return None
    if value < 1 or value > 288:
        raise ValueError(f"{label} must be in 1..288")
    return value


def render(name: str, m: int | None = None, n: int | None = None, o: int | None = None) -> str:
    if m is None and n is None and o is None:
        params = "m, n, o"
    else:
        parts = []
        parts.append(f"m={m}" if m is not None else "m")
        parts.append(f"n={n}" if n is not None else "n")
        parts.append(f"o={o}" if o is not None else "o")
        params = ", ".join(parts)
    return f'&{name}::Action({params}):"m = n (mod o)";\n'


def self_test() -> None:
    assert render("default") == '&default::Action(m, n, o):"m = n (mod o)";\n'
    assert render("default", 2, 3, 4) == '&default::Action(m=2, n=3, o=4):"m = n (mod o)";\n'
    try:
        check_param("m", 0)
        raise AssertionError("expected range error")
    except ValueError:
        pass


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Generate an Action modulation atom.")
    p.add_argument("--name", default="default")
    p.add_argument("--m", type=int, default=None)
    p.add_argument("--n", type=int, default=None)
    p.add_argument("--o", type=int, default=None)
    p.add_argument("--dir", default=None, help="output directory; default is the shared place rule")
    p.add_argument("--stdout", action="store_true")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args(argv)
    if args.self_test:
        self_test()
        print("action_gen self-test ok")
        return 0
    name = check_name(args.name)
    m = check_param("m", args.m)
    n = check_param("n", args.n)
    o = check_param("o", args.o)
    text = render(name, m, n, o)
    folder = place.place_dir(place.family_dir(__file__), name, args.dir)
    (folder / f"{name}.cas").write_text(text, encoding="utf-8", newline="\n")
    if args.stdout:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ValueError as exc:
        print(f"action_gen: {exc}", file=sys.stderr)
        raise SystemExit(1)
