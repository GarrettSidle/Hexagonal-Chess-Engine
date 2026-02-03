#pragma once

#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace hexchess {
namespace search {

struct SearchResult {
  std::optional<board::Move> best_move;
  int score = 0;
  int depth = 0;
};

// One node: position + optional best move and children (for culling).
struct Node {
  board::State state;
  std::optional<board::Move> best_move;
  int best_score = 0;
  std::vector<std::pair<board::Move, std::unique_ptr<Node>>> children;
};

// Minimax with depth limit. White maximizes, black minimizes.
// Returns score from white's perspective.
int minimax(board::State& state, int depth, int alpha, int beta);

// Iterative deepening: run minimax at depth 1, 2, ... until stop() returns true or max_depth.
// stop() is checked at the start of each depth; pass nullptr to never stop early.
void iterative_deepen(Node& root, int max_depth, std::function<bool()> stop = nullptr);

// Find child of root that matches move (from/to). Returns pointer to that child or nullptr.
Node* find_child(Node& root, const board::Move& move);

}  // namespace search
}  // namespace hexchess
