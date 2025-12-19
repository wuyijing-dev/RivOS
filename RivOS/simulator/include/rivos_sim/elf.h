#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rivos_sim/machine.h"

bool load_elf(Machine *m, const char *path, uint64_t *entry_out);
