CC := x86_64-elf-gcc
LD := x86_64-elf-gcc
ASM := nasm

#CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -nostdlib -Wall -Wextra -c
#CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mcmodel=kernel -mno-red-zone -nostdlib -Wall -Wextra -Ikernel -c
CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -nostdlib -Wall -Wextra -Ikernel -c
LDFLAGS := -ffreestanding -nostdlib -T linker.ld -z max-page-size=0x1000

BUILD := build
ISO_DIR := iso

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
        $(BUILD)/user_program.o $(BUILD)/elf.o userland/hello_embed.o

all: $(ISO_DIR)/boot/kernel.bin myos.iso

$(BUILD)/elf.o: kernel/user/elf.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

userland/hello_embed.o: userland/hello.asm userland/user.ld
	nasm -f elf64 userland/hello.asm -o userland/hello.o
	x86_64-elf-ld -T userland/user.ld userland/hello.o -o userland/hello.elf
	x86_64-elf-objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
	    --redefine-sym _binary_userland_hello_elf_start=hello_elf_start \
	    --redefine-sym _binary_userland_hello_elf_end=hello_elf_end \
	    userland/hello.elf userland/hello_embed.o

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

myos.iso: $(ISO_DIR)/boot/kernel.bin
	grub-mkrescue -o myos.iso $(ISO_DIR)

run: myos.iso
	qemu-system-x86_64 -cdrom myos.iso -serial stdio

clean:
	rm -rf $(BUILD) myos.iso $(ISO_DIR)/boot/kernel.bin

.PHONY: all run clean
