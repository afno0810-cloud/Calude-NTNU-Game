"""Helgasetr as real models (round 4b): the building parts that replace the blocks of round 4a, part by part.

    blender --background --python Tools/Blender/building_helgasetr.py -- [--render] [--force]

Reads Tools/Blender/helgasetr_plan.py (the one data source) and writes SourceArt/Models/<part>.fbx + .blend:
  SM_Bygg_Helgasetr_P<n>_<V|O>_<N|MN|MS|S>  structure of one level, split at X = 0 and Y = -18 / 0 / +18 (<= ca. 26 m, K17)
  SM_Bygg_Helgasetr_Trapper / _Trapperom    the atrium stairs (yellow) and the stair wells (concrete)
  SM_Bygg_Helgasetr_Overatriet              everything over Plan 4 inside the atrium (box roofs, Plan 5/6 rails) and the
                                            yellow flights + bridge that cross the atrium from Plan 5 to Plan 6 (scenery)
  SM_Bygg_Helgasetr_Tak / _Takfagverk       the opaque saw-tooth sheets + flat roof / the white steel truss (Warren)
  SM_Bygg_Helgasetr_Pynt                    decoration without collision: dark wood lamellas and white soffits on the slab
                                            edges, downlights, column rings, stair undersides, handrails
  SM_Glass_Helgasetr_Fasade / _Tak / _Overatriet   fixed glass
  SM_Prop_Helg_Hengelampe                   the big round ring lamp (Ø 2.4 m, glowing underside)

Every structure part is built from exactly the boxes that helgasetr_plan.model_part_of() gives it, so model and blocks
stand in the same place, and AGSHelgasetrBuilder can use the blocks of any part that is missing (K13).
A part is exported only when its geometry or materials changed (hash in <part>.hash, K18), so Unreal does not import it
again for nothing. --force exports everything. --render writes .claude/lag/grafikk/kontroll/r04b-*.png.
Coordinates: game meters (X east, Y south, Z up); MeshKit mirrors Y for the FBX (Unreal flips it back).
"""
import hashlib
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import bpy  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

import helgasetr_plan as H  # noqa: E402
from gs_common import OUT_DIR, cli_args  # noqa: E402
from meshkit import MeshKit  # noqa: E402

ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = cli_args()

# Preview colours in Blender (Unreal puts MI_GS_<slot> on each slot by name, see Content/Python/gs_materials.py)
COLORS = {
    "PlasterWhite_In": ((0.85, 0.85, 0.83), 0.0, 0.8), "ConcreteSmooth_In": ((0.58, 0.58, 0.56), 0.0, 0.7),
    "WoodLight_In": ((0.62, 0.48, 0.32), 0.0, 0.5), "Floor_In": ((0.33, 0.24, 0.17), 0.0, 0.6),
    "Terrazzo_In": ((0.74, 0.73, 0.70), 0.0, 0.4), "HelgOrange_In": ((0.95, 0.38, 0.06), 0.0, 0.6),
    "HelgYellow_In": ((1.0, 0.58, 0.05), 0.0, 0.45), "Wood_In": ((0.22, 0.14, 0.08), 0.0, 0.6),
    "MetalDark_In": ((0.04, 0.04, 0.045), 0.8, 0.4), "Glass": ((0.55, 0.7, 0.72), 0.0, 0.05),
    "LightPanel_In": ((1.0, 1.0, 0.95), 0.0, 0.5), "PaintRed_In": ((0.7, 0.05, 0.05), 0.0, 0.5),
    "PaintBlue_In": ((0.05, 0.15, 0.7), 0.0, 0.5), "PaintGreen_In": ((0.1, 0.35, 0.2), 0.0, 0.5),
    "Gunmetal_In": ((0.2, 0.21, 0.23), 0.8, 0.4), "Fabric_In": ((0.15, 0.45, 0.42), 0.0, 0.9),
    "Concrete_In": ((0.5, 0.5, 0.48), 0.0, 0.8), "PaintWhite_In": ((0.92, 0.92, 0.9), 0.0, 0.45),
}


