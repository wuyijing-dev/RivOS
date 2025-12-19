#include <stdbool.h>
#include <stdint.h>

#include "rivos_sim/common.h"
#include "rivos_sim/cpu.h"
#include "rivos_sim/csr.h"
#include "rivos_sim/mem.h"
#include "rivos_sim/sbi.h"

static void trap(Cpu *cpu, uint64_t scause, uint64_t sepc, uint64_t stval) {
  cpu->scause = scause;
  cpu->sepc = sepc;
  cpu->stval = stval;
  cpu->pc = cpu->stvec;
}

static inline uint64_t sext32(uint32_t v) {
  return (uint64_t)(int64_t)(int32_t)v;
} 

void cpu_exec_one(struct Machine *m, Cpu *cpu) {
  uint64_t pc = cpu->pc;

  if ((pc & 3ull) != 0) {
    trap(cpu, 0, pc, pc);
    return;
  }

  uint32_t insn = mem_read32((Machine *)m, pc);
  cpu->pc = pc + 4;

  uint32_t opcode = insn & 0x7F;
  uint32_t rd = (insn >> 7) & 0x1F;
  uint32_t funct3 = (insn >> 12) & 0x7;
  uint32_t rs1 = (insn >> 15) & 0x1F;
  uint32_t rs2 = (insn >> 20) & 0x1F;
  uint32_t funct7 = (insn >> 25) & 0x7F;

  uint64_t x1 = cpu->x[rs1];
  uint64_t x2 = cpu->x[rs2];

  switch (opcode) {
  case 0x37: {
    uint64_t imm = (uint64_t)(insn & 0xFFFFF000u);
    if (rd)
      cpu->x[rd] = imm;
    break;
  }
  case 0x17: {
    uint64_t imm = (uint64_t)(insn & 0xFFFFF000u);
    if (rd)
      cpu->x[rd] = pc + imm;
    break;
  }
  case 0x6F: {
    uint64_t imm = 0;
    imm |= (uint64_t)((insn >> 31) & 0x1) << 20;
    imm |= (uint64_t)((insn >> 21) & 0x3FF) << 1;
    imm |= (uint64_t)((insn >> 20) & 0x1) << 11;
    imm |= (uint64_t)((insn >> 12) & 0xFF) << 12;
    imm = sign_extend(imm, 21);
    if (rd)
      cpu->x[rd] = cpu->pc;
    cpu->pc = pc + imm;
    break;
  }
  case 0x67: {
    uint64_t imm = sign_extend((uint64_t)(insn >> 20), 12);
    uint64_t t = cpu->pc;
    cpu->pc = (x1 + imm) & ~1ull;
    if (rd)
      cpu->x[rd] = t;
    break;
  }
  case 0x63: {
    uint64_t imm = 0;
    imm |= (uint64_t)((insn >> 31) & 0x1) << 12;
    imm |= (uint64_t)((insn >> 25) & 0x3F) << 5;
    imm |= (uint64_t)((insn >> 8) & 0xF) << 1;
    imm |= (uint64_t)((insn >> 7) & 0x1) << 11;
    imm = sign_extend(imm, 13);

    bool take = false;
    switch (funct3) {
    case 0x0:
      take = (x1 == x2);
      break;
    case 0x1:
      take = (x1 != x2);
      break;
    case 0x4:
      take = ((int64_t)x1 < (int64_t)x2);
      break;
    case 0x5:
      take = ((int64_t)x1 >= (int64_t)x2);
      break;
    case 0x6:
      take = (x1 < x2);
      break;
    case 0x7:
      take = (x1 >= x2);
      break;
    default:
      trap(cpu, 2, pc, insn);
      return;
    }

    if (take)
      cpu->pc = pc + imm;
    break;
  }
  case 0x03: {
    uint64_t imm = sign_extend((uint64_t)(insn >> 20), 12);
    uint64_t addr = x1 + imm;

    switch (funct3) {
    case 0x0: {
      uint8_t v = mem_read8((Machine *)m, addr);
      if (rd)
        cpu->x[rd] = sign_extend(v, 8);
      break;
    }
    case 0x1: {
      uint16_t v = mem_read16((Machine *)m, addr);
      if (rd)
        cpu->x[rd] = sign_extend(v, 16);
      break;
    }
    case 0x2: {
      uint32_t v = mem_read32((Machine *)m, addr);
      if (rd)
        cpu->x[rd] = sign_extend(v, 32);
      break;
    }
    case 0x3: {
      uint64_t v = mem_read64((Machine *)m, addr);
      if (rd)
        cpu->x[rd] = v;
      break;
    }
    case 0x4: {
      uint8_t v = mem_read8((Machine *)m, addr);
      if (rd)
        cpu->x[rd] = v;
      break;
    }
    case 0x5: {
      uint16_t v = mem_read16((Machine *)m, addr);
      if (rd)
        cpu->x[rd] = v;
      break;
    }
    case 0x6: {
      uint32_t v = mem_read32((Machine *)m, addr);
      if (rd)
        cpu->x[rd] = v;
      break;
    }
    default:
      trap(cpu, 2, pc, insn);
      return;
    }

    break;
  }
  case 0x23: {
    uint64_t imm = 0;
    imm |= (uint64_t)((insn >> 25) & 0x7F) << 5;
    imm |= (uint64_t)((insn >> 7) & 0x1F);
    imm = sign_extend(imm, 12);
    uint64_t addr = x1 + imm;

    switch (funct3) {
    case 0x0:
      mem_write8((Machine *)m, addr, (uint8_t)x2);
      break;
    case 0x1:
      mem_write16((Machine *)m, addr, (uint16_t)x2);
      break;
    case 0x2:
      mem_write32((Machine *)m, addr, (uint32_t)x2);
      break;
    case 0x3:
      mem_write64((Machine *)m, addr, x2);
      break;
    default:
      trap(cpu, 2, pc, insn);
      return;
    }

    break;
  }
  case 0x13: {
    uint64_t imm = sign_extend((uint64_t)(insn >> 20), 12);

    switch (funct3) {
    case 0x0:
      if (rd)
        cpu->x[rd] = x1 + imm;
      break;
    case 0x2:
      if (rd)
        cpu->x[rd] = ((int64_t)x1 < (int64_t)imm) ? 1 : 0;
      break;
    case 0x3:
      if (rd)
        cpu->x[rd] = (x1 < (uint64_t)imm) ? 1 : 0;
      break;
    case 0x4:
      if (rd)
        cpu->x[rd] = x1 ^ (uint64_t)imm;
      break;
    case 0x6:
      if (rd)
        cpu->x[rd] = x1 | (uint64_t)imm;
      break;
    case 0x7:
      if (rd)
        cpu->x[rd] = x1 & (uint64_t)imm;
      break;
    case 0x1: {
      uint32_t shamt = (insn >> 20) & 0x3F;
      if (rd)
        cpu->x[rd] = x1 << shamt;
      break;
    }
    case 0x5: {
      uint32_t shamt = (insn >> 20) & 0x3F;
      if ((insn >> 30) & 1) {
        if (rd)
          cpu->x[rd] = (uint64_t)((int64_t)x1 >> shamt);
      } else {
        if (rd)
          cpu->x[rd] = x1 >> shamt;
      }
      break;
    }
    default:
      trap(cpu, 2, pc, insn);
      return;
    }

    break;
  }
  case 0x1B: {
    uint64_t imm = sign_extend((uint64_t)(insn >> 20), 12);

    switch (funct3) {
    case 0x0: {
      uint32_t r = (uint32_t)(x1 + imm);
      if (rd)
        cpu->x[rd] = sext32(r);
      break;
    }
    case 0x1: {
      uint32_t shamt = (insn >> 20) & 0x1F;
      uint32_t r = (uint32_t)x1 << shamt;
      if (rd)
        cpu->x[rd] = sext32(r);
      break;
    }
    case 0x5: {
      uint32_t shamt = (insn >> 20) & 0x1F;
      uint32_t r;
      if ((insn >> 30) & 1) {
        r = (uint32_t)((int32_t)(uint32_t)x1 >> shamt);
      } else {
        r = (uint32_t)x1 >> shamt;
      }
      if (rd)
        cpu->x[rd] = sext32(r);
      break;
    }
    default:
      trap(cpu, 2, pc, insn);
      return;
    }

    break;
  }
  case 0x33: {
    switch (funct3) {
    case 0x0:
      if (funct7 == 0x20) {
        if (rd)
          cpu->x[rd] = x1 - x2;
      } else {
        if (rd)
          cpu->x[rd] = x1 + x2;
      }
      break;
    case 0x1:
      if (rd)
        cpu->x[rd] = x1 << (x2 & 0x3F);
      break;
    case 0x2:
      if (rd)
        cpu->x[rd] = ((int64_t)x1 < (int64_t)x2) ? 1 : 0;
      break;
    case 0x3:
      if (rd)
        cpu->x[rd] = (x1 < x2) ? 1 : 0;
      break;
    case 0x4:
      if (rd)
        cpu->x[rd] = x1 ^ x2;
      break;
    case 0x5:
      if (funct7 == 0x20) {
        if (rd)
          cpu->x[rd] = (uint64_t)((int64_t)x1 >> (x2 & 0x3F));
      } else {
        if (rd)
          cpu->x[rd] = x1 >> (x2 & 0x3F);
      }
      break;
    case 0x6:
      if (rd)
        cpu->x[rd] = x1 | x2;
      break;
    case 0x7:
      if (rd)
        cpu->x[rd] = x1 & x2;
      break;
    default:
      trap(cpu, 2, pc, insn);
      return;
    }

    break;
  }
  case 0x3B: {
    switch (funct3) {
    case 0x0: {
      uint32_t r;
      if (funct7 == 0x20) {
        r = (uint32_t)((uint32_t)x1 - (uint32_t)x2);
      } else {
        r = (uint32_t)((uint32_t)x1 + (uint32_t)x2);
      }
      if (rd)
        cpu->x[rd] = sext32(r);
      break;
    }
    case 0x1: {
      uint32_t r = (uint32_t)x1 << (x2 & 0x1F);
      if (rd)
        cpu->x[rd] = sext32(r);
      break;
    }
    case 0x5: {
      uint32_t shamt = (uint32_t)(x2 & 0x1F);
      uint32_t r;
      if (funct7 == 0x20) {
        r = (uint32_t)((int32_t)(uint32_t)x1 >> shamt);
      } else {
        r = (uint32_t)x1 >> shamt;
      }
      if (rd)
        cpu->x[rd] = sext32(r);
      break;
    }
    default:
      trap(cpu, 2, pc, insn);
      return;
    }

    break;
  }
  case 0x73: {
    if (funct3 == 0x0) {
      uint32_t imm = insn >> 20;
      if (imm == 0) {
        sbi_handle(cpu);
      } else if (imm == 1) {
        trap(cpu, 3, pc, 0);
      } else if (imm == 0x105) {
        /* wfi: treat as a no-op for MVP */
      } else {
        trap(cpu, 2, pc, insn);
      }
      break;
    }

    uint32_t csr = insn >> 20;
    uint64_t old = csr_read(cpu, csr);

    if (rd)
      cpu->x[rd] = old;

    if (funct3 == 0x1) {
      csr_write(cpu, csr, x1);
    } else if (funct3 == 0x2) {
      csr_write(cpu, csr, old | x1);
    } else if (funct3 == 0x3) {
      csr_write(cpu, csr, old & ~x1);
    } else {
      trap(cpu, 2, pc, insn);
      return;
    }

    break;
  }
  default:
    trap(cpu, 2, pc, insn);
    return;
  }

  cpu->x[0] = 0;
}
