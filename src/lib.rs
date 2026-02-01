pub mod board;
pub mod eval;
pub mod piece;
pub mod protocol;

pub use board::{Board, Coord, Variant};
pub use eval::evaluate;
pub use piece::{Piece, PieceKind};
pub use protocol::{handle_command, parse_position, run_loop};
