CC := x86_64-elf-gcc
LD := x86_64-elf-gcc
ASM := nasm

#CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -nostdlib -Wall -Wextra -c
#CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mcmodel=kernel -mno-red-zone -nostdlib -Wall -Wextra -Ikernel -c
CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -nostdlib -Wall -Wextra -Ikernel -c
CFLAGS += -mno-sse -mno-sse2 -mno-mmx -mno-80387
CFLAGS += $(EXTRA)
LDFLAGS := -ffreestanding -nostdlib -T linker.ld -z max-page-size=0x1000

BUILD := build
ISO_DIR := iso
ISO = myos.iso

#OBJS := $(BUILD)/boot.o $(BUILD)/main.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
#        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
#        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
#        $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/idt_load.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
#        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
#        $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/idt_load.o \
#        $(BUILD)/pic.o $(BUILD)/irq.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
#        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
#        $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/idt_load.o \
#        $(BUILD)/pic.o $(BUILD)/irq.o \
#        $(BUILD)/pmm.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
#        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
#        $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/idt_load.o \
#        $(BUILD)/pic.o $(BUILD)/irq.o \
#        $(BUILD)/pmm.o $(BUILD)/vmm.o $(BUILD)/vga.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
#        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
#        $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/idt_load.o \
#        $(BUILD)/pic.o $(BUILD)/irq.o \
#        $(BUILD)/pmm.o $(BUILD)/vmm.o $(BUILD)/vga.o $(BUILD)/heap.o
#OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
#        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
#        $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/idt_load.o \
#        $(BUILD)/pic.o $(BUILD)/irq.o \
#        $(BUILD)/pmm.o $(BUILD)/vmm.o $(BUILD)/vga.o $(BUILD)/heap.o \
#        $(BUILD)/sched.o $(BUILD)/context_switch.o $(BUILD)/task_entry.o \
#	$(BUILD)/usermode.o
OBJS := $(BUILD)/boot.o $(BUILD)/main.o $(BUILD)/serial.o $(BUILD)/kprintf.o \
        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
        $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/idt_load.o \
        $(BUILD)/pic.o $(BUILD)/irq.o \
        $(BUILD)/pmm.o $(BUILD)/vmm.o $(BUILD)/vga.o $(BUILD)/heap.o \
        $(BUILD)/sched.o $(BUILD)/context_switch.o $(BUILD)/task_entry.o \
        $(BUILD)/usermode.o $(BUILD)/syscall.o $(BUILD)/syscall_entry.o \
        $(BUILD)/user_program.o $(BUILD)/elf.o \
		$(BUILD)/ata.o $(BUILD)/fat32.o $(BUILD)/vfs.o $(BUILD)/keyboard.o \
		$(BUILD)/shell.o $(BUILD)/ramdisk.o $(BUILD)/bootinfo.o \
		$(BUILD)/rtc.o $(BUILD)/font8x16.o $(BUILD)/gfx.o $(BUILD)/fbcon.o \
		$(BUILD)/desktop.o $(BUILD)/mouse.o $(BUILD)/uaccess.o $(BUILD)/fpu.o \
		$(BUILD)/file.o

all: $(ISO_DIR)/boot/kernel.bin myos.iso

$(BUILD)/file.o: kernel/fs/file.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/fpu.o: kernel/arch/x86_64/fpu.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/uaccess.o: kernel/mm/uaccess.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/mouse.o: kernel/drivers/mouse.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/rtc.o: kernel/drivers/rtc.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/font8x16.o: kernel/gfx/font8x16.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/gfx.o: kernel/gfx/gfx.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/fbcon.o: kernel/gfx/fbcon.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/desktop.o: kernel/gui/desktop.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/bootinfo.o: kernel/bootinfo.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ramdisk.o: kernel/drivers/ramdisk.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/shell.o: kernel/shell.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c kernel/shell.c -o $@

$(BUILD)/keyboard.o: kernel/drivers/keyboard.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c kernel/drivers/keyboard.c -o $@

$(BUILD)/vfs.o: kernel/fs/vfs.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c kernel/fs/vfs.c -o $@

$(BUILD)/fat32.o: kernel/fs/fat32.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c kernel/fs/fat32.c -o $@

$(BUILD)/elf.o: kernel/user/elf.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

# ---- C programs ----
# User programs are compiled differently from the kernel: no -mcmodel=kernel,
# SSE allowed (the kernel saves it per task), normal optimisation.
USER_CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -fno-pie -O2 \
               -Wall -Wextra -fno-asynchronous-unwind-tables \
               -I userland/libc/include
LIBC_OBJS := userland/libc/crt0.o userland/libc/syscall.o userland/libc/string.o \
             userland/libc/stdio.o userland/libc/stdlib.o userland/libc/malloc.o \
			 userland/libc/time.o
