"""Doors and glass bits for the interactive world (Interaktiv verden, round 1b).

    "C:\\Program Files\\Blender Foundation\\Blender 5.3\\blender.exe" --background --python Tools/Blender/interactive_doors.py
    ... --python Tools/Blender/interactive_doors.py -- Handle Keypad      # only these
    ... --python Tools/Blender/interactive_doors.py -- --render           # all, then a contact sheet

Writes SourceArt/Models/SM_Int_<Name>.fbx/.blend (Content/Python/init_unreal.py imports them into /Game/Models; SM_Int_ is
not Nanite). Meters, Unreal space (MeshKit), front = +X.

Sizes the C++ code relies on (GSDoor.cpp scales the leaves to each door):
- Door leaves: 1.00 wide (Y), 2.09 high (Z), origin on the floor in the middle under the leaf. Wood 0.04 thick,
  fire door and glass frame 0.05.
- Glass frame: stiles 0.09, bottom rail 0.20, top rail 0.10 (the collision boxes in GSDoor.cpp have the same sizes).
- Handle: origin on the door face at the rose, sticks out along +X, the lever points along +Y.
- Keypad: 0.08 (Y) x 0.14 (Z) x 0.03 (X), origin at the back centre (on the wall), the LED sits at the top front.
- Glass bits: real size, origin in the middle. Granules: a flat scatter about 0.12 m across, origin on the floor.
"""
import math
import os
import random
import runpy
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gs_common import ROOT, cli_args  # noqa: E402
from meshkit import MeshKit  # noqa: E402

MODELS = {}


def model(f):
    MODELS[f.__name__[2:]] = f  # "m_Handle" -> "Handle"
    return f


W, H = 1.0, 2.09


@model
def m_Door_Wood(k):
    """Light oak veneer leaf with darker edge bands and a lock case on the free edge, like NTNU's room doors."""
    oak = k.mat("WoodLight_In", (0.7, 0.55, 0.38), rough=0.55)
    edge = k.mat("Wood_In", (0.42, 0.28, 0.16), rough=0.55)
    steel = k.mat("M_Steel_In", (0.55, 0.56, 0.58), metallic=0.9, rough=0.35)
    T = 0.04
    k.box((0, 0, H / 2), (T, W - 0.016, H - 0.008), oak)
    for y in (-W / 2 + 0.004, W / 2 - 0.004):
        k.box((0, y, H / 2), (T, 0.008, H), edge)
    k.box((0, 0, H - 0.004), (T, W - 0.016, 0.008), edge)
    k.box((0, 0, 0.004), (T, W - 0.016, 0.008), edge)
    # Lock case face plates on both free edges (the handle side is mirrored by the code)
    for y in (-W / 2 - 0.0005, W / 2 + 0.0005):
        k.box((0, y, 1.05), (0.022, 0.001, 0.24), steel)
    # Thin groove lines (veneer joints)
    for z in (0.6, 1.5):
        k.box((0, 0, z), (T + 0.0006, W - 0.06, 0.002), edge)


