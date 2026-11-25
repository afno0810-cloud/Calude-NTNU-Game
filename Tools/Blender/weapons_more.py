"""Builds the rest of the weapons from Andreas' reference photos.
Each weapon: meters, muzzle/blade towards +X, origin at the top of the grip.
Run: python Tools/Blender/weapons_more.py   (or blender --background --python ...)"""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from gs_common import *


def mats():
    return dict(
        steel=material("M_Steel", (0.05, 0.05, 0.055), metallic=0.85, roughness=0.45),
        dark=material("M_DarkSteel", (0.02, 0.02, 0.022), metallic=0.7, roughness=0.55),
        wood=material("M_Wood", (0.30, 0.15, 0.07), roughness=0.65),
        bake=material("M_Bakelite", (0.12, 0.06, 0.03), roughness=0.5),
        poly=material("M_BlackPolymer", (0.03, 0.03, 0.03), roughness=0.85),
        brass=material("M_Brass", (0.6, 0.45, 0.15), metallic=0.9, roughness=0.35),
        blade=material("M_Blade", (0.75, 0.76, 0.78), metallic=1.0, roughness=0.2),
        leather=material("M_Leather", (0.18, 0.1, 0.05), roughness=0.8),
        wrap=material("M_KatanaWrap", (0.03, 0.03, 0.05), roughness=0.9),
        gold=material("M_Gold", (0.75, 0.55, 0.2), metallic=1.0, roughness=0.3),
    )


def mg3():
    reset_scene(); m = mats()
    box("Receiver", (0.42, 0.05, 0.075), (0.08, 0, 0.0), m["steel"])
    box("TopCover", (0.26, 0.055, 0.025), (0.0, 0, 0.05), m["steel"])
    box("FeedTray", (0.08, 0.09, 0.03), (0.0, 0, 0.03), m["dark"])
    cyl("Jacket", 0.022, 0.42, (0.50, 0, 0.0), m["steel"], verts=20)
    for i in range(10):
        box(f"Hole{i}", (0.018, 0.046, 0.012), (0.33 + i * 0.035, 0, 0.0), m["dark"], bevel=0.0)
    cyl("Muzzle", 0.026, 0.07, (0.745, 0, 0.0), m["steel"], verts=16)
    cyl("Barrel", 0.009, 0.06, (0.80, 0, 0.0), m["steel"])
    prism("Stock", [(-0.52, -0.12), (-0.13, -0.035), (-0.13, 0.035), (-0.52, 0.03)], 0.05, m["poly"], bevel=0.008)
    prism("Grip", [(-0.06, -0.035), (0.0, -0.035), (-0.025, -0.15), (-0.08, -0.14)], 0.034, m["poly"])
    box("TriggerGuard", (0.08, 0.012, 0.006), (0.0, 0, -0.075), m["steel"])
    box("FrontSight", (0.01, 0.004, 0.05), (0.68, 0, 0.045), m["steel"])
    box("RearSight", (0.03, 0.03, 0.03), (0.14, 0, 0.055), m["steel"])
    box("CarryHandle", (0.12, 0.012, 0.012), (0.48, 0.03, 0.06), m["steel"])
    # bipod folded under the jacket
    box("BipodL", (0.25, 0.008, 0.01), (0.47, -0.02, -0.035), m["steel"], rot=(0, -6, 0))
    box("BipodR", (0.25, 0.008, 0.01), (0.47, 0.02, -0.035), m["steel"], rot=(0, -6, 0))
    # belt hanging from the left
    for i in range(8):
        cyl(f"Round{i}", 0.006, 0.07, (0.0 - 0.003 * i, -0.06 - i * 0.012, -0.01 - i * 0.012), m["brass"], axis="X", verts=8)
    join_all("SM_MG3"); export("SM_MG3")


