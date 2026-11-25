"""Hovedbygningen (NTNU main building) with Lysgaarden (the big hall) and the tower, as one walkable mesh.

Coordinates in meters, Unreal space: X east, Y south, Z up, origin at the middle of the main block
(the same as the "Hovedbygningen" entry in GSCampus.h). Run:
    python Tools/Blender/building_hovedbygningen.py
Exports SourceArt/Models/SM_Hovedbygningen.(fbx|glb|blend). Unreal uses complex-as-simple collision for it.

Layout:
  * Main block 110 x 28 m, 4 floors (5.5 m), walls with windows, doors on all sides.
  * Front bay (north) with the arched main door, a gable and two octagonal turrets with spires.
  * Lysgaarden: atrium through all floors with a glass roof, arcades, galleries, grand stair, stage.
  * Ramps between all floors at both ends of the building.
  * Tower behind the building (south) with a spiral ramp to a belfry at 33 m and a spire to 50 m.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from meshkit import MeshKit  # noqa: E402

K = MeshKit("SM_Hovedbygningen")
STONE = K.mat("Stone", (0.22, 0.23, 0.24), rough=0.85)
STONE_D = K.mat("StoneDark", (0.15, 0.155, 0.16), rough=0.9)
TRIM = K.mat("Trim", (0.46, 0.45, 0.42), rough=0.8)
SLATE = K.mat("Slate", (0.11, 0.12, 0.14), rough=0.6)
PLASTER = K.mat("Plaster", (0.86, 0.83, 0.76), rough=0.9)
FLOOR = K.mat("Floor", (0.52, 0.47, 0.41), rough=0.6)
MARBLE = K.mat("Marble", (0.78, 0.76, 0.72), rough=0.35)
WOOD = K.mat("Wood", (0.33, 0.19, 0.09), rough=0.6)
METAL = K.mat("Metal", (0.25, 0.26, 0.28), metallic=0.8, rough=0.4)
GLASS = K.mat("Glass", (0.55, 0.7, 0.78), metallic=0.3, rough=0.1)
COPPER = K.mat("Copper", (0.25, 0.5, 0.42), metallic=0.4, rough=0.5)

W = 0.7            # outer wall thickness
FH = 5.5           # floor height
FLOORS = 4
H = FH * FLOORS    # 22 m
SLAB = 0.3
X0, X1, Y0, Y1 = -55.0, 55.0, -14.0, 14.0
BAY = (-9.0, 9.0, -18.0)  # front bay x range and its front face
ATRIUM = (-14.0, 14.0, -9.0, 9.0)
RAMP_X = (40.0, 52.5)
RAMP_Y = (10.5, 13.2)
TOWER = (-6.0, 6.0, 14.0, 26.0)
TOWER_H = 38.0


# ---------------------------------------------------------------- walls with openings
def wall(axis, at, a0, a1, z0, z1, t, holes, mi):
    """Wall along `axis` ("X": runs along X at y=at; "Y": runs along Y at x=at), from a0..a1 and z0..z1.
    holes = [(a_lo, a_hi, z_lo, z_hi)]. Solid cells are merged vertically to keep the box count low."""
    holes = [h for h in holes if h[1] > a0 and h[0] < a1]
    acuts = sorted({a0, a1, *[min(max(h[0], a0), a1) for h in holes], *[min(max(h[1], a0), a1) for h in holes]})
    zcuts = sorted({z0, z1, *[min(max(h[2], z0), z1) for h in holes], *[min(max(h[3], z0), z1) for h in holes]})
    for i in range(len(acuts) - 1):
        al, ah = acuts[i], acuts[i + 1]
        if ah - al < 1e-4:
            continue
        am = (al + ah) / 2
        run = None
        for j in range(len(zcuts) - 1):
            zl, zh = zcuts[j], zcuts[j + 1]
            zm = (zl + zh) / 2
            solid = not any(h[0] <= am <= h[1] and h[2] <= zm <= h[3] for h in holes)
            if solid:
                run = (run[0], zh) if run else (zl, zh)
            if (not solid or j == len(zcuts) - 2) and run:
                _wall_box(axis, at, al, ah, run[0], run[1], t, mi)
                run = None


def _wall_box(axis, at, al, ah, zl, zh, t, mi):
    if axis == "X":
        K.box_minmax((al, at - t / 2, zl), (ah, at + t / 2, zh), mi)
    else:
        K.box_minmax((at - t / 2, al, zl), (at + t / 2, ah, zh), mi)


def window_row(a0, a1, spacing, width, skip=()):
    """Centers of evenly spaced windows between a0 and a1, skipping any within 2.5 m of a skip position."""
    n = int((a1 - a0) // spacing)
    start = (a0 + a1) / 2 - (n - 1) * spacing / 2
    out = []
    for k in range(n):
        c = start + k * spacing
        if c - width / 2 < a0 + 0.8 or c + width / 2 > a1 - 0.8:
            continue
        if any(abs(c - s[0]) < s[1] for s in skip):
            continue
        out.append(c)
    return out


def facade(axis, at, a0, a1, outward, doors=(), skip=(), height=H, floors=FLOORS, spacing=3.6, win_w=1.6, mi=STONE):
    """Outer wall with windows on every floor, doors (center, width, height), sills, lintels and floor bands."""
    holes, deco = [], []
    skips = list(skip) + [(d[0], d[1] / 2 + 1.6) for d in doors]
    for f in range(floors):
        zb = f * FH
        top = zb + (4.2 if f == 0 else 4.0)
        bot = zb + (1.2 if f == 0 else 1.0)
        if f == floors - 1:
            top = zb + 3.8
        for c in window_row(a0, a1, spacing, win_w, skips):
            holes.append((c - win_w / 2, c + win_w / 2, bot, top))
            deco.append((c, win_w, bot, top))
    for (c, w, h) in doors:
        holes.append((c - w / 2, c + w / 2, 0.0, h))
    wall(axis, at, a0, a1, 0.0, height, W, holes, mi)

    o = outward  # +1 or -1: which side is outside
    def ob(al, ah, zl, zh, depth, m):
        if axis == "X":
            y = at + o * (W / 2 + depth / 2)
            K.box_minmax((al, y - depth / 2, zl), (ah, y + depth / 2, zh), m)
        else:
            x = at + o * (W / 2 + depth / 2)
            K.box_minmax((x - depth / 2, al, zl), (x + depth / 2, ah, zh), m)
    # sills, lintels (keystone look) and frames
    for (c, w, b, t) in deco:
        ob(c - w / 2 - 0.15, c + w / 2 + 0.15, b - 0.15, b, 0.18, TRIM)
        ob(c - w / 2 - 0.2, c + w / 2 + 0.2, t, t + 0.35, 0.14, TRIM)
        ob(c - 0.18, c + 0.18, t, t + 0.5, 0.2, TRIM)
    # plinth, floor bands and cornice
    ob(a0, a1, 0.0, 1.0, 0.25, STONE_D)
    for f in range(1, floors):
        ob(a0, a1, f * FH - 0.2, f * FH + 0.1, 0.12, TRIM)
    ob(a0 - 0.3, a1 + 0.3, height - 0.5, height, 0.45, TRIM)
    # pilasters every 4th window gap
    for k, (c, w, b, t) in enumerate(deco[: len(deco) // floors]):
        if k % 4 == 0:
            p = c - spacing / 2
            if a0 + 1 < p < a1 - 1:
                ob(p - 0.35, p + 0.35, 1.0, height - 0.5, 0.12, STONE_D)
    # door frames, arches and steps
    for (c, w, h) in doors:
        ob(c - w / 2 - 0.4, c - w / 2, 0, h, 0.3, TRIM)
        ob(c + w / 2, c + w / 2 + 0.4, 0, h, 0.3, TRIM)
        ob(c - w / 2 - 0.4, c + w / 2 + 0.4, h, h + 0.6, 0.3, TRIM)


# ---------------------------------------------------------------- main block walls
doors_n = []
doors_s = [(0.0, 2.4, 3.2), (-48.0, 2.4, 3.2), (48.0, 2.4, 3.2)]
# north wall either side of the bay (the bay has its own front)
facade("X", Y0 + W / 2, X0, BAY[0], -1)
facade("X", Y0 + W / 2, BAY[1], X1, -1)
# south wall (tower at x 0, wings at +-48); no windows where the tower touches
facade("X", Y1 - W / 2, X0, X1, +1, doors=doors_s, skip=[(0.0, 7.0)])
# end walls with a door each
facade("Y", X0 + W / 2, Y0, Y1, -1, doors=[(0.0, 2.4, 3.2)], spacing=3.4)
facade("Y", X1 - W / 2, Y0, Y1, +1, doors=[(0.0, 2.4, 3.2)], spacing=3.4)

# ---------------------------------------------------------------- front bay with main door, gable and turrets
bx0, bx1, by = BAY
facade("X", by + W / 2, bx0, bx1, -1, doors=[(0.0, 3.6, 4.2)], spacing=3.0, mi=STONE)
wall("Y", bx0 + W / 2, by, Y0 + W, 0, H, W, [], STONE)
wall("Y", bx1 - W / 2, by, Y0 + W, 0, H, W, [], STONE)
# arch over the main door + big stair
K.arch(0.0, by - 0.25, 4.2, 3.6, 0.5, 0.6, TRIM, along="X", segs=11)
# simple flat landing in front of the door (players walk straight in; ground is z=0)
K.box_minmax((-5.0, by - 2.5, -0.2), (5.0, by, 0.02), STONE_D)
# gable above the bay
K.prism_y(by - 0.2, Y0 + W, [(bx0 - 0.3, H), (bx1 + 0.3, H), (0.0, H + 8.5)], STONE)
K.prism_y(by - 0.45, by - 0.2, [(bx0 - 0.6, H - 0.2), (bx1 + 0.6, H - 0.2), (0.0, H + 9.0)], TRIM)
# rose window (decor ring) in the gable
for i in range(16):
    a = 2 * math.pi * i / 16
    K.box((math.cos(a) * 1.8, by - 0.55, H + 3.2 + math.sin(a) * 1.8), (0.7, 0.15, 0.35), TRIM, rot=(0, -math.degrees(a), 0))
K.box((0, by - 0.5, H + 3.2), (2.4, 0.1, 2.4), GLASS, rot=(0, 45, 0))
# twin octagonal turrets with spires
for tx in (bx0, bx1):
    K.cyl((tx, by, 0), 2.4, 1.2, STONE_D, verts=8, rot_z=22.5)
    K.cyl((tx, by, 0), 2.2, H + 6.0, STONE, verts=8, rot_z=22.5)
    for z in (FH, 2 * FH, 3 * FH, H):
        K.cyl((tx, by, z - 0.2), 2.4, 0.35, TRIM, verts=8, rot_z=22.5)
    # little arched openings at the top of the turret
    K.cyl((tx, by, H + 6.0), 2.55, 0.5, TRIM, verts=8, rot_z=22.5)
    K.cyl((tx, by, H + 6.5), 2.3, 11.5, SLATE, verts=8, r_top=0.0, rot_z=22.5)
    K.cyl((tx, by, H + 17.8), 0.08, 2.0, METAL, verts=6)
    K.cyl((tx, by, H + 18.6), 0.25, 0.25, COPPER, verts=8, r_top=0.0)

# ---------------------------------------------------------------- floors (slabs) with atrium and ramp holes
ramp_holes = [(RAMP_X[0], RAMP_X[1], RAMP_Y[0], RAMP_Y[1]), (-RAMP_X[1], -RAMP_X[0], RAMP_Y[0], RAMP_Y[1])]
atrium_hole = ATRIUM
for f in range(FLOORS + 1):
    z = f * FH
    holes = []
    if 0 < f < FLOORS:
        holes = ramp_holes + [atrium_hole]
    elif f == FLOORS:
        holes = [atrium_hole]
    mi = MARBLE if f == 0 else FLOOR
    if f == FLOORS:
        mi = STONE_D
    K.slab_with_holes(X0 + W, X1 - W, Y0 + W, Y1 - W, z, SLAB, holes, mi)
    K.slab_with_holes(bx0 + W, bx1 - W, by + W, Y0 + W, z, SLAB, [], mi)

# ---------------------------------------------------------------- interior: corridor walls with doors (outside the atrium)
for f in range(FLOORS):
    zb, zt = f * FH, f * FH + FH - SLAB
    for y in (-5.0, 5.0):
        for (a0, a1) in ((X0 + W, ATRIUM[0] - 0.5), (ATRIUM[1] + 0.5, X1 - W)):
            doors = [(c - 0.6, c + 0.6, zb, zb + 2.6) for c in [a0 + 3.5 + 8.0 * k for k in range(7)] if c < a1 - 2.0]
            wall("X", y, a0, a1, zb, zt, 0.2, doors, PLASTER)
    # cross walls splitting rooms
    for x in (-46.0, -36.0, -26.0, 26.0, 36.0, 46.0):
        for (ya, yb) in ((Y0 + W, -5.1), (5.1, Y1 - W)):
            if yb > 0 and abs(x) > 39:  # keep the ramp rooms open
                continue
            wall("Y", x, ya, yb, zb, zt, 0.2, [], PLASTER)
    # walls closing the atrium ends (with big arches/openings)
    for x in (ATRIUM[0] - 0.5, ATRIUM[1] + 0.5):
        wall("Y", x, -5.0, 5.0, zb, zt, 0.3, [(-2.0, 2.0, zb, zb + 3.6)], PLASTER)

# ---------------------------------------------------------------- ramps between floors (both ends)
RW = RAMP_Y[1] - RAMP_Y[0] - 0.05
ry = (RAMP_Y[0] + RAMP_Y[1]) / 2
for sgn in (1, -1):
    for f in range(FLOORS - 1):
        z0, z1 = f * FH, (f + 1) * FH
        xa = sgn * (RAMP_X[0] - 0.2)
        xb = sgn * (RAMP_X[1] - 0.15)
        K.ramp((xa, ry, z0 + 0.02), (xb, ry, z1), RW, 0.3, FLOOR)
        # rail along the open side of the ramp hole on the floor above
        zt = z1
        K.box_minmax((min(sgn * RAMP_X[0], sgn * RAMP_X[1]), RAMP_Y[0] - 0.12, zt), (max(sgn * RAMP_X[0], sgn * RAMP_X[1]), RAMP_Y[0], zt + 1.0), METAL)
    # railing around the top-floor ramp hole
    zt = (FLOORS - 1) * FH
    K.box_minmax((min(sgn * RAMP_X[0], sgn * (RAMP_X[0] + 0.12)), RAMP_Y[0], zt), (max(sgn * RAMP_X[0], sgn * (RAMP_X[0] + 0.12)), RAMP_Y[1], zt + 1.0), METAL)

# ---------------------------------------------------------------- Lysgaarden (atrium)
ax0, ax1, ay0, ay1 = ATRIUM
# grand stair from the hall floor up to the first gallery (south side)
STEPS = 30
run = (ay1 - 0.2) / STEPS
for s in range(STEPS):
    y_a = 0.2 + s * run
    K.box_minmax((-3.8, y_a, 0.0), (3.8, ay1 + 0.01, (s + 1) * FH / STEPS), MARBLE if s % 2 == 0 else TRIM)
# stair balustrades
for x in (-4.0, 4.0):
    K.ramp((x, 0.2, 1.0), (x, ay1, FH + 1.0), 0.25, 0.25, WOOD)
    K.ramp((x, 0.2, 0.15), (x, ay1, FH + 0.15), 0.3, 0.15, TRIM)
    K.box((x, 0.35, 0.6), (0.45, 0.45, 1.2), TRIM)
    K.cyl((x, 0.35, 1.2), 0.18, 0.5, METAL, verts=8, r_top=0.08)
# stage on the north side of the hall
K.box_minmax((-6.0, ay0, 0.0), (6.0, ay0 + 3.0, 0.5), WOOD)
K.box_minmax((-2.0, ay0 + 3.0, 0.0), (2.0, ay0 + 3.5, 0.25), WOOD)
K.box_minmax((-6.0, ay0 + 0.05, 0.5), (6.0, ay0 + 0.25, 5.0), K.mat("Curtain", (0.45, 0.06, 0.07), rough=0.95))
# arcades: columns + arches around the hall on the ground floor and every gallery
for f in range(FLOORS):
    zb = f * FH + (0.0 if f == 0 else 0.0)
    ch = FH - SLAB - (1.6 if f == 0 else 1.4)  # column height to the arch spring
    r_col = 0.32 if f == 0 else 0.24
    xs = [ax0 + 2.0 + 4.0 * k for k in range(7)]  # -12 .. 12
    ys = [ay0 + 1.5 + 3.0 * k for k in range(6)]  # -7.5 .. 7.5
    lines = []
    for yline, ypos in ((ay0, ay0 - 0.45), (ay1, ay1 + 0.45)):
        for x in xs:
            if f > 0 and yline == ay1 and abs(x) < 4.5 and f == 1:
                continue
            lines.append((x, ypos))
    for xline, xpos in ((ax0, ax0 - 0.45), (ax1, ax1 + 0.45)):
        for y in ys:
            lines.append((xpos, y))
    for (x, y) in lines:
        K.box((x, y, zb + 0.15), (r_col * 2.6, r_col * 2.6, 0.3), TRIM)
        K.cyl((x, y, zb + 0.3), r_col, ch - 0.3, MARBLE, verts=12)
        K.box((x, y, zb + ch + 0.12), (r_col * 2.8, r_col * 2.8, 0.25), TRIM)
    # arches between neighbours
    for yline, ypos in ((ay0, ay0 - 0.45), (ay1, ay1 + 0.45)):
        for k in range(len(xs) - 1):
            span = xs[k + 1] - xs[k] - 0.6
            K.arch((xs[k] + xs[k + 1]) / 2, ypos, zb + ch + 0.25, span, 0.5, 0.35, PLASTER, along="X", segs=9)
    for xline, xpos in ((ax0, ax0 - 0.45), (ax1, ax1 + 0.45)):
        for k in range(len(ys) - 1):
            span = ys[k + 1] - ys[k] - 0.6
            K.arch(xpos, (ys[k] + ys[k + 1]) / 2, zb + ch + 0.25, span, 0.5, 0.35, PLASTER, along="Y", segs=9)
    # balustrade on the gallery edge (floor above)
    zg = (f + 1) * FH
    if f + 1 < FLOORS:
        for (a, b) in ((ax0, -4.2), (4.2, ax1)) if f == 0 else ((ax0, ax1),):
            K.box_minmax((a, ay1 + 0.0, zg), (b, ay1 + 0.2, zg + 0.15), TRIM)
            K.box_minmax((a, ay1 + 0.0, zg + 0.95), (b, ay1 + 0.25, zg + 1.1), WOOD)
            for x in [a + 0.3 + 0.35 * i for i in range(int((b - a) / 0.35))]:
                K.cyl((x, ay1 + 0.1, zg + 0.15), 0.06, 0.8, PLASTER, verts=6)
        K.box_minmax((ax0, ay0 - 0.2, zg), (ax1, ay0, zg + 0.15), TRIM)
        K.box_minmax((ax0, ay0 - 0.25, zg + 0.95), (ax1, ay0, zg + 1.1), WOOD)
        for x in [ax0 + 0.3 + 0.35 * i for i in range(int((ax1 - ax0) / 0.35))]:
            K.cyl((x, ay0 - 0.1, zg + 0.15), 0.06, 0.8, PLASTER, verts=6)
        for xe in (ax0 - 0.2, ax1):
            K.box_minmax((xe, ay0, zg), (xe + 0.2, ay1, zg + 0.15), TRIM)
            K.box_minmax((xe - 0.05, ay0, zg + 0.95), (xe + 0.25, ay1, zg + 1.1), WOOD)
            for y in [ay0 + 0.3 + 0.35 * i for i in range(int((ay1 - ay0) / 0.35))]:
                K.cyl((xe + 0.1, y, zg + 0.15), 0.06, 0.8, PLASTER, verts=6)
# chandeliers hanging in the hall
for cx in (-8.0, 0.0, 8.0):
    K.cyl((cx, 0.0, 11.0), 0.03, H - 11.0, METAL, verts=4)
    K.cyl((cx, 0.0, 10.4), 1.2, 0.15, METAL, verts=12)
    K.cyl((cx, 0.0, 10.55), 0.9, 0.5, COPPER, verts=12, r_top=0.2)
    for i in range(8):
        a = 2 * math.pi * i / 8
        K.cyl((cx + math.cos(a) * 1.1, math.sin(a) * 1.1, 10.55), 0.06, 0.25, K.mat("Bulb", (1.0, 0.9, 0.6), rough=0.2), verts=6)
# glass roof over Lysgaarden: steel frame + glass panes
gz = H
for x in [ax0 - 1.0 + 2.0 * i for i in range(16)]:
    K.ramp((x, Y0 + W, gz + 0.15), (x, 0.0, gz + 5.0 + 0.15), 0.12, 0.25, METAL)
    K.ramp((x, 0.0, gz + 5.0 + 0.15), (x, Y1 - W, gz + 0.15), 0.12, 0.25, METAL)
# purlins (the glass itself is left out so daylight reaches the hall; Unreal adds a glass material later)
for k in range(1, 5):
    t = k / 5
    for yy in (Y0 + W + (0.0 - Y0 - W) * t, (Y1 - W) * (1 - t)):
        K.box_minmax((ax0 - 1.0, yy - 0.06, gz + 5.0 * (t if yy < 0 else 1 - t) + 0.05), (ax1 + 1.0, yy + 0.06, gz + 5.0 * (t if yy < 0 else 1 - t) + 0.25), METAL)
K.box_minmax((ax0 - 1.0, -0.2, gz + 4.9), (ax1 + 1.0, 0.2, gz + 5.3), METAL)
# gable ends of the glass roof
for x in (ax0 - 1.0, ax1 + 1.0):
    K.prism_x(x - 0.3, x, [(Y0 + W, gz), (Y1 - W, gz), (0.0, gz + 5.0)], STONE)

# ---------------------------------------------------------------- slate roofs over the wings of the main block
RIDGE = H + 7.0
for (xa, xb) in ((X0 - 0.4, ax0 - 1.3), (ax1 + 1.3, X1 + 0.4)):
    K.prism_x(xa, xb, [(Y0 - 0.6, H - 0.1), (Y1 + 0.6, H - 0.1), (0.0, RIDGE)], SLATE)
    K.box_minmax((xa, -0.15, RIDGE - 0.1), (xb, 0.15, RIDGE + 0.2), METAL)
    # dormers
    for x in [xa + 4.0 + 7.2 * k for k in range(int((xb - xa - 4.0) / 7.2) + 1)]:
        if x > xb - 3.0:
            continue
        for sgn in (-1, 1):
            yb = sgn * 9.0
            K.box_minmax((x - 1.2, yb - 1.0, H), (x + 1.2, yb + 1.0, H + 3.0), STONE)
            K.prism_y(yb - 1.3, yb + 1.3, [(x - 1.5, H + 3.0), (x + 1.5, H + 3.0), (x, H + 4.3)], SLATE)
            K.box((x, yb - sgn * 1.0, H + 1.6), (1.2, 0.05, 1.6), GLASS)
# end gables (stepped) and chimneys
for xe, sgn in ((X0, 1), (X1, -1)):
    K.prism_x(xe if sgn > 0 else xe - 0.6, xe + 0.6 if sgn > 0 else xe, [(Y0, H), (Y1, H), (0.0, RIDGE + 0.6)], STONE)
    for cx in (xe + sgn * 6.0, xe + sgn * 20.0):
        K.box_minmax((cx - 0.8, 3.0, H + 3.0), (cx + 0.8, 4.6, RIDGE + 2.5), STONE_D)
        K.box_minmax((cx - 1.0, 2.8, RIDGE + 2.5), (cx + 1.0, 4.8, RIDGE + 2.8), TRIM)

# ---------------------------------------------------------------- the tower (behind the building) with a spiral ramp
tx0, tx1, ty0, ty1 = TOWER
tcx, tcy = (tx0 + tx1) / 2, (ty0 + ty1) / 2
TW = 0.7
belfry = [(a - 1.4, a + 1.4, 33.6, 37.2) for a in (-2.8, 2.8)]
slits = []
for z in (6.5, 12.0, 17.5, 23.0, 28.5):
    slits.append((-0.4, 0.4, z, z + 2.2))
# north wall touches the main block: door at ground level into the spiral
wall("X", ty0 + TW / 2, tx0, tx1, 0, TOWER_H, TW,
     [(tcx - 1.2, tcx + 1.2, 0, 3.0)] + [(tcx + h[0], tcx + h[1], h[2], h[3]) for h in belfry] +
     [(tcx - 0.4, tcx + 0.4, z, z + 2.2) for z in (23.0, 28.5)], STONE)
wall("X", ty1 - TW / 2, tx0, tx1, 0, TOWER_H, TW,
     [(tcx - 1.2, tcx + 1.2, 0, 3.0)] + [(tcx + h[0], tcx + h[1], h[2], h[3]) for h in belfry + slits], STONE)
wall("Y", tx0 + TW / 2, ty0 + TW, ty1 - TW, 0, TOWER_H, TW, [(tcy + h[0], tcy + h[1], h[2], h[3]) for h in belfry + slits], STONE)
wall("Y", tx1 - TW / 2, ty0 + TW, ty1 - TW, 0, TOWER_H, TW, [(tcy + h[0], tcy + h[1], h[2], h[3]) for h in belfry + slits], STONE)
# corner buttresses and bands
for (cx, cy) in ((tx0, ty0), (tx1, ty0), (tx0, ty1), (tx1, ty1)):
    K.box((cx, cy, 16.0), (1.6, 1.6, 32.0), STONE_D)
    K.box((cx, cy, 33.0), (1.4, 1.4, 2.0), TRIM)
    K.cyl((cx, cy, TOWER_H + 0.4), 0.6, 4.0, SLATE, verts=4, r_top=0.0, rot_z=45)
for z in (FH, 2 * FH, 3 * FH, H, 33.0, TOWER_H):
    K.box_minmax((tx0 - 0.3, ty0 - 0.3, z - 0.3), (tx1 + 0.3, ty1 + 0.3, z), TRIM)
# belfry arches
for (axis, at) in (("X", ty0 - 0.05), ("X", ty1 + 0.05)):
    for a in (-2.8, 2.8):
        K.arch(tcx + a, at, 37.2, 2.8, 0.85, 0.4, TRIM, along="X", segs=7)
for (axis, at) in (("Y", tx0 - 0.05), ("Y", tx1 + 0.05)):
    for a in (-2.8, 2.8):
        K.arch(at, tcy + a, 37.2, 2.8, 0.85, 0.4, TRIM, along="Y", segs=7)
# clock faces just under the belfry
CLOCK = K.mat("ClockFace", (0.9, 0.88, 0.8), rough=0.5)
for (x, y, ax, o) in ((tcx, ty0 - 0.4, "Y", -1), (tcx, ty1 + 0.4, "Y", 1), (tx0 - 0.4, tcy, "X", -1), (tx1 + 0.4, tcy, "X", 1)):
    K.cyl_axis((x, y, 30.0), 1.5, 0.2, TRIM, axis=ax, verts=24)
    K.cyl_axis((x + (0.1 * o if ax == "X" else 0), y + (0.1 * o if ax == "Y" else 0), 30.0), 1.3, 0.1, CLOCK, axis=ax, verts=24)
    d = (0.2 * o if ax == "X" else 0, 0.2 * o if ax == "Y" else 0)
    K.box((x + d[0], y + d[1], 30.45), (0.1, 0.1, 0.9) if ax == "Y" else (0.1, 0.1, 0.9), METAL)
    K.box((x + d[0] + (0.3 if ax == "Y" else 0), y + d[1] + (0.3 if ax == "X" else 0), 30.0),
          (0.65, 0.1, 0.1) if ax == "Y" else (0.1, 0.65, 0.1), METAL)
# spire
K.cyl((tcx, tcy, TOWER_H), 8.3, 0.6, TRIM, verts=4, rot_z=45)
K.cyl((tcx, tcy, TOWER_H + 0.6), 7.4, 12.0, SLATE, verts=4, r_top=0.0, rot_z=45)
K.cyl((tcx, tcy, TOWER_H + 12.4), 0.1, 2.2, METAL, verts=6)
K.cyl((tcx, tcy, TOWER_H + 13.6), 0.3, 0.3, COPPER, verts=8)

# spiral ramp: corner landings at offset D from the center, sides rise 2.75 m each.
D, RWT, RISE = 4.2, 2.2, 2.75
corners = {"A": (-D, -D), "B": (D, -D), "C": (D, D), "D": (-D, D)}  # A=NW, B=NE, C=SE, D=SW (y south)
order = ["B", "C", "D", "A"]
SIDES = 12
z = 0.0
K.box_minmax((tcx - 1.4, tcy - 1.4, 0), (tcx + 1.4, tcy + 1.4, 33.0), STONE_D)  # central core
for s in range(SIDES):
    a = corners[order[s % 4]]
    b = corners[order[(s + 1) % 4]]
    za, zb = z, z + RISE
    # landing at a (stands on its own height)
    if s > 0:
        K.box_minmax((tcx + a[0] - RWT / 2, tcy + a[1] - RWT / 2, za - 0.3), (tcx + a[0] + RWT / 2, tcy + a[1] + RWT / 2, za), FLOOR)
    # ramp from the edge of landing a to the edge of landing b
    dx, dy = (b[0] - a[0]) / (2 * D), (b[1] - a[1]) / (2 * D)
    p0 = (tcx + a[0] + dx * RWT / 2, tcy + a[1] + dy * RWT / 2, za)
    p1 = (tcx + b[0] - dx * RWT / 2, tcy + b[1] - dy * RWT / 2, zb)
    K.ramp(p0, p1, RWT, 0.3, FLOOR)
    # inner rail (towards the core void)
    nx, ny = -dy, dx  # perpendicular; point it towards the center
    cxm, cym = (a[0] + b[0]) / 2, (a[1] + b[1]) / 2
    if nx * -cxm + ny * -cym < 0:
        nx, ny = -nx, -ny
    off = RWT / 2 - 0.06
    K.ramp((p0[0] + nx * off, p0[1] + ny * off, za + 1.0), (p1[0] + nx * off, p1[1] + ny * off, zb + 1.0), 0.08, 0.08, METAL)
    z = zb
# top floor of the belfry with a hole over the last ramp
last_a = corners[order[(SIDES - 1) % 4]]
last_b = corners[order[SIDES % 4]]
hx0 = tcx + min(last_a[0], last_b[0]) - RWT / 2
hx1 = tcx + max(last_a[0], last_b[0]) - RWT / 2
hy0 = tcy + min(last_a[1], last_b[1]) - RWT / 2
hy1 = tcy + max(last_a[1], last_b[1]) + RWT / 2
K.slab_with_holes(tx0 + TW, tx1 - TW, ty0 + TW, ty1 - TW, z, 0.3, [(hx0, hx1, hy0, hy1)], FLOOR)
# a bell in the belfry
K.cyl((tcx, tcy + 2.0, z + 1.4), 1.0, 1.4, COPPER, verts=16, r_top=0.45)
K.box((tcx, tcy + 2.0, z + 3.0), (2.6, 0.25, 0.3), WOOD)
for sx in (-1.2, 1.2):
    K.box((tcx + sx, tcy + 2.0, z + 1.5), (0.25, 0.25, 3.0), WOOD)
# railing around the hole at the top
K.box_minmax((hx0, hy1, z), (hx1, hy1 + 0.1, z + 1.0), METAL)

# ---------------------------------------------------------------- outside: lamps by the main door, flag, sign
for sx in (-6.0, 6.0):
    K.cyl((sx, by - 2.2, 0), 0.15, 3.4, METAL, verts=8)
    K.box((sx, by - 2.2, 3.6), (0.45, 0.45, 0.6), K.mat("Lamp", (1.0, 0.85, 0.55), rough=0.3))
K.box((0.0, by - 0.5, 6.2), (6.0, 0.12, 0.9), K.mat("SignDark", (0.05, 0.07, 0.15), rough=0.4))
K.cyl((0.0, by - 1.0, H + 8.5), 0.07, 6.0, METAL, verts=6)

K.finish()
print("done SM_Hovedbygningen")
