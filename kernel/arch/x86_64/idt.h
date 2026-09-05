#ifndef IDT_H
#define IDT_H

#include<stdint.h>

//the register/state snapshot our asm stubs build on the stack,
//laid out so the C handler receives it as a pointer

struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;      //pushed by our stubs
    uint64_t rip, cs, rflags, rsp, ss; //pushed by the CPU automatically
};

void idt_init(void);

#endif