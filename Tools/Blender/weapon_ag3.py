"""AG3 (H&K G3 made by Kongsberg) - Norwegian service rifle. Built from Andreas' reference photo.
Approx. real size: 1.025 m long. Origin at the top of the pistol grip, muzzle towards +X."""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from gs_common import *

reset_scene()
steel = material("M_AG3_Steel", (0.05, 0.05, 0.055), metallic=0.85, roughness=0.45)
poly = material("M_AG3_Polymer", (0.06, 0.075, 0.05), roughness=0.85)

# Receiver (stamped steel, slightly taller at the back)
box("Receiver", (0.36, 0.046, 0.066), (0.06, 0, 0.0), steel)
box("ReceiverTop", (0.30, 0.03, 0.012), (0.07, 0, 0.039), steel)
box("Magwell", (0.085, 0.05, 0.035), (0.07, 0, -0.045), steel)
# Rear drum sight and front sight hood
cyl("RearDrum", 0.014, 0.03, (-0.085, 0, 0.055), steel, axis="Y")
box("RearSightBase", (0.04, 0.03, 0.018), (-0.085, 0, 0.043), steel)
# Front sight assembly clamps the barrel and the cocking tube together
box("FrontBase", (0.04, 0.03, 0.075), (0.69, 0, 0.03), steel)
cyl("FrontHood", 0.017, 0.022, (0.69, 0, 0.082), steel, axis="X", verts=16)
box("FrontPost", (0.006, 0.004, 0.022), (0.69, 0, 0.078), steel)
# Cocking tube + handle (above the barrel, inside the handguard)
cyl("CockingTube", 0.015, 0.45, (0.465, 0, 0.034), steel)
box("CockingHandle", (0.03, 0.05, 0.012), (0.27, -0.03, 0.04), steel, rot=(0, 0, -10))
# Handguard (slim AG3 polymer handguard with vents)
prism("Handguard", [(0.24, -0.03), (0.54, -0.022), (0.54, 0.05), (0.24, 0.052)], 0.058, poly, bevel=0.006)
for i in range(5):
    box(f"Vent{i}", (0.03, 0.06, 0.008), (0.29 + i * 0.05, 0, 0.012), steel, bevel=0.0)
# Barrel + flash hider
cyl("Barrel", 0.0095, 0.60, (0.54, 0, 0.0), steel)
cyl("FlashHider", 0.0125, 0.065, (0.865, 0, 0.0), steel, verts=16)
for i in range(4):
    box(f"HiderSlot{i}", (0.04, 0.027, 0.003), (0.87, 0, -0.006 + i * 0.004), material("M_AG3_Dark", (0.01, 0.01, 0.01)), bevel=0.0)
# Pistol grip (angled back)
prism("Grip", [(-0.045, -0.03), (0.02, -0.03), (-0.005, -0.15), (-0.065, -0.14)], 0.034, poly, bevel=0.006)
# Trigger guard and trigger
box("TriggerGuardBottom", (0.07, 0.012, 0.006), (0.035, 0, -0.075), poly)
box("TriggerGuardFront", (0.006, 0.012, 0.04), (0.068, 0, -0.055), poly)
box("Trigger", (0.006, 0.006, 0.025), (0.035, 0, -0.05), steel, rot=(0, 15, 0))
# 20 round magazine, slight curve
prism("Magazine", [(0.035, -0.035), (0.105, -0.035), (0.115, -0.21), (0.05, -0.215)], 0.03, steel, bevel=0.003)
box("MagBase", (0.072, 0.034, 0.01), (0.082, 0, -0.215), steel, rot=(0, -4, 0))
# Fixed stock
prism("Stock", [(-0.48, -0.13), (-0.10, -0.03), (-0.10, 0.033), (-0.48, 0.045)], 0.044, poly, bevel=0.008)
box("ButtPlate", (0.02, 0.046, 0.18), (-0.49, 0, -0.043), poly, rot=(0, 3, 0))
# Sling loops
cyl("SlingFront", 0.012, 0.004, (0.60, 0.032, 0.0), steel, axis="Y", verts=12)
cyl("SlingRear", 0.012, 0.004, (-0.40, 0.025, -0.02), steel, axis="Y", verts=12)

join_all("SM_AG3")
export("SM_AG3")
