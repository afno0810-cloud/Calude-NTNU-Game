"""Checks the map data in Source/Gloshaugen/Public/GSCampus.h (plain Python, no Blender needed).

    python Tools/Blender/check_campus.py

  a) every building that is not scenery (bBackdrop) lies completely inside PlayArea()
  b) no road or path (made half a width longer at each end, like BuildRoads does) runs into a building
  c) spawn points and flags lie inside PlayArea(); spawn points at least 1.5 m outside every building,
     each flag inside its own base building
  d) the two base buildings are the same size, and buildings and flags are mirror images about (0, 10)
     (turn the map 180 degrees and west becomes east)
  e) for each base a path ends at most 2.5 m from one of its end doors and at most 0.5 m to the side of it
     (the door position comes from the same formula as Tools/Blender/buildings.py, turned when Yaw is 180)
  f) every base spawn point lies behind its own building: between the outer wall and the edge of PlayArea(),
     and inside the Y extent of the building
  g) no base spawn point can be seen from the north path, the south promenade (both also 2.5 m to the side), the
     paths from the base doors or the passages: 2D lines with the buildings and RigFences() (mirrored) as walls
  h) the openings in the rig area are either closed or wide enough to walk through (fences are 0.6 m thick)
  i) the invisible wall round the map (BlockH in GSCampusBuilder.cpp) has a lid and is higher than a grapple hook
     (GSCharacter.cpp) can carry you from the highest roof, and higher than every building outside
There must be exactly 10 buildings. d) to i) each have a counter-test (the values from before),
which must FAIL: otherwise the check would say OK for the wrong reason.
Prints one line per check and exits with code 1 if anything is wrong.
"""
import math
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = open(os.path.join(ROOT, "Source", "Gloshaugen", "Public", "GSCampus.h"), encoding="utf-8").read()
NUM = r"(-?[\d.]+)f?"
errors = 0


def report(ok, text):
    global errors
    print(("OK      " if ok else "FEIL    ") + text)
    if not ok:
        errors += 1


def section(name):
    a = SRC.index(name)
    return SRC[a:SRC.index("return List;", a)]


# ---------------------------------------------------------------- read the data
buildings = []
for m in re.finditer(r'\{\s*TEXT\("([^"]+)"\),\s*' + r',\s*'.join([NUM] * 7) + r',\s*(\w+),\s*(true|false)\s*(?:,\s*(true|false)\s*)?\}', SRC):
    x, y, sx, sy, yaw = (float(m.group(i)) for i in range(2, 7))
    buildings.append(dict(name=m.group(1), x0=x - sx / 2, x1=x + sx / 2, y0=y - sy / 2, y1=y + sy / 2,
                          x=x, y=y, sx=sx, sy=sy, yaw=yaw, height=float(m.group(7)), backdrop=m.group(11) == "true"))
roads = []
for m in re.finditer(r'\{\s*FVector2D\(' + NUM + r',\s*' + NUM + r'\),\s*FVector2D\(' + NUM + r',\s*' + NUM + r'\),\s*' + NUM + r',\s*(true|false)\s*\}',
                     section("Roads()")):
    roads.append(tuple(float(m.group(i)) for i in range(1, 6)))
pa = re.search(r'PlayArea\(\)\s*\{\s*return FBox2D\(FVector2D\(' + NUM + r',\s*' + NUM + r'\),\s*FVector2D\(' + NUM + r',\s*' + NUM + r'\)', SRC)
PX0, PY0, PX1, PY1 = (float(pa.group(i)) for i in range(1, 5))
bases = []
for m in re.finditer(r'\{\s*TEXT\("([^"]+)"\),\s*TEXT\("([^"]+)"\),\s*FVector\(' + NUM + r',\s*' + NUM + r',\s*' + NUM + r'\),\s*' + NUM +
                     r',\s*\{((?:\s*\{[^{}]*\}\s*,?)*)\s*\}\s*\}', section("Bases()")):
    pts = [(float(a), float(b)) for a, b in re.findall(r'\{\s*' + NUM + r',\s*' + NUM + r'\s*\}', m.group(7))]
    bases.append(dict(name=m.group(1), building=m.group(2), flag=(float(m.group(3)), float(m.group(4))), spawns=pts))
spawns = [(float(a), float(b)) for a, b in re.findall(r'\{\s*' + NUM + r',\s*' + NUM + r'\s*\}', section("SpawnPoints()"))]

