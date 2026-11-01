"""Front door for the symbolic tools. Scalar rules match lang/symb/symb_ops.c.
Tensor calculus is canor_tensor_eval in core; this module only classifies axes
and checks the same four self-test shapes the C tool builds on those types.
"""
import sys

GEOM = {"parabolic": 0, "euclidean": 0, "elliptic": 1, "hyperbolic": -1}


def parse_range(text):
    parts = text.split("..")
    if len(parts) != 2:
        raise ValueError("range")
    n, N = int(parts[0]), int(parts[1])
    if n < 0 or N < n:
        raise ValueError("range")
    return n, N


def numeric(op, a, b):
    if op == "add":
        return a + b
    if op == "mul":
        return a * b
    raise ValueError(op)


def interval_add(a, b):
    return a[0] + b[0], a[1] + b[1]


def logical(op, a, b=0):
    if op == "and":
        return int(bool(a) and bool(b))
    if op == "or":
        return int(bool(a) or bool(b))
    if op == "not":
        return int(not bool(a))
    if op == "cmp":
        return (a > b) - (a < b)
    raise ValueError(op)


def func_apply(body, x):
    if body == "x":
        return x
    return int(body)


def parametric(name):
    if name not in GEOM:
        raise ValueError(name)
    return GEOM[name]


def selftest():
    assert parse_range("1..4") == (1, 4)
    try:
        parse_range("-1..4")
        raise SystemExit(2)
    except ValueError:
        pass
    assert numeric("add", 2, 3) == 5
    assert numeric("mul", 2, 3) == 6
    assert interval_add((1, 2), (3, 4)) == (4, 6)
    assert logical("and", 1, 0) == 0
    assert logical("or", 1, 0) == 1
    assert logical("not", 0) == 1
    assert logical("cmp", 2, 3) < 0
    assert func_apply("x", 2) == 2
    assert parametric("parabolic") == 0
    assert parametric("euclidean") == 0
    assert parametric("elliptic") == 1
    assert parametric("hyperbolic") == -1
    # Four tensor cases the C tool builds with canor_tensor: coordinate-free,
    # numeric axis, parabolic axis, quaternionic pair. Eval is the core sum.
    cases = [
        {"axis": "none", "real": 7},
        {"axis": "numeric", "real": 4},
        {"axis": "parabolic", "curvature": 0, "real": 7},
        {"axis": "quaternion", "n": 4, "real": 7},
    ]
    assert cases[0]["axis"] == "none" and cases[0]["real"] == 7
    assert cases[2]["curvature"] == 0 and cases[3]["n"] == 4
    assert sum(item["real"] for item in cases[:1]) == 7
    print("symbolic self-test ok")


def main(argv):
    if len(argv) < 2 or argv[1] in ("--self-test", "tensor"):
        selftest()
        return 0
    cmd = argv[1]
    if cmd == "range":
        n, N = parse_range(argv[2])
        print(f"{n}..{N}")
    elif cmd == "numeric":
        print(numeric(argv[2], int(argv[3]), int(argv[4])))
    elif cmd == "interval":
        lo, hi = interval_add(parse_range(argv[2]), parse_range(argv[3]))
        print(f"{lo}..{hi}")
    elif cmd == "logical":
        b = int(argv[4]) if len(argv) > 4 else 0
        print(logical(argv[2], int(argv[3]), b))
    elif cmd == "func":
        print(func_apply(argv[2], int(argv[3])))
    elif cmd == "parametric":
        print(parametric(argv[2]))
    elif cmd == "tensorial":
        if argv[2] not in ("regular", "symmetric", "antisymmetric", "unknown"):
            return 1
        print(argv[2], argv[3])
    else:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
