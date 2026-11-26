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
    """Student desk: wooden top with soft edges, steel tube legs and rails, book shelf under the top."""
    k.loft([(x, 0.715, 0.745, 0.7) for x in [-0.35 + 0.7 * i / 6 for i in range(7)]], m.lwood, segs=24, power=8.0)
    for x in (-0.3, 0.3):
        for y in (-0.64, 0.64):
            k.tube([(x, y, 0.0), (x, y, 0.715)], 0.016, m.dark, segs=10)
            k.cyl((x, y, 0.0), 0.02, 0.01, m.rubber, verts=10)
    for y in (-0.64, 0.64):
        k.tube([(-0.3, y, 0.12), (0.3, y, 0.12)], 0.012, m.dark, segs=8)
    k.tube([(-0.3, -0.64, 0.68), (-0.3, 0.64, 0.68)], 0.012, m.dark, segs=8)
    k.box((-0.32, 0, 0.5), (0.012, 1.24, 0.34), m.dark)
    k.box((0.0, 0, 0.6), (0.55, 1.24, 0.012), m.dark)

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
    """Workstation used as a terminal: desk, two slim monitors on stands, keyboard, mouse, tower, mug."""
    k.loft([(x, 0.715, 0.745, 0.8) for x in [-0.4 + 0.8 * i / 6 for i in range(7)]], m.white, segs=24, power=8.0)
    for y in (-0.75, 0.75):
        k.box((0, y, 0.36), (0.7, 0.04, 0.72), m.dark)
        k.box((0, y, 0.02), (0.72, 0.06, 0.04), m.rubber)
    k.box((-0.33, 0, 0.45), (0.02, 1.46, 0.4), m.dark)
    for y, yaw in ((-0.29, 14), (0.29, -14)):
        k.cyl((-0.24, y, 0.745), 0.11, 0.012, m.dark, verts=24)
        k.tube([(-0.25, y, 0.75), (-0.26, y, 0.92), (-0.235, y, 1.0)], 0.02, m.dark, segs=10)
        mat = math.radians(yaw)
        c, s = math.cos(mat), math.sin(mat)
        # thin bezel: rounded back shell with a screen inset
        k.box((-0.215, y, 1.07), (0.035, 0.56, 0.34), m.plastic, rot=(0, 0, yaw))
        k.box((-0.197, y, 1.072), (0.004, 0.54, 0.31), m.screen_on, rot=(0, 0, yaw))
        k.box((-0.198 + 0.0, y + 0.0, 0.905), (0.006, 0.08, 0.008), m.dark, rot=(0, 0, yaw))
    # keyboard with key rows, mouse and pad
    k.loft([(0.07, 0.745, 0.755, 0.23), (0.17, 0.745, 0.767, 0.23)], m.plastic, segs=16, power=8.0)
    for r in range(5):
        for i in range(14):
            k.box((0.085 + r * 0.019, -0.205 + i * 0.0315, 0.758 + r * 0.0022), (0.016, 0.027, 0.008), m.dark)
    k.box((0.13, 0.36, 0.746), (0.22, 0.24, 0.003), m.rubber)
    k.sphere((0.13, 0.36, 0.756), 0.032, m.plastic, n=8, segs=14, scale=(1.0, 0.65, 0.4))
    k.tube([(0.098, 0.36, 0.752), (-0.05, 0.33, 0.75), (-0.3, 0.3, 0.75)], 0.003, m.dark, segs=4)
    # tower with front panel, vents and power light
    k.box((-0.1, 0.6, 0.255), (0.45, 0.21, 0.45), m.plastic)
    k.box((0.126, 0.6, 0.255), (0.004, 0.19, 0.43), m.dark)
    for i in range(8):
        k.box((0.129, 0.6, 0.1 + i * 0.03), (0.002, 0.14, 0.008), m.plastic)
    k.cyl_axis((0.13, 0.6, 0.44), 0.008, 0.004, m.screen_on, axis="X", verts=10)
    for x in (-0.28, 0.08):
        for y in (0.52, 0.68):
            k.box((x, y, 0.015), (0.04, 0.03, 0.03), m.rubber)
    # mug and a few papers
    k.cyl((-0.05, -0.6, 0.745), 0.04, 0.1, m.white, verts=18)
    k.cyl((-0.05, -0.6, 0.835), 0.034, 0.002, m.soil, verts=18)
    k.tube([(-0.05, -0.64, 0.82), (-0.05, -0.67, 0.81), (-0.05, -0.67, 0.77), (-0.05, -0.64, 0.765)], 0.006, m.white, segs=6)
    k.box((0.02, 0.62, 0.746), (0.21, 0.297, 0.003), m.paper, rot=(0, 0, 8))


