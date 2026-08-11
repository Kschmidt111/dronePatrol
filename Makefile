CXX      ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -O2
LDFLAGS  ?=

# Optional OpenCV (uncomment when installed; or: make OPENCV=1)
# OPENCV=1
ifeq ($(OPENCV),1)
  CXXFLAGS += $(shell pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv)
  LDFLAGS  += $(shell pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv)
endif

SRC_DIR  := src
BUILD_DIR := build
TARGET   := $(BUILD_DIR)/dronePatrol

SRCS := $(SRC_DIR)/main.cpp
OBJS := $(BUILD_DIR)/main.o

.PHONY: all clean run

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
