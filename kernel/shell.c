#include "shell.h"
#include "kprintf.h"
#include "drivers/keyboard.h"
#include "fs/vfs.h"
#include "fs/fat32.h"
#include "mm/heap.h"
#include "sched/sched.h"
#include "user/elf.h"
#include "drivers/vga.h"
#include "mm/pmm.h"

// These live in main.c; declare them so the shell can launch a process.
extern struct fat32_fs g_fs;                       // the mounted filesystem (see main.c note)
struct task* spawn_user_process(const uint8_t* image, uint64_t size);

// --- tiny string helpers (no libc) ---
static int str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}
// Split "cmd arg" -> cmd in `line` (nul-terminated at the space), returns arg
// pointer (or empty string if no arg). Modifies `line` in place.
static char* split_arg(char* line) {
    char* p = line;
    while (*p && *p != ' ') p++;
    if (*p == ' ') { *p = '\0'; p++; while (*p == ' ') p++; return p; }
    return p;   // points at the trailing '\0' => empty arg
}

static void print_hex(uint64_t v, int digits) {
    static const char hex[] = "0123456789abcdef";
    for (int i = digits-1; i>=0; i--)
        kprintf("%c", hex[(v >> (i * 4)) & 0xF]);
}

static void cmd_help(void) {
    kprintf("commands:\n");
    kprintf("  ls              list files in the root directory\n");
    kprintf("  cat <file>      print a text file's contents\n");
    kprintf("  hexdump <file>  show a file's bytes (first 256)\n");
    kprintf("  run <file>      load and execute a program\n");
    kprintf("  echo <text>     print text\n");
    kprintf("  meminfo         physical memory and heap usage\n");
    kprintf("  clear           clear the screen\n");
    kprintf("  help            show this message\n");
}

static void cmd_clear(void) {
    vga_clear();          // screen only; the serial log keeps its history
}

static void cmd_echo(const char* text) {
    kprintf("%s\n", text);
}

static void cmd_meminfo(void) {
    uint64_t total = pmm_total_frames();
    uint64_t used  = pmm_used_frames();
    kprintf("physical memory (4 KB frames):\n");
    kprintf("  total          %d frames  (%d KB)\n", (int)total, (int)(total * 4));
    kprintf("  used/reserved  %d frames  (%d KB)\n", (int)used, (int)(used * 4));
    kprintf("  free           %d frames  (%d KB)\n", (int)(total - used), (int)((total - used) * 4));

    struct heap_stats h;
    heap_get_stats(&h);
    kprintf("kernel heap:\n");
    kprintf("  capacity       %d KB\n", (int)(h.capacity / 1024));
    kprintf("  used           %d bytes\n", (int)h.used);
    kprintf("  free           %d bytes  (largest block %d bytes)\n",
            (int)h.free, (int)h.largest_free);
    kprintf("  blocks         %d  (header overhead %d bytes)\n",
            (int)h.blocks, (int)(h.capacity - h.used - h.free));
}

#define HEXDUMP_MAX 256

static void cmd_hexdump(const char* name) {
    if (name[0] == '\0') { kprintf("usage: hexdump <file>\n"); return; }

    struct vfs_file f;
    if (vfs_open(name, &f) != 0) { kprintf("hexdump: '%s' not found\n", name); return; }
    uint8_t* buf = (uint8_t*)kmalloc(f.size ? f.size : 1);
    if (!buf) { kprintf("hexdump: out of memory\n"); return; }
    if (vfs_read(&f, buf, f.size) != (int)f.size) {
        kprintf("hexdump: read error\n"); kfree(buf); return;
    }

    uint32_t n = (f.size < HEXDUMP_MAX) ? f.size : HEXDUMP_MAX;
    for (uint32_t off = 0; off < n; off += 16) {
        print_hex(off, 8);
        kprintf("  ");
        for (uint32_t i = 0; i < 16; i++) {
            if (off + i < n) { print_hex(buf[off + i], 2); kprintf(" "); }
            else             { kprintf("   "); }
            if (i == 7) kprintf(" ");                  // gap between the two 8-byte halves
        }
        kprintf(" |");
        for (uint32_t i = 0; i < 16 && off + i < n; i++) {
            uint8_t c = buf[off + i];
            kprintf("%c", (c >= 32 && c < 127) ? c : '.');   // non-printables as '.'
        }
        kprintf("|\n");
    }
    if (f.size > n)
        kprintf("(showing first %d of %d bytes)\n", (int)n, (int)f.size);
    kfree(buf);
}

static void cmd_ls(void) {
    fat32_list_root(&g_fs);
}

static void cmd_cat(const char* name) {
    struct vfs_file f;
    if (vfs_open(name, &f) != 0) { kprintf("cat: '%s' not found\n", name); return; }
    uint8_t* buf = (uint8_t*)kmalloc(f.size + 1);
    if (!buf) { kprintf("cat: out of memory\n"); return; }
    if (vfs_read(&f, buf, f.size) != (int)f.size) { kprintf("cat: read error\n"); kfree(buf); return; }
    buf[f.size] = '\0';
    // print as text; kprintf %s stops at nul, which is fine for text files
    kprintf("%s\n", (char*)buf);
    kfree(buf);
}

static void cmd_run(const char* name) {
    struct vfs_file f;
    if (vfs_open(name, &f) != 0) { kprintf("run: '%s' not found\n", name); return; }
    uint8_t* image = (uint8_t*)kmalloc(f.size);
    if (!image) { kprintf("run: out of memory\n"); return; }
    if (vfs_read(&f, image, f.size) != (int)f.size) { kprintf("run: read error\n"); kfree(image); return; }
    struct task* proc = spawn_user_process(image, f.size);
    kfree(image);
    if (!proc) { kprintf("run: failed to start '%s'\n", name); return; }
    kprintf("run: started '%s' (task %d)\n", name, proc->id);

    while (proc->state != TASK_DONE)
    yield();

    task_reap(proc);

    // kprintf("run: '%s' finished.\n", name);
    kprintf("run: '%s' finished.\n", name);
}

void shell_run(void) {
    char line[128];
    kprintf("\nRevinixOS shell. Type 'help'.\n");
    kprintf("$ ");

    for (;;) {
        if (keyboard_poll_line(line, sizeof(line))) {
            // kprintf("[dbg] raw line: '%s'\n", line);
            char* arg = split_arg(line);      // line now holds just the command word

            if (line[0] == '\0')          { /* empty line */ }
            else if (str_eq(line, "help")) cmd_help();
            else if (str_eq(line, "ls"))   cmd_ls();
            else if (str_eq(line, "cat"))  cmd_cat(arg);
            else if (str_eq(line, "run"))  cmd_run(arg);
            else if (str_eq(line, "clear")) cmd_clear();
            else if (str_eq(line, "echo")) cmd_echo(arg);
            else if (str_eq(line, "meminfo")) cmd_meminfo();
            else if (str_eq(line, "hexdump")) cmd_hexdump(arg);
            else kprintf("unknown command: '%s' (try 'help')\n", line);

            kprintf("$ ");
        }
        __asm__ volatile ("hlt");
    }
}