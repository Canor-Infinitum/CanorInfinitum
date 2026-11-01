#!/usr/bin/env python3
"""Name generated Flow, MetaFlow, and Action lines from the two lexicons.

categories.json maps a word to original / general / distictive / specific /
arbitrary counts. original.json maps a word to a single value. A word is
kept from either file. Counts are attached only when that file has them.

Each simplex line is named in lexicon order, wrapping when the simplexes
outnumber the words. The compact and sanitized values already on the line
are kept.

DIM_* inference is a plug-in. This script does not guess those names.
Pass an object with infer(name, values, profile) -> "DIM_..." or None.
A Grok persona can be that object later; its profile is stored and not
called here.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

CLASSES = ("general", "distinctive", "specific", "arbitrary")
# The category file spells one class this way.
CLASS_KEYS = {
    "general": "general",
    "distinctive": "distictive",
    "specific": "specific",
    "arbitrary": "arbitrary",
}


class Lexeme:
    def __init__(self, name: str):
        self.name = name
        self.original = None
        self.general = None
        self.distinctive = None
        self.specific = None
        self.arbitrary = None

    def values(self) -> dict:
        out = {}
        if self.original is not None:
            out["original"] = self.original
        for kind in CLASSES:
            value = getattr(self, kind)
            if value is not None:
                out[kind] = value
        return out

    def suffix(self) -> str:
        parts = ["%s=%s" % (key, value) for key, value in self.values().items()]
        return " | ".join(parts)


def load_lexicon(categories_path: Path, original_path: Path) -> list[Lexeme]:
    categories = json.loads(categories_path.read_text(encoding="utf-8"))
    original = json.loads(original_path.read_text(encoding="utf-8"))
    order = list(original.keys())
    for name in categories:
        if name not in original:
            order.append(name)
    lexicon = []
    for name in order:
        item = Lexeme(name)
        if name in original and original[name] is not None:
            item.original = original[name]
        block = categories.get(name)
        if isinstance(block, dict):
            if "original" in block and block["original"] is not None:
                item.original = block["original"]
            for kind, key in CLASS_KEYS.items():
                if key in block and block[key] is not None:
                    setattr(item, kind, block[key])
        lexicon.append(item)
    return lexicon


HEADS = (
    "STATISTICS", "ANALYSIS", "QUANTUM", "DIFFEOMORPHIC",
    "MENTALITY", "AXIOMATIC", "SIMULATION", "SOLVED",
)
DOMAINS = (
    "NARRATIVE", "CAUSAL", "MODAL", "TEMPORAL", "ETHICAL", "LINGUISTIC",
    "GEOMETRIC", "HARMONIC", "FORENSIC", "HISTORIC", "CLINICAL", "ECONOMIC",
    "ECOLOGICAL", "RITUAL", "MYTHIC", "PEDAGOGIC", "LEGAL", "AESTHETIC",
    "DIALOGIC", "RHETORICAL", "PRAGMATIC", "SEMANTIC", "SYNTACTIC", "PHONETIC",
    "CARTOGRAPHIC", "TAXONOMIC", "PROBATIVE", "ABDUCTIVE", "INDUCTIVE",
    "DEDUCTIVE", "ANALOGICAL", "COUNTERFACTUAL", "NORMATIVE", "DESCRIPTIVE",
    "PROCEDURAL",
)
ASPECTS = (
    "SCOPE", "WITNESS", "RESIDUE", "MOTIVE",
    "SEQUENCE", "CONTRAST", "QUORUM", "CLOSURE",
)
# Longer stems win. A head has no aspect. A domain takes one.
STEMS = (
    ("diffeomorph", "DIFFEOMORPHIC"), ("manifold", "DIFFEOMORPHIC"),
    ("statistic", "STATISTICS"), ("analys", "ANALYSIS"),
    ("quantum", "QUANTUM"), ("axiom", "AXIOMATIC"),
    ("simul", "SIMULATION"), ("mental", "MENTALITY"), ("psych", "MENTALITY"),
    ("solv", "SOLVED"), ("narrat", "NARRATIVE"), ("story", "NARRATIVE"),
    ("caus", "CAUSAL"), ("modal", "MODAL"), ("tempor", "TEMPORAL"),
    ("chron", "TEMPORAL"), ("ethic", "ETHICAL"), ("moral", "ETHICAL"),
    ("lingu", "LINGUISTIC"), ("lexic", "LINGUISTIC"), ("geometr", "GEOMETRIC"),
    ("harmon", "HARMONIC"), ("forens", "FORENSIC"), ("histor", "HISTORIC"),
    ("clinic", "CLINICAL"), ("medic", "CLINICAL"), ("econom", "ECONOMIC"),
    ("ecolog", "ECOLOGICAL"), ("ritual", "RITUAL"), ("myth", "MYTHIC"),
    ("pedagog", "PEDAGOGIC"), ("teach", "PEDAGOGIC"), ("legal", "LEGAL"),
    ("aesthet", "AESTHETIC"), ("dialog", "DIALOGIC"), ("rhetor", "RHETORICAL"),
    ("pragmat", "PRAGMATIC"), ("semant", "SEMANTIC"), ("syntac", "SYNTACTIC"),
    ("phonet", "PHONETIC"), ("cartog", "CARTOGRAPHIC"), ("taxonom", "TAXONOMIC"),
    ("probat", "PROBATIVE"), ("abduct", "ABDUCTIVE"), ("induct", "INDUCTIVE"),
    ("deduct", "DEDUCTIVE"), ("analog", "ANALOGICAL"),
    ("counterfact", "COUNTERFACTUAL"), ("normat", "NORMATIVE"),
    ("descript", "DESCRIPTIVE"), ("procedur", "PROCEDURAL"),
)


def infer_aspect(values: dict) -> str:
    """The winning count class picks a pair of aspects. A weak winner, one
    that is under half the original count, takes the second of the pair."""
    scores = [values.get(kind) or 0 for kind in CLASSES]
    best = max(scores)
    original = values.get("original") or 0
    if best <= 0:
        return ASPECTS[7 if original else 0]
    which = scores.index(best)
    weak = original > 0 and best * 2 < original
    return ASPECTS[which * 2 + (1 if weak else 0)]


def infer_dim(name: str, values: dict, profile: dict | None = None) -> str:
    """Read the spelling first, then the counts. profile may name a persona;
    the reading itself does not change, so two personas agree on the map."""
    word = name.lower()
    for stem, label in STEMS:
        if stem in word:
            if label in HEADS:
                return "DIM_" + label
            return "DIM_%s_%s" % (label, infer_aspect(values))
    fold = (ord(word[0]) * 26 + ord(word[1])) if len(word) > 1 else ord(word[0]) * 26
    domain = DOMAINS[fold % len(DOMAINS)]
    return "DIM_%s_%s" % (domain, infer_aspect(values))


class DimInferer:
    """Assigns one DIM_* to a lexicon name. Deterministic across personas."""

    def infer(self, name: str, values: dict, profile: dict | None = None) -> str | None:
        return infer_dim(name, values, profile)


class GrokPersona:
    """A bot profile held for the next inference pass. Not called yet."""

    def __init__(self, persona: str, profile: dict):
        self.persona = persona
        self.profile = profile

    def infer(self, name: str, values: dict, profile: dict | None = None) -> str | None:
        raise NotImplementedError("DIM inference is reserved for the next pass")


def atom_value(key: str, rest: str) -> str:
    """Bare atom when there is one. A symmetric group when there are several.
    An Action equation is a quoted string so its (mod o) is not a second group."""
    if key.startswith("Action."):
        return '"%s"' % rest.replace('"', "")
    compact, sep, sanitized = rest.partition(" | ")
    atoms = [part.strip() for part in sanitized.split(",")] if sep else []
    atoms = [part for part in atoms if part]
    if len(atoms) <= 1:
        return atoms[0] if atoms else compact
    return "(%s)" % ", ".join([compact] + atoms)


def name_line(line: str, lexeme: Lexeme, inferer: DimInferer | None = None, profile: dict | None = None) -> str:
    text = line.rstrip("\n")
    key, sep, rest = text.partition(" : ")
    if not sep:
        rest = text
        key = lexeme.name
    value = atom_value(key, rest)
    if inferer is not None:
        dim = inferer.infer(lexeme.name, lexeme.values(), profile)
        if dim:
            value = "(%s, %s)" % (value, dim)
    return "#%s::%s:%s;\n" % (lexeme.name, key, value)


def load_original_names(path: Path) -> list[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return list(data.keys())


def apply_original_names(src: Path, names: list[str]) -> int:
    """Rewrite src in place. Each line takes the next original.json word, wrapping."""
    tmp = src.with_suffix(src.suffix + ".tmp")
    count = 0
    width = len(names)
    with src.open("r", encoding="utf-8", newline="\n") as inp, tmp.open("wb", buffering=8 * 1024 * 1024) as out:
        batch = []
        for line in inp:
            if not line.strip():
                continue
            text = line.rstrip("\n")
            key, sep, rest = text.partition(" : ")
            if not sep:
                batch.append(line.encode("utf-8"))
            else:
                value = atom_value(key, rest)
                batch.append(("#%s::%s:%s;\n" % (names[count % width], key, value)).encode("utf-8"))
            count += 1
            if len(batch) >= 100000:
                out.write(b"".join(batch))
                batch.clear()
        if batch:
            out.write(b"".join(batch))
    tmp.replace(src)
    return count


def name_file(src: Path, dst: Path, lexicon: list[Lexeme], limit: int, inferer: DimInferer | None, profile: dict | None) -> int:
    count = 0
    with src.open("r", encoding="ascii", newline="\n") as inp, dst.open("wb", buffering=8 * 1024 * 1024) as out:
        batch = []
        for line in inp:
            if not line.strip():
                continue
            batch.append(name_line(line, lexicon[count % len(lexicon)], inferer, profile).encode("ascii"))
            count += 1
            if len(batch) >= 200000:
                out.write(b"".join(batch))
                batch.clear()
            if limit and count >= limit:
                break
        if batch:
            out.write(b"".join(batch))
    return count


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    base = root / "CanorDatabasis"
    parser = argparse.ArgumentParser()
    parser.add_argument("--categories", type=Path, default=base / "categories.json")
    parser.add_argument("--original", type=Path, default=base / "original.json")
    parser.add_argument("--src", type=Path, default=base)
    parser.add_argument("--out", type=Path, default=None)
    parser.add_argument("--limit", type=int, default=5)
    parser.add_argument("--map", type=Path, default=None)
    parser.add_argument("--apply-original", action="store_true")
    args = parser.parse_args()
    if args.apply_original:
        names = load_original_names(args.original)
        print("original %d" % len(names))
        for kind in ("flow", "action", "metaflow"):
            src = args.src / ("%s.fwh" % kind)
            count = apply_original_names(src, names)
            print("named %d %s" % (count, src))
        return
    lexicon = load_lexicon(args.categories, args.original)
    inferer = DimInferer()
    print("lexicon %d" % len(lexicon))
    print("first %s %s %s" % (lexicon[0].name, lexicon[0].values(), inferer.infer(lexicon[0].name, lexicon[0].values())))
    if args.map is not None:
        ranked = sorted(lexicon, key=lambda item: (inferer.infer(item.name, item.values()), item.name))
        args.map.parent.mkdir(parents=True, exist_ok=True)
        with args.map.open("wb", buffering=8 * 1024 * 1024) as out:
            batch = []
            for item in ranked:
                dim = inferer.infer(item.name, item.values())
                values = ", ".join("%s=%s" % pair for pair in item.values().items())
                body = "(%s)" % values if values else dim
                batch.append(("#%s::%s:%s;\n" % (item.name, dim, body)).encode("utf-8"))
                if len(batch) >= 200000:
                    out.write(b"".join(batch))
                    batch.clear()
            if batch:
                out.write(b"".join(batch))
        print("wrote %d %s" % (len(ranked), args.map))
    for kind in ("flow", "metaflow", "action"):
        src = args.src / ("%s.fwh" % kind)
        if not src.exists():
            print("missing %s" % src)
            continue
        if args.out is None:
            with src.open("r", encoding="ascii", newline="\n") as inp:
                for i, line in enumerate(inp):
                    if i >= args.limit:
                        break
                    print(name_line(line, lexicon[i % len(lexicon)]).rstrip())
            continue
        dst = args.out / ("%s.fwh" % kind)
        dst.parent.mkdir(parents=True, exist_ok=True)
        count = name_file(src, dst, lexicon, args.limit, None, None)
        print("wrote %d %s" % (count, dst))


if __name__ == "__main__":
    main()
