# CAUCHY - Lock-Free Distributed State Convergence
# Build System

CC ?= gcc
AR ?= ar

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
LIB_DIR := lib
BIN_DIR := bin
TEST_DIR := tests
BENCH_DIR := benchmarks

# Compiler flags
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -I$(INC_DIR)
CFLAGS += -fPIC -D_POSIX_C_SOURCE=200809L

# Architecture-specific flags
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    CFLAGS += -mcx16  # Enable CMPXCHG16B
endif

# Build type
BUILD ?= release

ifeq ($(BUILD),debug)
    CFLAGS += -O0 -g3 -DDEBUG -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
else ifeq ($(BUILD),release)
    CFLAGS += -O3 -DNDEBUG -flto
    LDFLAGS += -flto
else ifeq ($(BUILD),profile)
    CFLAGS += -O2 -g -pg
    LDFLAGS += -pg
endif

# Source files
CORE_SRC := $(wildcard $(SRC_DIR)/core/*.c)
CRDT_SRC := $(wildcard $(SRC_DIR)/crdt/*.c)
NET_SRC := $(wildcard $(SRC_DIR)/net/*.c)
ALL_SRC := $(CORE_SRC) $(CRDT_SRC) $(NET_SRC)

# Object files
CORE_OBJ := $(CORE_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CRDT_OBJ := $(CRDT_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
NET_OBJ := $(NET_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
ALL_OBJ := $(CORE_OBJ) $(CRDT_OBJ) $(NET_OBJ)

# Library outputs
STATIC_LIB := $(LIB_DIR)/libcauchy.a
SHARED_LIB := $(LIB_DIR)/libcauchy.so

# Test sources
TEST_SRC := $(wildcard $(TEST_DIR)/*.c)
TEST_BIN := $(TEST_SRC:$(TEST_DIR)/%.c=$(BIN_DIR)/%)

# Phony targets
.PHONY: all clean test bench lib static shared dirs

# Default target
all: dirs lib

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR)/core $(BUILD_DIR)/crdt $(BUILD_DIR)/net
	@mkdir -p $(LIB_DIR) $(BIN_DIR)

# Build static library
static: dirs $(STATIC_LIB)

$(STATIC_LIB): $(ALL_OBJ)
	$(AR) rcs $@ $^

# Build shared library
shared: dirs $(SHARED_LIB)

$(SHARED_LIB): $(ALL_OBJ)
	$(CC) -shared $(LDFLAGS) -o $@ $^ -lpthread

# Build both libraries
lib: static shared

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build tests
test: lib $(TEST_BIN)
	@for t in $(TEST_BIN); do echo "Running $$t..."; $$t || exit 1; done

$(BIN_DIR)/%: $(TEST_DIR)/%.c $(STATIC_LIB)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -lcauchy -lpthread -o $@

# Build benchmarks
bench: lib
	@echo "Benchmarks not yet implemented"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) $(BIN_DIR)

# Install (default: /usr/local)
PREFIX ?= /usr/local
install: lib
	install -d $(PREFIX)/lib $(PREFIX)/include/cauchy
	install -m 644 $(STATIC_LIB) $(PREFIX)/lib/
	install -m 755 $(SHARED_LIB) $(PREFIX)/lib/
	install -m 644 $(INC_DIR)/cauchy/*.h $(PREFIX)/include/cauchy/

# Generate compile_commands.json for IDE support
compile_commands:
	@echo "[" > compile_commands.json
	@for f in $(ALL_SRC); do \
		echo "  {\"directory\": \"$(PWD)\", \"file\": \"$$f\", \"command\": \"$(CC) $(CFLAGS) -c $$f\"},"; \
	done >> compile_commands.json
	@echo "]" >> compile_commands.json

# Print configuration
info:
	@echo "CC:       $(CC)"
	@echo "CFLAGS:   $(CFLAGS)"
	@echo "BUILD:    $(BUILD)"
	@echo "ARCH:     $(UNAME_M)"
	@echo "Sources:  $(ALL_SRC)"