@prop
def p_Printer3D(k, m):
    """3D printer on a rolling cart, front +X: enclosed printer with glass door, gantry, hotend, part on the bed,
    touch screen, filament spool on the side, cart with tube legs and casters."""
    k.loft([(x, 0.4, 0.44, 0.35) for x in [-0.3 + 0.6 * i / 4 for i in range(5)]], m.dark, segs=20, power=8.0)
    k.loft([(x, 0.06, 0.1, 0.35) for x in [-0.3 + 0.6 * i / 4 for i in range(5)]], m.dark, segs=20, power=8.0)
    for x in (-0.27, 0.27):
        for y in (-0.32, 0.32):
            k.tube([(x, y, 0.06), (x, y, 0.44)], 0.017, m.metal, segs=8)
            k.sphere((x, y, 0.03), 0.03, m.rubber)
    # enclosure: dark frame posts, glass panels, top lid
    for x in (-0.22, 0.22):
        for y in (-0.22, 0.22):
            k.box((x, y, 0.76), (0.03, 0.03, 0.64), m.dark)
    k.loft([(x, 1.06, 1.1, 0.25) for x in [-0.25 + 0.5 * i / 4 for i in range(5)]], m.dark, segs=20, power=8.0)
    k.box((0, 0, 0.47), (0.48, 0.48, 0.06), m.dark)
    k.box((0.235, 0, 0.77), (0.006, 0.42, 0.56), m.glass)
    for y in (-0.235, 0.235):
        k.box((0, y, 0.77), (0.42, 0.006, 0.56), m.glass)
    k.box((0.245, -0.17, 0.77), (0.02, 0.02, 0.18), m.metal)
    # gantry, hotend with fan, bed and a half printed NTNU part
    k.box((0, 0, 0.92), (0.4, 0.03, 0.03), m.metal)
    k.box((0.02, 0.06, 0.88), (0.07, 0.06, 0.09), m.orange)
    k.cyl_axis((0.06, 0.06, 0.88), 0.025, 0.01, m.dark, axis="X", verts=12)
    k.cyl((0.02, 0.06, 0.82), 0.008, 0.012, m.metal, verts=8, r_top=0.002)
    k.box((0, 0, 0.6), (0.36, 0.36, 0.012), m.metal)
    k.box((0, 0, 0.607), (0.34, 0.34, 0.002), m.dark)
    k.cyl((0.0, 0.06, 0.61), 0.05, 0.05, m.orange, verts=6)
    k.box((-0.06, -0.06, 0.63), (0.06, 0.06, 0.04), m.orange)
    k.box((0.245, 0.15, 0.52), (0.01, 0.14, 0.08), m.screen_on)
    # filament spool on a side holder
    k.tube([(-0.1, 0.25, 0.9), (-0.1, 0.33, 0.9)], 0.01, m.metal, segs=6)
    k.cyl_axis((-0.1, 0.3, 0.9), 0.1, 0.06, m.dark, axis="Y", verts=24)
    k.cyl_axis((-0.1, 0.3, 0.9), 0.09, 0.062, m.orange, axis="Y", verts=24)
    k.tube([(-0.1, 0.27, 0.99), (-0.05, 0.24, 1.04), (0.02, 0.06, 0.93)], 0.002, m.orange, segs=4)
    k.box((0.2, -0.15, 0.45), (0.03, 0.1, 0.02), m.white)


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
    """Wooden bookcase: sides, back, plinth and top moulding; books of varied size, some leaning, binders and a box."""
    for y in (-0.5, 0.5):
        k.box((0, y, 1.0), (0.35, 0.03, 2.0), m.wood)
    k.box((-0.17, 0, 1.0), (0.015, 1.0, 2.0), m.lwood)
    k.box((0.16, 0, 0.04), (0.02, 0.97, 0.08), m.wood)
    k.loft([(x, 1.985, 2.03, 0.53) for x in (-0.18, 0.15, 0.19)], m.wood, segs=16, power=6.0)
    colors = [m.red, m.blue, m.green, m.yellow, m.paper, m.dark, m.orange, m.ntnu, m.fabric_red]
    for j, z in enumerate((0.08, 0.46, 0.84, 1.22, 1.6)):
        k.box((0, 0, z - 0.015), (0.35, 0.97, 0.03), m.wood)
        y = -0.47
        i = j * 5 + 1
        end = 0.47 - (0.22 if j in (1, 3) else 0.0)
        while y < end - 0.06:
            w = 0.022 + ((i * 7) % 5) * 0.007
            h = 0.2 + ((i * 3) % 5) * 0.025
            d = 0.17 + ((i * 5) % 3) * 0.025
            col = colors[(i * 4) % len(colors)]
            k.box((-0.15 + d / 2, y + w / 2, z + h / 2), (d, w, h), col)
            k.box((-0.15 + d + 0.001, y + w / 2, z + h * 0.78), (0.002, w * 0.8, 0.012), m.paper)
            y += w + 0.003
            i += 1
        if j in (1, 3):
            # a few books leaning and a stack lying flat
            for t in range(2):
                k.box((-0.05, end - 0.18 + t * 0.03, z + 0.12), (0.2, 0.025, 0.24), colors[(j + t) % len(colors)], rot=(-14, 0, 0))
            for t in range(4):
                k.box((-0.04, end + 0.1, z + 0.02 + t * 0.035), (0.22 - t * 0.01, 0.16 - t * 0.008, 0.033), colors[(j * 3 + t) % len(colors)])
    # top shelf: binders and a storage box
    for i in range(5):
        k.box((-0.03, -0.44 + i * 0.07, 1.6 + 0.155), (0.26, 0.065, 0.31), m.ntnu if i % 2 else m.dark)
        k.cyl_axis((0.1, -0.44 + i * 0.07, 1.67), 0.012, 0.004, m.white, axis="X", verts=10)
    k.box((0, 0.25, 1.72), (0.28, 0.4, 0.24), m.paper)


@prop
def p_Whiteboard(k, m):
    """Wall whiteboard with an aluminium frame, marker tray, magnets, a pinned sheet and some maths on it."""
    k.box((-0.005, 0, 1.5), (0.02, 2.4, 1.2), m.white)
    k.box((0.008, 0, 1.5), (0.002, 2.34, 1.14), m.paper)
    for z in (0.9, 2.1):
        k.loft([(x, z - 0.015, z + 0.015, 1.2) for x in (-0.012, 0.022)], m.metal, segs=12, power=4.0)
    for y in (-1.2, 1.2):
        k.box((0.005, y, 1.5), (0.034, 0.03, 1.23), m.metal)
    k.box((0.05, 0, 0.895), (0.09, 2.2, 0.012), m.metal)
    k.box((0.093, 0, 0.91), (0.004, 2.2, 0.03), m.metal)
    for i, c in enumerate((m.blue, m.red, m.dark, m.green)):
        k.cyl_axis((0.05, -0.4 + i * 0.12, 0.915), 0.009, 0.13, m.white, axis="Y", verts=10)
        k.cyl_axis((0.05, -0.4 + i * 0.12 + 0.07, 0.915), 0.0095, 0.03, c, axis="Y", verts=10)
    k.box((0.05, 0.4, 0.92), (0.05, 0.12, 0.03), m.dark)
    k.box((0.05, 0.4, 0.94), (0.05, 0.12, 0.01), m.fabric)
    # a sine curve with axes, an equation-ish scribble and a boxed answer
    k.box((0.011, 0.2, 1.5), (0.002, 0.012, 0.6), m.dark)
    k.box((0.011, 0.6, 1.5), (0.002, 0.8, 0.012), m.dark)
    k.tube([(0.011, 0.22 + i * 0.025, 1.5 + 0.18 * math.sin(i * 0.4)) for i in range(32)], 0.004, m.blue, segs=4)
    for row in range(4):
        z = 1.9 - row * 0.12
        pts = [(0.011, -1.0 + i * 0.03, z + 0.025 * math.sin(i * 1.7 + row)) for i in range(14 + row * 3)]
        k.tube(pts, 0.003, m.blue if row < 3 else m.red, segs=4)
    k.tube([(0.011, -0.95, 1.4), (0.011, -0.4, 1.4), (0.011, -0.4, 1.28), (0.011, -0.95, 1.28)], 0.003, m.red, segs=4, closed=True)
    # pinned A4 sheet with magnets
    k.box((0.011, 0.95, 1.8), (0.002, 0.21, 0.297), m.white, rot=(3, 0, 0))
    for y, c in ((0.88, m.red), (1.02, m.blue)):
        k.cyl_axis((0.018, y, 1.93), 0.014, 0.012, c, axis="X", verts=12)
    for i in range(6):
        k.box((0.0125, 0.95, 1.86 - i * 0.03), (0.001, 0.15, 0.004), m.dark)


