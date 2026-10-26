// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// Standalone GICv2 register-sequence test for QEMU virt.
//
// This is not part of the XNU driver. It uses the same distributor, banked CPU
// interface, virtual-timer PPI 27 and self-SGI sequence as ArmCpu's GICv2 path,
// so a regression in the register setup can be reproduced without building XNU.

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long u64;

#define UART0       0x09000000UL
#define GICD        0x08000000UL
#define GICC        0x08010000UL

#define GICD_CTLR       (GICD + 0x000)
#define GICD_TYPER      (GICD + 0x004)
#define GICD_ISENABLER  (GICD + 0x100)
#define GICD_ICENABLER  (GICD + 0x180)
#define GICD_IPRIORITYR (GICD + 0x400)
#define GICD_ITARGETSR  (GICD + 0x800)
#define GICD_ICFGR      (GICD + 0xc00)
#define GICD_SGIR       (GICD + 0xf00)
#define GICC_CTLR       (GICC + 0x00)
#define GICC_PMR        (GICC + 0x04)
#define GICC_BPR        (GICC + 0x08)
#define GICC_IAR        (GICC + 0x0c)
#define GICC_EOIR       (GICC + 0x10)
#define GICC_RPR        (GICC + 0x14)
#define GICC_IIDR       (GICC + 0xfc)

extern char vectors[];
void irq_handler(void);

static volatile int seen_timer;
static volatile int seen_ipi;
static volatile int passed;

static inline u32 read32(u64 address)
{
    return *(volatile u32 *)address;
}

static inline void write32(u64 address, u32 value)
{
    *(volatile u32 *)address = value;
}

static inline void write8(u64 address, u8 value)
{
    *(volatile u8 *)address = value;
}

static inline void dsb(void)
{
    __asm__ volatile("dsb sy" ::: "memory");
}

static inline void isb(void)
{
    __asm__ volatile("isb" ::: "memory");
}

static void uart_init(void)
{
    write32(UART0 + 0x30, 0x301); // UARTEN | TXE | RXE
    write32(UART0 + 0x24, 13);   // IBRD for 115200 at 24 MHz
    write32(UART0 + 0x28, 37);   // FBRD
}

static void uart_putc(char c)
{
    while (read32(UART0 + 0x18) & (1U << 5)) {
    }
    write32(UART0, (u32)(unsigned char)c);
}

static void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

static void uart_puthex(u32 value)
{
    static const char digits[] = "0123456789abcdef";

    uart_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        uart_putc(digits[(value >> shift) & 0xf]);
    }
}

static u64 read_cntfrq(void)
{
    u64 value;

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

static void timer_start(void)
{
    const u64 frequency = read_cntfrq();
    const u64 tval = frequency / 10; // 100 ms

    __asm__ volatile("msr cntv_ctl_el0, %0; isb" :: "r"((u64)0) : "memory");
    __asm__ volatile("msr cntv_tval_el0, %0; isb" :: "r"(tval) : "memory");
    __asm__ volatile("msr cntv_ctl_el0, %0; isb" :: "r"((u64)1) : "memory");

    uart_puts("timer freq=");
    uart_puthex((u32)frequency);
    uart_putc('\n');
}

static void timer_stop(void)
{
    __asm__ volatile("msr cntv_ctl_el0, %0; isb" :: "r"((u64)0) : "memory");
}

// Mirrors ArmCpu::initDistributor() and ArmCpu::enableGic() for GICv2. The
// interrupt group is deliberately left alone: firmware owns that split.
static u32 gicv2_init(void)
{
    write32(GICD_CTLR, 0);
    dsb();

    u32 interrupts = ((read32(GICD_TYPER) & 31U) + 1U) * 32U;
    if (interrupts > 1020U) {
        interrupts = 1020U;
    }
    uart_puts("gic interrupts=");
    uart_puthex(interrupts);
    uart_putc('\n');

    for (u32 id = 0; id < interrupts; id += 32) {
        write32(GICD_ICENABLER + id / 8, 0xffffffffU);
    }
    for (u32 id = 0; id < interrupts; id += 4) {
        write32(GICD_IPRIORITYR + id, 0x80808080U);
    }

    u32 targets = read32(GICD_ITARGETSR);
    targets |= targets >> 16;
    targets |= targets >> 8;
    targets &= 0xffU;
    if (targets == 0) {
        targets = 1;
    }
    uart_puts("gic target=");
    uart_puthex(targets);
    uart_putc('\n');

    const u32 repeated = targets * 0x01010101U;
    for (u32 id = 32; id < interrupts; id += 4) {
        write32(GICD_ITARGETSR + id, repeated);
    }
    write32(GICD_CTLR, 1);
    dsb();

    write32(GICC_PMR, 0xff);
    write32(GICC_BPR, 0x7);
    write32(GICC_CTLR, 0x1);

    write8(GICD_IPRIORITYR + 27, 0x80);
    const u32 cfg = GICD_ICFGR + (27 / 16) * 4;
    const u32 shift = (27 % 16) * 2;
    write32(cfg, read32(cfg) & ~(3U << shift));

    const u32 enable = (1U << 27) | 1U; // virtual timer PPI and SGI 0
    write32(GICD_ISENABLER, read32(GICD_ISENABLER) | enable);
    write32(GICD_CTLR, 1);
    dsb();
    isb();

    uart_puts("gicc ctlr=");
    uart_puthex(read32(GICC_CTLR));
    uart_puts(" pmr=");
    uart_puthex(read32(GICC_PMR));
    uart_puts(" iidr=");
    uart_puthex(read32(GICC_IIDR));
    uart_puts(" rpr=");
    uart_puthex(read32(GICC_RPR));
    uart_putc('\n');

    return targets;
}

void irq_handler(void)
{
    const u32 iar = read32(GICC_IAR);
    const u32 id = iar & 0x3ffU;

    if (id < 1020U) {
        if (id == 27) {
            seen_timer = 1;
            timer_stop();
            uart_putc('T');
        } else if (id == 0) {
            seen_ipi = 1;
            uart_putc('I');
        } else {
            uart_puts(" unexpected=");
            uart_puthex(id);
        }
        write32(GICC_EOIR, iar);
    }

    if (seen_timer && seen_ipi && !passed) {
        passed = 1;
        uart_puts("\nPASS gicv2 timer+self-ipi\n");
    }
}

void main(void)
{
    uart_init();
    uart_puts("\nBOOT gicv2-test\n");

    u64 current_el;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(current_el));
    uart_puts("EL=");
    uart_puthex((u32)current_el);
    uart_putc('\n');

    __asm__ volatile("msr vbar_el1, %0; isb" :: "r"((u64)(void *)vectors) : "memory");

    const u32 targets = gicv2_init();
    timer_start();

    __asm__ volatile("dsb ishst" ::: "memory");
    write32(GICD_SGIR, (targets << 16) | 0U);
    __asm__ volatile("dsb sy" ::: "memory");

    __asm__ volatile("msr daifclr, #2; isb" ::: "memory");
    for (;;) {
        __asm__ volatile("wfi");
    }
}
