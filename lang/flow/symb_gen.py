#!/usr/bin/env python3
"""Symbolic Flow and MetaFlow generator.

--flow is the default. --metaflow selects the R,G,B,A alphabet and MetaFlow atoms.
--symbolic is eight channels: LTR prime, dot, caret, under, then RTL under, caret, dot, prime.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "symb"))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import ring_expr
import place

MAX_ORDER = 288
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


def cardinality(n_bases: int, order: int) -> int:
    if order < 1 or order > MAX_ORDER:
        raise ValueError(f"order must be in 1..{MAX_ORDER}")
    if n_bases < 1:
        raise ValueError("basis alphabet is empty")
    return n_bases ** order


def digits_of(index: int, n_bases: int, order: int) -> list[int]:
    card = cardinality(n_bases, order)
    if index < 0 or index >= card:
        raise ValueError(f"index {index} is outside 0..{card - 1}")
    out = [0] * order
    for i in range(order - 1, -1, -1):
        index, out[i] = divmod(index, n_bases)
    return out


def iter_words(bases: list[str], order: int, index: int, count: int, direction: str):
    card = cardinality(len(bases), order)
    if count < 0 or index < 0 or index > card or index + count > card:
        raise ValueError(f"slice index={index} count={count} exceeds cardinality {card}")
    digits = digits_of(index, len(bases), order) if count else [0] * order
    for _ in range(count):
        seq = digits if direction == "LTR" else list(reversed(digits))
        yield " ".join(bases[d] for d in seq)
        for i in range(order - 1, -1, -1):
            digits[i] += 1
            if digits[i] < len(bases):
                break
            digits[i] = 0


def check_name(name: str) -> str:
    if not name or not name[0].isalpha() or any(c != "_" and c != "-" and not c.isalnum() for c in name):
        raise ValueError(f"name {name!r} must be a letter followed by letters, digits, '_' or '-'")
    return name


def quote_atom(text: str) -> str:
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def header_text(bases, order, direction, index, count, name, kind, symbolic: str) -> str:
    listed = ", ".join(bases)
    card = cardinality(len(bases), order)
    return (
        f"@{name}::{kind}.{order}.{index}.{count}:{{\n"
        f"    $bases:{{ {listed} }};\n"
        f"    $direction:{quote_atom(direction)};\n"
        f"    $symbolic:{quote_atom(symbolic)};\n"
        f"    $cardinality:{card};\n"
        f"}};\n"
    )


def iter_atoms(bases, order, index, count, direction, name, kind, prog: ring_expr.Prog):
    for offset, word in enumerate(iter_words(bases, order, index, count, direction)):
        k = index + offset
        prev = (0, 0, 0, 0, 0, 0, 0, 0)
        parts = []
        for p, symbol in enumerate(word.split(" ")):
            vals = prog.eval(p, bases.index(symbol), order, prev, k, count)
            parts.append(ring_expr.decorate(symbol, vals))
            prev = vals
        body = "{ " + ", ".join(parts) + " }"
        yield f"@{name}::{kind}.{order}.{k}.{count}:{body};"


def write_pair(stem, bases, order, direction, index, count, name, kind, prog) -> None:
    with open(stem + ".cah", "w", encoding="utf-8", newline="\n") as fh:
        fh.write(header_text(bases, order, direction, index, count, name, kind, prog.source))
    with open(stem + ".cas", "w", encoding="utf-8", newline="\n") as fb:
        for line in iter_atoms(bases, order, index, count, direction, name, kind, prog):
            fb.write(line + "\n")


def self_test() -> None:
    bases = parse_bases("i")
    bare = ring_expr.compile_symbolic("0,0,0,0,0,0,0,0")
    assert list(iter_atoms(bases, 1, 0, 1, "LTR", "flow", "Flow", bare)) == [
        "@flow::Flow.1.0.1:{ i };"
    ]
    walk = ring_expr.compile_symbolic("0,0,0,p+1,p+1,0,0,0")
    recur = ring_expr.compile_symbolic("0,0,0,U+1,RU+1,0,0,0")
    got = list(iter_atoms(bases, 3, 0, 1, "LTR", "flow", "Flow", walk))
    assert got == ["@flow::Flow.3.0.1:{ 1_i_1, 2_i_2, 3_i_3 };"]
    assert list(iter_atoms(bases, 3, 0, 1, "LTR", "flow", "Flow", recur)) == got
    full = ring_expr.compile_symbolic("1,2,3,4,4,3,2,1")
    assert list(iter_atoms(bases, 1, 0, 1, "LTR", "flow", "Flow", full)) == [
        "@flow::Flow.1.0.1:{ 1'2.3^4_i_4^3.2'1 };"
    ]
    ijk = parse_bases("i,j,k")
    mod = ring_expr.compile_symbolic("0,0,0,k%4,k%4,0,0,0")
    assert list(iter_atoms(ijk, 2, 5, 1, "LTR", "flow", "Flow", mod)) == [
        "@flow::Flow.2.5.1:{ 1_j_1, 1_k_1 };"
    ]
    caret = ring_expr.compile_symbolic("0,0,a+1,0,0,a+1,0,0")
    assert list(iter_atoms(METAFLOW_BASES, 4, 0, 1, "LTR", "metaflow", "MetaFlow", caret)) == [
        "@metaflow::MetaFlow.4.0.1:{ 1^R^1, 1^R^1, 1^R^1, 1^R^1 };"
    ]
    cah = header_text(parse_bases("i,j"), 2, "LTR", 0, 4, "flow", "Flow", "0,0,0,0,0,0,0,0")
    assert cah == (
        "@flow::Flow.2.0.4:{\n"
        "    $bases:{ i, j };\n"
        '    $direction:"LTR";\n'
        '    $symbolic:"0,0,0,0,0,0,0,0";\n'
        "    $cardinality:4;\n"
        "};\n"
    )
    try:
        ring_expr.compile_symbolic("k+1,0,0,0,0,0,0,0")
        raise AssertionError("expected k error")
    except ring_expr.RingError:
        pass


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Generate Flow or MetaFlow atoms with ring scripts.")
    p.add_argument("--flow", action="store_true", help="Flow atoms (default)")
    p.add_argument("--metaflow", action="store_true", help="MetaFlow atoms, default bases R,G,B,A")
    p.add_argument("--bases", default=None, help="comma-separated imaginary bases")
    p.add_argument("--order", type=int, help=f"string length, 1..{MAX_ORDER}")
    p.add_argument("--index", default="0", help="starting lexicographic index (decimal)")
    p.add_argument("--count", default="1", help="how many strings to emit, or 'all'")
    p.add_argument("--direction", choices=("ltr", "rtl", "LTR", "RTL"), default="LTR")
    p.add_argument("--name", default=None, help="atom name")
    p.add_argument("--symbolic", default="0,0,0,0,0,0,0,0", help="eight ring-script expressions")
    p.add_argument("--dir", default=None, help="output directory; default is the shared place rule")
    p.add_argument("--stdout", action="store_true", help="print the .cas lines to stdout")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args(argv)
    if args.self_test:
        self_test()
        print("symb_gen self-test ok")
        return 0
    if args.flow and args.metaflow:
        p.error("--flow and --metaflow are mutually exclusive")
    meta = args.metaflow
    if args.order is None:
        p.error("--order is required")
    bases = parse_bases(args.bases if args.bases is not None else ",".join(METAFLOW_BASES if meta else FLOW_BASES))
    name = check_name(args.name if args.name is not None else ("metaflow" if meta else "flow"))
    kind = "MetaFlow" if meta else "Flow"
    direction = args.direction.upper()
    index = int(args.index, 10)
    card = cardinality(len(bases), args.order)
    count = card - index if args.count == "all" else int(args.count, 10)
    prog = ring_expr.compile_symbolic(args.symbolic)
    folder = place.place_dir(place.family_dir(__file__), name, args.dir)
    write_pair(str(folder / name), bases, args.order, direction, index, count, name, kind, prog)
    if args.stdout:
        for line in iter_atoms(bases, args.order, index, count, direction, name, kind, prog):
            sys.stdout.write(line + "\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, ring_expr.RingError) as exc:
        print(f"symb_gen: {exc}", file=sys.stderr)
        raise SystemExit(1)
