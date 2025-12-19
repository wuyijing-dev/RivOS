#include "console.h"
#include "riscv.h"
#include "sbi.h"

extern char __bss_start;
extern char __bss_end;

extern void trap_entry(void);

static void bss_clear(void) {
  for (char *p = &__bss_start; p < &__bss_end; p++) {
    *p = 0;
  }
}

void kmain(void) {
  bss_clear();

  w_stvec((unsigned long)trap_entry);

  console_puts("RivOS MVP boot (RV64 + OpenSBI + S-mode + virt)\n");
  console_puts("Trigger trap by ebreak...\n");

  __asm__ volatile("ebreak");

  console_puts("Should not reach here\n");
  sbi_shutdown();
}
