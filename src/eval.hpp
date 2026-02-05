#pragma once

#include "board.hpp"

namespace hexchess {
namespace eval {

// P=1 R=5 N=3 B=3 K=0 Q=9 (same as gui)
int piece_value(char type);

// positive = white better
int evaluate(const board::State& state);

// was last move king capture
bool is_terminal(const board::State& state, const board::Move& move_just_made);

}  // namespace eval
}  // namespace hexchess
