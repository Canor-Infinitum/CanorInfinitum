"""Eight-channel integer ring expressions.

Left-to-right side, then right-to-left side:
  prime `'`, dot `.`, caret `^`, underscore `_`,
  underscore `_`, caret `^`, dot `.`, prime `'`.
Previous LTR channels: P, D, C, U.
Previous RTL channels: RP, RD, RC, RU.
k and c are legal only as k % <positive literal> and c % <positive literal>.
"""

from __future__ import annotations

from dataclasses import dataclass


class RingError(ValueError):
    pass


@dataclass
class Prog:
    roots: tuple
    source: str

    def eval(self, p: int, a: int, n: int, prev: tuple[int, ...], k: int, c: int) -> tuple[int, ...]:
        env = {
            "p": p, "a": a, "n": n, "k": k, "c": c,
            "P": prev[0], "D": prev[1], "C": prev[2], "U": prev[3],
            "RU": prev[4], "RC": prev[5], "RD": prev[6], "RP": prev[7],
        }
        out = []
        for root in self.roots:
            value = _eval(root, env)
            if value < 0:
                value = 0
            out.append(value)
        return tuple(out)


def compile_symbolic(source: str) -> Prog:
    text = source.strip()
    if not text:
        raise RingError("symbolic expression is empty")
    parts = _split_channels(text, 8)
    parser_roots = tuple(_Parser(part).parse() for part in parts)
    return Prog(parser_roots, text)


def decorate(symbol: str, vals: tuple[int, ...]) -> str:
    left_marks = ("'", ".", "^", "_")
    right_marks = ("_", "^", ".", "'")
    left = [f"{v}{m}" for v, m in zip(vals[:4], left_marks) if v > 0]
    right = [f"{m}{v}" for v, m in zip(vals[4:], right_marks) if v > 0]
    return "".join(left) + symbol + "".join(right)


def _split_channels(text: str, count: int) -> list[str]:
    parts = []
    depth = 0
    start = 0
    for i, ch in enumerate(text):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth < 0:
                raise RingError("unbalanced parenthesis")
        elif ch == "," and depth == 0:
            parts.append(text[start:i])
            start = i + 1
    if depth != 0:
        raise RingError("unbalanced parenthesis")
    parts.append(text[start:])
    if len(parts) != count:
        raise RingError(f"symbolic expression needs {count} channels")
    return parts


class _Parser:
    def __init__(self, text: str) -> None:
        self.text = text
        self.i = 0

    def parse(self):
        node = self._expr()
        self._skip()
        if self.i != len(self.text):
            raise RingError(f"unexpected {self.text[self.i:]!r}")
        return node

    def _skip(self) -> None:
        while self.i < len(self.text) and self.text[self.i].isspace():
            self.i += 1

    def _expr(self):
        cond = self._sum()
        self._skip()
        if self.i < len(self.text) and self.text[self.i] == "?":
            self.i += 1
            then = self._expr()
            self._skip()
            if self.i >= len(self.text) or self.text[self.i] != ":":
                raise RingError("ternary is missing ':'")
            self.i += 1
            else_ = self._expr()
            return ("tern", cond, then, else_)
        return cond

    def _sum(self):
        node = self._term()
        while True:
            self._skip()
            if self.i >= len(self.text) or self.text[self.i] not in "+-":
                return node
            op = self.text[self.i]
            self.i += 1
            right = self._term()
            node = ("add" if op == "+" else "sub", node, right)

    def _term(self):
        node = self._factor()
        while True:
            self._skip()
            if self.i >= len(self.text) or self.text[self.i] not in "*%":
                return node
            op = self.text[self.i]
            self.i += 1
            right = self._factor()
            node = ("mul" if op == "*" else "mod", node, right)

    def _factor(self):
        self._skip()
        if self.i >= len(self.text):
            raise RingError("expression ended early")
        ch = self.text[self.i]
        if ch == "-":
            self.i += 1
            return ("neg", self._factor())
        if ch == "(":
            self.i += 1
            node = self._expr()
            self._skip()
            if self.i >= len(self.text) or self.text[self.i] != ")":
                raise RingError("unbalanced parenthesis")
            self.i += 1
            return node
        if ch.isdigit():
            start = self.i
            while self.i < len(self.text) and self.text[self.i].isdigit():
                self.i += 1
            return ("imm", int(self.text[start:self.i]))
        if ch.isalpha():
            self.i += 1
            name = ch
            if name == "R" and self.i < len(self.text) and self.text[self.i] in "PDCU":
                name += self.text[self.i]
                self.i += 1
                return ("var", name)
            if name in "kc":
                self._skip()
                if self.i >= len(self.text) or self.text[self.i] != "%":
                    raise RingError(f"{name} is legal only as {name} % <positive integer>")
                self.i += 1
                self._skip()
                if self.i >= len(self.text) or not self.text[self.i].isdigit():
                    raise RingError(f"{name} modulus must be a positive integer literal")
                start = self.i
                while self.i < len(self.text) and self.text[self.i].isdigit():
                    self.i += 1
                mod = int(self.text[start:self.i])
                if mod <= 0:
                    raise RingError(f"{name} modulus must be a positive integer literal")
                return ("kmod" if name == "k" else "cmod", mod)
            if name not in ("p", "a", "n", "P", "D", "C", "U", "RP", "RD", "RC", "RU"):
                raise RingError(f"unknown name {name}")
            return ("var", name)
        raise RingError(f"unexpected {ch!r}")


def _eval(node, env: dict) -> int:
    kind = node[0]
    if kind == "imm":
        return node[1]
    if kind == "var":
        return env[node[1]]
    if kind == "kmod":
        return env["k"] % node[1]
    if kind == "cmod":
        return env["c"] % node[1]
    if kind == "neg":
        return -_eval(node[1], env)
    if kind == "tern":
        return _eval(node[2], env) if _eval(node[1], env) != 0 else _eval(node[3], env)
    left = _eval(node[1], env)
    right = _eval(node[2], env)
    if kind == "add":
        return left + right
    if kind == "sub":
        return left - right
    if kind == "mul":
        return left * right
    if kind == "mod":
        if right == 0:
            raise RingError("modulo by zero")
        return left % right
    raise RingError(f"bad node {kind}")
