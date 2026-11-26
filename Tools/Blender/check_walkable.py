"""Checks that doors and ramp tops in the building models are not blocked. Reads the .blend files, changes nothing.

    blender --background --python Tools/Blender/check_walkable.py

Shoots short rays through every door of SM_Hovedbygningen and sideways off the top of the ramps in the
building models, and prints OK / SPERRET (blocked) per ray. Exits with code 1 if anything is blocked.
Coordinates below are Unreal space (X east, Y south); the models are mirrored in Y inside Blender.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

from buildings import RAMP_ANGLE, RAMP_W, WALL_T, read_buildings, safe  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MODELS = os.path.join(ROOT, "SourceArt", "Models")
failures = []


def load(name):
    bpy.ops.wm.open_mainfile(filepath=os.path.join(MODELS, name + ".blend"))
    obj = bpy.data.objects.get(name) or next(o for o in bpy.data.objects if o.type == "MESH")
    return obj


def ray(obj, label, start, end, want_hit=False):
    """start/end in Unreal meters (building-local). Prints OK when the result is what we want."""
    a = Vector((start[0], -start[1], start[2]))
    b = Vector((end[0], -end[1], end[2]))
    inv = obj.matrix_world.inverted()
    a, b = inv @ a, inv @ b
    d = b - a
    hit, loc, _n, _i = obj.ray_cast(a, d.normalized(), distance=d.length)
    ok = hit == want_hit
    where = f" (treff ved {loc.x:.2f}, {-loc.y:.2f}, {loc.z:.2f})" if hit else ""
    print(f"{'OK' if ok else 'SPERRET' if not want_hit else 'MANGLER'}  {label}{where}")
    if not ok:
        failures.append(label)


def check_main_building():
    obj = load("SM_Hovedbygningen")
    # (label, door centre x, y, axis the door faces along, outward sign)
    doors = [("hoveddor", 0.0, -18.0, "Y", -1),
             ("endedor vest", -55.0, 0.0, "X", -1), ("endedor oest", 55.0, 0.0, "X", 1),
             ("sordor midt (inn fra taarnet)", 0.0, 14.0, "Y", 1),
             ("sordor vest (til floeyen)", -48.0, 14.0, "Y", 1), ("sordor oest (til floeyen)", 48.0, 14.0, "Y", 1),
             ("taarndor mot sor", 0.0, 26.0, "Y", 1)]
    for label, x, y, axis, out in doors:
        for z in (0.3, 0.9, 1.7):
            if axis == "Y":
                s, e = (x, y + out * 1.5, z), (x, y - out * 1.5, z)
            else:
                s, e = (x + out * 1.5, y, z), (x - out * 1.5, y, z)
            ray(obj, f"SM_Hovedbygningen {label}, z {z}", s, e)


def check_ramps(name):
    b = next(b for b in read_buildings() if b["name"] == name)
    obj = load(f"SM_Bygg_{safe(name)}")
    FH = b["fh"]
    floors = max(1, int(math.floor(b["h"] / FH + 0.01)))
    ramp_len = FH / math.tan(math.radians(RAMP_ANGLE))
    ix1 = b["sx"] / 2 - WALL_T
    iy1 = b["sy"] / 2 - WALL_T
    oy0 = iy1 - RAMP_W
    ox0 = ix1 - ramp_len
    cy = (oy0 + iy1) / 2
    for kf in range(1, floors):
        z0, zt = (kf - 1) * FH, kf * FH
        # 1.3 m before the top: step sideways (-Y, towards the floor) at three heights above the floor
        x = ix1 - 1.3
        for dz in (0.3, 0.9, 1.5):
            ray(obj, f"{name} rampe {kf}: toppen, {dz} m over gulvet", (x, cy, zt + dz), (x, cy - 2.0, zt + dz))
        # middle of the ramp: the rail on the floor side must still be there
        xm = ox0 + ramp_len / 2
        zr = z0 + FH / 2  # ramp surface at the middle
        ray(obj, f"{name} rampe {kf}: midt paa, rekkverket finnes", (xm, cy, zr + 0.9), (xm, cy - 2.0, zr + 0.9), want_hit=True)


check_main_building()
for n in ("Kjelhuset", "Gamle fysikk", "Gamle kjemi", "Hovedbygningen vest", "Hovedbygningen oest"):
    check_ramps(n)

if failures:
    print(f"\n{len(failures)} stråle(r) feilet")
    sys.stdout.flush()
    sys.exit(1)
print("\nAlle dorer og ramper er aapne")
