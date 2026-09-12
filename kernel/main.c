#include <stdint.h>
#include "drivers/serial.h"
#include "kprintf.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/pic.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "drivers/vga.h"
#include "mm/heap.h"
#include "sched/sched.h"
#include "user/elf.h"
#include "syscall/syscall.h"

void irq_install(void);

// static void task_a(void) {
//     int count=0;
//     for (;;) {
//         kprintf("   [Task A] count=%d\n", count++);
//         for (volatile int d=0; d<5000000; d++);         //burn time, NO yield
//     }
// }

// static void task_b(void) {
//     int count=0;
//     for (;;) {
//         kprintf("   [Task B] count=%d\n", count++);
//         for (volatile int d=0; d<5000000; d++);         //burn time, NO yield
//     }
// }

extern void jump_usermode(uint64_t entry, uint64_t user_stack);

extern uint8_t user_program_start[];
extern uint8_t user_program_end[];

extern uint8_t hello_elf_start[];
extern uint8_t hello_elf_end[];

#define USER_CODE_VADDR 0x0000000100000000ull   //4GB (above the identity map)
#define USER_STACK_VADDR 0x0000000200000000ull   //8GB

// static void usermode_test(void) {
//     // // user code page: an infinite loop written as raw machine code
//     // uint64_t code_frame = pmm_alloc_frame();
//     // vmm_map_page(USER_CODE_VADDR, code_frame, PAGE_WRITABLE | PAGE_USER);
//     // volatile uint8_t* code = (volatile uint8_t*)USER_CODE_VADDR;
//     // code[0] = 0xEB;         //JMP rel8
//     // code[1] = 0xFE;         //-2 -> jumps to itself: infinite loop

//     // // user stack page
//     // uint64_t stack_frame = pmm_alloc_frame();
//     // vmm_map_page(USER_STACK_VADDR, stack_frame, PAGE_WRITABLE | PAGE_USER);
//     // uint64_t user_stack_top = USER_STACK_VADDR + 4096;

//     // kprintf("Dropping to Ring 3 at %p (user will spin)...\n", (void*)USER_CODE_VADDR);

//     // // extern struct gdt_entry gdt[];      //if not accessible, see note below

//     // // kprintf("About to iretq: entry=%p stack=%p\n", (void*)USER_CODE_VADDR, (void*)user_stack_top);
//     // // kprintf("   user code byte[0]=%x byte[1]=%x\n", code[0], code[1]);
//     // // kprintf("   user code phys=%p\n", (void*)vmm_get_phys(USER_CODE_VADDR));
//     // // kprintf("   user stack phys=%p\n", (void*)vmm_get_phys(USER_STACK_VADDR));

//     // // verify the PAGE_USER bit is actually set on the mapping
//     // kprintf("   code mapping present+user check via get_phys above\n");

//     // jump_usermode(USER_CODE_VADDR, user_stack_top);
//     // //never returns, user code spins, timer periodically traps in

//     uint64_t prog_size = (uint64_t)(user_program_end - user_program_start);

//     // user code page
//     uint64_t code_frame = pmm_alloc_frame();
//     vmm_map_page(USER_CODE_VADDR, code_frame, PAGE_WRITABLE | PAGE_USER);

//     // copy the program bytes into the user page
//     uint8_t* dst = (uint8_t*)USER_CODE_VADDR;
//     for (uint64_t i = 0; i<prog_size; i++) dst[i] = user_program_start[i];

//     // user stack page
//     uint64_t stack_frame = pmm_alloc_frame();
//     vmm_map_page(USER_STACK_VADDR, stack_frame, PAGE_WRITABLE | PAGE_USER);
//     uint64_t user_stack_top = USER_STACK_VADDR + 4096;
//     jump_usermode(USER_CODE_VADDR, user_stack_top);
// }

