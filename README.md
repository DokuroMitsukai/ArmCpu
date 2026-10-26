# ArmCpu

ARM generic interrupt controller and timer.

Starts the CPU, GIC and architectural timer on sheng, the QEMU virt machine, and boards with a GICv2 (GIC-400 or
Cortex-A15 GIC) interrupt controller. GICv3 uses the system-register CPU interface and per-core redistributors; GICv2
uses the distributor plus the banked CPU interface and currently brings up the boot CPU only.

`tests/gicv2-qemu` checks the GICv2 distributor, CPU-interface, virtual-timer PPI and self-SGI sequence against QEMU's
GIC-400 model without requiring XNU.

Build and interface details: [documentation](https://unixnu.com/reference/components). Component sources and dependencies: [catalog](https://unixnu.com/reference/components).
