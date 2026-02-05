#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hexchess {
namespace board {

// 0=Glinski, 1=McCooey, 2=Hexofen
enum class Variant { Glinski = 0, McCooey = 1, Hexofen = 2 };

// 11 cols, variable rows per col
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

// logical row (move math) vs storage row
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

// empty or one piece
using Square = std::optional<Piece>;

// columns 0..10, variable row length. prev_move for ep
struct State {
  std::vector<std::vector<Square>> cells;
  bool white_to_play = true;
  std::optional<Move> prev_move;
  Variant variant = Variant::Glinski;

  State();
  // Glinski/McCooey/Hexofen start pos
  void set_glinski();
  void set_mccooey();
  void set_hexofen();

  // in bounds for current variant
  bool on_board(int col, int storage_row) const;
  static bool on_board(Variant v, int col, int storage_row);

  std::optional<Piece> at(int col, int storage_row) const;

  // zobrist for TT
  uint64_t hash() const;

  // make move (assumes legal). returns undo info
  struct UndoInfo {
    std::optional<Piece> captured;
    bool was_ep = false;
    std::optional<Move> prev_move;
  };
  UndoInfo make_move(const Move& move);

  void undo_move(const Move& move, const UndoInfo& undo);
};

// "A1", "B2" etc. col 0 = A, row 0 = 1
std::string square_notation(int col, int row);

}  // namespace board
}  // namespace hexchess
