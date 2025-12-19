#pragma once

#include <stdint.h>

#include "rivos_sim/cpu.h"

enum {
  CSR_STVEC = 0x105,
  CSR_SEPC = 0x141,
  CSR_SCAUSE = 0x142,
  CSR_STVAL = 0x143,
};

uint64_t csr_read(Cpu *cpu, uint32_t csr);
void csr_write(Cpu *cpu, uint32_t csr, uint64_t v);
