# gpd-rig -- the GPD Win 2 test bench

Netboot + a debug link for the real GPD Win 2 (a Realtek RTL8153 USB Ethernet adapter on its USB-A port; USB-C stays on
power): the machine boots whatever image the lab serves, and the kernel's log and a small command channel run over the
same adapter, so nobody has to sit at the machine.

One-time setup, on the GPD's own Manjaro (Wi-Fi is enough):

    curl -fsSLO https://raw.githubusercontent.com/Phorepoc/adaptation-pc/main/tools/gpd-rig/setup-manjaro.sh
    sudo bash setup-manjaro.sh <lab-ip>:<port>

It builds `ipxe.efi` from source there and runs `install-rig.sh`. Until the lab answers, every boot falls back to
Manjaro after ~30 s.

| File | What |
|---|---|
| `build_ipxe.sh <out>` | builds `ipxe.efi` (iPXE v2.0.0, `rig.ipxe` embedded), `grubx64.efi` (GRUB, `grub-net.cfg` embedded) and `boot.ipxe` |
| `setup-manjaro.sh <lab host:port>` | the one-time setup above: build tools, `build_ipxe.sh` with `NO_GRUB=1`, then `install-rig.sh` |
| `install-rig.sh <lab host:port>` | run once on the GPD's Manjaro as root: ipxe.efi onto the ESP, a boot entry first, Manjaro kept as the fallback; `--undo` |
| `rig.ipxe` | iPXE: DHCP, then `http://<lab>/gpd/boot.ipxe`, 5 tries, then `exit` (the firmware boots Manjaro) |
| `boot.ipxe`, `grub-net.cfg` | the lab's side: GRUB loads `/gpd/phorepoc.elf` by multiboot2 over HTTP, as the ISO's GRUB does |
| `test_netboot.sh <elf>` | the chain in QEMU (OVMF, usb-net on xHCI, a local HTTP server); `FALLBACK=1` = the no-lab path |
| `rigctl.py` | the debug link from a lab PC: `log`, `ping`, `reboot`, `dump`, `hang`, `key`, `touch`, `shot` |

The kernel side (`NETLOG=1 TCO=1`, the gpd-win2 profile): adaptation-x32 `usbhost/xk_netlog.cpp` (the log as UDP
broadcasts to :6666, commands on :6667; `ekern/xk_netlog_proto.h`), pheusb `phe_usb_ure.c` (FreeBSD if_ure's chip code)
and `ekern/xk_tco.cpp` (the chipset watchdog). iPXE is GPL-2.0: hand `ipxe.efi` out with `ipxe.src-commit` (its source
is https://github.com/ipxe/ipxe at that commit).