# footprints that block (the front bay of Hovedbygningen sticks out of its box)
footprints = [(b["name"], b["x0"], b["y0"], b["x1"], b["y1"]) for b in buildings]
footprints.append(("Hovedbygningen frontkarnapp", -12.0, -21.0, 12.0, -14.0))

print(f"Lest: {len(buildings)} bygg, {len(roads)} veier, {len(bases)} baser, {len(spawns)} spawnpunkter, "
      f"spillomraade X {PX0:g}..{PX1:g}, Y {PY0:g}..{PY1:g}")
report(len(buildings) > 0 and len(roads) > 0 and len(bases) == 2 and len(spawns) > 0, "alle lister ble lest")
report(len(buildings) == 10, f"noyaktig 10 bygg (fant {len(buildings)}; taarnet skal vaere borte)")


def in_area(x, y, margin=0.0):
    return PX0 + margin <= x <= PX1 - margin and PY0 + margin <= y <= PY1 - margin


def dist_to_rect(x, y, r):
    dx = max(r[1] - x, 0.0, x - r[3])
    dy = max(r[2] - y, 0.0, y - r[4])
    inside = r[1] < x < r[3] and r[2] < y < r[4]
    return -1.0 if inside else math.hypot(dx, dy)


# ---------------------------------------------------------------- a) buildings inside the play area
for b in buildings:
    if b["backdrop"]:
        continue
    ok = in_area(b["x0"], b["y0"]) and in_area(b["x1"], b["y1"])
    report(ok, f"a) {b['name']} ligger inne i spillomraadet")

# ---------------------------------------------------------------- b) roads do not run into buildings
def road_box(ax, ay, bx, by, w):
    """The road as BuildRoads lays it: a rectangle half a width longer at each end (roads run along X or Y)."""
    L = math.hypot(bx - ax, by - ay)
    ux, uy = (bx - ax) / L, (by - ay) / L
    hw = w / 2
    sax, say, sbx, sby = ax - ux * hw, ay - uy * hw, bx + ux * hw, by + uy * hw
    return (min(sax, sbx) - hw * abs(uy), min(say, sby) - hw * abs(ux), max(sax, sbx) + hw * abs(uy), max(say, sby) + hw * abs(ux))


for (ax, ay, bx, by, w) in roads:
    if ax != bx and ay != by:
        report(False, f"b) vei ({ax:g},{ay:g})-({bx:g},{by:g}) er skraa; sjekken kan bare rette veier")
        continue
    r = road_box(ax, ay, bx, by, w)
    hit = [f[0] for f in footprints if r[0] < f[3] and f[1] < r[2] and r[1] < f[4] and f[2] < r[3]]
    report(not hit, f"b) vei ({ax:g},{ay:g})-({bx:g},{by:g}) bredde {w:g} treffer ikke bygg" + (f" (treffer {', '.join(hit)})" if hit else ""))

# ---------------------------------------------------------------- c) spawn points and flags
for (x, y) in spawns + [p for b in bases for p in b["spawns"]]:
    near = [f[0] for f in footprints if dist_to_rect(x, y, f) < 1.5]
    report(in_area(x, y) and not near, f"c) spawnpunkt ({x:g},{y:g}) inne i omraadet og minst 1,5 m fra bygg" + (f" (for naer {', '.join(near)})" if near else ""))
for b in bases:
    x, y = b["flag"]
    own = next((f for f in footprints if f[0] == b["building"]), None)
    report(in_area(x, y) and own is not None and dist_to_rect(x, y, own) < 0,
           f"c) flagget i base {b['name']} ({x:g},{y:g}) ligger inne i {b['building']}")

# ---------------------------------------------------------------- d) the bases mirror each other about (0, 10)
MIRROR = (0.0, 10.0)


def mirrored(a, b, tol=0.01):
    """Is point b the point a turned 180 degrees about MIRROR?"""
    return abs(a[0] + b[0] - 2 * MIRROR[0]) <= tol and abs(a[1] + b[1] - 2 * MIRROR[1]) <= tol


def same_size(a, b):
    return abs(a["sx"] - b["sx"]) < 0.01 and abs(a["sy"] - b["sy"]) < 0.01


