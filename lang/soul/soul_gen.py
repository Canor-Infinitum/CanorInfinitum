#!/usr/bin/env python3
"""Soul cornerstone text and one 64x64 four-channel mip.

A cornerstone is a strategy plus a vector. No Flow and no MetaFlow.
No Action: one coverage mask, 255 on every channel.
An Action result times --scale, clamped to 0..255, replaces that copy.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "raster"))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import image
import range as scale_range
import place

MIP = 64
SRC = 20
NAMES = [f"CS{i:02d}" for i in range(1, 27)]
HERE = Path(__file__).resolve().parent
DEFAULT_DIR = HERE / "default"
RECT = re.compile(r"M(-?\d+) (-?\d+)h(-?\d+)v(-?\d+)h(-?\d+)z")
# 8x8 glyphs, bit 7 is the left pixel. Face name: canor.
GLYPHS = {
    " ": [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    "H": [0x81, 0x81, 0x81, 0xFF, 0x81, 0x81, 0x81, 0x81],
    "I": [0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E],
}


def check_name(name: str) -> str:
    if not name or not (name[0].isalpha() or name[0] == "_"):
        raise ValueError(f"name {name!r} must start with a letter or underscore")
    for ch in name:
        if not (ch.isalnum() or ch == "_"):
            raise ValueError(f"name {name!r} must be letters, digits, or underscore")
    return name


DIRECTIONS = (("x", True, "x.asc"), ("x", False, "x.desc"), ("y", True, "y.asc"), ("y", False, "y.desc"))
COS1024 = (
    1024, 1024, 1023, 1023, 1022, 1020, 1018, 1016, 1014, 1011, 1008, 1005, 1002, 998, 994, 989, 984, 979, 974, 968,
    962, 956, 949, 943, 935, 928, 920, 912, 904, 896, 887, 878, 868, 859, 849, 839, 828, 818, 807, 796, 784, 773,
    761, 749, 737, 724, 711, 698, 685, 672, 658, 644, 630, 616, 602, 587, 573, 558, 543, 527, 512, 496, 481, 465,
    449, 433, 416, 400, 384, 367, 350, 333, 316, 299, 282, 265, 248, 230, 213, 195, 178, 160, 143, 125, 107, 89, 71, 54, 36, 18, 0,
)


def norm_degrees(degrees: int) -> int:
    return degrees % 360


def cos1024(degrees: int) -> int:
    d = norm_degrees(degrees)
    if d <= 90:
        return COS1024[d]
    if d <= 180:
        return -COS1024[180 - d]
    if d <= 270:
        return -COS1024[d - 180]
    return COS1024[360 - d]


def sin1024(degrees: int) -> int:
    return cos1024(90 - degrees)


def header_text() -> str:
    listed = ",".join(NAMES)
    return f'&soul::Header:{{\n    $cornerstones:"{listed}";\n}};\n'


def parse_degrees(text: str | None) -> list[int]:
    if text is None or text == "":
        return []
    parts = [part.strip() for part in text.split(",") if part.strip()]
    if len(parts) == 1:
        values = [int(parts[0])]
    elif len(parts) == 2:
        start, stop = int(parts[0]), int(parts[1])
        if start > stop:
            raise ValueError("degree range must run from the smaller angle to the larger")
        values = list(range(start, stop + 1))
    else:
        raise ValueError("degrees must be N or START,STOP")
    for value in values:
        if value < 0 or value > 360:
            raise ValueError("degrees must be in 0..360")
    return values


def draw_spec(svg_text: str) -> str:
    return svg_text.split('d="', 1)[1].split('"', 1)[0]


def components(mask: bytearray) -> list[list[tuple[int, int]]]:
    seen = bytearray(SRC * SRC)
    found = []
    for index in range(SRC * SRC):
        if not mask[index] or seen[index]:
            continue
        stack = [index]
        seen[index] = 1
        cells = []
        while stack:
            here = stack.pop()
            x, y = here % SRC, here // SRC
            cells.append((x, y))
            for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                if not (0 <= nx < SRC and 0 <= ny < SRC):
                    continue
                nxt = ny * SRC + nx
                if mask[nxt] and not seen[nxt]:
                    seen[nxt] = 1
                    stack.append(nxt)
        found.append(cells)
    return found


def _center(groups: list[list[tuple[int, int]]]) -> tuple[int, int, int]:
    sums = []
    for cells in groups:
        sums.append((sum(x for x, _y in cells), sum(y for _x, y in cells), len(cells)))
    (sx0, sy0, n0), (sx1, sy1, n1) = sums
    return sx0 * n1 + sx1 * n0, sy0 * n1 + sy1 * n0, 2 * n0 * n1


def directional_mip(svg_text: str, axis: str, ascending: bool, size: int = MIP) -> bytes:
    mask = svg_mask(svg_text)
    groups = components(mask)
    toward = len(groups) == 2
    center = _center(groups) if toward else (0, 0, 1)
    axis_i = 0 if axis == "x" else 1
    num, den = (center[axis_i], center[2]) if toward else (0, 1)
    cells = [cell for group in groups for cell in group]
    dists = [abs(cell[axis_i] * den - num) for cell in cells] if toward else [0]
    max_d = max(dists) if dists else 1
    if max_d == 0:
        max_d = 1
    level = {}
    for (x, y), dist in zip(cells, dists):
        if toward:
            span = (max_d - dist) if ascending else dist
            level[(x, y)] = span * 252 // max_d
        else:
            level[(x, y)] = 0
    out = bytearray(size * size * 4)
    for y in range(size):
        sy = y * SRC // size
        for x in range(size):
            sx = x * SRC // size
            if not mask[sy * SRC + sx]:
                continue
            if toward:
                value = level[(sx, sy)]
            else:
                coord = x if axis == "x" else y
                value = (coord if ascending else (size - 1 - coord)) * 4
            pixel = (y * size + x) * 4
            out[pixel:pixel + 3] = bytes((value, value, value))
            out[pixel + 3] = 255
    return bytes(out)


def degree_mip(svg_text: str, degrees: int, size: int = MIP) -> bytes:
    degrees = norm_degrees(degrees)
    mask = svg_mask(svg_text)
    groups = components(mask)
    cosine, sine = cos1024(degrees), sin1024(degrees)
    out = bytearray(size * size * 4)
    if len(groups) == 2:
        (sx0, sy0, n0), (sx1, sy1, n1) = (
            (sum(x for x, _y in group), sum(y for _x, y in group), len(group)) for group in groups
        )
        den = 2 * n0 * n1
        numx = sx0 * n1 + sx1 * n0
        numy = sy0 * n1 + sy1 * n0
        center = numx * cosine + numy * sine
        dists = []
        for group in groups:
            for x, y in group:
                dist = (x * cosine + y * sine) * den - center
                dists.append(abs(dist))
        max_d = max(dists) or 1
        level = {}
        for group in groups:
            for x, y in group:
                dist = abs((x * cosine + y * sine) * den - center)
                level[(x, y)] = (max_d - dist) * 252 // max_d
        for y in range(size):
            sy = y * SRC // size
            for x in range(size):
                sx = x * SRC // size
                if not mask[sy * SRC + sx]:
                    continue
                value = level[(sx, sy)]
                pixel = (y * size + x) * 4
                out[pixel:pixel + 3] = bytes((value, value, value))
                out[pixel + 3] = 255
        return bytes(out)
    covered = []
    for y in range(size):
        sy = y * SRC // size
        for x in range(size):
            if mask[sy * SRC + x * SRC // size]:
                covered.append((x, y, x * cosine + y * sine))
    if not covered:
        return bytes(out)
    low = min(item[2] for item in covered)
    high = max(item[2] for item in covered)
    span = high - low or 1
    for x, y, proj in covered:
        value = (proj - low) * 252 // span
        pixel = (y * size + x) * 4
        out[pixel:pixel + 3] = bytes((value, value, value))
        out[pixel + 3] = 255
    return bytes(out)


def linear_atom(name: str, svg_text: str, label: int | None = None) -> str:
    head = name if label is None else f"{name}.d{label}"
    templates = "linear" if label is None else f"linear,{label}"
    return (
        f"&{head}::Soul:{{\n"
        f'    &{name}.linear::Strategy(bases="x,y", templates="{templates}"):{{\n'
        "        $d.x::linear:( %fn::(x); );\n"
        "        $d.y::linear:( %fn::(y); );\n"
        "    };\n"
        "    %fn::(x): x;\n"
        "    %fn::(y): y;\n"
        f'    $vector:"{draw_spec(svg_text)}";\n'
        "};"
    )


def store_picture(folder: Path, kind: str, stem: str, pixels: bytes, edge: int) -> None:
    primary = folder / "cornerstones" / kind
    backup = folder / "cornerstones" / ".backup" / kind
    primary.mkdir(parents=True, exist_ok=True)
    backup.mkdir(parents=True, exist_ok=True)
    (primary / f"{stem}.mip").write_bytes(pixels)
    (primary / f"{stem}.png").write_bytes(image.png(edge, edge, pixels))
    (backup / f"{stem}.bmp").write_bytes(image.bitmap(edge, edge, pixels))


def stream_counts(svg_text: str) -> str:
    """Count-stream of the draw, in path order. Each painted cell is one step."""
    d = svg_text.split('d="', 1)[1].split('"', 1)[0]
    seen: dict[tuple[int, int], int] = {}
    lines = []
    step = 0
    for x, y, w, h, _back in RECT.findall(d):
        x, y, w, h = int(x), int(y), int(w), int(h)
        if w < 0:
            x += w
            w = -w
        if h < 0:
            y += h
            h = -h
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                if not (0 <= xx < SRC and 0 <= yy < SRC):
                    continue
                seen[(xx, yy)] = seen.get((xx, yy), 0) + 1
                lines.append(f"{step} {seen[(xx, yy)]}")
                step += 1
    return "\n".join(lines) + ("\n" if lines else "")


def write_soul(folder: Path, svg_dir: Path = DEFAULT_DIR, mip_edge: int = MIP, action: str | None = None, scale: int = 1, degrees: list[int] | None = None) -> None:
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "cornerstones" / "default").mkdir(parents=True, exist_ok=True)
    atoms = []
    chosen = degrees or []
    for name in NAMES:
        source = svg_dir / f"{name}.svg"
        svg_text = source.read_text(encoding="utf-8")
        (folder / "cornerstones" / "default" / f"{name}.svg").write_bytes(source.read_bytes())
        stream = folder / "cornerstones" / "streamcount"
        stream.mkdir(parents=True, exist_ok=True)
        (stream / f"{name}.stream").write_text(stream_counts(svg_text), encoding="utf-8", newline="\n")
        store_picture(folder, "default", name, cornerstone_mip(source), MIP)
        if action is not None or scale != 1 or mip_edge != MIP:
            store_picture(folder, "generated/linear", name, cornerstone_mip(source, action, scale, mip_edge), mip_edge)
        else:
            store_picture(folder, "generated/linear", name, degree_mip(svg_text, 0, MIP), MIP)
        atoms.append(linear_atom(name, svg_text))
        for label in chosen:
            store_picture(folder, f"generated/linear/d{label}", name, degree_mip(svg_text, norm_degrees(label), mip_edge), mip_edge)
            atoms.append(linear_atom(name, svg_text, label))
    (folder / "soul.cah").write_text(header_text(), encoding="utf-8", newline="\n")
    (folder / "soul.cab").write_text("\n".join(atoms) + "\n", encoding="utf-8", newline="\n")


def svg_mask(text: str) -> bytearray:
    mask = bytearray(SRC * SRC)
    d = text.split('d="', 1)[1].split('"', 1)[0]
    for x, y, w, h, _back in RECT.findall(d):
        x, y, w, h = int(x), int(y), int(w), int(h)
        if w < 0:
            x += w
            w = -w
        if h < 0:
            y += h
            h = -h
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                if 0 <= xx < SRC and 0 <= yy < SRC:
                    mask[yy * SRC + xx] = 1
    return mask


def fit_mask(mask: bytearray, src: int, size: int) -> bytearray:
    out = bytearray(size * size)
    for y in range(size):
        sy = y * src // size
        for x in range(size):
            sx = x * src // size
            out[y * size + x] = mask[sy * src + sx]
    return out


def glyph_mask(text: str) -> bytearray:
    width = 8 * len(text)
    height = 8
    mask = bytearray(width * height)
    for i, ch in enumerate(text):
        if ch not in GLYPHS:
            raise ValueError(f"canor face has no glyph {ch!r}")
        for row, bits in enumerate(GLYPHS[ch]):
            for col in range(8):
                if bits & (0x80 >> col):
                    mask[row * width + i * 8 + col] = 1
    return mask


def action_byte(spec: str | None, x: int, y: int, channel: int, scale: int) -> int:
    if spec is None:
        return 255
    if spec == "m = n (mod o)":
        result = 1 if (x % channel) == (y % channel) else 0
    elif spec.startswith("(mod ") and spec.endswith(")"):
        mod = int(spec[5:-1])
        if mod < 1:
            raise ValueError("modulus must be positive")
        result = (x + y) % mod
    else:
        raise ValueError(f"unsupported action {spec!r}")
    value = result * scale
    return 255 if value > 255 else value


def raster(mask: bytearray, size: int, action: str | None, scale: int) -> bytes:
    out = bytearray(size * size * 4)
    for y in range(size):
        for x in range(size):
            if not mask[y * size + x]:
                continue
            for c in range(4):
                out[(y * size + x) * 4 + c] = action_byte(action, x, y, c + 1, scale)
    return bytes(out)


def cornerstone_mip(svg_path: Path, action: str | None = None, scale: int = 1, size: int = MIP) -> bytes:
    fitted = fit_mask(svg_mask(svg_path.read_text(encoding="utf-8")), SRC, size)
    return raster(fitted, size, action, scale)


def font_mip(text: str, action: str | None = None, scale: int = 1, size: int = MIP) -> bytes:
    raw = glyph_mask(text)
    width = 8 * len(text)
    side = max(8, width)
    square = bytearray(side * side)
    for y in range(8):
        for x in range(width):
            square[y * side + x] = raw[y * width + x]
    return raster(fit_mask(square, side, size), size, action, scale)


def hilbert(order_side: int, index: int) -> tuple[int, int]:
    x = y = 0
    s = 1
    d = index
    while s < order_side:
        rx = 1 & (d // 2)
        ry = 1 & (d ^ rx)
        if ry == 0:
            if rx == 1:
                x, y = s - 1 - x, s - 1 - y
            x, y = y, x
        x += s * rx
        y += s * ry
        d //= 4
        s *= 2
    return x, y


def compose_mip(parts: list[bytes], cell: int = MIP) -> bytes:
    n = 1
    while n * n < len(parts):
        n *= 2
    side = n * cell
    out = bytearray(side * side * 4)
    for i, mip in enumerate(parts):
        cx, cy = hilbert(n, i)
        for y in range(cell):
            for x in range(cell):
                src = (y * cell + x) * 4
                dst = ((cy * cell + y) * side + (cx * cell + x)) * 4
                out[dst:dst + 4] = mip[src:src + 4]
    return bytes(out)


def self_test() -> None:
    assert header_text().startswith('&soul::Header:{')
    svg_text = (DEFAULT_DIR / "CS01.svg").read_text(encoding="utf-8")
    sample = cornerstone_mip(DEFAULT_DIR / "CS01.svg")
    assert "CS01,CS02," in header_text() and "x.asc" not in header_text()
    atom = linear_atom("CS01", svg_text)
    assert '&CS01.linear::Strategy(bases="x,y", templates="linear")' in atom
    assert "%fn::(x): x;" in atom and "%fn::(y): y;" in atom
    assert linear_atom("CS01", svg_text, 360).startswith("&CS01.d360::Soul:")
    assert parse_degrees("0,360")[0] == 0 and parse_degrees("0,360")[-1] == 360
    assert parse_degrees("45") == [45]
    paired = directional_mip((DEFAULT_DIR / "CS23.svg").read_text(encoding="utf-8"), "x", True)
    def at(sx: int, sy: int) -> int:
        x = next(i for i in range(MIP) if i * SRC // MIP == sx)
        y = next(i for i in range(MIP) if i * SRC // MIP == sy)
        return paired[(y * MIP + x) * 4]
    assert at(11, 3) > at(9, 0)
    assert at(16, 7) > at(18, 10)
    turned = degree_mip(svg_text, 0)
    assert turned[(30 * MIP + 60) * 4] > turned[(30 * MIP + 2) * 4]
    assert "templates=\"linear,45\"" in linear_atom("CS01", svg_text, 45)
    mip = cornerstone_mip(DEFAULT_DIR / "CS01.svg")
    assert len(mip) == MIP * MIP * 4
    # CS01 paints source rows 9 and 10. Dest row 30 samples source row 9.
    offset = (30 * MIP + 0) * 4
    assert mip[offset:offset + 4] == b"\xff\xff\xff\xff"
    assert mip[0:4] == b"\x00\x00\x00\x00"
    scaled = cornerstone_mip(DEFAULT_DIR / "CS01.svg", "m = n (mod o)", 10)
    assert scaled[offset] == 10  # x=0,y=30, channel 1: 0 % 1 == 30 % 1
    assert scaled[offset + 1] in (0, 10)
    modded = cornerstone_mip(DEFAULT_DIR / "CS01.svg", "(mod 255)", 1)
    assert modded[offset] == (0 + 30) % 255
    glyph = font_mip("HI")
    assert len(glyph) == MIP * MIP * 4
    assert any(glyph)
    composed = compose_mip([mip, mip])
    assert len(composed) == 128 * 128 * 4
    assert scale_range.fold(None, 1, 1, 255) == (1, 0)
    assert scale_range.fold(6, 1, 1, 4) == (2, 1)
    blob = image.bitmap(1, 1, bytes((1, 2, 3, 4)))
    assert blob.startswith(b"BM")
    picture = image.png(1, 1, bytes((1, 2, 3, 4)))
    assert picture.startswith(b"\x89PNG\r\n\x1a\n")


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Write Soul cornerstone text and a 64x64 mip.")
    p.add_argument("--dir", default=None, help="output directory; default is the shared place rule")
    p.add_argument("--compose", help="comma-separated cornerstone names")
    p.add_argument("--font", help="monospaced face; this slice provides canor")
    p.add_argument("--text", help="glyph string for --font")
    p.add_argument("--name", default="glyph")
    p.add_argument("--strategy", default=None, help="strategy header; default master.strategy for a font glyph")
    p.add_argument("--mip", type=int, default=MIP, help="square mip edge, profile default 64")
    p.add_argument("--scale", type=int, default=None, help="folded into scale_min..scale_max; default is scale_nom")
    p.add_argument("--scale-min", type=int, default=1)
    p.add_argument("--scale-nom", type=int, default=1)
    p.add_argument("--scale-max", type=int, default=255)
    p.add_argument("--action", default=None, help='omit for one mask; or "m = n (mod o)" or "(mod N)"')
    p.add_argument("--degrees", default=None, help="direction N, or START,STOP, in 0..360")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args(argv)
    if args.self_test:
        self_test()
        print("soul_gen self-test ok")
        return 0
    if args.mip < 1:
        raise ValueError("mip edge must be positive")
    scale, _quot = scale_range.fold(args.scale, args.scale_min, args.scale_nom, args.scale_max)
    folder = place.place_dir(place.family_dir(__file__), "soul", args.dir)
    folder.mkdir(parents=True, exist_ok=True)

    if args.font or args.text or args.compose:
        if args.font and args.font != "canor":
            raise ValueError("face must be canor")
        if args.compose:
            names = [part.strip() for part in args.compose.split(",") if part.strip()]
            mips = [cornerstone_mip(DEFAULT_DIR / f"{name}.svg", args.action, scale, args.mip) for name in names]
            tiled = compose_mip(mips, args.mip)
            side = 1
            while side * side < len(mips):
                side *= 2
            store_picture(folder, "compositions", "compose", tiled, side * args.mip)
        if args.text:
            check_name(args.name)
            strategy = args.strategy or "master.strategy"
            atom = (
                f"&{args.name}::Soul(scale={scale}):{{\n"
                f"    &{strategy}::Strategy;\n"
                f'    $vector:"{args.name}.mask";\n'
                "};\n"
            )
            (folder / f"{args.name}.cab").write_text(atom, encoding="utf-8", newline="\n")
            store_picture(folder, "generated", args.name, font_mip(args.text, args.action, scale, args.mip), args.mip)
    write_soul(folder, action=args.action, scale=scale, mip_edge=args.mip, degrees=parse_degrees(args.degrees))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ValueError as exc:
        print(f"soul_gen: {exc}", file=sys.stderr)
        raise SystemExit(1)
