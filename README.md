# RevinixOS

A ground-up x86-64 operating system kernel, written from scratch in C and NASM assembly — not a Linux fork, not a configuration of an existing kernel — targeting hardware the industry has declared obsolete.

> **Status: Stage 1 complete.** The kernel boots, manages memory, runs isolated preemptive processes in user mode, mounts a real FAT32 filesystem, and drops into an **interactive shell** that lists, reads, and executes programs off the disk by name — rendered on-screen, in QEMU's display, with a live keyboard.

---

## Why this project exists

Two problems, addressed together:

1. **Modern operating systems have abandoned older hardware.** Machines that are 8–12 years old — mechanical HDD, 1 GB RAM, no GPU — are fully functional but get thrown out as e-waste because current OSes won't run acceptably on them.
2. **Existing lightweight Linux distributions solve efficiency but not usability.** A large share of everyday productivity software is Windows-only and doesn't run accessibly on a stripped-down Linux box. A lightweight OS that can't run the software people actually need isn't a daily driver.

**RevinixOS aims at the intersection:** light enough for a decade-old laptop, and eventually capable of running Windows productivity software via API translation (not emulation), through a familiar interface usable by non-technical people out of the box.

**Two stages:**
- **Stage 1 (this repository — complete):** an original x86-64 monolithic kernel, written from scratch, to actually learn where resource costs live on constrained hardware — something you can't learn by configuring someone else's kernel.
- **Stage 2 (planned):** evolve into a full Linux-based distribution — Wine translation layer, HDD-tuned I/O, a lightweight desktop — built on the systems knowledge from Stage 1.

**Target hardware profile:** x86-64, single/dual core, 1 GB RAM design floor, mechanical HDD (no SSD assumed), no discrete GPU, legacy BIOS boot.

---

## Demo, in one path

Boot the kernel and you land at a shell running on the machine's own screen:

```
RevinixOS shell. Type 'help'.
$ ls
   HELLO.ELF   (cluster 3, 4872 bytes)
   README.TXT  (cluster 13, 122 bytes)
$ cat README.TXT
RevinixOS: a kernel written from scratch.
$ run HELLO.ELF
run: started 'HELLO.ELF' (task 1)
[user] ring3 tick
... (runs in an isolated Ring 3 process, preempted by the timer) ...
run: 'HELLO.ELF' finished.
$
```

`run` loads an ELF executable **off the FAT32 filesystem by name**, into its **own isolated virtual address space**, executes it in **user mode (Ring 3)** where it's **preempted by the timer**, and returns to the prompt when it exits. That single command exercises nearly every subsystem below.

---

## What's implemented (Stage 1)

