#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

# recipes/first-boot.sh -- regression test ("ph image recipe first-boot -b pc", in the container): the home screen on a freshly formatted C:, first and second boot.
#   after "ph image rom -b pc" and "ph image recipe kernel -b pc"
# Formats a NEW blank disk (never a reused one: the first boots of a disk install the home-screen configuration and
# create C:'s system data), boots the image twice from copies of it, and fails unless both boots show the home screen
# with its widgets (first_boot.py). Out: $OUT = $PHOREPOC_BUILD/x86/regress/first-boot: bootN .log .ppm .png.
set -eu
HERE="$(cd "$(dirname "$0")/.." && pwd)"; cd "${PHOREPOC_PDK:?run it with ph image recipe first-boot -b pc}"
B="${PHOREPOC_BUILD:-/build}"
OUT="${OUT:-$B/x86/regress/first-boot}"
ISO="${ISO:-$B/x86/ekern/phorepoc-ekern.iso}"
[ -f "$ISO" ] || { echo "no $ISO -- build the ROM and the kernel first" >&2; exit 2; }
rm -rf "$OUT"; mkdir -p "$OUT"
OSV=$(python3 tools/emitter/paths.py)
IMG="$OUT/blank.img" KOUT="$OUT" FRESH=1 bash "$OSV/tools/phorepoc-x86/make_disk.sh" "${DISK_MB:-256}" > "$OUT/make_disk.log"
exec python3 "$HERE/tests/first_boot.py" --iso "$ISO" --disk "$OUT/blank.img" --out "$OUT" \
     --smp "${SMP:-2}" --mem "${MEM:-1024}" --settle "${SETTLE:-25}" --boots "${BOOTS:-2}"
