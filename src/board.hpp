#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hexchess {
namespace board {

// Variant: 0 = Glinski, 1 = McCooey, 2 = Hexofen.
enum class Variant { Glinski = 0, McCooey = 1, Hexofen = 2 };

// Glinski: 11 columns, variable rows per column.
// col 0: 6 rows, col 1: 7, ... col 5: 11, col 6: 10, ... col 10: 6.
constexpr int NUM_COLS = 11;

inline int max_row_glinski(int col) {
  return col <= 5 ? 6 + col : 16 - col;
}

inline int max_row_mccooey(int col) {
  static const int rows[] = { 6, 7, 8, 9, 10, 11, 10, 9, 8, 7, 6 };
  return col >= 0 && col < 11 ? rows[col] : 0;
}

inline int max_row_hexofen(int col) {
  return max_row_glinski(col);  // same shape as Glinski
}

inline int max_row(Variant v, int col) {
  if (v == Variant::McCooey) return max_row_mccooey(col);
  return max_row_glinski(col);  // Glinski and Hexofen
}

// Logical row (for move direction math) vs storage row (array index).
// Right half (col > 5): same logical line has higher logical row index.
inline int get_logical_row(int col, int storage_row) {
  return col <= 5 ? storage_row : storage_row + col - 5;
}
inline int get_storage_row(int col, int logical_row) {
  return col <= 5 ? logical_row : logical_row + 5 - col;
}

struct Piece {
  char type;   // P, R, N, B, K, Q (uppercase in logic)
  bool white;
};

struct Move {
  int from_col, from_row, to_col, to_row;
  bool capture = false;
  bool en_passant = false;
  bool promotion = false;
};

// Square: empty or one piece.
using Square = std::optional<Piece>;

// Board state: columns 0..10, each column has variable-length row.
// white_to_play: true = white to move.
// prev_move: for en passant (pawn double-step); nullopt if not applicable.
struct State {
  std::vector<std::vector<Square>> cells;
  bool white_to_play = true;
  std::optional<Move> prev_move;
  Variant variant = Variant::Glinski;

  State();
  // Build from Glinski / McCooey / Hexofen initial position.
  void set_glinski();
  void set_mccooey();
  void set_hexofen();

  // (col, storage_row) in bounds for current variant?
  bool on_board(int col, int storage_row) const;
  static bool on_board(Variant v, int col, int storage_row);

  // Get piece at (col, storage_row). Returns nullopt if off-board or empty.
  std::optional<Piece> at(int col, int storage_row) const;

  // Make move; assumes legal. Updates prev_move for en passant.
  // Returns info needed to undo (captured piece, was en passant, old prev_move).
  struct UndoInfo {
    std::optional<Piece> captured;
    bool was_ep = false;
    std::optional<Move> prev_move;
  };
  UndoInfo make_move(const Move& move);

  void undo_move(const Move& move, const UndoInfo& undo);
};

// Notation for a square: "A1", "B2", etc. (col 0 = A, row 0 = 1).
std::string square_notation(int col, int row);

}  // namespace board
}  // namespace hexchess
