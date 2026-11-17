#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later

# recipes/release.sh -- the PC release files into $OUT ("phe image release -b pc", run on the host after every component
# is built): the bootable ISO, the VirtualBox appliance and the VMware machine (a zip with the .vmx and the C: disk).
set -eu
: "${OUT:?run it with phe image release -b pc}" "${PHOREPOC_BUILD:?}"
phe image zdata -b pc
# PHOREPOC_NATIVE=1: the image shows the display at its native size (720x1440, no pixel doubling): the board's screen modes come from
# os/board/wsini-native.txt (the merged OS tree links os/board/wsini.txt to it), and the kernel recipe (adaptation-x32) sets the
# framebuffer. Not set (the default): the image is unchanged.
if [ "${PHOREPOC_NATIVE:-0}" = 1 ]; then
  src="$(cd "$(dirname "$0")/.." && pwd)/os/board/wsini-native.txt"
  view="$(python3 "${PHOREPOC_PDK:?}/tools/emitter/paths.py")/board"
  [ -f "$src" ] && [ -d "$view" ] || { echo "PHOREPOC_NATIVE=1: no $src or no merged board folder $view" >&2; exit 1; }
  ln -sfn "$src" "$view/wsini.txt"
  echo "native resolution: $view/wsini.txt -> $src"
fi
phe image skin -b pc                   # the default skin package (the UI needs one)
# PHOREPOC_NGA=1: the NGA window server (composition through OpenWF-C, on the CPU: Mesa's software renderer, no GPU) instead of the classic one,
# the first step of the window compositor (core phecomp). Not set (the default): the image is unchanged.
# PHOREPOC_PHECOMP=1|2 (with NGA) adds core's phecomp render stage (1 = trace, 2 = compose).
ROM_OPTS=(-O prune=1)       # (prune=1 drops what needs layers this board does not ship)
if [ "${PHOREPOC_NGA:-0}" = 1 ]; then
  ROM_OPTS+=(-O nga=1)
  [ "${PHOREPOC_PHECOMP:-0}" = 0 ] || ROM_OPTS+=(-O "phecomp=${PHOREPOC_PHECOMP}")
fi
phe image rom -b pc "${ROM_OPTS[@]}"
phe image recipe kernel -b pc
phe image recipe vbox -b pc
phe image recipe vmware -b pc
mkdir -p "$OUT"
cp "$PHOREPOC_BUILD/x86/ekern/phorepoc-ekern.iso" "$OUT/phorepoc-pc.iso"
cp "$PHOREPOC_BUILD/x86/Phorepoc.ova" "$OUT/phorepoc-pc.ova"
cp "$PHOREPOC_BUILD/x86/Phorepoc-vmware.zip" "$OUT/phorepoc-pc-vmware.zip"
