#pragma once

#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace hexchess {
namespace search {

// Transposition table entry
struct TTEntry {
  uint64_t key = 0;
  int score = 0;
  int depth = 0;
  uint8_t flag = 0;  // 0=exact, 1=lower_bound, 2=upper_bound
};

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

// Search context: node budget, TT, and usage.
struct SearchContext {
  int nodes_used = 0;
  int max_nodes = 1000;
  std::vector<TTEntry>* tt = nullptr;
  int tt_mask = 0;  // size - 1 for power-of-2 table
  bool budget_exceeded() const { return nodes_used >= max_nodes; }
};

// Minimax with depth limit. White maximizes, black minimizes.
// Returns score from white's perspective.
int minimax(board::State& state, int depth, int alpha, int beta, SearchContext& ctx);

// Minimax that builds Node tree while searching. Populates node.children.
int minimax_node(Node& node, int depth, int ply, int alpha, int beta, SearchContext& ctx);

// Iterative deepening: run minimax at depth 1, 2, ... until stop() returns true or node budget exceeded.
// stop() is checked at the start of each depth; pass nullptr to never stop early.
void iterative_deepen(Node& root, int max_nodes = 1000, std::function<bool()> stop = nullptr);

// Find child of root that matches move (from/to). Returns pointer to that child or nullptr.
Node* find_child(Node& root, const board::Move& move);

}  // namespace search
}  // namespace hexchess
