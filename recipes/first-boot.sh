#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later

# recipes/first-boot.sh -- regression test ("phe image recipe first-boot -b pc", in the container): the home screen on a freshly formatted C:, first and second boot.
#   after "phe image rom -b pc" and "phe image recipe kernel -b pc"
# Formats a NEW blank disk (never a reused one: the first boots of a disk install the home-screen configuration and
# create C:'s system data), boots the image twice from copies of it, and fails unless both boots show the home screen
# with its widgets (first_boot.py); then the same under UEFI with C: on AHCI (below). Out: $OUT = $PHOREPOC_BUILD/x86/regress/first-boot: bootN .log .ppm .png.
set -eu
HERE="$(cd "$(dirname "$0")/.." && pwd)"; cd "${PHOREPOC_PDK:?run it with phe image recipe first-boot -b pc}"
B="${PHOREPOC_BUILD:-/build}"
OUT="${OUT:-$B/x86/regress/first-boot}"
ISO="${ISO:-$B/x86/ekern/phorepoc-ekern.iso}"
[ -f "$ISO" ] || { echo "no $ISO -- build the ROM and the kernel first" >&2; exit 2; }
rm -rf "$OUT"; mkdir -p "$OUT"
OSV=$(python3 tools/emitter/paths.py)
IMG="$OUT/blank.img" KOUT="$OUT" FRESH=1 bash "$OSV/tools/phorepoc-x86/make_disk.sh" "${DISK_MB:-256}" > "$OUT/make_disk.log"
python3 "$HERE/tests/first_boot.py" --iso "$ISO" --disk "$OUT/blank.img" --out "$OUT" \
     --smp "${SMP:-2}" --mem "${MEM:-1024}" --settle "${SETTLE:-25}" --boots "${BOOTS:-2}" --disk-bus "${DISK_BUS:-ide}" --firmware "${FIRMWARE:-bios}"
# then the real-PC path once: UEFI firmware (OVMF, q35) and C: on an AHCI controller, from its own fresh disk. Skipped when the
# toolchain image has no OVMF (UEFI_GATE=0 = off).
if [ "${UEFI_GATE:-1}" = 1 ] && [ -f /usr/share/OVMF/OVMF_CODE_4M.fd ]; then
  U="$OUT/uefi-ahci"; mkdir -p "$U"
  IMG="$U/blank.img" KOUT="$U" FRESH=1 bash "$OSV/tools/phorepoc-x86/make_disk.sh" "${DISK_MB:-256}" > "$U/make_disk.log"
  echo "== UEFI + AHCI"
  python3 "$HERE/tests/first_boot.py" --iso "$ISO" --disk "$U/blank.img" --out "$U" --smp "${SMP:-2}" --mem "${MEM:-1024}" \
       --settle "${SETTLE:-25}" --boots "${UEFI_BOOTS:-2}" --disk-bus ahci --firmware uefi
else
  echo "== UEFI + AHCI: skipped (no OVMF in the toolchain image)"
fi
# and a live boot with no disk at all: C: is the kernel's RAM drive (S60's CheckUserDrive refuses a RAM user drive; the
# kernel reports it as a fixed-size disk). One boot: nothing survives it. NODISK_GATE=0 = off.
if [ "${NODISK_GATE:-1}" = 1 ]; then
  N="$OUT/no-disk"; mkdir -p "$N"
  echo "== no disk (C: on the RAM drive)"
  python3 "$HERE/tests/first_boot.py" --iso "$ISO" --disk "$N/none.img" --out "$N" --smp "${SMP:-2}" --mem "${MEM:-1024}" \
       --settle "${SETTLE:-25}" --boots 1 --disk-bus none --firmware bios
fi
