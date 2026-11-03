# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

"""components_pc -- the adaptation-pc components (merged into core's registry when this adaptation is built:
PHOREPOC_ADAPTATIONS, see the pdk's tools/emitter/paths.py). Paths are relative to the merged OS tree."""
import os

from paths import OS_ROOT  # noqa: F401

COMPONENTS = {
    # OURS (vm-gpu-options.md S2): the first user of the SVGA3D kernel driver (RVmwSvgaChannel): object tables / MOB / GB surface / screen target / DX context /
    # GPU clear on the VirtualBox VMSVGA device (VBOX_3D=1). SSM_TEST=phesvgatest (x86 only)
    "phesvgatest": dict(mmp=os.path.join(OS_ROOT, "src/phesvgatest/phesvgatest.mmp"), test=True),
}

# core components that get this board's data (core registry: EXTEND)
EXTEND = {
    # OpenGL ES + EGL (mesa-symbian's software renderer): the window server here is not the NGA one, and core marks the GL
    # stack `nga` because its surfaces are SurfaceManager surfaces the NGA compositor shows. symgl_window.cpp falls back
    # without one -- the frame is copied into a bitmap and drawn into the window -- so on this board the stack ships in the
    # ordinary ROM. (surfacemanager is only linked against: its kernel half is absent, RSurfaceManager::Open fails, and
    # that is what selects the bitmap path.)
    **{k: dict(nga=False) for k in ("surfacemanager", "phgl", "libegl", "libglesv2", "libglesv1_cm", "libgles_cm")},
    # the download app's device identity (board + architecture): it shows only what fits this board
    "pheget": dict(rom_data=[r"resource\phorepoc\device.txt=data/phorepoc/device_pc.txt"]),
}
