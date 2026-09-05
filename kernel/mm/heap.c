#include "heap.h"
#include "vmm.h"
#include "pmm.h"
#include "../kprintf.h"
#include <stdint.h>
#include <stddef.h>

// Where the heap lives in virtual memory (higher half, clear of the HHDM).
#define HEAP_START 0xFFFFC00000000000ull
#define HEAP_INITIAL_SIZE 0x100000ull      // 1 MB to start (256 frames)

// Every allocation is preceded by this header.
struct block_header {
    size_t size;                  // usable bytes in this block (excl. header)
    int    free;                  // 1 = free, 0 = allocated
    struct block_header* next;
    struct block_header* prev;
};

#define HEADER_SIZE (sizeof(struct block_header))
#define ALIGN8(x) (((x) + 7) & ~((size_t)7))

static struct block_header* heap_head;
static uint64_t heap_end;         // current virtual end of mapped heap

// Map [virt, virt+size) by pulling frames from the PMM.
static void heap_map_region(uint64_t virt, uint64_t size) {
    for (uint64_t off = 0; off < size; off += 4096) {
        uint64_t frame = pmm_alloc_frame();
        if (!frame) {
            kprintf("heap: OUT OF PHYSICAL MEMORY during map!\n");
            for (;;) __asm__ volatile ("cli; hlt");
        }
        vmm_map_page(virt + off, frame, PAGE_PRESENT | PAGE_WRITABLE);
    }
}

void heap_init(void) {
    heap_map_region(HEAP_START, HEAP_INITIAL_SIZE);
    heap_end = HEAP_START + HEAP_INITIAL_SIZE;

    // one big free block spanning the whole initial region
    heap_head = (struct block_header*)HEAP_START;
    heap_head->size = HEAP_INITIAL_SIZE - HEADER_SIZE;
    heap_head->free = 1;
    heap_head->next = NULL;
    heap_head->prev = NULL;

    kprintf("Heap initialized: base=%p, size=%d KB\n",
            (void*)HEAP_START, (int)(HEAP_INITIAL_SIZE / 1024));
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = ALIGN8(size);

    // first-fit: find a free block large enough
    struct block_header* cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            // split if there's room for another header + a little payload
            if (cur->size >= size + HEADER_SIZE + 8) {
                struct block_header* split =
                    (struct block_header*)((uint8_t*)cur + HEADER_SIZE + size);
                split->size = cur->size - size - HEADER_SIZE;
                split->free = 1;
                split->next = cur->next;
                split->prev = cur;
                if (cur->next) cur->next->prev = split;
                cur->next = split;
                cur->size = size;
            }
            cur->free = 0;
            return (void*)((uint8_t*)cur + HEADER_SIZE);
        }
        cur = cur->next;
    }

    kprintf("kmalloc: no block large enough for %d bytes\n", (int)size);
    return NULL;   // (heap growth comes later — see checklist)
}

void kfree(void* ptr) {
    if (!ptr) return;

    struct block_header* block =
        (struct block_header*)((uint8_t*)ptr - HEADER_SIZE);
    block->free = 1;

    // coalesce with next if free
    if (block->next && block->next->free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }
    // coalesce with prev if free
    if (block->prev && block->prev->free) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}

void heap_dump(void) {
    struct block_header* cur = heap_head;
    int i = 0;
    while (cur) {
        kprintf("  block %d: addr=%p size=%d %s\n",
                i++, (void*)cur, (int)cur->size, cur->free ? "FREE" : "USED");
        cur = cur->next;
    }
}