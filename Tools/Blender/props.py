"""Furniture and campus props: python Tools/Blender/props.py [Name ...]

Every prop is SM_Prop_<Name>, meters, origin on the floor in the middle, front facing +X (Unreal space).
Material slot names (Wood, Metal, Plastic, Fabric, Concrete ...) are matched to Megascans materials by
Content/Python/init_unreal.py, and the meshes have tiling UVs, so they look real once those are added.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from meshkit import MeshKit  # noqa: E402

PROPS = {}


def prop(f):
    PROPS[f.__name__[2:]] = f  # "p_Desk" -> "Desk"
    return f


class Mats:
    def __init__(self, k):
        self.wood = k.mat("Wood", (0.42, 0.28, 0.16), rough=0.55)
        self.lwood = k.mat("WoodLight", (0.7, 0.55, 0.38), rough=0.55)
        self.metal = k.mat("Metal", (0.55, 0.56, 0.58), metallic=0.9, rough=0.35)
        self.dark = k.mat("MetalDark", (0.08, 0.08, 0.09), metallic=0.7, rough=0.5)
        self.plastic = k.mat("Plastic", (0.06, 0.06, 0.07), rough=0.6)
        self.white = k.mat("PlasticWhite", (0.85, 0.86, 0.87), rough=0.5)
        self.fabric = k.mat("Fabric", (0.12, 0.18, 0.32), rough=0.95)
        self.fabric_red = k.mat("FabricRed", (0.45, 0.07, 0.07), rough=0.95)
        self.concrete = k.mat("Concrete", (0.55, 0.54, 0.52), rough=0.9)
        self.screen = k.mat("Screen", (0.02, 0.05, 0.12), metallic=0.2, rough=0.1)
        self.screen_on = k.mat("ScreenOn", (0.15, 0.45, 0.95), rough=0.2)
        self.glass = k.mat("Glass", (0.6, 0.75, 0.8), metallic=0.3, rough=0.05)
        self.leaf = k.mat("Leaf", (0.12, 0.32, 0.1), rough=0.7)
        self.soil = k.mat("Soil", (0.18, 0.12, 0.08), rough=1.0)
        self.red = k.mat("PaintRed", (0.75, 0.05, 0.04), rough=0.4)
        self.blue = k.mat("PaintBlue", (0.05, 0.2, 0.55), rough=0.4)
        self.yellow = k.mat("PaintYellow", (0.95, 0.75, 0.1), rough=0.45)
        self.green = k.mat("PaintGreen", (0.1, 0.35, 0.2), rough=0.45)
        self.orange = k.mat("PaintOrange", (1.0, 0.45, 0.08), rough=0.45)
        self.rubber = k.mat("Rubber", (0.02, 0.02, 0.02), rough=0.95)
        self.paper = k.mat("Paper", (0.9, 0.88, 0.82), rough=0.9)
        self.ntnu = k.mat("NTNUBlue", (0.0, 0.25, 0.55), rough=0.4)


# ------------------------------------------------------------------ indoor
@prop
def p_Desk(k, m):
    """Student desk: wooden top, metal legs, modesty panel."""
    k.box((0, 0, 0.73), (0.7, 1.4, 0.03), m.lwood)
    for x in (-0.3, 0.3):
        for y in (-0.65, 0.65):
            k.box((x, y, 0.36), (0.04, 0.04, 0.72), m.dark)
    k.box((-0.32, 0, 0.5), (0.02, 1.3, 0.4), m.dark)
    k.box((0, -0.65, 0.12), (0.6, 0.03, 0.03), m.dark)
    k.box((0, 0.65, 0.12), (0.6, 0.03, 0.03), m.dark)


@prop
def p_Chair(k, m):
    """Stackable classroom chair, faces +X: moulded seat and back shell on a bent tube frame."""
    k.loft([(x, 0.43 + 0.012 * (1 - abs(x) / 0.21), 0.462 + 0.008 * (1 - abs(x) / 0.21), 0.21) for x in [-0.21 + 0.42 * i / 8 for i in range(9)]], m.blue, segs=20, power=4.0)
    k.loft([(x, 0.6, 0.86, 0.2 - 0.01 * abs(x + 0.22) / 0.02) for x in (-0.235, -0.225, -0.215)], m.blue, segs=20, power=4.0)
    for y in (-0.18, 0.18):
        k.tube([(0.18, y, 0.0), (0.18, y, 0.42), (0.17, y, 0.44), (-0.17, y, 0.44), (-0.19, y, 0.42), (-0.19, y, 0.0)], 0.012, m.metal, segs=8)
        k.tube([(-0.19, y, 0.44), (-0.215, y, 0.62), (-0.225, y, 0.8)], 0.012, m.metal, segs=8)
        for x in (-0.19, 0.18):
            k.cyl((x, y, 0.0), 0.016, 0.015, m.rubber, verts=8)
    k.tube([(0.18, -0.18, 0.14), (0.18, 0.18, 0.14)], 0.009, m.metal, segs=6)

@prop
def p_OfficeChair(k, m):
    """Office chair: five-star base with casters, gas lift, padded seat and back, armrests."""
    for i in range(5):
        a = math.radians(i * 72)
        c, sn = math.cos(a), math.sin(a)
        k.tube([(0.0, 0.0, 0.1), (c * 0.15, sn * 0.15, 0.085), (c * 0.3, sn * 0.3, 0.07)], 0.022, m.plastic, segs=8)
        k.cyl((c * 0.3, sn * 0.3, 0.0), 0.03, 0.05, m.rubber, verts=10)
        k.tube([(c * 0.3, sn * 0.3, 0.05), (c * 0.3, sn * 0.3, 0.075)], 0.01, m.metal, segs=6)
    k.cyl((0, 0, 0.08), 0.04, 0.06, m.plastic, verts=14)
    k.cyl((0, 0, 0.14), 0.025, 0.28, m.metal, verts=14)
    k.cyl((0, 0, 0.4), 0.045, 0.04, m.plastic, verts=14)
    k.loft([(x, 0.44, 0.53 + 0.01 * (1 - abs(x) / 0.25), 0.25) for x in [-0.25 + 0.5 * i / 8 for i in range(9)]], m.fabric, segs=24, power=3.0)
    k.tube([(-0.2, 0.0, 0.46), (-0.28, 0.0, 0.5), (-0.29, 0.0, 0.62)], 0.025, m.plastic, segs=8)
    k.loft([(x, 0.6, 1.12, 0.23 - 0.03 * abs(x + 0.3) / 0.05) for x in (-0.33, -0.3, -0.27)], m.fabric, segs=24, power=3.2)
    for y in (-0.27, 0.27):
        k.tube([(-0.05, y * 0.85, 0.5), (-0.02, y, 0.6), (0.02, y, 0.66)], 0.015, m.plastic, segs=6)
        k.loft([(-0.12, 0.65, 0.69, 0.03), (0.0, 0.65, 0.69, 0.03), (0.12, 0.65, 0.685, 0.03)], m.plastic, segs=14, power=3.0, y0=y)


@prop
def p_PC(k, m):
    """Workstation used as a terminal: desk, two monitors, keyboard, mouse, tower and office chair."""
    k.box((0, 0, 0.73), (0.8, 1.6, 0.03), m.white)
    for y in (-0.75, 0.75):
        k.box((0, y, 0.36), (0.7, 0.04, 0.72), m.dark)
    for y, yaw in ((-0.28, 12), (0.28, -12)):
        k.box((-0.22, y, 0.76), (0.18, 0.22, 0.02), m.plastic)
        k.box((-0.24, y, 0.88), (0.05, 0.05, 0.22), m.plastic)
        k.box((-0.2, y, 1.06), (0.03, 0.55, 0.34), m.plastic, rot=(0, 0, yaw))
        k.box((-0.183, y, 1.06), (0.005, 0.52, 0.3), m.screen_on, rot=(0, 0, yaw))
    k.box((0.12, 0, 0.755), (0.16, 0.45, 0.02), m.plastic)
    for i in range(5):
        k.box((0.12, -0.2 + i * 0.1, 0.767), (0.12, 0.08, 0.004), m.dark)
    k.box((0.12, 0.35, 0.755), (0.1, 0.06, 0.02), m.plastic)
    k.box((-0.1, 0.62, 0.25), (0.45, 0.2, 0.45), m.plastic)
    k.box((0.126, 0.62, 0.35), (0.004, 0.02, 0.02), m.screen_on)
    k.box((-0.3, -0.6, 0.79), (0.08, 0.08, 0.1), m.white)


@prop
def p_Printer3D(k, m):
    """3D printer on a rolling cart, front +X, with a half-printed part and a filament spool."""
    k.box((0, 0, 0.42), (0.6, 0.7, 0.04), m.dark)
    k.box((0, 0, 0.08), (0.6, 0.7, 0.04), m.dark)
    for x in (-0.27, 0.27):
        for y in (-0.32, 0.32):
            k.box((x, y, 0.25), (0.04, 0.04, 0.4), m.metal)
            k.cyl((x, y, 0.0), 0.035, 0.05, m.rubber, verts=10)
    # printer frame
    for x in (-0.22, 0.22):
        for y in (-0.22, 0.22):
            k.box((x, y, 0.75), (0.04, 0.04, 0.62), m.dark)
    k.box((0, 0, 1.07), (0.48, 0.48, 0.04), m.dark)
    k.box((0, 0, 0.47), (0.48, 0.48, 0.06), m.dark)
    k.box((0, 0, 0.56), (0.36, 0.36, 0.01), m.glass)
    k.box((0, 0, 0.8), (0.04, 0.46, 0.03), m.metal)
    k.box((0, 0.05, 0.76), (0.08, 0.06, 0.08), m.orange)
    k.box((0, 0, 0.6), (0.12, 0.12, 0.06), m.orange)
    k.box((0.245, 0.15, 0.6), (0.01, 0.12, 0.08), m.screen_on)
    k.cyl_axis((-0.15, 0, 1.2), 0.1, 0.07, m.orange, axis="Y", verts=20)
    k.box((0.2, -0.15, 0.48), (0.03, 0.1, 0.02), m.white)


@prop
def p_SecurityCamera(k, m):
    """Wall camera: bracket at the origin, camera body pointing +X."""
    k.box((-0.05, 0, 0.0), (0.1, 0.12, 0.12), m.white)
    k.box((0.05, 0, 0.0), (0.12, 0.03, 0.03), m.white)
    k.box((0.2, 0, -0.03), (0.28, 0.12, 0.11), m.white)
    k.box((0.2, 0, 0.04), (0.32, 0.16, 0.02), m.white)
    k.cyl_axis((0.345, 0, -0.03), 0.035, 0.01, m.screen, axis="X", verts=16)


@prop
def p_Locker(k, m):
    """Row of three lockers against a wall (back at -X): doors with vent slots, handles, padlock, legs, number plates."""
    for i in range(3):
        y = -0.4 + i * 0.4
        k.box((-0.01, y, 1.0), (0.48, 0.39, 1.8), m.dark)
        k.box((0.235, y, 1.0), (0.012, 0.37, 1.76), m.blue if i != 1 else m.green)
        for z in (1.55, 1.6, 1.65, 1.7, 0.3, 0.35, 0.4):
            k.box((0.243, y, z), (0.004, 0.22, 0.012), m.dark)
        k.box((0.248, y + 0.14, 1.0), (0.02, 0.03, 0.16), m.metal)
        k.box((0.252, y + 0.14, 0.97), (0.02, 0.025, 0.04), m.yellow)
        k.box((0.243, y, 1.78), (0.004, 0.08, 0.04), m.white)
        for yy in (-0.17, 0.17):
            k.box((0.0, y + yy, 0.05), (0.04, 0.04, 0.1), m.dark)
    k.box((0, 0, 1.92), (0.52, 1.22, 0.04), m.dark)


@prop
def p_Bookshelf(k, m):
    k.box((0, -0.5, 1.0), (0.35, 0.03, 2.0), m.wood)
    k.box((0, 0.5, 1.0), (0.35, 0.03, 2.0), m.wood)
    k.box((-0.17, 0, 1.0), (0.02, 1.0, 2.0), m.wood)
    colors = [m.red, m.blue, m.green, m.yellow, m.paper, m.dark, m.orange]
    for j, z in enumerate((0.02, 0.42, 0.82, 1.22, 1.62, 1.99)):
        k.box((0, 0, z), (0.35, 1.0, 0.03), m.wood)
        if z > 1.9:
            continue
        y = -0.46
        i = j * 3
        while y < 0.44:
            w = 0.03 + ((i * 7) % 5) * 0.008
            h = 0.25 + ((i * 3) % 4) * 0.025
            k.box((0.0, y + w / 2, z + 0.015 + h / 2), (0.24, w, h), colors[i % len(colors)])
            y += w + 0.004
            i += 1


@prop
def p_Whiteboard(k, m):
    """Wall whiteboard, board surface facing +X, bottom at 0.9 m."""
    k.box((0, 0, 1.5), (0.03, 2.4, 1.2), m.white)
    k.box((0.016, 0, 1.5), (0.002, 2.3, 1.1), m.paper)
    k.box((0.05, 0, 0.9), (0.08, 2.4, 0.02), m.metal)
    for i, c in enumerate((m.blue, m.red, m.dark)):
        k.cyl_axis((0.05, -0.3 + i * 0.12, 0.915), 0.008, 0.12, c, axis="Y", verts=8)
    # some scribbles: an equation, a graph
    k.box((0.018, -0.6, 1.75), (0.002, 0.7, 0.02), m.blue)
    k.box((0.018, -0.7, 1.6), (0.002, 0.5, 0.02), m.blue)
    k.box((0.018, 0.6, 1.4), (0.002, 0.02, 0.6), m.dark)
    k.box((0.018, 0.85, 1.11), (0.002, 0.5, 0.02), m.dark)
    k.box((0.018, 0.8, 1.4), (0.002, 0.45, 0.02), m.red, rot=(35, 0, 0))


@prop
def p_Plant(k, m):
    k.cyl((0, 0, 0), 0.22, 0.45, m.white, verts=16, r_top=0.26)
    k.cyl((0, 0, 0.42), 0.24, 0.02, m.soil, verts=16)
    k.cyl((0, 0, 0.42), 0.025, 0.7, m.wood, verts=6)
    for i in range(14):
        a = i * 2.4
        z = 0.7 + (i % 5) * 0.12
        r = 0.18 + (i % 3) * 0.08
        k.box((math.cos(a) * r, math.sin(a) * r, z), (0.32, 0.12, 0.02), m.leaf, rot=(0, -25 - (i % 3) * 10, math.degrees(a)))


@prop
def p_VendingMachine(k, m):
    k.box((0, 0, 0.95), (0.8, 1.0, 1.9), m.red)
    k.box((0.405, -0.1, 1.1), (0.01, 0.7, 1.3), m.glass)
    for r in range(5):
        for c in range(5):
            col = (m.blue, m.yellow, m.green, m.orange, m.white)[(r + c) % 5]
            k.box((0.3, -0.38 + c * 0.14, 0.6 + r * 0.25), (0.12, 0.08, 0.16), col)
    k.box((0.405, 0.38, 1.3), (0.01, 0.15, 0.3), m.dark)
    k.box((0.41, 0.38, 1.0), (0.01, 0.1, 0.1), m.screen_on)
    k.box((0.405, -0.1, 0.25), (0.02, 0.6, 0.2), m.dark)


@prop
def p_CoffeeMachine(k, m):
    """Counter with a coffee machine and cups."""
    k.box((0, 0, 0.45), (0.6, 1.4, 0.9), m.lwood)
    k.box((0, 0, 0.915), (0.62, 1.42, 0.03), m.dark)
    k.box((-0.05, -0.3, 1.18), (0.4, 0.32, 0.5), m.dark)
    k.box((0.16, -0.3, 1.1), (0.04, 0.12, 0.08), m.metal)
    k.box((0.155, -0.3, 1.33), (0.01, 0.2, 0.08), m.screen_on)
    for i in range(4):
        k.cyl((0.05, 0.1 + i * 0.1, 0.93), 0.035, 0.1, m.white, verts=12)
    k.box((0, 0.5, 0.98), (0.3, 0.2, 0.12), m.paper)


@prop
def p_Sofa(k, m):
    """Lounge sofa: soft rounded base, back, arms and three seat cushions, wooden feet."""
    k.loft([(x, 0.08, 0.36, 0.95) for x in [-0.42 + 0.84 * i / 6 for i in range(7)]], m.fabric_red, segs=24, power=5.0)
    k.loft([(x, 0.3, 0.9 - 0.06 * (x + 0.42) / 0.2, 0.95) for x in [-0.42 + 0.2 * i / 4 for i in range(5)]], m.fabric_red, segs=24, power=4.0)
    for y in (-0.87, 0.87):
        k.loft([(-0.42, 0.3, 0.62, 0.09), (0.0, 0.3, 0.64, 0.09), (0.42, 0.3, 0.6, 0.09)], m.fabric_red, segs=20, power=3.0, y0=y)
    for y in (-0.52, 0.0, 0.52):
        k.loft([(x, 0.35, 0.47 + 0.015 * (1 - abs(x - 0.03) / 0.34), 0.25) for x in [-0.3 + 0.64 * i / 6 for i in range(7)]], m.fabric_red, segs=20, power=3.0, y0=y)
    for x in (-0.35, 0.35):
        for y in (-0.85, 0.85):
            k.cyl((x, y, 0.0), 0.025, 0.08, m.wood, verts=10, r_top=0.03)


@prop
def p_TrashBin(k, m):
    k.cyl((0, 0, 0), 0.2, 0.7, m.dark, verts=16, r_top=0.22)
    k.cyl((0, 0, 0.7), 0.23, 0.04, m.metal, verts=16)
    k.box((0.21, 0, 0.5), (0.01, 0.15, 0.08), m.green)


@prop
def p_FireExtinguisher(k, m):
    k.box((-0.04, 0, 0.9), (0.02, 0.2, 0.3), m.dark)
    k.cyl((0.06, 0, 0.6), 0.075, 0.5, m.red, verts=16)
    k.cyl((0.06, 0, 1.1), 0.075, 0.06, m.red, verts=16, r_top=0.03)
    k.box((0.06, 0, 1.18), (0.12, 0.03, 0.03), m.dark)
    k.cyl_axis((0.15, 0, 1.12), 0.012, 0.12, m.rubber, axis="X", verts=8)
    k.box((0.137, 0, 0.8), (0.004, 0.1, 0.12), m.paper)


@prop
def p_FilingCabinet(k, m):
    k.box((0, 0, 0.66), (0.6, 0.47, 1.32), m.metal)
    for i in range(4):
        z = 0.17 + i * 0.32
        k.box((0.302, 0, z), (0.006, 0.44, 0.3), m.metal)
        k.box((0.31, 0, z + 0.06), (0.02, 0.15, 0.02), m.dark)


@prop
def p_ServerRack(k, m):
    k.box((0, 0, 1.0), (1.0, 0.6, 2.0), m.dark)
    k.box((0.505, 0, 1.0), (0.01, 0.55, 1.9), m.glass)
    for i in range(16):
        z = 0.2 + i * 0.11
        k.box((0.44, 0, z), (0.1, 0.5, 0.08), m.plastic)
        for j in range(4):
            k.box((0.492, -0.18 + j * 0.06, z), (0.004, 0.015, 0.015), m.screen_on if (i + j) % 3 else m.green)


@prop
def p_LabBench(k, m):
    """Chemistry lab bench with sink, tap and glassware."""
    k.box((0, 0, 0.45), (0.9, 2.4, 0.9), m.white)
    k.box((0, 0, 0.92), (0.95, 2.45, 0.04), m.dark)
    k.box((0, 0.8, 0.9), (0.4, 0.5, 0.05), m.metal)
    k.cyl((-0.25, 0.8, 0.94), 0.015, 0.35, m.metal, verts=8)
    k.box((-0.15, 0.8, 1.28), (0.22, 0.025, 0.025), m.metal)
    k.box((-0.0, -0.4, 1.5), (0.1, 2.0, 0.03), m.metal)
    for y in (-1.1, 1.1):
        k.box((-0.0, y, 1.21), (0.03, 0.03, 0.6), m.metal)
    for i, y in enumerate((-0.9, -0.6, -0.3, 0.1)):
        k.cyl((0.15, y, 0.94), 0.05 - (i % 2) * 0.015, 0.18 + (i % 3) * 0.05, m.glass, verts=12, r_top=0.02 + (i % 2) * 0.02)
    k.box((0.2, -0.0, 0.96), (0.25, 0.3, 0.02), m.paper)


@prop
def p_Stool(k, m):
    k.cyl((0, 0, 0.66), 0.18, 0.05, m.dark, verts=16)
    for i in range(4):
        a = math.radians(45 + i * 90)
        k.box((math.cos(a) * 0.12, math.sin(a) * 0.12, 0.33), (0.025, 0.025, 0.66), m.metal, rot=(0, 0, math.degrees(a)))
    k.cyl((0, 0, 0.25), 0.14, 0.02, m.metal, verts=16, r_top=0.14)


@prop
def p_CafeTable(k, m):
    k.box((0, 0, 0.74), (0.9, 1.6, 0.04), m.white)
    for y in (-0.6, 0.6):
        k.cyl((0, y, 0), 0.25, 0.03, m.dark, verts=16)
        k.cyl((0, y, 0.03), 0.04, 0.7, m.metal, verts=12)
    k.box((0.1, -0.3, 0.77), (0.3, 0.25, 0.02), m.orange)
    k.cyl((-0.2, 0.4, 0.76), 0.04, 0.12, m.white, verts=12)


@prop
def p_LectureSeats(k, m):
    """Row of 6 auditorium seats with a writing table in front (+X)."""
    for i in range(6):
        y = -1.5 + i * 0.6
        k.box((0, y, 0.45), (0.45, 0.5, 0.08), m.fabric_red)
        k.box((-0.22, y, 0.8), (0.08, 0.5, 0.6), m.fabric_red, rot=(0, -10, 0))
    for y in (-1.8, -0.6, 0.6, 1.8):
        k.box((0, y, 0.35), (0.5, 0.05, 0.7), m.dark)
    k.box((0.45, 0, 0.78), (0.35, 3.6, 0.03), m.lwood)
    k.box((0.6, 0, 0.4), (0.03, 3.6, 0.78), m.lwood)


@prop
def p_Lectern(k, m):
    k.box((0, 0, 0.55), (0.5, 0.7, 1.1), m.wood)
    k.box((0.05, 0, 1.15), (0.5, 0.75, 0.04), m.wood, rot=(0, -15, 0))
    k.box((0.26, 0, 0.7), (0.01, 0.4, 0.15), m.ntnu)
    k.cyl((0.1, 0.2, 1.17), 0.008, 0.3, m.dark, verts=6)


@prop
def p_Projector(k, m):
    """Ceiling screen: hangs from z = 3 m down."""
    k.box((0, 0, 2.95), (0.12, 3.2, 0.12), m.white)
    k.box((0, 0, 2.05), (0.01, 3.0, 1.7), m.white)
    k.box((0.006, 0, 2.05), (0.002, 2.9, 1.6), m.screen_on)


# ------------------------------------------------------------------ outdoor
@prop
def p_ParkBench(k, m):
    for i in range(4):
        k.box((0.05 - i * 0.11, 0, 0.45), (0.09, 1.8, 0.04), m.wood)
    for i in range(3):
        k.box((-0.33, 0, 0.62 + i * 0.12), (0.03, 1.8, 0.09), m.wood, rot=(0, -12, 0))
    for y in (-0.75, 0.75):
        k.box((-0.1, y, 0.22), (0.5, 0.06, 0.06), m.dark)
        k.box((0.12, y, 0.22), (0.06, 0.06, 0.45), m.dark)
        k.box((-0.3, y, 0.4), (0.06, 0.06, 0.8), m.dark, rot=(0, -12, 0))
        k.box((0.0, y, 0.62), (0.45, 0.05, 0.04), m.dark)


@prop
def p_BikeRack(k, m):
    """Rack with 5 bikes."""
    for i in range(6):
        y = -1.5 + i * 0.6
        k.box((0, y, 0.38), (0.9, 0.04, 0.04), m.metal)
        for x in (-0.45, 0.45):
            k.box((x, y, 0.19), (0.04, 0.04, 0.38), m.metal)
    k.box((0, 0, 0.02), (1.0, 3.2, 0.04), m.concrete)
    for i in range(5):
        y = -1.2 + i * 0.6
        c = (m.blue, m.red, m.dark, m.green, m.yellow)[i]
        for x in (-0.5, 0.5):
            k.cyl_axis((x, y, 0.35), 0.33, 0.04, m.rubber, axis="Y", verts=20)
            k.cyl_axis((x, y, 0.35), 0.29, 0.045, m.metal, axis="Y", verts=20)
        k.ramp((-0.5, y, 0.35), (0.0, y, 0.85), 0.03, 0.04, c)
        k.ramp((0.0, y, 0.85), (0.5, y, 0.35), 0.03, 0.04, c)
        k.box((0.05, y, 0.55), (0.8, 0.03, 0.04), c)
        k.box((-0.1, y, 0.95), (0.25, 0.1, 0.05), m.rubber)
        k.box((0.45, y, 1.0), (0.05, 0.6, 0.03), m.dark)


@prop
def p_Bike(k, m):
    """City bike: tyres, rims and spokes, diamond frame, fork, handlebar, saddle, cranks, fenders and a basket."""
    W = 0.33
    for x in (-0.52, 0.52):
        k.ring_y((x, 0, 0.35), W, 0.022, m.rubber, n=32, segs=8)
        k.ring_y((x, 0, 0.35), W - 0.03, 0.01, m.metal, n=32, segs=6)
        for i in range(16):
            a = math.pi * i / 8
            k.tube([(x, 0, 0.35), (x + math.cos(a) * (W - 0.03), 0, 0.35 + math.sin(a) * (W - 0.03))], 0.0018, m.metal, segs=4)
        k.cyl_axis((x, 0, 0.35), 0.025, 0.07, m.dark, axis="Y", verts=12)
        # fender over the top of each wheel
        k.tube([(x + 0.36 * math.cos(a), 0, 0.35 + 0.36 * math.sin(a)) for a in [math.radians(d) for d in range(20, 161, 20)]], 0.018, m.red, segs=6)
    hub_r, bb, seat, head_t, head_b = (-0.52, 0, 0.35), (-0.02, 0, 0.32), (-0.14, 0, 0.88), (0.38, 0, 0.9), (0.42, 0, 0.72)
    for a, b in ((bb, seat), (seat, head_t), (bb, head_b), (head_b, head_t), (hub_r, bb), (hub_r, seat)):
        k.tube([a, b], 0.018, m.red, segs=8)
    for y in (-0.04, 0.04):
        k.tube([(0.4, y, 0.78), (0.47, y, 0.55), (0.52, y, 0.35)], 0.012, m.metal, segs=6)
    k.tube([(0.38, 0, 0.9), (0.36, 0, 1.02)], 0.016, m.metal, segs=8)
    k.tube([(0.3, -0.28, 1.06), (0.34, -0.2, 1.03), (0.36, 0, 1.02), (0.34, 0.2, 1.03), (0.3, 0.28, 1.06)], 0.011, m.metal, segs=6)
    for y in (-0.28, 0.28):
        k.cyl_axis((0.3, y, 1.06), 0.016, 0.1, m.rubber, axis="Y", verts=10)
    k.tube([(-0.14, 0, 0.88), (-0.16, 0, 0.98)], 0.014, m.metal, segs=8)
    k.box((-0.15, 0, 1.0), (0.26, 0.1, 0.05), m.rubber)
    k.box((-0.04, 0, 1.0), (0.06, 0.05, 0.04), m.rubber)
    k.cyl_axis((-0.02, 0.03, 0.32), 0.09, 0.008, m.dark, axis="Y", verts=24)
    for s_ in (-1, 1):
        k.tube([(-0.02, s_ * 0.05, 0.32), (-0.02 + 0.08 * s_, s_ * 0.06, 0.32 - 0.14 * s_)], 0.008, m.metal, segs=6)
        k.box((-0.02 + 0.08 * s_, s_ * 0.1, 0.32 - 0.14 * s_), (0.1, 0.08, 0.02), m.rubber)
    k.tube([(-0.02, 0.04, 0.32), (-0.52, 0.04, 0.35)], 0.004, m.dark, segs=4)
    k.tube([(-0.02, 0.04, 0.23), (-0.52, 0.04, 0.3)], 0.004, m.dark, segs=4)
    # front basket
    k.box((0.48, 0, 0.98), (0.26, 0.34, 0.02), m.dark)
    for x, y in ((0.36, -0.17), (0.6, -0.17), (0.36, 0.17), (0.6, 0.17)):
        k.tube([(x, y, 0.98), (x, y, 1.14)], 0.006, m.dark, segs=4)
    for z in (1.06, 1.14):
        k.tube([(0.36, -0.17, z), (0.6, -0.17, z), (0.6, 0.17, z), (0.36, 0.17, z)], 0.005, m.dark, segs=4, closed=True)
    k.cyl_axis((-0.15, 0.0, 0.62), 0.006, 0.01, m.dark, axis="Y", verts=6)


@prop
def p_Scooter(k, m):
    """Rental e-scooter (Trondheim has thousands)."""
    k.box((0, 0, 0.12), (0.8, 0.18, 0.06), m.dark)
    for x in (-0.42, 0.42):
        k.cyl_axis((x, 0, 0.12), 0.12, 0.06, m.rubber, axis="Y", verts=16)
    k.box((0.42, 0, 0.6), (0.04, 0.05, 1.0), m.green, rot=(0, 10, 0))
    k.box((0.5, 0, 1.1), (0.05, 0.55, 0.04), m.dark)
    k.box((0.45, 0, 0.75), (0.02, 0.12, 0.25), m.green)


@prop
def p_OutdoorBin(k, m):
    k.box((0, 0, 0.5), (0.5, 0.5, 1.0), m.green)
    k.cyl((0, 0, 1.0), 0.3, 0.12, m.green, verts=4, rot_z=45)
    k.box((0.255, 0, 0.85), (0.02, 0.25, 0.1), m.dark)
    k.box((0, 0, 0.02), (0.6, 0.6, 0.04), m.concrete)


@prop
def p_NTNUSign(k, m):
    k.box((0, 0, 0.3), (0.4, 2.6, 0.6), m.concrete)
    k.box((0, 0, 1.2), (0.12, 2.4, 1.2), m.ntnu)
    for i in range(4):  # "NTNU" letters as blocks
        y = -0.8 + i * 0.53
        k.box((0.065, y, 1.25), (0.02, 0.32, 0.6), m.white)
        k.box((0.07, y, 1.25), (0.02, 0.14, 0.45), m.ntnu)
    k.box((0.065, 0, 0.75), (0.02, 2.0, 0.08), m.white)


@prop
def p_Bollard(k, m):
    k.cyl((0, 0, 0), 0.1, 0.9, m.dark, verts=12)
    k.cyl((0, 0, 0.75), 0.105, 0.06, m.yellow, verts=12)


@prop
def p_Planter(k, m):
    k.box((0, 0, 0.3), (1.2, 1.2, 0.6), m.concrete)
    k.box((0, 0, 0.58), (1.05, 1.05, 0.04), m.soil)
    for i in range(9):
        a = i * 0.7
        k.cyl((math.cos(a) * 0.3, math.sin(a) * 0.3, 0.6), 0.18, 0.35 + (i % 3) * 0.1, m.leaf, verts=8, r_top=0.05)


@prop
def p_Barrier(k, m):
    k.prism_y(-1.0, 1.0, [(-0.3, 0.0), (0.3, 0.0), (0.12, 0.25), (0.08, 0.8), (-0.08, 0.8), (-0.12, 0.25)], m.concrete)
    k.box((0, -0.6, 0.6), (0.17, 0.3, 0.06), m.red)
    k.box((0, 0.6, 0.6), (0.17, 0.3, 0.06), m.white)


@prop
def p_Pallet(k, m):
    for y in (-0.5, 0.0, 0.5):
        k.box((0, y, 0.05), (1.2, 0.1, 0.1), m.lwood)
    for i in range(7):
        k.box((-0.55 + i * 0.183, 0, 0.12), (0.1, 1.1, 0.025), m.lwood)
    k.box((0, 0, 0.5), (1.0, 0.9, 0.7), m.paper)
    k.box((0, 0, 0.85), (1.01, 0.12, 0.01), m.orange)


@prop
def p_Crates(k, m):
    for (x, y, z, s) in ((0, 0, 0, 0.8), (0.05, 0.8, 0, 0.7), (0.0, 0.3, 0.8, 0.6)):
        k.box((x, y, z + s / 2), (s, s, s), m.wood)
        for a in (-1, 1):
            k.box((x + a * s / 2, y, z + s / 2), (0.02, s * 0.95, 0.1), m.lwood)


@prop
def p_Dumpster(k, m):
    k.box((0, 0, 0.65), (1.4, 2.0, 1.1), m.green)
    k.box((0, 0, 1.22), (1.45, 2.05, 0.06), m.dark, rot=(0, -6, 0))
    for x in (-0.55, 0.55):
        for y in (-0.85, 0.85):
            k.cyl((x, y, 0.0), 0.07, 0.1, m.rubber, verts=10)
    k.box((0.71, 0, 0.9), (0.03, 1.6, 0.05), m.metal)


@prop
def p_PicnicTable(k, m):
    k.box((0, 0, 0.75), (0.8, 2.0, 0.05), m.wood)
    for x in (-0.6, 0.6):
        k.box((x, 0, 0.45), (0.3, 2.0, 0.04), m.wood)
    for y in (-0.8, 0.8):
        for a in (-1, 1):
            k.box((a * 0.3, y, 0.37), (0.06, 0.06, 0.85), m.wood, rot=(0, a * 30, 0))
        k.box((0, y, 0.42), (1.5, 0.06, 0.05), m.wood)


@prop
def p_BusStop(k, m):
    k.box((0, 0, 0.02), (2.0, 4.0, 0.04), m.concrete)
    k.box((-0.9, 0, 1.25), (0.04, 3.8, 2.3), m.glass)
    for y in (-1.9, 1.9):
        k.box((-0.9, y, 1.25), (0.08, 0.08, 2.5), m.dark)
        k.box((0.0, y, 1.25), (1.8, 0.04, 2.3), m.glass)
    k.box((-0.2, 0, 2.52), (1.9, 4.1, 0.08), m.dark)
    k.box((-0.6, 0, 0.45), (0.4, 2.5, 0.05), m.wood)
    k.box((0.9, 2.1, 1.4), (0.06, 0.06, 2.8), m.metal)
    k.box((0.9, 2.1, 2.7), (0.04, 0.5, 0.4), m.yellow)
    k.box((-0.88, -1.0, 1.3), (0.01, 1.0, 1.4), m.screen_on)


@prop
def p_Fence(k, m):
    for y in (-1.5, 0.0, 1.5):
        k.box((0, y, 0.6), (0.06, 0.06, 1.2), m.dark)
    for z in (0.3, 1.0):
        k.box((0, 0, z), (0.03, 3.0, 0.04), m.dark)
    for i in range(30):
        k.box((0, -1.45 + i * 0.1, 0.65), (0.015, 0.015, 0.75), m.dark)


@prop
def p_StreetSign(k, m):
    k.cyl((0, 0, 0), 0.04, 2.6, m.metal, verts=8)
    k.box((0.04, 0, 2.3), (0.02, 0.6, 0.6), m.blue, rot=(45, 0, 0))
    k.box((0.05, 0, 2.3), (0.01, 0.45, 0.45), m.white, rot=(45, 0, 0))
    k.box((0.04, 0, 1.7), (0.02, 0.9, 0.25), m.white)


@prop
def p_StreetLamp(k, m):
    """6 m street lamp: tapered pole on a concrete foot, curved arm, flat LED head over +X."""
    k.cyl((0, 0, 0), 0.25, 0.3, m.concrete, verts=12)
    k.cyl((0, 0, 0.3), 0.09, 5.6, m.dark, verts=12, r_top=0.055)
    k.cyl((0, 0, 0.3), 0.12, 0.6, m.dark, verts=12, r_top=0.1)
    k.ramp((0, 0, 5.8), (0.6, 0, 6.05), 0.07, 0.07, m.dark)
    k.ramp((0.6, 0, 6.05), (1.2, 0, 6.1), 0.07, 0.07, m.dark)
    k.box((1.45, 0, 6.08), (0.7, 0.3, 0.1), m.dark)
    k.box((1.45, 0, 6.02), (0.6, 0.24, 0.02), m.white)
    k.box((0.0, 0, 2.6), (0.03, 0.2, 0.3), m.paper)


@prop
def p_Bollard_Light(k, m):
    k.cyl((0, 0, 0), 0.1, 0.9, m.dark, verts=12)
    k.cyl((0, 0, 0.7), 0.105, 0.15, m.white, verts=12)


# ------------------------------------------------------------------ robots (static shells; Isaac Lab animation later)
@prop
def p_RobotDog(k, m):
    """Quadruped robot dog, about 0.7 m long, facing +X."""
    body = k.mat("RobotBody", (0.9, 0.75, 0.1), metallic=0.3, rough=0.4)
    k.box((0, 0, 0.45), (0.65, 0.26, 0.16), body)
    k.box((0, 0, 0.535), (0.5, 0.2, 0.03), m.dark)
    k.box((0.36, 0, 0.47), (0.12, 0.2, 0.12), m.dark)
    for y in (-0.06, 0.06):
        k.cyl_axis((0.425, y, 0.48), 0.025, 0.01, m.screen, axis="X", verts=12)
    for x in (-0.24, 0.24):
        for y in (-0.15, 0.15):
            k.cyl_axis((x, y, 0.42), 0.055, 0.06, m.dark, axis="Y", verts=12)
            k.ramp((x, y * 1.1, 0.42), (x + 0.06, y * 1.1, 0.24), 0.06, 0.05, body)
            k.ramp((x + 0.06, y * 1.1, 0.24), (x - 0.02, y * 1.1, 0.03), 0.04, 0.04, m.dark)
            k.cyl((x - 0.02, y * 1.1, 0.0), 0.03, 0.04, m.rubber, verts=10)


@prop
def p_RobotHumanoid(k, m):
    """Humanoid robot, 1.75 m."""
    shell = k.mat("RobotWhite", (0.88, 0.88, 0.9), rough=0.35)
    for y in (-0.11, 0.11):
        k.box((0.02, y, 0.05), (0.24, 0.1, 0.06), m.dark)
        k.cyl((0, y, 0.08), 0.06, 0.42, shell, verts=12, r_top=0.055)
        k.cyl((0, y, 0.5), 0.065, 0.06, m.dark, verts=12)
        k.cyl((0, y, 0.55), 0.075, 0.38, shell, verts=12, r_top=0.08)
    k.box((0, 0, 0.98), (0.22, 0.32, 0.12), m.dark)
    k.box((0, 0, 1.25), (0.26, 0.42, 0.42), shell)
    k.box((0.13, 0, 1.3), (0.01, 0.2, 0.12), m.screen_on)
    for y in (-0.25, 0.25):
        k.cyl((0, y, 1.38), 0.06, 0.08, m.dark, verts=12)
        k.cyl((0, y, 1.08), 0.05, 0.3, shell, verts=12)
        k.cyl((0, y, 0.82), 0.045, 0.26, shell, verts=12)
        k.box((0.02, y, 0.78), (0.08, 0.05, 0.1), m.dark)
    k.cyl((0, 0, 1.46), 0.05, 0.06, m.dark, verts=12)
    k.box((0, 0, 1.62), (0.22, 0.2, 0.24), shell)
    k.box((0.112, 0, 1.64), (0.005, 0.17, 0.07), m.screen)
    k.box((0.115, 0, 1.64), (0.003, 0.12, 0.015), m.screen_on)


@prop
def p_RobotGuard(k, m):
    """Wheeled security robot (egg shape with a camera dome and lights)."""
    shell = k.mat("RobotWhite", (0.88, 0.88, 0.9), rough=0.35)
    for x in (-0.25, 0.25):
        for y in (-0.28, 0.28):
            k.cyl_axis((x, y, 0.13), 0.13, 0.08, m.rubber, axis="Y", verts=16)
    k.cyl((0, 0, 0.12), 0.4, 0.7, shell, verts=24, r_top=0.32)
    k.cyl((0, 0, 0.82), 0.32, 0.35, shell, verts=24, r_top=0.12)
    k.cyl((0, 0, 0.5), 0.405, 0.06, m.ntnu, verts=24)
    k.cyl((0, 0, 1.17), 0.12, 0.08, m.dark, verts=16)
    k.cyl((0, 0, 1.25), 0.1, 0.1, m.screen, verts=16, r_top=0.0)
    k.box((0.38, 0, 0.75), (0.02, 0.3, 0.06), m.screen_on)
    k.box((0, 0, 0.92), (0.66, 0.05, 0.03), m.red)


@prop
def p_RobotDelivery(k, m):
    """Six-wheeled sidewalk delivery robot with a flag."""
    shell = k.mat("RobotWhite", (0.88, 0.88, 0.9), rough=0.35)
    for x in (-0.25, 0.0, 0.25):
        for y in (-0.25, 0.25):
            k.cyl_axis((x, y, 0.1), 0.1, 0.06, m.rubber, axis="Y", verts=14)
    k.box((0, 0, 0.32), (0.7, 0.5, 0.34), shell)
    k.box((0, 0, 0.51), (0.66, 0.46, 0.05), m.dark)
    k.box((0.355, 0, 0.32), (0.01, 0.3, 0.08), m.screen_on)
    k.cyl((-0.3, 0.2, 0.5), 0.008, 0.9, m.metal, verts=6)
    k.box((-0.3, 0.27, 1.32), (0.01, 0.14, 0.1), m.orange)


def build(name):
    k = MeshKit(f"SM_Prop_{name}")
    PROPS[name](k, Mats(k))
    k.finish()


if __name__ == "__main__":
    names = [a for a in sys.argv[1:] if a in PROPS] or list(PROPS)
    for n in names:
        build(n)
        print("built", n)
    print(len(names), "props")