base_b = [next((b for b in buildings if b["name"] == base["building"]), None) for base in bases]
if len(bases) == 2 and all(base_b):
    w, e = base_b
    report(same_size(w, e), f"d) basebyggene er like store ({w['sx']:g} x {w['sy']:g} og {e['sx']:g} x {e['sy']:g})")
    report(mirrored((w["x"], w["y"]), (e["x"], e["y"])) and abs(abs(w["yaw"] - e["yaw"]) - 180.0) < 0.01,
           "d) basebyggene ligger speilvendt om (0, 10) og er snudd 180 grader mot hverandre")
    report(mirrored(bases[0]["flag"], bases[1]["flag"]), "d) flaggene ligger speilvendt om (0, 10)")
    report(len(bases[0]["spawns"]) == len(bases[1]["spawns"]) and
           all(any(mirrored(p, q) for q in bases[1]["spawns"]) for p in bases[0]["spawns"]),
           "d) spawnpunktene i de to basene er speilbilder av hverandre")
    report(not same_size(w, dict(e, sx=56.0)), "d) motprove: et basebygg paa 56 m blir avvist")
    report(not mirrored((w["x"], w["y"]), (e["x"], e["y"] + 1.0)), "d) motprove: et bygg 1 m ute av speilingen blir avvist")
else:
    report(False, "d) fant ikke begge basebyggene")

# ---------------------------------------------------------------- e) a path ends at an end door of each base
WALL_T, WINDOW_W, MIN_PIER = 0.4, 1.8, 1.4   # as in buildings.py and GSCampusBuilder.cpp


