//! Board representation. Matches C# Utils.Board (gameBoard[col][row]).

use crate::piece::{Piece, PieceKind};
use std::fmt;

/// Chess variant (0=Glinski, 1=McCooey, 2=Hexofen)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Variant {
    Glinski,
    McCooey,
    Hexofen,
}

impl Variant {
    pub fn from_str(s: &str) -> Option<Self> {
        match s.to_lowercase().as_str() {
            "glinski" | "0" => Some(Variant::Glinski),
            "mccooey" | "1" => Some(Variant::McCooey),
            "hexofen" | "2" => Some(Variant::Hexofen),
            _ => None,
        }
    }
}

/// Coordinate on the board. col in 0..11, row is storage row (array index).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Coord {
    pub col: i32,
    pub row: i32,
}

impl Coord {
    pub fn new(col: i32, row: i32) -> Self {
        Self { col, row }
    }

    /// Convert to notation string (e.g. A1, B2). Matches C# LocNotation.
    pub fn to_notation(&self) -> String {
        let col_char = (b'A' + self.col as u8) as char;
        format!("{}{}", col_char, self.row + 1)
    }

    /// Parse notation like A1, B2
    pub fn from_notation(s: &str) -> Option<Self> {
        let s = s.trim().to_uppercase();
        if s.len() < 2 {
            return None;
        }
        let col = (s.chars().next()? as u8).saturating_sub(b'A') as i32;
        let row: i32 = s[1..].parse().ok()?;
        if row >= 1 {
            Some(Self::new(col, row - 1))
        } else {
            None
        }
    }
}

/// Logical row for move direction math. Matches C# GetLogicalRow/GetStorageRow.
pub fn logical_row(col: i32, storage_row: i32) -> i32 {
    if col <= 5 {
        storage_row
    } else {
        storage_row + col - 5
    }
}

pub fn storage_row(col: i32, logical_row: i32) -> i32 {
    if col <= 5 {
        logical_row
    } else {
        logical_row + 5 - col
    }
}

/// Board layout: gameBoard[col][row]. Each column has variable row count (hex layout).
pub struct Board {
    pub variant: Variant,
    /// Columns 0..11, each with variable-length rows. None = empty square.
    pub grid: Vec<Vec<Option<Piece>>>,
    pub white_to_play: bool,
}

impl Board {
    /// Create empty board structure for variant (no pieces).
    pub fn empty(variant: Variant) -> Self {
        let row_counts = variant_row_counts(variant);
        let grid: Vec<Vec<Option<Piece>>> = row_counts
            .iter()
            .map(|&n| vec![None; n as usize])
            .collect();
        Self {
            variant,
            grid,
            white_to_play: true,
        }
    }

    /// Set up starting position for the variant.
    pub fn new_startpos(variant: Variant) -> Self {
        let mut board = Self::empty(variant);
        let setup = variant_starting_setup(variant);
        for (col, rows) in setup.iter().enumerate() {
            for (row, &pc) in rows.iter().enumerate() {
                if pc != ' ' {
                    board.grid[col][row] = Piece::from_char(pc);
                }
            }
        }
        board.white_to_play = true;
        board
    }

    /// Get piece at (col, row). Returns None if out of bounds.
    pub fn at(&self, col: i32, row: i32) -> Option<Option<Piece>> {
        if col < 0 || col >= self.grid.len() as i32 {
            return None;
        }
        let col = col as usize;
        if row < 0 || row >= self.grid[col].len() as i32 {
            return None;
        }
        let row = row as usize;
        Some(self.grid[col][row])
    }

    /// Get piece at coord.
    pub fn at_coord(&self, c: Coord) -> Option<Option<Piece>> {
        self.at(c.col, c.row)
    }

    /// Set piece at (col, row). Does nothing if out of bounds.
    pub fn set(&mut self, col: i32, row: i32, piece: Option<Piece>) {
        if col < 0 || col >= self.grid.len() as i32 {
            return;
        }
        let col = col as usize;
        if row < 0 || row >= self.grid[col].len() as i32 {
            return;
        }
        let row = row as usize;
        self.grid[col][row] = piece;
    }

