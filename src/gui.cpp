#ifdef USE_GUI

#include "gui.hpp"
#include "protocol.hpp"
#include <mutex>
#include <cmath>
#include <sstream>

#ifdef _WIN32
#include <SDL.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

namespace hexchess {
namespace gui {

using namespace board;

static std::mutex g_state_mutex;
static GuiState g_state;
static std::atomic<bool> g_quit_requested{false};
static std::atomic<bool> g_running{false};
static std::thread g_gui_thread;

static const int WIN_BOARD_W = 720;
static const int WIN_BOARD_H = 560;
static const int WIN_TREE_W = 480;
static const int WIN_TREE_H = 560;
static const int HEX_RADIUS = 22;

// Hex center for (col, row) - same layout as C# Board.buildBoard
static void hex_center(int col, int row, int& cx, int& cy) {
  const double hexShortRadius = (HEX_RADIUS / 2.0) * 1.732050808;  // sqrt(3)
  double startX = WIN_BOARD_W / 2.0;
  double startY = WIN_BOARD_H / 2.0 + HEX_RADIUS * 8.0;
  double x = startX + (col * hexShortRadius * 0.9 * 2);
  double y = startY - (row * hexShortRadius * 2) + ((col < 6 ? col : (10 - col)) * hexShortRadius);
  cx = static_cast<int>(x);
  cy = static_cast<int>(y);
}

static int max_row_for_variant(Variant v, int col) {
  if (v == Variant::McCooey) return max_row_mccooey(col);
  return max_row_glinski(col);
}

static void draw_hex(SDL_Renderer* ren, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b) {
  SDL_SetRenderDrawColor(ren, r, g, b, 255);
  for (int i = 0; i < 6; ++i) {
    double a1 = i * 60 * 3.14159265 / 180.0;
    double a2 = (i + 1) * 60 * 3.14159265 / 180.0;
    int x1 = cx + static_cast<int>(radius * std::cos(a1));
    int y1 = cy - static_cast<int>(radius * std::sin(a1));
    int x2 = cx + static_cast<int>(radius * std::cos(a2));
    int y2 = cy - static_cast<int>(radius * std::sin(a2));
    SDL_RenderDrawLine(ren, x1, y1, x2, y2);
  }
}

// Draw filled hex by drawing 6 triangles from center (SDL2 has no built-in filled polygon)
static void fill_hex(SDL_Renderer* ren, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b) {
  SDL_SetRenderDrawColor(ren, r, g, b, 255);
  for (int i = 0; i < 6; ++i) {
    double a1 = i * 60 * 3.14159265 / 180.0;
    double a2 = (i + 1) * 60 * 3.14159265 / 180.0;
    int x1 = cx + static_cast<int>(radius * std::cos(a1));
    int y1 = cy - static_cast<int>(radius * std::sin(a1));
    int x2 = cx + static_cast<int>(radius * std::cos(a2));
    int y2 = cy - static_cast<int>(radius * std::sin(a2));
    SDL_RenderDrawLine(ren, cx, cy, x1, y1);
    SDL_RenderDrawLine(ren, x1, y1, x2, y2);
    SDL_RenderDrawLine(ren, x2, y2, cx, cy);
  }
}

static void draw_hex_cell(SDL_Renderer* ren, int cx, int cy, Uint8 r, Uint8 g, Uint8 b) {
  fill_hex(ren, cx, cy, HEX_RADIUS, r, g, b);
  draw_hex(ren, cx, cy, HEX_RADIUS, 80, 70, 60);  // outline
}

static char piece_char(const std::optional<Piece>& p) {
  if (!p) return ' ';
  return p->white ? p->type : static_cast<char>(std::tolower(static_cast<unsigned char>(p->type)));
}

static void render_board(SDL_Renderer* ren, SDL_Window* win, TTF_Font* font, const State* state) {
  SDL_SetRenderDrawColor(ren, 250, 243, 232, 255);
  SDL_RenderClear(ren);

  if (!state) return;

  int rowMax = 5;
  int rowColorCode = 0;
  int colColorCode = 0;
  Variant v = state->variant;

  for (int col = 0; col < NUM_COLS; ++col) {
    int maxr = max_row_for_variant(v, col);
    for (int row = 0; row < maxr; ++row) {
      int cx, cy;
      hex_center(col, row, cx, cy);

      int code = (rowColorCode + colColorCode) % 3;
      Uint8 r, g, b;
      switch (code) {
        case 0: r = 209; g = 139; b = 71; break;
        case 2: r = 232; g = 171; b = 111; break;
        default: r = 255; g = 206; b = 158; break;
      }
      draw_hex_cell(ren, cx, cy, r, g, b);

      auto p = state->at(col, row);
      if (p) {
        char ch = piece_char(p);
        std::string s(1, ch);
        SDL_Color fg = { 40, 40, 40, 255 };
        if (font) {
          SDL_Surface* surf = TTF_RenderText_Solid(font, s.c_str(), fg);
          if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
            if (tex) {
              int tw = surf->w, th = surf->h;
              SDL_Rect dst = { cx - tw/2 - 4, cy - th/2 - 4, tw + 8, th + 8 };
              SDL_RenderCopy(ren, tex, nullptr, &dst);
              SDL_DestroyTexture(tex);
            }
            SDL_FreeSurface(surf);
          }
        }
      }
      colColorCode++;
      if (col == 0 || (row - col == 5 && col > 0 && col < 6)) { /* row label */ }
    }
    if (col < 5) { rowMax++; rowColorCode++; }
    else { rowMax--; rowColorCode--; }
    colColorCode = 0;
  }