def bren():
    reset_scene(); m = mats()
    box("Receiver", (0.32, 0.05, 0.07), (0.05, 0, 0.0), m["steel"])
    cyl("Barrel", 0.016, 0.45, (0.43, 0, 0.0), m["steel"], verts=20)
    for i in range(8):
        cyl(f"Fin{i}", 0.022, 0.008, (0.25 + i * 0.03, 0, 0.0), m["steel"], verts=20)
    cyl("Flash", 0.02, 0.08, (0.69, 0, 0.0), m["steel"], verts=16)
    cyl("GasTube", 0.012, 0.35, (0.35, 0, -0.04), m["steel"])
    # curved top magazine
    prism("Magazine", [(0.0, 0.035), (0.07, 0.035), (0.1, 0.12), (0.09, 0.2), (0.04, 0.19), (0.03, 0.12)], 0.025, m["steel"])
    box("CarryHandle", (0.12, 0.015, 0.015), (0.26, 0.04, 0.05), m["wood"], rot=(0, 0, 20))
    prism("Stock", [(-0.5, -0.11), (-0.1, -0.035), (-0.1, 0.03), (-0.5, 0.03)], 0.045, m["wood"], bevel=0.008)
    box("ButtPlate", (0.015, 0.05, 0.145), (-0.505, 0, -0.04), m["steel"])
    prism("Grip", [(-0.05, -0.035), (0.01, -0.035), (-0.02, -0.14), (-0.07, -0.13)], 0.034, m["wood"])
    box("TriggerGuard", (0.07, 0.012, 0.006), (0.0, 0, -0.07), m["steel"])
    box("BipodL", (0.006, 0.01, 0.28), (0.5, -0.05, -0.13), m["steel"], rot=(18, 15, 0))
    box("BipodR", (0.006, 0.01, 0.28), (0.5, 0.05, -0.13), m["steel"], rot=(-18, 15, 0))
    box("RearSight", (0.02, 0.03, 0.04), (-0.05, -0.03, 0.05), m["steel"])
    box("FrontSight", (0.01, 0.004, 0.03), (0.47, -0.03, 0.04), m["steel"])
    join_all("SM_Bren"); export("SM_Bren")


def pkm():
    reset_scene(); m = mats()
    box("Receiver", (0.34, 0.05, 0.07), (0.07, 0, 0.0), m["steel"])
    box("TopCover", (0.22, 0.054, 0.02), (0.03, 0, 0.045), m["steel"])
    cyl("Barrel", 0.012, 0.46, (0.47, 0, 0.01), m["steel"], verts=20)
    cyl("GasTube", 0.011, 0.30, (0.37, 0, -0.03), m["steel"])
    cyl("Flash", 0.016, 0.07, (0.73, 0, 0.01), m["steel"], verts=16)
    box("CarryHandle", (0.10, 0.012, 0.03), (0.30, 0, 0.05), m["wood"])
    prism("Stock", [(-0.5, -0.1), (-0.12, -0.035), (-0.12, 0.03), (-0.5, 0.04), (-0.34, -0.03)], 0.04, m["bake"], bevel=0.008)
    prism("Grip", [(-0.05, -0.035), (0.01, -0.035), (-0.02, -0.14), (-0.07, -0.13)], 0.034, m["bake"])
    box("TriggerGuard", (0.07, 0.012, 0.006), (0.0, 0, -0.07), m["steel"])
    # ammo box hanging under the receiver, belt into the left side
    box("AmmoBox", (0.11, 0.09, 0.13), (0.08, 0, -0.12), m["steel"])
    box("AmmoBoxLid", (0.115, 0.095, 0.01), (0.08, 0, -0.05), m["dark"])
    for i in range(5):
        cyl(f"Round{i}", 0.005, 0.06, (0.06 + i * 0.01, -0.035, -0.04 + i * 0.012), m["brass"], verts=8)
    box("BipodL", (0.006, 0.01, 0.3), (0.55, -0.06, -0.13), m["steel"], rot=(20, 12, 0))
    box("BipodR", (0.006, 0.01, 0.3), (0.55, 0.06, -0.13), m["steel"], rot=(-20, 12, 0))
    join_all("SM_PKM"); export("SM_PKM")


def _xl(name, prof, mat, segs=24, x0=0.0, z0=0.0, y0=0.0):
    """Turned part along +X from a (radius, x) profile."""
    import math as _m
    from gen_models import lathe
    o = lathe(name, prof, mat, segs=segs)
    o.location = (x0, y0, z0)
    o.rotation_euler = (0, _m.radians(90), 0)
    return o


def _tube(*a, **k):
    from gen_models import tube
    return tube(*a, **k)


