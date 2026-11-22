#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# install-rig.sh -- run ONCE on the GPD Win 2, from its installed Linux (Ubuntu Server 24.04 next to the ESP; the unallocated rest of the SSD stays free for C:), as root, in the folder that holds ipxe.efi (build_ipxe.sh's):
#
#   sudo bash install-rig.sh 192.168.1.79:8737      the lab's address (host:port of PhoneLab's HTTP side)
#   sudo bash install-rig.sh --undo                 remove the netboot entry, the installed Linux first again
#
# It copies ipxe.efi to the internal SSD's EFI system partition (\EFI\phorepoc-rig\), writes the lab's address next to
# it (lab.ipxe; edit it there if the lab moves), adds a UEFI boot entry "Phorepoc rig (iPXE)" and puts it FIRST, with
# every existing entry (the installed Linux's) after it. iPXE tries the lab 5 times (~30 s); without an answer it exits and the firmware
# boots the next entry, the installed Linux -- the fallback until the netboot works. Nothing else on the disk changes.
set -eu
DIR="$(cd "$(dirname "$0")" && pwd)"
LABEL="Phorepoc rig (iPXE)"
[ "$(id -u)" = 0 ] || { echo "run as root: sudo bash $0 $*"; exit 1; }
command -v efibootmgr >/dev/null || { echo "install efibootmgr first"; exit 1; }
[ -d /sys/firmware/efi ] || { echo "not booted in UEFI mode"; exit 1; }

ESP="$(findmnt -no TARGET /boot/efi 2>/dev/null || true)"
[ -n "$ESP" ] || ESP="$(findmnt -no TARGET /efi 2>/dev/null || true)"
[ -n "$ESP" ] || ESP="$(findmnt -no TARGET /boot 2>/dev/null || true)"
[ -n "$ESP" ] && [ -d "$ESP/EFI" ] || { echo "no mounted EFI system partition (/boot/efi, /efi or /boot)"; exit 1; }
PART="$(findmnt -no SOURCE "$ESP")"
DISK="/dev/$(lsblk -no PKNAME "$PART")"
PARTN="$(cat "/sys/class/block/$(basename "$PART")/partition")"
echo "ESP: $PART (disk $DISK, partition $PARTN) at $ESP"

# our entries (efibootmgr >= 18 lists the device path after a tab)
ours() { efibootmgr | sed -n "s/^Boot\([0-9A-Fa-f]\{4\}\)\*\? $LABEL\([[:space:]].*\)\?\$/\1/p"; }
old="$(ours)"
for n in $old; do efibootmgr -q -b "$n" -B; done
if [ "${1:-}" = "--undo" ]; then
  rm -rf "$ESP/EFI/phorepoc-rig"
  echo "removed: $old"; efibootmgr; exit 0
fi

LAB="${1:?usage: $0 <lab host:port> | --undo}"
[ -f "$DIR/ipxe.efi" ] || { echo "no ipxe.efi next to this script"; exit 1; }
mkdir -p "$ESP/EFI/phorepoc-rig"
cp "$DIR/ipxe.efi" "$ESP/EFI/phorepoc-rig/ipxe.efi"
printf '#!ipxe\nset lab %s\n' "$LAB" > "$ESP/EFI/phorepoc-rig/lab.ipxe"
[ -f "$DIR/ipxe.src-commit" ] && cp "$DIR/ipxe.src-commit" "$ESP/EFI/phorepoc-rig/"

if command -v mokutil >/dev/null && mokutil --sb-state 2>/dev/null | grep -q enabled; then
  echo "WARNING: Secure Boot is on -- the firmware will refuse ipxe.efi (unsigned). Turn it off in the BIOS setup (DEL at power-on)."
fi

prev="$(efibootmgr | sed -n 's/^BootOrder: //p')"
efibootmgr -q --create --disk "$DISK" --part "$PARTN" --label "$LABEL" --loader '\EFI\phorepoc-rig\ipxe.efi'
new="$(ours | head -1)"; [ -n "$new" ] || { echo "the new entry is not listed:"; efibootmgr; exit 1; }
efibootmgr -q -o "$new${prev:+,$prev}"
echo "boot entry $new first, then: $prev"
efibootmgr -v | head -20
echo "Done. Next boot: iPXE on the USB Ethernet adapter -> http://$LAB/gpd/boot.ipxe; no lab = the installed Linux after ~30 s."
