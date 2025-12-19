#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_BASE 0x80000000ull
#define RAM_SIZE (128ull * 1024ull * 1024ull)

#define UART16550_BASE 0x10000000ull

#define CSR_STVEC 0x105
#define CSR_SEPC 0x141
#define CSR_SCAUSE 0x142
#define CSR_STVAL 0x143

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

static inline void write_u64_le(uint8_t *p, uint64_t v) {
  p[0] = (uint8_t)(v);
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
  p[4] = (uint8_t)(v >> 32);
  p[5] = (uint8_t)(v >> 40);
  p[6] = (uint8_t)(v >> 48);
  p[7] = (uint8_t)(v >> 56);
}

typedef struct {
  uint64_t pc;
  uint64_t x[32];

  uint64_t stvec;
  uint64_t sepc;
  uint64_t scause;
  uint64_t stval;

  bool halted;
} Cpu;

typedef struct {
  uint8_t *ram;
  size_t ram_size;
} Machine;

static void die(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static void trap(Cpu *cpu, uint64_t scause, uint64_t sepc, uint64_t stval) {
  cpu->scause = scause;
  cpu->sepc = sepc;
  cpu->stval = stval;
  cpu->pc = cpu->stvec;
}

static uint8_t mem_read8(Machine *m, uint64_t addr) {
  if (addr >= RAM_BASE && addr < RAM_BASE + m->ram_size) {
    return m->ram[addr - RAM_BASE];
  }
  return 0;
}

static void mem_write8(Machine *m, uint64_t addr, uint8_t val) {
  if (addr >= RAM_BASE && addr < RAM_BASE + m->ram_size) {
    m->ram[addr - RAM_BASE] = val;
    return;
  }

  if (addr == UART16550_BASE) {
    putchar((int)val);
    fflush(stdout);
    return;
  }
}

static uint16_t mem_read16(Machine *m, uint64_t addr) {
  uint16_t v = 0;
  v |= (uint16_t)mem_read8(m, addr);
  v |= (uint16_t)mem_read8(m, addr + 1) << 8;
  return v;
}

static uint32_t mem_read32(Machine *m, uint64_t addr) {
  uint32_t v = 0;
  v |= (uint32_t)mem_read8(m, addr);
  v |= (uint32_t)mem_read8(m, addr + 1) << 8;
  v |= (uint32_t)mem_read8(m, addr + 2) << 16;
  v |= (uint32_t)mem_read8(m, addr + 3) << 24;
  return v;
}

static uint64_t mem_read64(Machine *m, uint64_t addr) {
  uint64_t v = 0;
  v |= (uint64_t)mem_read32(m, addr);
  v |= (uint64_t)mem_read32(m, addr + 4) << 32;
  return v;
}

static void mem_write16(Machine *m, uint64_t addr, uint16_t val) {
  mem_write8(m, addr, (uint8_t)val);
  mem_write8(m, addr + 1, (uint8_t)(val >> 8));
}

static void mem_write32(Machine *m, uint64_t addr, uint32_t val) {
  mem_write8(m, addr, (uint8_t)val);
  mem_write8(m, addr + 1, (uint8_t)(val >> 8));
  mem_write8(m, addr + 2, (uint8_t)(val >> 16));
  mem_write8(m, addr + 3, (uint8_t)(val >> 24));
}

static void mem_write64(Machine *m, uint64_t addr, uint64_t val) {
  mem_write32(m, addr, (uint32_t)val);
  mem_write32(m, addr + 4, (uint32_t)(val >> 32));
}

static uint64_t csr_read(Cpu *cpu, uint32_t csr) {
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

static void csr_write(Cpu *cpu, uint32_t csr, uint64_t v) {
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

static void sbi_handle(Cpu *cpu) {
  uint64_t ext = cpu->x[17];
  if (ext == 1) {
    uint8_t ch = (uint8_t)cpu->x[10];
    putchar((int)ch);
    fflush(stdout);
    cpu->x[10] = 0;
    return;
  }

  if (ext == 8) {
    cpu->halted = true;
    cpu->x[10] = 0;
    return;
  }

  cpu->x[10] = (uint64_t)-1;
}

static void exec_one(Machine *m, Cpu *cpu) {
  uint64_t pc = cpu->pc;

  if ((pc & 3ull) != 0) {
    trap(cpu, 0, pc, pc);
    return;
  }

  uint32_t insn = mem_read32(m, pc);
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
      uint8_t v = mem_read8(m, addr);
      if (rd)
        cpu->x[rd] = sign_extend(v, 8);
      break;
    }
    case 0x1: {
      uint16_t v = mem_read16(m, addr);
      if (rd)
        cpu->x[rd] = sign_extend(v, 16);
      break;
    }
    case 0x2: {
      uint32_t v = mem_read32(m, addr);
      if (rd)
        cpu->x[rd] = sign_extend(v, 32);
      break;
    }
    case 0x3: {
      uint64_t v = mem_read64(m, addr);
      if (rd)
        cpu->x[rd] = v;
      break;
    }
    case 0x4: {
      uint8_t v = mem_read8(m, addr);
      if (rd)
        cpu->x[rd] = v;
      break;
    }
    case 0x5: {
      uint16_t v = mem_read16(m, addr);
      if (rd)
        cpu->x[rd] = v;
      break;
    }
    case 0x6: {
      uint32_t v = mem_read32(m, addr);
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
      mem_write8(m, addr, (uint8_t)x2);
      break;
    case 0x1:
      mem_write16(m, addr, (uint16_t)x2);
      break;
    case 0x2:
      mem_write32(m, addr, (uint32_t)x2);
      break;
    case 0x3:
      mem_write64(m, addr, x2);
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
  case 0x73: {
    if (funct3 == 0x0) {
      uint32_t imm = insn >> 20;
      if (imm == 0) {
        sbi_handle(cpu);
      } else if (imm == 1) {
        trap(cpu, 3, pc, 0);
      } else {
        trap(cpu, 2, pc, insn);
      }
      break;
    }

    uint32_t csr = insn >> 20;
    uint64_t old = csr_read(cpu, csr);

    if (rd)
      cpu->x[rd] = old;

    uint64_t write_val = 0;
    if (funct3 == 0x1) {
      write_val = x1;
      csr_write(cpu, csr, write_val);
    } else if (funct3 == 0x2) {
      write_val = old | x1;
      csr_write(cpu, csr, write_val);
    } else if (funct3 == 0x3) {
      write_val = old & ~x1;
      csr_write(cpu, csr, write_val);
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

typedef struct {
  uint8_t ident[16];
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint64_t entry;
  uint64_t phoff;
  uint64_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
} Elf64_Ehdr;

typedef struct {
  uint32_t type;
  uint32_t flags;
  uint64_t offset;
  uint64_t vaddr;
  uint64_t paddr;
  uint64_t filesz;
  uint64_t memsz;
  uint64_t align;
} Elf64_Phdr;

static bool load_elf(Machine *m, const char *path, uint64_t *entry_out) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "open %s failed: %s\n", path, strerror(errno));
    return false;
  }

  uint8_t hdr_buf[64];
  if (fread(hdr_buf, 1, sizeof(hdr_buf), f) != sizeof(hdr_buf)) {
    fclose(f);
    return false;
  }

  Elf64_Ehdr eh;
  memcpy(&eh.ident[0], hdr_buf, 16);
  eh.type = (uint16_t)(hdr_buf[16] | (hdr_buf[17] << 8));
  eh.machine = (uint16_t)(hdr_buf[18] | (hdr_buf[19] << 8));
  eh.version = read_u32_le(&hdr_buf[20]);
  eh.entry = read_u64_le(&hdr_buf[24]);
  eh.phoff = read_u64_le(&hdr_buf[32]);
  eh.shoff = read_u64_le(&hdr_buf[40]);
  eh.flags = read_u32_le(&hdr_buf[48]);
  eh.ehsize = (uint16_t)(hdr_buf[52] | (hdr_buf[53] << 8));
  eh.phentsize = (uint16_t)(hdr_buf[54] | (hdr_buf[55] << 8));
  eh.phnum = (uint16_t)(hdr_buf[56] | (hdr_buf[57] << 8));
  eh.shentsize = (uint16_t)(hdr_buf[58] | (hdr_buf[59] << 8));
  eh.shnum = (uint16_t)(hdr_buf[60] | (hdr_buf[61] << 8));
  eh.shstrndx = (uint16_t)(hdr_buf[62] | (hdr_buf[63] << 8));

  if (eh.ident[0] != 0x7F || eh.ident[1] != 'E' || eh.ident[2] != 'L' ||
      eh.ident[3] != 'F') {
    fclose(f);
    return false;
  }

  if (eh.ident[4] != 2) {
    fclose(f);
    return false;
  }

  if (eh.machine != 0xF3) {
    fclose(f);
    return false;
  }

  *entry_out = eh.entry;

  if (fseek(f, (long)eh.phoff, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }

  for (uint16_t i = 0; i < eh.phnum; i++) {
    uint8_t ph_buf[56];
    if (fread(ph_buf, 1, sizeof(ph_buf), f) != sizeof(ph_buf)) {
      fclose(f);
      return false;
    }

    Elf64_Phdr ph;
    ph.type = read_u32_le(&ph_buf[0]);
    ph.flags = read_u32_le(&ph_buf[4]);
    ph.offset = read_u64_le(&ph_buf[8]);
    ph.vaddr = read_u64_le(&ph_buf[16]);
    ph.paddr = read_u64_le(&ph_buf[24]);
    ph.filesz = read_u64_le(&ph_buf[32]);
    ph.memsz = read_u64_le(&ph_buf[40]);
    ph.align = read_u64_le(&ph_buf[48]);

    if (ph.type != 1) {
      continue;
    }

    if (ph.memsz == 0) {
      continue;
    }

    uint64_t dst = ph.paddr ? ph.paddr : ph.vaddr;

    if (!(dst >= RAM_BASE && (dst + ph.memsz) <= RAM_BASE + m->ram_size)) {
      fprintf(stderr, "ELF segment out of RAM: paddr=0x%016" PRIx64
                      " memsz=0x%016" PRIx64 "\n",
              dst, ph.memsz);
      fclose(f);
      return false;
    }

    memset(&m->ram[dst - RAM_BASE], 0, (size_t)ph.memsz);

    if (ph.filesz > 0) {
      long cur = ftell(f);
      if (cur < 0) {
        fclose(f);
        return false;
      }

      if (fseek(f, (long)ph.offset, SEEK_SET) != 0) {
        fclose(f);
        return false;
      }

      if (fread(&m->ram[dst - RAM_BASE], 1, (size_t)ph.filesz, f) !=
          (size_t)ph.filesz) {
        fclose(f);
        return false;
      }

      if (fseek(f, cur, SEEK_SET) != 0) {
        fclose(f);
        return false;
      }
    }
  }

  fclose(f);
  return true;
}

static void usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s <kernel.elf> [max_insns]\n"
          "\n"
          "Runs a minimal RV64 interpreter with virt UART16550 output and\n"
          "minimal CSR/trap + SBI (console_putchar/shutdown) emulation.\n",
          argv0);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argv[0]);
    return 2;
  }

  const char *elf_path = argv[1];
  uint64_t max_insns = 50ull * 1000ull * 1000ull;
  if (argc >= 3) {
    max_insns = strtoull(argv[2], NULL, 0);
    if (max_insns == 0) {
      die("invalid max_insns");
    }
  }

  Machine m;
  m.ram_size = (size_t)RAM_SIZE;
  m.ram = (uint8_t *)calloc(1, m.ram_size);
  if (!m.ram) {
    die("failed to allocate RAM");
  }

  uint64_t entry = 0;
  if (!load_elf(&m, elf_path, &entry)) {
    die("failed to load ELF");
  }

  Cpu cpu;
  memset(&cpu, 0, sizeof(cpu));
  cpu.pc = entry;

  for (uint64_t i = 0; i < max_insns && !cpu.halted; i++) {
    exec_one(&m, &cpu);
  }

  free(m.ram);
  return 0;
}
