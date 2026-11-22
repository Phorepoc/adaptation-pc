#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# build_ipxe.sh <out-dir> -- the GPD Win 2 rig's boot files (run in the base build image):
#   ipxe.efi      iPXE (pinned IPXE_REF) with rig.ipxe embedded; goes on the GPD's ESP once (install-rig.sh)
#   grubx64.efi   a standalone GRUB with grub-net.cfg embedded; the lab serves it at /gpd/grubx64.efi
#   boot.ipxe     the lab serves it at /gpd/boot.ipxe
# LAB=host[:port] = the lab's HTTP server (default 192.168.1.79:8737; ipxe.efi prefers lab.ipxe on the ESP, which
# install-rig.sh writes); GRUB_LAB = the one baked into grubx64.efi (default LAB: the lab serves a GRUB with its own address). iPXE is GPL-2.0: the binary comes with
# its source at IPXE_REF (https://github.com/ipxe/ipxe), the same as every other GPL binary we hand out.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
O="$(mkdir -p "$1" && cd "$1" && pwd)"
LAB="${LAB:-192.168.1.79:8737}"
IPXE_REF="${IPXE_REF:-v2.0.0}"
W="${WORK:-/tmp/ipxe-build}"
[ -d "$W/ipxe/.git" ] || git clone -q https://github.com/ipxe/ipxe "$W/ipxe"
git -C "$W/ipxe" fetch -q --tags origin && git -C "$W/ipxe" checkout -q "$IPXE_REF"
sed "s/@LAB@/$LAB/" "$HERE/rig.ipxe" > "$O/rig.ipxe"
# the USB NICs the rig may use (ecm + ncm are in iPXE's default USB set; the RTL8153 offers an ECM configuration)
cat > "$W/ipxe/src/config/local/general.h" <<'H'
#define DOWNLOAD_PROTO_HTTP
#define NET_PROTO_IPV4
#define IMAGE_EFI
#define IMAGE_SCRIPT
#define REBOOT_CMD
#define POWEROFF_CMD
#define PING_CMD
#define NSLOOKUP_CMD
H
make -s -C "$W/ipxe/src" -j"$(nproc)" bin-x86_64-efi/ipxe.efi EMBED="$O/rig.ipxe" NO_WERROR=1
cp "$W/ipxe/src/bin-x86_64-efi/ipxe.efi" "$O/ipxe.efi"
git -C "$W/ipxe" rev-parse HEAD > "$O/ipxe.src-commit"
[ "${NO_GRUB:-0}" = 1 ] && { ls -l "$O"; exit 0; }		# NO_GRUB=1: ipxe.efi only (setup-linux.sh)
sed "s/@LAB@/${GRUB_LAB:-$LAB}/" "$HERE/grub-net.cfg" > "$O/grub-net.cfg"
grub-mkstandalone -O x86_64-efi -o "$O/grubx64.efi" --modules="efinet http net multiboot2 efi_gop efi_uga normal echo" \
  "boot/grub/grub.cfg=$O/grub-net.cfg"
cp "$HERE/boot.ipxe" "$O/boot.ipxe"
ls -l "$O"