# ---------------------------------------------------------------- geometry helpers (game space)
def rot_axes(yaw, pitch, roll):
    """Unit axes of an Unreal FRotator(Pitch, Yaw, Roll) (FRotationMatrix rows), so a box turns exactly as in C++."""
    p, y, r = (math.radians(v) for v in (pitch, yaw, roll))
    CP, SP, CY, SY, CR, SR = math.cos(p), math.sin(p), math.cos(y), math.sin(y), math.cos(r), math.sin(r)
    X = Vector((CP * CY, CP * SY, SP))
    Y = Vector((SR * SP * CY - CR * SY, SR * SP * SY + CR * CY, -SR * CP))
    Z = Vector((-(CR * SP * CY + SR * SY), CY * SR - CR * SP * SY, CR * CP))
    return X, Y, Z


BOX_FACES = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
SIGNS = [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1), (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]


def box_ue(K, c, s, mi, yaw=0.0, pitch=0.0, roll=0.0):
    X, Y, Z = rot_axes(yaw, pitch, roll)
    c = Vector(c)
    v = [c + X * (a * s[0] / 2) + Y * (b * s[1] / 2) + Z * (d * s[2] / 2) for a, b, d in SIGNS]
    K._add([tuple(p) for p in v], BOX_FACES, mi, Matrix.Identity(4))


def box_mm(K, x0, y0, z0, x1, y1, z1, mi):
    if x1 - x0 > 1e-4 and y1 - y0 > 1e-4 and z1 - z0 > 1e-4:
        K.box_minmax((x0, y0, z0), (x1, y1, z1), mi)


def smooth_from(K, n0):
    K.bm.faces.ensure_lookup_table()
    for i in range(n0, len(K.bm.faces)):
        K.bm.faces[i].smooth = True


def tube(K, a, b, r, mi, segs=10):
    n0 = len(K.bm.faces)
    K.tube([a, b], r, mi, segs=segs)
    smooth_from(K, n0)


def cylinder(K, c, r, h, mi, verts=20):
    n0 = len(K.bm.faces)
    K.cyl(c, r, h, mi, verts=verts)
    smooth_from(K, n0)


def ring(K, c, r_in, r_out, z0, z1, mi, segs=48):
    """Flat annulus solid around c from z0 to z1 (lamp housings, column rings)."""
    verts, faces = [], []
    for i in range(segs):
        a = 2 * math.pi * i / segs
        ca, sa = math.cos(a), math.sin(a)
        for rr in (r_in, r_out):
            for z in (z0, z1):
                verts.append((c[0] + ca * rr, c[1] + sa * rr, z))
    for i in range(segs):
        j = (i + 1) % segs
        a = [4 * i + k for k in range(4)]   # in-bottom, in-top, out-bottom, out-top
        b = [4 * j + k for k in range(4)]
        faces += [(a[2], b[2], b[3], a[3]), (a[1], b[1], b[0], a[0]), (a[3], b[3], b[1], a[1]), (a[0], b[0], b[2], a[2])]
    n0 = len(K.bm.faces)
    K._add(verts, faces, mi, Matrix.Identity(4))
    smooth_from(K, n0)


# ---------------------------------------------------------------- the data
PLAN = H.make_plan()
G = H.derive(PLAN)
PARTS, BOX_PART = H.model_parts(G)
OPS = {name: [] for name, _ in PARTS}
OPS["SM_Prop_Helg_Hengelampe"] = []


def op(part):
    def deco(fn):
        OPS[part].append(fn)
        return fn
    return deco


def is_column(b):
    return b["mat"] == "ConcreteSmooth_In" and abs(b["s"][0] - 0.5) < 1e-3 and abs(b["s"][1] - 0.5) < 1e-3 \
        and abs(abs(b["c"][0]) - 13.54) < 0.01 and not (b["yaw"] or b["pitch"] or b["roll"])


def is_stair_guard(b):
    return b["bit"] == H.B_STAIRS and b["mat"] == "MetalDark_In" and abs(b["s"][1] - 0.04) < 1e-3


def emit_box(K, b):
    mat = b["mat"]
    if is_stair_guard(b):
        mat = "HelgYellow_In"            # the solid yellow side panels of the atrium stairs (bilde 03, 17)
    mi = K.mat(mat, *COLORS[mat])
    if is_column(b):
        (cx, cy, cz), (_, _, sz) = b["c"], b["s"]
        cylinder(K, (cx, cy, cz - sz / 2), 0.25, sz, mi, verts=24)
        return
    box_ue(K, b["c"], b["s"], mi, b["yaw"], b["pitch"], b["roll"])


