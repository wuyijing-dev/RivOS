#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rivos_sim/cpu.h"
#include "rivos_sim/elf.h"
#include "rivos_sim/machine.h"

static void die(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static void usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s <kernel.elf> [max_insns]\n"
          "\n"
          "Runs a minimal RV64 interpreter with virt UART16550 output and\n"
          "minimal CSR/trap + legacy SBI (console_putchar/shutdown) emulation.\n",
          argv0);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argv[0]);
    return 2;
  }

  const char *elf_path = argv[1];
  uint64_t max_insns = 50ull * 1000ull * 1000ull;
  if (argc >= 3) {
    max_insns = strtoull(argv[2], NULL, 0);
    if (max_insns == 0) {
      die("invalid max_insns");
    }
  }

  Machine m;
  m.ram_size = (size_t)RIVOS_SIM_RAM_SIZE;
  m.ram = (uint8_t *)calloc(1, m.ram_size);
  if (!m.ram) {
    die("failed to allocate RAM");
  }

  uint64_t entry = 0;
  if (!load_elf(&m, elf_path, &entry)) {
    fprintf(stderr, "failed to load ELF: %s\n", strerror(errno));
    free(m.ram);
    return 1;
  }

  Cpu cpu;
  memset(&cpu, 0, sizeof(cpu));
  cpu.pc = entry;

  for (uint64_t i = 0; i < max_insns && !cpu.halted; i++) {
    cpu_exec_one((struct Machine *)&m, &cpu);
  }

  free(m.ram);
  return 0;
}
