# adaptation-pc

This repository makes Phorepoc OS run on PCs: QEMU, VirtualBox, VMware and real hardware. It sits on top of
[adaptation-x86-64](https://github.com/Phorepoc/adaptation-x86-64), which does the kernel port, and adds everything
specific to a PC. That's the settings the system starts with, the list of what goes into the ROM, how the drives are
set up at boot, and a few PC-only components and tests.

## Try it

Download the files from the [releases](https://github.com/Phorepoc/adaptation-pc/releases). The first boot takes about a
minute, and later ones are faster.

**VirtualBox.** Import the `.ova` with **File > Import Appliance**.

**VMware Workstation Pro** (free for everyone since November 2024) or Workstation Player:

1. Download `phorepoc-pc.iso` and `phorepoc-pc-vmware.zip` into one folder and unzip the zip there. It holds `Phorepoc.vmx`
   and `phorepoc-disk.vmdk`, the drive C: that keeps your data between runs.
2. In Workstation, **File > Open** `Phorepoc.vmx`, then **Power On**. Nothing needs changing: the machine has two CPUs, 1 GB of
   RAM, an IDE disk and DVD drive, an Intel e1000 network adapter (NAT), Intel HD Audio, the VMware SVGA adapter with 3D
   and an xHCI USB controller. The kernel's log goes to `serial.log` next to the `.vmx`.
3. Workstation may show a notice about side-channel mitigations when Windows' hypervisor is on; it is only a notice.
   Click OK.

The `.ova` is for VirtualBox: VMware's importer rejects it (the appliance describes VirtualBox's AC'97 sound card, which
VMware does not have). Use the `.vmx` zip instead. The pointer is absolute (VMware's mouse protocol), and the screen is a
360x640 phone.

A `.vmx` of your own needs the PCIe root ports (`pciBridge4` to `pciBridge7`) as in `Phorepoc.vmx`; with `usb_xhci` and
without them VMware refuses to power on with "No PCIe slot available for usb_xhci".

The bootable `.iso` also runs on QEMU and on real hardware.

## Building

The [pdk](https://github.com/Phorepoc/pdk) README walks through a full PC build. With this repository and
adaptation-x86-64 checked out next to core and the pdk, `-b pc` picks them up by itself.

Every push to `main` builds the images on GitHub Actions, from the commits `board.lock` pins, and boots them in CI to
the home screen. When the commits ask for a release ([Conventional Commits](https://www.conventionalcommits.org):
`feat`, `fix` or a breaking change), the images are released under the next version.

## What's in here

```
os/board/                             the board's settings: central repository, start-up, drives, installer
recipes/                              the image recipes: kernel ISO, VirtualBox appliance (tools/make_ova.py), VMware machine (tools/make_vmware.py)
os/tools/phorepoc-x86/*.py            generators for the network settings and the software version
os/src/phesvgatest/                   a test for the VMware/VirtualBox graphics driver
os/src/phescdv/                       the screen driver's factory: draw devices that rotate in software and keep the display's row pitch
os/src/phelandtest/                   a test: the landscape screen mode and a pattern that shows a wrong pitch or rotation
os/third_party/symbian2010-camerasc/  the 2010 camera driver, built into the kernel
registry/components_pc.py             the components this repository adds
```

At build time `os/` is laid over core's `os/`, so the paths line up with core's.

## License

Code we wrote is Apache-2.0. Symbian files we've changed are EPL-2.0, with GPL-2.0-or-later as a Secondary License, and
Symbian files we use unchanged stay EPL-1.0. The license texts are in `LICENSES/`.

Contributions are welcome under the same terms. Add your name to `AUTHORS` in your first pull request.