def mp40():
    reset_scene(); m = mats()
    # ribbed receiver tube, barrel with nut and the bakelite resting bar
    prof = [(0.0, -0.112), (0.02, -0.112), (0.022, -0.105)]
    for i in range(10):
        x = -0.1 + i * 0.024
        prof += [(0.022, x), (0.0205, x + 0.006), (0.0205, x + 0.012), (0.022, x + 0.018)]
    prof += [(0.022, 0.15), (0.0, 0.152)]
    _xl("Receiver", prof, m["steel"], segs=24, x0=0.02 - 0.02)
    _xl("BarrelNut", [(0.0, 0.15), (0.016, 0.15), (0.016, 0.175), (0.0, 0.176)], m["dark"], segs=16)
    _xl("Barrel", [(0.0, 0.17), (0.0095, 0.17), (0.0085, 0.33), (0.0105, 0.332), (0.0105, 0.345), (0.0, 0.346)], m["steel"], segs=16, z0=-0.005)
    prism("RestBar", [(0.16, -0.022), (0.215, -0.022), (0.22, -0.034), (0.16, -0.036)], 0.026, m["bake"], bevel=0.003)
    _xl("FrontHood", [(0.0, 0.318), (0.009, 0.318), (0.009, 0.332), (0.0, 0.332)], m["dark"], segs=12, z0=0.018)
    box("FrontSight", (0.006, 0.003, 0.016), (0.325, 0, 0.012), m["steel"], bevel=0)
    # magwell and magazine with ribs
    box("Magwell", (0.04, 0.034, 0.075), (0.12, 0, -0.05), m["steel"], bevel=0.003)
    prism("Magazine", [(0.106, -0.085), (0.134, -0.085), (0.134, -0.33), (0.106, -0.33)], 0.021, m["dark"], bevel=0.002)
    for sy in (-1, 1):
        box(f"MagRib{sy}", (0.008, 0.002, 0.2), (0.12, sy * 0.0108, -0.22), m["steel"], bevel=0)
    # bakelite lower and grip with finger grooves
    prism("Frame", [(-0.1, -0.022), (0.1, -0.022), (0.1, -0.045), (0.02, -0.045), (-0.1, -0.035)], 0.034, m["bake"], bevel=0.004)
    prism("Grip", [(-0.048, -0.04), (0.002, -0.04), (-0.018, -0.145), (-0.065, -0.138)], 0.032, m["bake"], bevel=0.006)
    for i in range(6):
        box(f"GripRib{i}", (0.04, 0.0335, 0.003), (-0.03 - i * 0.004, 0, -0.06 - i * 0.014), m["bake"], rot=(0, -20, 0), bevel=0)
    _tube("Guard", [(0.006, 0, -0.045), (0.04, 0, -0.05), (0.044, 0, -0.078), (0.0, 0, -0.08)], [0.0035] * 4, m["steel"], segs=8, squash=(1.6, 1.0))
    box("Trigger", (0.006, 0.006, 0.024), (0.022, 0, -0.06), m["steel"], rot=(0, -12, 0), bevel=0.001)
    box("CockingKnob", (0.014, 0.034, 0.014), (0.06, -0.03, 0.012), m["steel"], bevel=0.004)
    box("CockSlot", (0.14, 0.002, 0.006), (0.02, -0.0215, 0.01), m["dark"], bevel=0)
    # folding stock folded under the gun
    for sy in (-1, 1):
        _tube(f"StockArm{sy}", [(-0.09, sy * 0.022, -0.045), (0.02, sy * 0.022, -0.062), (0.14, sy * 0.022, -0.062)], [0.005] * 3, m["steel"], segs=8, squash=(1.0, 1.6))
    box("ButtPlate", (0.012, 0.052, 0.065), (0.15, 0, -0.088), m["steel"], bevel=0.004)
    box("RearSight", (0.02, 0.022, 0.02), (-0.04, 0, 0.026), m["steel"], bevel=0.002)
    for sy in (-1, 1):
        box(f"RearEar{sy}", (0.012, 0.004, 0.014), (-0.04, sy * 0.009, 0.04), m["steel"], bevel=0)
    _tube("Sling", [(-0.1, 0.0, -0.02), (-0.112, 0.0, -0.03), (-0.1, 0.0, -0.04)], [0.003] * 3, m["steel"], segs=6)
    join_all("SM_MP40"); export("SM_MP40")




