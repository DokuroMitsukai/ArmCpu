# GICv2 QEMU test

Bare-metal AArch64 test for the register sequence used by `ArmCpu` on a GICv2:
distributor setup, banked CPU interface, virtual-timer PPI 27 and a self-SGI
through `GICD_SGIR`.

The test deliberately leaves `GICD_IGROUPR` unchanged. Firmware owns the
secure/Group 0/Group 1 split; forcing Group 1 before enabling the CPU interface
prevents interrupt delivery on QEMU's single-security-state GIC-400 model, and
`Mu`'s GICv2 driver likewise does not reprogram it.

Run on a Linux host with clang's AArch64 target, `ld.lld` and QEMU:

```sh
tests/gicv2-qemu/run.sh
```

The script builds a temporary ELF, boots it with
`-M virt,gic-version=2,virtualization=off,secure=off`, and succeeds only when
the virtual timer and self-SGI have both reached `irq_handler`.
