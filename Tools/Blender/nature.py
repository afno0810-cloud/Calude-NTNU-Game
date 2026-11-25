"""Trees and bushes for Gloshaugen:  python Tools/Blender/nature.py

Trunks and branches use the Bark / BirchBark materials, the leaves are crossed cards that show cells of the
leaf atlas T_GS_Leaves_D (Tools/Textures/gen_textures.py), drawn with the masked two-sided foliage material.
Exports SM_Tree_Maple, SM_Tree_Birch, SM_Tree_Spruce, SM_Tree_SpruceSmall, SM_Bush, SM_Hedge.
No collision in the mesh: GSCampusBuilder adds an invisible trunk collider instead, so bullets pass leaves.
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from meshkit import MeshKit  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402


class NatureKit(MeshKit):
    def __init__(self, name):
        super().__init__(name)
        self.uv = self.bm.loops.layers.uv.verify()
        self.card_faces = set()

    def card(self, c, w, h, yaw, pitch, mi, cell, cells=4, roll=0.0):
        """One leaf card (quad) centred at c, showing atlas cell `cell` (0..cells*cells-1)."""
        m = (Matrix.Translation(c) @ Matrix.Rotation(math.radians(yaw), 4, "Z") @
             Matrix.Rotation(math.radians(pitch), 4, "Y") @ Matrix.Rotation(math.radians(roll), 4, "X"))
        v = [(-w / 2, 0, -h / 2), (w / 2, 0, -h / 2), (w / 2, 0, h / 2), (-w / 2, 0, h / 2)]
        vs = [self.bm.verts.new(m @ Vector(p)) for p in v]
        f = self.bm.faces.new(vs)
        f.material_index = mi
        cx, cy = cell % cells, cell // cells
        u0, v0 = cx / cells, 1 - (cy + 1) / cells
        s = 1 / cells
        for loop, (uu, vv) in zip(f.loops, ((u0, v0), (u0 + s, v0), (u0 + s, v0 + s), (u0, v0 + s))):
            loop[self.uv].uv = (uu, vv)
        self.card_faces.add(f)

    def cross(self, c, size, mi, rnd):
        """Two crossed cards (looks full from every side)."""
        cell = rnd.randrange(16)
        yaw = rnd.uniform(0, 180)
        tilt = rnd.uniform(-25, 25)
        self.card(c, size, size, yaw, tilt, mi, cell)
        self.card(c, size, size, yaw + 90, -tilt, mi, (cell + 5) % 16)
        self.card(c, size * 0.9, size * 0.9, yaw + 45, 0, mi, (cell + 9) % 16, roll=90)

    def branch(self, p0, p1, r0, r1, mi, verts=7):
        """Tapered cylinder from p0 to p1."""
        p0, p1 = Vector(p0), Vector(p1)
        d = p1 - p0
        length = d.length
        if length < 1e-4:
            return
        z = d.normalized()
        x = z.cross(Vector((0, 0, 1)))
        if x.length < 1e-3:
            x = Vector((1, 0, 0))
        x.normalize()
        y = z.cross(x)
        ring0, ring1 = [], []
        for i in range(verts):
            a = 2 * math.pi * i / verts
            off = x * math.cos(a) + y * math.sin(a)
            ring0.append(self.bm.verts.new(p0 + off * r0))
            ring1.append(self.bm.verts.new(p1 + off * r1))
        for i in range(verts):
            j = (i + 1) % verts
            f = self.bm.faces.new((ring0[i], ring0[j], ring1[j], ring1[i]))
            f.material_index = mi
        f = self.bm.faces.new(list(reversed(ring0)))
        f.material_index = mi
        f = self.bm.faces.new(ring1)
        f.material_index = mi

    def _box_uvs(self, tile=2.0):
        uv = self.uv
        self.bm.normal_update()
        for f in self.bm.faces:
            if f in self.card_faces:
                continue
            n = f.normal
            ax = max(range(3), key=lambda i: abs(n[i]))
            for loop in f.loops:
                c = loop.vert.co
                # bark: wrap around the trunk, 1 m per UV
                if ax == 2:
                    loop[uv].uv = (c.x, c.y)
                else:
                    loop[uv].uv = (math.atan2(c.y, c.x) / math.pi * 0.6, c.z)


def maple(seed=1):
    k = NatureKit("SM_Tree_Maple")
    rnd = random.Random(seed)
    bark = k.mat("Bark", (0.2, 0.15, 0.11))
    leaf = k.mat("Leaves", (0.15, 0.3, 0.08))
    trunk_h = 3.2
    k.branch((0, 0, -0.3), (0.05, 0.02, trunk_h), 0.32, 0.22, bark, 10)
    tips = []
    for i in range(9):
        a = i * 2.4 + rnd.uniform(-0.3, 0.3)
        up = rnd.uniform(0.55, 0.85)
        L = rnd.uniform(2.8, 4.2)
        z0 = trunk_h - rnd.uniform(0, 1.0)
        p0 = Vector((0.03, 0.01, z0))
        d = Vector((math.cos(a) * (1 - up), math.sin(a) * (1 - up), up)).normalized()
        p1 = p0 + d * L
        k.branch(p0, p1, 0.13, 0.05, bark)
        # sub-branches
        for j in range(2):
            t = rnd.uniform(0.4, 0.8)
            q0 = p0 + d * L * t
            d2 = (d + Vector((rnd.uniform(-0.7, 0.7), rnd.uniform(-0.7, 0.7), rnd.uniform(0, 0.5)))).normalized()
            q1 = q0 + d2 * rnd.uniform(1.2, 2.0)
            k.branch(q0, q1, 0.05, 0.02, bark, 5)
            tips.append(q1)
        tips.append(p1)
    k.branch((0.05, 0.02, trunk_h), (0.1, 0.0, trunk_h + 3.5), 0.2, 0.05, bark)
    tips.append(Vector((0.1, 0, trunk_h + 3.6)))
    # canopy: leaf clusters around branch tips and filling the crown
    for tip in tips:
        for _ in range(7):
            c = tip + Vector((rnd.uniform(-1.1, 1.1), rnd.uniform(-1.1, 1.1), rnd.uniform(-0.9, 0.9)))
            k.cross(c, rnd.uniform(1.3, 1.9), leaf, rnd)
    for _ in range(40):
        r = rnd.uniform(0, 3.2)
        a = rnd.uniform(0, math.pi * 2)
        c = Vector((math.cos(a) * r, math.sin(a) * r, trunk_h + 2.4 + rnd.uniform(-1.6, 2.2) * (1 - r / 4)))
        k.cross(c, rnd.uniform(1.4, 2.0), leaf, rnd)
    k.finish()


def birch(seed=2):
    k = NatureKit("SM_Tree_Birch")
    rnd = random.Random(seed)
    bark = k.mat("BirchBark", (0.8, 0.8, 0.78))
    leaf = k.mat("Leaves", (0.2, 0.35, 0.1))
    H = 11.0
    pts = [Vector((0, 0, -0.3))]
    for i in range(1, 6):
        pts.append(Vector((rnd.uniform(-0.25, 0.25), rnd.uniform(-0.25, 0.25), H * i / 5)))
    for i in range(5):
        k.branch(pts[i], pts[i + 1], 0.2 * (1 - i / 6), 0.2 * (1 - (i + 1) / 6), bark, 8)
    for i in range(16):
        z = rnd.uniform(4.0, H - 0.5)
        a = rnd.uniform(0, math.pi * 2)
        L = (H - z) * 0.35 + 1.2
        p0 = Vector((0, 0, z))
        d = Vector((math.cos(a), math.sin(a), rnd.uniform(0.2, 0.6))).normalized()
        p1 = p0 + d * L
        k.branch(p0, p1, 0.05, 0.015, bark, 5)
        # drooping leaf curtains
        for t in (0.3, 0.55, 0.8, 1.0):
            c = p0 + d * L * t + Vector((0, 0, -0.5))
            k.cross(c, rnd.uniform(1.1, 1.5), leaf, rnd)
    for _ in range(55):
        z = rnd.uniform(4.5, H + 0.5)
        r = (H + 1 - z) * 0.3
        a = rnd.uniform(0, math.pi * 2)
        k.cross(Vector((math.cos(a) * r, math.sin(a) * r, z)), rnd.uniform(1.2, 1.6), leaf, rnd)
    k.finish()


def spruce(name="SM_Tree_Spruce", H=16.0, seed=3):
    k = NatureKit(name)
    rnd = random.Random(seed)
    bark = k.mat("Bark", (0.2, 0.15, 0.11))
    nee = k.mat("Needles", (0.08, 0.16, 0.08))
    k.branch((0, 0, -0.3), (0, 0, H), 0.28 * H / 16, 0.03, bark, 9)
    z = 1.2 * H / 16
    while z < H - 0.3:
        frac = (H - z) / H
        L = 0.4 + frac * 3.4 * H / 16
        n = max(4, int(5 + frac * 5))
        for i in range(n):
            a = 2 * math.pi * i / n + rnd.uniform(-0.2, 0.2) + z
            d = Vector((math.cos(a), math.sin(a), -0.25)).normalized()
            p0 = Vector((0, 0, z))
            k.branch(p0, p0 + d * L, 0.04, 0.01, bark, 4)
            # needle cards lying along the branch, drooping
            for t in (0.35, 0.75):
                c = p0 + d * L * t + Vector((0, 0, 0.05))
                yaw = math.degrees(a)
                k.card(c, L * 0.75, 0.9 * (0.6 + frac * 0.6), yaw, 0, nee, rnd.randrange(16), roll=80)
                k.card(c, L * 0.7, 0.7 * (0.6 + frac * 0.6), yaw, 0, nee, rnd.randrange(16), roll=20)
        z += 0.55 * H / 16
    k.cross(Vector((0, 0, H)), 0.9, nee, rnd)
    k.finish()


def bush(seed=4):
    k = NatureKit("SM_Bush")
    rnd = random.Random(seed)
    leaf = k.mat("Leaves", (0.15, 0.3, 0.08))
    bark = k.mat("Bark", (0.2, 0.15, 0.11))
    for i in range(6):
        a = i * 1.1
        k.branch((0, 0, 0), (math.cos(a) * 0.6, math.sin(a) * 0.6, 1.1), 0.03, 0.01, bark, 4)
    for _ in range(26):
        u = rnd.uniform(0, math.pi * 2)
        v = rnd.uniform(0.1, 1.0)
        c = Vector((math.cos(u) * 0.8 * v, math.sin(u) * 0.8 * v, 0.35 + rnd.uniform(0, 0.9) * (1.1 - v * 0.5)))
        k.cross(c, rnd.uniform(0.7, 1.0), leaf, rnd)
    k.finish()


def hedge(seed=5):
    """4 m long trimmed hedge, 1.2 m tall."""
    k = NatureKit("SM_Hedge")
    rnd = random.Random(seed)
    leaf = k.mat("Leaves", (0.15, 0.3, 0.08))
    core = k.mat("LeafCore", (0.05, 0.1, 0.03))
    k.box((0, 0, 0.55), (3.9, 0.7, 1.05), core)
    for _ in range(90):
        c = Vector((rnd.uniform(-1.9, 1.9), rnd.uniform(-0.4, 0.4), rnd.uniform(0.15, 1.15)))
        k.cross(c, rnd.uniform(0.45, 0.6), leaf, rnd)
    k.finish()


if __name__ == "__main__":
    maple()
    birch()
    spruce()
    spruce("SM_Tree_SpruceSmall", 8.0, 6)
    bush()
    hedge()
    print("trees done")
