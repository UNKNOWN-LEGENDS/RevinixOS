bits 64
default rel

extern isr_handler
global isr_stub_table

; --- macro for exceptions that DO NOT push an error code ---
%macro ISR_NOERR 1
isr_stub_%1:
    push qword 0          ; dummy error code, so layout is uniform
    push qword %1         ; interrupt number
    jmp isr_common
%endmacro

; --- macro for exceptions that DO push an error code ---
%macro ISR_ERR 1
isr_stub_%1:
    ; CPU already pushed the real error code
    push qword %1         ; interrupt number
    jmp isr_common
%endmacro

; vectors 8, 10-14, 17, 21, 29, 30 push an error code; the rest don't
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29
ISR_ERR   30
ISR_NOERR 31

isr_common:
    ; save all general-purpose registers (reverse order of the struct)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp          ; pass pointer to interrupt_frame as 1st arg
    call isr_handler

    ; restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16           ; pop int_no and err_code
    iretq                 ; return from interrupt

; table of stub pointers, indexed by vector, read by idt.c
isr_stub_table:
%assign i 0
%rep 32
    dq isr_stub_%+i
%assign i i+1
%endrep

extern irq_handler
global irq_stub_table

%macro IRQ 2
irq_stub_%1:
    push qword 0          ; dummy error code (IRQs don't push one)
    push qword %2         ; vector number (32 + irq)
    jmp irq_common
%endmacro

IRQ 0, 32    ; timer  (PIT)
IRQ 1, 33    ; keyboard
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

irq_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call irq_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq

irq_stub_table:
%assign i 0
%rep 16
    dq irq_stub_%+i
%assign i i+1
%endrep