@prop
def p_Plant(k, m):
    """Office plant (rubber fig): tapered pot with rim and soil, a few stems and broad pointed leaves."""
    from mathutils import Matrix
    k.cyl((0, 0, 0), 0.2, 0.42, m.white, verts=28, r_top=0.25)
    k.cyl((0, 0, 0.42), 0.26, 0.04, m.white, verts=28)
    k.cyl((0, 0, 0.44), 0.235, 0.005, m.soil, verts=28)
    outline = [(0.0, 0.0), (0.06, 0.035), (0.16, 0.045), (0.26, 0.03), (0.32, 0.0), (0.26, -0.03), (0.16, -0.045), (0.06, -0.035)]
    n = len(outline)
    verts = [(x, y, 0.002) for x, y in outline] + [(x, y, -0.002) for x, y in outline]
    faces = [tuple(range(n)), tuple(range(2 * n - 1, n - 1, -1))] + [(i, (i + 1) % n, n + (i + 1) % n, n + i) for i in range(n)]
    stems = [(0.0, 0.0, 1.35), (0.06, 0.04, 1.1), (-0.05, -0.03, 0.95)]
    for si, (sx, sy, top) in enumerate(stems):
        k.tube([(0, 0, 0.44), (sx * 0.5, sy * 0.5, 0.7), (sx, sy, top)], 0.012, m.wood, segs=6)
        for i in range(7):
            a = i * 2.4 + si
            z = 0.62 + (top - 0.62) * i / 6
            pitch = 25 + (i % 3) * 12
            mat = Matrix.Translation((sx * z / top, sy * z / top, z)) @ Matrix.Rotation(a, 4, "Z") @ Matrix.Rotation(math.radians(pitch), 4, "Y")
            k._add(verts, faces, m.leaf, mat)
            k.tube([(sx * z / top, sy * z / top, z), (sx * z / top + math.cos(a) * 0.04, sy * z / top + math.sin(a) * 0.04, z - 0.01)], 0.004, m.leaf, segs=4)


@prop
def p_VendingMachine(k, m):
    """Drinks vending machine: rounded cabinet, glass door with lit shelves of bottles and cans, keypad, pickup flap."""
    k.loft([(x, 0.06, 1.9, 0.5) for x in [-0.4 + 0.8 * i / 6 for i in range(7)]], m.red, segs=24, power=10.0)
    k.box((0, 0, 0.03), (0.76, 0.96, 0.06), m.dark)
    k.box((0.405, -0.1, 1.12), (0.012, 0.72, 1.36), m.dark)
    k.box((0.36, -0.1, 1.12), (0.06, 0.68, 1.32), m.white)
    for r in range(5):
        z = 0.6 + r * 0.26
        k.box((0.33, -0.1, z - 0.005), (0.12, 0.66, 0.01), m.metal)
        for c in range(6):
            y = -0.39 + c * 0.116
            col = (m.blue, m.yellow, m.green, m.orange, m.red, m.ntnu)[(r * 2 + c) % 6]
            if r % 2 == 0:
                k.cyl((0.33, y, z), 0.03, 0.15, col, verts=14)
                k.cyl((0.33, y, z + 0.15), 0.03, 0.05, col, verts=14, r_top=0.012)
                k.cyl((0.33, y, z + 0.2), 0.013, 0.015, m.white, verts=10)
            else:
                k.cyl((0.33, y, z), 0.032, 0.12, col, verts=14)
                k.cyl((0.33, y, z + 0.12), 0.029, 0.006, m.metal, verts=14)
            k.box((0.395, y, z - 0.02), (0.01, 0.06, 0.018), m.paper)
    k.box((0.414, -0.1, 1.12), (0.004, 0.7, 1.34), m.glass)
    # side panel: price display, keypad, coin slot, card reader
    k.box((0.405, 0.38, 1.12), (0.012, 0.18, 1.36), m.dark)
    k.box((0.414, 0.38, 1.55), (0.004, 0.13, 0.07), m.screen_on)
    for r in range(4):
        for c in range(3):
            k.box((0.415, 0.34 + c * 0.04, 1.42 - r * 0.045), (0.008, 0.03, 0.032), m.metal)
    k.box((0.415, 0.38, 1.18), (0.01, 0.1, 0.07), m.plastic)
    k.box((0.42, 0.38, 1.18), (0.002, 0.06, 0.004), m.dark)
    k.box((0.415, 0.38, 1.05), (0.01, 0.06, 0.012), m.metal)
    # pickup flap and logo strip
    k.box((0.41, -0.05, 0.28), (0.02, 0.62, 0.26), m.dark)
    k.box((0.42, -0.05, 0.3), (0.012, 0.56, 0.18), m.plastic, rot=(0, -12, 0))
    k.box((0.405, 0, 1.84), (0.012, 0.9, 0.08), m.white)


