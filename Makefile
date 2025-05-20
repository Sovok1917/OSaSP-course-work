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
# All build artifacts will go directly into BUILD_DIR
BUILD_DIR = build

# Set CFLAGS based on MODE
ifeq ($(MODE), release)
  CURRENT_CFLAGS = $(RELEASE_CFLAGS)
else
  CURRENT_CFLAGS = $(DEBUG_CFLAGS)
endif

# Source files and object files
# Object files will now be in $(BUILD_DIR)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Program name
PROG_NAME = fdupes_mime
# Place executable directly in $(BUILD_DIR)
PROG = $(BUILD_DIR)/$(PROG_NAME)

# Default target: ensure build directory exists before trying to build the program
all: $(BUILD_DIR) $(PROG)

# Link object files to create the executable
# Depends on the build directory existing
$(PROG): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CURRENT_CFLAGS) $^ -o $@ $(LDLIBS)

# Compile source files into object files
# Object files go into $(BUILD_DIR)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CURRENT_CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Create build directory (target for prerequisites)
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: clean all

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
