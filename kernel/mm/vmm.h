#ifndef VMM_H
#define VMM_H

#include<stdint.h>

#define PAGE_PRESENT (1ull << 0)
#define PAGE_WRITABLE (1ull << 1)
#define PAGE_USER (1ull << 2)
#define PAGE_HUGE (1ull << 7)           //2MB page at the PD level

//all physical RAM is mapped starting here (canonical higher half)
#define HHDM_OFFSET 0xFFFF800000000000ull

void vmm_init(void);
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_ummap_page(uint64_t virt);
uint64_t vmm_get_phys(uint64_t virt);       //translate; 0 if unmapped

//convert a physical address to its HHDM virtual address and back
static inline void* phys_to_virt(uint64_t phys) {return (void*)(phys + HHDM_OFFSET); }
static inline uint64_t virt_to_phys_hhdm(void* v) { return (uint64_t)v - HHDM_OFFSET; }

#endif