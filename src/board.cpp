#include "board.hpp"
#include <stdexcept>

namespace hexchess {
namespace board {

State::State() {
  cells.resize(NUM_COLS);
  for (int c = 0; c < NUM_COLS; ++c)
    cells[c].resize(static_cast<size_t>(max_row_glinski(c)));
}

void State::set_glinski() {
  // Glinski initial position (from Utils.cs glinskiBoard).
  const char* glinski[] = {
    "      ",           // col 0: 6
    "P     p",          // col 1: 7
    "RP    pr",         // col 2: 8
    "N P   p n",        // col 3: 9
    "Q  P  p  q",       // col 4: 10
    "BBB P p bbb",      // col 5: 11
    "K  P  p  k",       // col 6: 10
    "N P   p n",        // col 7: 9
    "RP    pr",         // col 8: 8
    "P     p",          // col 9: 7
    "      ",           // col 10: 6
  };
  for (int c = 0; c < NUM_COLS; ++c) {
    int maxr = max_row_glinski(c);
    const char* row_str = glinski[c];
    for (int r = 0; r < maxr; ++r) {
      char ch = row_str[r];
      if (ch == ' ' || ch == '\0') {
        cells[c][static_cast<size_t>(r)] = std::nullopt;
      } else {
        Piece p;
        p.type = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        p.white = (ch >= 'A' && ch <= 'Z');
        cells[c][static_cast<size_t>(r)] = p;
      }
    }
  }
  white_to_play = true;
  prev_move = std::nullopt;
}

bool State::on_board(int col, int storage_row) {
  if (col < 0 || col >= NUM_COLS) return false;
  int maxr = max_row_glinski(col);
  return storage_row >= 0 && storage_row < maxr;
}

std::optional<Piece> State::at(int col, int storage_row) const {
  if (!State::on_board(col, storage_row)) return std::nullopt;
  return cells[static_cast<size_t>(col)][static_cast<size_t>(storage_row)];
}

State::UndoInfo State::make_move(const Move& move) {
  UndoInfo ui;
  ui.prev_move = prev_move;

  Square& from_sq = cells[static_cast<size_t>(move.from_col)][static_cast<size_t>(move.from_row)];
  Square& to_sq = cells[static_cast<size_t>(move.to_col)][static_cast<size_t>(move.to_row)];
  Piece p = *from_sq;
  from_sq = std::nullopt;

  if (move.en_passant) {
    int ep_col = move.to_col;
    int ep_row = p.white ? move.to_row + 1 : move.to_row - 1;
    if (State::on_board(ep_col, ep_row)) {
      ui.captured = cells[static_cast<size_t>(ep_col)][static_cast<size_t>(ep_row)];
      ui.was_ep = true;
      cells[static_cast<size_t>(ep_col)][static_cast<size_t>(ep_row)] = std::nullopt;
    }
  } else if (to_sq) {
    ui.captured = *to_sq;
  }

  if (move.promotion)
    p.type = 'Q';
  to_sq = p;

  if (p.type == 'P' && std::abs(move.to_row - move.from_row) == 2)
    prev_move = move;
  else
    prev_move = std::nullopt;

  white_to_play = !white_to_play;
  return ui;
}

void State::undo_move(const Move& move, const UndoInfo& undo) {
  white_to_play = !white_to_play;
  prev_move = undo.prev_move;

  Square& from_sq = cells[static_cast<size_t>(move.from_col)][static_cast<size_t>(move.from_row)];
  Square& to_sq = cells[static_cast<size_t>(move.to_col)][static_cast<size_t>(move.to_row)];

  Piece p = *to_sq;
  if (move.promotion) p.type = 'P';
  from_sq = p;
  to_sq = std::nullopt;

  if (undo.was_ep && undo.captured && State::on_board(move.to_col, move.to_row + (p.white ? 1 : -1)))
    cells[static_cast<size_t>(move.to_col)][static_cast<size_t>(move.to_row + (p.white ? 1 : -1))] = *undo.captured;
  else if (undo.captured)
    to_sq = *undo.captured;
}

std::string square_notation(int col, int row) {
  if (col < 0 || col > 25) return "??";
  return std::string(1, static_cast<char>('A' + col)) + std::to_string(row + 1);
}

}  // namespace board
}  // namespace hexchess
