#pragma once

#include <stddef.h>
#include <stdint.h>

enum {
  RIVOS_SIM_RAM_BASE = 0x80000000ull,
  RIVOS_SIM_RAM_SIZE = 128ull * 1024ull * 1024ull,

  RIVOS_SIM_UART16550_BASE = 0x10000000ull,
};

typedef struct {
  uint8_t *ram;
  size_t ram_size;
} Machine;
