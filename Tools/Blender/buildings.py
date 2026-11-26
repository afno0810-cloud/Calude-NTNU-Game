"""Detailed models of every enterable Gloshaugen building except Hovedbygningen (which has its own script).

    blender --background --python Tools/Blender/buildings.py -- [Name ...]

Scenery buildings (bBackdrop in GSCampus.h) are only rebuilt when they are named.

Reads the building list straight from Source/Gloshaugen/Public/GSCampus.h and rebuilds EXACTLY the walkable
layout that AGSCampusBuilder makes from boxes (outer walls with the same window and door openings, floor slabs,
stairwell opening and ramps), so loot, furniture, PCs and cameras still line up. On top of that it adds what
makes a building look real: window frames with mullions and sills, lintels, plinth, floor bands, cornice,
parapets or slate roofs with chimneys, entrance canopies, interior plaster and floor finishes, railings,
rooftop machinery.

Exports SM_Bygg_<Name> (walls etc., per-triangle collision) and SM_Glass_<Name> (window panes, no collision).
"""
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from meshkit import MeshKit  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

# Same constants as GSCampusBuilder.cpp
WALL_T = 0.4
SLAB_T = 0.3
WINDOW_W = 1.8
MIN_PIER = 1.4
RAMP_W = 2.6
RAMP_ANGLE = 25.0


def safe(name):
    return re.sub(r"[^A-Za-z0-9]", "_", name)


def read_buildings():
    src = open(os.path.join(ROOT, "Source", "Gloshaugen", "Public", "GSCampus.h"), encoding="utf-8").read()
    pat = re.compile(r'\{\s*TEXT\("([^"]+)"\),\s*([-\d.]+)f,\s*([-\d.]+)f,\s*([-\d.]+)f,\s*([-\d.]+)f,\s*([-\d.]+)f,'
                     r'\s*([-\d.]+)f,\s*([-\d.]+)f,\s*(\w+),\s*(true|false)\s*(?:,\s*(true|false)\s*)?\}')
    out = []
    for m in pat.finditer(src):
        out.append(dict(name=m.group(1), x=float(m.group(2)), y=float(m.group(3)), sx=float(m.group(4)),
                        sy=float(m.group(5)), yaw=float(m.group(6)), h=float(m.group(7)), fh=float(m.group(8)),
                        color=m.group(9), enter=m.group(10) == "true", backdrop=m.group(11) == "true"))
    return out


# Look of each building: wall, trim, plinth, frame material, roof type, extras
STYLES = {
    "stone": dict(wall="Stone", trim="Trim", plinth="StoneDark", frame="PaintedFrame", roof="gable", bands=True, cornice=True),
    "brick": dict(wall="Brick", trim="Trim", plinth="StoneDark", frame="PaintedFrame", roof="gable", bands=True, cornice=True),
    "brickbrown": dict(wall="BrickBrown", trim="ConcreteSmooth", plinth="Concrete", frame="MetalDark", roof="flat", bands=False, cornice=False),
    "brickyellow": dict(wall="BrickYellow", trim="ConcreteSmooth", plinth="Concrete", frame="MetalDark", roof="flat", bands=True, cornice=False),
    "tower": dict(wall="Concrete", trim="ConcreteSmooth", plinth="Concrete", frame="Metal", roof="flat", bands=True, cornice=False),
    "modern": dict(wall="PlasterWhite", trim="ConcreteSmooth", plinth="Concrete", frame="MetalDark", roof="flat", bands=False, cornice=False),
    "light": dict(wall="ConcreteSmooth", trim="Concrete", plinth="Concrete", frame="Metal", roof="flat", bands=True, cornice=False),
    "glass": dict(wall="CurtainPanel", trim="MetalDark", plinth="Concrete", frame="MetalDark", roof="flat", bands=False, cornice=False),
}
BUILDING_STYLE = {
    "Hovedbygningen vest": "stone", "Hovedbygningen oest": "stone",
    "Gamle fysikk": "brick", "Gamle kjemi": "brick", "Kjelhuset": "brick", "Bergbygget": "brick",
    "Elektrobygget": "brickbrown", "Realfagbygget": "brickyellow",
    "Sentralbygg 1": "tower", "Sentralbygg 2": "tower", "Varmeteknisk": "tower",
    "IT-bygget": "modern", "Kjemiblokkene": "light",
    "Stripa": "glass", "Idrettsbygget": "glass",
}