def luger():
    reset_scene(); m = mats()
    _xl("Barrel", [(0.0, 0.045), (0.012, 0.045), (0.012, 0.06), (0.0095, 0.065), (0.0082, 0.148), (0.0, 0.15)], m["steel"], segs=18, z0=0.02)
    box("FrontSight", (0.006, 0.003, 0.01), (0.142, 0, 0.032), m["steel"], bevel=0)
    box("Receiver", (0.105, 0.022, 0.03), (-0.002, 0, 0.012), m["steel"], bevel=0.004)
    # toggle joint with its two knurled knobs
    box("Toggle1", (0.035, 0.016, 0.014), (-0.02, 0, 0.033), m["steel"], bevel=0.004)
    box("Toggle2", (0.03, 0.016, 0.014), (-0.05, 0, 0.033), m["steel"], bevel=0.004)
    cyl("Pin", 0.006, 0.02, (-0.035, 0, 0.033), m["dark"], axis="Y", verts=12)
    for sy in (-1, 1):
        cyl(f"Knob{sy}", 0.0095, 0.008, (-0.066, sy * 0.012, 0.036), m["steel"], axis="Y", verts=16)
        for k in range(6):
            box(f"Knurl{sy}{k}", (0.019, 0.0085, 0.0015), (-0.066, sy * 0.012, 0.036), m["dark"], rot=(0, k * 30, 0), bevel=0)
    # raked wooden grip with checkering and the magazine base knob
    prism("Frame", [(-0.06, -0.003), (0.0, -0.003), (-0.035, -0.112), (-0.094, -0.102)], 0.022, m["steel"], bevel=0.003)
    for sy in (-1, 1):
        prism(f"Panel{sy}", [(-0.054, -0.012), (-0.008, -0.012), (-0.036, -0.1), (-0.087, -0.093)], 0.004, m["wood"], y=sy * 0.012, bevel=0.0015)
        cyl(f"GripScrew{sy}", 0.004, 0.0045, (-0.05, sy * 0.0142, -0.055), m["steel"], axis="Y", verts=10)
    box("MagBase", (0.03, 0.018, 0.01), (-0.066, 0, -0.11), m["wood"], rot=(0, 10, 0), bevel=0.003)
    _tube("Guard", [(0.008, 0, -0.003), (0.035, 0, -0.012), (0.036, 0, -0.038), (0.0, 0, -0.04)], [0.0032] * 4, m["steel"], segs=8, squash=(1.6, 1.0))
    box("Trigger", (0.006, 0.006, 0.02), (0.016, 0, -0.02), m["steel"], rot=(0, -12, 0), bevel=0.001)
    box("Safety", (0.02, 0.003, 0.006), (-0.045, 0.012, 0.002), m["steel"], bevel=0)
    join_all("SM_Luger"); export("SM_Luger")




