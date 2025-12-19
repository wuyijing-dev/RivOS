#pragma once

#include <stdint.h>

#include "rivos_sim/machine.h"

uint8_t mem_read8(Machine *m, uint64_t addr);
uint16_t mem_read16(Machine *m, uint64_t addr);
uint32_t mem_read32(Machine *m, uint64_t addr);
uint64_t mem_read64(Machine *m, uint64_t addr);

void mem_write8(Machine *m, uint64_t addr, uint8_t val);
void mem_write16(Machine *m, uint64_t addr, uint16_t val);
void mem_write32(Machine *m, uint64_t addr, uint32_t val);
void mem_write64(Machine *m, uint64_t addr, uint64_t val);
