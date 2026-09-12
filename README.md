# RevinixOS

A ground-up x86-64 operating system kernel, written from scratch in C and NASM assembly — not a Linux fork, not a config of an existing kernel — targeting hardware the industry has declared obsolete.

> **Status:** Stage 1 (custom kernel) in active development. Ring 3 user mode, syscalls, and an ELF64 loader are working end-to-end in QEMU. Currently building per-process address spaces on a freshly-relinked higher-half kernel.

---

## Why this project exists

Two problems, addressed together:

1. **Modern operating systems have abandoned older hardware.** Machines that are 8–12 years old — mechanical HDD, 1 GB RAM, no GPU — are fully functional but get thrown out as e-waste because current OSes won't run acceptably on them.
2. **Existing lightweight Linux distributions solve efficiency but not usability.** A large share of everyday productivity software is Windows-only and doesn't run accessibly on a stripped-down Linux box. A lightweight OS that can't run the software people actually need isn't a daily driver.

**RevinixOS aims at the intersection:** light enough for a decade-old laptop, and eventually capable of running Windows productivity software via API translation (not emulation), through a familiar interface usable by non-technical people out of the box.

**Two stages:**
- **Stage 1 (current):** an original x86-64 monolithic kernel, written from scratch, to actually learn where resource costs live on constrained hardware — something you can't learn by configuring someone else's kernel.
- **Stage 2 (planned):** evolve into a full Linux-based distribution — Wine translation layer, HDD-tuned I/O, a lightweight desktop — built on top of the systems knowledge from Stage 1.

**Target hardware profile:** x86-64, single/dual core, 1 GB RAM design floor, mechanical HDD (no SSD assumed), no discrete GPU, legacy BIOS boot.

---

## What's implemented (Stage 1)

