#include "rivos_sim/cpu.h"
#include "rivos_sim/csr.h"

uint64_t csr_read(Cpu *cpu, uint32_t csr) {
  switch (csr) {
  case CSR_STVEC:
    return cpu->stvec;
  case CSR_SEPC:
    return cpu->sepc;
  case CSR_SCAUSE:
    return cpu->scause;
  case CSR_STVAL:
    return cpu->stval;
  default:
    return 0;
  }
}

void csr_write(Cpu *cpu, uint32_t csr, uint64_t v) {
  switch (csr) {
  case CSR_STVEC:
    cpu->stvec = v;
    break;
  case CSR_SEPC:
    cpu->sepc = v;
    break;
  case CSR_SCAUSE:
    cpu->scause = v;
    break;
  case CSR_STVAL:
    cpu->stval = v;
    break;
  default:
    break;
  }
}
