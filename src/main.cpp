#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include "search.hpp"
#include "protocol.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <string>

int main() {
  std::vector<std::string> board_lines;
  std::string line;
  bool have_board = false;
  bool engine_plays_white = false;
  std::optional<hexchess::board::State> state_opt;
  std::unique_ptr<hexchess::search::Node> root;

  while (std::getline(std::cin, line)) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();

    if (line.empty()) continue;

    if (line == "quit") break;

    if (!have_board) {
      std::string lower;
      lower.resize(line.size());
      std::transform(line.begin(), line.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      if (lower == "glinski white") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_glinski();
        have_board = true;
        engine_plays_white = true;
        std::cout << "position glinski (white to move)" << std::endl;
        std::cout << "thinking..." << std::endl;
        hexchess::search::iterative_deepen(*root, 4, nullptr);
        if (root->best_move) {
          const auto& mv = *root->best_move;
          auto piece = root->state.at(mv.from_col, mv.from_row);
          auto captured = root->state.at(mv.to_col, mv.to_row);
          char pt = piece ? piece->type : 'P';
          std::optional<char> cap_type = captured ? std::optional<char>(captured->type) : std::nullopt;
          std::cout << "bestmove " << hexchess::protocol::format_move_long(mv, pt, cap_type) << std::endl;
          root->state.make_move(mv);
          root->best_move = std::nullopt;
        } else {
          std::cout << "bestmove (none)" << std::endl;
        }
      } else if (lower == "glinski") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_glinski();
        have_board = true;
        std::cout << "position glinski (white to move)" << std::endl;
      } else {
        board_lines.push_back(line);
        if (board_lines.size() == 12) {
          state_opt = hexchess::protocol::parse_board(board_lines);
          if (!state_opt) {
            std::cerr << "invalid board" << std::endl;
            board_lines.clear();
            continue;
          }
          have_board = true;
          root = std::make_unique<hexchess::search::Node>();
          root->state = *state_opt;
        }
      }
      continue;
    }

    // Accept input when it's the opponent's turn (we respond with our move)
    bool opponent_to_play = (engine_plays_white && !root->state.white_to_play) || (!engine_plays_white && root->state.white_to_play);
    if (!opponent_to_play) continue;

    std::string move_str;
    if (line.size() >= 5 && line.substr(0, 5) == "move ") {
      move_str = line.substr(5);
    } else if (line.size() >= 4) {
      move_str = line;
    } else {
      continue;
    }

    auto move_opt = hexchess::protocol::parse_move(move_str);
    if (!move_opt) {
      std::cerr << "invalid move" << std::endl;
      continue;
    }

    root->state.make_move(*move_opt);
    root->children.clear();
    root->best_move = std::nullopt;

    std::cout << "thinking..." << std::endl;
    hexchess::search::iterative_deepen(*root, 4, nullptr);
    if (root->best_move) {
      const auto& mv = *root->best_move;
      auto piece = root->state.at(mv.from_col, mv.from_row);
      auto captured = root->state.at(mv.to_col, mv.to_row);
      char pt = piece ? piece->type : 'P';
      std::optional<char> cap_type = captured ? std::optional<char>(captured->type) : std::nullopt;
      std::cout << "bestmove " << hexchess::protocol::format_move_long(mv, pt, cap_type) << std::endl;
      root->state.make_move(mv);
      root->best_move = std::nullopt;
    } else {
      std::cout << "bestmove (none)" << std::endl;
    }
  }

  return 0;
}
