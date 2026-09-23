#ifndef HEAP_H
#define HEAP_H

#include<stddef.h>
#include<stdint.h>

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

//diagnostics
void heap_dump(void);

struct heap_stats {
    uint64_t capacity;
    uint64_t used;
    uint64_t free;
    uint64_t blocks;
    uint64_t largest_free;
};

void heap_get_stats(struct heap_stats* s);

#endif