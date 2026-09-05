#include "sched.h"
#include "../mm/heap.h"
#include "../kprintf.h"
#include <stdint.h>

#define STACK_SIZE 16384       // 16 KB kernel stack per task

extern void context_switch(uint64_t* save_old_rsp, uint64_t new_rsp);
extern void task_entry_trampoline(void);

static struct task* current   = NULL;
static struct task* task_list = NULL;   // circular
static int next_id = 0;

// Register the currently-executing boot context as the first task.
// It already runs on a real stack, so it needs no primed stack.
void sched_init(void) {
    struct task* boot = (struct task*)kmalloc(sizeof(struct task));
    boot->id = next_id++;
    boot->state = TASK_RUNNING;
    boot->stack_base = NULL;     // boot stack isn't heap-allocated
    boot->next = boot;           // circle of one
    task_list = boot;
    current = boot;
}

struct task* task_create(void (*entry)(void)) {
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    uint8_t* stack = (uint8_t*)kmalloc(STACK_SIZE);

    // 16-byte align the stack top (System V ABI needs it for C entry)
    uint64_t top = ((uint64_t)stack + STACK_SIZE) & ~0xFULL;
    uint64_t* sp = (uint64_t*)top;

    // Build a fake initial frame. Order (high -> low address):
    //   [ task_exit ]  <- entry returns here when it finishes
    //   [ entry     ]  <- context_switch's `ret` lands here
    //   [ rbp=0     ]
    //   [ rbx=0     ]
    //   [ r12=0     ]
    //   [ r13=0     ]
    //   [ r14=0     ]
    //   [ r15=0     ]  <- rsp points here
    *(--sp) = (uint64_t)task_exit;   // safety net if entry ever returns
    *(--sp) = (uint64_t)entry;       // first resume jumps here ; trampoline pops this into rax
    *(--sp) = (uint64_t)task_entry_trampoline;  //context_switch ret lands here
    *(--sp) = 0;  // rbp
    *(--sp) = 0;  // rbx
    *(--sp) = 0;  // r12
    *(--sp) = 0;  // r13
    *(--sp) = 0;  // r14
    *(--sp) = 0;  // r15

    t->rsp = (uint64_t)sp;
    t->stack_base = stack;
    t->id = next_id++;
    t->state = TASK_READY;

    // append to the circular list (preserve creation order)
    struct task* p = task_list;
    while (p->next != task_list) p = p->next;
    p->next = t;
    t->next = task_list;

    return t;
}

void schedule(void) {
    if (!current) return;

    struct task* prev = current;
    struct task* next = prev->next;

    // skip finished tasks
    while (next->state == TASK_DONE && next != prev)
        next = next->next;

    if (next == prev) return;    // nobody else runnable; keep running prev

    if (prev->state == TASK_RUNNING) prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    current = next;

    context_switch(&prev->rsp, next->rsp);
    // execution resumes here when someone later switches back to prev
}

void yield(void) {
    schedule();
}

void task_exit(void) {
    kprintf("[task %d exiting]\n", current->id);
    current->state = TASK_DONE;
    schedule();                  // switch away for good
    for (;;) __asm__ volatile ("hlt");   // unreachable if others exist
}

int sched_has_other_runnable(void) {
    struct task* p = current->next;
    while (p != current) {
        if (p->state != TASK_DONE) return 1;
        p = p->next;
    }
    return 0;
}

// new tasks first land here. we enable interrupts (which were disabled when the timer IRQ switched us in) before running the real entry.
static void task_trampoline(void (*entry)(void)) {
    __asm__ volatile ("sti");
    entry();
    task_exit();
}