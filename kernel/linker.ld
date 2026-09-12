/* linker.ld - Скрипт линковщика для ядра */

ENTRY(start)

SECTIONS
{
    . = 1M;

    .boot :
    {
        *(.boot)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        __bss_end = .;
    }
}