LIBC_HDRS := $(wildcard userland/libc/include/*.h userland/libc/include/sys/*.h)

# Keep the libc objects between builds (make would otherwise delete them as
# "intermediate" files after each link).
.SECONDARY: $(LIBC_OBJS)

userland/libc/crt0.o: userland/libc/crt0.asm
	nasm -f elf64 $< -o $@

userland/libc/%.o: userland/libc/%.c $(LIBC_HDRS)
	$(CC) $(USER_CFLAGS) -c $< -o $@

# userland/NAME.c -> userland/NAME.elf, linked with crt0 + the mini libc
userland/%.elf: userland/%.c $(LIBC_OBJS) $(LIBC_HDRS) userland/user.ld
	$(CC) $(USER_CFLAGS) -c $< -o userland/$*.o
	x86_64-elf-ld -T userland/user.ld $(LIBC_OBJS) userland/$*.o -o $@

# Deliberately broken programs: linked at addresses the ELF loader must refuse.
userland/lowelf.elf: userland/badelf.asm userland/sys.inc
	nasm -f elf64 -i userland/ userland/badelf.asm -o userland/badelf.o
	x86_64-elf-ld -Ttext=0x1000 -e _start userland/badelf.o -o $@

userland/kernelf.elf: userland/badelf.asm userland/sys.inc userland/kernelf.ld
	nasm -f elf64 -i userland/ userland/badelf.asm -o userland/badelf.o
	x86_64-elf-ld -T userland/kernelf.ld userland/badelf.o -o $@

userland/%.elf: userland/%.asm userland/sys.inc userland/user.ld
	nasm -f elf64 -i userland/ $< -o userland/$*.o
	x86_64-elf-ld -T userland/user.ld userland/$*.o -o $@

$(BUILD)/ata.o: kernel/drivers/ata.c
	$(CC) $(CFLAGS) -c kernel/drivers/ata.c -o $@

$(BUILD)/syscall.o: kernel/syscall/syscall.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/syscall_entry.o: kernel/syscall/syscall.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/user_program.o: kernel/user/user_program.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/usermode.o: kernel/arch/x86_64/usermode.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/task_entry.o: kernel/sched/task_entry.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/sched.o: kernel/sched/sched.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/context_switch.o: kernel/sched/context_switch.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/heap.o: kernel/mm/heap.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/vga.o: kernel/drivers/vga.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/vmm.o: kernel/mm/vmm.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/pmm.o: kernel/mm/pmm.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/pic.o: kernel/arch/x86_64/pic.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/irq.o: kernel/arch/x86_64/irq.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/idt.o: kernel/arch/x86_64/idt.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/isr.o: kernel/arch/x86_64/isr.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/idt_load.o: kernel/arch/x86_64/idt_load.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/gdt.o: kernel/arch/x86_64/gdt.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/gdt_flush.o: kernel/arch/x86_64/gdt_flush.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/serial.o: kernel/drivers/serial.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/kprintf.o: kernel/kprintf.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/boot.o: boot/boot.asm
	mkdir -p $(BUILD)
	$(ASM) -f elf64 $< -o $@

$(BUILD)/main.o: kernel/main.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(ISO_DIR)/boot/kernel.bin: $(OBJS) linker.ld
	mkdir -p $(ISO_DIR)/boot/grub
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

run: $(ISO) disk-load
	qemu-system-x86_64 -cdrom myos.iso \
		-drive file=disk.img,format=raw,if=ide \
		-boot order=d \
		-serial stdio

disk.img: userland/hello.elf userland/ptrtest.elf userland/ssetest.elf \
          userland/faulttest.elf userland/nxtest.elf userland/wxtest.elf \
		  userland/cattest.elf userland/echotest.elf userland/stacktest.elf \
		  userland/lowelf.elf userland/kernelf.elf userland/chello.elf \
		  userland/memtest.elf userland/timetest.elf
	qemu-img create -f raw disk.img 16M
	mkfs.vfat -F 32 -n REVINIX disk.img
	mcopy -i disk.img userland/hello.elf ::/HELLO.ELF
	mcopy -i disk.img userland/ptrtest.elf ::/PTRTEST.ELF
	mcopy -i disk.img userland/readme.txt ::/README.TXT
	mcopy -i disk.img userland/ssetest.elf ::/SSETEST.ELF
	mcopy -i disk.img userland/faulttest.elf ::/FAULTTST.ELF
	mcopy -i disk.img userland/nxtest.elf ::/NXTEST.ELF
	mcopy -i disk.img userland/wxtest.elf ::/WXTEST.ELF
	mcopy -i disk.img userland/cattest.elf ::/CATTEST.ELF
	mcopy -i disk.img userland/echotest.elf ::/ECHOTEST.ELF
	mcopy -i disk.img userland/stacktest.elf ::/STACKTST.ELF
	mcopy -i disk.img userland/lowelf.elf ::/LOWELF.ELF
	mcopy -i disk.img userland/kernelf.elf ::/KERNELF.ELF
	mcopy -i disk.img userland/chello.elf ::/CHELLO.ELF
	mcopy -i disk.img userland/memtest.elf ::/MEMTEST.ELF
	mcopy -i disk.img userland/timetest.elf ::/TIMETEST.ELF

myos.iso: $(ISO_DIR)/boot/kernel.bin disk.img
	cp disk.img $(ISO_DIR)/boot/disk.img
	grub-mkrescue -o myos.iso $(ISO_DIR)

# run: myos.iso
# 	qemu-system-x86_64 -cdrom myos.iso -serial stdio

clean:
	rm -rf $(BUILD) userland/libc/*.o myos.iso $(ISO_DIR)/boot/kernel.bin $(ISO_DIR)/boot/disk.img

.PHONY: disk-load
disk-load: disk.img