def end_doors(b):
    """World positions of the doors in the two short (+-X) walls of an enterable building."""
    length = b["sy"] - 2 * WALL_T
    nw = int(math.floor((length - MIN_PIER) / (WINDOW_W + MIN_PIER)))
    pier = (length - nw * WINDOW_W) / (nw + 1)
    s = -length / 2 + pier + WINDOW_W / 2 + (nw // 2) * (pier + WINDOW_W)   # local y of the door
    turn = -1.0 if abs(b["yaw"] % 360.0 - 180.0) < 0.01 else 1.0
    return [(b["x"] + sgn * b["sx"] / 2, b["y"] + turn * s) for sgn in (-1.0, 1.0)]


def path_at_door(door, road_list):
    """A path end at most 2.5 m from the door along the path and at most 0.5 m to the side of it."""
    for (ax, ay, bx, by, _w) in road_list:
        for (px, py) in ((ax, ay), (bx, by)):
            if abs(px - door[0]) <= 2.5 and abs(py - door[1]) <= 0.5:
                return True
    return False


for base, b in zip(bases, base_b):
    if not b:
        continue
    report(abs(b["yaw"] % 180.0) < 0.01, f"e) {b['name']} staar rett (Yaw 0 eller 180), ellers gjelder ikke dorformelen")
    doors = end_doors(b)
    hit = [d for d in doors if path_at_door(d, roads)]
    report(bool(hit), f"e) base {base['name']}: en sti ender ved en endedor i {b['name']} "
                      f"(dorene: {', '.join(f'({d[0]:g}, {d[1]:.2f})' for d in doors)})")
    old = [(-100.0, 10.0, -78.0, 10.0, 3.0), (78.0, 10.0, 95.0, 10.0, 3.0)]
    report(not any(path_at_door(d, old) for d in doors), f"e) motprove: de gamle stubbene paa y = 10 treffer ikke dorene i {b['name']}")

# ---------------------------------------------------------------- f) base spawns behind the own building


def behind_base(p, b):
    """Between the outer wall of the base building and the edge of the play area, inside the building's Y extent."""
    if not b["y0"] <= p[1] <= b["y1"]:
        return False
    if b["x"] < MIRROR[0]:
        return PX0 < p[0] < b["x0"]
    return b["x1"] < p[0] < PX1


for base, b in zip(bases, base_b):
    if not b:
        continue
    for p in base["spawns"]:
        report(behind_base(p, b), f"f) spawnpunkt ({p[0]:g},{p[1]:g}) i base {base['name']} ligger bak {b['name']}")
    old = (-140.0, -14.0) if b["x"] < 0 else (140.0, -14.0)
    report(not behind_base(old, b), f"f) motprove: det gamle spawnpunktet ({old[0]:g},{old[1]:g}) foran {b['name']} blir avvist")
    report(all(p in spawns for p in base["spawns"]), f"f) alle spawnpunktene i base {base['name']} staar ogsaa i SpawnPoints()")

# ---------------------------------------------------------------- g) nobody on a path can see into the rig areas
def mirror_pt(p):
    return (2 * MIRROR[0] - p[0], 2 * MIRROR[1] - p[1])


rig_west = [((float(a), float(b)), (float(c), float(d))) for a, b, c, d in re.findall(
    r'\{\s*FVector2D\(' + NUM + r',\s*' + NUM + r'\),\s*FVector2D\(' + NUM + r',\s*' + NUM + r'\)\s*\}', section("RigFences()"))]
rig = rig_west + [(mirror_pt(a), mirror_pt(b)) for a, b in rig_west]
report(len(rig_west) >= 2, f"g) leste {len(rig_west)} riggjerder (vest) fra RigFences()")


def extend(a, b, e=0.1):
    """A fence has posts and feet: it reaches a little past its end points (and joins are closed)."""
    L = math.hypot(b[0] - a[0], b[1] - a[1])
    ux, uy = (b[0] - a[0]) / L, (b[1] - a[1]) / L
    return ((a[0] - ux * e, a[1] - uy * e), (b[0] + ux * e, b[1] + uy * e))


def crosses(p, q, a, b):
    def cr(o, u, v):
        return (u[0] - o[0]) * (v[1] - o[1]) - (u[1] - o[1]) * (v[0] - o[0])
    return cr(a, b, p) * cr(a, b, q) < 0 and cr(p, q, a) * cr(p, q, b) < 0


def rect_sides(f):
    _, x0, y0, x1, y1 = f
    return [((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))]


def observers():
    """Where players walk: the north path, the south promenade (each also 2.5 m to both sides), the paths from the
    base doors and the passages, every 0.5 m, inside the edge fence."""
    pts = []
    for (ax, ay, bx, by, w) in roads:
        if (ax, ay, bx, by) not in [(-158.0, -30.0, 158.0, -30.0), (-200.0, 120.0, 200.0, 120.0), (-125.0, -30.0, -125.0, -3.0),
                                    (125.0, -30.0, 125.0, -3.0), (-125.0, 23.0, -125.0, 120.0), (125.0, 23.0, 125.0, 120.0),
                                    (-78.0, -30.0, -78.0, 120.0), (78.0, -30.0, 78.0, 120.0)]:
            continue
        L = math.hypot(bx - ax, by - ay)
        ux, uy = (bx - ax) / L, (by - ay) / L
        offs = (-2.5, 0.0, 2.5) if ay == by else (0.0,)
        for o in offs:
            for i in range(int(L / 0.5) + 1):
                x, y = ax + ux * i * 0.5 - uy * o, ay + uy * i * 0.5 + ux * o
                if in_area(x, y, 0.8) and not any(dist_to_rect(x, y, f) < 0.5 for f in footprints):
                    pts.append((x, y))
    return pts


OBS = observers()


def seen_spawns(fences):
    """Base spawn points (and 0.4 m round them, a player is 0.8 m wide) that some observer has a free line to.
    Buildings and the rig fences block; nothing else is counted (trees and props are random), so it is strict."""
    walls = [s for f in footprints for s in rect_sides(f)] + [extend(a, b) for a, b in fences]
    out = []
    for base in bases:
        for (sx, sy) in base["spawns"]:
            body = [(sx, sy), (sx + 0.4, sy), (sx - 0.4, sy), (sx, sy + 0.4), (sx, sy - 0.4)]
            # only walls near the line can block it; check the cheap bounding box first
            for o in OBS:
                if any(not any(crosses(o, p, a, b) for a, b in walls
                               if min(a[0], b[0]) <= max(o[0], p[0]) and max(a[0], b[0]) >= min(o[0], p[0])
                               and min(a[1], b[1]) <= max(o[1], p[1]) and max(a[1], b[1]) >= min(o[1], p[1]))
                       for p in body):
                    out.append(((sx, sy), o))
                    break
    return out


report(len(OBS) > 1000, f"g) {len(OBS)} utsiktspunkter langs stiene")
seen = seen_spawns(rig)
report(not seen, "g) ingen spawnpunkter i basene kan ses fra nordstien, sorpromenaden, dorstiene eller passasjene" +
       (" (ses: " + ", ".join(f"({p[0]:g},{p[1]:g}) fra ({o[0]:g},{o[1]:g})" for p, o in seen[:6]) + ")" if seen else ""))
old_rig = [((-164.5, -3.0), (-152.0, -3.0)), ((-164.5, 23.0), (-152.0, 23.0))]
old_rig += [(mirror_pt(a), mirror_pt(b)) for a, b in old_rig]
report(bool(seen_spawns(old_rig)), "g) motprove: riggomraadet fra runde 2 (4 m aapning uten sluse) blir avvist")
short = old_rig[:2] + [((-157.0, -6.0), (-145.0, -6.0)), ((-157.0, 26.0), (-145.0, 26.0))]
short += [(mirror_pt(a), mirror_pt(b)) for a, b in short]
report(bool(seen_spawns(short)), "g) motprove: en kort sluse uten ben mot veggen (12 m, 3 m ut) blir avvist")


# ---------------------------------------------------------------- h) the gaps in the rig area are walkable
def seg_dist(p, q, a, b):
    """Shortest distance between segments p-q and a-b (2D)."""
    if crosses(p, q, a, b):
        return 0.0

    def pt_seg(c, u, v):
        dx, dy = v[0] - u[0], v[1] - u[1]
        L2 = dx * dx + dy * dy
        t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((c[0] - u[0]) * dx + (c[1] - u[1]) * dy) / L2))
        return math.hypot(c[0] - u[0] - t * dx, c[1] - u[1] - t * dy)
    return min(pt_seg(p, a, b), pt_seg(q, a, b), pt_seg(a, p, q), pt_seg(b, p, q))


