#include "gdt.h"
#include<stdint.h>

// a standard 8-byte GDT descriptor
struct __attribute__((packed)) gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;         //present, DPL, type bits
    uint8_t granularity;    //flags (G, L, D) + high nibble of limit
    uint8_t base_high;
};

//the TSS descriptor is 16 bytes in long mode (spans two normal slots)
struct __attribute__((packed)) tss_descriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
    uint32_t base_upper;    //top 32 bits of the 64-bit base
    uint32_t reserved;
};

//the 64-bit Task State Segment. we only really need rsp0 for now;
//the IST entries become useful once we handle critical faults
struct __attribute__((packed)) tss {
    uint32_t reserved0;
    uint64_t rsp0;              //stack loaded on Ring3 -> Ring0 transition
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];            //ist[0] = IST1 ... ist[6] = IST7
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;      //I/O permission bitmap offset
};

//5 normal entries (null, kcode, kdata, ucode, udata) + 2 slots for the TSS.
static struct gdt_entry gdt[7];
static struct tss tss;

//a dedicated kernel stack for privilege transitions (16 KB)
static uint8_t kernel_stack[16384] __attribute__((aligned(16)));

struct __attribute__((packed)) gdt_ptr {
    uint16_t limit;
    uint64_t base;
};

//defined in gdt_flush.asm
extern void gdt_flush(struct gdt_ptr* ptr);
extern void tss_flush(void);

static void set_entry(int i, uint8_t access, uint8_t gran) {
    //in long mode base/limit are ignored for code/data, so we leave them 0
    //except the flags/access bits that actually matter
    gdt[i].limit_low = 0;
    gdt[i].base_low = 0;
    gdt[i].base_mid = 0;
    gdt[i].access = access;
    gdt[i].granularity = gran;
    gdt[i].base_high = 0;
}

static void set_tss(int i, uint64_t base, uint32_t limit) {
    struct tss_descriptor* d = (struct tss_descriptor*)&gdt[i];
    d->limit_low = limit & 0xFFFF;
    d->base_low = base & 0xFFFF;
    d->base_mid = (base >> 16) & 0xFF;
    d->access = 0x89;                       //present, type = available 64-bit TSS
    d->granularity = (limit >> 16) & 0x0F;  //no G flag, byte-granular
    d->base_high = (base >> 24) & 0xFF;
    d->base_upper = (base >> 32) & 0xFFFFFFFF;
    d->reserved = 0;
}

void gdt_init(void) {
    //null descriptor
    set_entry(0, 0, 0);
    //kernel code: present, DPL0, code, readable | long-mode(L), 4K gran
    set_entry(1, 0x9A, 0xA0);
    // kernel data: present, DPL0, data, writable
    set_entry(2, 0x92, 0xC0);
    // user code: present, DPL3, code, readable | long-mode(L)
    set_entry(3, 0xFA, 0xA0);
    // user data: present, DPL3, data, writable
    set_entry(4, 0xF2, 0xC0);

    // zero the TSS, then point rsp0 at the top of our kernel stack
    uint8_t* t = (uint8_t*)&tss;
    for (unsigned i=0; i<sizeof(tss); i++) t[i] = 0;
    tss.rsp0 = (uint64_t)(kernel_stack + sizeof(kernel_stack));
    tss.iopb_offset = sizeof(tss);

    set_tss(5, (uint64_t)&tss, sizeof(tss) -1);

    struct gdt_ptr ptr;
    ptr.limit = sizeof(gdt) -1;
    ptr.base = (uint64_t)&gdt;

    gdt_flush(&ptr);
    tss_flush();
}

void gdt_debug(void) {
    uint64_t* raw = (uint64_t*)&gdt[3];   // entry 3 = user code = selector 0x18
    // kprintf("GDT[3] (user code, sel 0x18) raw = %p\n", (void*)*raw);
    // uint64_t* raw4 = (uint64_t*)&gdt[4];  // entry 4 = user data = selector 0x20
    // kprintf("GDT[4] (user data, sel 0x20) raw = %p\n", (void*)*raw4);
}