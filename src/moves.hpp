#pragma once

#include "board.hpp"
#include <vector>

namespace hexchess {
namespace moves {

// Generate all legal moves for the side to move.
std::vector<board::Move> generate(const board::State& state);

// Is (col, storage_row) a white/black pawn starting square for the variant?
bool is_starting_pawn_white(const board::State& state, int col, int storage_row);
bool is_starting_pawn_black(const board::State& state, int col, int storage_row);

}  // namespace moves
}  // namespace hexchess
