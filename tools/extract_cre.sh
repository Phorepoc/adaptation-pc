#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later

# tools/extract_cre.sh <c-drive.img> <out.cre> -- take CommsDat's database (cccccc00.cre) off the C: disk of a boot-test run
# (the artifact boot-test-pc/c-drive.img of a variant built with PHOREPOC_BOOT=bootui: CED made it at the first boot).
# The image is an MBR disk with one FAT partition; needs mtools (mdir, mcopy). Prints the path it found.
set -eu
img="${1:?usage: extract_cre.sh c-drive.img out.cre}"; out="${2:?usage: extract_cre.sh c-drive.img out.cre}"
off=$(( $(od -An -t u4 -j 454 -N 4 "$img") * 512 ))      # first partition's start LBA (MBR entry 1, offset 446+8)
src=$(mdir -/ -b -i "$img@@$off" :: | grep -i 'cccccc00\.cre$' | head -1)
[ -n "$src" ] || { echo "no cccccc00.cre on $img" >&2; exit 1; }
mcopy -n -i "$img@@$off" "$src" "$out"
echo "$src -> $out ($(wc -c < "$out") bytes)"