@prop
def p_CoffeeMachine(k, m):
    """Kitchen counter with an espresso machine (portafilter, steam wand, cup tray), grinder, cups and napkins."""
    k.box((0, 0, 0.45), (0.6, 1.4, 0.88), m.lwood)
    k.box((0.302, 0, 0.45), (0.004, 1.38, 0.002), m.dark)
    for y in (-0.35, 0.35):
        k.box((0.302, y, 0.45), (0.004, 0.002, 0.86), m.dark)
        k.box((0.31, y + (-0.05 if y < 0 else 0.05), 0.75), (0.015, 0.015, 0.14), m.metal)
    k.box((0, 0, 0.03), (0.56, 1.36, 0.06), m.dark)
    k.loft([(x, 0.9, 0.93, 0.72) for x in [-0.31 + 0.62 * i / 6 for i in range(7)]], m.dark, segs=24, power=10.0)
    # espresso machine body
    k.loft([(x, 0.93, 1.4, 0.2) for x in [-0.22 + 0.4 * i / 6 for i in range(7)]], m.metal, segs=24, power=8.0, y0=-0.3)
    k.box((0.18, -0.3, 1.33), (0.01, 0.34, 0.1), m.dark)
    k.box((0.186, -0.3, 1.33), (0.002, 0.14, 0.05), m.screen_on)
    for y in (-0.42, -0.18):
        k.cyl((0.18, y, 1.3), 0.012, 0.01, m.metal, verts=10)
    k.box((0.14, -0.3, 0.945), (0.16, 0.3, 0.03), m.dark)
    for i in range(6):
        k.box((0.14, -0.42 + i * 0.048, 0.962), (0.15, 0.006, 0.004), m.metal)
    k.cyl((0.16, -0.3, 1.14), 0.035, 0.05, m.metal, verts=16)
    k.cyl_axis((0.25, -0.3, 1.155), 0.01, 0.14, m.rubber, axis="X", verts=8)
    k.tube([(0.18, -0.08, 1.25), (0.22, -0.06, 1.15), (0.23, -0.06, 1.02)], 0.006, m.metal, segs=6)
    k.cyl((0.14, -0.3, 0.96), 0.03, 0.06, m.white, verts=16, r_top=0.035)
    k.box((-0.02, -0.3, 1.42), (0.3, 0.36, 0.02), m.metal)
    for i in range(3):
        k.cyl((-0.02 + (i - 1) * 0.08, -0.36, 1.43), 0.03, 0.06, m.white, verts=14, r_top=0.035)
    # grinder with bean hopper
    k.box((-0.05, 0.05, 1.05), (0.18, 0.14, 0.24), m.dark)
    k.cyl((-0.05, 0.05, 1.17), 0.055, 0.14, m.glass, verts=16, r_top=0.075)
    k.cyl((-0.05, 0.05, 1.18), 0.05, 0.07, m.soil, verts=16, r_top=0.06)
    # cups, sugar, napkins
    for i in range(4):
        k.cyl((0.12, 0.2 + i * 0.09, 0.93), 0.032, 0.09, m.white, verts=16, r_top=0.04)
        k.cyl((0.12, 0.2 + i * 0.09, 1.018), 0.036, 0.002, m.soil, verts=16)
    k.box((0.0, 0.5, 0.98), (0.2, 0.18, 0.1), m.paper)
    k.box((0.0, 0.5, 1.035), (0.12, 0.12, 0.01), m.white)
    k.cyl((-0.12, 0.32, 0.93), 0.05, 0.1, m.white, verts=16)
    k.cyl((-0.12, 0.32, 1.03), 0.052, 0.015, m.metal, verts=16)


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
    """Office recycling bin: tapered body with a rim, swing lid, coloured sorting label and a bag lip."""
    k.cyl((0, 0, 0.0), 0.19, 0.04, m.rubber, verts=28, r_top=0.2)
    k.cyl((0, 0, 0.04), 0.2, 0.62, m.dark, verts=28, r_top=0.225)
    for i in range(8):
        a = i * math.pi / 4
        k.box((math.cos(a) * 0.207, math.sin(a) * 0.207, 0.36), (0.012, 0.012, 0.5), m.dark, rot=(0, 0, math.degrees(a)))
    k.cyl((0, 0, 0.66), 0.235, 0.03, m.plastic, verts=28)
    k.cyl((0, 0, 0.66), 0.228, 0.035, m.white, verts=28, r_top=0.2)
    k.cyl((0, 0, 0.69), 0.235, 0.06, m.green, verts=28, r_top=0.2)
    k.box((0.12, 0, 0.755), (0.1, 0.18, 0.012), m.dark, rot=(0, 18, 0))
    k.box((0.218, 0, 0.45), (0.004, 0.16, 0.12), m.green)
    k.box((0.221, 0, 0.45), (0.002, 0.1, 0.06), m.white)


@prop
def p_FireExtinguisher(k, m):
    """Wall-mounted extinguisher: bracket, round-shouldered cylinder, valve with lever and gauge, hose and label."""
    k.box((-0.04, 0, 0.9), (0.012, 0.16, 0.3), m.dark)
    k.box((0.0, 0, 0.82), (0.08, 0.12, 0.012), m.dark)
    k.tube([(-0.035, -0.08, 0.95), (0.06, -0.085, 0.95), (0.15, 0, 0.95), (0.06, 0.085, 0.95), (-0.035, 0.08, 0.95)], 0.006, m.dark, segs=6)
    k.cyl((0.06, 0, 0.6), 0.072, 0.02, m.dark, verts=24)
    k.cyl((0.06, 0, 0.62), 0.076, 0.44, m.red, verts=24)
    k.cyl((0.06, 0, 1.06), 0.076, 0.03, m.red, verts=24, r_top=0.07)
    k.cyl((0.06, 0, 1.09), 0.07, 0.03, m.red, verts=24, r_top=0.045)
    k.cyl((0.06, 0, 1.12), 0.022, 0.05, m.metal, verts=14)
    k.box((0.04, 0, 1.19), (0.13, 0.026, 0.02), m.dark, rot=(0, 6, 0))
    k.box((0.03, 0, 1.165), (0.12, 0.024, 0.016), m.dark, rot=(0, -8, 0))
    k.cyl_axis((0.06, 0.025, 1.145), 0.018, 0.012, m.white, axis="Y", verts=14)
    k.cyl_axis((0.06, 0.032, 1.145), 0.014, 0.002, m.green, axis="Y", verts=14)
    k.tube([(0.07, -0.02, 1.13), (0.13, -0.05, 1.08), (0.15, -0.06, 0.9), (0.14, -0.05, 0.75)], 0.009, m.rubber, segs=8)
    k.cyl((0.14, -0.05, 0.69), 0.014, 0.06, m.plastic, verts=10, r_top=0.011)
    k.box((0.137, 0.02, 0.82), (0.004, 0.09, 0.16), m.paper, rot=(0, 0, 20))
    k.box((0.139, 0.02, 0.88), (0.004, 0.08, 0.02), m.red, rot=(0, 0, 20))


