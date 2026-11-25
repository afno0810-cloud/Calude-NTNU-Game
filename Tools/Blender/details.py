"""Ground details that make streets and lawns look lived in:  python Tools/Blender/details.py

SM_Rock_A/B/C   boulders (moraine stones, typical for Trondheim lawns)
SM_FlowerBed    round stone-edged bed with soil, low shrubs and flowers
SM_Manhole      cast iron cover set in the asphalt
SM_Drain        kerb drain grate
SM_Hydrant      red fire hydrant
SM_PowerBox     grey utility cabinet
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402,F401
import bmesh  # noqa: E402
from mathutils import Vector  # noqa: E402
from meshkit import MeshKit  # noqa: E402


def rock(name, seed, size):
    k = MeshKit(name)
    stone = k.mat("StoneDark", (0.3, 0.3, 0.29), rough=0.9)
    rnd = random.Random(seed)
    res = bmesh.ops.create_icosphere(k.bm, subdivisions=3, radius=1.0)
    lumps = [(Vector((rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-0.3, 1))).normalized(), rnd.uniform(0.1, 0.3))
             for _ in range(9)]
    for v in res["verts"]:
        d = v.co.normalized()
        r = 1.0 + sum(a * max(0.0, d.dot(c)) ** 3 for c, a in lumps) + rnd.uniform(-0.03, 0.03)
        # flat cuts like split granite
        p = d * r
        p.x *= size[0]
        p.y *= size[1]
        p.z = p.z * size[2]
        v.co = p
    for v in res["verts"]:
        v.co.z = max(v.co.z, -0.15 * size[2])
        v.co.z += 0.1 * size[2]
    for f in k.bm.faces:
        f.material_index = stone
    k.finish(also_blend=False)


def flower_bed():
    k = MeshKit("SM_FlowerBed")
    edge = k.mat("Stone", (0.5, 0.5, 0.48))
    soil = k.mat("Soil", (0.12, 0.08, 0.05), rough=1.0)
    green = k.mat("PaintGreen", (0.1, 0.3, 0.08))
    petals = [k.mat(n, c) for n, c in (("PaintRed", (0.7, 0.05, 0.05)), ("PaintYellow", (0.9, 0.7, 0.05)),
                                       ("PlasticWhite", (0.9, 0.9, 0.9)), ("PaintBlue", (0.2, 0.25, 0.7)))]
    R = 1.8
    n = 22
    for i in range(n):
        a = 2 * math.pi * i / n
        k.box((math.cos(a) * R, math.sin(a) * R, 0.15), (0.55, 0.3, 0.3), edge, rot=(0, 0, math.degrees(a) + 90))
    k.cyl((0, 0, 0), R - 0.1, 0.22, soil, verts=24)
    rnd = random.Random(4)
    for _ in range(14):
        a, r = rnd.uniform(0, 6.28), rnd.uniform(0, R - 0.5)
        k.cyl((math.cos(a) * r, math.sin(a) * r, 0.2), rnd.uniform(0.2, 0.35), rnd.uniform(0.3, 0.5), green, verts=7, r_top=0.08)
    for _ in range(70):
        a, r = rnd.uniform(0, 6.28), rnd.uniform(0, R - 0.25)
        x, y = math.cos(a) * r, math.sin(a) * r
        h = rnd.uniform(0.3, 0.55)
        k.cyl((x, y, 0.2), 0.012, h, green, verts=4)
        pm = rnd.choice(petals)
        for p in range(5):
            b = 2 * math.pi * p / 5
            k.box((x + math.cos(b) * 0.04, y + math.sin(b) * 0.04, 0.2 + h), (0.06, 0.035, 0.01), pm, rot=(0, 15, math.degrees(b)))
        k.cyl((x, y, 0.19 + h), 0.018, 0.025, petals[1], verts=6)
    k.finish(also_blend=False)


def manhole():
    k = MeshKit("SM_Manhole")
    iron = k.mat("MetalDark", (0.08, 0.08, 0.08), metallic=1, rough=0.6)
    k.cyl((0, 0, -0.02), 0.34, 0.035, iron, verts=32)
    # raised pattern rings and cross bars
    for r in (0.12, 0.22, 0.31):
        for i in range(24):
            a = 2 * math.pi * i / 24
            k.box((math.cos(a) * r, math.sin(a) * r, 0.018), (0.012, 2 * math.pi * r / 24 + 0.005, 0.008), iron, rot=(0, 0, math.degrees(a)))
    for a in (0, 45, 90, 135):
        k.box((0, 0, 0.018), (0.6, 0.02, 0.008), iron, rot=(0, 0, a))
    k.finish(also_blend=False)


def drain():
    k = MeshKit("SM_Drain")
    iron = k.mat("MetalDark", (0.08, 0.08, 0.08), metallic=1, rough=0.6)
    hole = k.mat("Rubber", (0.01, 0.01, 0.01), rough=1)
    k.box((0, 0, -0.03), (0.5, 0.35, 0.02), hole)
    k.box((0, -0.18, 0.0), (0.56, 0.03, 0.03), iron)
    k.box((0, 0.18, 0.0), (0.56, 0.03, 0.03), iron)
    for i in range(9):
        k.box((-0.24 + i * 0.06, 0, 0.0), (0.022, 0.36, 0.03), iron)
    k.finish(also_blend=False)


def hydrant():
    k = MeshKit("SM_Hydrant")
    red = k.mat("PaintRed", (0.6, 0.04, 0.03), rough=0.45)
    steel = k.mat("Steel", (0.6, 0.6, 0.6), metallic=1, rough=0.35)
    k.cyl((0, 0, 0), 0.13, 0.08, red, verts=16)
    k.cyl((0, 0, 0.08), 0.1, 0.62, red, verts=16)
    k.cyl((0, 0, 0.7), 0.12, 0.06, red, verts=16)
    k.cyl((0, 0, 0.76), 0.1, 0.1, red, verts=16, r_top=0.03)
    k.cyl((0, 0, 0.86), 0.025, 0.04, steel, verts=5)
    for s in (-1, 1):
        k.cyl_axis((0, s * 0.13, 0.5), 0.045, 0.08, steel, axis="Y")
    k.cyl_axis((0.14, 0, 0.45), 0.06, 0.08, steel, axis="X")
    k.finish(also_blend=False)


def power_box():
    k = MeshKit("SM_PowerBox")
    grey = k.mat("Metal", (0.5, 0.52, 0.5), rough=0.5)
    base = k.mat("Concrete", (0.5, 0.5, 0.5))
    dark = k.mat("MetalDark", (0.1, 0.1, 0.1), metallic=1)
    k.box((0, 0, 0.08), (1.0, 0.45, 0.16), base)
    k.box((0, 0, 0.66), (0.9, 0.38, 1.0), grey)
    k.box((0, 0, 1.2), (0.96, 0.44, 0.06), grey)
    k.box((0, 0.195, 0.66), (0.004, 0.005, 0.9), dark)
    for x in (-0.22, 0.22):
        k.box((x, 0.2, 0.7), (0.03, 0.02, 0.08), dark)
    k.finish(also_blend=False)


if __name__ == "__main__":
    rock("SM_Rock_A", 1, (1.2, 0.9, 0.7))
    rock("SM_Rock_B", 2, (0.7, 0.6, 0.5))
    rock("SM_Rock_C", 3, (2.0, 1.4, 1.0))
    flower_bed()
    manhole()
    drain()
    hydrant()
    power_box()
