"""Horizon around Gloshaugen:  python Tools/Blender/backdrop.py  ->  SM_Backdrop

Rolling forest hills in a ring 1.3-4 km out (Trondheim lies in a valley: Byaasen, Estenstadmarka, Lade),
a strip of fjord to the north, and a few hundred town houses and blocks between campus and the hills,
so the world does not end at a flat green edge. Centered on the middle of campus (GSCampusBuilder places it),
no collision, flat materials (it is always far away and in haze).
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from meshkit import MeshKit  # noqa: E402


def noise(x, y, seed):
    s = 0.0
    a = 1.0
    f = 1.0
    for o in range(5):
        s += a * (math.sin(x * 0.0017 * f + seed * 1.3 + o) * math.cos(y * 0.0013 * f - seed * 0.7 + o * 2.1))
        a *= 0.5
        f *= 2.03
    return s


def main():
    k = MeshKit("SM_Backdrop")
    forest = k.mat("Forest", (0.05, 0.09, 0.05), rough=0.95)
    field = k.mat("Field", (0.18, 0.24, 0.1), rough=0.95)
    water = k.mat("Water", (0.03, 0.06, 0.08), metallic=0.2, rough=0.08)
    rock = k.mat("Rock", (0.25, 0.25, 0.25), rough=0.95)
    walls = [k.mat(n, c) for n, c in (("HouseWhite", (0.8, 0.79, 0.75)), ("HouseRed", (0.45, 0.1, 0.07)),
                                     ("HouseYellow", (0.75, 0.6, 0.3)), ("HouseGrey", (0.5, 0.5, 0.5)))]
    roofs = [k.mat("RoofDark", (0.08, 0.08, 0.09)), k.mat("RoofRed", (0.35, 0.1, 0.06))]
    window = k.mat("WindowDark", (0.03, 0.04, 0.05), rough=0.1)
    bm = k.bm
    # hills: polar grid, height grows with distance, fjord cut out to the north (-Y)
    rings = [1250, 1500, 1800, 2200, 2700, 3300, 4000]
    seg = 96
    verts = []
    for ri, r in enumerate(rings):
        row = []
        for i in range(seg):
            a = 2 * math.pi * i / seg
            x, y = math.cos(a) * r, math.sin(a) * r
            north = max(0.0, -math.sin(a))  # -Y is north
            h = (ri / (len(rings) - 1)) ** 1.3 * (160 + 120 * noise(x, y, 3)) + 25 * noise(x * 3, y * 3, 7)
            h *= 1.0 - 0.85 * north ** 3 * (1 if ri < 4 else 0.3)
            h = max(h, -2.0 if north > 0.8 and ri < 4 else 2.0)
            row.append(bm.verts.new((x, y, h - 1.0)))
        verts.append(row)
    for ri in range(len(rings) - 1):
        for i in range(seg):
            j = (i + 1) % seg
            f = bm.faces.new((verts[ri][i], verts[ri][j], verts[ri + 1][j], verts[ri + 1][i]))
            zc = sum(v.co.z for v in f.verts) / 4
            f.material_index = field if zc < 30 and ri < 2 else (rock if zc > 260 else forest)
    # fjord water to the north
    k.box((0, -2600, -0.6), (5000, 2000, 0.2), water)
    # town between campus and the hills (outside the playable ground box, 1.25 km out)
    rnd = random.Random(11)
    for _ in range(420):
        a = rnd.uniform(0, 2 * math.pi)
        r = rnd.uniform(1230, 1650)
        x, y = math.cos(a) * r, math.sin(a) * r
        if -math.sin(a) > 0.8:
            continue  # fjord
        big = rnd.random() < 0.25
        w, d = (rnd.uniform(25, 60), rnd.uniform(12, 18)) if big else (rnd.uniform(8, 12), rnd.uniform(7, 10))
        h = rnd.uniform(12, 24) if big else rnd.uniform(5, 8)
        yaw = math.degrees(a) + rnd.choice((0, 90)) + rnd.uniform(-10, 10)
        wm = rnd.choice(walls)
        k.box((x, y, h / 2), (w, d, h), wm, rot=(0, 0, yaw))
        # window band so they read as buildings from far away
        for z in range(3, int(h) - 1, 3):
            k.box((x, y, z), (w + 0.1, d + 0.1, 1.1), window, rot=(0, 0, yaw))
        if not big:
            rr = math.radians(yaw)
            # gable roof as two slabs
            for sgn in (-1, 1):
                ox, oy = -math.sin(rr) * sgn * d / 4, math.cos(rr) * sgn * d / 4
                k.box((x + ox, y + oy, h + 1.0), (w + 0.6, d / 2 + 0.8, 0.25), rnd.choice(roofs), rot=(sgn * 28, 0, yaw))
    k.finish(also_blend=False)


if __name__ == "__main__":
    main()
