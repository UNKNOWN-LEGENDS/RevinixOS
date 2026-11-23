#include "elf.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../kprintf.h"
#include <stdint.h>

#define PAGE_SIZE 4096
#define ALIGN_DOWN(x, a) ((x) & ~((uint64_t)(a) - 1))
#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((uint64_t)(a) - 1))

// Map a user page if not already mapped; return its HHDM-accessible pointer
// so the kernel can write into it.
static uint8_t* ensure_user_page(uint64_t pml4, uint64_t vaddr) {
    uint64_t page = ALIGN_DOWN(vaddr, PAGE_SIZE);
    uint64_t phys = vmm_get_phys_in(pml4, page);
    if (!phys) {
        phys = pmm_alloc_frame();
        vmm_map_page_in(pml4, page, phys, PAGE_WRITABLE | PAGE_USER);
    }
    // reach the physical frame through the HHDM to write into it
    return (uint8_t*)phys_to_virt(phys);
}

uint64_t elf_load(uint64_t pml4, const uint8_t* image, uint64_t image_size) {
    if (image_size < sizeof(struct elf64_ehdr)) {
        kprintf("ELF: image too small\n");
        return 0;
    }

    const struct elf64_ehdr* eh = (const struct elf64_ehdr*)image;

    // verify magic
    if (eh->e_ident[0] != ELF_MAGIC0 || eh->e_ident[1] != ELF_MAGIC1 ||
        eh->e_ident[2] != ELF_MAGIC2 || eh->e_ident[3] != ELF_MAGIC3) {
        kprintf("ELF: bad magic\n");
        return 0;
    }
    if (eh->e_ident[4] != 2) {          // EI_CLASS: 2 = 64-bit
        kprintf("ELF: not 64-bit\n");
        return 0;
    }
    if (eh->e_type != 2) {              // ET_EXEC
        kprintf("ELF: not an executable (type=%d)\n", eh->e_type);
        return 0;
    }
    if (eh->e_machine != 0x3E) {        // x86-64
        kprintf("ELF: wrong machine (0x%x)\n", eh->e_machine);
        return 0;
    }

    // walk program headers
    const struct elf64_phdr* ph =
        (const struct elf64_phdr*)(image + eh->e_phoff);

    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;

        if ((ph[i].p_flags & PF_W) && (ph[i].p_flags & PF_X)) {
            kprintf("ELF: segment at %p is writable AND executable - refused (W^X)\n", (void*)ph[i].p_vaddr);
            return 0;
        }

        uint64_t vaddr  = ph[i].p_vaddr;
        uint64_t filesz = ph[i].p_filesz;
        uint64_t memsz  = ph[i].p_memsz;
        uint64_t off    = ph[i].p_offset;

        kprintf("ELF: loading segment vaddr=%p filesz=%d memsz=%d\n",
                (void*)vaddr, (int)filesz, (int)memsz);

        // copy filesz bytes from the image into user pages, byte by byte,
        // mapping pages on demand. (Simple and correct; optimize later.)
        for (uint64_t b = 0; b < memsz; b++) {
            uint8_t* dst_page = ensure_user_page(pml4, vaddr + b);
            uint64_t page_off = (vaddr + b) & (PAGE_SIZE - 1);
            // bytes beyond filesz are zero (handles .bss)
            dst_page[page_off] = (b < filesz) ? image[off + b] : 0;
        }
    }

    // ---- permissions (W^X) ----
    // The copy above went through the HHDM, so the user mappings' own flags
    // didn't matter yet. Now set them for real:
    //   pass 1: every loaded page -> read-only + no-execute
    //   pass 2: give back write / execute exactly where a segment asks for it
    // A page shared by two segments ends up with both segments' rights, so
    // pass 3 refuses the program if any page became writable AND executable.
    uint64_t nx = vmm_nx();
    for (int pass = 1; pass <= 3; pass++) {
        for (int i = 0; i < eh->e_phnum; i++) {
            if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0) continue;
            uint64_t start = ALIGN_DOWN(ph[i].p_vaddr, PAGE_SIZE);
            uint64_t end   = ph[i].p_vaddr + ph[i].p_memsz;
            uint64_t set   = (ph[i].p_flags & PF_W) ? PAGE_WRITABLE : 0;
            uint64_t clear = (ph[i].p_flags & PF_X) ? nx : 0;

            for (uint64_t page = start; page < end; page += PAGE_SIZE) {
                if (pass == 1) {
                    vmm_protect_page_in(pml4, page, nx, PAGE_WRITABLE);
                } else if (pass == 2) {
                    vmm_protect_page_in(pml4, page, set, clear);
                } else {
                    uint64_t pte = vmm_get_pte_in(pml4, page);
                    if (nx && (pte & PAGE_WRITABLE) && !(pte & nx)) {
                        kprintf("ELF: page %p would be writable AND executable - refused (W^X)\n",
                                (void*)page);
                        return 0;
                    }
                }
            }
            if (pass == 3)
                kprintf("ELF: segment %p-%p %c%c%c\n", (void*)ph[i].p_vaddr, (void*)end,
                        (ph[i].p_flags & PF_R) ? 'r' : '-',
                        (ph[i].p_flags & PF_W) ? 'w' : '-',
                        (ph[i].p_flags & PF_X) ? 'x' : '-');
        }
    }
    kprintf("ELF: loaded, entry=%p\n", (void*)eh->e_entry);
    return eh->e_entry;
}