@model
def m_Door_Fire(k):
    """White steel fire door: push bar on the front (+X), a small wired-glass window, a kick plate."""
    paint = k.mat("PlasticWhite_In", (0.85, 0.86, 0.87), rough=0.5)
    steel = k.mat("M_Steel_In", (0.55, 0.56, 0.58), metallic=0.9, rough=0.35)
    dark = k.mat("MetalDark_In", (0.08, 0.08, 0.09), metallic=0.7, rough=0.5)
    glass = k.mat("GlassDark", (0.2, 0.25, 0.28), metallic=0.3, rough=0.05)
    red = k.mat("PaintRed_In", (0.75, 0.05, 0.04), rough=0.4)
    T = 0.05
    wz, wh, ww = 1.5, 0.5, 0.22
    # Leaf with a hole for the window: four boxes round it
    k.box((0, 0, (wz - wh / 2) / 2), (T, W, wz - wh / 2), paint)
    top0 = wz + wh / 2
    k.box((0, 0, (top0 + H) / 2), (T, W, H - top0), paint)
    side = (W - ww) / 2
    for s in (-1, 1):
        k.box((0, s * (ww / 2 + side / 2), wz), (T, side, wh), paint)
    # Window: frame and wired glass (a 25 mm grid of wire in the middle)
    for s in (-1, 1):
        k.box((s * (T / 2 + 0.004), 0, wz + wh / 2 + 0.008), (0.008, ww + 0.032, 0.016), dark)
        k.box((s * (T / 2 + 0.004), 0, wz - wh / 2 - 0.008), (0.008, ww + 0.032, 0.016), dark)
        for y in (-1, 1):
            k.box((s * (T / 2 + 0.004), y * (ww / 2 + 0.008), wz), (0.008, 0.016, wh), dark)
    k.box((0, 0, wz), (0.008, ww, wh), glass)
    n = int(ww / 0.025)
    for i in range(n + 1):
        y = -ww / 2 + i * ww / n
        k.box((0, y, wz), (0.0092, 0.0008, wh), dark)
    for i in range(int(wh / 0.025) + 1):
        z = wz - wh / 2 + i * 0.025
        k.box((0, 0, z), (0.0092, ww, 0.0008), dark)
    # Push bar (front): two end boxes and the bar at 1.0 m
    bl = W - 0.24
    for y in (-bl / 2, bl / 2):
        k.box((T / 2 + 0.035, y, 1.0), (0.07, 0.05, 0.08), dark)
    k.cyl_axis((T / 2 + 0.055, 0, 1.0), 0.016, bl - 0.04, steel, axis="Y", verts=16)
    # Kick plate on both faces
    for s in (-1, 1):
        k.box((s * (T / 2 + 0.0008), 0, 0.15), (0.0016, W - 0.04, 0.28), steel)
    # The little red "fire door, keep closed" sign on the front
    k.box((T / 2 + 0.0006, 0, 1.75), (0.0012, 0.18, 0.06), red)


@model
def m_Door_GlassFrame(k):
    """Black anodised aluminium frame of a group-room glass door, without the glass (the pane is breakable)."""
    alu = k.mat("MetalDark_In", (0.04, 0.045, 0.05), metallic=0.8, rough=0.4)
    rubber = k.mat("Rubber", (0.02, 0.02, 0.02), rough=0.95)
    T, stile, bottom, top = 0.05, 0.09, 0.20, 0.10
    for y in (-W / 2 + stile / 2, W / 2 - stile / 2):
        k.box((0, y, H / 2), (T, stile, H), alu)
    k.box((0, 0, bottom / 2), (T, W - 2 * stile, bottom), alu)
    k.box((0, 0, H - top / 2), (T, W - 2 * stile, top), alu)
    # Glazing beads and gaskets round the opening, both faces
    iw, ih, iz = W - 2 * stile, H - bottom - top, bottom + (H - bottom - top) / 2
    for s in (-1, 1):
        x = s * 0.008
        k.box((x, 0, bottom + 0.006), (0.006, iw, 0.012), rubber)
        k.box((x, 0, H - top - 0.006), (0.006, iw, 0.012), rubber)
        for y in (-1, 1):
            k.box((x, y * (iw / 2 - 0.006), iz), (0.006, 0.012, ih), rubber)
    # A slim pull rail line on the edge stile and the hinge knuckles on the other
    for z in (0.25, 1.05, 1.85):
        k.cyl((0, W / 2 - 0.004, z - 0.06), 0.011, 0.12, alu, verts=12)


@model
def m_Handle(k):
    """Brushed steel lever handle on a round rose (NTNU standard). One side; the code places one per face."""
    chrome = k.mat("M_Chrome_In", (0.6, 0.6, 0.62), metallic=1.0, rough=0.25)
    # Rose: a flat cylinder lying along X
    k.cyl_axis((0.004, 0, 0), 0.026, 0.008, chrome, axis="X", verts=24)
    # Neck out from the door
    k.cyl_axis((0.03, 0, 0), 0.009, 0.046, chrome, axis="X", verts=16)
    # Lever along +Y with a rounded end, slightly flattened
    k.tube([(0.055, 0.0, 0.0), (0.06, 0.05, 0.0), (0.062, 0.12, 0.0), (0.05, 0.135, 0.0)], 0.0095, chrome, segs=12)
    # Key cylinder escutcheon under the handle
    k.cyl_axis((0.003, 0, -0.085), 0.018, 0.006, chrome, axis="X", verts=20)
    k.box((0.0065, 0, -0.085), (0.001, 0.0025, 0.012), k.mat("MetalDark_In", (0.08, 0.08, 0.09), metallic=0.7, rough=0.5))