Everything below is written, tested in QEMU, and committed. Each subsystem was built and proven in isolation before the next was added — see [Development Methodology](#development-methodology).

| # | Subsystem | Notes |
|---|---|---|
| 1 | Toolchain & build pipeline | Cross-compiler (`x86_64-elf-gcc`/`binutils`) built from source; Makefile; `grub-mkrescue` ISO pipeline |
| 2 | Multiboot2 boot | GRUB loads the kernel at 1 MB |
| 3 | Long mode (64-bit) | CPUID feature checks → PAE → PML4/PDPT/PD with 2 MB huge pages → EFER.LME → CR0.PG → far jump into 64-bit |
| 4 | Serial output + `kprintf` | 16550 UART driver (polled), custom `printf`-style formatter (`%s %d %x %c %p`) — all kernel diagnostics go over serial |
| 5 | GDT + TSS | Flat segmentation, 64-bit TSS descriptor, dedicated privilege-transition stack (`rsp0`) |
| 6 | IDT + exception handling | All 256 vectors installed, uniform interrupt-frame handling, page-fault `CR2` reporting |
| 7 | PIC remap + PIT + keyboard IRQ | 8259 remapped off the exception vectors, 100 Hz timer tick, raw scancode capture |
| 8 | Physical memory manager | Multiboot2 memory-map parsing, bitmap frame allocator |
| 9 | Virtual memory manager | Custom 4-level page tables, identity map, higher-half direct map (HHDM) for physical RAM access |
| 10 | VGA text-mode driver | Direct framebuffer writes at `0xB8000` |
| 11 | Kernel heap | Free-list allocator with splitting and bidirectional coalescing (`kmalloc`/`kfree`) |
| 12 | Cooperative scheduler | Context switch saving callee-saved registers, circular run queue |
| 13 | Preemptive scheduler | Timer-IRQ-driven `schedule()`, correct EOI ordering, safe entry path for freshly-created tasks |
| 14 | Ring 3 (user mode) | Faked `iretq` frames, privilege-level GDT selectors, verified kernel-stack switch on trap-in |
| 15 | Syscall interface | `int 0x80` gate, register-based calling convention, working `write`/`exit` |
| 16 | ELF64 loader | Parses ELF headers, loads `PT_LOAD` segments, zero-fills `.bss`, hands off to Ring 3 |
| 17 | **Higher-half kernel relink** | Kernel now links at `0xFFFFFFFF80000000` (PML4 entry 511), freeing the lower half of the address space for per-process user mappings |

**Proven end-to-end:** a standalone ELF binary is loaded to `0x100000000`, executed in **Ring 3**, prints through a `sys_write` syscall, and exits cleanly through `sys_exit` — from a kernel now running entirely out of the higher half of the virtual address space.

### In progress / next
Per-process address spaces → FPU/SSE state save-restore (required for compiled C in userspace) → CR3-switching process model → ATA PIO disk driver → VFS → FAT32/ext2 → PS/2 keyboard driver → expanded POSIX-style syscalls → interactive shell (Stage 1 completion criterion).

---

## Key design decisions

| Decision | Choice made | Why | Planned evolution |
|---|---|---|---|
| Kernel architecture | Monolithic | Lower overhead on a weak single core; no IPC tax; simpler integration with a future translation layer | — |
| Physical allocator | Bitmap | Simple, verifiable, unblocks paging immediately | → buddy allocator |
| Heap allocator | Free-list | Correct `kmalloc`/`kfree` with minimal code | → slab allocator |
| Page-table access | Higher-half direct map (HHDM) | Generalizes better than recursive page-table mapping | — |
| Scheduling | Cooperative → preemptive | Proved the context-switch mechanism in isolation before adding interrupt-driven preemption | → priority scheduling / CFS-style (Stage 2) |
| Syscall mechanism | `int 0x80` | Far easier to debug than `syscall`/`sysret` while the kernel is young | → fast `syscall` instruction |
| Kernel linking | Higher-half (`0xFFFFFFFF80000000`) | Standard prerequisite for per-process address-space isolation | — |

Interfaces are deliberately kept stable across upgrades — `pmm_alloc_frame`/`pmm_free_frame` and `kmalloc`/`kfree` will keep identical signatures when the buddy and slab allocators land.

---

## Development methodology

This isn't built by dumping large amounts of generated code — it's built one subsystem at a time, deliberately:

1. Design the mechanism and reason through it before writing code.
2. Implement exactly one subsystem.
3. Define the expected output and what each line of it proves.
4. Build and test in QEMU; verify against the expected output.
5. Debug from real serial/QEMU output — not from theory.
6. Commit the working state before moving on.

Concrete examples of proving one mechanism before stacking the next on top: cooperative scheduling was verified before preemption was layered onto the same context-switch code; the Ring 3 transition was proven with a hand-assembled 2-instruction spin loop before any syscalls existed; the ELF loader was proven against a binary embedded directly in the kernel image before any disk or filesystem code existed.

This discipline directly paid off — every bug found so far (double faults, a physical-memory-manager bitmap overlapping the multiboot2 info structure, preemption silently dying after one context switch, a GDT selector RPL mismatch) was root-caused by isolating exactly which single new mechanism could have caused it, then confirming with `objdump` disassembly and `qemu -d int -no-reboot` fault dumps rather than by guessing.

---

## Technical stack

| Layer | Tooling |
|---|---|
| Languages | C (freestanding, `-ffreestanding -nostdlib`), x86-64 assembly (NASM) |
| Cross-compiler | `x86_64-elf-gcc` + `x86_64-elf-binutils`, built from source |
| Assembler | NASM (`-f elf64`) |
| Boot | GRUB2, Multiboot2 protocol, `grub-mkrescue`, `xorriso`, `mtools` |
| Test/debug targets | QEMU (`qemu-system-x86_64`), GDB, Bochs |
| Build | GNU Make, custom linker scripts |
| Host environment | Windows + WSL2 (Ubuntu) |
| Version control | Git |

**Planned Stage 2 stack:** musl, BusyBox, runit, Wayland/Sway or Openbox, LXQt, PipeWire, Wine-Staging, WineD3D + lavapipe (software Vulkan — no GPU assumed), BFQ I/O scheduler, zram, ext4.

---

## Repository layout

```
boot/                   boot.asm — multiboot2 header + long-mode/higher-half trampoline
kernel/
  arch/x86_64/          GDT, IDT, PIC/IRQ, ISR stubs, Ring 3 entry
  mm/                   physical memory manager, virtual memory manager, kernel heap
  sched/                scheduler, context switch, task entry trampoline
  syscall/               int 0x80 dispatcher
  drivers/               serial (UART), VGA text mode
  user/                  ELF64 loader
  kprintf.c/h            kernel printf
  main.c                 kernel entry point
userland/               standalone user-mode ELF test program + its own linker script
linker.ld               kernel linker script (higher-half VMA/LMA split)
Makefile
```

---

## Building and running

Requires a `x86_64-elf` cross-toolchain, NASM, QEMU, and GRUB tools (`grub-mkrescue`, `xorriso`, `mtools`) — developed and tested under WSL2 (Ubuntu).

```bash
make clean && make run
```

This builds the kernel, generates a bootable ISO via GRUB, and launches it in QEMU. **All kernel diagnostics are printed over the serial port**, so the terminal running `make run` is where the real output appears — the QEMU graphical window is expected to stay blank aside from VGA text output.

For fault debugging:
```bash
qemu-system-x86_64 -cdrom myos.iso -serial stdio -d int -no-reboot
```

---

## Academic context

Developed as a university project (Project Work Phase 1) under faculty guidance, with accompanying technical documentation and presentations tracking design decisions, debugging history, and known technical debt as the kernel grows.

---

## License

*(Add a license here — MIT/GPL/BSD, whichever fits your intentions for the project.)*
