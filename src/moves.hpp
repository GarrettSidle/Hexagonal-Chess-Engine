#pragma once

#include "board.hpp"
#include <optional>
#include <vector>

namespace hexchess {
namespace moves {

// Generate all legal moves for the side to move.
std::vector<board::Move> generate(const board::State& state);

// Order moves for alpha-beta: hash move first, then captures (MVV-LVA), then killers, then rest.
void order_moves(std::vector<board::Move>& moves, const board::State& state,
    std::optional<board::Move> hash_move,
    std::optional<board::Move> killer1, std::optional<board::Move> killer2);

// Is (col, storage_row) a white/black pawn starting square for the variant?
bool is_starting_pawn_white(const board::State& state, int col, int storage_row);
bool is_starting_pawn_black(const board::State& state, int col, int storage_row);

}  // namespace moves
}  // namespace hexchess
