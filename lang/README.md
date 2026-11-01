# CanorLingua

Path: `./lang/`

Reactive, quantum-symbolic, physical multi-meta-programming language suite: ten core languages, an object interchange, an intermediate representation, and plug-and-play linkage for external complementary languages.

Every instantiated block integrates the Aether temporal timing suite. Evaluation minimizes ε-error and maximizes Chaitin’s ω.

## Languages

1. **Strategy™ (Differential Imaginary Bases & Projections)**
    - **Header / Scope:** Differential Imaginary Bases hardcoded or derived through higher-dimensional structural extraction.
    - **Source / Mechanics:** Utilizes **Cayley–Dickson constructions** to project irreducible non-associative 16-dimensional Sedenionic spaces into compact, associative Quaternionic Projective caches.
2. **Flow™ (Quaternionic Atom Sequences)**
    - **Header:** Trinities of Hamilton's Quaternionic Trifectas (i.e. such as imaginary bases i, j, k with ring indices e.g. i_0, j_1, k_0).
    - **Source:** Immutable ordered strings of both LTR- and RTL-ordered **ATOMs**, utilizing left- and right-handed subscripts (2x: `_` or `.`) and superscripts (2x: `^`, `'`), strictly adhering to the `atoms.ebnf` grammar while preserving whitespace as lexical metadata.
3. **MetaFlow™ (4-Slot Bounded Extension)**
    - **Header:** Bounded Labeled ATOMs with default slot labels R **(Reals)**, G **(Gaps)**, B **(Bases)**, and A **(Auxiliaries)**.
    - **Source:** Bounded sequence expressions (maximum 4 slots) of LTR- and RTL-ordered Labeled ATOMs with left/right subscripting and superscripting, used to describe complex-fractal strings and function compositions.
4. **Action™ (Dimensional Constants & Modular Constraints)**
    - **Header:** Integer-only Dimensional Constant definitions with profile-dependent grammar (e.g., `DIM_ANALYTICAL := 2;`).
    - **Source:** Modulation expressions or equations based upon header definitions (e.g., `... = ... (mod DIM_ANALYTICAL);`). Applies typed modular constraints (L = R (mod N)) using **Integer Programming with Interval Arithmetic** (LO/HI bounds) to model dataflow uncertainty.
5. **Soul™ (Pixel-Space Diffusion & Symbol Composition)**
    - **Header:** Hilbert-ordering Pixel-Space Diffusion Transformers working within Quadtree structures, composing Flow, MetaFlow, and Action.
    - **Source:** Pixel-Space Identity-, Contextual-, Versioning-, and Checksum-Embeddings across 4-channel Visual MipMaps. Generates n+1 cumulative count vectors for a Flow of length n across mathematical permutations (LTR, RTL, MLO, MRO).
6. **Maneuver™ (Functional Binding & Symbolic Benchmarking)**
    - **Header:** Functional Soul: Elaborations (functional expressions) or Evaluations (symbolic equations) utilizing Bra-ket tensor notation and 1-to-1, 1-to-many, or many-to-1 junction arities.
    - **Source:** Benchmarking Soul using 27-element 3-tuple symbolic interval-valued timing logic (PRE/PERI/POST × MIN/NOM/MAX × LOW/VALUE/HIGH × LO/HI).
7. **United™ Interchange (Meta-Schema & Stochastic Processes)**
    - **Header:** Unifies earlier grammars into a logic- and numeric interval-arithmetic Meta-Schema carrying **JSON-Notation** and **XML-Markup** to eliminate ambiguity (e.g., `<flow>iiikji</flow><logic>xor</logic><action>(mod N)</action>`).
    - **Source:** Enables all underlying symbolic structures and schemas to operate within **Markov-Chain Monte-Carlo (MCMC)** methods, including infinitely recursive or reflexive Markov processes.
8. **Elite™ Programming (Agent-Guided Rewrite & CAD Logic)**
    - **Header:** 3-tuple symbolic interval-valued timing logic (PRE/PERI/POST × MIN/NOM/MAX × LOW/VALUE/HIGH × LO/HI) with profile-dependent grammar executing within CAD environments - be that Classical, Semi-Classical, Hybrid, Neuromorphic, or Quantum Circuit designs.
    - **Source:** Procedural assembly layer and symbolic rewrite engine ( $ : mov | jmp | cmp ) maintaining branch traces up to configured limits using AI/SI agents, LLMs, or stochastic modules.
9. **Follower™ / Carmen™ Interlingua (Complementary Language Linkage)**
    - **Header:** Interoperability package management (`.cap` profiles and `.cas` settings) for external Complementary Languages (e.g., Prolog, C, Python, WASM, FPC/Pascal).
    - **Source:** Asynchronous runtime dispatch and compilation engine for external code blocks defined via pipe-dot (`| ... .`) boundary framing.
10. **Ether™ / Aether™ IR (Intermediate Representation)**
    - **Header & Source:** Integer-only binary tree structure stored as compiled objects (`.cao`) or `*.error`-files. Embeds the 27x element cubic timing field, and LO/HI interval bounds to regulate update rates and error mitigation.

Shared types live in [`../core/CONTEXT.md`](../core/CONTEXT.md). Suite map: [`../README.md`](../README.md).

Every generator writes beside its own program, not the shell's current directory. It uses `../../.repositorium/NAME` when that directory already exists. Otherwise it creates `../generated/NAME`. If that cannot be created, it uses `./generated/NAME`. An existing `NAME` becomes `NAME_1`, `NAME_2`, and so on. `--dir` selects the directory itself. `--name` is `NAME`. `--stdout` prints as well.