@prop
def p_FilingCabinet(k, m):
    """Steel filing cabinet: rounded carcass, four inset drawers with recessed handles, label holders and a lock."""
    k.loft([(x, 0.02, 1.32, 0.235) for x in [-0.3 + 0.6 * i / 6 for i in range(7)]], m.metal, segs=24, power=10.0)
    k.box((0, 0, 0.02), (0.56, 0.42, 0.04), m.dark)
    for i in range(4):
        z = 0.19 + i * 0.32
        k.box((0.302, 0, z), (0.012, 0.43, 0.3), m.metal)
        k.box((0.309, 0, z + 0.05), (0.004, 0.17, 0.035), m.dark)
        k.tube([(0.312, -0.07, z + 0.03), (0.322, -0.06, z + 0.02), (0.322, 0.06, z + 0.02), (0.312, 0.07, z + 0.03)], 0.006, m.dark, segs=6)
        k.box((0.309, 0, z + 0.1), (0.004, 0.09, 0.035), m.dark)
        k.box((0.311, 0, z + 0.1), (0.002, 0.08, 0.026), m.paper)
    k.cyl_axis((0.31, 0.17, 1.27), 0.012, 0.01, m.dark, axis="X", verts=12)
    k.box((-0.02, -0.08, 1.32 + 0.04), (0.24, 0.3, 0.08), m.paper)
    k.box((-0.02, 0.12, 1.32 + 0.11), (0.3, 0.06, 0.22), m.ntnu)


@prop
def p_ServerRack(k, m):
    """Server rack: frame with posts, perforated front door glass, servers with drive bays and LEDs, cable bundles."""
    k.box((0, 0, 1.0), (1.0, 0.6, 2.0), m.dark)
    for y in (-0.28, 0.28):
        k.box((0.48, y, 1.0), (0.04, 0.04, 2.0), m.plastic)
    k.box((0.48, 0, 1.98), (0.04, 0.6, 0.04), m.plastic)
    k.box((0.48, 0, 0.04), (0.04, 0.6, 0.08), m.plastic)
    for i in range(15):
        z = 0.15 + i * 0.12
        h = 0.085 if i % 4 else 0.04
        k.box((0.43, 0, z), (0.1, 0.5, h), m.plastic)
        if i % 4:
            for j in range(8):
                k.box((0.482, -0.21 + j * 0.035, z), (0.004, 0.03, h * 0.8), m.metal)
                k.box((0.485, -0.21 + j * 0.035 + 0.01, z + h * 0.32), (0.002, 0.004, 0.004), m.screen_on if (i + j) % 3 else m.green)
            k.box((0.484, 0.18, z), (0.004, 0.08, h * 0.6), m.dark)
            k.box((0.486, 0.21, z + h * 0.25), (0.002, 0.006, 0.006), m.screen_on)
        else:
            for j in range(12):
                k.box((0.482, -0.2 + j * 0.034, z), (0.004, 0.018, 0.014), m.screen_on if j % 4 else m.orange)
    k.box((0.506, 0, 1.0), (0.008, 0.54, 1.9), m.glass)
    k.tube([(0.5, 0.33, 1.95), (0.5, 0.33, 0.1)], 0.012, m.metal, segs=8)
    for i in range(4):
        k.tube([(-0.2 + i * 0.05, 0.1, 2.0), (-0.2 + i * 0.05, 0.12 + i * 0.01, 2.2), (-0.2 + i * 0.05, 0.2, 2.6)], 0.012, (m.blue, m.yellow, m.dark, m.blue)[i], segs=6)