@model
def m_Keypad(k):
    """Code lock by the glass doors: dark steel box, 12 rubber keys, a recess for the LED at the top."""
    body = k.mat("MetalDark_In", (0.08, 0.08, 0.09), metallic=0.7, rough=0.5)
    keys = k.mat("Plastic_In", (0.06, 0.06, 0.07), rough=0.6)
    white = k.mat("PlasticWhite_In", (0.85, 0.86, 0.87), rough=0.5)
    D, Wk, Hk = 0.03, 0.08, 0.14
    k.box((D / 2, 0, 0), (D, Wk, Hk), body)
    # Key field: 3 x 4 keys
    for r in range(4):
        for c in range(3):
            y = (c - 1) * 0.021
            z = 0.02 - r * 0.021
            k.box((D + 0.002, y, z), (0.004, 0.016, 0.016), keys)
            k.box((D + 0.0042, y, z), (0.0004, 0.006, 0.006), white)   # the digit
    # Card reader mark and the LED recess
    k.box((D + 0.0005, 0, 0.05), (0.001, 0.02, 0.014), keys)
    k.box((D + 0.0005, 0, -0.06), (0.001, 0.05, 0.008), white)


def _chunk(k, mat, rng, centre, n, size):
    for _ in range(n):
        c = tuple(centre[i] + rng.uniform(-size, size) * (0.4 if i == 2 else 1.0) for i in range(3))
        s = (rng.uniform(0.5, 1.0) * size, rng.uniform(0.5, 1.0) * size, rng.uniform(0.4, 0.7) * size)
        k.box(c, s, mat, rot=(rng.uniform(-20, 20), rng.uniform(-20, 20), rng.uniform(0, 90)))


@model
def m_GlassClump(k):
    """A clump of tempered-glass cubes (tempered glass breaks into small blunt pieces)."""
    glass = k.mat("Glass", (0.6, 0.75, 0.8), metallic=0.3, rough=0.05)
    rng = random.Random(11)
    _chunk(k, glass, rng, (0, 0, 0), 9, 0.008)


@model
def m_GlassGranules(k):
    """Flat scatter of tempered-glass bits on the floor, about 12 cm across."""
    glass = k.mat("Glass", (0.6, 0.75, 0.8), metallic=0.3, rough=0.05)
    rng = random.Random(7)
    for _ in range(26):
        a, r = rng.uniform(0, 2 * math.pi), 0.06 * math.sqrt(rng.random())
        s = rng.uniform(0.003, 0.007)
        k.box((math.cos(a) * r, math.sin(a) * r, s * 0.25), (s, s * rng.uniform(0.7, 1.2), s * 0.5), glass,
              rot=(0, 0, rng.uniform(0, 90)))


SHARDS = {
    "A": [(-0.06, -0.07), (0.07, -0.05), (0.01, 0.09)],
    "B": [(-0.05, -0.09), (0.06, -0.08), (0.08, 0.04), (-0.03, 0.1)],
    "C": [(-0.09, -0.03), (0.08, -0.06), (0.05, 0.02), (-0.04, 0.05)],
    "D": [(-0.03, -0.1), (0.04, -0.09), (0.02, 0.11)],
}


def _shard(k, pts):
    glass = k.mat("Glass", (0.6, 0.75, 0.8), metallic=0.3, rough=0.05)
    foil = k.mat("PlasticWhite_In", (0.85, 0.86, 0.87), rough=0.5)
    # Laminated: two 4 mm sheets and the foil between (a thin white edge)
    k.prism_y(-0.0045, -0.0005, pts, glass)
    k.prism_y(0.0005, 0.0045, pts, glass)
    k.prism_y(-0.0005, 0.0005, pts, foil)


for _n, _p in SHARDS.items():
    MODELS[f"Shard_{_n}"] = (lambda p: (lambda k: _shard(k, p)))(_p)


def build(name):
    k = MeshKit(f"SM_Int_{name}")
    MODELS[name](k)
    k.finish()


if __name__ == "__main__":
    args = cli_args()
    render = "--render" in args
    names = [a for a in args if a in MODELS] or list(MODELS)
    for n in names:
        build(n)
        print("built", n)
    print(len(names), "interactive models")
    if render:
        out = os.path.join(ROOT, ".claude", "lag", "interaktiv", "kontroll", "r01b-modeller.png")
        sys.argv = ["contact_sheet.py", out] + [f"SM_Int_{n}" for n in MODELS]
        os.environ.setdefault("COLS", "6")
        runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), "contact_sheet.py"), run_name="__main__")