for k, b in enumerate(G["boxes"]):
    p = BOX_PART[k]
    if p != 255:
        OPS[PARTS[p][0]].append(lambda K, b=b: emit_box(K, b))
# The roof truss is drawn new (round tubes, Warren trusses) instead of its square blocks.
TRUSS = H.PART_PREFIX + "Takfagverk"
OPS[TRUSS] = []
PYNT = H.PART_PREFIX + "Pynt"
OVER = H.PART_PREFIX + "Overatriet"
OVER_GLASS = H.GLASS_PREFIX + "Overatriet"


def floor_at(lv, x, y):
    return any(r[0] - 1e-3 <= x <= r[2] + 1e-3 and r[1] - 1e-3 <= y <= r[3] + 1e-3 for r in G["floors"][lv])


# ---------------------------------------------------------------- slab edges: lamellas, soffit, downlights (Pynt)
EDGES = []   # (lv, o, c, a, b, s) s = +1/-1: which side of the edge line is the hole
for w in G["walls"]:
    if w["k"] not in ("rail", "glass") or w["lv"] == H.P1:
        continue
    lv, o, c = w["lv"], w["o"], w["c"]
    for a, b_, h in w["solid"]:
        if h is not None or b_ - a < 0.2:
            continue
        t = (a + b_) / 2
        pa = (c - 0.15, t) if o == "V" else (t, c - 0.15)
        pb = (c + 0.15, t) if o == "V" else (t, c + 0.15)
        fa, fb = floor_at(lv, *pa), floor_at(lv, *pb)
        if fa == fb:
            continue                     # floor on both sides (glass rooms on the mezzanine) or none
        EDGES.append((lv, o, c, a, b_, 1.0 if fa else -1.0))