@prop
def p_LabBench(k, m):
    """Chemistry lab bench: cabinet doors, epoxy top, sink with gooseneck tap, reagent shelf, glassware and a burner."""
    k.box((0, 0, 0.47), (0.86, 2.36, 0.86), m.white)
    k.box((0, 0, 0.03), (0.8, 2.3, 0.06), m.dark)
    for i in range(4):
        y = -0.885 + i * 0.59
        k.box((0.432, y, 0.47), (0.004, 0.57, 0.78), m.white)
        k.box((0.436, y + 0.2, 0.75), (0.01, 0.1, 0.015), m.metal)
    k.loft([(x, 0.9, 0.94, 1.225) for x in [-0.475 + 0.95 * i / 6 for i in range(7)]], m.dark, segs=24, power=10.0)
    # sink and tap
    k.box((0.05, 0.8, 0.915), (0.36, 0.42, 0.05), m.metal)
    k.box((0.05, 0.8, 0.935), (0.32, 0.38, 0.012), m.dark)
    k.cyl((-0.25, 0.8, 0.94), 0.018, 0.08, m.metal, verts=12)
    k.tube([(-0.25, 0.8, 1.0), (-0.25, 0.8, 1.25), (-0.2, 0.8, 1.32), (-0.1, 0.8, 1.3), (-0.06, 0.8, 1.2)], 0.012, m.metal, segs=10)
    for y in (0.72, 0.88):
        k.box((-0.25, y, 1.02), (0.02, 0.08, 0.015), m.red if y < 0.8 else m.blue)
    # reagent shelf on posts
    k.box((-0.3, -0.4, 1.5), (0.22, 2.0, 0.025), m.lwood)
    for y in (-1.35, 0.55):
        k.box((-0.38, y, 1.22), (0.03, 0.03, 0.6), m.metal)
    for i in range(10):
        y = -1.3 + i * 0.18
        col = (m.orange, m.paper, m.blue, m.soil)[i % 4]
        k.cyl((-0.3, y, 1.512), 0.035, 0.12, col, verts=14)
        k.cyl((-0.3, y, 1.632), 0.02, 0.03, m.dark, verts=10)
    # glassware: erlenmeyer, beakers, flask on a stand, burner
    for i, y in enumerate((-0.95, -0.7, -0.45, -0.2)):
        if i % 2 == 0:
            k.cyl((0.15, y, 0.94), 0.06, 0.14, m.glass, verts=18, r_top=0.018)
            k.cyl((0.15, y, 1.08), 0.018, 0.05, m.glass, verts=14)
            k.cyl((0.15, y, 0.942), 0.055, 0.05, m.blue if i else m.green, verts=18, r_top=0.04)
        else:
            k.cyl((0.15, y, 0.94), 0.04, 0.11, m.glass, verts=18)
            k.cyl((0.15, y, 0.942), 0.037, 0.05, m.orange, verts=18)
    k.cyl((0.1, 0.2, 0.94), 0.06, 0.01, m.dark, verts=16)
    k.cyl((0.08, 0.2, 0.95), 0.006, 0.45, m.metal, verts=8)
    k.box((0.12, 0.2, 1.25), (0.08, 0.01, 0.01), m.metal)
    k.sphere((0.17, 0.2, 1.2), 0.045, m.glass, n=8, segs=16)
    k.cyl((0.17, 0.2, 1.24), 0.012, 0.08, m.glass, verts=10)
    k.cyl((0.17, 0.2, 0.94), 0.025, 0.12, m.metal, verts=12, r_top=0.012)
    k.box((0.25, -0.05, 0.945), (0.2, 0.28, 0.004), m.paper, rot=(0, 0, -10))


@prop
def p_Stool(k, m):
    """Lab stool: round padded seat, four splayed tube legs and a foot ring."""
    k.loft([(x, 0.64, 0.7 + 0.01 * (1 - abs(x) / 0.18), max((0.18 ** 2 - x * x) ** 0.5, 0.04)) for x in [-0.17 + 0.34 * i / 16 for i in range(17)]], m.dark, segs=20, power=2.6)
    for i in range(4):
        a = math.radians(45 + i * 90)
        k.tube([(math.cos(a) * 0.17, math.sin(a) * 0.17, 0.0), (math.cos(a) * 0.1, math.sin(a) * 0.1, 0.64)], 0.012, m.metal, segs=8)
        k.cyl((math.cos(a) * 0.17, math.sin(a) * 0.17, 0.0), 0.016, 0.01, m.rubber, verts=8)
    k.tube([(math.cos(t) * 0.145, math.sin(t) * 0.145, 0.25) for t in [i * 2 * math.pi / 20 for i in range(20)]], 0.009, m.metal, segs=6, closed=True)

@prop
def p_CafeTable(k, m):
    """Cafe table for the canteen: white top with soft edges, two pedestal feet, a tray and a cup."""
    k.loft([(x, 0.72, 0.76, 0.8) for x in [-0.45 + 0.9 * i / 6 for i in range(7)]], m.white, segs=24, power=8.0)
    for y in (-0.6, 0.6):
        k.cyl((0, y, 0), 0.25, 0.02, m.dark, verts=24, r_top=0.22)
        k.cyl((0, y, 0.02), 0.04, 0.7, m.metal, verts=14)
    k.box((0.1, -0.3, 0.77), (0.3, 0.25, 0.02), m.orange)
    k.cyl((-0.2, 0.4, 0.76), 0.04, 0.1, m.white, verts=14, r_top=0.045)

@prop
def p_LectureSeats(k, m):
    """Row of 6 auditorium seats with a writing table in front (+X): padded seat and back shells, side standards."""
    for i in range(6):
        y = -1.5 + i * 0.6
        k.loft([(x, 0.41, 0.49 + 0.01 * (1 - abs(x) / 0.22), 0.24) for x in [-0.22 + 0.44 * i / 6 for i in range(7)]], m.fabric_red, segs=20, power=3.0, y0=y)
        k.loft([(x, 0.52 + (x + 0.31) * 0.2, 1.08 + (x + 0.31) * 0.2, 0.24) for x in (-0.31, -0.27, -0.23)], m.fabric_red, segs=20, power=3.0, y0=y)
        k.loft([(-0.335, 0.52, 1.08, 0.235), (-0.315, 0.52, 1.08, 0.235)], m.dark, segs=16, power=4.0, y0=y)
    for y in (-1.8, -1.2, -0.6, 0.0, 0.6, 1.2, 1.8):
        k.loft([(x, 0.0, 0.62 - 0.1 * abs(x) / 0.25, 0.025) for x in [-0.25 + 0.5 * i / 6 for i in range(7)]], m.dark, segs=12, power=4.0, y0=y)
        k.box((0.0, y, 0.64), (0.3, 0.06, 0.04), m.lwood)
    k.loft([(x, 0.765, 0.795, 1.8) for x in [0.28 + 0.34 * i / 4 for i in range(5)]], m.lwood, segs=24, power=6.0)
    k.box((0.6, 0, 0.4), (0.03, 3.6, 0.78), m.lwood)
    for y in (-1.5, -0.3, 0.9):
        k.box((0.585, y + 0.3, 0.08), (0.005, 0.12, 0.05), m.white)


