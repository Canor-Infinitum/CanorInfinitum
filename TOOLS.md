# TOOLS.md: Toolchains

| Tool | Purpose | Min version | Check |
|---|---|---|---|
| GCC or Clang | C11 core | TODO | `cc --version` |
| CMake / Ninja | Host build | TODO | `cmake --version` |
| Zephyr SDK + `west` | RTOS builds (ARM Cortex-M/AArch32 via `arm-zephyr-eabi`, RISC-V via `riscv64-zephyr-elf`, x86) | 1.0.1 / 1.5.0 | `west --version` |
| NASM / GAS | Assembly meta-language | TODO | `nasm -v` |
| FPC / Lazarus | Complementary language (Carmen) | TODO | `fpc -iV` |
| SWI-Prolog | Complementary language (Carmen) | TODO | `swipl --version` |
| Rust (cargo, bindgen) | Complementary language (Carmen, FFI) | TODO | `cargo --version` |
| Emscripten | WASM target (present: `D:\Workspace\tools\emsdk`) | TODO | `emcc -v` |
| FFmpeg / libvpx | WebM artifacts (optional) | TODO | `ffmpeg -version` |
| Python + pypdf | Spec text extraction | 3.x | `py -m pypdf --help` |
| Git + Git Bash | Sync (see WORKFLOW.md) | TODO | `git --version` |

## Agent tools
- Invariant/naming lint: `rg -n "float|double|math\.h|malloc" source/include source/src` (guard: fails if zero files match)
- TODO: `clang-format`, `clang-tidy`, `cppcheck`, sanitizers (`-fsanitize=address,undefined`).

## Windows notes
TODO: MSYS2 / clang-cl setup.
