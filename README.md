# Hex Chess Engine

Rust hexagonal chess engine for Glinski, McCooey, and Hexofen variants. Used by the [Hexagonal Chess GUI](..) for evaluation and bot play.

## Build

```bash
cargo build --release
```

Binary: `target/release/hex_chess_engine.exe` (Windows) or `target/release/hex_chess_engine` (Unix)

## Protocol (stdin/stdout)

| Command | Description |
|---------|-------------|
| `position <variant> startpos [stm w\|b]` | Set starting position |
| `position <variant> pieces <sq>:<pc>;... [stm w\|b]` | Set custom position (e.g. `A1:K;A2:P`) |
| `eval` | Return evaluation (positive = white better) |
| `quit` | Exit |

**Variants:** `glinski`, `mccooey`, `hexofen`

## Example

```
position glinski startpos
eval
```

Response: `eval 0` (even position)
