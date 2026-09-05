#ifndef SCHED_H
#define SCHED_H

#include<stdint.h>
enum task_state { TASK_READY, TASK_RUNNING, TASK_DONE };

struct task {
    uint64_t rsp;               //saved stack pointer (must be first field)
    void* stack_base;           //heap allocation, for freeing later
    int id;
    enum task_state state;
    struct task* next;          //circular run queue
};

void sched_init(void);                              //register the boot context as task 0
struct task* task_create(void (*entry)(void));      //spawn a new kernel task
void schedule(void);                                //pick next task and switch
void yield(void);                                   //cooperative hand-off
void task_exit(void);                               //end current task
int sched_has_other_runnable(void);                //for the boot loop

#endif