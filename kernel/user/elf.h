#ifndef ELF_H
#define ELF_H

#include<stdint.h>

// ELF64 file header
struct elf64_ehdr {
    uint8_t e_ident[16];        //magic + class/endianness/etc
    uint16_t e_type;            //2 = executable
    uint16_t e_machine;         // 0x3E = x86-64
    uint32_t e_version;
    uint64_t e_entry;           //entry point virtual address
    uint64_t e_phoff;           //program header table file offset
    uint64_t e_shoff;           
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;       //size of one program header
    uint16_t e_phnum;           //number of program headers
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

//ELF64 program header
struct elf64_phdr {
    uint32_t p_type;            //1 = PT_LOAD
    uint32_t p_flags;           // R/W/X
    uint64_t p_offset;          //file offset of segment data
    uint64_t p_vaddr;           //virtual address to load at
    uint64_t p_paddr;
    uint64_t p_filesz;          //bytes present in file
    uint64_t p_memsz;           //bytes in memory (>= filesz; extra is zeroed, eg: .bss)
    uint64_t p_align;
};

#define PT_LOAD 1
#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'

//load an ELF64 image (pointed to in kernel memory) into user pages
//returns the entry-point vritual address, or 0 on failure
uint64_t elf_load(const uint8_t* image, uint64_t image_size);

#endif