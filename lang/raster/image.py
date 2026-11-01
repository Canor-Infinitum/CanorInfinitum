"""Bitmap and PNG writers for RGBA8 pixels. No external image library."""

from __future__ import annotations

import struct
import zlib


def _crc(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def _chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", _crc(tag + data))


def bitmap(width: int, height: int, rgba: bytes) -> bytes:
    if width < 1 or height < 1 or len(rgba) != width * height * 4:
        raise ValueError("rgba size does not match width and height")
    row = width * 4
    pixels = bytearray()
    for y in range(height - 1, -1, -1):
        start = y * row
        row_bytes = rgba[start:start + row]
        for x in range(width):
            r, g, b, a = row_bytes[x * 4:(x + 1) * 4]
            pixels += bytes((b, g, r, a))
    header = struct.pack("<IIIHHIIIIII", 40, width, height, 1, 32, 0, len(pixels), 2835, 2835, 0, 0)
    file_size = 14 + len(header) + len(pixels)
    file_header = struct.pack("<2sIHHI", b"BM", file_size, 0, 0, 14 + len(header))
    return file_header + header + pixels


def _stored_zlib(raw: bytes) -> bytes:
    out = bytearray(b"\x78\x01")
    src = 0
    while True:
        take = min(65535, len(raw) - src)
        final = src + take >= len(raw)
        out.append(1 if final else 0)
        out += struct.pack("<HH", take, take ^ 0xFFFF)
        out += raw[src:src + take]
        src += take
        if final:
            break
    out += struct.pack(">I", zlib.adler32(raw) & 0xFFFFFFFF)
    return bytes(out)


def png(width: int, height: int, rgba: bytes) -> bytes:
    if width < 1 or height < 1 or len(rgba) != width * height * 4:
        raise ValueError("rgba size does not match width and height")
    raw = bytearray()
    row = width * 4
    for y in range(height):
        raw.append(0)
        raw += rgba[y * row:(y + 1) * row]
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + _chunk(b"IHDR", ihdr) + _chunk(b"IDAT", _stored_zlib(bytes(raw))) + _chunk(b"IEND", b"")
