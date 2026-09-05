bits 64
default rel

global user_program_start
global user_program_end

user_program_start:
    ; --- write(1, msg, len) ---
    mov rax, 1              ; SYS_WRITE
    lea rdi, [rel msg]      ; buf  (arg1)  -- NOTE: address fixups below
    mov rsi, msg_len        ; count (arg2)
    int 0x80

    ; --- exit(0) ---
    mov rax, 2              ; SYS_EXIT
    mov rdi, 0             ; exit code
    int 0x80

    ; should never reach here
.hang:
    jmp .hang

msg:    db "Hello from a Ring 3 user program!", 0x0A
msg_len equ $ - msg
user_program_end:
