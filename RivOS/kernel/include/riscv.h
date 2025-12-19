#pragma once

#include "types.h"

static inline void w_stvec(u64 x) {
  __asm__ volatile("csrw stvec, %0" : : "r"(x));
}

static inline u64 r_scause(void) {
  u64 x;
  __asm__ volatile("csrr %0, scause" : "=r"(x));
  return x;
}

static inline u64 r_sepc(void) {
  u64 x;
  __asm__ volatile("csrr %0, sepc" : "=r"(x));
  return x;
}

static inline u64 r_stval(void) {
  u64 x;
  __asm__ volatile("csrr %0, stval" : "=r"(x));
  return x;
}
