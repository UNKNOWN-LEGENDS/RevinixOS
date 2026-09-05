bits 64
default rel

global idt_load

; void idt_load(struct idt_ptr* ptr)  — ptr in rdi
idt_load:
    lidt [rdi]
    ret