@op(PYNT)
def slab_edges(K):
    wood = K.mat("Wood_In", *COLORS["Wood_In"])
    back = K.mat("MetalDark_In", *COLORS["MetalDark_In"])
    white = K.mat("PlasterWhite_In", *COLORS["PlasterWhite_In"])
    glow = K.mat("LightPanel_In", *COLORS["LightPanel_In"])
    for lv, o, c, a, b, s in EDGES:
        z0 = H.LEVELS[lv]
        zs = z0 - H.SLAB          # underside of the slab
        zb = z0 - 0.62            # bottom of the edge band (bilde 07, 10: a deep band of dark wood under the rail)

        def bx(n0, n1, t0, t1, za, zb_, m):
            """Box between offsets n0..n1 from the edge line (towards the hole = +) and along t0..t1."""
            lo, hi = c + s * n0, c + s * n1
            lo, hi = min(lo, hi), max(lo, hi)
            if o == "V":
                box_mm(K, lo, t0, za, hi, t1, zb_, m)
            else:
                box_mm(K, t0, lo, za, t1, hi, zb_, m)
        bx(0.0, 0.012, a, b, zb, z0 - 0.02, back)                      # dark backing
        for q in range(6):                                              # horizontal lamellas with shadow gaps
            zq = zb + 0.02 + q * 0.095
            bx(0.012, 0.055, a, b, zq, zq + 0.065, wood)
        bx(-0.35, 0.0, a, b, zb, zs, white)                             # white soffit downstand behind the band
        if lv <= H.P4:
            n = max(1, int((b - a) // 2.4))
            for q in range(n):
                t = a + (b - a) * (q + 0.5) / n
                x, y = (c - s * 0.9, t) if o == "V" else (t, c - s * 0.9)
                if floor_at(lv, x, y):
                    cylinder(K, (x, y, zs - 0.012), 0.11, 0.012, glow, verts=16)


# ---------------------------------------------------------------- columns: two black rings (bilde 22)
@op(PYNT)
def column_rings(K):
    dark = K.mat("MetalDark_In", *COLORS["MetalDark_In"])
    for b in G["boxes"]:
        if is_column(b):
            (cx, cy, cz), (_, _, sz) = b["c"], b["s"]
            z0 = cz - sz / 2
            for dz in (2.45, 2.62):
                ring(K, (cx, cy), 0.245, 0.262, z0 + dz, z0 + dz + 0.06, dark, segs=24)


# ---------------------------------------------------------------- stairs: undersides (bunnplater) and handrails
def flight_groups(s):
    steps, rise, tread = H.stair_steps(s)
    groups, cur = [], []
    for st in steps:
        if st[5] == "landing":
            if cur:
                groups.append(cur)
            cur = []
        else:
            cur.append(st)
    if cur:
        groups.append(cur)
    return groups, rise


@op(PYNT)
def stair_details(K):
    yellow = K.mat("HelgYellow_In", *COLORS["HelgYellow_In"])
    dark = K.mat("MetalDark_In", *COLORS["MetalDark_In"])
    for s in PLAN["stairs"]:
        if not s.get("atrium") or s.get("solid"):
            continue
        groups, rise = flight_groups(s)
        thick = rise + 0.22
        for gp in groups:
            first, last = gp[0], gp[-1]
            # along the run: start of the first tread to the end of the last, under the step bottoms
            if s["axis"] == "y":
                a0 = first[1] if s["dir"] > 0 else first[3]
                a1 = last[3] if s["dir"] > 0 else last[1]
                pa = ((s["c0"] + s["c1"]) / 2, a0, first[4] - thick - 0.02)
                pb = ((s["c0"] + s["c1"]) / 2, a1, last[4] - thick - 0.02)
                width = s["c1"] - s["c0"]
            else:
                a0 = first[0] if s["dir"] > 0 else first[2]
                a1 = last[2] if s["dir"] > 0 else last[0]
                pa = (a0, (s["c0"] + s["c1"]) / 2, first[4] - thick - 0.02)
                pb = (a1, (s["c0"] + s["c1"]) / 2, last[4] - thick - 0.02)
                width = s["c1"] - s["c0"]
            if pa[2] > s["z0"] + 0.3:     # only where the flight hangs free (not over the fill near the floor)
                K.ramp(pa, pb, width, 0.04, yellow)
    for b in G["boxes"]:
        if is_stair_guard(b):
            X, Y, Z = rot_axes(b["yaw"], b["pitch"], b["roll"])
            c = Vector(b["c"]) + Z * (b["s"][2] / 2 + 0.035)
            tube(K, tuple(c - X * (b["s"][0] / 2)), tuple(c + X * (b["s"][0] / 2)), 0.025, dark, segs=8)


# ---------------------------------------------------------------- the roof truss (Warren, white tubes; bilde 03, 10, 26)
TRUSS_X = (-13.54, -6.77, 0.0, 6.77, 13.54)
TRUSS_BOTTOM = H.ROOF_Z - 2.6


@op(TRUSS)
def roof_truss(K):
    white = K.mat("PaintWhite_In", *COLORS["PaintWhite_In"])
    ar = H.ROOF_AREA
    teeth = H.roof_teeth()
    zv, zr = H.ROOF_Z - 0.2, H.RIDGE_Z - 0.15
    valleys = [t[0] for t in teeth] + [teeth[-1][2]]
    for y in valleys:
        tube(K, (ar[0], y, zv), (ar[2], y, zv), 0.12, white)
    nx = 18
    xs = [ar[0] + (ar[2] - ar[0]) * i / nx for i in range(nx + 1)]
    for ya, yr, yb in teeth:
        tube(K, (ar[0], yr, zr), (ar[2], yr, zr), 0.08, white)
        for x in xs[1:-1]:
            tube(K, (x, ya, zv + 0.08), (x, yr, zr), 0.045, white, segs=8)       # rafters under the sheet
            tube(K, (x, yr, zr), (x, yb, zv + 0.08), 0.03, white, segs=6)        # mullions of the glass face
    # main Warren trusses along Y on the axes: top chord under the valleys, bottom chord 2.4 m lower, zigzag diagonals
    pitch = (ar[3] - ar[1]) / H.ROOF_TEETH
    zt, zb = H.ROOF_Z - 0.34, TRUSS_BOTTOM
    for x in TRUSS_X:
        tube(K, (x, ar[1], zt), (x, ar[3], zt), 0.14, white, segs=12)
        yb0, yb1 = ar[1] + pitch / 2, ar[3] - pitch / 2
        tube(K, (x, yb0, zb), (x, yb1, zb), 0.14, white, segs=12)
        n = 2 * H.ROOF_TEETH
        for i in range(n):
            ytop = ar[1] + pitch * (i // 2 + (1 if i % 2 else 0))
            ybot = ar[1] + pitch * (i // 2 + 0.5)
            tube(K, (x, ytop, zt), (x, ybot, zb), 0.1, white)
        for i in range(H.ROOF_TEETH):
            tube(K, (x, ar[1] + pitch * (i + 0.5), zb), (x, ar[1] + pitch * (i + 0.5), zt), 0.05, white, segs=8)
    # bracing between the trusses at the bottom chord, and the big edge beams on the ring
    for i in range(H.ROOF_TEETH):
        y = ar[1] + pitch * (i + 0.5)
        tube(K, (TRUSS_X[0], y, zb), (TRUSS_X[-1], y, zb), 0.06, white, segs=8)
    for y in (ar[1], ar[3]):
        box_mm(K, ar[0], y - 0.25, H.ROOF_Z - 0.75, ar[2], y + 0.25, H.ROOF_Z - 0.05, white)
    for x in (ar[0], ar[2]):
        box_mm(K, x - 0.25, ar[1], H.ROOF_Z - 0.75, x + 0.25, ar[3], H.ROOF_Z - 0.05, white)


# ---------------------------------------------------------------- over the atrium: hangers and the flights Plan 5 -> 6
@op(OVER)
def hangers(K):
    white = K.mat("PaintWhite_In", *COLORS["PaintWhite_In"])
    top = H.ROOF_Z - 0.4
    for r in PLAN["rooms"]:
        if r.kind != "boks" or r.lv != H.P4:
            continue
        z = H.LEVELS[H.P4] + H.BOX_H + H.BOX_ROOF
        for x, y in ((r.x0 + 0.3, r.y0 + 0.3), (r.x1 - 0.3, r.y0 + 0.3), (r.x0 + 0.3, r.y1 - 0.3), (r.x1 - 0.3, r.y1 - 0.3)):
            if abs(x) < 13.0 and abs(y) < 17.5:
                tube(K, (x, y, z), (x, y, top), 0.035, white, segs=8)


# The yellow flights that cross the atrium high up (bilde 01, 06, 26; K7): Plan 5 west edge -> bridge -> Plan 6 east edge.
# Scenery: AboveAtrium collision (nobody stands on them; the grapple hook passes), stairs to Plan 5 are locked.
XB_Y0, XB_Y1 = -10.4, -8.0
XB_Z = H.LEVELS[H.P5] + 2.0


def crossing_flight(K, x_lo, x_hi, z_lo, z_hi, up_east, yellow, dark):
    n = 12
    rise, run = (z_hi - z_lo) / n, (x_hi - x_lo) / n
    for i in range(n):
        xa = x_lo + i * run if up_east else x_hi - (i + 1) * run
        zt = z_lo + (i + 1) * rise
        box_mm(K, xa, XB_Y0, zt - rise - 0.22, xa + run, XB_Y1, zt, yellow)
    zc = (z_lo + z_hi) / 2
    L = math.hypot(x_hi - x_lo, z_hi - z_lo)
    ang = math.degrees(math.atan2(z_hi - z_lo, x_hi - x_lo))
    for y in (XB_Y0 - 0.03, XB_Y1 + 0.03):
        box_ue(K, ((x_lo + x_hi) / 2, y, zc + 0.35), (L, 0.05, 1.5), yellow, yaw=0.0 if up_east else 180.0, pitch=ang)
        X, _, Z = rot_axes(0.0 if up_east else 180.0, ang, 0.0)
        c = Vector(((x_lo + x_hi) / 2, y, zc + 0.35)) + Z * 0.78
        tube(K, tuple(c - X * L / 2), tuple(c + X * L / 2), 0.025, dark, segs=8)


@op(OVER)
def crossing(K):
    yellow = K.mat("HelgYellow_In", *COLORS["HelgYellow_In"])
    dark = K.mat("MetalDark_In", *COLORS["MetalDark_In"])
    white = K.mat("PaintWhite_In", *COLORS["PaintWhite_In"])
    wood = K.mat("Wood_In", *COLORS["Wood_In"])
    xa, xb = -H.RING_IN[0], H.RING_IN[0]
    crossing_flight(K, xa, xa + 3.6, H.LEVELS[H.P5], XB_Z, True, yellow, dark)
    crossing_flight(K, xb - 3.6, xb, XB_Z, H.LEVELS[H.P6], True, yellow, dark)
    # the bridge: yellow deck with a white underside, wooden top rails on the glass
    box_mm(K, xa + 3.6, XB_Y0, XB_Z - 0.3, xb - 3.6, XB_Y1, XB_Z, yellow)
    box_mm(K, xa + 3.6, XB_Y0 + 0.1, XB_Z - 0.34, xb - 3.6, XB_Y1 - 0.1, XB_Z - 0.3, white)
    for y in (XB_Y0 + 0.03, XB_Y1 - 0.03):
        box_mm(K, xa + 3.6, y - 0.04, XB_Z + 1.1, xb - 3.6, y + 0.04, XB_Z + 1.16, wood)
    x = xa + 4.0
    while x < xb - 3.6:
        for y in (XB_Y0 - 0.06, XB_Y1 + 0.06):
            tube(K, (x, y, XB_Z - 0.3), (x, y, TRUSS_BOTTOM), 0.03, white, segs=8)
        x += 4.8


@op(OVER_GLASS)
def crossing_glass(K):
    g = K.mat("Glass", *COLORS["Glass"])
    xa, xb = -H.RING_IN[0] + 3.6, H.RING_IN[0] - 3.6
    for y in (XB_Y0 + 0.03, XB_Y1 - 0.03):
        box_mm(K, xa, y - 0.03, XB_Z, xb, y + 0.03, XB_Z + 1.1, g)


# ---------------------------------------------------------------- the ring lamp (Ø 2.4 m; bilde 02, 07, 17)
@op("SM_Prop_Helg_Hengelampe")
def ring_lamp(K):
    white = K.mat("PaintWhite_In", *COLORS["PaintWhite_In"])
    glow = K.mat("LightPanel_In", *COLORS["LightPanel_In"])
    dark = K.mat("MetalDark_In", *COLORS["MetalDark_In"])
    ring(K, (0.0, 0.0), 1.10, 1.30, 0.012, 0.09, white, segs=64)
    ring(K, (0.0, 0.0), 1.12, 1.28, 0.0, 0.012, glow, segs=64)
    for k in range(3):
        a = 2 * math.pi * k / 3 + 0.5
        cylinder(K, (math.cos(a) * 1.2, math.sin(a) * 1.2, 0.09), 0.02, 0.06, dark, verts=8)


# ---------------------------------------------------------------- build, hash, export
def mesh_hash(K):
    h = hashlib.sha1()
    for v in K.bm.verts:
        h.update(("%.4f,%.4f,%.4f;" % tuple(v.co)).encode())
    for f in K.bm.faces:
        h.update(("%d:%d:%s;" % (f.material_index, int(f.smooth), ",".join(str(v.index) for v in f.verts))).encode())
    h.update("|".join(m.name for m in K.mats).encode())
    return h.hexdigest()


def build(name, force):
    K = MeshKit(name)
    for fn in OPS[name]:
        fn(K)
    if len(K.bm.faces) == 0:
        K.bm.free()
        return "tom"
    K.bm.verts.index_update()
    digest = mesh_hash(K)
    stamp = os.path.join(OUT_DIR, name + ".hash")
    fbx = os.path.join(OUT_DIR, name + ".fbx")
    old = open(stamp).read().strip() if os.path.exists(stamp) else ""
    if not force and old == digest and os.path.exists(fbx) and os.path.exists(os.path.join(OUT_DIR, name + ".blend")):
        K.bm.free()
        return "uendret"
    nf = len(K.bm.faces)
    K.finish(also_blend=True, also_glb=False)
    with open(stamp, "w") as fh:
        fh.write(digest)
    return "eksportert (%d flater)" % nf


def main():
    force = "--force" in ARGS
    names = [n for n, _ in PARTS] + ["SM_Prop_Helg_Hengelampe"]
    only = [a for a in ARGS if a.startswith("SM_")]
    for name in names:
        if only and name not in only:
            continue
        print("%-42s %s" % (name, build(name, force)))
    if "--render" in ARGS:
        import render_helgasetr
        render_helgasetr.render_all(names)


if __name__ == "__main__":
    main()