## Symbolic generator

[`flow/symb_gen.py`](flow/symb_gen.py) and [`flow/symb_gen.c`](flow/symb_gen.c) write a `.cah` / `.cas` pair for Flow or MetaFlow. The header is one atom, `@Name::Flow.order.index.count:{ $bases:{ ... }; $direction:"LTR"; $symbolic:"..."; $cardinality:N; };`. The body stays one atom per word. `--flow` is the default (bases `i j k l m n o p q r s t e f g`, atom kind `Flow`). `--metaflow` selects bases `R, G, B, A` and atom kind `MetaFlow`. Order is 1 through 288. `--symbolic` is eight integer expressions: LTR prime, dot, caret, underscore, then RTL underscore, caret, dot, prime. A channel is omitted when it is 0. The default expression is all zeros, which leaves the atoms bare. `P D C U` are the previous atom's LTR channels. `RP RD RC RU` are its RTL channels. `k` and `c` appear only as `k % m` and `c % m`. The word session in [`flow/flow_gen.c`](flow/flow_gen.c) allocates nothing and does not touch files.

## Strategy generator

[`strategy/strategy_gen.py`](strategy/strategy_gen.py) and [`strategy/strategy_gen.c`](strategy/strategy_gen.c) write one `.cas` file. The default name is `default`. Each differential child stores an unevaluated chain, `$d.i::diff:( %fn_1::(...); );`. The same file then defines each `%fn_N::(...);` it uses. `--templates` defaults to `diff`. Each listed template is used once per basis, and the header records `templates="..."`. With no `--bases`, the alphabet is the Flow bases followed by the MetaFlow bases `R, G, B, A`. `--flow` or `--metaflow` alone selects that alphabet. `--bases` replaces it. The flow or metaflow result is implied by the child that uses the strategy, not written as a yield. `strategy_format` writes into a caller buffer and does not allocate; the host `main` writes the file.

## Language defaults

[`lang_gen.py`](lang_gen.py) and [`lang_gen.c`](lang_gen.c) write the project files by default, the same set as `--default`: `project_name.cap` (profile), `project_name.cas` (settings), `main.cah` (header), and `main.cab` (body). For `--name TEST`, `main.cah` is `&TEST.main::Header:{ &master.strategy::Strategy; };`. `main.cab` is `&TEST.main::Body` and holds `&master.strategy::Strategy` with both alphabets and the default templates. `project_name.cap` and `project_name.cas` each declare `\Profile`, `\Settings`, `\Strategy`, `\Header`, `\Body`, `\Flow`, and `\MetaFlow`. The file list is only in the settings atom. The generator also writes `strategy.cas`, which holds the default strategy and its own functional definitions. `--strategy NAME` writes another `NAME.cas` with its own functional definitions. It also writes `flows.cah` / `flows.cab` (order-3 Flow, count all) and `metaflows.cah` / `metaflows.cab` (order-4 MetaFlow, count all). `--language strategy`, `flow`, `metaflow`, `action`, or `soul` writes that language's default files. `--default` writes the project files and all five of those languages, including the 26 Soul cornerstones. `--file PATH` writes an empty atom of that type, `&stem::Profile`, `&stem::Settings`, `&stem::Header`, or `&stem::Body`, and with `--default` the settings list names it. The settings name `project_name.cao` as the future object. This generator does not compile it.

## Action generator

[`action/action_gen.py`](action/action_gen.py) and [`action/action_gen.c`](action/action_gen.c) write one `.cas` atom. The default is `&default::Action(m, n, o):"m = n (mod o)";`. `--m`, `--n`, and `--o` bind those parameters to integers in `1..288`.

## Soul generator

[`soul/soul_gen.py`](soul/soul_gen.py) and [`soul/soul_gen.c`](soul/soul_gen.c) write `soul.cah` and `soul.cab` in the output directory. `cornerstones/default` holds each source `.svg` and its plain `.mip` and `.png`, with no strategy and no direction. The default strategy is `linear`, `%fn::(x): x;` and `%fn::(y): y;`, written to `cornerstones/generated/linear`. `--degrees N` or `--degrees START,STOP` adds `cornerstones/generated/linear/dN` for each angle in `0..360`; `360` is kept as a name and drawn as `0`. `CS23` through `CS26` have two strokes, and both are ordered toward the center between them. `--compose` goes to `cornerstones/compositions`. Bitmap files are only the redundancy copy, under `cornerstones/.backup/` in the same three names. The originals stay in [`soul/default`](soul/default). Each atom is that strategy plus `$vector`, the SVG draw specification. A `.mip` file is raw 64×64 RGBA with no image header, so open the `.png` or `.bmp` beside it. There is no Flow and no MetaFlow. Python also writes a 64×64 four-channel `NAME.mip`. With no `--action`, a covered cell is `255` on every channel. `--action` is `m = n (mod o)` or `(mod N)`; the channel byte is that result times `--scale`, clamped to `0..255`. `--font canor --text STRING` draws a from-scratch glyph with `&master.strategy::Strategy`. `--compose CS01,CS05` tiles those mips in Hilbert order. `--mip` defaults to 64, the Soul setting in the project profile. `--scale` folds into `$scale_min`, `$scale_nom`, and `$scale_max` by `MIN + (value - MIN) mod (MAX - MIN + 1)`; an omitted scale is NOM. [`raster/image.py`](raster/image.py) and [`raster/raster.c`](raster/raster.c) write the same pixels as a 32-bit Bitmap and an RGBA PNG.
