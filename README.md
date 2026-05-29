# pokerapp-phh_equity_eval

A modernized fork of [PokerStove](https://github.com/andrewprock/pokerstove) — a fast C++ poker hand evaluation and **exact equity** enumeration library.

Upstream PokerStove is a highly optimized evaluator for multiple poker variants. This fork upgrades the core to **C++20**, removes the **Boost** runtime dependency, and keeps the original `peval` / `penum` architecture.

## Features

- **C++20** with zero third-party runtime dependencies for core libraries (`peval`, `penum`, `util`)
- **14 poker variants** via `PokerHandEvaluator::alloc()` (Hold'em, Omaha, Stud, Razz, Draw, lowball, Badugi, …)
- **Exact equity** via exhaustive enumeration — not Monte Carlo sampling
- CLI tools: `ps-eval`, `ps-colex`, `ps-lut`
- GoogleTest unit tests (build-time only)

## Libraries

| Library | Purpose |
|---------|---------|
| **peval** | Hand evaluation — `CardSet`, rank/suit types, game-specific evaluators |
| **penum** | Equity enumeration — `CardDistribution`, `ShowdownEnumerator` |
| **util** | Header-only helpers — combinations, CLI parser, timing, string/format utils |

## Programs

| Tool | Description |
|------|-------------|
| **ps-eval** | Calculate showdown equity for one or more players |
| **ps-colex** | Print colexicographical indices for card combinations |
| **ps-lut** | Generate / inspect evaluation lookup tables |

Command syntax, hand formats, and limitations: **[USAGEA.md](USAGEA.md)**

Design docs: [RD.md](RD.md) (requirements), [PD.md](PD.md) (architecture)

## Quick start

```bash
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build -G "MinGW Makefiles"   # Windows MinGW
cmake --build build -j 4
ctest --test-dir build
```

```bash
# Hold'em — preflop equity (enumerates all 5-card runouts)
./build/bin/ps-eval AcAs Kh4d

# Hold'em — flop known
./build/bin/ps-eval AcAs Kh4d --board 5c8s9h

# Lowball 2-7 (no community cards)
./build/bin/ps-eval --game k 7c5c4c3c2c
```

`ps-eval` prints elapsed time to **stderr** after each run:

```text
elapsed: 0.012 s
```

## Equity model

### Exhaustive enumeration (not Monte Carlo)

`ps-eval` calls `ShowdownEnumerator::calculateEquity()`, which:

1. Iterates every hand combination in each player's `CardDistribution`
2. Deals remaining cards from the deck via `PartitionEnumerator`
3. Evaluates each complete showdown and accumulates win/tie shares

Results are **exact** within the enumerated scenario space. There is no random sampling.

### Hold'em and the board

For `--game h` (default), `boardSize()` is **5**.

| `--board` | Behavior |
|-----------|----------|
| omitted | Preflop — enumerates **all 5** community cards (standard preflop all-in equity) |
| 3 cards | Enumerates remaining 2 cards (turn + river) |
| 4 cards | Enumerates remaining 1 card (river) |
| 5 cards | Fixed board — no further cards dealt |

Hold'em does **not** support “0 community cards, compare hole cards only”. Games without a board (Stud, Draw, lowball, Badugi, …) use `boardSize() == 0`; pick the matching `--game` code.

### Hand input (ps-eval)

Supported:

- Explicit cards: `AcAs`, `Kh4d`
- Weighted combos: `"AcAs=0.5,KhKd=0.5"`
- Random range: `.` (enumerates all opponent hands, not Monte Carlo)

**Not** supported: range shorthand such as `22+`, `AKo`, `QQ+`. See [USAGEA.md](USAGEA.md).

## Building

### Requirements

- CMake ≥ 3.20
- C++20 compiler (GCC ≥ 11, Clang ≥ 14, or MSVC 2022)

### Linux

```bash
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build -j 4
ctest --test-dir build
```

### Windows (MSYS2 MinGW)

Ensure `D:\msys64\ucrt64\bin` (or your MSYS2 path) is on `PATH`, then:

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake --build build -j 4
```

The repo includes `.vscode/settings.json` and `tasks.json` configured for MinGW Makefiles + Release.

### Build types

`CMAKE_BUILD_TYPE` must be set at **configure** time (single-config generators):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug    # debug
cmake -B build -DCMAKE_BUILD_TYPE=Release  # optimized (recommended)
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTING` | ON | Build and register unit tests |
| `BUILD_PYTHON` | OFF | Build SWIG Python bindings |
| `BUILD_WHEEL` | OFF | scikit-build wheel mode |

## Python support (optional)

Python bindings via SWIG are retained but optional. On Ubuntu:

```bash
sudo apt install python3 swig
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build -DBUILD_PYTHON=ON
cmake --build build -j 4
```

Wheel build via `pipx run build` and `pyproject.toml` — see upstream workflow in `.github/workflows/cmake-build-test.yml`.

## Project layout

```text
src/lib/pokerstove/
  peval/     # hand evaluation
  penum/     # equity enumeration
  util/      # header-only utilities
src/programs/
  ps-eval/ ps-colex/ ps-lut/
```

## Upstream & breaking changes

- Based on [andrewprock/pokerstove](https://github.com/andrewprock/pokerstove) v1.2 lineage
- **Removed:** Boost dependency (program_options, format, lexical_cast, …)
- **Requires:** C++20 (`std::format`, `std::shared_ptr`, …)
- Legacy installers: `win32/`, `android/` (not maintained in this fork)

## License

See [LICENSE.txt](LICENSE.txt). Core library copyright remains with the original PokerStove authors.