    /// Iterate all valid (col, row) coordinates.
    pub fn all_coords(&self) -> impl Iterator<Item = Coord> + '_ {
        self.grid.iter().enumerate().flat_map(|(col, rows)| {
            rows.iter()
                .enumerate()
                .map(move |(row, _)| Coord::new(col as i32, row as i32))
        })
    }
}

fn variant_row_counts(v: Variant) -> Vec<i32> {
    match v {
        Variant::Glinski => vec![6, 7, 8, 9, 10, 11, 10, 9, 8, 7, 6],
        Variant::McCooey => vec![6, 7, 8, 9, 10, 11, 10, 9, 8, 7, 6],
        Variant::Hexofen => vec![6, 7, 8, 9, 10, 11, 10, 9, 8, 7, 6],
    }
}

/// Starting setup: grid[col][row] = char (' ' = empty, uppercase=white, lowercase=black)
fn variant_starting_setup(v: Variant) -> Vec<Vec<char>> {
    match v {
        Variant::Glinski => {
            vec![
                vec![' ', ' ', ' ', ' ', ' ', ' '],
                vec!['P', ' ', ' ', ' ', ' ', ' ', 'p'],
                vec!['R', 'P', ' ', ' ', ' ', ' ', 'p', 'r'],
                vec!['N', ' ', 'P', ' ', ' ', ' ', 'p', ' ', 'n'],
                vec!['Q', ' ', ' ', 'P', ' ', ' ', 'p', ' ', ' ', 'q'],
                vec!['B', 'B', 'B', ' ', 'P', ' ', 'p', ' ', 'b', 'b', 'b'],
                vec!['K', ' ', ' ', 'P', ' ', ' ', 'p', ' ', ' ', 'k'],
                vec!['N', ' ', 'P', ' ', ' ', ' ', 'p', ' ', 'n'],
                vec!['R', 'P', ' ', ' ', ' ', ' ', 'p', 'r'],
                vec!['P', ' ', ' ', ' ', ' ', ' ', 'p'],
                vec![' ', ' ', ' ', ' ', ' ', ' '],
            ]
        }
        Variant::McCooey => {
            vec![
                vec![' ', ' ', ' ', ' ', ' ', ' '],
                vec![' ', ' ', ' ', ' ', ' ', ' ', ' '],
                vec!['P', ' ', ' ', ' ', ' ', ' ', ' ', 'p'],
                vec!['R', 'P', ' ', ' ', ' ', ' ', ' ', 'p', 'r'],
                vec!['Q', 'N', 'P', ' ', ' ', ' ', ' ', 'p', 'n', 'q'],
                vec!['B', 'B', 'B', 'P', ' ', ' ', ' ', 'p', 'b', 'b', 'b'],
                vec!['K', 'N', 'P', ' ', ' ', ' ', ' ', 'p', 'n', 'k'],
                vec!['R', 'P', ' ', ' ', ' ', ' ', ' ', 'p', 'r'],
                vec!['P', ' ', ' ', ' ', ' ', ' ', ' ', 'p'],
                vec![' ', ' ', ' ', ' ', ' ', ' ', ' '],
                vec![' ', ' ', ' ', ' ', ' ', ' '],
            ]
        }
        Variant::Hexofen => {
            vec![
                vec!['P', ' ', ' ', ' ', ' ', 'p'],
                vec!['P', ' ', ' ', ' ', ' ', ' ', 'p'],
                vec!['N', 'P', ' ', ' ', ' ', ' ', 'p', 'b'],
                vec!['R', 'P', ' ', ' ', ' ', ' ', ' ', 'p', 'r'],
                vec!['B', 'N', 'P', ' ', ' ', ' ', ' ', 'p', 'n', 'q'],
                vec!['K', 'B', 'P', ' ', ' ', ' ', ' ', ' ', 'p', 'b', 'k'],
                vec!['Q', 'N', 'P', ' ', ' ', ' ', ' ', 'p', 'n', 'b'],
                vec!['R', 'P', ' ', ' ', ' ', ' ', ' ', 'p', 'r'],
                vec!['B', 'P', ' ', ' ', ' ', ' ', 'p', 'n'],
                vec!['P', ' ', ' ', ' ', ' ', ' ', 'p'],
                vec!['P', ' ', ' ', ' ', ' ', 'p'],
            ]
        }
    }
}

impl fmt::Debug for Board {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "Board({:?}, white_to_play={})",
            self.variant, self.white_to_play
        )
    }
}
