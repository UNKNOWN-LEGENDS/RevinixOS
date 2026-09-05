ENTRY(_start)

SECTIONS
{
    . = 1M;

    .boot : ALIGN(4K)
    {
        *(.multiboot2)
    }

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);
    kernel_end = .;      /* physical/virtual end of kernel image */
}
