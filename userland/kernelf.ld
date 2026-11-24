/* Linker script for the KERNELF.ELF test only: normal code at 0x400000, but
   its read-only data placed in the kernel half, which the loader must refuse. */
ENTRY(_start)
SECTIONS
{
    . = 0x400000;
    .text : { *(.text) }
    . = 0xFFFF800000100000;
    .rodata : { *(.rodata) }
}