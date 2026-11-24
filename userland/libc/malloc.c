// malloc / free / calloc / realloc: a simple first-fit allocator on top of
// sbrk(). Blocks sit back to back in address order, each = header + payload,
// so a block's neighbour in memory is simply the next block in the list.
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define ALIGN     16                    // every payload is 16-byte aligned
#define GROW_MIN  (64 * 1024)           // grow the heap at least 64 KB at a time
#define TRIM_MIN  (64 * 1024)           // give memory back once 64 KB+ at the top is free
#define MAGIC     0x5245564Eu           // "REVN": spots free() of a pointer we never gave out

struct block {
    size_t        size;                 // payload bytes (a multiple of ALIGN)
    struct block* next;                 // next block in memory
    uint32_t      free;
    uint32_t      magic;
    uint64_t      pad;                  // header = 32 bytes, so payloads stay 16-aligned
};

static struct block* head = 0;          // lowest block
static struct block* tail = 0;          // highest block (ends at the program break)

static size_t round_up(size_t n) { return (n + ALIGN - 1) & ~(size_t)(ALIGN - 1); }

// If b is much bigger than needed, cut the rest off into a new free block.
static void split(struct block* b, size_t size) {
    if (b->size < size + sizeof(struct block) + ALIGN) return;
    struct block* rest = (struct block*)((char*)(b + 1) + size);
    rest->size  = b->size - size - sizeof(struct block);
    rest->next  = b->next;
    rest->free  = 1;
    rest->magic = MAGIC;
    b->size = size;
    b->next = rest;
    if (tail == b) tail = rest;
}

void* malloc(size_t n) {
    if (n == 0 || n > ((size_t)1 << 40)) return 0;
    size_t size = round_up(n);

    for (struct block* b = head; b; b = b->next)       // first fit
        if (b->free && b->size >= size) {
            split(b, size);
            b->free = 0;
            return b + 1;
        }

    // Nothing fits: grow the heap. If the top block is free, extend it
    // instead of leaving a too-small free block behind.
    struct block* b;
    size_t need = (tail && tail->free) ? size - tail->size : sizeof(struct block) + size;
    size_t grow = need < GROW_MIN ? GROW_MIN : round_up(need);
    char* p = sbrk((intptr_t)grow);
    if (p == (char*)-1) return 0;

    if (tail && tail->free) {
        tail->size += grow;
        b = tail;
    } else {
        b = (struct block*)p;
        b->size  = grow - sizeof(struct block);
        b->next  = 0;
        b->free  = 1;
        b->magic = MAGIC;
        if (tail) tail->next = b; else head = b;
        tail = b;
    }
    split(b, size);
    b->free = 0;
    return b + 1;
}

// Hand a large free top block back to the kernel (sbrk with a negative size).
static void trim(void) {
    if (!tail || !tail->free || tail->size < TRIM_MIN) return;
    struct block* prev = 0;
    if (tail != head)
        for (prev = head; prev->next != tail; prev = prev->next) { }
    sbrk(-(intptr_t)(sizeof(struct block) + tail->size));
    if (prev) prev->next = 0; else head = 0;
    tail = prev;
}

void free(void* ptr) {
    if (!ptr) return;
    struct block* b = (struct block*)ptr - 1;
    if (b->magic != MAGIC || b->free) return;          // not ours, or freed twice: ignore
    b->free = 1;

    // Merge every pair of neighbouring free blocks.
    for (struct block* c = head; c && c->next; ) {
        if (c->free && c->next->free) {
            if (tail == c->next) tail = c;
            c->size += sizeof(struct block) + c->next->size;
            c->next  = c->next->next;
        } else {
            c = c->next;
        }
    }
    trim();
}

void* calloc(size_t count, size_t size) {
    if (size && count > ((size_t)1 << 40) / size) return 0;   // count * size overflow
    void* p = malloc(count * size);
    if (p) memset(p, 0, count * size);     // a reused block still holds old data
    return p;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return 0; }
    struct block* b = (struct block*)ptr - 1;
    if (b->size >= size) return ptr;       // already big enough
    void* q = malloc(size);
    if (!q) return 0;                      // the old block stays valid
    memcpy(q, ptr, b->size);
    free(ptr);
    return q;
}