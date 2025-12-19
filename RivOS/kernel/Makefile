CROSS_COMPILE ?= $(shell \
	if command -v riscv64-unknown-elf-gcc >/dev/null 2>&1; then echo riscv64-unknown-elf-; \
	elif command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then echo riscv64-linux-gnu-; \
	else echo riscv64-unknown-elf-; fi)

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc
OBJDUMP := $(CROSS_COMPILE)objdump

ifeq ($(shell command -v $(CC) 2>/dev/null),)
$(error RISC-V toolchain not found. Install riscv64-unknown-elf-gcc or riscv64-linux-gnu-gcc, or run: make CROSS_COMPILE=riscv64-linux-gnu-)
endif

BUILD_DIR := build

CFLAGS := -Wall -Wextra -O2 -g \
	-ffreestanding -fno-builtin -fno-omit-frame-pointer \
	-march=rv64ima_zicsr_zifencei -mabi=lp64 -mcmodel=medany \
	-Iinclude

LDFLAGS := -nostdlib -Wl,--build-id=none -T linker.ld

SRCS := \
	src/entry.S \
	src/trap_entry.S \
	src/sbi.c \
	src/console.c \
	src/kernel.c \
	src/trap.c

OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))
OBJS := $(OBJS:.S=.o)

.PHONY: all clean disasm

all: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/src

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

disasm: $(BUILD_DIR)/kernel.elf
	$(OBJDUMP) -d $(BUILD_DIR)/kernel.elf | cat

clean:
	@rm -rf $(BUILD_DIR)
