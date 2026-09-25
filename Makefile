CXX ?= c++
CXXFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude

RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)

BUILD_DIR := build
GAME := $(BUILD_DIR)/isometric_snake
TESTS := $(BUILD_DIR)/snake_tests

.PHONY: all run test clean check-raylib

all: $(GAME)

check-raylib:
	@pkg-config --exists raylib 2>/dev/null || { \
		echo "raylib was not found."; \
		echo "macOS:  brew install raylib pkg-config"; \
		echo "Linux:  install raylib development files and pkg-config"; \
		echo "Windows: use MSYS2/MinGW or the CMake build"; \
		exit 1; \
	}

$(GAME): src/main.cpp src/game.cpp include/game.hpp | check-raylib
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CXXFLAGS) -std=c++20 \
		src/main.cpp src/game.cpp -o $@ $(RAYLIB_LIBS)

$(TESTS): tests/game_tests.cpp src/game.cpp include/game.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 \
		tests/game_tests.cpp src/game.cpp -o $@

run: $(GAME)
	./$(GAME)

test: $(TESTS)
	./$(TESTS)

clean:
	rm -f $(GAME) $(TESTS)

