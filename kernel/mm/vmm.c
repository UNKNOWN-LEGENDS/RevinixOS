#include "vmm.h"
#include "pmm.h"
#include "../kprintf.h"
#include <stdint.h>

// Extract the 9-bit index for each paging level from a virtual address.
#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)

// A page-table entry stores the next table's physical address in bits 51:12.
#define ENTRY_ADDR(e) ((e) & 0x000FFFFFFFFFF000ull)

static uint64_t pml4_phys;   // physical address of our top-level table
static int vmm_active = 0;   // 0 = access tables via identity map, 1 = via HHDM

// Get a writable pointer to a page-table frame.
// Before we switch CR3 we're on boot.asm's identity map (phys == virt, low RAM).
// After the switch, every physical frame is reachable through the HHDM.
static uint64_t* table(uint64_t phys) {
    if (vmm_active) return (uint64_t*)(phys + HHDM_OFFSET);
    return (uint64_t*)phys;
}

static void zero_table(uint64_t phys) {
    uint64_t* t = table(phys);
    for (int i = 0; i < 512; i++) t[i] = 0;
}

// Return the next-level table's physical address, allocating it if absent.
// Intermediate entries get USER set so user leaf pages work later; actual
// permission is still decided by the leaf entry.
static uint64_t next_level(uint64_t table_phys, int index, int create) {
    uint64_t* t = table(table_phys);
    if (!(t[index] & PAGE_PRESENT)) {
        if (!create) return 0;
        uint64_t frame = pmm_alloc_frame();
        zero_table(frame);
        t[index] = frame | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }
    return ENTRY_ADDR(t[index]);
}

// Map one 2 MB huge page (used only during init for the identity + HHDM regions).
static void map_huge(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pdpt = next_level(pml4_phys, PML4_INDEX(virt), 1);
    uint64_t pd   = next_level(pdpt, PDPT_INDEX(virt), 1);
    uint64_t* pd_t = table(pd);
    pd_t[PD_INDEX(virt)] = phys | flags | PAGE_HUGE;
}

void vmm_init(void) {
    pml4_phys = pmm_alloc_frame();
    zero_table(pml4_phys);

    const uint64_t TWO_MB = 0x200000ull;

    // 1) Identity-map the first 4 GB so the kernel, its stack, VGA, and low
    //    MMIO keep working the instant we load the new CR3.
    for (uint64_t a = 0; a < 0x100000000ull; a += TWO_MB)
        map_huge(a, a, PAGE_PRESENT | PAGE_WRITABLE);

    // 2) HHDM: map all physical RAM at HHDM_OFFSET.
    uint64_t max_phys = pmm_total_frames() * 4096ull;
    max_phys = (max_phys + TWO_MB - 1) & ~(TWO_MB - 1);   // round up to 2 MB
    for (uint64_t a = 0; a < max_phys; a += TWO_MB)
        map_huge(HHDM_OFFSET + a, a, PAGE_PRESENT | PAGE_WRITABLE);

    // 3) Switch to our tables, then start using the HHDM for table access.
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
    vmm_active = 1;

    kprintf("VMM: paging active. PML4 phys=%p, HHDM base=%p, RAM mapped=%d MB\n",
            (void*)pml4_phys, (void*)HHDM_OFFSET, (int)(max_phys / (1024*1024)));
}

// Map a single 4 KB page. Do NOT use this inside the identity 0-4GB range
// (those are huge pages; walking into them as tables would misbehave).
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pdpt = next_level(pml4_phys, PML4_INDEX(virt), 1);
    uint64_t pd   = next_level(pdpt, PDPT_INDEX(virt), 1);
    uint64_t pt   = next_level(pd, PD_INDEX(virt), 1);
    uint64_t* pt_t = table(pt);
    pt_t[PT_INDEX(virt)] = (phys & ~0xFFFull) | flags | PAGE_PRESENT;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap_page(uint64_t virt) {
    uint64_t pdpt = next_level(pml4_phys, PML4_INDEX(virt), 0);
    if (!pdpt) return;
    uint64_t pd = next_level(pdpt, PDPT_INDEX(virt), 0);
    if (!pd) return;
    uint64_t pt = next_level(pd, PD_INDEX(virt), 0);
    if (!pt) return;
    uint64_t* pt_t = table(pt);
    pt_t[PT_INDEX(virt)] = 0;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

uint64_t vmm_get_phys(uint64_t virt) {
    uint64_t pdpt = next_level(pml4_phys, PML4_INDEX(virt), 0);
    if (!pdpt) return 0;
    uint64_t pde = table(pdpt)[PDPT_INDEX(virt)];  // reuse var name loosely below
    (void)pde;

    uint64_t pd = next_level(pdpt, PDPT_INDEX(virt), 0);
    if (!pd) return 0;

    uint64_t pd_entry = table(pd)[PD_INDEX(virt)];
    if (!(pd_entry & PAGE_PRESENT)) return 0;
    if (pd_entry & PAGE_HUGE)                       // 2 MB page
        return ENTRY_ADDR(pd_entry) + (virt & 0x1FFFFF);

    uint64_t pt = ENTRY_ADDR(pd_entry);
    uint64_t pt_entry = table(pt)[PT_INDEX(virt)];
    if (!(pt_entry & PAGE_PRESENT)) return 0;
    return ENTRY_ADDR(pt_entry) + (virt & 0xFFF);
}