cmake_minimum_required(VERSION 3.14)
project(hexchess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(engine
  src/main.cpp
  src/board.cpp
  src/moves.cpp
  src/eval.cpp
  src/search.cpp
  src/protocol.cpp
)

target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
