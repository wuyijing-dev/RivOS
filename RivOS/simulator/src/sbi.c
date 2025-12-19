#include <stdint.h>
#include <stdio.h>

#include "rivos_sim/sbi.h"

void sbi_handle(Cpu *cpu) {
  uint64_t ext = cpu->x[17];

  if (ext == 1) {
    uint8_t ch = (uint8_t)cpu->x[10];
    putchar((int)ch);
    fflush(stdout);
    cpu->x[10] = 0;
    return;
  }

  if (ext == 8) {
    cpu->halted = true;
    cpu->x[10] = 0;
    return;
  }

  cpu->x[10] = (uint64_t)-1;
}
