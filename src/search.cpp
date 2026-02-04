#include "search.hpp"
#include <algorithm>
#include <limits>

namespace hexchess {
namespace search {

using namespace board;

const int KING_CAPTURED_WHITE_WINS = 10000;
const int KING_CAPTURED_BLACK_WINS = -10000;
const int CULL_MARGIN = 10;
const int CULL_MIN_DEPTH = 4;

static constexpr int TT_SIZE = 1 << 18;  // 256k entries

int minimax(State& state, int depth, int alpha, int beta, SearchContext& ctx) {
  ctx.nodes_used++;
  if (ctx.budget_exceeded()) return eval::evaluate(state);

  auto moves = moves::generate(state);
  if (moves.empty()) return eval::evaluate(state);

  if (depth == 0) return eval::evaluate(state);

  // Futility pruning: at depth >= 4, skip children if static eval is obviously bad
  if (depth >= CULL_MIN_DEPTH) {
    int static_eval = eval::evaluate(state);
    if (state.white_to_play && static_eval <= alpha - CULL_MARGIN)
      return static_eval;
    if (!state.white_to_play && static_eval >= beta + CULL_MARGIN)
      return static_eval;
  }

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
      if (!terminal) score = minimax(state, depth - 1, alpha, beta, ctx);
      state.undo_move(m, ui);
      if (ctx.budget_exceeded()) return max_eval;
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
      if (!terminal) score = minimax(state, depth - 1, alpha, beta, ctx);
      state.undo_move(m, ui);
      if (ctx.budget_exceeded()) return min_eval;
      min_eval = std::min(min_eval, score);
      beta = std::min(beta, score);
      if (beta <= alpha) break;
    }
    return min_eval;
  }
}

int minimax_node(Node& node, int depth, int ply, int alpha, int beta, SearchContext& ctx) {
  ctx.nodes_used++;
  if (ctx.budget_exceeded()) return eval::evaluate(node.state);

  auto moves = moves::generate(node.state);
  if (moves.empty()) return eval::evaluate(node.state);

  if (depth == 0) return eval::evaluate(node.state);

  uint64_t h = node.state.hash();
  if (ctx.tt && ctx.tt_mask > 0) {
    const TTEntry& entry = (*ctx.tt)[h & ctx.tt_mask];
    if (entry.key == h && entry.depth >= depth)
      return entry.score;
  }

  // Futility pruning: at depth >= 4, skip children if static eval is obviously bad
  if (depth >= CULL_MIN_DEPTH) {
    int static_eval = eval::evaluate(node.state);
    if (node.state.white_to_play && static_eval <= alpha - CULL_MARGIN)
      return static_eval;
    if (!node.state.white_to_play && static_eval >= beta + CULL_MARGIN)
      return static_eval;
  }

  if (node.state.white_to_play) {
    int max_eval = std::numeric_limits<int>::min();
    std::optional<Move> best_move;
    for (const Move& m : moves) {
      State::UndoInfo ui = node.state.make_move(m);
      int score;
      bool terminal = false;
      if (ui.captured && ui.captured->type == 'K') {
        score = KING_CAPTURED_WHITE_WINS;
        terminal = true;
      }
      auto child = std::make_unique<Node>();
      child->state = node.state;
      if (terminal) {
        child->best_score = score;
      } else {
        score = minimax_node(*child, depth - 1, ply + 1, alpha, beta, ctx);
      }
      node.state.undo_move(m, ui);
      node.children.push_back({m, std::move(child)});

      if (ctx.budget_exceeded()) return max_eval;
      if (score > max_eval) {
        max_eval = score;
        best_move = m;
      }
      alpha = std::max(alpha, score);
      if (beta <= alpha) break;
    }
    node.best_move = best_move;
    node.best_score = max_eval;
    if (ctx.tt && ctx.tt_mask > 0) {
      TTEntry& e = (*ctx.tt)[h & ctx.tt_mask];
      e.key = h;
      e.score = max_eval;
      e.depth = depth;
      e.flag = 0;
    }
    return max_eval;
  } else {
    int min_eval = std::numeric_limits<int>::max();
    std::optional<Move> best_move;
    for (const Move& m : moves) {
      State::UndoInfo ui = node.state.make_move(m);
      int score;
      bool terminal = false;
      if (ui.captured && ui.captured->type == 'K') {
        score = KING_CAPTURED_BLACK_WINS;
        terminal = true;
      }
      auto child = std::make_unique<Node>();
      child->state = node.state;
      if (terminal) {
        child->best_score = score;
      } else {
        score = minimax_node(*child, depth - 1, ply + 1, alpha, beta, ctx);
      }
      node.state.undo_move(m, ui);
      node.children.push_back({m, std::move(child)});

      if (ctx.budget_exceeded()) return min_eval;
      if (score < min_eval) {
        min_eval = score;
        best_move = m;
      }
      beta = std::min(beta, score);
      if (beta <= alpha) break;
    }
    node.best_move = best_move;
    node.best_score = min_eval;
    if (ctx.tt && ctx.tt_mask > 0) {
      TTEntry& e = (*ctx.tt)[h & ctx.tt_mask];
      e.key = h;
      e.score = min_eval;
      e.depth = depth;
      e.flag = 0;
    }
    return min_eval;
  }
}

void iterative_deepen(Node& root, int max_nodes, std::function<bool()> stop) {
  static std::vector<TTEntry> g_tt(TT_SIZE);

  SearchContext ctx;
  ctx.max_nodes = max_nodes;
  ctx.tt = &g_tt;
  ctx.tt_mask = TT_SIZE - 1;

  for (int d = 1; ; ++d) {
    if (stop && stop()) break;
    ctx.nodes_used = 0;

    auto moves = moves::generate(root.state);
    if (moves.empty()) break;

    // Save previous tree; we'll restore if budget exceeded mid-depth
    decltype(root.children) saved_children = std::move(root.children);
    auto saved_best_move = root.best_move;
    auto saved_best_score = root.best_score;
    root.children.clear();

    minimax_node(root, d, 0, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), ctx);

    if (ctx.budget_exceeded()) {
      root.children = std::move(saved_children);
      root.best_move = saved_best_move;
      root.best_score = saved_best_score;
      break;
    }
    // Tree reflects last completed depth; continue to next depth
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
