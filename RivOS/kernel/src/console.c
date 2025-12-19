#include "console.h"
#include "sbi.h"

void console_putc(char c) {
  if (c == '\n') {
    sbi_console_putchar('\r');
  }
  sbi_console_putchar((int)c);
}

void console_puts(const char *s) {
  for (; *s; s++) {
    console_putc(*s);
  }
}

static char hex_digit(unsigned v) {
  v &= 0xF;
  return (v < 10) ? (char)('0' + v) : (char)('a' + (v - 10));
}

void console_puthex(u64 x) {
  console_puts("0x");
  for (int i = 60; i >= 0; i -= 4) {
    console_putc(hex_digit((unsigned)(x >> i)));
  }
}
