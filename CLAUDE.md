# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a decompiled/reconstructed C implementation of Super Metroid (SNES) that runs alongside the original ROM for frame-by-frame comparison. When a mismatch is detected between the reconstructed code and the original ROM execution, it saves a snapshot in `saves/` and displays a countdown. The project is early-stage and the code is described as messy.

## Build Commands

**macOS (primary dev environment):**
```sh
brew install sdl2          # one-time dependency
make                       # build
make -j$(nproc)            # parallel build
make clean all             # clean rebuild
CC=clang make              # use clang instead of gcc
```

**Output:** single executable `sm` in root directory.

**Run:** place `sm.smc` ROM (SHA1: `da957f0d63d14cb441d215462904c4fa8519c613`) in root, then `./sm`.

**No test suite exists.** Correctness is validated by running both the original ROM and reconstructed code in parallel and comparing frames.

## Commits

**CRITICAL: NEVER reference Claude, Claude Code, AI, or any AI assistant in commit messages, co-author tags, or anywhere in commits. No `Co-Authored-By` lines referencing Claude. No mention of AI-generated code. This is non-negotiable.**

## Architecture

### Dual-execution model
The emulator runs the original SNES ROM and the reconstructed C code simultaneously, comparing output frame-by-frame. This is the core verification mechanism — the reconstructed C code in `src/sm_*.c` is validated against the original ROM behavior.

### Layer structure

1. **SNES hardware emulation** (`src/snes/`) — Complete emulator: CPU (65c816), PPU (graphics), APU/DSP (audio), DMA, cartridge loading. This runs the original ROM.

2. **Reconstructed game code** (`src/sm_*.c`) — Decompiled Super Metroid logic split by original ROM bank number (sm_80.c through sm_ad.c, ~40 files). Function and variable names come from IDA Pro decompilation.

3. **Runtime layer** (`src/sm_rtl.c`) — Bridges the reconstructed code with the SNES emulator. Handles save/load states, replay recording/playback, and frame comparison between the two execution paths.

4. **Frontend** (`src/main.c`) — SDL2 event loop, rendering (SDL/OpenGL), audio output, gamepad input. Entry point.

### Key header files

- `src/variables.h` — Complete SNES RAM memory map with 1000+ named game variables
- `src/funcs.h` — All game function declarations (~7800 lines)
- `src/enemy_types.h` — Enemy entity definitions
- `src/types.h` — Custom integer types (uint8, uint16, etc.), SNES-style macros (LOBYTE, HIBYTE, GET_WORD), and a coroutine system
- `src/ida_types.h` — Type definitions from IDA Pro decompilation

### Configuration

Runtime config is in `sm.ini` (INI format parsed by `src/config.c`). Controls graphics mode, audio, keybindings, and features like EnhancedMode7 and NoSpriteLimits.

## Code Conventions

- Custom integer types everywhere: `uint8`, `uint16`, `uint32`, `int8`, `int16`, `int32` (defined in `types.h`)
- SNES memory access macros: `BYTE()`, `WORD()`, `LOBYTE()`, `HIBYTE()`, `GET_WORD()`, `load24()`
- ROM pointer macros: `RomPtr_XX()` where XX is the bank number
- Function naming follows decompilation output, not standard C conventions
- Game code files named by ROM bank: `sm_80.c` = bank $80, `sm_90.c` = bank $90, etc.
- Coroutine macros (`COROUTINE_BEGIN`, `COROUTINE_AWAIT`, `COROUTINE_END`) used for async game logic

## Compiler Flags

Default: `-O2 -fno-strict-aliasing -Werror`. The `-fno-strict-aliasing` is important because the codebase does extensive type-punning through pointer casts (SNES memory access patterns). `-Werror` means all warnings are errors.
