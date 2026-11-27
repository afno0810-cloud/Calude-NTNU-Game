"""Walks the Helgasetr MODEL (round 4b) with rays, the way the game will (Blender, reads SourceArt/Models/*.blend):

    blender --background --python Tools/Blender/check_helgasetr_walk.py

check_helgasetr.py checks the data; this checks what building_helgasetr.py actually exported, together with what stays
a block in the game (BoxPartM 255: L-screens, blockers) and the interactive pieces the builder lays out from the same
lists (glass panels, breach walls, cover). Game meters (X east, Y south, Z up); the .blend files are mirrored in Y.
Every check has a counter-test that MUST fail:
  1) GameMode: per spawn point the object trace from Flag.Z + 2 m down to -3 m lands on the Plan 4 floor (+-5 cm), nothing
     blocks 2.2 m over it (9 rays in the capsule's footprint); per flag the same from +2 m      (counter: point in Trapp sør)
  2) doors (not the sealed slot): rays through the middle at 1.0 and 1.9 m pass   (counter: 0.7 m to the side hits the wall)
  3) stairs: every tread is where the data says (+-3 cm), the steps rise <= 0.20 m (counter: a tread lifted 0.25 m)
  4) rails: along every atrium edge on Plan 2-4, every 0.5 m, a ray outwards 1.0 m over the floor meets glass or
     structure within 0.3 m of the edge                                             (counter: one glass panel removed)
  5) nothing in a Full part over Plan 4 inside the atrium, higher than a rail, faces up with room for a body over it
     (nothing to stand on, K10; the box wall tops have the box roof right on them)            (counter: a test plate)
  6) body test on Plan 1-4 every 0.5 m: where there is floor, nothing hangs lower than 1.84 m over it (points inside
     walls and furniture are skipped: the first face over them looks up)                                     (counter: a beam 1.5 m over P4 west)
Prints one line per check and exits 1 when anything is wrong.
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402
from mathutils.bvhtree import BVHTree  # noqa: E402

import helgasetr_plan as H  # noqa: E402
from gs_common import OUT_DIR  # noqa: E402

PLAN = H.make_plan()
G = H.derive(PLAN)
PARTS, BOX_PART = H.model_parts(G)
Z4 = H.LEVELS[H.P4]
errors = 0


def report(ok, text):
    global errors
    print(("OK      " if ok else "FEIL    ") + text)
    if not ok:
        errors += 1


def counter(failed, text):
    report(failed, "motprove: %s blir avvist" % text)


# ---------------------------------------------------------------- geometry
def part_polys(name):
    path = os.path.join(OUT_DIR, name + ".blend")
    if not os.path.exists(path):
        return None
    with bpy.data.libraries.load(path, link=False) as (src, dst):
        dst.meshes = [n for n in src.meshes if n == name]
    me = dst.meshes[0] if dst.meshes else None
    if me is None:
        return None
    verts = [(v.co.x, -v.co.y, v.co.z) for v in me.vertices]   # back to game space
    polys = [tuple(reversed(p.vertices)) for p in me.polygons]   # un-mirroring Y flips the winding: normals out again
    return verts, polys


def box_polys(c, s, yaw=0.0, pitch=0.0, roll=0.0):
    from building_helgasetr import rot_axes, SIGNS, BOX_FACES
    X, Y, Z = rot_axes(yaw, pitch, roll)
    c = Vector(c)
    v = [tuple(c + X * (a * s[0] / 2) + Y * (b * s[1] / 2) + Z * (d * s[2] / 2)) for a, b, d in SIGNS]
    return v, BOX_FACES


class Geo:
    def __init__(self):
        self.verts, self.polys = [], []

    def add(self, vp):
        if vp is None:
            return
        v, p = vp
        n = len(self.verts)
        self.verts += list(v)
        self.polys += [tuple(i + n for i in f) for f in p]

    def tree(self):
        return BVHTree.FromPolygons(self.verts, self.polys, epsilon=0.0)


KIND = {n: k for n, k in PARTS}
MISSING = [n for n, k in PARTS if k != H.MC_NONE and not os.path.exists(os.path.join(OUT_DIR, n + ".blend"))]
FULL_POLYS = {}
for n, k in PARTS:
    if k in (H.MC_FULL, H.MC_GLASS_FULL, H.MC_ABOVE, H.MC_GLASS_ABOVE):
        FULL_POLYS[n] = part_polys(n)


def world(skip_panels=(), extra=(), with_above=True, with_pieces=True):
    """Everything that stops a body: Full and glass parts, (Above parts block Pawn too), blocks that stay blocks,
    the glass panels, breach walls and cover."""
    g = Geo()
    for n, vp in FULL_POLYS.items():
        if KIND[n] in (H.MC_ABOVE, H.MC_GLASS_ABOVE) and not with_above:
            continue
        g.add(vp)
    for k, b in enumerate(G["boxes"]):
        if BOX_PART[k] == 255 and b["coll"] in (H.C_SOLID, H.C_BLOCKER, H.C_HIGH):
            g.add(box_polys(b["c"], b["s"], b["yaw"], b["pitch"], b["roll"]))
    if with_pieces:
        for p in G["panels"]:
            if p["id"] in skip_panels:
                continue
            g.add(box_polys(p["c"], (p["len"], p["t"], p["h"]), p["yaw"]))
        for b in G["breach"] + G["cover"]:
            x0, y0, z0, x1, y1, z1 = b["box"]
            g.add(box_polys(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (x1 - x0, y1 - y0, z1 - z0)))
    for vp in extra:
        g.add(vp)
    return g.tree()


def ray(t, a, b, normal=False):
    a, b = Vector(a), Vector(b)
    d = b - a
    L = d.length
    loc, nrm, idx, dist = t.ray_cast(a, d.normalized(), L)
    if normal:
        return (loc, dist, nrm) if loc is not None else (None, None, None)
    return (loc, dist) if loc is not None else (None, None)


# ---------------------------------------------------------------- 1) GameMode traces
def check_spawns(T, extra_points=(), quiet=False):
    bad = []
    pts = []
    for b in PLAN["bases"]:
        fx, fy, fz = b["flag"]
        pts.append(("flagg " + b["name"], fx, fy, False))
        for (x, y) in b["spawns"]:
            pts.append(("spawn %s (%.1f, %.1f)" % (b["name"], x, y), x, y, True))
    pts += [("ekstra (%.1f, %.1f)" % p, p[0], p[1], True) for p in extra_points]
    zf = H.FLAG_Z_BRIDGE
    for name, x, y, body in pts:
        hit, _ = ray(T, (x, y, zf + 2.0), (x, y, zf - 3.0))
        if hit is None or abs(hit.z - Z4) > 0.05:
            bad.append("%s: treff %s" % (name, "%.2f" % hit.z if hit is not None else "ingen"))
            continue
        if body:
            for k in range(9):
                ox, oy = (0.0, 0.0) if k == 8 else (0.4 * math.cos(k * math.pi / 4), 0.4 * math.sin(k * math.pi / 4))
                h2, _ = ray(T, (x + ox, y + oy, hit.z + 0.05), (x + ox, y + oy, hit.z + 2.2))
                if h2 is not None:
                    bad.append("%s: noe %.2f m over gulvet" % (name, h2.z - hit.z))
                    break
    if not quiet:
        report(not bad, "1) GameModes spor (objektspor fra Flag.Z + 2 m): 2 flagg og %d spawnpunkter treffer gulvet på 13,9 m, 2,2 m fritt over %s" % (
            sum(len(b["spawns"]) for b in PLAN["bases"]), "; ".join(bad[:4])))
    return not bad


# ---------------------------------------------------------------- 2) doors
def door_rays(T, d, side=0.0):
    """True when both rays (1.0 and 1.9 m) through the door (moved `side` m along the wall) are free."""
    yaw = math.radians(d["yaw"])
    nx, ny = math.cos(yaw), math.sin(yaw)          # front normal
    tx, ty = -ny, nx                               # along the wall
    cx, cy = d["x"] + tx * side, d["y"] + ty * side
    for h in (1.0, 1.9):
        a = (cx - nx * 0.4, cy - ny * 0.4, d["z"] + h)
        b = (cx + nx * 0.4, cy + ny * 0.4, d["z"] + h)
        if ray(T, a, b)[0] is not None:
            return False
    return True


def check_doors(T, quiet=False):
    bad = [d for d in G["doors"] if not d["no_opening"] and not door_rays(T, d)]
    if not quiet:
        report(not bad, "2) alle %d døråpninger (unntatt den forseglede alarmdøra) er åpne i 1,0 og 1,9 m %s" % (
            sum(1 for d in G["doors"] if not d["no_opening"]), "; ".join("%d %s" % (d["id"], d["room"]) for d in bad[:4])))
    return not bad


# ---------------------------------------------------------------- 3) stairs
def check_stairs(T, lift=None, quiet=False):
    bad = []
    prev = {}
    for k, st in enumerate(G["steps"]):
        x0, y0, x1, y1 = st["rect"]
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        want = st["ztop"] + (0.25 if lift == k else 0.0)
        hit, _ = ray(T, (cx, cy, st["ztop"] + 0.6), (cx, cy, st["ztop"] - 1.0))
        if hit is None or abs(hit.z - want) > 0.03:
            bad.append("%s %.1f/%.1f: %s" % (st["stair"], cx, cy, "%.2f" % hit.z if hit is not None else "ingen"))
            continue
        if st["kind"] == "step" and st["stair"] != "Sittetrinn":   # the seat steps are 0.47 m on purpose (stair beside)
            p = prev.get(st["stair"])
            if p is not None and abs(hit.z - p) > 0.205:
                bad.append("%s %.1f/%.1f: trinn %.2f m" % (st["stair"], cx, cy, abs(hit.z - p)))
            prev[st["stair"]] = hit.z
    if not quiet:
        report(not bad, "3) %d trinn og repos ligger der dataene sier, opptrinn <= 0,20 m %s" % (len(G["steps"]), "; ".join(bad[:4])))
    return not bad


def lifted_world(k):
    st = G["steps"][k]
    x0, y0, x1, y1 = st["rect"]
    plate = box_polys(((x0 + x1) / 2, (y0 + y1) / 2, st["ztop"] + 0.125), (x1 - x0, y1 - y0, 0.25))
    return world(extra=[plate])


# ---------------------------------------------------------------- 4) rails
def rail_samples():
    out = []
    for w in G["walls"]:
        if w["k"] != "rail" or w["lv"] not in (H.P2, H.P3, H.P4):
            continue
        lv, o, c = w["lv"], w["o"], w["c"]
        floors = G["floors"][lv]

        def fl(x, y):
            return any(r[0] - 1e-3 <= x <= r[2] + 1e-3 and r[1] - 1e-3 <= y <= r[3] + 1e-3 for r in floors)
        for a, b, h in w["solid"]:
            if h is not None:
                continue
            t = a + 0.25
            while t < b - 0.2:
                pa = (c - 0.2, t) if o == "V" else (t, c - 0.2)
                pb = (c + 0.2, t) if o == "V" else (t, c + 0.2)
                fa, fb = fl(*pa), fl(*pb)
                if fa != fb:
                    s = 1.0 if fa else -1.0
                    out.append((lv, o, c, t, s))
                t += 0.5
    return out


def check_rails(T, quiet=False):
    bad = []
    S = rail_samples()
    for lv, o, c, t, s in S:
        z = H.LEVELS[lv] + 1.0
        ok = False
        for dt in (-0.02, 0.02):          # the glass panels have 1 cm joints: one of two rays 4 cm apart must meet glass
            if o == "V":
                a, b = (c - s * 0.4, t + dt, z), (c + s * 0.6, t + dt, z)
            else:
                a, b = (t + dt, c - s * 0.4, z), (t + dt, c + s * 0.6, z)
            hit, dist = ray(T, a, b)
            ok |= hit is not None and dist <= 0.7     # rail glass, a column or a wall at the edge, at most 0.3 m beyond it
        if not ok:
            bad.append("%s %s=%.2f t=%.2f" % (H.LEVEL_NAMES[lv], "x" if o == "V" else "y", c, t))
    if not quiet:
        report(not bad, "4) rekkverk langs hele atriekanten på 2.-4. etasje (%d stråler hver 0,5 m) %s" % (len(S), "; ".join(bad[:4])))
    return not bad


# ---------------------------------------------------------------- 5) nothing to stand on over Plan 4 (Full parts)
def check_k10(T, extra=(), quiet=False):
    """Up-facing faces of Full parts higher than a rail (Z4 + 1.3) inside the atrium, with room for a body over them."""
    ri = H.RING_IN
    bad = []
    sets = [(n, vp) for n, vp in FULL_POLYS.items() if vp and KIND[n] == H.MC_FULL] + [("test", e) for e in extra]
    for n, (verts, polys) in sets:
        for p in polys:
            vs = [Vector(verts[i]) for i in p]
            nrm = (vs[1] - vs[0]).cross(vs[2] - vs[0])
            if nrm.length < 1e-9:
                continue
            nrm.normalize()
            ctr = sum(vs, Vector()) / len(vs)
            if nrm.z > 0.7 and ctr.z > Z4 + 1.3 and abs(ctr.x) < ri[0] - 0.1 and abs(ctr.y) < ri[1] - 0.1 and "Tak" not in n:
                probe = ctr + Vector((0.013, 0.017, 0.01))     # off the shared edges of neighbouring roofs
                if ray(T, tuple(probe), tuple(probe + Vector((0, 0, 1.84))))[0] is not None:
                    continue                                   # something that stops bodies sits right on it
                bad.append("%s (%.1f, %.1f, %.1f)" % (n, ctr.x, ctr.y, ctr.z))
    if not quiet:
        report(not bad, "5) ingen flate å stå på over 4. etasje inne i atriet i modelldelene med full kollisjon (K10) %s" % "; ".join(bad[:4]))
    return not bad


# ---------------------------------------------------------------- 6) body test
def inside_wall(T, x, y, z):
    """The point is inside a wall (<= 0.5 m thick): rays both ways along X or along Y meet a face within 0.45 m (no walkable gap is that narrow)."""
    for (dx, dy) in ((1, 0), (0, 1)):
        a = ray(T, (x, y, z), (x + dx * 0.45, y + dy * 0.45, z))[0]
        b = ray(T, (x, y, z), (x - dx * 0.45, y - dy * 0.45, z))[0]
        if a is not None and b is not None:
            return True
    return False


def check_body(T, quiet=False):
    bad = []
    n = 0
    for lv in (H.P1, H.P2, H.P3, H.P4):
        z0 = H.LEVELS[lv]
        for r in G["floors"][lv]:
            x = r[0] + 0.25
            while x < r[2]:
                y = r[1] + 0.25
                while y < r[3]:
                    hit, _ = ray(T, (x, y, z0 + 0.3), (x, y, z0 - 0.2))
                    if hit is not None and abs(hit.z - z0) < 0.02:
                        n += 1
                        up, _, nrm = ray(T, (x, y, z0 + 0.02), (x, y, z0 + 1.84), normal=True)
                        # a hit whose face looks up is the top of a solid we stand inside (a wall, a planter): no floor
                        if up is not None and up.z - z0 > 0.25 and nrm.z < 0.5 and not inside_wall(T, x, y, z0 + 0.5):
                            bad.append("%s (%.2f, %.2f): %.2f m" % (H.LEVEL_NAMES[lv], x, y, up.z - z0))
                    y += 0.5
                x += 0.5
    if not quiet:
        report(not bad, "6) kroppsprøve på 1.-4. etasje (%d gulvpunkter hver 0,5 m): ingenting henger lavere enn 1,84 m %s" % (
            n, "; ".join(bad[:4])))
    return not bad


def main():
    report(not MISSING, "alle modelldeler med kollisjon finnes som .blend %s" % ", ".join(MISSING[:4]))
    T = world()
    check_spawns(T)
    counter(not check_spawns(T, extra_points=[(-8.4, 31.5)], quiet=True), "spawnpunkt i Trapp sør (ingen gulv på 4. etasje, under reposet)")
    Td = world(with_pieces=False)
    check_doors(Td)
    wood = [d for d in G["doors"] if not d["no_opening"] and d["kind"] in ("tre", "brann", "dobbel")]
    blocked = sum(1 for d in wood if not door_rays(Td, d, side=0.7 + d["w"] / 2 - 0.45))
    counter(blocked >= 0.8 * len(wood), "strålene 0,7 m ved siden av %d tre-/branndører (%d treffer veggen)" % (len(wood), blocked))
    check_stairs(T)
    counter(not check_stairs(lifted_world(5), lift=5, quiet=True), "ett trinn hevet 0,25 m")
    check_rails(T)
    s0 = rail_samples()[0]
    lv, o, c, t, s = s0
    pid = min((p for p in G["panels"] if p["lv"] == lv and p["type"] == 0),
              key=lambda p: (p["c"][0] - (c if o == "V" else t)) ** 2 + (p["c"][1] - (t if o == "V" else c)) ** 2)["id"]
    counter(not check_rails(world(skip_panels=(pid,)), quiet=True), "glassfelt %d fjernet" % pid)
    check_k10(T)
    counter(not check_k10(T, extra=[box_polys((0.0, 0.0, Z4 + 3.0), (2.0, 2.0, 0.1))], quiet=True), "en testplate på 16,9 m midt i atriet")
    check_body(T)
    beam = box_polys((-14.4, 0.0, Z4 + 1.5), (0.3, 2.0, 0.2))
    counter(not check_body(world(extra=[beam]), quiet=True), "en bjelke 1,5 m over gulvet i vestkorridoren på 4. etasje")
    print()
    print("Alt OK" if errors == 0 else "%d FEIL" % errors)
    sys.exit(1 if errors else 0)


if __name__ == "__main__":
    main()
