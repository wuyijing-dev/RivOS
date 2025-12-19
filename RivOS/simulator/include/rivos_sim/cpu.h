#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint64_t pc;
  uint64_t x[32];

  uint64_t stvec;
  uint64_t sepc;
  uint64_t scause;
  uint64_t stval;

  bool halted;
} Cpu;

struct Machine;

void cpu_exec_one(struct Machine *m, Cpu *cpu);
