"""Where a lang tool writes its files.

Try ``../../.repositorium/NAME`` from the current directory when that
directory already exists. Otherwise ``../generated/NAME``. Otherwise
``FAMILY/generated/NAME``. An existing NAME becomes NAME_1, NAME_2, ...
``--dir`` is used as given.
"""

from __future__ import annotations

import os
from pathlib import Path


def _claim(parent: Path, name: str) -> Path:
    parent.mkdir(parents=True, exist_ok=True)
    candidate = parent / name
    if not candidate.exists():
        candidate.mkdir()
        return candidate
    index = 1
    while True:
        numbered = parent / f"{name}_{index}"
        if not numbered.exists():
            numbered.mkdir()
            return numbered
        index += 1


def place_dir(family: Path, name: str, override: str | None = None, cwd: Path | None = None) -> Path:
    if override:
        chosen = Path(override)
        if not chosen.is_absolute():
            chosen = Path.cwd() / chosen
        chosen = chosen.resolve()
        chosen.mkdir(parents=True, exist_ok=True)
        return chosen
    del cwd
    base = family.resolve()
    repositorium = (base / ".." / ".." / ".repositorium").resolve()
    if repositorium.is_dir():
        return _claim(repositorium, name)
    try:
        return _claim((base / ".." / "generated").resolve(), name)
    except OSError:
        pass
    return _claim((base / "generated").resolve(), name)


def family_dir(file: str) -> Path:
    return Path(file).resolve().parent
