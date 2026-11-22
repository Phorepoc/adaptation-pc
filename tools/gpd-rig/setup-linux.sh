#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# setup-linux.sh <lab host:port> -- the GPD Win 2 rig's one-time setup, run on the GPD's own Linux (Ubuntu Server 24.04;
# Debian or Arch work too). The rig boots Phorepoc FROM THE DISK through the installed Linux:
#
#   curl -fsSLO https://raw.githubusercontent.com/Phorepoc/adaptation-pc/main/tools/gpd-rig/setup-linux.sh
#   sudo bash setup-linux.sh 192.168.1.79:8737
#
#   1. a 64 MB raw "phorepoc-log" partition in the SSD's free space (sgdisk; type 5048524c-4f47-4c4f-8750-484f5245504c),
#      where the kernel (DISKLOG=1) leaves every boot's log -- also after a forced power-off; existing partitions untouched
#   2. /etc/grub.d/45_phorepoc (the "Phorepoc x86_64 (disk)" entry: multiboot2 from the ESP), GRUB_DEFAULT=saved so
#      grub-reboot works; Linux stays the default entry
#   3. phorepoc-collect.service (the last Phorepoc boot's log -> /var/log/phorepoc/boot-<n>.log + a report to the lab) and
#      phorepoc-stage.service (a new lab image -> the ESP -> grub-reboot -> reboot)
#   NETBOOT=1 also installs the iPXE entry (build_ipxe.sh + install-rig.sh).
# Re-running is safe. Undo: sudo bash setup-linux.sh --undo (the partition stays; delete it with sgdisk if wanted).
set -eu
TYPE=5048524C-4F47-4C4F-8750-484F5245504C
REF="${REF:-main}"
W=/var/tmp/phorepoc-rig
[ "$(id -u)" = 0 ] || { echo "run as root: sudo bash $0 $*"; exit 1; }
if [ "${1:-}" = "--undo" ]; then
  systemctl disable --now phorepoc-stage.service phorepoc-collect.service 2>/dev/null || true
  rm -f /etc/systemd/system/phorepoc-*.service /usr/local/sbin/phorepoc-stage /usr/local/sbin/phorepoc-collect /etc/grub.d/45_phorepoc
  update-grub 2>/dev/null || grub-mkconfig -o /boot/grub/grub.cfg
  echo "removed (the phorepoc-log partition and /var/log/phorepoc stay)"; exit 0
fi
LAB="${1:?usage: sudo bash $0 <lab host:port> | --undo}"
if command -v apt-get >/dev/null; then
  pkgs="git gdisk curl python3 efibootmgr ca-certificates"
  [ "${NETBOOT:-0}" = 1 ] && pkgs="$pkgs build-essential liblzma-dev perl"
  DEBIAN_FRONTEND=noninteractive apt-get update -q
  DEBIAN_FRONTEND=noninteractive apt-get install -y -q $pkgs
elif command -v pacman >/dev/null; then
  pkgs="git gptfdisk curl python efibootmgr"
  [ "${NETBOOT:-0}" = 1 ] && pkgs="$pkgs base-devel xz perl"
  pacman -S --needed --noconfirm $pkgs
else
  echo "no apt-get or pacman: install git, gdisk, curl, python3, efibootmgr, then re-run"; exit 1
fi
mkdir -p "$W"
if [ -d "$W/adaptation-pc/.git" ]; then
  git -C "$W/adaptation-pc" fetch -q origin "$REF" && git -C "$W/adaptation-pc" checkout -q FETCH_HEAD
else
  git clone -q --depth 1 -b "$REF" https://github.com/Phorepoc/adaptation-pc "$W/adaptation-pc"
fi
G="$W/adaptation-pc/tools/gpd-rig"

# 1. the log partition, on the ESP's disk, in its free space
ESP="$(findmnt -no TARGET /boot/efi)"
DISK="/dev/$(lsblk -no PKNAME "$(findmnt -no SOURCE "$ESP")")"
if lsblk -rno PARTTYPE "$DISK" | grep -qi "^${TYPE}$"; then
  echo "phorepoc-log partition: present"
else
  sgdisk -n "0:0:+64M" -t "0:$TYPE" -c "0:phorepoc-log" "$DISK"
  partprobe "$DISK" 2>/dev/null || partx -u "$DISK" 2>/dev/null || true
  echo "phorepoc-log partition: created on $DISK"
fi
sgdisk -p "$DISK" | tail -n +8

# 2. GRUB
install -m 0755 "$G/linux/45_phorepoc" /etc/grub.d/45_phorepoc
if grep -q '^GRUB_DEFAULT=' /etc/default/grub; then
  grep -q '^GRUB_DEFAULT=saved' /etc/default/grub || sed -i 's/^GRUB_DEFAULT=.*/GRUB_DEFAULT=saved/' /etc/default/grub
else
  echo 'GRUB_DEFAULT=saved' >> /etc/default/grub
fi
update-grub 2>/dev/null || grub-mkconfig -o /boot/grub/grub.cfg
grub-set-default 0

# 3. the services
install -m 0755 "$G/linux/phorepoc-stage" /usr/local/sbin/phorepoc-stage
install -m 0755 "$G/linux/phorepoc-collect" /usr/local/sbin/phorepoc-collect
install -m 0644 "$G/linux/phorepoc-stage.service" "$G/linux/phorepoc-collect.service" /etc/systemd/system/
[ -f /etc/default/phorepoc-rig ] || printf 'LAB=%s\nGUARD=0\n' "$LAB" > /etc/default/phorepoc-rig
sed -i "s/^LAB=.*/LAB=$LAB/" /etc/default/phorepoc-rig
systemctl daemon-reload
systemctl enable phorepoc-collect.service phorepoc-stage.service

if [ "${NETBOOT:-0}" = 1 ]; then
  NO_GRUB=1 LAB="$LAB" WORK="$W/build" bash "$G/build_ipxe.sh" "$W/out"
  cp "$G/install-rig.sh" "$W/out/"
  bash "$W/out/install-rig.sh" "$LAB"
fi
echo "Done. Each Linux boot: the last Phorepoc log -> /var/log/phorepoc, then a new image at http://$LAB/gpd/phorepoc.elf is booted once."
