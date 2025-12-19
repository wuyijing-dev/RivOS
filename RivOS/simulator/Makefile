CC ?= gcc

CFLAGS := -Wall -Wextra -O2 -g -std=c11

BUILD_DIR := build

SRCS := \
	src/main.c \
	src/cpu.c \
	src/mem.c \
	src/csr.c \
	src/sbi.c \
	src/elf.c
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

.PHONY: all clean

all: $(BUILD_DIR)/rivos-sim

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/src

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/rivos-sim: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	@rm -rf $(BUILD_DIR)
