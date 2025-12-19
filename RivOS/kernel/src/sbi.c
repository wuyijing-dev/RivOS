#include "sbi.h"

static inline long sbi_ecall(long a0, long a1, long a2, long a3,
                            long a4, long a5, long a6, long a7) {
  register long _a0 __asm__("a0") = a0;
  register long _a1 __asm__("a1") = a1;
  register long _a2 __asm__("a2") = a2;
  register long _a3 __asm__("a3") = a3;
  register long _a4 __asm__("a4") = a4;
  register long _a5 __asm__("a5") = a5;
  register long _a6 __asm__("a6") = a6;
  register long _a7 __asm__("a7") = a7;
  __asm__ volatile("ecall"
                   : "+r"(_a0)
                   : "r"(_a1), "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5), "r"(_a6),
                     "r"(_a7)
                   : "memory");
  return _a0;
}

void sbi_console_putchar(int ch) {
  (void)sbi_ecall(ch, 0, 0, 0, 0, 0, 0, 1);
}

void sbi_shutdown(void) {
  (void)sbi_ecall(0, 0, 0, 0, 0, 0, 0, 8);
  for (;;) {
    __asm__ volatile("wfi");
  }
}
