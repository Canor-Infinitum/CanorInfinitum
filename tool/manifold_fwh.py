#!/usr/bin/env python3
"""Preset simplexes whose shape matches the worked examples.

Flow orders 1..12 on each of the five trifectas. An order-n flow is a word
of length n on that trifecta's three atoms (repeats allowed), so the jammed
form looks like iiiijijjjjjkjkkkk and the sanitized form is i_1, j_1, k_1.

MetaFlow orders 1..9 on R, G, B, A. Each word is written once per ring
index 1..96: RG and R_3, G_3.

Action is unchanged: m = n (mod o) for m, n, o in 1..288. Pass --action to
rewrite that file.
"""

from __future__ import annotations

import argparse
from pathlib import Path

TRIFECTAS = (
    ("i", "j", "k"),
    ("l", "m", "n"),
    ("o", "p", "q"),
    ("r", "s", "t"),
    ("e", "f", "g"),
)
RGBA = "RGBA"


def word(alphabet: str, length: int, index: int) -> str:
    chars = []
    for _ in range(length):
        chars.append(alphabet[index % len(alphabet)])
        index //= len(alphabet)
    chars.reverse()
    return "".join(chars)


def sanitized(text: str, ring: int) -> str:
    return ", ".join("%s_%d" % (ch, ring) for ch in text)


def write_flows(path: Path, order_max: int) -> None:
    with path.open("wb", buffering=8 * 1024 * 1024) as out:
        batch = []
        for t, names in enumerate(TRIFECTAS, start=1):
            for n in range(1, order_max + 1):
                count = 3 ** n
                for i in range(count):
                    compact = word(names, n, i)
                    batch.append(
                        "Flow.%d.%d.%d : %s | %s\n"
                        % (t, n, i, compact, sanitized(compact, t))
                    )
                if len(batch) >= 200000:
                    out.write("".join(batch).encode("ascii"))
                    batch.clear()
        if batch:
            out.write("".join(batch).encode("ascii"))


def write_metaflows(path: Path, order_max: int, ring_max: int) -> None:
    with path.open("wb", buffering=8 * 1024 * 1024) as out:
        batch = []
        for n in range(1, order_max + 1):
            count = 4 ** n
            for i in range(count):
                compact = word(RGBA, n, i)
                for ring in range(1, ring_max + 1):
                    batch.append(
                        "MetaFlow.%s.%d : %s | %s\n"
                        % (compact, ring, compact, sanitized(compact, ring))
                    )
                if len(batch) >= 200000:
                    out.write("".join(batch).encode("ascii"))
                    batch.clear()
        if batch:
            out.write("".join(batch).encode("ascii"))


def write_actions(path: Path, limit: int = 288) -> None:
    with path.open("wb", buffering=8 * 1024 * 1024) as out:
        batch = []
        for m in range(1, limit + 1):
            for n in range(1, limit + 1):
                for o in range(1, limit + 1):
                    batch.append("Action.%d.%d.%d : %d = %d (mod %d)\n" % (m, n, o, m, n, o))
                if len(batch) >= 200000:
                    out.write("".join(batch).encode("ascii"))
                    batch.clear()
        if batch:
            out.write("".join(batch).encode("ascii"))


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=root / "CanorDatabasis")
    parser.add_argument("--flow-order", type=int, default=12)
    parser.add_argument("--meta-order", type=int, default=9)
    parser.add_argument("--rings", type=int, default=96)
    parser.add_argument("--action", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    write_flows(args.out / "flow.fwh", args.flow_order)
    write_metaflows(args.out / "metaflow.fwh", args.meta_order, args.rings)
    if args.action:
        write_actions(args.out / "action.fwh")


if __name__ == "__main__":
    main()
