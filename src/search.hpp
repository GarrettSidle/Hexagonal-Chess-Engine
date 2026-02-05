#pragma once

#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace hexchess {
namespace search {

// TT entry
struct TTEntry {
  uint64_t key = 0;
  int score = 0;
  int depth = 0;
  uint8_t flag = 0;  // 0=exact, 1=lower, 2=upper
  std::optional<board::Move> best_move;
};

struct SearchResult {
  std::optional<board::Move> best_move;
  int score = 0;
  int depth = 0;
};

// position + best move + children
struct Node {
  board::State state;
  std::optional<board::Move> best_move;
  int best_score = 0;
  std::vector<std::pair<board::Move, std::unique_ptr<Node>>> children;
};

// 2 killer slots per ply
static constexpr int MAX_PLY = 64;

struct SearchContext {
  int nodes_used = 0;
  int max_nodes = 3000;
  std::vector<TTEntry>* tt = nullptr;
  int tt_mask = 0;  // size-1 for power-of-2
  std::array<std::array<std::optional<board::Move>, 2>, MAX_PLY> killers{};
  bool budget_exceeded() const { return nodes_used >= max_nodes; }
};

// white max, black min. score from white POV
int minimax(board::State& state, int depth, int alpha, int beta, SearchContext& ctx);

// minimax that builds node tree (populates children)
int minimax_node(Node& node, int depth, int ply, int alpha, int beta, SearchContext& ctx);

// depth 1, 2, ... until stop() or budget. stop checked at start of each depth
void iterative_deepen(Node& root, int max_nodes = 3000, std::function<bool()> stop = nullptr);

// child matching move (from/to), or nullptr
Node* find_child(Node& root, const board::Move& move);

}  // namespace search
}  // namespace hexchess
