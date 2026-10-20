# adaptation-pc

The PC device adaptation of Phorepoc, on [adaptation-x86-64](https://github.com/Phorepoc/adaptation-x86-64): QEMU,
VirtualBox and real PCs. The board files (ROM recipe lists, central repository settings, System State Manager
command lists, start-up drive table) and the PC-only components and tests.

## Layout

```
os/tools/phorepoc-x86/board/          the board: cenrep/, ssm/, swi/, rfs/, ups/, estart.txt ...
os/tools/phorepoc-x86/*.py            CommsDat and software-version generators
os/src/phesvgatest/                   VMware SVGA3D (VirtualBox VMSVGA) driver test
os/third_party/symbian2010-camerasc/  the 2010 shared-chunk camera driver (EPL), built into the kernel
registry/components_pc.py             this adaptation's components (merged into core's registry)
```

`os/` is laid over [core](https://github.com/Phorepoc/core)'s `os/` at build time, so paths match core's.

## Building

With the [pdk](https://github.com/Phorepoc/pdk):

```sh
export PHOREPOC_ADAPTATIONS=../adaptation-x86-64:../adaptation-pc
./ph build ...
```

## Licence

Eclipse Public License 1.0 (see `LICENSE`), copyright The Phorepoc Authors (`AUTHORS`, `NOTICE`).

Contributions are welcome under the same terms: add yourself to `AUTHORS`.
