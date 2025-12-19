#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "rivos_sim/common.h"
#include "rivos_sim/elf.h"
#include "rivos_sim/machine.h"

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

static uint16_t read_u16_le(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

bool load_elf(Machine *m, const char *path, uint64_t *entry_out) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    return false;
  }

  uint8_t hdr_buf[64];
  if (fread(hdr_buf, 1, sizeof(hdr_buf), f) != sizeof(hdr_buf)) {
    fclose(f);
    return false;
  }

  Elf64_Ehdr eh;
  memcpy(&eh.ident[0], hdr_buf, 16);
  eh.type = read_u16_le(&hdr_buf[16]);
  eh.machine = read_u16_le(&hdr_buf[18]);
  eh.version = read_u32_le(&hdr_buf[20]);
  eh.entry = read_u64_le(&hdr_buf[24]);
  eh.phoff = read_u64_le(&hdr_buf[32]);
  eh.shoff = read_u64_le(&hdr_buf[40]);
  eh.flags = read_u32_le(&hdr_buf[48]);
  eh.ehsize = read_u16_le(&hdr_buf[52]);
  eh.phentsize = read_u16_le(&hdr_buf[54]);
  eh.phnum = read_u16_le(&hdr_buf[56]);
  eh.shentsize = read_u16_le(&hdr_buf[58]);
  eh.shnum = read_u16_le(&hdr_buf[60]);
  eh.shstrndx = read_u16_le(&hdr_buf[62]);

  if (eh.ident[0] != 0x7F || eh.ident[1] != 'E' || eh.ident[2] != 'L' ||
      eh.ident[3] != 'F') {
    fclose(f);
    errno = EINVAL;
    return false;
  }

  if (eh.ident[4] != 2) {
    fclose(f);
    errno = EINVAL;
    return false;
  }

  if (eh.machine != 0xF3) {
    fclose(f);
    errno = EINVAL;
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

    if (!(dst >= RIVOS_SIM_RAM_BASE && (dst + ph.memsz) <= RIVOS_SIM_RAM_BASE + m->ram_size)) {
      fprintf(stderr, "ELF segment out of RAM: paddr=0x%016" PRIx64
                      " memsz=0x%016" PRIx64 "\n",
              dst, ph.memsz);
      fclose(f);
      errno = EINVAL;
      return false;
    }

    memset(&m->ram[dst - RIVOS_SIM_RAM_BASE], 0, (size_t)ph.memsz);

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

      if (fread(&m->ram[dst - RIVOS_SIM_RAM_BASE], 1, (size_t)ph.filesz, f) !=
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
