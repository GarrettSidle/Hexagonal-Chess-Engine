CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -I.
SRC = src/board.cpp src/moves.cpp src/eval.cpp src/search.cpp src/protocol.cpp src/main.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = engine

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)
