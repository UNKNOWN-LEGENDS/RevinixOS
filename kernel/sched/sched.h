#ifndef SCHED_H
#define SCHED_H
#include<stdint.h>
#include "../fs/file.h"

enum task_state { TASK_READY, TASK_RUNNING, TASK_DONE };

struct task {
    uint64_t rsp;               //saved stack pointer (must be first field)
    void* stack_base;
    uint64_t pml4;           //heap allocation, for freeing later
    uint64_t kstack_top;    //top of this task's kernel stack -> TSS.rsp0
    int id;
    enum task_state state;
    struct task* next;          //circular run queue
    void* fpu;
    int exit_code;
    int killed_by;
    struct file* fds[MAX_FDS];  // open files: fd number -> file (NULL = free)
    uint64_t brk_start;         //user heap: first byte (page after the image)
    uint64_t brk;               //user heap: current end ("program break")
};

struct task* task_create_user(uint64_t entry, uint64_t pml4, uint64_t user_stack_top);

void sched_init(void);                              //register the boot context as task 0
struct task* task_create(void (*entry)(void));      //spawn a new kernel task
void schedule(void);                                //pick next task and switch
void yield(void);                                   //cooperative hand-off
void task_exit(void);
void task_exit_with(int code);
void task_kill(int vector);
int sched_has_other_runnable(void);                //for the boot loop
void task_reap(struct task* t);     //unlink a DONE task from the run queue and free it
struct task* sched_current(void);   // the task that is running right now

#endif