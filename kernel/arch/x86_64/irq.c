#include "idt.h"
#include "pic.h"
#include "../../kprintf.h"
#include "../../sched/sched.h"
#include <stdint.h>

extern void* irq_stub_table[];
void set_gate_external(int vec, void* handler, uint8_t type_attr); // from idt.c

static volatile uint64_t timer_ticks = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// program the PIT (channel 0) to fire at `hz` interrupts per second
static void pit_init(uint32_t hz) {
    uint32_t divisor = 1193182 / hz;   // PIT base frequency
    outb(0x43, 0x36);                  // channel 0, lo/hi byte, mode 3
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void irq_install(void) {
    // register the 16 IRQ stubs at vectors 32-47
    for (int i = 0; i < 16; i++) {
        set_gate_external(32 + i, irq_stub_table[i], 0x8E);
    }
    pit_init(100);                 // 100 Hz = one tick every 10 ms
    pic_clear_mask(0);             // unmask timer (IRQ0)
    pic_clear_mask(1);             // unmask keyboard (IRQ1)
}

uint64_t get_ticks(void) {
    return timer_ticks;
}

// single dispatch point for all hardware IRQs
void irq_handler(struct interrupt_frame* frame) {
    uint8_t irq = frame->int_no - 32;

    switch (irq) {
        case 0:  // timer
            timer_ticks++;
            // // print a heartbeat once per second so we can see it working
            // if (timer_ticks % 100 == 0) {
            //     kprintf("[tick %d]\n", (int)(timer_ticks / 100));
            // }
            // break;

            // if (timer_ticks <= 10) kprintf("[TICK %d]\n", (int)timer_ticks);
            // pic_send_eoi(irq);                  //EOI FIRST, before any task switch
            // schedule();                         //preempt: switch to the next task
            // return;                             //EOI already sent; dont fall through

            if ((frame->cs & 3) == 3) {            //interrupted code was Ring 3
                static int reported = 0;
                if (!reported) {
                    kprintf("[Timer fired from Ring 3! cs=%p] User mode CONFIRMED.\n", (void*)frame->cs);
                    reported = 1;
                }
            }
            pic_send_eoi(irq);
            schedule();                     //no-op here (ony the boot task exists)
            return;
        case 1: { // keyboard
            uint8_t scancode;
            __asm__ volatile ("inb $0x60, %0" : "=a"(scancode));
            kprintf("[key scancode: %x]\n", scancode);
            break;
        }
        default:
            break;
    }

    pic_send_eoi(irq);   // MUST acknowledge, or no more IRQs of this type
}
