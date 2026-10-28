# adaptation-pc

This repository makes Phorepoc run on PCs: QEMU, VirtualBox, VMware and real hardware. It sits on top of
[adaptation-x86-64](https://github.com/Phorepoc/adaptation-x86-64), which does the kernel port, and adds everything
specific to a PC. That's the settings the system starts with, the list of what goes into the ROM, how the drives are
set up at boot, and a few PC-only components and tests.

## Try it

Grab the `.ova` file from the [releases](https://github.com/Phorepoc/adaptation-pc/releases) and import it into
VirtualBox with **File > Import Appliance**. There's also a bootable `.iso` if you'd rather use QEMU or real hardware.
The first boot takes about a minute, and later ones are faster.

## Building

The [pdk](https://github.com/Phorepoc/pdk) README walks through a full PC build. With this repository and
adaptation-x86-64 checked out next to core and the pdk, `-b pc` picks them up by itself.

The release images are built by GitHub Actions at the commits listed in `board.lock`. Pushing a `v*` tag attaches them
to a draft release.

## What's in here

```
os/board/                             the board's settings: central repository, start-up, drives, installer
os/tools/phorepoc-x86/*.py            generators for the network settings and the software version
os/src/phesvgatest/                   a test for the VMware/VirtualBox graphics driver
os/third_party/symbian2010-camerasc/  the 2010 camera driver, built into the kernel
registry/components_pc.py             the components this repository adds
```

At build time `os/` is laid over core's `os/`, so the paths line up with core's.

## License

Code we wrote is Apache-2.0. Symbian files we've changed are EPL-2.0, with GPL-2.0-or-later as a Secondary License, and
Symbian files we use unchanged stay EPL-1.0. The license texts are in `LICENSES/`.

Contributions are welcome under the same terms. Add your name to `AUTHORS` in your first pull request.
