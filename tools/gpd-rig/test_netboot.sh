#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# test_netboot.sh <phorepoc.elf> -- the rig's boot chain in QEMU: OVMF boots ipxe.efi from an ESP disk, iPXE
# brings up a USB NIC on an xHCI controller (QEMU's usb-net, CDC ECM -- QEMU has no RTL8153; the real adapter
# offers ECM too), fetches boot.ipxe + grubx64.efi + phorepoc.elf from a local HTTP server (the lab's role) and
# the kernel must reach MARK (the kernel heartbeat). FALLBACK=1: the no-lab path instead. The kernel keeps the e1000-less machine: its log is the serial.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
ELF="$1"; MARK="${MARK:-[ekern] heartbeat}"; SECS="${SECS:-120}"
T="${OUT:-/tmp/rig-test}"; rm -rf "$T"; mkdir -p "$T/srv/gpd" "$T/esp/EFI/BOOT"
LAB=192.0.2.1:9 GRUB_LAB=10.0.2.2:8737 WORK="${WORK:-/tmp/ipxe-build}" bash "$HERE/build_ipxe.sh" "$T/files" > "$T/build.log" 2>&1
cp "$T/files/boot.ipxe" "$T/files/grubx64.efi" "$T/srv/gpd/"; cp "$ELF" "$T/srv/gpd/phorepoc.elf"
cp "$T/files/ipxe.efi" "$T/esp/EFI/BOOT/BOOTX64.EFI"
# the built-in lab address is a dead one: the run passes only if lab.ipxe on the ESP (install-rig.sh's) takes over
mkdir -p "$T/esp/EFI/phorepoc-rig"
{ echo '#!ipxe'; echo 'set lab 10.0.2.2:8737'; } > "$T/esp/EFI/phorepoc-rig/lab.ipxe"
# FALLBACK=1: no lab at all -- iPXE must give up and hand back to the firmware (the Manjaro entry on the GPD)
[ "${FALLBACK:-0}" = 1 ] && { rm "$T/esp/EFI/phorepoc-rig/lab.ipxe"; MARK="rig: giving up"; }
( cd "$T/srv" && exec python3 -m http.server 8737 --bind 0.0.0.0 ) > "$T/http.log" 2>&1 & HP=$!
trap 'kill $HP 2>/dev/null || true' EXIT
cp /usr/share/OVMF/OVMF_VARS_4M.fd "$T/vars.fd"
timeout "$SECS" qemu-system-x86_64 -machine q35 -m 1024 -smp 2 -display none -serial "file:$T/serial.log" \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd -drive if=pflash,format=raw,file="$T/vars.fd" \
  -drive file=fat:rw:"$T/esp",format=raw,if=none,id=esp -device ahci,id=ahci -device ide-hd,drive=esp,bus=ahci.0 \
  -device qemu-xhci,id=xhci -netdev user,id=n0 -device usb-net,netdev=n0,bus=xhci.0 -nic none \
  -debugcon "file:$T/debugcon.log" -global isa-debugcon.iobase=0x402 &
QP=$!
for i in $(seq 1 "$SECS"); do
  grep -qF -- "$MARK" "$T/serial.log" 2>/dev/null && { echo "PASS: $MARK after ${i}s"; kill $QP; grep -c GET "$T/http.log" | sed 's/^/http GETs: /'; exit 0; }
  kill -0 $QP 2>/dev/null || break; sleep 1
done
kill $QP 2>/dev/null || true; echo "FAIL (no '$MARK')"; tail -5 "$T/http.log"; tail -c 600 "$T/serial.log"; exit 1
