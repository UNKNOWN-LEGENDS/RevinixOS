bits 64
default rel

global gdt_flush
global tss_flush

; void gdt_flush(struct gdt_ptr* ptr)
; System V AMD64 ABI: first argument arrives in rdi
gdt_flush:
	lgdt [rdi] 		; load the new GDT

	; reload data segment registers with the kernel data selector (0x10)
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax

	; reload CS with the kernel code selector (0x08) via a far return
	push qword 0x08			; new CS
	lea rax, [rel .reload_cs]
	push rax			; new RIP
	retfq

.reload_cs:
	ret

; void tss_flush(void)
tss_flush:
	mov ax, 0x28			; TSS selector (RPL 0)
	ltr ax
	ret