  SDL_RenderPresent(ren);
}

static void render_tree_recursive(SDL_Renderer* ren, TTF_Font* font, const search::Node* node,
                                  int x, int y, int depth, int max_depth, int& max_y) {
  if (!node || depth > max_depth) return;
  if (y > max_y) max_y = y;

  std::ostringstream oss;
  oss << "score:" << node->best_score;
  if (node->best_move) {
    oss << " " << square_notation(node->best_move->from_col, node->best_move->from_row)
        << "-" << square_notation(node->best_move->to_col, node->best_move->to_row);
  }
  std::string s = oss.str();
  SDL_Color fg = { 60, 50, 45, 255 };
  if (font) {
    SDL_Surface* surf = TTF_RenderText_Blended(font, s.c_str(), fg);
    if (surf) {
      SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
      if (tex) {
        SDL_Rect dst = { x, y, surf->w, surf->h };
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
      }
      SDL_FreeSurface(surf);
    }
  }
  int next_y = y + 22;
  int child_x = x + 180;
  for (const auto& [m, child] : node->children) {
    if (child) {
      render_tree_recursive(ren, font, child.get(), child_x, next_y, depth + 1, max_depth, max_y);
      next_y = max_y + 12;
    }
  }
}

static void render_tree(SDL_Renderer* ren, SDL_Window* win, TTF_Font* font,
                        const std::string& status,
                        const std::string& last_player, const std::string& last_engine,
                        const search::Node* root) {
  SDL_SetRenderDrawColor(ren, 255, 251, 245, 255);
  SDL_RenderClear(ren);

  int y = 10;
  SDL_Color fg = { 60, 50, 45, 255 };
  auto draw_text = [&](const std::string& s) {
    if (s.empty() || !font) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, s.c_str(), fg);
    if (surf) {
      SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
      if (tex) {
        SDL_Rect dst = { 10, y, surf->w, surf->h };
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
      }
      SDL_FreeSurface(surf);
    }
    y += 24;
  };
  draw_text("Search Tree");
  draw_text(status);
  if (!last_player.empty()) draw_text("Player: " + last_player);
  if (!last_engine.empty()) draw_text("Engine: " + last_engine);
  y += 10;

  if (root) {
    int max_y = y;
    render_tree_recursive(ren, font, root, 10, y, 0, 6, max_y);
  }

  SDL_RenderPresent(ren);
}

static void gui_loop() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) return;
  if (TTF_Init() != 0) { SDL_Quit(); return; }

  SDL_Window* win_board = SDL_CreateWindow("Hex Chess - Board", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                           WIN_BOARD_W, WIN_BOARD_H, SDL_WINDOW_SHOWN);
  SDL_Window* win_tree = SDL_CreateWindow("Hex Chess - Search Tree", SDL_WINDOWPOS_CENTERED + WIN_BOARD_W + 20, SDL_WINDOWPOS_CENTERED,
                                          WIN_TREE_W, WIN_TREE_H, SDL_WINDOW_SHOWN);
  if (!win_board || !win_tree) {
    if (win_board) SDL_DestroyWindow(win_board);
    if (win_tree) SDL_DestroyWindow(win_tree);
    TTF_Quit();
    SDL_Quit();
    return;
  }

  SDL_Renderer* ren_board = SDL_CreateRenderer(win_board, -1, SDL_RENDERER_ACCELERATED);
  SDL_Renderer* ren_tree = SDL_CreateRenderer(win_tree, -1, SDL_RENDERER_ACCELERATED);
  TTF_Font* font = nullptr;
#ifdef _WIN32
  font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 14);
#elif __APPLE__
  font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 14);
#else
  font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
#endif

  if (!ren_board || !ren_tree) {
    if (ren_board) SDL_DestroyRenderer(ren_board);
    if (ren_tree) SDL_DestroyRenderer(ren_tree);
    if (font) TTF_CloseFont(font);
    SDL_DestroyWindow(win_board);
    SDL_DestroyWindow(win_tree);
    TTF_Quit();
    SDL_Quit();
    return;
  }

  g_running = true;
  while (!g_quit_requested) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        g_quit_requested = true;
    }

    GuiState local;
    {
      std::lock_guard<std::mutex> lock(g_state_mutex);
      local = g_state;
    }
    render_board(ren_board, win_board, font, local.board);
    render_tree(ren_tree, win_tree, font, local.status, local.last_player_move, local.last_engine_move, local.root);
    SDL_Delay(80);
  }
  g_running = false;

  SDL_DestroyRenderer(ren_board);
  SDL_DestroyRenderer(ren_tree);
  if (font) TTF_CloseFont(font);
  SDL_DestroyWindow(win_board);
  SDL_DestroyWindow(win_tree);
  TTF_Quit();
  SDL_Quit();
}

bool is_available() { return true; }

void start() {
  if (g_running) return;
  g_quit_requested = false;
  g_gui_thread = std::thread(gui_loop);
}

void stop() {
  g_quit_requested = true;
  if (g_gui_thread.joinable())
    g_gui_thread.join();
}

void update(const State* board, const search::Node* root,
            const std::string& status,
            const std::string& last_player_move,
            const std::string& last_engine_move) {
  std::lock_guard<std::mutex> lock(g_state_mutex);
  g_state.board = board;
  g_state.root = root;
  g_state.status = status;
  g_state.last_player_move = last_player_move;
  g_state.last_engine_move = last_engine_move;
}

bool poll_events() {
  return !g_quit_requested;
}

}  // namespace gui
}  // namespace hexchess

#else  // !USE_GUI

#include "gui.hpp"

namespace hexchess {
namespace gui {

bool is_available() { return false; }
void start() {}
void stop() {}
void update(const board::State*, const search::Node*, const std::string&, const std::string&, const std::string&) {}
bool poll_events() { return true; }

}  // namespace gui
}  // namespace hexchess

#endif
