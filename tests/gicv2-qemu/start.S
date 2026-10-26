// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// Bare-metal AArch64 entry and IRQ vector for the GICv2 QEMU test.

    .section .text.boot, "ax"
    .global _start
_start:
    msr spsel, #1
    ldr x0, =stack_top
    mov sp, x0
    bl main
1:
    wfi
    b 1b

    .align 11
    .global vectors
vectors:
    .balign 0x80
    b unexpected          // 0x000 Current EL with SP0, synchronous
    .balign 0x80
    b unexpected          // 0x080 Current EL with SP0, IRQ
    .balign 0x80
    b unexpected          // 0x100 Current EL with SP0, FIQ
    .balign 0x80
    b unexpected          // 0x180 Current EL with SP0, SError
    .balign 0x80
    b unexpected          // 0x200 Current EL with SPx, synchronous
    .balign 0x80
    b irq_entry           // 0x280 Current EL with SPx, IRQ
    .balign 0x80
    b unexpected          // 0x300 Current EL with SPx, FIQ
    .balign 0x80
    b unexpected          // 0x380 Current EL with SPx, SError
    .balign 0x80
    b unexpected          // 0x400 lower EL AArch64, synchronous
    .balign 0x80
    b unexpected          // 0x480 lower EL AArch64, IRQ
    .balign 0x80
    b unexpected          // 0x500 lower EL AArch64, FIQ
    .balign 0x80
    b unexpected          // 0x580 lower EL AArch64, SError
    .balign 0x80
    b unexpected          // 0x600 lower EL AArch32, synchronous
    .balign 0x80
    b unexpected          // 0x680 lower EL AArch32, IRQ
    .balign 0x80
    b unexpected          // 0x700 lower EL AArch32, FIQ
    .balign 0x80
    b unexpected          // 0x780 lower EL AArch32, SError

irq_entry:
    sub sp, sp, #176
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    str x18, [sp, #144]
    stp x29, x30, [sp, #152]
    bl irq_handler
    ldp x29, x30, [sp, #152]
    ldr x18, [sp, #144]
    ldp x16, x17, [sp, #128]
    ldp x14, x15, [sp, #112]
    ldp x12, x13, [sp, #96]
    ldp x10, x11, [sp, #80]
    ldp x8, x9, [sp, #64]
    ldp x6, x7, [sp, #48]
    ldp x4, x5, [sp, #32]
    ldp x2, x3, [sp, #16]
    ldp x0, x1, [sp, #0]
    add sp, sp, #176
    eret

unexpected:
    b unexpected
