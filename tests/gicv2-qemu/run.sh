#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause-Patent
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cc=${CC:-clang}
ld=${LD:-ld.lld}
qemu=${QEMU:-qemu-system-aarch64}
timeout_command=${TIMEOUT:-timeout}

for tool in "$cc" "$ld" "$qemu" "$timeout_command"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "missing: $tool" >&2
        exit 2
    fi
done

tmp=$(mktemp -d "${TMPDIR:-/tmp}/armcpu-gicv2.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

"$cc" --target=aarch64-none-elf -ffreestanding -fno-stack-protector -fno-pic -O2 \
    -c "$here/start.S" -o "$tmp/start.o"
"$cc" --target=aarch64-none-elf -ffreestanding -fno-stack-protector -fno-pic -O2 \
    -c "$here/gicv2_test.c" -o "$tmp/gicv2_test.o"
"$ld" -T "$here/link.ld" -o "$tmp/gicv2-test.elf" "$tmp/start.o" "$tmp/gicv2_test.o"

output=$("$timeout_command" 8 "$qemu" \
    -M virt,gic-version=2,virtualization=off,secure=off \
    -cpu cortex-a57 -m 256M -nographic -monitor none -serial stdio \
    -kernel "$tmp/gicv2-test.elf" -no-reboot 2>&1 || true)

printf '%s\n' "$output"
printf '%s\n' "$output" | grep -q "PASS gicv2 timer+self-ipi"
