#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

# recipes/release.sh -- the PC release files into $OUT ("ph image release -b pc", run on the host after every component
# is built): the bootable ISO and the VirtualBox appliance.
set -eu
: "${OUT:?run it with ph image release -b pc}" "${PHOREPOC_BUILD:?}"
ph image zdata -b pc
ph image skin -b pc                   # the default skin package (the UI needs one)
ph image rom -b pc -O prune=1         # (drops what needs layers this board does not ship)
ph image recipe kernel -b pc
ph image recipe vbox -b pc
mkdir -p "$OUT"
cp "$PHOREPOC_BUILD/x86/ekern/phorepoc-ekern.iso" "$OUT/phorepoc-pc.iso"
cp "$PHOREPOC_BUILD/x86/Phorepoc.ova" "$OUT/phorepoc-pc.ova"
