#include <stdio.h>

#include "rivos_sim/machine.h"
#include "rivos_sim/mem.h"

uint8_t mem_read8(Machine *m, uint64_t addr) {
  if (addr >= RIVOS_SIM_RAM_BASE && addr < RIVOS_SIM_RAM_BASE + m->ram_size) {
    return m->ram[addr - RIVOS_SIM_RAM_BASE];
  }
  return 0;
}

void mem_write8(Machine *m, uint64_t addr, uint8_t val) {
  if (addr >= RIVOS_SIM_RAM_BASE && addr < RIVOS_SIM_RAM_BASE + m->ram_size) {
    m->ram[addr - RIVOS_SIM_RAM_BASE] = val;
    return;
  }

  if (addr == RIVOS_SIM_UART16550_BASE) {
    putchar((int)val);
    fflush(stdout);
    return;
  }
}

uint16_t mem_read16(Machine *m, uint64_t addr) {
  uint16_t v = 0;
  v |= (uint16_t)mem_read8(m, addr);
  v |= (uint16_t)mem_read8(m, addr + 1) << 8;
  return v;
}

uint32_t mem_read32(Machine *m, uint64_t addr) {
  uint32_t v = 0;
  v |= (uint32_t)mem_read8(m, addr);
  v |= (uint32_t)mem_read8(m, addr + 1) << 8;
  v |= (uint32_t)mem_read8(m, addr + 2) << 16;
  v |= (uint32_t)mem_read8(m, addr + 3) << 24;
  return v;
}

uint64_t mem_read64(Machine *m, uint64_t addr) {
  uint64_t v = 0;
  v |= (uint64_t)mem_read32(m, addr);
  v |= (uint64_t)mem_read32(m, addr + 4) << 32;
  return v;
}

void mem_write16(Machine *m, uint64_t addr, uint16_t val) {
  mem_write8(m, addr, (uint8_t)val);
  mem_write8(m, addr + 1, (uint8_t)(val >> 8));
}

void mem_write32(Machine *m, uint64_t addr, uint32_t val) {
  mem_write8(m, addr, (uint8_t)val);
  mem_write8(m, addr + 1, (uint8_t)(val >> 8));
  mem_write8(m, addr + 2, (uint8_t)(val >> 16));
  mem_write8(m, addr + 3, (uint8_t)(val >> 24));
}

void mem_write64(Machine *m, uint64_t addr, uint64_t val) {
  mem_write32(m, addr, (uint32_t)val);
  mem_write32(m, addr + 4, (uint32_t)(val >> 32));
}
