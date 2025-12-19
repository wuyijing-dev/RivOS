CROSS_COMPILE ?= $(shell \
	if command -v riscv64-unknown-elf-gcc >/dev/null 2>&1; then echo riscv64-unknown-elf-; \
	elif command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then echo riscv64-linux-gnu-; \
	else echo riscv64-unknown-elf-; fi)

.PHONY: all kernel sim run sim-run clean

all: kernel

kernel:
	$(MAKE) -C kernel

sim:
	$(MAKE) -C simulator

run: kernel
	qemu-system-riscv64 \
		-machine virt \
		-m 256M \
		-nographic \
		-bios default \
		-kernel kernel/build/kernel.elf

sim-run: kernel sim
	./simulator/build/rivos-sim kernel/build/kernel.elf

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C simulator clean
