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

4. **heartbeat**: Optional. If the GUI sends `heartbeat` (e.g. every 0.5s), the engine stays alive. If the engine receives no heartbeat for 5 consecutive 0.5s checks (2.5s total), it exits. This allows the engine to shut down when the GUI disconnects.

5. **Idle timeout**: If the engine receives no input (any line) for 5 minutes, it exits. This catches cases where the GUI disconnects without sending heartbeats or closing stdin, so the engine does not run indefinitely in the background.

## Example

```
glinski
b2c3
bestmove e6e5
d4e5
bestmove ...
quit
```

---

## How the Hexagonal Chess Bot Thinks

**Radial tree**

![Radial tree at depth 4](Captures/RadialTree.png)

Building a chess bot comes down to turning a messy board position into math, then using that math to predict the future. At its core, the bot is just evaluating positions and exploring possible moves as efficiently as possible.

### Turning a Chess Position into a Number

The first thing the bot needs is a way to judge how good a position is. In this bot, that evaluation is done purely through material point values. Every piece on the board is assigned a numeric value, and the total position score is just the sum of those values for both sides.

A typical point system might look like this:

| Piece  | Value |
|--------|-------|
| Pawn   | 1     |
| Knight | 3     |
| Bishop | 3     |
| Rook   | 5     |
| Queen  | 9     |
| King   | 1000  |

The king's value is intentionally absurdly high and ideally it would be infinitely high. This ensures that no amount of material gain is ever considered more important than losing the king. The final evaluation is a single number: positive if the position favors one side, negative if it favors the other.

This simplicity makes the evaluation fast, which is critical when the bot needs to analyze thousands of positions per move.

### Predicting the Future with Move Trees

Once the bot can score a position, it needs to look ahead. This is done by building a game tree, where each node represents a board position and each edge represents a legal move. From the current position, the bot simulates all possible moves, then all possible replies, and so on.

To decide which move is best, the bot uses the **minimax algorithm**. Minimax assumes that both players will always choose the move that is best for themselves. When it's the bot's turn, it tries to maximize the evaluation score; when it's the opponent's turn, it assumes they will minimize it.

**Minimax tree**

![Minimax tree](Captures/MinimaxTree.png)

Above is a heavily simplified version of a tree, where white represents all even levels of the tree and black is all odd levels. In this tree white will always pick the move that brings the evaluation in the positive direction and black will do the opposite. It is important to note that in this example black is the first to move and only has two possible options in terms of movement. On a hexagonal board players have an average of around 140 possible moves at any given time if all pieces are still on the board. Making this a very large search tree.

The search depth plays a huge role here. A deeper search means the bot can see further into the future, but the number of positions grows exponentially with each extra ply.

**Example**

Given this board position (white to move), there are 19 possible moves for white (12 knight moves and 7 kings moves). If the bot was evaluating this position on a depth setting of 1 the move that would result in the greatest value would take the bishop (KxB H7 E6) resulting in a 3 point gain compared to the tree root.

**Depth 1**

![Depth 1](Captures/Depth1.png)

Although if the bot was instead set at a depth value of 2, it would look deeper and see the move Knight I10 (K H7 I10). Although this does not result in an immediate point increase it does put Black into a forced king capture on the following move since Black has no moves to block the capture.

**Depth 2**

![Depth 2](Captures/Depth2.png)

On a depth setting of 2 the engine would compare all of the possible moves stemming from the root in this case. A +3 for taking the bishop, a +1000 for taking the king, and +0 for every other move. The engine then simply picks the highest possible score and makes the move Knight I10.

These calculations take place for every possible move up to the engine max node count, looking at every board position stemming from the root position and comparing the outcomes.

To keep things manageable, the bot also has a node limit, typically somewhere between 100 and 5000 positions per move. For comparison, modern chess engines evaluate millions of nodes for a single position. At that scale, the main bottleneck becomes raw CPU power, which is why serious engines rely heavily on optimizations.

### Optimizations

As the search tree grows, evaluating every possible position quickly becomes infeasible. To push the bot further without increasing the node limit, several classic chess-engine optimizations are used.

#### Transposition Tables

