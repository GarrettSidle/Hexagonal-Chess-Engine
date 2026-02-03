#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include "search.hpp"
#include "protocol.hpp"
#include "gephi.hpp"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

static std::string format_game_timestamp(std::chrono::system_clock::time_point tp) {
  auto t = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&t), "%Y-%m-%d_%H-%M-%S");
  return oss.str();
}

int main(int argc, char** argv) {
  std::vector<std::string> board_lines;
  std::string line;
  bool have_board = false;
  bool game_start_time_set = false;
  std::chrono::system_clock::time_point game_start_time;
  int engine_response_count = 0;
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
      auto start_engine_white = [&](const char* pos_name) {
        have_board = true;
        if (!game_start_time_set) {
          game_start_time = std::chrono::system_clock::now();
          game_start_time_set = true;
        }
        engine_plays_white = true;
        std::cout << "position " << pos_name << " (white to move)" << std::endl;
        std::cout << "thinking....." << std::endl;
        hexchess::search::iterative_deepen(*root, 1000, []() { return false; });
        engine_response_count++;
        std::string gephi_path = "gephi_exports/" + format_game_timestamp(game_start_time) + " - Move " + std::to_string(engine_response_count) + ".gexf";
        hexchess::gephi::export_tree(*root, gephi_path);
        if (root->best_move) {
          const auto& mv = *root->best_move;
          auto piece = root->state.at(mv.from_col, mv.from_row);
          auto captured = root->state.at(mv.to_col, mv.to_row);
          char pt = piece ? piece->type : 'P';
          std::optional<char> cap_type = captured ? std::optional<char>(captured->type) : std::nullopt;
          std::string eng_move_str = hexchess::protocol::format_move_long(mv, pt, cap_type);
          std::cout << "Engine Move (White): " << eng_move_str << std::endl;
          root->state.make_move(mv);
          root->best_move = std::nullopt;
          root->children.clear();
        } else {
          std::cout << "Engine Move (White): (none)" << std::endl;
        }
      };
      auto start_position = [&](const char* pos_name) {
        have_board = true;
        if (!game_start_time_set) {
          game_start_time = std::chrono::system_clock::now();
          game_start_time_set = true;
        }
        std::cout << "position " << pos_name << " (white to move)" << std::endl;
      };
      if (lower == "glinski white") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_glinski();
        start_engine_white("glinski");
      } else if (lower == "glinski") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_glinski();
        start_position("glinski");
      } else if (lower == "mccooey white") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_mccooey();
        start_engine_white("mccooey");
      } else if (lower == "mccooey") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_mccooey();
        start_position("mccooey");
      } else if (lower == "hexofen white") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_hexofen();
        start_engine_white("hexofen");
      } else if (lower == "hexofen") {
        root = std::make_unique<hexchess::search::Node>();
        root->state.set_hexofen();
        start_position("hexofen");
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
          if (!game_start_time_set) {
            game_start_time = std::chrono::system_clock::now();
            game_start_time_set = true;
          }
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

    // Player just moved; state.white_to_play is still who moved (white moved if true).
    bool player_played_white = root->state.white_to_play;
    auto piece = root->state.at(move_opt->from_col, move_opt->from_row);
    auto captured = root->state.at(move_opt->to_col, move_opt->to_row);
    char pt = piece ? piece->type : 'P';
    std::optional<char> cap_type = captured ? std::optional<char>(captured->type) : std::nullopt;
    std::string player_notation = hexchess::protocol::format_move_long(*move_opt, pt, cap_type);
    std::cout << "Player Move (" << (player_played_white ? "White" : "Black") << "): " << player_notation << std::endl;

    root->state.make_move(*move_opt);
    root->children.clear();
    root->best_move = std::nullopt;

    std::cout << "thinking....." << std::endl;
    hexchess::search::iterative_deepen(*root, 1000, []() { return false; });
    engine_response_count++;
    std::string gephi_path = "gephi_exports/" + format_game_timestamp(game_start_time) + " - Move " + std::to_string(engine_response_count) + ".gexf";
    hexchess::gephi::export_tree(*root, gephi_path);
    if (root->best_move) {
      const auto& mv = *root->best_move;
      auto eng_piece = root->state.at(mv.from_col, mv.from_row);
      auto eng_captured = root->state.at(mv.to_col, mv.to_row);
      char eng_pt = eng_piece ? eng_piece->type : 'P';
      std::optional<char> eng_cap_type = eng_captured ? std::optional<char>(eng_captured->type) : std::nullopt;
      std::string eng_move_str = hexchess::protocol::format_move_long(mv, eng_pt, eng_cap_type);
      std::cout << "Engine Move (" << (engine_plays_white ? "White" : "Black") << "): " << eng_move_str << std::endl;
      root->state.make_move(mv);
      root->best_move = std::nullopt;
      root->children.clear();
    } else {
      std::cout << "Engine Move (" << (engine_plays_white ? "White" : "Black") << "): (none)" << std::endl;
    }
  }

  return 0;
}
