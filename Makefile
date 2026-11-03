CXX := g++

CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude

BUILD_DIR := build
BIN_DIR := bin
TARGET := $(BIN_DIR)/compiler

SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)