static void usermode_test(void) {
    uint64_t elf_size = (uint64_t)(hello_elf_end - hello_elf_start);
    kprintf("Loading embedded ELF (%d bytes)...\n", (int)elf_size);

    uint64_t entry = elf_load(hello_elf_start, elf_size);
    if (!entry) {
        kprintf("ELF load failed.\n");
        return;
    }

    // user stack (unchanged)
    uint64_t stack_frame = pmm_alloc_frame();
    vmm_map_page(USER_STACK_VADDR, stack_frame, PAGE_WRITABLE | PAGE_USER);
    uint64_t user_stack_top = USER_STACK_VADDR + 4096;

    kprintf("Jumping to ELF entry point in Ring 3...\n");
    jump_usermode(entry, user_stack_top);
}

void kmain(uint64_t mb2_magic, uint64_t mb2_info) {
    serial_init();
    kprintf("Kernel booted in 64-bit long mode.\n");

    gdt_init(); gdt_debug();     kprintf("GDT loaded.\n");
    idt_init();     kprintf("IDT loaded.\n");
    pic_remap();    irq_install();
    __asm__ volatile ("sti");
    kprintf("PIC remapped, timer + keyboard IRQs live.\n");

    pmm_init(mb2_info);
    vmm_init();

    vga_clear();
    vga_print_at("Hello World! I am Dominic Andrew, creator of RevinixOS!", 0, 0);
    kprintf("VGA: printed hello world to screen. \n");

    heap_init();

    sched_init();
    syscall_init();         // <-- install int 0x80 before going to user mode
    kprintf("Testing user program with syscalls...\n");
    // struct task* ta = task_create(task_a);
    // struct task* tb = task_create(task_b);
    // kprintf("Created tasks: A id=%d, B id=%d\n", ta->id, tb->id);
    // kprintf("Starting PREEMPTIVE scheduler (no yields)...\n");
    kprintf("Testing Ring 3 transition...\n");
    usermode_test();
    //unreachable

    __asm__ volatile ("sti");               //ake sure interrupts are on
    for (;;) __asm__ volatile ("hlt");      //boot task idles; timer drives switching

    while(sched_has_other_runnable())
        yield();

    kprintf("All tasks done. Back in boot context \n");

    // --- test the heap ---
    char* a = (char*)kmalloc(32);
    char* b = (char*)kmalloc(100);
    char* c = (char*)kmalloc(8);
    kprintf("kmalloc gave: a=%p b=%p c=%p\n", a, b, c);

    //write to them to prove they're real, independent memory
    for(int i=0; i<31; i++) a[i] = 'A';
    a[31] = '\0';
    kprintf("a=%s\n", a);

    kfree(b);                                   //free the middle one
    char* d = (char*)kmalloc(64);
    kprintf("after freeing b, kmalloc(64 gave: d=%p\n", d);

    kfree(a); kfree(c); kfree(d);
    kprintf("Freed all. Heap state: \n");
    heap_dump();                            //should coalesce back toward one big block

    // --- test map_page + translation on an unused higher-half address ---
    uint64_t phys = pmm_alloc_frame();
    uint64_t test_virt = 0xFFFFFF8000000000ull;    // PML4 slot 511, unused
    vmm_map_page(test_virt, phys, PAGE_WRITABLE);

    volatile uint64_t* p = (volatile uint64_t*)test_virt;
    *p = 0xCAFEBABEDEADBEEFull;
    kprintf("VMM test: wrote/read back %p\n", (void*)*p);
    kprintf("VMM test: virt %p -> phys %p (allocated %p)\n",
            (void*)test_virt, (void*)vmm_get_phys(test_virt), (void*)phys);

    // --- test HHDM access to the same physical frame ---
    volatile uint64_t* h = (volatile uint64_t*)phys_to_virt(phys);
    kprintf("VMM test: same frame via HHDM reads %p\n", (void*)*h);

    (void)mb2_magic;
    for (;;) __asm__ volatile ("hlt");
}