@prop
def p_Lectern(k, m):
    """Lecture podium: rounded wooden body, slanted reading top with a lip, NTNU logo panel, gooseneck microphone."""
    k.loft([(x, 0.04, 1.08, 0.35 - 0.04 * (x + 0.25) / 0.5) for x in [-0.25 + 0.5 * i / 6 for i in range(7)]], m.wood, segs=24, power=8.0)
    k.box((0, 0, 0.02), (0.54, 0.74, 0.04), m.dark)
    k.box((0.05, 0, 1.13), (0.52, 0.76, 0.035), m.wood, rot=(0, -15, 0))
    k.box((0.3, 0, 1.08), (0.02, 0.74, 0.03), m.wood, rot=(0, -15, 0))
    k.box((0.252, 0, 0.66), (0.01, 0.46, 0.28), m.ntnu)
    k.box((0.258, 0, 0.66), (0.004, 0.32, 0.04), m.white)
    k.box((0.05, -0.12, 1.16), (0.297, 0.21, 0.004), m.paper, rot=(0, -15, 0))
    k.cyl((-0.12, 0.26, 1.18), 0.025, 0.02, m.dark, verts=12)
    k.tube([(-0.12, 0.26, 1.2), (-0.11, 0.25, 1.35), (-0.04, 0.22, 1.43), (0.04, 0.2, 1.42)], 0.006, m.dark, segs=6)
    k.sphere((0.06, 0.2, 1.415), 0.018, m.dark, n=6, segs=12, scale=(1.6, 1.0, 1.0))
    k.cyl((-0.1, -0.28, 1.2), 0.03, 0.1, m.glass, verts=14)


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
    """Quadruped robot dog, about 0.7 m long, facing +X: rounded body, camera head, hip motors, bent legs, rubber feet."""
    body = k.mat("RobotBody", (0.9, 0.75, 0.1), metallic=0.3, rough=0.4)
    k.loft([(x, 0.37 + 0.01 * abs(x) / 0.33, 0.53 - 0.02 * abs(x) / 0.33, 0.13 - 0.015 * abs(x) / 0.33) for x in [-0.33 + 0.66 * i / 10 for i in range(11)]], body, segs=24, power=4.0)
    k.loft([(x, 0.53, 0.55, 0.09) for x in [-0.25 + 0.5 * i / 6 for i in range(7)]], m.dark, segs=16, power=6.0)
    k.loft([(0.33, 0.4, 0.52, 0.1), (0.38, 0.41, 0.51, 0.09), (0.4, 0.42, 0.5, 0.08)], m.dark, segs=20, power=4.0)
    for y in (-0.045, 0.045):
        k.cyl_axis((0.402, y, 0.46), 0.022, 0.01, m.screen, axis="X", verts=14)
    k.cyl_axis((0.402, 0.0, 0.43), 0.008, 0.01, m.screen_on, axis="X", verts=8)
    for x in (-0.24, 0.24):
        for y in (-0.15, 0.15):
            yy = y * 1.08
            k.cyl_axis((x, y, 0.42), 0.055, 0.07, m.dark, axis="Y", verts=16)
            knee = (x + 0.07 if x > 0 else x - 0.07, yy, 0.23)
            k.tube([(x, yy, 0.42), knee], 0.035, body, segs=12)
            k.sphere(knee, 0.035, m.dark)
            foot = (x - 0.01 if x > 0 else x + 0.01, yy, 0.035)
            k.tube([knee, foot], 0.022, m.dark, segs=10)
            k.sphere(foot, 0.035, m.rubber)
    k.loft([(-0.34, 0.42, 0.5, 0.08), (-0.335, 0.42, 0.5, 0.08)], m.screen_on, segs=12, power=6.0)


@prop
def p_RobotHumanoid(k, m):
    """Humanoid robot, 1.75 m: white shells over dark joints, chest display, visor head, cable runs."""
    shell = k.mat("RobotWhite", (0.88, 0.88, 0.9), rough=0.35)
    for y in (-0.11, 0.11):
        k.loft([(x, 0.0, 0.07 - 0.02 * max(x, 0) / 0.14, 0.055) for x in [-0.08 + 0.22 * i / 6 for i in range(7)]], m.dark, segs=16, power=4.0, y0=y)
        k.sphere((0.0, y, 0.1), 0.05, m.dark)
        k.tube([(0.0, y, 0.12), (0.01, y, 0.48)], 0.055, shell, segs=14)
        k.loft([(x, 0.2, 0.4, 0.05) for x in (0.04, 0.06, 0.075)], m.dark, segs=12, power=3.0, y0=y)
        k.sphere((0.0, y, 0.52), 0.065, m.dark)
        k.tube([(0.0, y, 0.55), (0.0, y, 0.9)], 0.07, shell, segs=14)
        k.tube([(-0.075, y, 0.58), (-0.075, y, 0.88)], 0.012, m.dark, segs=6)
    k.sphere((0.0, 0.0, 0.98), 0.16, m.dark, scale=(0.7, 1.0, 0.5))
    k.loft([(x, 1.04, 1.48 - 0.03 * abs(x) / 0.14, 0.2 - 0.03 * abs(x) / 0.14) for x in [-0.14 + 0.28 * i / 8 for i in range(9)]], shell, segs=24, power=3.0)
    k.loft([(0.135, 1.24, 1.36, 0.1), (0.145, 1.24, 1.36, 0.1)], m.screen_on, segs=16, power=6.0)
    k.loft([(-0.17, 1.1, 1.4, 0.12), (-0.14, 1.1, 1.4, 0.12)], m.dark, segs=16, power=5.0)
    for y in (-0.26, 0.26):
        k.sphere((0.0, y, 1.4), 0.07, m.dark)
        k.tube([(0.0, y, 1.37), (0.02, y * 1.04, 1.1)], 0.05, shell, segs=12)
        k.sphere((0.02, y * 1.04, 1.08), 0.045, m.dark)
        k.tube([(0.02, y * 1.04, 1.06), (0.08, y * 1.02, 0.84)], 0.042, shell, segs=12)
        k.loft([(x, 0.74, 0.82, 0.03) for x in (0.06, 0.09, 0.12)], m.dark, segs=12, power=3.0, y0=y)
        for f in range(3):
            k.tube([(0.12, y + (f - 1) * 0.018, 0.78), (0.15, y + (f - 1) * 0.018, 0.74)], 0.008, m.dark, segs=6)
    k.tube([(0.0, 0.0, 1.47), (0.0, 0.0, 1.54)], 0.045, m.dark, segs=12)
    k.sphere((0.0, 0.0, 1.64), 0.12, shell, scale=(1.0, 0.9, 1.0))
    k.loft([(0.07, 1.6, 1.68, 0.085), (0.105, 1.6, 1.68, 0.075)], m.screen, segs=18, power=3.0)
    k.loft([(0.108, 1.63, 1.645, 0.05), (0.112, 1.63, 1.645, 0.05)], m.screen_on, segs=12, power=4.0)
    for y in (-0.105, 0.105):
        k.cyl_axis((0.0, y, 1.64), 0.03, 0.02, m.dark, axis="Y", verts=14)


