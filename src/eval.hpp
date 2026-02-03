#pragma once

#include "board.hpp"

namespace hexchess {
namespace eval {

// Piece values (same as GUI): P=1, R=5, N=3, B=3, K=0, Q=9.
int piece_value(char type);

// Evaluate position: positive = white better, negative = black better.
// Terminal: if a king was just captured, return large win for capturer.
int evaluate(const board::State& state);

// Was the last move a king capture? (game over)
bool is_terminal(const board::State& state, const board::Move& move_just_made);

}  // namespace eval
}  // namespace hexchess
