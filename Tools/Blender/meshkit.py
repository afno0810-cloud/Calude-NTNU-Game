"""Fast mesh building for big models (buildings, props): everything goes straight into one bmesh.

Coordinates passed to MeshKit are in UNREAL space (meters, X east/forward, Y south/right, Z up).
finish() mirrors Y so that, after Unreal's FBX import flips Y back, the model ends up exactly as written.
"""
import math
import os
import sys

import bpy  # noqa: F401  (must be imported before bmesh)
import bmesh
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gs_common import export, material, reset_scene  # noqa: E402


class MeshKit:
    def __init__(self, name):
        reset_scene()
        self.name = name
        self.bm = bmesh.new()
        self.mats = []
        self.mat_index = {}

    def mat(self, name, color, metallic=0.0, rough=0.7):
        if name not in self.mat_index:
            self.mat_index[name] = len(self.mats)
            self.mats.append(material(name, color, metallic, rough))
        return self.mat_index[name]

    def _add(self, verts, faces, mi, matrix):
        vs = [self.bm.verts.new(matrix @ Vector(v)) for v in verts]
        for f in faces:
            face = self.bm.faces.new([vs[i] for i in f])
            face.material_index = mi
        return vs

    def box(self, c, size, mi, rot=(0, 0, 0)):
        """Box centered at c with size (sx, sy, sz); rot in degrees (x, y, z)."""
        sx, sy, sz = (s / 2 for s in size)
        v = [(-sx, -sy, -sz), (sx, -sy, -sz), (sx, sy, -sz), (-sx, sy, -sz),
             (-sx, -sy, sz), (sx, -sy, sz), (sx, sy, sz), (-sx, sy, sz)]
        f = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
        m = Matrix.Translation(c) @ Matrix.Rotation(math.radians(rot[2]), 4, "Z") @ \
            Matrix.Rotation(math.radians(rot[1]), 4, "Y") @ Matrix.Rotation(math.radians(rot[0]), 4, "X")
        self._add(v, f, mi, m)

    def box_minmax(self, lo, hi, mi):
        c = [(a + b) / 2 for a, b in zip(lo, hi)]
        s = [abs(b - a) for a, b in zip(lo, hi)]
        if min(s) <= 1e-4:
            return
        self.box(c, s, mi)

    def cyl(self, c, r, h, mi, verts=16, r_top=None, rot_z=0.0):
        """Vertical cylinder / cone / pyramid (verts=4) standing on c."""
        r_top = r if r_top is None else r_top
        v, f = [], []
        for i in range(verts):
            a = 2 * math.pi * i / verts + math.radians(rot_z)
            v.append((math.cos(a) * r, math.sin(a) * r, 0))
        for i in range(verts):
            a = 2 * math.pi * i / verts + math.radians(rot_z)
            v.append((math.cos(a) * r_top, math.sin(a) * r_top, h))
        f.append(tuple(range(verts - 1, -1, -1)))
        if r_top > 1e-5:
            f.append(tuple(range(verts, 2 * verts)))
            for i in range(verts):
                j = (i + 1) % verts
                f.append((i, j, verts + j, verts + i))
        else:
            v = v[:verts] + [(0, 0, h)]
            for i in range(verts):
                j = (i + 1) % verts
                f.append((i, j, verts))
        self._add(v, f, mi, Matrix.Translation(c))

    def cyl_axis(self, c, r, length, mi, axis="X", verts=12):
        """Cylinder lying along X or Y, centered at c."""
        self_rot = Matrix.Rotation(math.radians(90), 4, "Y") if axis == "X" else Matrix.Rotation(math.radians(-90), 4, "X")
        v, f = [], []
        for k, z in enumerate((-length / 2, length / 2)):
            for i in range(verts):
                a = 2 * math.pi * i / verts
                v.append((math.cos(a) * r, math.sin(a) * r, z))
        f.append(tuple(range(verts - 1, -1, -1)))
        f.append(tuple(range(verts, 2 * verts)))
        for i in range(verts):
            j = (i + 1) % verts
            f.append((i, j, verts + j, verts + i))
        self._add(v, f, mi, Matrix.Translation(c) @ self_rot)

    def prism_x(self, x0, x1, profile_yz, mi):
        """Extrude a (y, z) profile (counter-clockwise seen from +X) from x0 to x1. Roofs, gables."""
        n = len(profile_yz)
        v = [(x0, y, z) for y, z in profile_yz] + [(x1, y, z) for y, z in profile_yz]
        f = [tuple(range(n - 1, -1, -1)), tuple(range(n, 2 * n))]
        for i in range(n):
            j = (i + 1) % n
            f.append((i, j, n + j, n + i))
        self._add(v, f, mi, Matrix.Identity(4))

    def prism_y(self, y0, y1, profile_xz, mi):
        n = len(profile_xz)
        v = [(x, y0, z) for x, z in profile_xz] + [(x, y1, z) for x, z in profile_xz]
        f = [tuple(range(n)), tuple(range(2 * n - 1, n - 1, -1))]
        for i in range(n):
            j = (i + 1) % n
            f.append((j, i, n + i, n + j))
        self._add(v, f, mi, Matrix.Identity(4))

    def ramp(self, p0, p1, width, thick, mi):
        """Sloped slab whose top surface goes from p0 to p1 (both (x, y, z) at the centerline)."""
        p0, p1 = Vector(p0), Vector(p1)
        d = p1 - p0
        length = d.length
        yaw = math.degrees(math.atan2(d.y, d.x))
        pitch = math.degrees(math.atan2(d.z, Vector((d.x, d.y, 0)).length))
        c = (p0 + p1) / 2
        # move down by half thickness, perpendicular to the slope
        n = Vector((-math.sin(math.radians(pitch)) * math.cos(math.radians(yaw)),
                    -math.sin(math.radians(pitch)) * math.sin(math.radians(yaw)), math.cos(math.radians(pitch))))
        c = c - n * thick / 2
        self.box(c, (length, width, thick), mi, rot=(0, -pitch, yaw))

    def arch(self, cx, cy, z_spring, span, depth, thick, mi, along="X", segs=9):
        """Semicircular arch (made of small blocks) springing at z_spring, spanning `span` meters."""
        r = span / 2
        for i in range(segs):
            a0 = math.pi * i / segs
            a1 = math.pi * (i + 1) / segs
            am = (a0 + a1) / 2
            seg_len = 2 * (r + thick / 2) * math.sin((a1 - a0) / 2) + 0.02
            off = math.cos(am) * (r + thick / 2)
            z = z_spring + math.sin(am) * (r + thick / 2)
            ang = math.degrees(am) - 90
            if along == "X":
                self.box((cx + off, cy, z), (seg_len, depth, thick), mi, rot=(0, ang, 0))
            else:
                self.box((cx, cy + off, z), (depth, seg_len, thick), mi, rot=(-ang, 0, 0))

    def slab_with_holes(self, x0, x1, y0, y1, z_top, thick, holes, mi):
        xs = sorted({x0, x1, *[h[0] for h in holes], *[h[1] for h in holes]})
        ys = sorted({y0, y1, *[h[2] for h in holes], *[h[3] for h in holes]})
        xs = [x for x in xs if x0 <= x <= x1]
        ys = [y for y in ys if y0 <= y <= y1]
        for i in range(len(xs) - 1):
            for j in range(len(ys) - 1):
                cx, cy = (xs[i] + xs[i + 1]) / 2, (ys[j] + ys[j + 1]) / 2
                if any(h[0] <= cx <= h[1] and h[2] <= cy <= h[3] for h in holes):
                    continue
                self.box_minmax((xs[i], ys[j], z_top - thick), (xs[i + 1], ys[j + 1], z_top), mi)

    def _box_uvs(self, tile=2.0):
        """World-space box projection: 1 UV unit = `tile` meters, so tiling Megascans materials look right."""
        uv = self.bm.loops.layers.uv.verify()
        self.bm.normal_update()
        for f in self.bm.faces:
            n = f.normal
            ax = max(range(3), key=lambda i: abs(n[i]))
            for loop in f.loops:
                c = loop.vert.co
                if ax == 0:
                    loop[uv].uv = (c.y / tile, c.z / tile)
                elif ax == 1:
                    loop[uv].uv = (c.x / tile, c.z / tile)
                else:
                    loop[uv].uv = (c.x / tile, c.y / tile)

    def finish(self, also_blend=True, also_glb=False):
        bmesh.ops.remove_doubles(self.bm, verts=self.bm.verts, dist=1e-5)
        # Mirror Y (Unreal flips it back on import) and fix the winding.
        for v in self.bm.verts:
            v.co.y = -v.co.y
        bmesh.ops.reverse_faces(self.bm, faces=self.bm.faces)
        self._box_uvs()
        me = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(me)
        self.bm.free()
        for m in self.mats:
            me.materials.append(m)
        obj = bpy.data.objects.new(self.name, me)
        bpy.context.collection.objects.link(obj)
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        export(self.name, also_blend, also_glb)
        return obj
