#pragma once

#include "board.hpp"
#include <optional>
#include <string>
#include <vector>

namespace hexchess {
namespace protocol {

// Parse custom board dump: 11 lines (one per column), then "white" or "black".
// Each line: characters for each cell (uppercase = white, lowercase = black, . or space = empty).
// Returns state and side to move (true = white). Empty optional on parse error.
std::optional<board::State> parse_board(const std::vector<std::string>& lines);

// Parse move: "a1b2" or "N A3 B4" or "NxB A3 B4" or "PeP {from} {to} {captured}" for en passant.
std::optional<board::Move> parse_move(const std::string& s);

// Format move as "a1b2" (compact).
std::string format_move(const board::Move& m);

// Format move as "NxB A3 B4" (piece, x+captured if capture, from square, to square).
std::string format_move_long(const board::Move& m, char piece_type, std::optional<char> captured_type);

// Format en passant as "PeP {pawn start} {pawn end} {captured pawn square}". piece_white = moving pawn is white.
std::string format_move_ep(const board::Move& m, bool piece_white);

}  // namespace protocol
}  // namespace hexchess
