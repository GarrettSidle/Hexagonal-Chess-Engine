//! Piece types and values. Matches C# Utils.Piece.

use std::fmt;

/// Piece kind (P, N, B, R, Q, K)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum PieceKind {
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
}

impl PieceKind {
    pub fn from_char(c: char) -> Option<Self> {
        match c.to_ascii_upper() {
            'P' => Some(PieceKind::Pawn),
            'N' => Some(PieceKind::Knight),
            'B' => Some(PieceKind::Bishop),
            'R' => Some(PieceKind::Rook),
            'Q' => Some(PieceKind::Queen),
            'K' => Some(PieceKind::King),
            _ => None,
        }
    }

    pub fn to_char(self, is_white: bool) -> char {
        let c = match self {
            PieceKind::Pawn => 'P',
            PieceKind::Knight => 'N',
            PieceKind::Bishop => 'B',
            PieceKind::Rook => 'R',
            PieceKind::Queen => 'Q',
            PieceKind::King => 'K',
        };
        if is_white {
            c
        } else {
            c.to_ascii_lower()
        }
    }

    /// Piece value for evaluation (matches C# Utils.Piece.value)
    pub fn value(self) -> i32 {
        match self {
            PieceKind::Pawn => 1,
            PieceKind::Knight => 3,
            PieceKind::Bishop => 3,
            PieceKind::Rook => 5,
            PieceKind::Queen => 9,
            PieceKind::King => 0,
        }
    }
}

trait CharExt {
    fn to_ascii_upper(self) -> char;
    fn to_ascii_lower(self) -> char;
}

impl CharExt for char {
    fn to_ascii_upper(self) -> char {
        if self.is_ascii_lowercase() {
            (self as u8 - 32) as char
        } else {
            self
        }
    }
    fn to_ascii_lower(self) -> char {
        if self.is_ascii_uppercase() {
            (self as u8 + 32) as char
        } else {
            self
        }
    }
}

/// A piece with color
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Piece {
    pub kind: PieceKind,
    pub is_white: bool,
}

impl Piece {
    pub fn new(kind: PieceKind, is_white: bool) -> Self {
        Self { kind, is_white }
    }

    pub fn from_char(c: char) -> Option<Self> {
        let kind = PieceKind::from_char(c)?;
        let is_white = c.is_uppercase();
        Some(Self { kind, is_white })
    }

    pub fn value(self) -> i32 {
        let v = self.kind.value();
        if self.is_white {
            v
        } else {
            -v
        }
    }
}

impl fmt::Display for Piece {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.kind.to_char(self.is_white))
    }
}
