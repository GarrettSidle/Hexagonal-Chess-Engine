#include "eval.hpp"
#include "board.hpp"
#include <cstdlib>

namespace hexchess {
namespace eval {

int piece_value(char type) {
  switch (type) {
    case 'P': return 1;
    case 'R': return 5;
    case 'N': return 3;
    case 'B': return 3;
    case 'K': return 0;
    case 'Q': return 9;
    default: return 0;
  }
}

int evaluate(const board::State& state) {
  int score = 0;
  for (int c = 0; c < board::NUM_COLS; ++c) {
    int maxr = board::max_row(state.variant, c);
    for (int r = 0; r < maxr; ++r) {
      auto sq = state.at(c, r);
      if (!sq) continue;
      int v = piece_value(sq->type);
      if (sq->white) score += v; else score -= v;
    }
  }
  return score;
}

bool is_terminal(const board::State& state, const board::Move& move_just_made) {
  auto captured = state.white_to_play ? state.at(move_just_made.to_col, move_just_made.to_row) : std::optional<board::Piece>{};
  (void)state;
  (void)move_just_made;
  return false;  // We detect king capture by checking captured piece after make_move.
}

}  // namespace eval
}  // namespace hexchess
