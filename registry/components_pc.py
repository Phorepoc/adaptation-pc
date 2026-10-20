"""components_pc -- the adaptation-pc components (merged into core's registry when this adaptation is built:
PHOREPOC_ADAPTATIONS, see the pdk's tools/emitter/paths.py). Paths are relative to the merged OS tree."""
import os

from paths import OS_ROOT  # noqa: F401

COMPONENTS = {
    # OURS (vm-gpu-options.md S2): the first user of the SVGA3D kernel driver (RVmwSvgaChannel): object tables / MOB / GB surface / screen target / DX context /
    # GPU clear on the VirtualBox VMSVGA device (VBOX_3D=1). SSM_TEST=phesvgatest (x86 only)
    "phesvgatest": dict(mmp=os.path.join(OS_ROOT, "src/phesvgatest/phesvgatest.mmp"), test=True),
}
