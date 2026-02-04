# Hexagonal Chess Engine (C++)

Glinski hexagonal chess engine. Bot plays **black**; white is the opponent.

## Build

**With CMake:**

```bash
mkdir build && cd build
cmake ..
cmake --build .
cmake -G "Visual Studio 17 2022" -A x64 .
cmake --build . --config Release
```

**With Make (g++):**

```bash
make
```

## Protocol (stdin/stdout)

1. **Start a game**: Type the variant name to load the default starting position.
   - **glinski** – Glinski hexagonal chess (white to move).

   Alternatively, send 12 lines (one per column A–K, then `white` or `black`) for a custom position.

2. **move** (white): Type your move as `move a1b2` or just `a1b2`. The engine applies it, searches, and immediately prints `bestmove <from><to>` (black’s reply), then applies that move. Repeat with your next move.

3. **quit**: Exit.

## Example

```
glinski
b2c3
bestmove e6e5
d4e5
bestmove ...
quit
```
