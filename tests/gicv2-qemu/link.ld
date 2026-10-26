/* SPDX-License-Identifier: BSD-2-Clause-Patent */
ENTRY(_start)
SECTIONS
{
    . = 0x40080000;
    .text : { *(.text.boot) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : {
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        . += 0x10000;
        stack_top = .;
    }
}
