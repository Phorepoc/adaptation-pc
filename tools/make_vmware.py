#!/usr/bin/env python3
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

"""make_vmware -- package the x86-64 image as a VMware Workstation machine (a zip). Runs on Linux in the toolchain
container (phe image recipe vmware -b pc); needs neither VMware nor root: qemu-img makes the disk, this file writes the .vmx.

The zip holds two files, which go next to the released phorepoc-pc.iso (the machine's DVD drive reads it by that name):

    Phorepoc.vmx         the machine: 2 CPUs, 1 GB, IDE, Intel e1000 (NAT), Intel HD Audio, the VMware SVGA adapter
                         with 3D, an xHCI USB controller on a PCIe root port, COM1 -> serial.log
    phorepoc-disk.vmdk   the C: disk: FAT32 with the Data folder a phone's C: has (a single growable file)

An OVF cannot say "Intel HD Audio" the way VMware reads it (the VirtualBox appliance carries AC'97, which VMware does
not have), so VMware gets its own machine file instead of the appliance.

    make_vmware.py [-n name] [-m MB RAM] [-c cpus] [-d MB of C:] [-i iso name] [-o file.zip]
"""
import argparse
import os
import shutil
import subprocess
import sys
import zipfile

PDK = os.environ.get("PHOREPOC_PDK") or sys.exit("make_vmware.py: run it with phe image recipe vmware -b <board>")
sys.path.insert(0, os.path.join(PDK, "tools", "emitter"))
from paths import BUILD, OS_ROOT  # noqa: E402

VMX = """.encoding = "UTF-8"
# VMware Workstation machine for the Phorepoc PC image. Open it with File > Open; no settings need changing.
# It reads {iso} (the release's kernel + ROM image) from this folder and keeps the C: drive in {disk}.
# serial.log is the kernel's log (COM1).
config.version = "8"
virtualHW.version = "19"
displayName = "{name}"
guestOS = "other-64"
firmware = "bios"
memsize = "{mem}"
numvcpus = "{cpus}"

ide0:0.present = "TRUE"
ide0:0.fileName = "{disk}"
ide1:0.present = "TRUE"
ide1:0.deviceType = "cdrom-image"
ide1:0.fileName = "{iso}"
bios.bootOrder = "cdrom"

ethernet0.present = "TRUE"
ethernet0.virtualDev = "e1000"
ethernet0.connectionType = "nat"
ethernet0.addressType = "generated"

sound.present = "TRUE"
sound.virtualDev = "hdaudio"
sound.autoDetect = "TRUE"

svga.present = "TRUE"
svga.vramSize = "134217728"
mks.enable3d = "TRUE"
svga.graphicsMemoryKB = "1048576"

serial0.present = "TRUE"
serial0.fileType = "file"
serial0.fileName = "serial.log"
serial0.yieldOnMsrRead = "TRUE"

# The PCI topology VMware's own machines declare (hardware version 19): a host bridge and four PCIe root ports. The
# xHCI controller sits on a root port; without these lines VMware refuses to power on ("No PCIe slot available for
# usb_xhci"). The e1000, the sound and the SVGA devices stay on bus 0.
pciBridge0.present = "TRUE"
pciBridge4.present = "TRUE"
pciBridge4.virtualDev = "pcieRootPort"
pciBridge4.functions = "8"
pciBridge5.present = "TRUE"
pciBridge5.virtualDev = "pcieRootPort"
pciBridge5.functions = "8"
pciBridge6.present = "TRUE"
pciBridge6.virtualDev = "pcieRootPort"
pciBridge6.functions = "8"
pciBridge7.present = "TRUE"
pciBridge7.virtualDev = "pcieRootPort"
pciBridge7.functions = "8"

usb.present = "TRUE"
usb_xhci.present = "TRUE"
usb_xhci.pciSlotNumber = "224"
floppy0.present = "FALSE"
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("-n", "--name", default="Phorepoc")
    ap.add_argument("-m", "--mem", type=int, default=1024, help="MB of RAM (the ROM lives in RAM: 1024)")
    ap.add_argument("-c", "--cpus", type=int, default=2)
    ap.add_argument("-d", "--disk", type=int, default=1024, help="MB of the C: disk (1 GiB, as the PinePhone's C:)")
    ap.add_argument("-i", "--iso", default="phorepoc-pc.iso", help="the ISO's file name next to the machine")
    ap.add_argument("-o", "--out", default=None, help="default: <build>/x86/<name>-vmware.zip")
    a = ap.parse_args()

    x86 = os.path.join(BUILD, "x86")
    if not shutil.which("qemu-img"):
        sys.exit("qemu-img not found (the toolchain image needs qemu-utils)")
    mkdisk = os.path.join(OS_ROOT, "tools", "phorepoc-x86", "make_disk.sh")
    if not os.path.isfile(mkdisk):
        sys.exit(f"no {mkdisk} (board pc: adaptation-x86-64)")

    work = os.path.join(x86, "vmware")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    raw = os.path.join(work, "c.img")
    env = dict(os.environ, KOUT=x86, IMG=raw, LABEL="PHOREPOC")
    subprocess.run(["bash", mkdisk, str(a.disk)], env=env, check=True, stdout=subprocess.DEVNULL)
    disk_file = "phorepoc-disk.vmdk"
    vmdk = os.path.join(work, disk_file)
    subprocess.run(["qemu-img", "convert", "-f", "raw", "-O", "vmdk", "-o", "subformat=monolithicSparse", raw, vmdk],
                   check=True)
    os.unlink(raw)

    vmx_file = a.name + ".vmx"
    vmx = os.path.join(work, vmx_file)
    with open(vmx, "w", newline="\n") as f:
        f.write(VMX.format(name=a.name, mem=a.mem, cpus=a.cpus, disk=disk_file, iso=a.iso))

    out = a.out or os.path.join(x86, a.name + "-vmware.zip")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        z.write(vmx, vmx_file)
        z.write(vmdk, disk_file)
    print(f"vmware ok: {os.path.getsize(out)} {out}")


if __name__ == "__main__":
    main()
