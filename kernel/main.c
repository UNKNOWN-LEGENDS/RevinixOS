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
#include "drivers/ata.h"
#include "fs/fat32.h"
#include "fs/vfs.h"
#include "drivers/keyboard.h"
#include "shell.h"

void irq_install(void);

extern void jump_usermode(uint64_t entry, uint64_t user_stack);

extern uint8_t user_program_start[];
extern uint8_t user_program_end[];

#define USER_ELF_LBA 2048           // sector where the Makefile writes hello.elf
#define USER_ELF_SECTORS 16         // 8 KB, generous for the ~5 KB ELF.
                                    // Day-1 hardcode; a real filesystem reports the true size later.

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

// static void usermode_test(void) {
//     uint64_t elf_size = (uint64_t)(hello_elf_end - hello_elf_start);
//     kprintf("Loading embedded ELF (%d bytes)...\n", (int)elf_size);

//     // 1) fresh address space: empty user half, shared kernel half
//     uint64_t proc = vmm_create_address_space();
//     kprintf("Created process address space: PML4 phys=%p (kernel PML4=%p)\n",
//             (void*)proc, (void*)vmm_kernel_pml4());

//     // 2) load the program INTO the process space (writes go via HHDM, so the
//     //    space need not be active yet)
//     uint64_t entry = elf_load(proc, hello_elf_start, elf_size);
//     if (!entry) { kprintf("ELF load failed.\n"); return; }

//     // 3) user stack, mapped in the process space
//     uint64_t stack_frame = pmm_alloc_frame();
//     vmm_map_page_in(proc, USER_STACK_VADDR, stack_frame, PAGE_WRITABLE | PAGE_USER);
//     uint64_t user_stack_top = USER_STACK_VADDR + 4096;

//     // 4) become the process address space, then drop to Ring 3 inside it
//     vmm_switch_address_space(proc);
//     kprintf("Switched to process address space. Entering Ring 3...\n");

//     // PROOF: allocate + free a frame while a process (no entry-0 identity map)
//     // is the active address space. Pre-fix this faults; post-fix it works.
//     uint64_t probe = pmm_alloc_frame();
//     kprintf("[PMM/HHDM] alloc while process active: frame=%p\n", (void*)probe);
//     pmm_free_frame(probe);

//     jump_usermode(entry, user_stack_top);
// }

struct fat32_fs g_fs;

struct task* spawn_user_process(const uint8_t* image, uint64_t size) {
    uint64_t pml4 = vmm_create_address_space();
    uint64_t entry = elf_load(pml4, image, size);
    if (!entry) { kprintf("ELF load failed.\n"); return NULL; }

    uint64_t sframe = pmm_alloc_frame();
    vmm_map_page_in(pml4, USER_STACK_VADDR, sframe, PAGE_WRITABLE | PAGE_USER);

    return task_create_user(entry, pml4, USER_STACK_VADDR + 4096);
}

// Load a named program through the VFS into a heap buffer sized to the file.
// Caller frees it once elf_load has copied the segments out.
static uint8_t* load_program(const char* name, uint64_t* out_size) {
    struct vfs_file file;
    if (vfs_open(name, &file) != 0) {
        kprintf("fs: '%s' not found\n", name);
        return NULL;
    }
    uint8_t* buf = (uint8_t*)kmalloc(file.size);
    if (!buf) { kprintf("fs: kmalloc(%d) failed for '%s'\n", (int)file.size, name); return NULL; }
    if (vfs_read(&file, buf, file.size) != (int)file.size) {
        kprintf("fs: read of '%s' failed\n", name);
        kfree(buf);
        return NULL;
    }
    *out_size = file.size;
    kprintf("fs: loaded '%s' via VFS (%d bytes)\n", name, (int)file.size);
    return buf;
}

static void disk_test(void) {
    uint8_t out[ATA_SECTOR_SIZE];
    uint8_t in[ATA_SECTOR_SIZE];

    // recognizable pattern
    for (int i = 0; i < ATA_SECTOR_SIZE; i++) out[i] = (uint8_t)(i ^ 0x5A);

    const uint32_t lba = 100;   // scratch sector, well clear of anything

    kprintf("disk: writing pattern to LBA %d...\n", (int)lba);
    if (ata_write(lba, 1, out) != 0) { kprintf("disk: WRITE FAILED\n"); return; }

    for (int i = 0; i < ATA_SECTOR_SIZE; i++) in[i] = 0;   // clear read buffer

    kprintf("disk: reading LBA %d back...\n", (int)lba);
    if (ata_read(lba, 1, in) != 0) { kprintf("disk: READ FAILED\n"); return; }

    int mismatch = -1;
    for (int i = 0; i < ATA_SECTOR_SIZE; i++)
        if (in[i] != out[i]) { mismatch = i; break; }

    if (mismatch < 0)
        kprintf("disk: ROUND-TRIP PASS (512 bytes match). first bytes: %x %x %x %x\n",
                in[0], in[1], in[2], in[3]);
    else
        kprintf("disk: MISMATCH at byte %d (wrote %x, read %x)\n",
                mismatch, out[mismatch], in[mismatch]);
}