Everything below is written from scratch, tested in QEMU, and committed. Each subsystem was built and **proven in isolation** before the next was layered on — see [Development methodology](#development-methodology).

### Boot & CPU
- Multiboot2 boot via GRUB; 32-bit → **long mode (64-bit)** transition (CPUID checks → PAE → 4-level paging → EFER.LME)
- **Higher-half kernel**: relinked to `0xFFFFFFFF80000000` (loaded low, run high via a linker VMA/LMA split) — the standard foundation for per-process address-space isolation
- GDT + TSS, a full IDT with exception handlers (page-fault `CR2` reporting), and 8259 PIC / PIT / IRQ handling

### Memory
- **Physical memory manager** — parses the Multiboot2 memory map; bitmap frame allocator
- **Virtual memory manager** — custom 4-level page tables, a higher-half direct map (HHDM) of all physical RAM, per-address-space mapping primitives
- **Kernel heap** — free-list allocator with block splitting and bidirectional coalescing

### Processes & scheduling
- **Preemptive, timer-driven scheduler** — cooperative context switching proven first, then extended to preemption
- **Per-process isolated address spaces** — each process gets its own PML4; the kernel half is shared across all of them by reference; the scheduler swaps CR3 (and per-task `rsp0`) on every context switch
- **Ring 3 user mode**, a **custom `int 0x80` syscall interface**, and an **ELF64 loader** that loads program segments into a target address space
- **Full Ring 3 preemption** — a looping user process is preempted mid-execution by the timer and correctly resumed, its complete register state preserved across the switch, interleaved with a separate kernel task in a different address space

### Storage & filesystem
- **ATA PIO disk driver** — LBA28 sector read/write over programmed I/O (round-trip verified)
- **FAT32 filesystem (read path)** — boot-sector/BPB parsing, FAT cluster-chain walking, root-directory parsing, 8.3 filename lookup, and reading files by their true size
- **VFS layer** — a thin `open`/`read` abstraction (operations table) so the rest of the kernel opens files generically; the filesystem sits behind the interface and callers never name FAT32

### Interface
- **PS/2 keyboard driver** — scancode→ASCII translation, shift handling, backspace, line buffering
- **VGA text output** — direct framebuffer rendering with scrolling and a hardware cursor that tracks typing; kernel output is mirrored to both the screen and a serial debug channel
- **Interactive shell** — `ls`, `cat <file>`, `run <file>`, `help`; `run` launches a program, waits for it to exit, reaps the finished task, and returns to the prompt

---

## Key design decisions

| Decision | Choice | Why | Planned evolution |
|---|---|---|---|
| Kernel architecture | Monolithic | Lower overhead on a weak single core; no IPC tax | — |
| Physical allocator | Bitmap | Simple, verifiable, unblocks paging immediately | → buddy allocator |
| Heap allocator | Free-list | Correct `kmalloc`/`kfree` with minimal code | → slab allocator |
| Page-table access | Higher-half direct map (HHDM) | Generalizes better than recursive mapping | — |
| Scheduling | Cooperative → preemptive | Proved the switch mechanism before adding preemption | → priorities / fairness |
| Syscalls | `int 0x80` | Far easier to debug than `syscall`/`sysret` while young | → fast `syscall` |
| Filesystem | FAT32, read-only | Standard, host-tooling-friendly; a real FS to build the VFS on | → read/write, subdirs, long names |
| Filesystem access | VFS operations table | The loader/shell depend on the interface, not the FS | → multiple backends (e.g. ext2) |

Interfaces are deliberately kept stable across upgrades — `pmm_alloc_frame`/`kmalloc`/`vfs_read` keep their signatures when the allocator or filesystem behind them is replaced.

---

## Development methodology

This isn't built by generating large amounts of code at once — it's built one subsystem at a time, deliberately:

1. Design and reason through the mechanism before writing code.
2. Implement exactly one subsystem.
3. Define the expected output and what each line of it proves.
4. Build and test in QEMU; verify against the expectation.
5. Debug from real serial/QEMU fault output — not from theory.
6. Commit the working state before moving on.

Each risky mechanism was proven alone before the next stacked on it: cooperative scheduling before preemption; the Ring 3 transition (via a hand-assembled spin loop) before syscalls existed; the ELF loader (on an embedded binary) before any disk; the CR3-swap plumbing (with kernel tasks) before Ring 3 processes were scheduled; the keyboard input layer (echoing lines) before a shell parsed them.

That discipline paid off directly. Every bug found across the project — double faults, a physical-memory bitmap overlapping the boot-info structure, preemption silently dying after one context switch, a GDT selector privilege mismatch, a 1 MB virtual-memory mapping offset, a keyboard/IRQ data race, drawing to video memory before it was mapped — was root-caused by isolating exactly which single new mechanism could have caused it, then confirming with `objdump` disassembly and `qemu -d int -no-reboot` fault dumps rather than by guessing.

---

## Technical stack

| Layer | Tooling |
|---|---|
| Languages | C (freestanding, `-ffreestanding -nostdlib -mcmodel=kernel`), x86-64 assembly (NASM) |
| Cross-compiler | `x86_64-elf-gcc` + `x86_64-elf-binutils`, built from source |
| Boot | GRUB2, Multiboot2, `grub-mkrescue`, `xorriso` |
| Filesystem tooling | `mkfs.vfat` (dosfstools), `mcopy`/`mdir` (mtools) — host-side FAT32 image creation |
| Test / debug | QEMU (`qemu-system-x86_64`), GDB, Bochs |
| Build | GNU Make, custom linker scripts |
| Version control | Git |

---

## Repository layout

```
boot/boot.asm            multiboot2 header + long-mode / higher-half trampoline
kernel/
  arch/x86_64/           GDT, IDT, PIC/IRQ, ISR stubs, Ring 3 entry, port I/O
  mm/                    physical + virtual memory managers, kernel heap
  sched/                 scheduler, context switch, task entry
  syscall/               int 0x80 dispatcher
  drivers/               serial (UART), VGA text, ATA disk, PS/2 keyboard
  fs/                    FAT32 driver, VFS layer
  user/                  ELF64 loader
  shell.c                interactive shell
  kprintf.c, main.c      kernel printf (serial + VGA), entry point
userland/                standalone user-mode ELF program + linker script
linker.ld                kernel linker script (higher-half VMA/LMA split)
Makefile
```

---

## Building and running

Requires an `x86_64-elf` cross-toolchain, NASM, QEMU, GRUB tools (`grub-mkrescue`, `xorriso`), and FAT tooling (`dosfstools`, `mtools`).

```bash
rm -f disk.img && make clean && make run
```

This builds the kernel, generates a bootable ISO, creates and FAT32-formats a disk image (placing the user program and a text file in its root), and launches QEMU with the ISO as boot media and the disk image as a writable ATA drive.

- **The shell runs in the QEMU graphical window** — click into it and type (`help`, `ls`, `cat README.TXT`, `run HELLO.ELF`).
- **Kernel diagnostics also stream to the serial console** (the terminal running `make run`), which doubles as the debugging channel.

For fault debugging:
```bash
qemu-system-x86_64 -cdrom myos.iso -drive file=disk.img,format=raw,if=ide -boot order=d -serial stdio -d int -no-reboot
```

---

## Roadmap

Stage 1 is complete. Known limitations are tracked deliberately, not hidden — the honest edges are the roadmap:

- **Filesystem is read-only**, root-directory-only, 8.3-short-names-only. Write support (enabling `touch`/`cp`/`mv`), subdirectories, and long filenames are the next filesystem arc.
- **Syscall user-pointer validation** is not yet implemented — the one deliberate security gap before running untrusted code.
- **FPU/SSE state is not saved on context switch**, so compiled-C user programs aren't safe yet (current user programs are hand-written assembly).
- Allocators are the simple-but-correct versions (bitmap PMM, free-list heap); buddy/slab replacements are planned behind the same interfaces.
- Real-hardware bring-up (ATA drive detection, PS/2 controller init, APIC) is deferred until the project moves off QEMU.

**Stage 2** pivots to a Linux-based distribution with a Wine translation layer, HDD-tuned I/O, and a lightweight desktop.

---

## Academic context

Developed as a university project (Project Work Phase 1) under faculty guidance, with accompanying technical documentation and presentations tracking design decisions, debugging history, and known technical debt as the kernel grew.

---

## License

*(Add a license here — MIT / GPL / BSD, whichever fits your intentions for the project.)*