def uzi():
    reset_scene(); m = mats()
    box("Receiver", (0.3, 0.045, 0.058), (0.06, 0, 0.01), m["steel"], bevel=0.006)
    for sy in (-1, 1):
        for k in range(3):
            box(f"Rib{sy}{k}", (0.24, 0.002, 0.004), (0.05, sy * 0.0228, -0.005 + k * 0.014), m["dark"], bevel=0)
    box("TopCover", (0.22, 0.047, 0.012), (0.04, 0, 0.044), m["dark"], bevel=0.003)
    for k in range(8):
        box(f"CoverRib{k}", (0.004, 0.044, 0.003), (0.08 + k * 0.007, 0, 0.051), m["steel"], bevel=0)
    box("CockingKnob", (0.02, 0.02, 0.014), (0.12, 0, 0.057), m["dark"], bevel=0.004)
    _xl("BarrelNut", [(0.0, 0.21), (0.021, 0.21), (0.021, 0.228), (0.0, 0.229)], m["dark"], segs=20)
    for k in range(10):
        box(f"Knurl{k}", (0.018, 0.0435, 0.002), (0.219, 0, 0.0), m["steel"], rot=(k * 18, 0, 0), bevel=0)
    _xl("Barrel", [(0.0, 0.228), (0.0095, 0.228), (0.0095, 0.262), (0.0, 0.263)], m["steel"], segs=16)
    # front sight guard ears and rear sight
    for sy in (-1, 1):
        box(f"FrontEar{sy}", (0.016, 0.004, 0.03), (0.18, sy * 0.012, 0.058), m["steel"], bevel=0.001)
    box("FrontPost", (0.004, 0.003, 0.02), (0.18, 0, 0.055), m["steel"], bevel=0)
    for sy in (-1, 1):
        box(f"RearEar{sy}", (0.016, 0.004, 0.022), (-0.06, sy * 0.012, 0.055), m["steel"], bevel=0.001)
    # grip with the magazine inside it, plastic panels and grip safety
    prism("GripMag", [(-0.025, -0.02), (0.025, -0.02), (0.035, -0.3), (-0.015, -0.3)], 0.032, m["steel"], bevel=0.003)
    for sy in (-1, 1):
        prism(f"Panel{sy}", [(-0.022, -0.03), (0.022, -0.03), (0.03, -0.17), (-0.016, -0.17)], 0.004, m["poly"], y=sy * 0.017, bevel=0.0015)
        for k in range(8):
            box(f"Chk{sy}{k}", (0.04, 0.0045, 0.002), (0.004 + k * 0.001, sy * 0.017, -0.045 - k * 0.015), m["dark"], bevel=0)
    box("GripSafety", (0.008, 0.026, 0.06), (-0.028, 0, -0.07), m["dark"], rot=(0, -4, 0), bevel=0.002)
    box("MagBase", (0.055, 0.036, 0.012), (0.01, 0, -0.302), m["steel"], bevel=0.003)
    box("Handguard", (0.12, 0.05, 0.03), (0.14, 0, -0.03), m["poly"], bevel=0.008)
    for k in range(5):
        box(f"HgRib{k}", (0.004, 0.051, 0.022), (0.1 + k * 0.018, 0, -0.03), m["dark"], bevel=0)
    _tube("Guard", [(0.03, 0, -0.02), (0.05, 0, -0.04), (0.04, 0, -0.085), (-0.02, 0, -0.085), (-0.03, 0, -0.07)], [0.004] * 5, m["steel"], segs=8, squash=(1.6, 1.0))
    box("Trigger", (0.006, 0.006, 0.024), (0.012, 0, -0.055), m["steel"], rot=(0, -12, 0), bevel=0.001)
    box("Selector", (0.012, 0.004, 0.012), (-0.02, 0.025, -0.01), m["dark"], bevel=0.002)
    # folding metal stock (extended)
    for sy in (-1, 1):
        _tube(f"StockArm{sy}", [(-0.09, sy * 0.018, -0.005), (-0.18, sy * 0.02, -0.025), (-0.26, sy * 0.02, -0.035)], [0.005] * 3, m["steel"], segs=8, squash=(1.0, 1.6))
    box("Butt", (0.014, 0.055, 0.075), (-0.265, 0, -0.045), m["steel"], bevel=0.005)
    box("Hinge", (0.02, 0.05, 0.02), (-0.09, 0, -0.005), m["dark"], bevel=0.004)
    join_all("SM_Uzi"); export("SM_Uzi")




