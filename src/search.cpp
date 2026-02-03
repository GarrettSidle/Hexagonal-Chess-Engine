#include "search.hpp"
#include <algorithm>
#include <limits>

namespace hexchess {
namespace search {

using namespace board;

const int KING_CAPTURED_WHITE_WINS = 10000;
const int KING_CAPTURED_BLACK_WINS = -10000;

int minimax(State& state, int depth, int alpha, int beta) {
  auto moves = moves::generate(state);
  if (moves.empty()) return eval::evaluate(state);

  if (depth == 0) return eval::evaluate(state);

  if (state.white_to_play) {
    int max_eval = std::numeric_limits<int>::min();
    for (const Move& m : moves) {
      State::UndoInfo ui = state.make_move(m);
      int score = eval::evaluate(state);
      bool terminal = false;
      if (ui.captured && ui.captured->type == 'K') {
        score = KING_CAPTURED_WHITE_WINS;
        terminal = true;
      }
      if (!terminal) score = minimax(state, depth - 1, alpha, beta);
      state.undo_move(m, ui);
      max_eval = std::max(max_eval, score);
      alpha = std::max(alpha, score);
      if (beta <= alpha) break;
    }
    return max_eval;
  } else {
    int min_eval = std::numeric_limits<int>::max();
    for (const Move& m : moves) {
      State::UndoInfo ui = state.make_move(m);
      int score = eval::evaluate(state);
      bool terminal = false;
      if (ui.captured && ui.captured->type == 'K') {
        score = KING_CAPTURED_BLACK_WINS;
        terminal = true;
      }
      if (!terminal) score = minimax(state, depth - 1, alpha, beta);
      state.undo_move(m, ui);
      min_eval = std::min(min_eval, score);
      beta = std::min(beta, score);
      if (beta <= alpha) break;
    }
    return min_eval;
  }
}

void iterative_deepen(Node& root, int max_depth, std::function<bool()> stop) {
  for (int d = 1; d <= max_depth; ++d) {
    if (stop && stop()) break;
    auto moves = moves::generate(root.state);
    if (moves.empty()) break;
    int best_score = root.state.white_to_play ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    std::optional<Move> best_move;
    bool white_to_move = root.state.white_to_play;
    for (const Move& m : moves) {
      State::UndoInfo ui = root.state.make_move(m);
      int score = eval::evaluate(root.state);
      if (ui.captured && ui.captured->type == 'K')
        score = white_to_move ? KING_CAPTURED_WHITE_WINS : KING_CAPTURED_BLACK_WINS;
      else
        score = minimax(root.state, d - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
      root.state.undo_move(m, ui);
      if (white_to_move) {
        if (score > best_score) { best_score = score; best_move = m; }
      } else {
        if (score < best_score) { best_score = score; best_move = m; }
      }
    }
    if (best_move) {
      root.best_move = *best_move;
      root.best_score = best_score;
    }
  }
}

Node* find_child(Node& root, const Move& move) {
  for (auto& [m, child] : root.children) {
    if (m.from_col == move.from_col && m.from_row == move.from_row &&
        m.to_col == move.to_col && m.to_row == move.to_row)
      return child.get();
  }
  return nullptr;
}

}  // namespace search
}  // namespace hexchess
