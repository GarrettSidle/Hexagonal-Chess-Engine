#include "protocol.hpp"
#include "board.hpp"
#include <cctype>
#include <optional>
#include <sstream>
#include <string>

namespace hexchess {
namespace protocol {

static std::optional<std::pair<int, int>> parse_square(const std::string& s) {
  if (s.empty()) return std::nullopt;
  int col = std::tolower(static_cast<unsigned char>(s[0])) - 'a';
  if (col < 0 || col >= board::NUM_COLS) return std::nullopt;
  int row = 0;
  for (size_t i = 1; i < s.size(); ++i) {
    if (s[i] < '0' || s[i] > '9') return std::nullopt;
    row = row * 10 + (s[i] - '0');
  }
  row -= 1;
  if (row < 0) return std::nullopt;
  return std::pair{ col, row };
}

std::optional<board::State> parse_board(const std::vector<std::string>& lines) {
  if (lines.size() < 12) return std::nullopt;
  board::State state;
  for (int c = 0; c < board::NUM_COLS; ++c) {
    int maxr = board::max_row_glinski(c);
    const std::string& line = lines[static_cast<size_t>(c)];
    for (int r = 0; r < maxr; ++r) {
      char ch = r < static_cast<int>(line.size()) ? line[static_cast<size_t>(r)] : ' ';
      if (ch == ' ' || ch == '.' || ch == '\0') {
        state.cells[c][static_cast<size_t>(r)] = std::nullopt;
      } else {
        board::Piece p;
        p.type = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        p.white = (ch >= 'A' && ch <= 'Z');
        state.cells[c][static_cast<size_t>(r)] = p;
      }
    }
  }
  std::string side = lines[11];
  while (!side.empty() && side.back() == '\r') side.pop_back();
  if (side == "white") state.white_to_play = true;
  else if (side == "black") state.white_to_play = false;
  else return std::nullopt;
  state.prev_move = std::nullopt;
  return state;
}

std::optional<board::Move> parse_move(const std::string& s) {
  // PeP from to captured (en passant)
  {
    std::istringstream iss(s);
    std::string t0, t1, t2, t3;
    if (iss >> t0 >> t1 >> t2 >> t3 &&
        t0.size() == 3 && (t0[0] == 'P' || t0[0] == 'p') && (t0[1] == 'e' || t0[1] == 'E') && (t0[2] == 'P' || t0[2] == 'p')) {
      auto from = parse_square(t1);
      auto to = parse_square(t2);
      if (from && to) {
        return board::Move{ from->first, from->second, to->first, to->second, true, true, false };
      }
    }
  }
  // N A3 B4 or NxB A3 B4
  {
    std::istringstream iss(s);
    std::string t0, t1, t2;
    if (iss >> t0 >> t1 >> t2) {
      auto from = parse_square(t1);
      auto to = parse_square(t2);
      if (from && to) {
        return board::Move{ from->first, from->second, to->first, to->second, false, false, false };
      }
    }
  }
  if (s.size() < 4) return std::nullopt;
  size_t pos = 0;
  int c1 = std::tolower(static_cast<unsigned char>(s[pos])) - 'a';
  if (c1 < 0 || c1 >= board::NUM_COLS) return std::nullopt;
  ++pos;
  int r1 = 0;
  while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
    r1 = r1 * 10 + (s[pos] - '0');
    ++pos;
  }
  r1 -= 1;
  if (pos >= s.size() || r1 < 0) return std::nullopt;
  int c2 = std::tolower(static_cast<unsigned char>(s[pos])) - 'a';
  if (c2 < 0 || c2 >= board::NUM_COLS) return std::nullopt;
  ++pos;
  int r2 = 0;
  while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
    r2 = r2 * 10 + (s[pos] - '0');
    ++pos;
  }
  r2 -= 1;
  if (r2 < 0) return std::nullopt;
  return board::Move{ c1, r1, c2, r2, false, false, false };
}

std::string format_move(const board::Move& m) {
  return board::square_notation(m.from_col, m.from_row) + board::square_notation(m.to_col, m.to_row);
}

std::string format_move_long(const board::Move& m, char piece_type, std::optional<char> captured_type) {
  std::string from_sq = board::square_notation(m.from_col, m.from_row);
  std::string to_sq = board::square_notation(m.to_col, m.to_row);
  char p = static_cast<char>(std::toupper(static_cast<unsigned char>(piece_type)));
  if (captured_type) {
    char c = static_cast<char>(std::toupper(static_cast<unsigned char>(*captured_type)));
    return std::string(1, p) + "x" + c + " " + from_sq + " " + to_sq;
  }
  return std::string(1, p) + " " + from_sq + " " + to_sq;
}

std::string format_move_ep(const board::Move& m, bool piece_white) {
  std::string from_sq = board::square_notation(m.from_col, m.from_row);
  std::string to_sq = board::square_notation(m.to_col, m.to_row);
  int cap_row = piece_white ? m.to_row - 1 : m.to_row + 1;
  std::string cap_sq = board::square_notation(m.to_col, cap_row);
  return "PeP " + from_sq + " " + to_sq + " " + cap_sq;
}

}  // namespace protocol
}  // namespace hexchess
