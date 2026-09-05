#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC1_OFFSET 0x20   // IRQs 0-7  -> vectors 32-39
#define PIC2_OFFSET 0x28   // IRQs 8-15 -> vectors 40-47

void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

#endif
