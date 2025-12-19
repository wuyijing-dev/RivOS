#include "console.h"
#include "sbi.h"

void trap_handler(unsigned long scause, unsigned long sepc, unsigned long stval) {
  console_puts("[trap] scause=");
  console_puthex(scause);
  console_puts(" sepc=");
  console_puthex(sepc);
  console_puts(" stval=");
  console_puthex(stval);
  console_puts("\n");

  console_puts("[trap] shutdown\n");
  sbi_shutdown();
}
