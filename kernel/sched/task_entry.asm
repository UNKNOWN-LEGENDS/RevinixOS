bits 64
default rel

global task_entry_trampoline
extern task_exit

; when a new task first runs, context_switch's 'ret' lands here.
; interrupts are still disabled (we got here via a timer IRQ switch), so enable them, then call the real entry
; The entry pointer was placed just below this trampoline's lot on the stack
task_entry_trampoline:
	sti			; re-enable interrupts for this task
	pop rax			; rax = real entry function pointer
	call rax		; run the task
	call task_exit		; if it returns, clean up
