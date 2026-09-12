#include "pmm.h"
#include "../kprintf.h"
#include <stdint.h>
#include <stddef.h>

//provided by the linker script: physical end of the kernel image
extern char kernel_phys_end[];

// multiboot2 structures we need
struct __attribute__((packed)) mb2_tag {
    uint32_t type;
    uint32_t size;
};

struct __attribute__((packed)) mb2_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    //entries follow
};

struct __attribute__((packed)) mb2_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;          //1 = available RAM
    uint32_t reserved;
};

// ---- allocator state ----
static uint8_t* bitmap;
static uint64_t total_frames;
static uint64_t used_frames;
static uint64_t bitmap_size;   // in bytes

static void bitmap_set(uint64_t f)   { bitmap[f / 8] |=  (1 << (f % 8)); }
static void bitmap_clear(uint64_t f) { bitmap[f / 8] &= ~(1 << (f % 8)); }
static int  bitmap_test(uint64_t f)  { return bitmap[f / 8] & (1 << (f % 8)); }

// transition-based counting keeps used_frames exact
static void mark_used(uint64_t f) {
    if (!bitmap_test(f)) { bitmap_set(f); used_frames++; }
}
static void mark_free(uint64_t f) {
    if (bitmap_test(f)) { bitmap_clear(f); used_frames--; }
}

#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((uint64_t)(a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((uint64_t)(a) - 1))

void pmm_init(uint64_t mb2_info) {
    kprintf("PMM DEBUG: mb2_info ptr = %p\n", (void*)mb2_info);
    kprintf("PMM DEBUG: total_size field = %d\n", (int)*(uint32_t*)mb2_info);

    uint8_t* base = (uint8_t*)mb2_info;
    uint32_t total_size = *(uint32_t*)base;   // first field of the mb2 info struct
    uint8_t* end = base + total_size;
    uint8_t* p = base + 8;                     // tags start after total_size + reserved

    struct mb2_tag_mmap* mmap = NULL;

    // walk tags until the end tag (type 0)
    while (p < end) {
        struct mb2_tag* tag = (struct mb2_tag*)p;
        if (tag->type == 0) break;
        if (tag->type == 6) mmap = (struct mb2_tag_mmap*)tag;
        p += ALIGN_UP(tag->size, 8);           // tags are 8-byte aligned
    }

    if (!mmap) {
        kprintf("PMM: no memory map tag! Halting.\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }

    uint8_t* entries    = (uint8_t*)mmap + sizeof(struct mb2_tag_mmap);
    uint8_t* mmap_end   = (uint8_t*)mmap + mmap->size;
    uint32_t entry_size = mmap->entry_size;

    // pass 1: highest usable physical address determines bitmap size
    uint64_t max_addr = 0;
    for (uint8_t* e = entries; e < mmap_end; e += entry_size) {
        struct mb2_mmap_entry* m = (struct mb2_mmap_entry*)e;
        if (m->type == 1) {
            uint64_t region_end = m->base_addr + m->length;
            if (region_end > max_addr) max_addr = region_end;
        }
    }

    total_frames = max_addr / FRAME_SIZE;
    bitmap_size  = (total_frames + 7) / 8;

    // place the bitmap right after the kernel (frame-aligned).
    // NOTE assumes GRUB placed the mb2 info struct in low memory, not here.
    // On QEMU/GRUB this holds; worth revisiting for real-hardware bring-up.
    // The mb2 info struct sits somewhere in low memory (often right after the
    // kernel). Make sure the bitmap starts past BOTH the kernel and that struct,
    // or filling the bitmap would clobber the memory map we still need to read.
    // kernel_phys_end already accounts for the high VMA offset (linker-computed),
    // so this stays a plain physical address.

    uint64_t mb2_start = mb2_info;
    uint64_t mb2_end   = mb2_info + total_size;   // total_size = *(uint32_t*)mb2_info

    uint64_t bitmap_start = ALIGN_UP((uint64_t)kernel_phys_end, FRAME_SIZE);
    if (mb2_end > bitmap_start && mb2_start < bitmap_start + bitmap_size) {
        // overlap: push the bitmap to just past the mb2 info struct
        bitmap_start = ALIGN_UP(mb2_end, FRAME_SIZE);
    }
    bitmap = (uint8_t*)bitmap_start;

    // start with everything marked used...
    for (uint64_t i = 0; i < bitmap_size; i++) bitmap[i] = 0xFF;
    used_frames = total_frames;

    // pass 2: free the frames inside each available region
    for (uint8_t* e = entries; e < mmap_end; e += entry_size) {
        struct mb2_mmap_entry* m = (struct mb2_mmap_entry*)e;
        if (m->type != 1) continue;
        uint64_t start      = ALIGN_UP(m->base_addr, FRAME_SIZE);
        uint64_t region_end = m->base_addr + m->length;
        for (uint64_t a = start; a + FRAME_SIZE <= region_end; a += FRAME_SIZE)
            mark_free(a / FRAME_SIZE);
    }

    // reserve frame 0 (so a 0 return unambiguously means "out of memory")
    mark_used(0);

    // reserve everything from 0 through the end of the bitmap:
    // this covers low memory, the kernel image, and the bitmap itself.
    uint64_t reserve_end = ALIGN_UP((uint64_t)bitmap + bitmap_size, FRAME_SIZE);
    for (uint64_t a = 0; a < reserve_end; a += FRAME_SIZE)
        mark_used(a / FRAME_SIZE);

    kprintf("PMM: %d MB usable, %d frames total, %d free\n",
            (int)(max_addr / (1024 * 1024)),
            (int)total_frames,
            (int)(total_frames - used_frames));
}

uint64_t pmm_alloc_frame(void) {
    // linear first-fit scan. Simple and correct; optimize later if needed.
    for (uint64_t i = 1; i < total_frames; i++) {
        if (!bitmap_test(i)) {
            mark_used(i);
            return i * FRAME_SIZE;
        }
    }
    return 0;   // no free frames
}

void pmm_free_frame(uint64_t addr) {
    mark_free(addr / FRAME_SIZE);
}

uint64_t pmm_total_frames(void) { return total_frames; }
uint64_t pmm_used_frames(void)  { return used_frames; }
