//! Static evaluation. Positive = better for white, negative = better for black.
//! Matches C# board.evaluation convention.

use crate::board::Board;
use crate::piece::Piece;

/// Material-only evaluation. Returns score in centipawns (or piece-value units).
/// Positive = white advantage, negative = black advantage.
pub fn evaluate(board: &Board) -> i32 {
    let mut score = 0i32;
    for c in board.all_coords() {
        if let Some(Some(piece)) = board.at_coord(c) {
            score += piece.value();
        }
    }
    score
}
