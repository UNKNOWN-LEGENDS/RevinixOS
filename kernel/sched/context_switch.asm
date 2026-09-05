bits 64
default rel

global context_switch

; void context_switch(uint64_t* save_old_rsp, uint64_t new_rsp)
; rdi = where to store the current rsp
; rsi = the new rsp to load
context_switch:
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15

	mov [rdi], rsp		; save current stack pointer into old task
	mov rsp, rsi		; load new task's stack pointer

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx
	pop rbp
	ret			; returns into the new task's saved rip
