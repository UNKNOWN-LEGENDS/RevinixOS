#ifndef VMM_H
#define VMM_H

#include<stdint.h>

#define PAGE_PRESENT (1ull << 0)
#define PAGE_WRITABLE (1ull << 1)
#define PAGE_USER (1ull << 2)
#define PAGE_HUGE (1ull << 7)           //2MB page at the PD level

//all physical RAM is mapped starting here (canonical higher half)
#define HHDM_OFFSET 0xFFFF800000000000ull
#define KERNEL_VMA 0xFFFFFFFF80000000ull

#define KERNEL_VIRT_BASE 0xFFFFFFFF80100000ull   // the VMA that maps to phys 1 MB
#define KERNEL_PHYS_BASE 0x100000ull             // kernel image loads at 1 MB

void vmm_init(void);

// Map / translate / unmap within a SPECIFIC address space (target PML4 phys).
void     vmm_map_page_in(uint64_t pml4, uint64_t virt, uint64_t phys, uint64_t flags);
void     vmm_unmap_page_in(uint64_t pml4, uint64_t virt);
uint64_t vmm_get_phys_in(uint64_t pml4, uint64_t virt);

// Convenience wrappers that target the shared kernel address space.
void     vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void     vmm_unmap_page(uint64_t virt);
uint64_t vmm_get_phys(uint64_t virt);       // translate; 0 if unmapped

// Per-process address spaces.
uint64_t vmm_create_address_space(void);            // new PML4; kernel half shared in
void     vmm_switch_address_space(uint64_t pml4_phys);  // load CR3
uint64_t vmm_kernel_pml4(void);                     // the shared kernel / template PML4

//convert a physical address to its HHDM virtual address and back
static inline void* phys_to_virt(uint64_t phys) {return (void*)(phys + HHDM_OFFSET); }
static inline uint64_t virt_to_phys_hhdm(void* v) { return (uint64_t)v - HHDM_OFFSET; }

#endif
