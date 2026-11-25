"""Small physics props for effects: spent casings and dropped magazines.
    python Tools/Blender/fx_models.py  ->  SM_FX_CasingRifle, SM_FX_CasingPistol, SM_FX_ShellShotgun,
                                           SM_FX_MagRifle, SM_FX_MagPistol
Along +X (casing mouth forward), origin at the centre so they tumble nicely.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from gs_common import box, export, join_all, prism, reset_scene  # noqa: E402
from gen_models import M, col, lathe  # noqa: E402


def along_x(o, dx=0.0):
    o.rotation_euler = (0, math.radians(90), 0)
    o.location.x += dx
    return o


def casing(name, r, length, neck, mat, rim=True):
    reset_scene()
    m = M()
    h = length / 2
    prof = [(0.0, -h), (r * 1.02 if rim else r, -h), (r * 1.02 if rim else r, -h + 0.0012), (r * 0.82, -h + 0.0016), (r * 0.82, -h + 0.0028), (r, -h + 0.0034)]
    if neck:
        prof += [(r, h - length * 0.28), (r * 0.72, h - length * 0.16), (r * 0.72, h), (r * 0.6, h)]
    else:
        prof += [(r, h), (r * 0.86, h)]
    prof += [(r * 0.6 if neck else r * 0.86, -h + 0.004)]
    along_x(lathe("Case", prof, m[mat], segs=20))
    along_x(lathe("Primer", [(0.0, -h - 0.0002), (r * 0.32, -h - 0.0002), (0.0, -h - 0.0002)], m["chrome"], segs=12))
    join_all(name)
    export(name)


def shotgun_shell():
    reset_scene()
    m = M()
    along_x(lathe("Hull", [(0.0, -0.012), (0.0105, -0.012), (0.0105, 0.035), (0.009, 0.037), (0.0, 0.038)], col((0.65, 0.05, 0.04), 0, 0.5), segs=20), -0.012)
    along_x(lathe("Head", [(0.0, -0.035), (0.0115, -0.035), (0.0115, -0.033), (0.0107, -0.032), (0.0107, -0.016), (0.0, -0.016)], m["brass"], segs=20), -0.0)
    join_all("SM_FX_ShellShotgun")
    export("SM_FX_ShellShotgun")


def magazine(name, length, width, depth, curve, mat):
    reset_scene()
    m = M()
    n = 8
    front, back = [], []
    for i in range(n):
        t = i / (n - 1)
        a = curve * t
        cx = 0.3 * (1 - math.cos(a)) if curve else 0.0
        z = -length * t
        front.append((cx + depth / 2, z))
        back.append((cx - depth / 2, z))
    prism("Body", front + back[::-1], width, m[mat], bevel=0.0015)
    for i in range(1, n - 1, 2):
        x = (front[i][0] + back[i][0]) / 2
        box(f"Rib{i}", (depth * 0.85, width + 0.001, 0.004), (x, 0, front[i][1]), m[mat], rot=(0, -math.degrees(curve * i / (n - 1)), 0), bevel=0.0008)
    xb = (front[-1][0] + back[-1][0]) / 2
    box("Floor", (depth * 1.25, width * 1.15, 0.008), (xb, 0, -length - 0.003), m["poly"], rot=(0, -math.degrees(curve), 0), bevel=0.002)
    box("Lips", (depth * 0.7, width * 0.7, 0.006), (0, 0, 0.003), m[mat], bevel=0.001)
    box("Round", (depth * 0.6, width * 0.5, 0.006), (0.002, 0, 0.007), m["brass"], bevel=0.002)
    obj = join_all(name)
    # centre it so it tumbles around its middle
    bb = [obj.matrix_world @ v.co for v in obj.data.vertices]
    cz = (max(v.z for v in bb) + min(v.z for v in bb)) / 2
    for v in obj.data.vertices:
        v.co.z -= cz
    export(name)


if __name__ == "__main__":
    casing("SM_FX_CasingRifle", 0.0048, 0.045, True, "brass")
    casing("SM_FX_CasingPistol", 0.0049, 0.019, False, "brass")
    shotgun_shell()
    magazine("SM_FX_MagRifle", 0.19, 0.022, 0.06, math.radians(28), "dark")
    magazine("SM_FX_MagPistol", 0.13, 0.022, 0.032, 0.0, "dark")
    print("fx models done")
