#pragma once

#include "board.hpp"
#include <optional>
#include <string>
#include <vector>

namespace hexchess {
namespace protocol {

// 11 lines (one per col) then white/black. upper=white lower=black . or space=empty
std::optional<board::State> parse_board(const std::vector<std::string>& lines);

// a1b2 or N A3 B4 or NxB A3 B4 or PeP from to captured
std::optional<board::Move> parse_move(const std::string& s);

std::string format_move(const board::Move& m);

// NxB A3 B4 style (piece, x+captured if capture, from, to)
std::string format_move_long(const board::Move& m, char piece_type, std::optional<char> captured_type);

// PeP from to captured square. piece_white = moving pawn
std::string format_move_ep(const board::Move& m, bool piece_white);

}  // namespace protocol
}  // namespace hexchess