FENCE_T, WALK = 0.6, 1.2   # the collision of a fence panel is 0.6 m thick (its feet); a player needs 0.8 m and some room


def tight_gaps(fences):
    """Pairs of fences (or fence and building) that neither touch nor leave room to walk between them."""
    bad = []
    for i, (a, b) in enumerate(fences):
        others = [(f"gjerde {j}", s) for j, s in enumerate(fences) if j != i] + \
                 [(f[0], s) for f in footprints for s in rect_sides(f)]
        for name, (c, d) in others:
            g = seg_dist(a, b, c, d)
            if 0.3 < g < FENCE_T + WALK:
                bad.append((i, name, g))
    return bad


tight = tight_gaps(rig_west)
report(not tight, f"h) alle aapninger i riggomraadet er minst {FENCE_T + WALK:g} m (eller lukket)" +
       (" (" + ", ".join(f"gjerde {i} - {n}: {g:.2f} m" for i, n, g in tight[:4]) + ")" if tight else ""))
narrow = rig_west + [((-157.0, -4.0), (-150.0, -4.0))]
report(bool(tight_gaps(narrow)), "h) motprove: en sluse bare 1 m fra riggjerdet blir avvist")

# ---------------------------------------------------------------- i) the invisible wall round the map is out of reach
CPP = open(os.path.join(ROOT, "Source", "Gloshaugen", "Private", "GSCampusBuilder.cpp"), encoding="utf-8").read()
CHAR = open(os.path.join(ROOT, "Source", "Gloshaugen", "Private", "GSCharacter.cpp"), encoding="utf-8").read()
mh = re.search(r'BlockH\s*=\s*' + NUM, CPP)
mr = re.search(r'GetForwardVector\(\)\s*\*\s*' + NUM + r',\s*ECC_WorldStatic', CHAR)          # grapple trace length (cm)
ms = re.search(r'FMath::Clamp\(float\(To\.Size\(\)\)\s*\*\s*[\d.]+f?,\s*' + NUM + r',\s*' + NUM + r'\)\s*\+\s*FVector\(0\.f,\s*0\.f,\s*' + NUM, CHAR)
report(bool(mh and mr and ms), "i) leste BlockH (GSCampusBuilder.cpp) og gripekroken (rekkevidde, fart og loft i GSCharacter.cpp)")
if mh and mr and ms:
    block_h = float(mh.group(1))
    reach, vmax, lift = float(mr.group(1)) / 100.0, float(ms.group(2)) / 100.0, float(ms.group(3)) / 100.0
    roof = max(b["height"] for b in buildings if not b["backdrop"])
    rise = (vmax + lift) ** 2 / (2 * 9.81)   # straight up, no air drag


    def wall_ok(h, lid):
        """A single hook from the highest roof stays below the wall, the box is closed on top (hooking the wall
        again and again would climb any wall), and every building outside is lower than the wall, so a line from
        inside the box to it hits the wall first."""
        return roof + 1.7 + rise < h and lid and all(b["height"] < h for b in buildings if b["backdrop"])
    lid = "// the lid" in CPP
    report(wall_ok(block_h, lid), f"i) den usynlige veggen er {block_h:g} m med lokk ({'ja' if lid else 'nei'}): et krokskudd fra "
           f"hoyeste tak ({roof:g} m) naar {roof + 1.7 + rise:.0f} m (rekkevidde {reach:g} m), og bygg utenfor er lavere")
    report(not wall_ok(14.0, lid), "i) motprove: veggen fra runde 2 (14 m) blir avvist")
    report(not wall_ok(block_h, False), "i) motprove: vegg uten lokk blir avvist")

print(f"\n{'Alt OK' if errors == 0 else str(errors) + ' feil'}")
sys.exit(1 if errors else 0)
