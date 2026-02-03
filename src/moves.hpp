#pragma once

#include "board.hpp"
#include <vector>

namespace hexchess {
namespace moves {

// Generate all legal moves for the side to move.
std::vector<board::Move> generate(const board::State& state);

// Glinski: is (col, storage_row) a white/black pawn starting square?
bool is_starting_pawn_white_glinski(int col, int storage_row);
bool is_starting_pawn_black_glinski(int col, int storage_row);

}  // namespace moves
}  // namespace hexchess