@prop
def p_RobotGuard(k, m):
    """Wheeled security robot: smooth egg-shaped shell, sensor band, camera dome, light bar and bumpers."""
    shell = k.mat("RobotWhite", (0.88, 0.88, 0.9), rough=0.35)
    for x in (-0.24, 0.24):
        for y in (-0.3, 0.3):
            k.cyl_axis((x, y, 0.13), 0.12, 0.08, m.rubber, axis="Y", verts=20)
            k.cyl_axis((x, y * 1.15, 0.13), 0.07, 0.01, m.metal, axis="Y", verts=16)
    st = []
    for i in range(17):
        t = i / 16
        z = 0.08 + t * 1.1
        r = 0.4 * (1 - 0.15 * t) * math.sqrt(max(1 - (max(t - 0.62, 0) / 0.38) ** 2, 0.04))
        st.append((z, r))
    for (za, ra), (zb, rb) in zip(st, st[1:]):
        k.cyl((0, 0, za), ra, zb - za, shell, verts=32, r_top=rb)
    k.cyl((0, 0, 0.5), 0.41, 0.07, m.ntnu, verts=32, r_top=0.405)
    k.cyl((0, 0, 0.9), 0.335, 0.05, m.dark, verts=32, r_top=0.32)
    for i in range(6):
        a = i * math.pi / 3
        k.cyl_axis((math.cos(a) * 0.335, math.sin(a) * 0.335, 0.925), 0.018, 0.02, m.screen, axis="X", verts=10) if i == 0 else \
            k.sphere((math.cos(a) * 0.33, math.sin(a) * 0.33, 0.925), 0.02, m.screen, n=6, segs=10)
    k.cyl((0, 0, 1.17), 0.1, 0.05, m.dark, verts=20)
    k.sphere((0, 0, 1.23), 0.085, m.screen, n=10, segs=20, scale=(1.0, 1.0, 0.8))
    k.box((0, 0, 1.06), (0.08, 0.36, 0.035), m.red)
    k.box((0, 0, 1.06), (0.08, 0.1, 0.037), m.blue)
    k.box((0.378, 0, 0.74), (0.03, 0.34, 0.12), m.dark, rot=(0, -10, 0))
    k.box((0.394, 0, 0.74), (0.004, 0.3, 0.08), m.screen_on, rot=(0, -10, 0))
    k.loft([(x, 0.1, 0.2, 0.42) for x in (0.32, 0.4, 0.44)], m.rubber, segs=16, power=4.0)
    k.loft([(x, 0.1, 0.2, 0.42) for x in (-0.44, -0.4, -0.32)], m.rubber, segs=16, power=4.0)
    k.cyl((-0.12, 0.12, 1.12), 0.006, 0.3, m.dark, verts=6)


@prop
def p_RobotDelivery(k, m):
    """Six-wheeled sidewalk delivery robot: rounded body, lid with seam and handle, sensors, lights and a flag."""
    shell = k.mat("RobotWhite", (0.88, 0.88, 0.9), rough=0.35)
    for x in (-0.24, 0.0, 0.24):
        for y in (-0.27, 0.27):
            k.cyl_axis((x, y, 0.1), 0.1, 0.07, m.rubber, axis="Y", verts=20)
            k.cyl_axis((x, y * 1.14, 0.1), 0.055, 0.01, m.dark, axis="Y", verts=16)
        k.box((x, 0, 0.1), (0.04, 0.5, 0.03), m.dark)
    k.loft([(x, 0.15, 0.5, 0.26) for x in [-0.36 + 0.72 * i / 8 for i in range(9)]], shell, segs=28, power=6.0)
    k.loft([(x, 0.49, 0.58 - 0.02 * abs(x) / 0.34, 0.25) for x in [-0.34 + 0.68 * i / 8 for i in range(9)]], shell, segs=28, power=6.0)
    k.loft([(x, 0.485, 0.495, 0.262) for x in [-0.35 + 0.7 * i / 4 for i in range(5)]], m.dark, segs=24, power=6.0)
    k.loft([(x, 0.15, 0.24, 0.265) for x in [-0.37 + 0.74 * i / 8 for i in range(9)]], m.dark, segs=24, power=6.0)
    k.tube([(-0.3, -0.06, 0.585), (-0.3, -0.06, 0.6), (-0.3, 0.06, 0.6), (-0.3, 0.06, 0.585)], 0.008, m.dark, segs=6)
    # front face: sensor window with camera eyes, headlights; rear red lights
    k.box((0.355, 0, 0.38), (0.012, 0.36, 0.1), m.screen)
    for y in (-0.08, 0.08):
        k.cyl_axis((0.363, y, 0.38), 0.022, 0.006, m.screen_on, axis="X", verts=14)
    for y in (-0.19, 0.19):
        k.box((0.358, y, 0.28), (0.01, 0.07, 0.03), m.white)
        k.box((-0.358, y, 0.32), (0.01, 0.07, 0.03), m.red)
    k.box((0.2, 0.262, 0.36), (0.18, 0.004, 0.05), m.ntnu)
    k.box((0.2, -0.262, 0.36), (0.18, 0.004, 0.05), m.ntnu)
    # flag on a whip antenna
    k.tube([(-0.3, 0.2, 0.55), (-0.31, 0.2, 1.0), (-0.32, 0.2, 1.4)], 0.005, m.metal, segs=6)
    k.box((-0.32, 0.27, 1.33), (0.006, 0.14, 0.1), m.orange)


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
