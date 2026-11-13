# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

"""components_pc -- the adaptation-pc components (merged into core's registry when this adaptation is built:
PHOREPOC_ADAPTATIONS, see the pdk's tools/emitter/paths.py). Paths are relative to the merged OS tree."""
import os

from paths import OS_ROOT  # noqa: F401
from paths import resolve as _resolve_src  # ("baseline/..." -> the Symbian source)

_SCDV = "oss.FCL.sf.os.graphics/graphicsdeviceinterface/screendriver"

COMPONENTS = {
    # OURS (vm-gpu-options.md S2): the first user of the SVGA3D kernel driver (RVmwSvgaChannel): object tables / MOB / GB surface / screen target / DX context /
    # GPU clear on the VirtualBox VMSVGA device (VBOX_3D=1). SSM_TEST=phesvgatest (x86 only)
    "phesvgatest": dict(mmp=os.path.join(OS_ROOT, "src/phesvgatest/phesvgatest.mmp"), test=True),
    # OURS: what an application that asks for landscape does to the window server, without Avkon -- the landscape screen mode and a test pattern.
    # SSM_TEST=phelandtest
    "phelandtest": dict(mmp=os.path.join(OS_ROOT, "src/phelandtest/phelandtest.mmp"), test=True),
    # OURS: the WLAN hardware-init plug-in of a board with no WLAN chip -- no data, no firmware, no MAC address (KErrNotFound),
    # what a phone without the part answers; the 2010 adaptation stub invented an address <- wlmserversrv
    "wlanhwinit": dict(mmp=os.path.join(OS_ROOT, "src/wlanhwinit/group/wlanhwinit.mmp")),
}

# core components that get this board's data (core registry: EXTEND)
EXTEND = {
    # OpenGL ES + EGL (mesa-symbian's software renderer): the window server here is not the NGA one, and core marks the GL
    # stack `nga` because its surfaces are SurfaceManager surfaces the NGA compositor shows. symgl_window.cpp falls back
    # without one -- the frame is copied into a bitmap and drawn into the window -- so on this board the stack ships in the
    # ordinary ROM. (surfacemanager is only linked against: its kernel half is absent, RSurfaceManager::Open fails, and
    # that is what selects the bitmap path.)
    **{k: dict(nga=False) for k in ("surfacemanager", "libegl", "libglesv2", "libglesv1_cm", "libgles_cm")},
    # phgl also carries the channel to the VMware SVGA II / SVGA3D kernel driver (symvmw.h is core's seam): with it Mesa's svga driver draws on the host's
    # GPU when the VM has 3D (VirtualBox --accelerate3d on, VMware); without 3D, or on QEMU, eglInitialize keeps the software renderer
    "phgl": dict(nga=False, extra=["src/symgl/symvmw.cpp"], flags=["-iquote", os.path.join(OS_ROOT, "tools/phorepoc-x86/ekern")]),
    # Screen modes of this board: portrait 360x640 and a landscape 640x360 one rotated 90 degrees (os/board/wsini.txt). The window server
    # takes a mode's rotation only if the screen driver's draw device offers it (CFbsBitGc::OrientationsAvailable ->
    # CFbsDrawDevice::OrientationsAvailable, CScreen::LoadScreenSizeProperties: a rotation the device lacks is EWsPanicFailedToInitialise
    # at boot). The generic driver's draw device is the base CDrawBitmap, normal orientation only, because it expects the display
    # controller to rotate (the GCE/NGA compositor). The OMAP driver's draw devices (smomap/scdraw.h) offer all four orientations and
    # draw them in software (sbit BMDRAW*: the orientation is part of every pixel operation), over the same HAL framebuffer address,
    # pitch and size -- which is what the non-NGA window server of this board needs. Same sources, same Cdsb.cpp shim (its non-GCE
    # paths), the scdv.dll of the ROM.
    # The OMAP draw devices forget the display's row pitch (ConstructScreenL reads it from the HAL, then the base class's Construct sets
    # width * 4 again), and this display pads its rows like the real phones (1472 bytes for 360 pixels): the whole screen came out sheared
    # where the shadow screen is padded (VirtualBox). os/src/phescdv/scnew.cpp is the factory with the HAL's pitch put back; it replaces the
    # OMAP scnew.cpp. (EXTEND lists are appended to core's, which keep the Cdsb.cpp shim and the generic driver's include directories:
    # scnew.cpp includes the OMAP scdraw.h by its directory, "smomap/scdraw.h".)
    "scdv": dict(mmp=_SCDV + "/group/omapscdv.mmp",
                 drop=["scnew.cpp"],
                 extra=["src/phescdv/scnew.cpp"],
                 flags=["-iquote", _resolve_src(f"baseline/symbian-2010/{_SCDV}")]),
    # NGA images (NGA=1): libWFC's display back end. Core's talks to a display channel driver, which this board has not; ours copies each
    # composed frame into the display HAL's screen memory (EDisplayMemoryHandle), which the kernel's flusher shows (os/src/phewfcdisp).
    # Same DLL, UIDs and exports: only the source is replaced.
    "openwfc_ri_display": dict(drop=["openwfc_ri_displaychannel.cpp"], extra=["src/phewfcdisp/openwfc_ri_displayhal.cpp"],
                               flags=["-iquote", os.path.join(OS_ROOT, "src/phewfcdisp")]),
    # the download app's device identity (board + architecture): it shows only what fits this board (the catalogue layer)
    "catalogue_get": dict(rom_data=[r"resource\phorepoc\device.txt=data/phorepoc/device_pc.txt"]),
}
