#pragma once

#include <stdint.h>

static inline uint64_t sign_extend(uint64_t x, int bits) {
  uint64_t m = 1ull << (bits - 1);
  return (x ^ m) - m;
}

static inline uint32_t read_u32_le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static inline uint64_t read_u64_le(const uint8_t *p) {
  uint64_t lo = read_u32_le(p);
  uint64_t hi = read_u32_le(p + 4);
  return lo | (hi << 32);
}
