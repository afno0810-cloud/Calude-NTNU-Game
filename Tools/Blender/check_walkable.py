"""Checks that doors, ramp tops and the routes through the building models are open. Reads the .blend files,
changes nothing.

    blender --background --python Tools/Blender/check_walkable.py

Shoots rays through every door of SM_Hovedbygningen, along the two free ways through Lysgaarden, off the top of
the ramps in the building models and at the screen walls in the two bases. Prints OK / SPERRET (blocked) /
MANGLER (something that should be there is not) per ray, and exits with code 1 if anything is wrong.
Every group has a counter-test: a ray that MUST hit, so a check cannot say OK because it looks at nothing
(for example if the model is empty or the ray is in the wrong place).
Coordinates below are Unreal space (X east, Y south); the models are mirrored in Y inside Blender.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

from buildings import RAMP_ANGLE, RAMP_W, WALL_T, read_base_buildings, read_buildings, safe  # noqa: E402

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


def hit_z(obj, x, y, z_from, z_to):
    """Height of the first surface straight down from (x, y, z_from), or None if nothing is hit before z_to."""
    inv = obj.matrix_world.inverted()
    a = inv @ Vector((x, -y, z_from))
    hit, loc, _n, _i = obj.ray_cast(a, Vector((0, 0, -1)), distance=z_from - z_to)
    return (obj.matrix_world @ loc).z if hit else None


def check(ok, label):
    print(f"{'OK' if ok else 'FEIL'}  {label}")
    if not ok:
        failures.append(label)


def check_main_building():
    obj = load("SM_Hovedbygningen")
    # (label, door centre x, y, axis the door faces along, outward sign, door width)
    doors = [("hoveddor", 0.0, -18.0, "Y", -1, 3.6),
             ("endedor vest", -55.0, 0.0, "X", -1, 2.4), ("endedor oest", 55.0, 0.0, "X", 1, 2.4),
             ("bakdor til bakgaarden", 0.0, 14.0, "Y", 1, 2.4),
             ("sordor vest (til floeyen)", -48.0, 14.0, "Y", 1, 2.4), ("sordor oest (til floeyen)", 48.0, 14.0, "Y", 1, 2.4)]
    for label, x, y, axis, out, width in doors:
        for z in (0.3, 0.9, 1.7):
            if axis == "Y":
                s, e = (x, y + out * 1.5, z), (x, y - out * 1.5, z)
            else:
                s, e = (x + out * 1.5, y, z), (x - out * 1.5, y, z)
            ray(obj, f"SM_Hovedbygningen {label}, z {z}", s, e)
        # counter-test: 1 m to the side of the door frame the same ray must hit the wall
        side = width / 2 + 1.0
        if axis == "Y":
            s, e = (x + side, y + out * 1.5, 0.6), (x + side, y - out * 1.5, 0.6)
        else:
            s, e = (x + out * 1.5, y + side, 0.6), (x - out * 1.5, y + side, 0.6)
        ray(obj, f"SM_Hovedbygningen {label}: motprove, veggen 1 m til siden for karmen", s, e, want_hit=True)
    # the tower is gone: nothing over the back yard
    ray(obj, "SM_Hovedbygningen: taarnet er borte (rett ned over bakgaarden)", (0.0, 20.0, 60.0), (0.0, 20.0, 0.5))
    ray(obj, "SM_Hovedbygningen: motprove, taket finnes (rett ned over vestfloeyen av hovedblokka)", (-30.0, 5.0, 60.0), (-30.0, 5.0, 0.5),
        want_hit=True)
    # Lysgaarden, the middle route: one free way north-south past the plinth and one east-west under the galleries
    for z in (0.3, 0.9, 1.7):
        ray(obj, f"Lysgaarden nord-sor (x 2,5), z {z}", (2.5, -13.0, z), (2.5, -3.0, z))
        ray(obj, f"Lysgaarden oest-vest (y -0,4), z {z}", (-16.0, -0.4, z), (16.0, -0.4, z))
    ray(obj, "Lysgaarden: motprove, sokkelen staar midt paa nordsiden", (0.0, -8.9, 0.3), (0.0, -3.0, 0.3), want_hit=True)
    top = hit_z(obj, 0.0, -7.5, 2.0, -1.0)
    check(top is not None and abs(top - 0.5) < 0.005, f"Lysgaarden: toppen av sokkelen er paa z = 0,5 (maalt {top})")


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
        # stepping off: straight down on the ramp and on the floor beside it. The step between them must be
        # at most 0.45 m (what a player walks up) along at least 0.65 m at the top of the ramp.
        step, n_ok, x = 0.05, 0, ix1 - 2.0
        while x < ix1 - 0.02:
            zr = hit_z(obj, x, cy, zt + 1.2, z0 - 0.5)
            zf = hit_z(obj, x, oy0 - 0.4, zt + 1.2, zt - 1.0)
            if zr is not None and zf is not None and abs(zf - zr) <= 0.45:
                n_ok += 1
            x += step
        check(n_ok * step >= 0.65, f"{name} rampe {kf}: trinn paa hoyst 0,45 m over {n_ok * step:.2f} m i toppen (krav 0,65 m)")
        # counter-test: 2 m before the top the ramp is far below the floor
        zr = hit_z(obj, ix1 - 2.0, cy, zt + 1.2, z0 - 0.5)
        zf = hit_z(obj, ix1 - 2.0, oy0 - 0.4, zt + 1.2, zt - 1.0)
        check(zr is not None and zf is not None and zf - zr > 0.45,
              f"{name} rampe {kf}: motprove, 2 m for toppen er trinnet for hoyt ({'-' if zr is None or zf is None else f'{zf - zr:.2f}'} m)")
    return obj


def check_base(name):
    """Capture the Flag bases: the north and the south door (x 0) look straight at a screen wall, the way round is open."""
    obj = load(f"SM_Bygg_{safe(name)}")
    ray(obj, f"{name}: skjermvegg innenfor norddora", (0.0, -10.0, 1.5), (0.0, 0.0, 1.5), want_hit=True)
    ray(obj, f"{name}: skjermvegg innenfor sordora", (0.0, 10.0, 1.5), (0.0, 0.0, 1.5), want_hit=True)
    ray(obj, f"{name}: fritt forbi skjermveggen i nord (x -5)", (-5.0, -10.0, 1.5), (-5.0, 0.0, 1.5))
    ray(obj, f"{name}: fritt forbi skjermveggen i sor (x 5)", (5.0, 10.0, 1.5), (5.0, 0.0, 1.5))


check_main_building()
for n in ("Kjelhuset", "Gamle fysikk", "Gamle kjemi", "Hovedbygningen vest", "Hovedbygningen oest"):
    check_ramps(n)
for n in read_base_buildings():
    check_base(n)

if failures:
    print(f"\n{len(failures)} sjekk(er) feilet")
    sys.stdout.flush()
    sys.exit(1)
print("\nAlt OK: dorer, ramper, Lysgaarden og basene")
