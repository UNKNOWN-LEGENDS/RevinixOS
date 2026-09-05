#include "pic.h"
#include <stdint.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x11   // initialize + expect ICW4
#define ICW4_8086 0x01   // 8086/88 mode

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
// small delay: write to an unused port (gives the PIC time to react)
static inline void io_wait(void) {
    outb(0x80, 0);
}

void pic_remap(void) {
    // start init sequence (cascade mode)
    outb(PIC1_CMD, ICW1_INIT); io_wait();
    outb(PIC2_CMD, ICW1_INIT); io_wait();

    // ICW2: vector offsets
    outb(PIC1_DATA, PIC1_OFFSET); io_wait();   // master -> 32
    outb(PIC2_DATA, PIC2_OFFSET); io_wait();   // slave  -> 40

    // ICW3: tell master slave is at IRQ2, tell slave its cascade identity
    outb(PIC1_DATA, 0x04); io_wait();          // bit 2 = slave on IRQ2
    outb(PIC2_DATA, 0x02); io_wait();

    // ICW4: 8086 mode
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    // mask everything for now; we unmask IRQs as we add handlers
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, 0x20);  // EOI to slave if IRQ 8-15
    outb(PIC1_CMD, 0x20);                // always EOI to master
}

void pic_set_mask(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    uint8_t value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    uint8_t value = inb(port) & ~(1 << irq);
    outb(port, value);
}
