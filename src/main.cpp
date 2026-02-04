#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include "search.hpp"
#include "protocol.hpp"
#include "gephi.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

static std::string get_executable_dir() {
#ifdef _WIN32
  char buf[MAX_PATH];
  if (GetModuleFileNameA(nullptr, buf, sizeof(buf))) {
    std::filesystem::path p(buf);
    if (p.has_parent_path()) {
      return p.parent_path().string();
    }
  }
#endif
  return std::filesystem::current_path().string();
}

static std::string format_game_timestamp(std::chrono::system_clock::time_point tp) {
  auto t = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
#ifdef _WIN32
  struct std::tm tm_buf;
  if (localtime_s(&tm_buf, &t) == 0) {
    oss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
  } else {
    oss << "0000-00-00_00-00-00";
  }
#else
  oss << std::put_time(std::localtime(&t), "%Y-%m-%d_%H-%M-%S");
#endif
  return oss.str();
}

// Pondering: I/O thread reads stdin so main loop can run search while waiting for opponent.
static std::queue<std::string> g_input_queue;
static std::mutex g_queue_mutex;
static std::atomic<bool> g_quit_requested{false};
static std::atomic<bool> g_io_thread_done{false};

static void io_thread_func() {
  std::string line;
  while (!g_quit_requested && std::getline(std::cin, line)) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    std::lock_guard<std::mutex> lock(g_queue_mutex);
    g_input_queue.push(std::move(line));
  }
  g_io_thread_done = true;
}

static std::string get_next_line(bool opponent_to_play, std::unique_ptr<hexchess::search::Node>* ponder_root) {
  while (true) {
    {
      std::lock_guard<std::mutex> lock(g_queue_mutex);
      if (!g_input_queue.empty()) {
        std::string line = std::move(g_input_queue.front());
        g_input_queue.pop();
        return line;
      }
    }
    if (g_io_thread_done) return "quit";

    if (opponent_to_play && ponder_root && *ponder_root) {
      // Use large node budget for pondering so we search deeper while waiting; stop lambda exits when input arrives
      const int ponder_nodes = 100000;
      hexchess::search::iterative_deepen(**ponder_root, ponder_nodes, []() {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        return !g_input_queue.empty() || g_quit_requested;
      });
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

int main(int argc, char** argv) {
  std::string exe_dir = get_executable_dir();
  hexchess::gephi::set_export_base_dir(exe_dir);
  std::cerr << "Gephi exports: " << (std::filesystem::path(exe_dir) / "gephi_exports").string() << std::endl;

  std::vector<std::string> board_lines;
  std::string line;
  bool have_board = false;
  bool game_start_time_set = false;
  std::chrono::system_clock::time_point game_start_time;
  int engine_response_count = 0;
  bool engine_plays_white = false;
  std::optional<hexchess::board::State> state_opt;
  std::unique_ptr<hexchess::search::Node> root;
  std::unique_ptr<hexchess::search::Node> ponder_root;

  std::thread io_thread(io_thread_func);

  while (true) {
    bool opponent_to_play = have_board && root &&
        ((engine_plays_white && !root->state.white_to_play) || (!engine_plays_white && root->state.white_to_play));
    line = get_next_line(opponent_to_play, &ponder_root);

    if (line.empty()) continue;

    if (line == "quit") {
      g_quit_requested = true;
      break;
    }

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
        ponder_root = std::make_unique<hexchess::search::Node>();
        ponder_root->state = root->state;
      };
      auto start_position = [&](const char* pos_name) {
        have_board = true;
        if (!game_start_time_set) {
          game_start_time = std::chrono::system_clock::now();
          game_start_time_set = true;
        }
        std::cout << "position " << pos_name << " (white to move)" << std::endl;
        ponder_root = std::make_unique<hexchess::search::Node>();
        ponder_root->state = root->state;
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
          ponder_root = std::make_unique<hexchess::search::Node>();
          ponder_root->state = root->state;
        }
      }
      continue;
    }

    // Accept input when it's the opponent's turn (we respond with our move)
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

    // Try to reuse ponder result before clearing
    hexchess::search::Node* ponder_child = ponder_root ? hexchess::search::find_child(*ponder_root, *move_opt) : nullptr;

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

    bool reused_ponder = false;
    if (ponder_child) {
      root->state = ponder_child->state;
      root->children = std::move(ponder_child->children);
      root->best_move = ponder_child->best_move;
      root->best_score = ponder_child->best_score;
      reused_ponder = root->best_move.has_value();
    }
    ponder_root.reset();  // Clear after we're done with it

    if (!reused_ponder) {
      std::cout << "thinking....." << std::endl;
      hexchess::search::iterative_deepen(*root, 1000, []() { return false; });
    }
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
      ponder_root = std::make_unique<hexchess::search::Node>();
      ponder_root->state = root->state;
    } else {
      std::cout << "Engine Move (" << (engine_plays_white ? "White" : "Black") << "): (none)" << std::endl;
    }
  }

  g_quit_requested = true;
  if (io_thread.joinable()) io_thread.join();

  return 0;
}
