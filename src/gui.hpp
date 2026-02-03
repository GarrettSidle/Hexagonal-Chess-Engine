#pragma once

#include "board.hpp"
#include "search.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace hexchess {
namespace gui {

// GUI state shared with render thread
struct GuiState {
  const board::State* board = nullptr;
  const search::Node* root = nullptr;
  std::string status;           // e.g. "Thinking... depth 3"
  std::string last_player_move;
  std::string last_engine_move;
};

// Returns true if GUI subsystem is available (built with USE_GUI).
bool is_available();

// Start GUI in a background thread. Returns immediately. Call update() to push state.
// No-op if !is_available().
void start();

// Stop GUI and join thread. No-op if !is_available().
void stop();

// Update displayed state. Safe to call from main thread. No-op if !is_available().
void update(const board::State* board, const search::Node* root,
            const std::string& status = {},
            const std::string& last_player_move = {},
            const std::string& last_engine_move = {});

// Process GUI events (call periodically from main loop to keep window responsive).
// Returns false if user closed the window (engine should exit).
bool poll_events();

}  // namespace gui
}  // namespace hexchess