class Mats:
    def __init__(self, k, st):
        self.k = k
        col = {
            "Stone": (0.42, 0.42, 0.41), "StoneDark": (0.26, 0.26, 0.26), "Trim": (0.64, 0.62, 0.57),
            "Brick": (0.42, 0.17, 0.11), "BrickBrown": (0.27, 0.15, 0.1), "BrickYellow": (0.62, 0.52, 0.36),
            "Concrete": (0.55, 0.54, 0.52), "ConcreteSmooth": (0.6, 0.59, 0.57), "PlasterWhite": (0.86, 0.85, 0.82),
            "Metal": (0.56, 0.57, 0.58), "MetalDark": (0.18, 0.18, 0.19), "CurtainPanel": (0.2, 0.22, 0.24),
            "PaintedFrame": (0.88, 0.87, 0.84),
        }
        self.wall = k.mat(st["wall"] if st["wall"] != "CurtainPanel" else "MetalDark", col[st["wall"]],
                          metallic=0.6 if st["wall"] == "CurtainPanel" else 0.0)
        self.trim = k.mat(st["trim"], col[st["trim"]])
        self.plinth = k.mat(st["plinth"], col[st["plinth"]])
        fr = st["frame"]
        self.frame = k.mat("PlasticWhite" if fr == "PaintedFrame" else fr, col[fr], metallic=0.0 if fr == "PaintedFrame" else 0.8, rough=0.4)
        self.inner = k.mat("PlasterWhite", (0.86, 0.85, 0.82))
        self.floor = k.mat("Terrazzo", (0.66, 0.65, 0.62), rough=0.3)
        self.slab = k.mat("ConcreteSmooth", (0.6, 0.59, 0.57))
        self.ramp = k.mat("Concrete", (0.55, 0.54, 0.52))
        self.slate = k.mat("Slate", (0.16, 0.17, 0.2))
        self.roof = k.mat("Gravel", (0.4, 0.39, 0.37))
        self.metal = k.mat("Metal", (0.56, 0.57, 0.58), metallic=0.9, rough=0.35)
        self.dark = k.mat("MetalDark", (0.18, 0.18, 0.19), metallic=0.8, rough=0.4)
        self.copper = k.mat("Copper", (0.3, 0.55, 0.45), metallic=0.4)
        self.brick = k.mat("Brick", col["Brick"])
        self.rubber = k.mat("Rubber", (0.05, 0.05, 0.05))
        self.light = k.mat("LightPanel", (1.0, 0.98, 0.92))


