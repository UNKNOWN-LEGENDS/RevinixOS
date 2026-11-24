ENTRY(_start)
SECTIONS
{
    . = 0x400000;                 /* USER_IMAGE_MIN in kernel/user/layout.h */
    .text : { *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }

    /* Writable data starts on a fresh page: the code page is r-x and the data
       page rw-, so they must never share a 4 KB page (W^X). */
    . = ALIGN(0x1000);
    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.*) *(COMMON) }

    /* Compiler leftovers we don't load. */
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}