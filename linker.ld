ENTRY(_start)

KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
    . = 1M;

    /* ---------- Low-mapped boot region ----------
       Runs in 32-bit mode and during the long-mode trampoline, BEFORE the
       higher-half mapping exists. Early code references these as low
       addresses, so VMA == LMA == physical here.                          */
    .boot.text ALIGN(4K) :
    {
        *(.multiboot2)
        *(.boot.text)
    }

    .boot.rodata ALIGN(4K) :
    {
        *(.boot.rodata)
    }

    .boot.bss ALIGN(4K) :
    {
        *(.boot.bss)
    }

    /* ---------- Higher-half kernel ----------
       VMA in the top -2 GB (PML4 entry 511); LMA continues contiguously in
       low RAM via AT(), so GRUB still loads the whole image near 1 MB.     */
    . = ALIGN(4K);
    . += KERNEL_VMA;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);
    kernel_phys_end = . - KERNEL_VMA;   /* PHYSICAL end -> PMM bitmap placement */
    kernel_virt_end = .;
}
