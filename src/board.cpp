#include "board.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace hexchess {
namespace board {

// Zobrist keys: pieces + side-to-move + en-passant square
static constexpr int ZOBRIST_COLS = 11;
static constexpr int ZOBRIST_ROWS = 11;
static constexpr int ZOBRIST_PIECES = 12;  // 6 types * 2 colors
static constexpr int ZOBRIST_PIECE_KEYS = ZOBRIST_COLS * ZOBRIST_ROWS * ZOBRIST_PIECES;
static constexpr int ZOBRIST_EP_KEYS = ZOBRIST_COLS * ZOBRIST_ROWS;
static constexpr int ZOBRIST_SIZE = ZOBRIST_PIECE_KEYS + 1 + ZOBRIST_EP_KEYS;

static uint64_t g_zobrist_keys[ZOBRIST_SIZE];
static bool g_zobrist_init = false;

static void init_zobrist() {
  if (g_zobrist_init) return;
  for (int i = 0; i < ZOBRIST_SIZE; ++i) {
    g_zobrist_keys[i] = (static_cast<uint64_t>(rand()) << 32) | rand();
  }
  g_zobrist_init = true;
}

static int piece_to_index(char type, bool white) {
  int t = 0;
  switch (type) {
    case 'P': t = 0; break; case 'R': t = 1; break; case 'N': t = 2; break;
    case 'B': t = 3; break; case 'K': t = 4; break; case 'Q': t = 5; break;
    default: t = 0; break;
  }
  return t * 2 + (white ? 0 : 1);
}

uint64_t State::hash() const {
  init_zobrist();
  uint64_t h = 0;
  for (int c = 0; c < NUM_COLS; ++c) {
    int maxr = static_cast<int>(cells[static_cast<size_t>(c)].size());
    for (int r = 0; r < maxr && r < ZOBRIST_ROWS; ++r) {
      auto sq = at(c, r);
      if (sq) {
        int idx = (c * ZOBRIST_ROWS + r) * ZOBRIST_PIECES + piece_to_index(sq->type, sq->white);
        h ^= g_zobrist_keys[idx];
      }
    }
  }
  if (white_to_play) h ^= g_zobrist_keys[ZOBRIST_PIECE_KEYS];
  if (prev_move && std::abs(prev_move->to_row - prev_move->from_row) == 2) {
    bool moved_white = !white_to_play;
    int ep_row = moved_white ? prev_move->to_row - 1 : prev_move->to_row + 1;
    int ep_idx = ZOBRIST_PIECE_KEYS + 1 + prev_move->to_col * ZOBRIST_ROWS + ep_row;
    if (ep_idx < ZOBRIST_SIZE) h ^= g_zobrist_keys[ep_idx];
  }
  return h;
}

State::State() {
  cells.resize(NUM_COLS);
  for (int c = 0; c < NUM_COLS; ++c)
    cells[c].resize(static_cast<size_t>(max_row_glinski(c)));
}

void State::set_glinski() {
  variant = Variant::Glinski;
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
    cells[c].resize(static_cast<size_t>(maxr));
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

void State::set_mccooey() {
  variant = Variant::McCooey;
  const char* mcCooey[] = {
    "      ",
    "       ",
    "P      p",
    "RP     pr",
    "QN P   pnq",
    "BBB P  pbbb",
    "K NP   pnk",
    "RP     pr",
    "P      p",
    "       ",
    "      ",
  };
  for (int c = 0; c < NUM_COLS; ++c) {
    int maxr = max_row_mccooey(c);
    cells[c].resize(static_cast<size_t>(maxr));
    const char* row_str = mcCooey[c];
    for (int r = 0; r < maxr; ++r) {
      char ch = r < static_cast<int>(strlen(row_str)) ? row_str[r] : ' ';
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

void State::set_hexofen() {
  variant = Variant::Hexofen;
  const char* hexofen[] = {
    "P    p",      // col 0: 6
    "P     p",     // col 1: 7
    "NP    pb",    // col 2: 8
    "RP     pr",   // col 3: 9
    "BNP   pnq",   // col 4: 10
    "KBP     pbk", // col 5: 11 (K,B,P,4 spaces,p,b,k)
    "QNP   pnb",   // col 6: 10
    "RP     pr",   // col 7: 9
    "BP    pn",    // col 8: 8
    "P     p",     // col 9: 7
    "P    p",      // col 10: 6
  };
  for (int c = 0; c < NUM_COLS; ++c) {
    int maxr = max_row_hexofen(c);
    cells[c].resize(static_cast<size_t>(maxr));
    const char* row_str = hexofen[c];
    for (int r = 0; r < maxr; ++r) {
      char ch = r < static_cast<int>(strlen(row_str)) ? row_str[r] : ' ';
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

bool State::on_board(int col, int storage_row) const {
  return State::on_board(variant, col, storage_row);
}

bool State::on_board(Variant v, int col, int storage_row) {
  if (col < 0 || col >= NUM_COLS) return false;
  int maxr = max_row(v, col);
  return storage_row >= 0 && storage_row < maxr;
}

std::optional<Piece> State::at(int col, int storage_row) const {
  if (!on_board(col, storage_row)) return std::nullopt;
  return cells[static_cast<size_t>(col)][static_cast<size_t>(storage_row)];
}

State::UndoInfo State::make_move(const Move& move) {
  UndoInfo ui;
  ui.prev_move = prev_move;

  Square& from_sq = cells[static_cast<size_t>(move.from_col)][static_cast<size_t>(move.from_row)];
  Square& to_sq = cells[static_cast<size_t>(move.to_col)][static_cast<size_t>(move.to_row)];
  Piece p = *from_sq;
  from_sq = std::nullopt;

  // Detect en passant when protocol sends compact notation without ep flag (e.g. GUI sends "a5b6")
  bool is_ep = move.en_passant;
  if (!is_ep && p.type == 'P' && move.from_col != move.to_col && !to_sq && prev_move) {
    const Move& pm = *prev_move;
    if (std::abs(pm.to_row - pm.from_row) == 2 && pm.to_col == move.to_col) {
      bool moved_white = !white_to_play;
      int ep_row = moved_white ? pm.to_row - 1 : pm.to_row + 1;
      if (ep_row == move.to_row) is_ep = true;
    }
  }

  if (is_ep) {
    int ep_col = move.to_col;
    // Captured pawn is one step from ep square in the direction the captured pawn moved from.
    // White capturing: captured black pawn moved down (lower row) -> it's at to_row - 1.
    // Black capturing: captured white pawn moved up (higher row) -> it's at to_row + 1.
    int ep_row = p.white ? move.to_row - 1 : move.to_row + 1;
    if (on_board(ep_col, ep_row)) {
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

  // Restore en passant capture: captured pawn was at same offset as in make_move.
  int ep_row = p.white ? move.to_row - 1 : move.to_row + 1;
  if (undo.was_ep && undo.captured && on_board(move.to_col, ep_row))
    cells[static_cast<size_t>(move.to_col)][static_cast<size_t>(ep_row)] = *undo.captured;
  else if (undo.captured)
    to_sq = *undo.captured;
}

std::string square_notation(int col, int row) {
  if (col < 0 || col > 25) return "??";
  return std::string(1, static_cast<char>('A' + col)) + std::to_string(row + 1);
}

}  // namespace board
}  // namespace hexchess