def suomi():
    reset_scene(); m = mats()
    _xl("Receiver", [(0.0, -0.12), (0.022, -0.12), (0.024, -0.112), (0.024, 0.2), (0.0, 0.202)], m["steel"], segs=24)
    _xl("EndCap", [(0.0, -0.135), (0.016, -0.135), (0.02, -0.125), (0.023, -0.118), (0.0, -0.117)], m["dark"], segs=20)
    _xl("Jacket", [(0.0, 0.2), (0.02, 0.2), (0.02, 0.47), (0.0, 0.471)], m["steel"], segs=24)
    for i in range(7):
        box(f"Slot{i}", (0.022, 0.042, 0.008), (0.25 + i * 0.03, 0, 0.0), m["dark"], bevel=0.003)
    _xl("Muzzle", [(0.0, 0.47), (0.017, 0.47), (0.017, 0.5), (0.012, 0.505), (0.0, 0.506)], m["steel"], segs=18)
    box("Brake", (0.016, 0.004, 0.034), (0.488, 0, 0.0), m["dark"], bevel=0)
    # wooden stock with wrist and fore end
    prism("Stock", [(-0.43, -0.13), (-0.38, -0.135), (-0.2, -0.085), (-0.1, -0.075), (-0.05, -0.085), (-0.02, -0.06), (-0.02, -0.018),
                    (-0.15, -0.025), (-0.43, -0.01)], 0.044, m["wood"], bevel=0.01)
    box("ButtPlate", (0.008, 0.046, 0.12), (-0.432, 0, -0.07), m["dark"], rot=(0, 4, 0), bevel=0.003)
    prism("Forend", [(-0.02, -0.018), (0.2, -0.018), (0.2, -0.045), (0.16, -0.05), (-0.02, -0.06)], 0.046, m["wood"], bevel=0.008)
    _tube("Guard", [(0.0, 0, -0.058), (-0.01, 0, -0.1), (-0.04, 0, -0.105), (-0.06, 0, -0.085)], [0.004] * 4, m["steel"], segs=8, squash=(1.6, 1.0))
    box("Trigger", (0.006, 0.006, 0.026), (-0.025, 0, -0.08), m["steel"], rot=(0, -12, 0), bevel=0.001)
    # 70 round drum: ribbed shell, cap and winding key
    from gen_models import lathe as _lathe
    import math as _m
    prof = [(0.0, -0.032), (0.095, -0.032), (0.1, -0.026)]
    for k in range(3):
        z = -0.02 + k * 0.016
        prof += [(0.1, z), (0.102, z + 0.004), (0.1, z + 0.008)]
    prof += [(0.1, 0.026), (0.095, 0.032), (0.0, 0.032)]
    d = _lathe("Drum", prof, m["steel"], segs=40)
    d.location = (0.05, 0, -0.17)
    d.rotation_euler = (_m.radians(90), 0, 0)
    cyl("DrumCap", 0.06, 0.068, (0.05, 0, -0.17), m["dark"], axis="Y", verts=32)
    box("Key", (0.05, 0.012, 0.012), (0.05, -0.04, -0.17), m["steel"], bevel=0.004)
    box("DrumNeck", (0.045, 0.036, 0.06), (0.05, 0, -0.06), m["steel"], bevel=0.004)
    box("RearSight", (0.05, 0.02, 0.016), (0.15, 0, 0.03), m["steel"], bevel=0.003)
    box("FrontSight", (0.008, 0.004, 0.02), (0.46, 0, 0.026), m["steel"], bevel=0)
    box("CockHandle", (0.02, 0.012, 0.01), (-0.08, 0, 0.028), m["dark"], bevel=0.003)
    join_all("SM_Suomi"); export("SM_Suomi")




def katana():
    reset_scene(); m = mats()
    # slightly curved blade (several angled segments)
    for i in range(6):
        box(f"Blade{i}", (0.12, 0.006, 0.032 - i * 0.002), (0.08 + i * 0.115, 0, 0.003 * i * i * 0.3), m["blade"],
            rot=(0, -i * 1.2, 0), bevel=0.001)
    box("Tip", (0.05, 0.005, 0.02), (0.76, 0, 0.03), m["blade"], rot=(0, -12, 0), bevel=0.001)
    cyl("Tsuba", 0.04, 0.008, (0.015, 0, 0.0), m["gold"], verts=24)
    cyl("Handle", 0.016, 0.26, (-0.12, 0, 0.0), m["wrap"], verts=12)
    for i in range(8):
        box(f"Wrap{i}", (0.012, 0.034, 0.034), (-0.23 + i * 0.03, 0, 0.0), m["gold"], rot=(45, 0, 0), bevel=0.0)
    cyl("Pommel", 0.018, 0.015, (-0.255, 0, 0.0), m["gold"], verts=16)
    join_all("SM_Katana"); export("SM_Katana")


def viking_sword():
    reset_scene(); m = mats()
    prism("Blade", [(0.02, -0.025), (0.72, -0.02), (0.8, 0.0), (0.72, 0.02), (0.02, 0.025)], 0.006, m["blade"], bevel=0.001)
    box("Fuller", (0.6, 0.007, 0.012), (0.35, 0, 0.0), material("M_BladeDark", (0.4, 0.4, 0.42), metallic=1.0, roughness=0.4), bevel=0.0)
    box("Guard", (0.025, 0.03, 0.16), (0.0, 0, 0.0), m["steel"], bevel=0.004)
    cyl("Grip", 0.016, 0.1, (-0.065, 0, 0.0), m["leather"], verts=12)
    prism("Pommel", [(-0.115, -0.04), (-0.13, -0.03), (-0.15, 0.0), (-0.13, 0.03), (-0.115, 0.04)], 0.03, m["steel"], bevel=0.004)
    join_all("SM_VikingSword"); export("SM_VikingSword")


if __name__ == "__main__":
    want = sys.argv[1:]
    for fn in (mg3, bren, pkm, mp40, luger, uzi, suomi, katana, viking_sword):
        if not want or fn.__name__ in want:
            fn()
