; boot/boot.asm
bits 32

section .multiboot2
align 8
mb2_header_start:
    dd 0xE85250D6                ; multiboot2 magic
    dd 0                          ; architecture: 0 = i386 protected mode
    dd mb2_header_end - mb2_header_start
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))
    ; end tag
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

section .bss
align 4096
; Page tables (each 4 KB). Long mode needs PML4 -> PDPT -> PD.
p4_table:               ; PML4
    resb 4096
p3_table:               ; PDPT
    resb 4096
p2_table:               ; PD (maps 512 * 2MB = 1 GB with huge pages)
    resb 4096
align 16
stack_bottom:
    resb 16384          ; 16 KB stack
stack_top:

section .rodata
gdt64:
    dq 0                                   ; null descriptor
.code: equ $ - gdt64
    ; 64-bit code segment: present, DPL0, executable, long-mode
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1                       ; limit
    dq gdt64                               ; base

section .text
global _start
extern kmain

_start:
    mov esp, stack_top

    ; Save multiboot info for later (pass to kmain)
    mov edi, eax        ; multiboot2 magic  -> 1st arg (rdi in 64-bit)
    mov esi, ebx        ; multiboot2 info   -> 2nd arg (rsi in 64-bit)

    call check_long_mode

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start   ; far jump into 64-bit code

    ; should never return
    cli
.hang:
    hlt
    jmp .hang

; --- Verify CPUID + long mode availability ---
check_long_mode:
    ; check CPUID is supported by flipping ID bit (21) in EFLAGS
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_long_mode

    ; check extended CPUID is available
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    ; check LM bit (29) in EDX from CPUID 0x80000001
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    ; print 'ERR' to VGA and halt
    mov dword [0xB8000], 0x4F524F45
    mov dword [0xB8004], 0x4F3A4F52
    cli
    hlt

; --- Build identity-mapped page tables (first 1 GB, 2MB pages) ---
setup_page_tables:
    ; P4[0] -> P3
    mov eax, p3_table
    or eax, 0b11                ; present + writable
    mov [p4_table], eax

    ; P3[0] -> P2
    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    ; P2[0..511] -> 2MB huge pages, identity mapped
    mov ecx, 0
.map_p2:
    mov eax, 0x200000           ; 2 MiB
    mul ecx                     ; eax = 2MB * ecx
    or eax, 0b10000011          ; present + writable + huge
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2
    ret

; --- Enable PAE, set CR3, set LME, enable paging ---
enable_paging:
    ; load CR3 with PML4 address
    mov eax, p4_table
    mov cr3, eax

    ; enable PAE (CR4 bit 5)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; set long mode bit in EFER MSR (0xC0000080), bit 8
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging (CR0 bit 31)
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

; --- 64-bit code from here ---
bits 64
long_mode_start:
    ; reload data segment registers with null (long mode ignores most)
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; rdi and rsi already hold multiboot magic + info from earlier
    call kmain

    cli
.hang:
    hlt
    jmp .hang