void kmain(uint64_t mb2_magic, uint64_t mb2_info) {
    serial_init();
    uint64_t rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));
    kprintf("[STACK] kmain rsp=%p\n", (void*)rsp);
    kprintf("Kernel booted in 64-bit long mode.\n");

    gdt_init(); gdt_debug();     kprintf("GDT loaded.\n");
    idt_init();     kprintf("IDT loaded.\n");
    pic_remap();    irq_install();
    __asm__ volatile ("sti");
    kprintf("PIC remapped, timer + keyboard IRQs live.\n");

    pmm_init(mb2_info);
    vmm_init();
    pmm_use_hhdm();

    vga_clear();
    vga_print_at("Hello World! I am Dominic Andrew, creator of RevinixOS!", 0, 0);
    kprintf("VGA: printed hello world to screen. \n");

    heap_init();

    disk_test();

    // struct fat32_fs fs;
    int fs_ok = (fat32_init(&g_fs) == 0);
    if (fs_ok) {
        kprintf("fat32: geometry parsed successfully.\n");

        // Walk the root directory's cluster chain from root_cluster to EOC.
        kprintf("fat32: walking root-dir chain from cluster %d:\n", (int)g_fs.root_cluster);
        uint32_t c = g_fs.root_cluster;
        int guard = 0;
        while (c < FAT32_EOC && c >= 2) {
            kprintf("   cluster %d\n", (int)c);
            c = fat32_next_cluster(&g_fs, c);
            if (++guard > 64) { kprintf("   (guard hit — chain too long, stopping)\n"); break; }
        }
        kprintf("fat32: end of chain (last next=%p)\n", (void*)(uint64_t)c);

        vfs_mount(&g_fs);

        fat32_list_root(&g_fs);

        struct fat32_file f;
        if (fat32_find(&g_fs, "HELLO.ELF", &f) == 0) {
            kprintf("fat32: found HELLO.ELF -> start_cluster=%d, size=%d bytes\n",
                    (int)f.start_cluster, (int)f.size);

            uint8_t* filebuf = (uint8_t*)kmalloc(f.size);
            if (filebuf && fat32_read_file(&g_fs, &f, filebuf, f.size) == (int)f.size) {
                kprintf("fat32: read %d bytes. ELF magic: %x %x %x %x\n",
                        (int)f.size, filebuf[0], filebuf[1], filebuf[2], filebuf[3]);
            } else {
                kprintf("fat32: file read FAILED\n");
            }
            if (filebuf) kfree(filebuf);
        } else {
            kprintf("fat32: HELLO.ELF NOT FOUND\n");
        }
    } else {
        kprintf("fat32: init FAILED.\n");
    }

    

    sched_init();
    syscall_init();         // <-- install int 0x80 before going to user mode
    kprintf("Testing user program with syscalls...\n");
    // struct task* ta = task_create(task_a);
    // struct task* tb = task_create(task_b);
    // kprintf("Created tasks: A id=%d, B id=%d\n", ta->id, tb->id);
    // kprintf("Starting PREEMPTIVE scheduler (no yields)...\n");
    // kprintf("Testing Ring 3 transition...\n");
    // kprintf("Creating two kernel tasks in separate address space...\n");

    // kprintf("\nKeyboard test — type a line and press Enter (backspace works):\n> ");
    // __asm__ volatile ("sti");            // enable interrupts so the keyboard IRQ fires
    // shell_run();
    // char line[128];
    // for (;;) {
    //     if (keyboard_poll_line(line, sizeof(line))) {
    //         kprintf("you typed: \"%s\"\n> ", line);
    //     }
    //     __asm__ volatile ("hlt");        // sleep until the next interrupt
    // }

    // while(sched_has_other_runnable())
    //     yield();

    // kprintf("All tasks done. Back in boot context \n");

    // // --- test the heap ---
    // char* a = (char*)kmalloc(32);
    // char* b = (char*)kmalloc(100);
    // char* c = (char*)kmalloc(8);
    // kprintf("kmalloc gave: a=%p b=%p c=%p\n", a, b, c);

    // //write to them to prove they're real, independent memory
    // for(int i=0; i<31; i++) a[i] = 'A';
    // a[31] = '\0';
    // kprintf("a=%s\n", a);

    // kfree(b);                                   //free the middle one
    // char* d = (char*)kmalloc(64);
    // kprintf("after freeing b, kmalloc(64 gave: d=%p\n", d);

    // kfree(a); kfree(c); kfree(d);
    // kprintf("Freed all. Heap state: \n");
    // heap_dump();                            //should coalesce back toward one big block

    // // --- test map_page + translation on an unused higher-half address ---
    // uint64_t phys = pmm_alloc_frame();
    // uint64_t test_virt = 0xFFFFFF8000000000ull;    // PML4 slot 511, unused
    // vmm_map_page(test_virt, phys, PAGE_WRITABLE);

    // volatile uint64_t* p = (volatile uint64_t*)test_virt;
    // *p = 0xCAFEBABEDEADBEEFull;
    // kprintf("VMM test: wrote/read back %p\n", (void*)*p);
    // kprintf("VMM test: virt %p -> phys %p (allocated %p)\n",
    //         (void*)test_virt, (void*)vmm_get_phys(test_virt), (void*)phys);

    // // --- test HHDM access to the same physical frame ---
    // volatile uint64_t* h = (volatile uint64_t*)phys_to_virt(phys);
    // kprintf("VMM test: same frame via HHDM reads %p\n", (void*)*h);

    (void)mb2_magic;
    for (;;) __asm__ volatile ("sti");      //interrupts on: keyboard + timer
    shell_run();                            //interactive shell - never returns
}
