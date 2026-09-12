; boot/boot.asm
bits 32

; ---------- Multiboot2 header (low-mapped, must be early in the file) ----------
section .multiboot2
align 8
mb2_header_start:
    dd 0xE85250D6                ; multiboot2 magic
    dd 0                          ; architecture: 0 = i386 protected mode
    dd mb2_header_end - mb2_header_start
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

; ---------- Early boot data (low-mapped): page tables + stack ----------
section .boot.bss nobits alloc noexec write align=4096
p4_table:               ; PML4
    resb 4096
p3_table:               ; PDPT for the low identity map
    resb 4096
p2_table:               ; PD: 512 * 2MB = 1 GB identity, huge pages
    resb 4096
p3_hi:                  ; PDPT for the higher-half kernel map
    resb 4096
align 16
stack_bottom:
    resb 16384          ; 16 KB boot stack
stack_top:

; ---------- Early GDT (low-mapped) ----------
section .boot.rodata progbits alloc noexec nowrite align=8
gdt64:
    dq 0                                   ; null descriptor
.code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1                       ; limit
    dq gdt64                               ; base

; ---------- Early boot code (low-mapped) ----------
section .boot.text progbits alloc exec nowrite align=16
global _start
extern kmain

_start:
    mov esp, stack_top

    mov edi, eax        ; multiboot2 magic  -> rdi
    mov esi, ebx        ; multiboot2 info   -> rsi

    call check_long_mode
    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start   ; far jump into 64-bit code (still low)

    cli
.hang:
    hlt
    jmp .hang

; --- Verify CPUID + long mode availability ---
check_long_mode:
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

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov dword [0xB8000], 0x4F524F45
    mov dword [0xB8004], 0x4F3A4F52
    cli
    hlt

; --- Build page tables: low identity (0-1GB) + higher-half kernel ---
setup_page_tables:
    ; -- low identity map: P4[0] -> p3_table -> p2_table[0..511] (huge) --
    mov eax, p3_table
    or eax, 0b11                ; present + writable
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    mov ecx, 0
.map_p2:
    mov eax, 0x200000           ; 2 MiB
    mul ecx                     ; eax = 2MB * ecx
    or eax, 0b10000011          ; present + writable + huge
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2

    ; -- higher-half kernel map: P4[511] -> p3_hi ; p3_hi[510] -> p2_table --
    ; 0xFFFFFFFF80000000 decodes to PML4[511], PDPT[510], PD[0]; reusing
    ; p2_table (already identity huge pages from phys 0) covers the kernel.
    mov eax, p3_hi
    or eax, 0b11
    mov [p4_table + 511 * 8], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_hi + 510 * 8], eax
    ret

; --- Enable PAE, set CR3, set LME, enable paging ---
enable_paging:
    mov eax, p4_table
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5              ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080         ; EFER
    rdmsr
    or eax, 1 << 8              ; LME
    wrmsr

    mov eax, cr0
    or eax, 1 << 31             ; PG
    mov cr0, eax
    ret

; --- 64-bit trampoline, still executing from the LOW identity map ---
bits 64
long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Hop up into the higher half. Must be an absolute 64-bit jump: a direct
    ; jmp would be RIP-relative rel32 and cannot reach 0xFFFFFFFF8...
    mov rax, higher_half_entry
    jmp rax

; ---------- Higher-half kernel entry (high-mapped) ----------
section .text
bits 64
higher_half_entry:
    ; rdi/rsi (mb2 magic/info) preserved across the hop
    call kmain

    cli
.hang:
    hlt
    jmp .hang
