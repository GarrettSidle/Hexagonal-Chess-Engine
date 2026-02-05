#include "moves.hpp"
#include "board.hpp"
#include "eval.hpp"
#include <algorithm>
#include <cmath>

namespace hexchess {
namespace moves {

using namespace board;

// (col_delta, logical_row_delta)
struct Dir { int dc, dr; };

// Horizontal (rook) directions - 6.
static const Dir HORIZ[] = {
  { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 }, { 1, 1 }, { -1, -1 }
};
// Diagonal (bishop) directions - 6.
static const Dir DIAG[] = {
  { -2, -1 }, { 2, 1 }, { 1, 2 }, { -1, 1 }, { 1, -1 }, { -1, -2 }
};
// Knight jumps - 12.
static const Dir KNIGHT[] = {
  { 1, 3 }, { 2, 3 }, { 3, 1 }, { 3, 2 }, { 2, -1 }, { 1, -2 },
  { -1, -3 }, { -2, -3 }, { -3, -1 }, { -3, -2 }, { -2, 1 }, { -1, 2 }
};
// King - 12 (horizontal + diagonal).
static const Dir KING[] = {
  { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 }, { 1, 1 }, { -1, -1 },
  { -2, -1 }, { 2, 1 }, { 1, 2 }, { -1, 1 }, { 1, -1 }, { -1, -2 }
};
// White pawn capture directions (col, logical_row).
static const Dir W_PAWN_CAP[] = { { -1, 0 }, { 1, 1 } };
static const Dir B_PAWN_CAP[] = { { -1, -1 }, { 1, 0 } };

static bool is_starting_pawn_white_glinski(int col, int storage_row) {
  if (col < 6) return (col - 1) == storage_row;
  return (storage_row + col) == 9;
}
static bool is_starting_pawn_black_glinski(int col, int storage_row) {
  return storage_row == 6;
}
static bool is_starting_pawn_white_mccooey(int col, int storage_row) {
  if (col < 6) return (col - 2) == storage_row;
  return (storage_row + col) == 8;
}
static bool is_starting_pawn_black_mccooey(int col, int storage_row) {
  return storage_row == 7;
}
static bool is_starting_pawn_white_hexofen(int col, int storage_row) {
  static const int rows[] = { 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0 };
  return col >= 0 && col < 11 && rows[col] == storage_row;
}
static bool is_starting_pawn_black_hexofen(int col, int storage_row) {
  static const int rows[] = { 5, 6, 6, 7, 7, 8, 7, 7, 6, 6, 5 };
  return col >= 0 && col < 11 && rows[col] == storage_row;
}

bool is_starting_pawn_white(const State& state, int col, int storage_row) {
  switch (state.variant) {
    case Variant::Glinski: return is_starting_pawn_white_glinski(col, storage_row);
    case Variant::McCooey: return is_starting_pawn_white_mccooey(col, storage_row);
    case Variant::Hexofen: return is_starting_pawn_white_hexofen(col, storage_row);
  }
  return false;
}
bool is_starting_pawn_black(const State& state, int col, int storage_row) {
  switch (state.variant) {
    case Variant::Glinski: return is_starting_pawn_black_glinski(col, storage_row);
    case Variant::McCooey: return is_starting_pawn_black_mccooey(col, storage_row);
    case Variant::Hexofen: return is_starting_pawn_black_hexofen(col, storage_row);
  }
  return false;
}

// ep target square if any (pawns double-step only)
static std::string en_passant_square(const State& state) {
  if (!state.prev_move) return "";
  const Move& pm = *state.prev_move;
  if (std::abs(pm.to_row - pm.from_row) != 2) return "";
  int ep_col = pm.to_col;
  bool moved_white = !state.white_to_play;
  int ep_row = moved_white ? pm.to_row - 1 : pm.to_row + 1;
  return square_notation(ep_col, ep_row);
}

static void add_displacement_moves(std::vector<Move>& out, const State& state,
    int col, int row, bool piece_white, char piece_type, const Dir* dirs, int n) {
  int logical = get_logical_row(col, row);
  for (int i = 0; i < n; ++i) {
    int nc = col + dirs[i].dc;
    int nlog = logical + dirs[i].dr;
    int nr = get_storage_row(nc, nlog);
    if (!state.on_board(nc, nr)) continue;
    std::optional<Piece> target = state.at(nc, nr);
    if (!target) {
      out.push_back(Move{ col, row, nc, nr, false, false, false });
    } else if (target->white != piece_white) {
      out.push_back(Move{ col, row, nc, nr, true, false, false });
    }
  }
}

static void add_straight_moves(std::vector<Move>& out, const State& state,
    int col, int row, bool piece_white, const Dir* dirs, int n) {
  int logical = get_logical_row(col, row);
  for (int i = 0; i < n; ++i) {
    int dc = dirs[i].dc, dr = dirs[i].dr;
    int c = col, lr = logical;
    for (;;) {
      c += dc; lr += dr;
      int sr = get_storage_row(c, lr);
      if (!state.on_board(c, sr)) break;
      std::optional<Piece> target = state.at(c, sr);
      if (!target) {
        out.push_back(Move{ col, row, c, sr, false, false, false });
      } else {
        if (target->white != piece_white)
          out.push_back(Move{ col, row, c, sr, true, false, false });
        break;
      }
    }
  }
}

static void add_pawn_moves(std::vector<Move>& out, const State& state,
    int col, int row, bool piece_white) {
  int logical = get_logical_row(col, row);
  const Dir* cap_dirs = piece_white ? W_PAWN_CAP : B_PAWN_CAP;
  std::string ep_square = en_passant_square(state);

  for (int i = 0; i < 2; ++i) {
    int nc = col + cap_dirs[i].dc;
    int nlog = logical + cap_dirs[i].dr;
    int nr = get_storage_row(nc, nlog);
    if (!state.on_board(nc, nr)) continue;
    if (square_notation(nc, nr) == ep_square) {
      out.push_back(Move{ col, row, nc, nr, true, true, false });
      continue;
    }
    std::optional<Piece> target = state.at(nc, nr);
    if (target && target->white != piece_white)
      out.push_back(Move{ col, row, nc, nr, true, false, false });
  }

  int forward_lr = piece_white ? logical + 1 : logical - 1;
  int forward_sr = get_storage_row(col, forward_lr);
  if (!state.on_board(col, forward_sr)) return;
  if (state.at(col, forward_sr)) return;  // blocked
  out.push_back(Move{ col, row, col, forward_sr, false, false, false });

  bool starting = piece_white ? is_starting_pawn_white(state, col, row) : is_starting_pawn_black(state, col, row);
  if (!starting) return;
  int double_lr = piece_white ? logical + 2 : logical - 2;
  int double_sr = get_storage_row(col, double_lr);
  if (!state.on_board(col, double_sr)) return;
  if (state.at(col, double_sr)) return;
  out.push_back(Move{ col, row, col, double_sr, false, false, false });
}

// black promo row 0; white last rank (row-col==5 or col+row==15)
static bool is_promotion(const State& state, int to_col, int to_row, bool piece_white) {
  if (piece_white) {
    if (to_col <= 5 && (to_row - to_col) == 5) return true;
    if (to_col > 5 && (to_col + to_row) == 15) return true;
  } else {
    if (to_row == 0) return true;
  }
  return false;
}

std::vector<Move> generate(const State& state) {
  std::vector<Move> result;
  bool white_to_move = state.white_to_play;

  for (int c = 0; c < NUM_COLS; ++c) {
    int maxr = max_row(state.variant, c);
    for (int r = 0; r < maxr; ++r) {
      std::optional<Piece> sq = state.at(c, r);
      if (!sq || sq->white != white_to_move) continue;
      char t = sq->type;
      bool pw = sq->white;

      switch (t) {
        case 'P':
          add_pawn_moves(result, state, c, r, pw);
          break;
        case 'R':
          add_straight_moves(result, state, c, r, pw, HORIZ, 6);
          break;
        case 'N':
          add_displacement_moves(result, state, c, r, pw, t, KNIGHT, 12);
          break;
        case 'B':
          add_straight_moves(result, state, c, r, pw, DIAG, 6);
          break;
        case 'K':
          add_displacement_moves(result, state, c, r, pw, t, KING, 12);
          break;
        case 'Q':
          add_straight_moves(result, state, c, r, pw, HORIZ, 6);
          add_straight_moves(result, state, c, r, pw, DIAG, 6);
          break;
        default:
          break;
      }
    }
  }

  // mark promotions
  for (Move& m : result) {
    std::optional<Piece> p = state.at(m.from_col, m.from_row);
    if (p && p->type == 'P' && is_promotion(state, m.to_col, m.to_row, p->white))
      m.promotion = true;
  }
  return result;
}

static bool moves_equal(const Move& a, const Move& b) {
  return a.from_col == b.from_col && a.from_row == b.from_row &&
         a.to_col == b.to_col && a.to_row == b.to_row;
}

static int mvv_lva_score(const State& state, const Move& m) {
  auto victim = state.at(m.to_col, m.to_row);
  auto attacker = state.at(m.from_col, m.from_row);
  int victim_val = victim ? eval::piece_value(victim->type) : 0;
  int attacker_val = attacker ? eval::piece_value(attacker->type) : 1;
  return victim_val * 10 - attacker_val;  // higher = try first (mvv-lva)
}

void order_moves(std::vector<Move>& moves, const State& state,
    std::optional<Move> hash_move, std::optional<Move> killer1, std::optional<Move> killer2) {
  std::vector<Move> ordered;
  ordered.reserve(moves.size());

  auto is_hash = [&](const Move& m) {
    return hash_move && moves_equal(m, *hash_move);
  };
  auto is_killer = [&](const Move& m) {
    return (killer1 && moves_equal(m, *killer1)) || (killer2 && moves_equal(m, *killer2));
  };

  if (hash_move) {
    for (auto it = moves.begin(); it != moves.end(); ++it) {
      if (moves_equal(*it, *hash_move)) {
        ordered.push_back(*it);
        moves.erase(it);
        break;
      }
    }
  }

  std::vector<Move> captures, killers, rest;
  for (const Move& m : moves) {
    if (m.capture || m.en_passant) {
      captures.push_back(m);
    } else if (is_killer(m)) {
      killers.push_back(m);
    } else {
      rest.push_back(m);
    }
  }
  std::sort(captures.begin(), captures.end(), [&](const Move& a, const Move& b) {
    return mvv_lva_score(state, a) > mvv_lva_score(state, b);
  });

  for (const Move& m : captures) ordered.push_back(m);
  for (const Move& m : killers) ordered.push_back(m);
  for (const Move& m : rest) ordered.push_back(m);

  moves = std::move(ordered);
}

}  // namespace moves
}  // namespace hexchess
