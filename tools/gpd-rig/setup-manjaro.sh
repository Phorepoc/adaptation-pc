#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# setup-manjaro.sh <lab host:port> -- the GPD Win 2 rig's one-time setup, run on the GPD's own Manjaro (Wi-Fi is enough):
#
#   curl -fsSLO https://raw.githubusercontent.com/Phorepoc/adaptation-pc/main/tools/gpd-rig/setup-manjaro.sh
#   sudo bash setup-manjaro.sh 192.168.1.79:8737
#
# Installs the build tools, builds ipxe.efi from source (tools/gpd-rig/build_ipxe.sh, iPXE pinned) and runs
# install-rig.sh: ipxe.efi on the ESP, its boot entry first, Manjaro's after it (the fallback: no lab = Manjaro in ~30 s).
# Undo: sudo bash /var/tmp/phorepoc-rig/adaptation-pc/tools/gpd-rig/install-rig.sh --undo
set -eu
LAB="${1:?usage: sudo bash $0 <lab host:port>}"
REF="${REF:-main}"
W=/var/tmp/phorepoc-rig
[ "$(id -u)" = 0 ] || { echo "run as root: sudo bash $0 $*"; exit 1; }
pacman -S --needed --noconfirm base-devel git xz perl efibootmgr
mkdir -p "$W"
if [ -d "$W/adaptation-pc/.git" ]; then
  git -C "$W/adaptation-pc" fetch -q origin "$REF" && git -C "$W/adaptation-pc" checkout -q FETCH_HEAD
else
  git clone -q --depth 1 -b "$REF" https://github.com/Phorepoc/adaptation-pc "$W/adaptation-pc"
fi
G="$W/adaptation-pc/tools/gpd-rig"
NO_GRUB=1 LAB="$LAB" WORK="$W/build" bash "$G/build_ipxe.sh" "$W/out"
cp "$G/install-rig.sh" "$W/out/"
bash "$W/out/install-rig.sh" "$LAB"
