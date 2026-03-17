# Modern C Makefile for autoresearch (sp.h edition)

DEBUG ?= 0
BUILD_TYPE ?= release

NAME := autoresearch
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
BIN_DIR := bin

CC := cc
LINTER := clang-tidy
FORMATTER := clang-format

CFLAGS := -std=gnu17 -Wall -Wextra -Wpedantic
CPPFLAGS := -I$(INCLUDE_DIR)
LDFLAGS :=

ifeq ($(DEBUG),1)
  CFLAGS += -g -O0 -DDEBUG
else
  ifeq ($(BUILD_TYPE),release)
    CFLAGS += -O3 -DNDEBUG
  else ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -g -O2 -DDEBUG
  else ifeq ($(BUILD_TYPE),profile)
    CFLAGS += -g -O2 -pg
  endif
endif

COMMON_SRCS := $(SRC_DIR)/sp_impl.c $(SRC_DIR)/ar_common.c
PREPARE_SRCS := $(SRC_DIR)/prepare.c $(COMMON_SRCS)
TRAIN_SRCS := $(SRC_DIR)/train.c $(COMMON_SRCS)

PREPARE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(PREPARE_SRCS))
TRAIN_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(TRAIN_SRCS))

all: dir $(BIN_DIR)/prepare $(BIN_DIR)/train

$(BIN_DIR)/prepare: $(PREPARE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/train: $(TRAIN_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

dir:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

run-prepare: $(BIN_DIR)/prepare
	$(BIN_DIR)/prepare

run-train: $(BIN_DIR)/train
	$(BIN_DIR)/train

lint:
	@if command -v $(LINTER) >/dev/null 2>&1; then \
		$(LINTER) $(SRC_DIR)/*.c -- $(CPPFLAGS) $(CFLAGS); \
	else \
		echo "warning: $(LINTER) not found"; \
	fi

format:
	@if command -v $(FORMATTER) >/dev/null 2>&1; then \
		$(FORMATTER) -i $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h; \
	else \
		echo "warning: $(FORMATTER) not found"; \
	fi

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

help:
	@echo "Targets: all, run-prepare, run-train, lint, format, clean"

.PHONY: all dir run-prepare run-train lint format clean help