class Building:
    def __init__(self, b):
        self.b = b
        self.st = STYLES[BUILDING_STYLE.get(b["name"], "light")]
        self.k = MeshKit(f"SM_Bygg_{safe(b['name'])}")
        self.m = Mats(self.k, self.st)
        self.panes = []  # (center, size) of window glass, exported separately
        self.hx, self.hy = b["sx"] / 2, b["sy"] / 2
        self.floors = max(1, int(math.floor(b["h"] / b["fh"] + 0.01)))

    # ---------------------------------------------------------------- walls (mirrors AGSCampusBuilder::BuildWallSide)
    def wall_side(self, cx, cy, length, along_x, outward):
        """outward: +1/-1, which side (along the wall normal) is outside."""
        b, k, m = self.b, self.k, self.m
        H, FH, F = b["h"], b["fh"], self.floors

        def P(s, z):  # point on the wall centre line
            return (cx + s, cy, z) if along_x else (cx, cy + s, z)

        def piece(s, w, z0, z1, mat=None):
            if w <= 0.01 or z1 - z0 <= 0.01:
                return
            c = P(s, (z0 + z1) / 2)
            size = (w, WALL_T, z1 - z0) if along_x else (WALL_T, w, z1 - z0)
            k.box(c, size, m.wall if mat is None else mat)
            # interior plaster lining, 2 cm on the inside face
            off = -outward * (WALL_T / 2 + 0.01)
            ci = (c[0], c[1] + off, c[2]) if along_x else (c[0] + off, c[1], c[2])
            si = (w, 0.02, z1 - z0) if along_x else (0.02, w, z1 - z0)
            k.box(ci, si, m.inner)

        def proud(s, w, z0, z1, depth, mat):
            """Something sticking out of the outside face (bands, sills, lintels)."""
            off = outward * (WALL_T / 2 + depth / 2)
            c = P(s, (z0 + z1) / 2)
            c = (c[0], c[1] + off, c[2]) if along_x else (c[0] + off, c[1], c[2])
            size = (w, depth, z1 - z0) if along_x else (depth, w, z1 - z0)
            k.box(c, size, mat)

        def inset(s, w, z0, z1, depth, pos, mat):
            """Box inside the opening at `pos` meters from the outer face (frames)."""
            off = outward * (WALL_T / 2 - pos)
            c = P(s, (z0 + z1) / 2)
            c = (c[0], c[1] + off, c[2]) if along_x else (c[0] + off, c[1], c[2])
            size = (w, depth, z1 - z0) if along_x else (depth, w, z1 - z0)
            k.box(c, size, mat)

        nw = int(math.floor((length - MIN_PIER) / (WINDOW_W + MIN_PIER)))
        if nw < 1:
            piece(0, length, 0, H)
            return
        pier = (length - nw * WINDOW_W) / (nw + 1)
        doors = set()
        if length < 60:
            doors.add(nw // 2)
        else:
            doors.add(nw // 4)
            doors.add((nw * 3) // 4)

        glass_style = self.st["wall"] == "CurtainPanel"
        for f in range(F):
            zb = f * FH
            zt = H if f == F - 1 else (f + 1) * FH
            zsill = zb + 1.0
            zlin = zb + min(2.7, FH - 0.6)
            for i in range(nw + 1):
                s = -length / 2 + pier / 2 + i * (pier + WINDOW_W)
                piece(s, pier, zb, zlin)
                if glass_style:  # vertical aluminium fins on the curtain wall
                    proud(s, 0.08, zb, zt, 0.25, m.dark)
            for i in range(nw):
                s = -length / 2 + pier + WINDOW_W / 2 + i * (pier + WINDOW_W)
                is_door = f == 0 and i in doors
                if not is_door:
                    piece(s, WINDOW_W, zb, zsill)
                    z0, z1 = zsill, zlin
                    # frame (outer ring), mullion, transom
                    fw, fd, fp = 0.07, 0.1, 0.12
                    inset(s, WINDOW_W, z0, z0 + fw, fd, fp, m.frame)
                    inset(s, WINDOW_W, z1 - fw, z1, fd, fp, m.frame)
                    inset(s - WINDOW_W / 2 + fw / 2, fw, z0, z1, fd, fp, m.frame)
                    inset(s + WINDOW_W / 2 - fw / 2, fw, z0, z1, fd, fp, m.frame)
                    inset(s, 0.06, z0, z1, fd, fp, m.frame)
                    zt_ = z0 + (z1 - z0) * 0.72
                    inset(s, WINDOW_W, zt_ - 0.03, zt_ + 0.03, fd, fp, m.frame)
                    self.pane(P(s, (z0 + z1) / 2), WINDOW_W - 0.1, z1 - z0 - 0.1, along_x, outward, fp)
                    # sill and lintel
                    proud(s, WINDOW_W + 0.2, z0 - 0.06, z0, 0.1, m.trim)
                    if self.st["bands"] and not glass_style:
                        proud(s, WINDOW_W + 0.3, z1, z1 + 0.25, 0.04, m.trim)
                else:
                    # entrance: frame, canopy, light above, door mat
                    zd = zlin
                    inset(s - WINDOW_W / 2 + 0.05, 0.1, 0, zd, 0.15, 0.1, m.frame)
                    inset(s + WINDOW_W / 2 - 0.05, 0.1, 0, zd, 0.15, 0.1, m.frame)
                    inset(s, WINDOW_W, zd - 0.1, zd, 0.15, 0.1, m.frame)
                    self.pane(P(s, zd - 0.35), WINDOW_W - 0.2, 0.45, along_x, outward, 0.1)
                    proud(s, WINDOW_W + 1.6, zd + 0.3, zd + 0.48, 1.6, m.dark)
                    proud(s, 0.3, zd + 0.05, zd + 0.15, 0.08, m.light)
                    proud(s, WINDOW_W + 0.6, 0.0, 0.02, 1.2, m.rubber)
            piece(0, length, zlin, zt)
            if self.st["bands"] and f > 0:
                proud(0, length + 0.1, zb - 0.1, zb + 0.15, 0.1, m.trim)
            if glass_style:
                proud(0, length, zlin + 0.1, zlin + 0.2, 0.15, m.dark)
        # plinth along the bottom (with gaps at the doors)
        door_s = [(-length / 2 + pier + WINDOW_W / 2 + i * (pier + WINDOW_W)) for i in doors]
        edges = sorted([-length / 2] + [d - WINDOW_W / 2 for d in door_s] + [d + WINDOW_W / 2 for d in door_s] + [length / 2])
        for a, c in zip(edges[0::2], edges[1::2]):
            proud((a + c) / 2, c - a, 0, 0.6, 0.06, m.plinth)
        if self.st["cornice"]:
            proud(0, length + 0.6, H - 0.45, H - 0.15, 0.3, m.trim)
            proud(0, length + 0.8, H - 0.15, H, 0.45, m.trim)

    def pane(self, c, w, h, along_x, outward, pos):
        off = outward * (WALL_T / 2 - pos - 0.03)
        c = (c[0], c[1] + off, c[2]) if along_x else (c[0] + off, c[1], c[2])
        size = (w, 0.012, h) if along_x else (0.012, w, h)
        self.panes.append((c, size))

    # ---------------------------------------------------------------- floors and ramps (mirrors BuildBuilding)
    def floors_and_ramps(self):
        b, k, m = self.b, self.k, self.m
        FH = b["fh"]
        ramp_len = FH / math.tan(math.radians(RAMP_ANGLE))
        ix0, ix1 = -self.hx + WALL_T, self.hx - WALL_T
        iy0, iy1 = -self.hy + WALL_T, self.hy - WALL_T
        ox0 = ix1 - ramp_len
        oy0 = iy1 - RAMP_W
        stairs = (ix1 - ix0) > ramp_len + 2 and (iy1 - iy0) > RAMP_W + 2
        # ground floor finish
        k.box_minmax((ix0, iy0, 0.0), (ix1, iy1, 0.02), m.floor)
        for kf in range(1, self.floors):
            zt = kf * FH
            if stairs:
                k.box_minmax((ix0, iy0, zt - SLAB_T), (ox0, iy1, zt), m.slab)
                k.box_minmax((ox0, iy0, zt - SLAB_T), (ix1, oy0, zt), m.slab)
                k.box_minmax((ix0, iy0, zt), (ox0, iy1, zt + 0.02), m.floor)
                k.box_minmax((ox0, iy0, zt), (ix1, oy0, zt + 0.02), m.floor)
                z0 = (kf - 1) * FH
                length = math.hypot(ramp_len, FH)
                cxr, cyr = (ox0 + ix1) / 2, (oy0 + iy1) / 2
                k.box((cxr, cyr, (z0 + zt) / 2 - SLAB_T / 2), (length, RAMP_W - 0.1, SLAB_T), m.ramp, rot=(0, -RAMP_ANGLE, 0))
                # handrails: wall side runs the full length
                k.box((cxr, iy1 - 0.1, (z0 + zt) / 2 + 0.9), (length, 0.05, 0.05), m.metal, rot=(0, -RAMP_ANGLE, 0))
                # floor side stops 2.6 m before the top, so you can step off the ramp onto the floor (F2)
                t1 = max(0.0, (ix1 - 2.6 - ox0) / ramp_len)
                if t1 > 0.05:
                    k.box((ox0 + ramp_len * t1 / 2, oy0 + 0.1, z0 + FH * t1 / 2 + 0.9), (length * t1, 0.05, 0.05), m.metal,
                          rot=(0, -RAMP_ANGLE, 0))
                # railing along the edge of the opening on the upper floor
                rx0, rx1 = ox0 + 1.5, ix1 - 2.6
                if rx1 > rx0:
                    k.box_minmax((rx0, oy0 - 0.04, zt + 0.95), (rx1, oy0 + 0.04, zt + 1.02), m.metal)
                    x = rx0
                    while x <= rx1 + 0.01:
                        k.box_minmax((x - 0.02, oy0 - 0.02, zt), (x + 0.02, oy0 + 0.02, zt + 0.95), m.metal)
                        x += 1.2
            else:
                k.box_minmax((ix0, iy0, zt - SLAB_T), (ix1, iy1, zt), m.slab)
                k.box_minmax((ix0, iy0, zt), (ix1, iy1, zt + 0.02), m.floor)
        # ceiling lights: thin white panels under each slab (and the roof)
        for kf in range(1, self.floors + 1):
            zc = (kf * FH if kf < self.floors else b["h"]) - SLAB_T - 0.02
            x = ix0 + 3
            while x < ix1 - 2:
                y = iy0 + 3
                while y < iy1 - 2:
                    if not (stairs and x > ox0 - 1 and y > oy0 - 1):
                        k.box((x, y, zc), (1.2, 0.3, 0.03), m.light)
                    y += 5
                x += 5

    # ---------------------------------------------------------------- roof
    def roof(self):
        b, k, m = self.b, self.k, self.m
        H = b["h"]
        hx, hy = self.hx, self.hy
        k.box_minmax((-hx - 0.2, -hy - 0.2, H), (hx + 0.2, hy + 0.2, H + SLAB_T), m.slab)
        if self.st["roof"] == "gable":
            # pitched slate roof along the long side, with gable ends, chimneys and a copper ridge
            if hx >= hy:
                rh = hy * 0.75
                k.prism_x(-hx - 0.3, hx + 0.3, [(-hy - 0.5, H + SLAB_T), (hy + 0.5, H + SLAB_T), (0, H + SLAB_T + rh)], m.slate)
                k.box((0, 0, H + SLAB_T + rh), (b["sx"] + 0.7, 0.3, 0.15), m.copper)
                for i, x in enumerate((-hx * 0.6, 0, hx * 0.6)):
                    k.box((x, hy * 0.3, H + rh * 0.8), (0.9, 0.7, rh * 0.8 + 1.4), m.brick)
                    k.box((x, hy * 0.3, H + rh * 1.2 + 0.75), (1.1, 0.9, 0.15), m.trim)
                # dormers on both slopes
                n = max(1, int(b["sx"] / 9))
                for i in range(n):
                    x = -hx + (i + 0.5) * b["sx"] / n
                    for sgn in (-1, 1):
                        yy = sgn * hy * 0.55
                        zz = H + SLAB_T + rh * 0.45
                        k.box((x, yy, zz + 0.5), (1.6, 1.4, 1.4), m.wall)
                        k.prism_x(x - 0.95, x + 0.95, [(yy - 0.8, zz + 1.2), (yy + 0.8, zz + 1.2), (yy, zz + 1.9)], m.slate)
                        k.box((x, yy + sgn * 0.71, zz + 0.55), (1.0, 0.04, 0.9), m.frame)
            else:
                rh = hx * 0.75
                k.prism_y(-hy - 0.3, hy + 0.3, [(-hx - 0.5, H + SLAB_T), (hx + 0.5, H + SLAB_T), (0, H + SLAB_T + rh)], m.slate)
                k.box((0, 0, H + SLAB_T + rh), (0.3, b["sy"] + 0.7, 0.15), m.copper)
                for y in (-hy * 0.5, hy * 0.5):
                    k.box((hx * 0.3, y, H + rh * 0.8), (0.7, 0.9, rh * 0.8 + 1.4), m.brick)
        else:
            # flat roof: membrane, parapet with metal coping, machinery, skylights, antenna on tall ones
            k.box_minmax((-hx + 0.3, -hy + 0.3, H + SLAB_T), (hx - 0.3, hy - 0.3, H + SLAB_T + 0.05), m.roof)
            for (x0, y0, x1, y1) in ((-hx - 0.2, -hy - 0.2, hx + 0.2, -hy + 0.2), (-hx - 0.2, hy - 0.2, hx + 0.2, hy + 0.2),
                                     (-hx - 0.2, -hy - 0.2, -hx + 0.2, hy + 0.2), (hx - 0.2, -hy - 0.2, hx + 0.2, hy + 0.2)):
                k.box_minmax((x0, y0, H + SLAB_T), (x1, y1, H + SLAB_T + 1.0), m.wall if self.st["wall"] != "CurtainPanel" else m.trim)
                k.box_minmax((x0 - 0.05, y0 - 0.05, H + SLAB_T + 1.0), (x1 + 0.05, y1 + 0.05, H + SLAB_T + 1.08), m.metal)
            import random
            rnd = random.Random(b["name"])
            # machine room over the stairs
            k.box((hx - 6, hy - 4, H + SLAB_T + 1.6), (8, 5, 3.2), m.wall if self.st["wall"] != "CurtainPanel" else m.trim)
            k.box((hx - 6, hy - 4, H + SLAB_T + 3.25), (8.4, 5.4, 0.15), m.metal)
            for i in range(max(2, int(b["sx"] * b["sy"] / 500))):
                x = rnd.uniform(-hx + 3, hx - 10)
                y = rnd.uniform(-hy + 3, hy - 3)
                kind = rnd.random()
                if kind < 0.45:  # air handling unit with fans
                    k.box((x, y, H + SLAB_T + 0.8), (3.5, 1.8, 1.6), m.metal)
                    for j in (-1, 1):
                        k.cyl((x + j * 0.9, y, H + SLAB_T + 1.6), 0.6, 0.12, m.dark, verts=16)
                elif kind < 0.75:  # skylight
                    k.box((x, y, H + SLAB_T + 0.3), (2.4, 2.4, 0.6), m.dark)
                    self.panes.append(((x, y, H + SLAB_T + 0.61), (2.2, 2.2, 0.02)))
                else:  # ventilation pipes
                    k.cyl((x, y, H + SLAB_T), 0.25, 1.4, m.metal, verts=12)
                    k.cyl((x, y, H + SLAB_T + 1.4), 0.4, 0.2, m.metal, verts=12, r_top=0.1)
            if b["h"] > 30:
                k.cyl((-hx + 4, -hy + 4, H + SLAB_T), 0.12, 9.0, m.metal, verts=8)
                k.cyl((-hx + 4, -hy + 4, H + SLAB_T + 6.0), 0.5, 0.1, m.metal, verts=8)
                k.box((hx - 3, -hy + 3, H + SLAB_T + 2.5), (0.15, 1.2, 1.2), m.metal)  # dish-like panel
                # red aviation light
                k.box((-hx + 4, -hy + 4, H + SLAB_T + 9.1), (0.2, 0.2, 0.2), k.mat("PaintRed", (0.9, 0.05, 0.05)))

    def extras(self):
        b, k, m = self.b, self.k, self.m
        if b["name"] == "Kjelhuset":
            # the boiler house chimney: 38 m brick stack with iron bands
            cx, cy = -self.hx - 2.5, self.hy - 3
            k.cyl((cx, cy, 0), 1.6, 38, m.brick, verts=20, r_top=1.1)
            for z in range(4, 38, 6):
                r = 1.6 - (1.6 - 1.1) * z / 38 + 0.04
                k.cyl((cx, cy, z), r, 0.15, m.dark, verts=20)
            k.cyl((cx, cy, 38), 1.25, 0.6, m.trim, verts=20)
        if self.st["wall"] == "CurtainPanel":
            # roof overhang for glass buildings
            k.box_minmax((-self.hx - 1.2, -self.hy - 1.2, b["h"] + SLAB_T), (self.hx + 1.2, self.hy + 1.2, b["h"] + SLAB_T + 0.35), m.dark)
        # signs above the main doors
        sign = k.mat("NTNUBlue", (0.0, 0.25, 0.55))
        k.box((0, -self.hy - 0.08, min(b["fh"] - 0.3, 3.6)), (min(8, b["sx"] * 0.4), 0.08, 0.6), sign)

    def build(self):
        b = self.b
        hx, hy = self.hx, self.hy
        self.wall_side(0, -hy + WALL_T / 2, b["sx"], True, -1)
        self.wall_side(0, hy - WALL_T / 2, b["sx"], True, +1)
        self.wall_side(-hx + WALL_T / 2, 0, b["sy"] - 2 * WALL_T, False, -1)
        self.wall_side(hx - WALL_T / 2, 0, b["sy"] - 2 * WALL_T, False, +1)
        self.floors_and_ramps()
        self.roof()
        self.extras()
        self.k.finish()
        # glass in its own mesh (no collision, translucent material)
        g = MeshKit(f"SM_Glass_{safe(b['name'])}")
        gm = g.mat("Glass", (0.06, 0.09, 0.1), metallic=0.0, rough=0.05)
        for c, size in self.panes:
            g.box(c, size, gm)
        g.finish()


def main():
    from gs_common import cli_args
    names = cli_args()
    for b in read_buildings():
        if b["name"] == "Hovedbygningen":  # has its own script
            continue
        named = b["name"] in names or safe(b["name"]) in names
        if not b["enter"] and not (b["backdrop"] and named):  # scenery is only rebuilt when named
            continue
        if names and not named:
            continue
        Building(b).build()
        print("built", b["name"])


if __name__ == "__main__":
    main()
