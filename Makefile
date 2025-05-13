# Compiler and flags
CC = gcc
# LDLIBS is empty now

# Build mode (debug or release)
MODE ?= debug # Default to debug

# Common flags
COMMON_CFLAGS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -std=c11 -pedantic -W -Wall -Wextra
COMMON_CFLAGS += -Wno-unused-parameter -Wno-unused-variable

# Debug specific flags
DEBUG_CFLAGS = -g -ggdb $(COMMON_CFLAGS)

# Release specific flags
RELEASE_CFLAGS = -O2 -Werror $(COMMON_CFLAGS)

# Directories
SRC_DIR = src
BUILD_DIR = build
DEBUG_BUILD_DIR = $(BUILD_DIR)/debug
RELEASE_BUILD_DIR = $(BUILD_DIR)/release

# Output directory based on MODE
ifeq ($(MODE), release)
  CURRENT_CFLAGS = $(RELEASE_CFLAGS)
  CURRENT_BUILD_DIR = $(RELEASE_BUILD_DIR)
else
  CURRENT_CFLAGS = $(DEBUG_CFLAGS)
  CURRENT_BUILD_DIR = $(DEBUG_BUILD_DIR)
endif

# Source files and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(CURRENT_BUILD_DIR)/%.o,$(SRCS))

# Program name
PROG_NAME = fdupes_mime
# Place executable in the current build directory (debug or release)
PROG = $(CURRENT_BUILD_DIR)/$(PROG_NAME)

# Default target: ensure build directory exists before trying to build the program
all: $(CURRENT_BUILD_DIR) $(PROG)

# Link object files to create the executable
# Depends on the build directory existing (added as an order-only prerequisite for $(PROG))
$(PROG): $(OBJS) | $(CURRENT_BUILD_DIR)
	$(CC) $(CURRENT_CFLAGS) $^ -o $@ $(LDLIBS)

# Compile source files into object files
# Ensure build directory exists before compiling (already handled by $(CURRENT_BUILD_DIR) dependency)
$(CURRENT_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(CURRENT_BUILD_DIR)
	$(CC) $(CURRENT_CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Create build directory (target for prerequisites)
$(CURRENT_BUILD_DIR):
	mkdir -p $(CURRENT_BUILD_DIR)

.PHONY: clean all

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