Many different move sequences can lead to the same board position. Without optimization, the bot would evaluate these identical positions multiple times, wasting valuable search time. A transposition table fixes this by caching the evaluation of previously seen positions and reusing them whenever the same position appears again.

To make this efficient, the bot uses **Zobrist hashing**. Each piece–square combination is assigned a random 64-bit number. The hash of a board position is created by XOR-ing the values for all pieces currently on the board. This allows the bot to generate a near-unique identifier for a position extremely quickly.

When the bot encounters a position, it:

1. Computes the Zobrist hash
2. Checks if that hash exists in the transposition table
3. Reuses the stored evaluation if available, instead of recalculating it

This dramatically reduces duplicated work, especially in deeper searches where transpositions are common.

**Example**

In the following example the final positions for both board A and B are identical even though they took separate steps to reach those positions. In this case the bot would hash the two positions and notice the overlap and rather than making 2 new nodes it would simply point both cases to the same node. This prevents duplicate calculations being done for the positions of child nodes.

![Transposition table](Captures/TranspositionTable.png)

#### Iterative Deepening

Rather than immediately searching to the maximum depth, the bot uses **iterative deepening**. It first searches to depth 1, then depth 2, then depth 3, and so on until it reaches the depth or node limit.

While this may seem inefficient at first, it provides two major benefits:

- Results from earlier, shallower searches are reused to guide deeper searches
- The bot always has a "best move so far" available if the search must be stopped early

Iterative deepening also greatly improves move ordering, which makes other optimizations, especially alpha–beta pruning, far more effective.

#### Killer Move Heuristic

The **killer move heuristic** is based on the idea that some moves are so strong they tend to cause cutoffs repeatedly at the same depth of the search tree.

When a move causes a beta cutoff during search, it is stored as a killer move for that depth. In future nodes at the same depth, the bot tries these killer moves early, before exploring other options.

By testing historically strong moves first, the bot increases the likelihood of early cutoffs, reducing the total number of nodes that need to be searched.

#### Alpha–Beta Pruning

**Alpha–beta pruning** is an optimization applied directly to the minimax algorithm. It tracks two values:

- **Alpha**: the best score the maximizing player can guarantee
- **Beta**: the best score the minimizing player can guarantee

If the bot determines that a branch cannot possibly improve the current outcome, because it is already worse than a known alternative, that branch is discarded without further exploration.

On top of standard alpha–beta pruning, this bot applies an additional domain-specific culling rule. Any subtree that is at least 4 plies deep and has an evaluation that differs from the root by 10 or more points is immediately pruned. Positions that diverge this far from the root are treated as unrealistic continuations, typically representing early blunders or lines a rational opponent would never allow.

This aggressive pruning dramatically reduces the size of the search tree, keeping the engine fast under tight node limits. The downside is that sharp tactical ideas, such as queen sacrifices or long-term compensation plays, are unlikely to be discovered, since they often require temporarily accepting a large material deficit.

For a scrappy, lightweight engine, however, this tradeoff is acceptable. The bot sacrifices some tactical brilliance in exchange for speed, stability, and reasonably strong practical play.

### System Overview

The diagram shows how the Hexagonal Chess bot works at a high level. A GUI talks to the engine by sending text commands in and reading replies out. The control loop handles that conversation: it reads what you type (like `glinski` to start a game or `a1b2` to make a move), keeps an eye on the connection via heartbeats, and drives the rest of the system. The protocol layer translates human-style board positions and moves into the format the engine uses internally. The board is the core chess state, the pieces and their positions. When it's the bot's turn, the search module explores possible moves and picks the best one, using move generation and position evaluation under the hood. After each move, the bot can optionally export its thinking to a file for visualization. In short: you send commands, the engine parses them, updates the board, searches for the best reply, and sends it back.

![System diagram](Captures/SystenDiagram.png)

### Conclusion

Overall this chess bot is a resounding success—I am only able to beat it when it is at or near its lowest max node value. I encourage anyone interested in hexagon chess to check it out.

### Captures

**Radial representation of bot's tree at depth = 4**

![Radial tree](Captures/RadialTree.png)

**A video of bot play in action**

<video src="Captures/BotPlay.mp4" controls width="600">[BotPlay.mp4](Captures/BotPlay.mp4)</video>
