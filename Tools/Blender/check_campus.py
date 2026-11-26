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
There must be exactly 10 buildings. d), e) and f) each have a counter-test (the values from before plan 02),
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
                          x=x, y=y, sx=sx, sy=sy, yaw=yaw, backdrop=m.group(11) == "true"))
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

print(f"\n{'Alt OK' if errors == 0 else str(errors) + ' feil'}")
sys.exit(1 if errors else 0)
