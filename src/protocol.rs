//! Protocol parsing. Commands: position, eval, go, quit.

use crate::board::{Board, Coord, Variant};
use crate::eval;
use std::io::{BufRead, BufReader, Write};

/// Parse position command:
///   position <variant> startpos [stm w|b]
///   position <variant> pieces <sq>:<pc>;... [stm w|b]
pub fn parse_position(args: &[&str]) -> Option<Board> {
    if args.len() < 2 {
        return None;
    }
    let variant = Variant::from_str(args[0])?;
    let mut board = if args[1] == "startpos" {
        Board::new_startpos(variant)
    } else if args[1] == "pieces" {
        let mut b = Board::empty(variant);
        // args[2] may contain "A1:K;A2:P;..." or we have multiple args
        let pieces_str = args.get(2).copied().unwrap_or("");
        for part in pieces_str.split(';') {
            let part = part.trim();
            if part.is_empty() {
                continue;
            }
            let mut it = part.split(':');
            let sq = it.next()?;
            let pc = it.next()?.chars().next()?;
            let coord = Coord::from_notation(sq)?;
            b.set(coord.col, coord.row, crate::piece::Piece::from_char(pc));
        }
        b
    } else {
        return None;
    };

    // Parse stm w|b
    let args_str = args.join(" ");
    if args_str.contains("stm b") {
        board.white_to_play = false;
    } else {
        board.white_to_play = true;
    }
    Some(board)
}

/// Handle a single command. Returns false to quit.
pub fn handle_command(line: &str, board: &mut Option<Board>, out: &mut impl Write) -> bool {
    let line = line.trim();
    if line.is_empty() {
        return true;
    }
    let parts: Vec<&str> = line.split_whitespace().collect();
    if parts.is_empty() {
        return true;
    }
    let cmd = parts[0].to_lowercase();
    let args: Vec<&str> = parts[1..].to_vec();

    match cmd.as_str() {
        "quit" | "exit" => return false,
        "position" => {
            if let Some(b) = parse_position(&args) {
                *board = Some(b);
            } else {
                let _ = writeln!(out, "error invalid position");
            }
        }
        "eval" => {
            if let Some(ref b) = board {
                let score = eval::evaluate(b);
                let _ = writeln!(out, "eval {}", score);
            } else {
                let _ = writeln!(out, "error no position set");
            }
        }
        "uci" => {
            let _ = writeln!(out, "id name Hex Chess Engine");
            let _ = writeln!(out, "id author Hex Chess");
            let _ = writeln!(out, "uciok");
        }
        _ => {
            let _ = writeln!(out, "info unknown command {}", cmd);
        }
    }
    true
}

/// Main protocol loop: read lines from stdin, handle commands, write to stdout.
pub fn run_loop() {
    let stdin = std::io::stdin();
    let mut reader = BufReader::new(stdin.lock());
    let mut stdout = std::io::stdout();
    let mut line = String::new();
    let mut board: Option<Board> = None;

    loop {
        line.clear();
        match reader.read_line(&mut line) {
            Ok(0) => break,
            Ok(_) => {
                if !handle_command(&line, &mut board, &mut stdout) {
                    break;
                }
                let _ = stdout.flush();
            }
            Err(_) => break,
        }
    }
}
