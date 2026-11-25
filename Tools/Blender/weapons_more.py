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


def mp40():
    reset_scene(); m = mats()
    cyl("Receiver", 0.022, 0.26, (0.02, 0, 0.0), m["steel"], verts=20)
    cyl("Barrel", 0.01, 0.20, (0.25, 0, -0.005), m["steel"])
    box("RestBar", (0.03, 0.03, 0.02), (0.17, 0, -0.03), m["poly"])
    box("Magwell", (0.04, 0.035, 0.07), (0.12, 0, -0.05), m["steel"])
    prism("Magazine", [(0.104, -0.085), (0.136, -0.085), (0.136, -0.33), (0.104, -0.33)], 0.022, m["steel"], bevel=0.002)
    prism("Frame", [(-0.06, -0.025), (0.08, -0.025), (0.08, -0.04), (-0.06, -0.04)], 0.03, m["bake"])
    prism("Grip", [(-0.05, -0.035), (0.0, -0.035), (-0.02, -0.14), (-0.065, -0.13)], 0.03, m["bake"])
    box("TriggerGuard", (0.05, 0.01, 0.005), (0.015, 0, -0.07), m["steel"])
    box("CockingKnob", (0.012, 0.03, 0.012), (0.06, -0.03, 0.01), m["steel"])
    # folding stock (folded under)
    box("StockArmL", (0.24, 0.006, 0.01), (0.03, -0.022, -0.06), m["steel"])
    box("StockArmR", (0.24, 0.006, 0.01), (0.03, 0.022, -0.06), m["steel"])
    box("ButtPlate", (0.012, 0.05, 0.06), (0.15, 0, -0.085), m["steel"])
    box("RearSight", (0.02, 0.02, 0.025), (-0.04, 0, 0.03), m["steel"])
    box("FrontSight", (0.008, 0.004, 0.02), (0.33, 0, 0.012), m["steel"])
    join_all("SM_MP40"); export("SM_MP40")


def luger():
    reset_scene(); m = mats()
    cyl("Barrel", 0.0085, 0.11, (0.095, 0, 0.02), m["steel"], verts=16)
    box("Receiver", (0.10, 0.022, 0.03), (0.0, 0, 0.012), m["steel"])
    box("Toggle", (0.06, 0.018, 0.016), (-0.035, 0, 0.032), m["steel"])
    cyl("ToggleKnobL", 0.008, 0.035, (-0.06, 0, 0.032), m["steel"], axis="Y", verts=12)
    prism("Grip", [(-0.055, -0.005), (0.0, -0.005), (-0.035, -0.11), (-0.09, -0.1)], 0.026, m["wood"])
    box("TriggerGuard", (0.045, 0.008, 0.005), (0.015, 0, -0.035), m["steel"])
    box("FrontSight", (0.004, 0.003, 0.008), (0.145, 0, 0.032), m["steel"])
    join_all("SM_Luger"); export("SM_Luger")


def uzi():
    reset_scene(); m = mats()
    box("Receiver", (0.30, 0.045, 0.06), (0.06, 0, 0.01), m["steel"])
    cyl("Barrel", 0.01, 0.05, (0.235, 0, 0.0), m["steel"])
    box("BarrelNut", (0.02, 0.04, 0.04), (0.215, 0, 0.0), m["steel"])
    box("TopCover", (0.22, 0.046, 0.012), (0.04, 0, 0.045), m["dark"])
    box("CockingKnob", (0.02, 0.02, 0.014), (0.12, 0, 0.056), m["dark"])
    prism("GripMag", [(-0.025, -0.02), (0.025, -0.02), (0.035, -0.3), (-0.015, -0.3)], 0.032, m["poly"])
    box("MagBase", (0.055, 0.036, 0.012), (0.01, 0, -0.302), m["steel"])
    box("Handguard", (0.12, 0.05, 0.03), (0.14, 0, -0.03), m["poly"])
    box("TriggerGuard", (0.07, 0.012, 0.006), (-0.01, 0, -0.08), m["steel"])
    box("StockArm", (0.22, 0.03, 0.01), (-0.15, 0, -0.02), m["steel"])
    box("Butt", (0.012, 0.05, 0.07), (-0.26, 0, -0.04), m["steel"])
    box("FrontSightGuard", (0.015, 0.03, 0.03), (0.18, 0, 0.055), m["steel"])
    join_all("SM_Uzi"); export("SM_Uzi")


def suomi():
    reset_scene(); m = mats()
    cyl("Receiver", 0.024, 0.32, (0.04, 0, 0.0), m["steel"], verts=20)
    cyl("Jacket", 0.02, 0.28, (0.34, 0, 0.0), m["steel"], verts=20)
    for i in range(7):
        box(f"Slot{i}", (0.02, 0.042, 0.008), (0.24 + i * 0.03, 0, 0.0), m["dark"], bevel=0.0)
    cyl("Muzzle", 0.016, 0.03, (0.49, 0, 0.0), m["steel"], verts=16)
    # wooden stock + fore end
    prism("Stock", [(-0.42, -0.12), (-0.1, -0.07), (-0.02, -0.06), (-0.02, -0.02), (-0.42, -0.01)], 0.045, m["wood"], bevel=0.008)
    box("Forend", (0.2, 0.045, 0.03), (0.06, 0, -0.035), m["wood"])
    box("TriggerGuard", (0.06, 0.012, 0.006), (-0.02, 0, -0.1), m["steel"])
    box("Trigger", (0.006, 0.006, 0.025), (-0.02, 0, -0.08), m["steel"])
    # drum magazine (70 rounds)
    cyl("Drum", 0.1, 0.06, (0.05, 0, -0.16), m["steel"], axis="Y", verts=32)
    cyl("DrumCap", 0.06, 0.065, (0.05, 0, -0.16), m["dark"], axis="Y", verts=24)
    box("DrumNeck", (0.04, 0.035, 0.05), (0.05, 0, -0.06), m["steel"])
    box("RearSight", (0.03, 0.02, 0.02), (0.15, 0, 0.03), m["steel"])
    box("FrontSight", (0.008, 0.004, 0.02), (0.47, 0, 0.025), m["steel"])
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
    for fn in (mg3, bren, pkm, mp40, luger, uzi, suomi, katana, viking_sword):
        fn()
