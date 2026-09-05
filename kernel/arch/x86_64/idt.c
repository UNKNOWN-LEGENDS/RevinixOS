#include "idt.h"
#include "../../kprintf.h"
#include<stdint.h>

struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;          //code segment selector (kernel code = 0x08)
    uint8_t ist;                //interrupt stack table index (0 = none)
    uint8_t type_attr;          //present, DPL, gate type
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
};

struct __attribute__((packed)) idt_ptr {
    uint16_t limit;
    uint64_t base;
};


static struct idt_entry idt[256];
static struct idt_ptr idtp;

//each of these is an assembly stub (in isr.asm)
extern void* isr_stub_table[];

extern void idt_load(struct idt_ptr* ptr);

static void set_gate(int vec, void* handler, uint8_t type_attr) {
    uint64_t addr = (uint64_t)handler;
    idt[vec].offset_low = addr & 0xFFFF;
    idt[vec].selector = 0x08;           //kernel code segment
    idt[vec].ist = 0;
    idt[vec].type_attr = type_attr;     //0x8E = present, DPL0, 64-bit interrupt gate
    idt[vec].offset_mid = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vec].reserved = 0;
}

void set_gate_external(int vec, void* handler, uint8_t type_attr) {
    set_gate(vec, handler, type_attr);
}

static const char* exception_names[32] = {
    "Divide-by-Zero", "Debug", "Non-Maskable Interrupt", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
    "Stack-Segment Fault", "General Protection Fault", "Page Fault", "Reserved",
    "x87 FP Exception", "Alignment Check", "Machine Check", "SIMD FP Exception",
    "Virtualization", "Control Protection", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection", "VMM Communication", "Security", "Reserved"
};

void idt_init(void) {
    //install the first 32 vectors = CPU exceptions
    for (int i=0; i<32; i++) {
        set_gate(i, isr_stub_table[i], 0x8E);
    }

    idtp.limit = sizeof(idt) -1;
    idtp.base = (uint64_t)&idt;
    idt_load(&idtp);
}

//the single C entry point every exception stub jumps to
void isr_handler(struct interrupt_frame* frame) {
    kprintf("\n*** CPU EXCEPTION ***\n");
    if (frame->int_no < 32) {
        kprintf("   %s (vector %d)\n", exception_names[frame->int_no], (int)frame->int_no);
    } else {
        kprintf("   Unknown interrupt %d\n", (int)frame->int_no);
    }
    kprintf("   Error code: %x\n", (unsigned int)frame->err_code);
    kprintf("   RIP: %p\n", (void*)frame->rip);
    kprintf("  RSP: %p\n", (void*)frame->rsp);
    kprintf("  RFLAGS: %p\n", (void*)frame->rflags);

    // Page fault: CR2 holds the faulting address
    if (frame->int_no == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        kprintf("  Faulting address (CR2): %p\n", (void*)cr2);
    }

    kprintf("  System halted.\n");
    for (;;) __asm__ volatile ("cli; hlt");
}
