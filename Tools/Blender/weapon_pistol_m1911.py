"""M1911 / Kongsberg Colt m/1914 pistol. Built from Andreas' reference photo.
Approx. 0.216 m long. Origin at the top of the grip, muzzle towards +X."""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from gs_common import *

reset_scene()
steel = material("M_1911_Steel", (0.07, 0.07, 0.075), metallic=0.85, roughness=0.5)
wood = material("M_1911_Wood", (0.22, 0.11, 0.06), roughness=0.7)

# Slide with serrations at the back
box("Slide", (0.21, 0.024, 0.03), (0.06, 0, 0.022), steel, bevel=0.002)
for i in range(9):
    box(f"Serr{i}", (0.002, 0.026, 0.024), (-0.03 + i * 0.004, 0, 0.022), steel, bevel=0.0)
box("FrontSight", (0.006, 0.003, 0.006), (0.158, 0, 0.04), steel, bevel=0.0)
box("RearSight", (0.008, 0.012, 0.006), (-0.03, 0, 0.04), steel, bevel=0.0)
cyl("Muzzle", 0.0075, 0.006, (0.166, 0, 0.022), steel, verts=16)
# Frame / dust cover
box("Frame", (0.17, 0.022, 0.016), (0.065, 0, -0.001), steel)
# Grip with wooden panels (angled back)
prism("GripFrame", [(-0.045, -0.008), (0.01, -0.008), (-0.01, -0.12), (-0.06, -0.115)], 0.024, steel, bevel=0.003)
prism("GripPanels", [(-0.04, -0.018), (0.003, -0.018), (-0.013, -0.108), (-0.053, -0.104)], 0.029, wood, bevel=0.003)
# Trigger guard and trigger
box("TGBottom", (0.04, 0.008, 0.005), (0.025, 0, -0.04), steel)
box("TGFront", (0.005, 0.008, 0.032), (0.045, 0, -0.025), steel)
box("Trigger", (0.006, 0.006, 0.02), (0.014, 0, -0.02), steel)
# Hammer, slide stop, mag base, lanyard loop
box("Hammer", (0.012, 0.008, 0.016), (-0.052, 0, 0.03), steel, rot=(0, -25, 0))
box("SlideStop", (0.02, 0.028, 0.006), (0.03, 0, 0.008), steel)
box("MagBase", (0.03, 0.022, 0.006), (-0.035, 0, -0.12), steel, rot=(0, 10, 0))
cyl("Lanyard", 0.006, 0.003, (-0.05, 0, -0.127), steel, axis="Y", verts=12)

join_all("SM_Pistol_M1911")
export("SM_Pistol_M1911")
