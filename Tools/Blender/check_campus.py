"""Checks the map data in Source/Gloshaugen/Public/GSCampus.h (plain Python, no Blender needed).

    python Tools/Blender/check_campus.py

  a) every building that is not scenery (bBackdrop) lies completely inside PlayArea()
  b) no road or path (made half a width longer at each end, like BuildRoads does) runs into a building
  c) spawn points and flags lie inside PlayArea(); spawn points at least 1.5 m outside every building,
     each flag inside its own base building
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
                          backdrop=m.group(11) == "true"))
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

print(f"\n{'Alt OK' if errors == 0 else str(errors) + ' feil'}")
sys.exit(1 if errors else 0)
