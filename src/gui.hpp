#pragma once

#include "board.hpp"
#include "search.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace hexchess {
namespace gui {

// shared with render thread
struct GuiState {
  const board::State* board = nullptr;
  const search::Node* root = nullptr;
  std::string status;
  std::string last_player_move;
  std::string last_engine_move;
};

bool is_available();

// start in bg thread, returns immediately. update() to push state
void start();

void stop();

// safe from main thread
void update(const board::State* board, const search::Node* root,
            const std::string& status = {},
            const std::string& last_player_move = {},
            const std::string& last_engine_move = {});

// call periodically. false = user closed window
bool poll_events();

}  // namespace gui
}  // namespace hexchess
