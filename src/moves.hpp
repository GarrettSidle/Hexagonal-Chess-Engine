#pragma once

#include "board.hpp"
#include <optional>
#include <vector>

namespace hexchess {
namespace moves {

// all legal moves for side to move
std::vector<board::Move> generate(const board::State& state);

// hash first, then captures (mvv-lva), killers, rest
void order_moves(std::vector<board::Move>& moves, const board::State& state,
    std::optional<board::Move> hash_move,
    std::optional<board::Move> killer1, std::optional<board::Move> killer2);

// white/black pawn start square for variant
bool is_starting_pawn_white(const board::State& state, int col, int storage_row);
bool is_starting_pawn_black(const board::State& state, int col, int storage_row);

}  // namespace moves
}  // namespace hexchess
