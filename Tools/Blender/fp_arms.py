"""First-person arms (gloved hands and jacket sleeves) that hold every weapon:
    python Tools/Blender/fp_arms.py   ->  SM_FPArm_R (hand around a pistol grip) and SM_FPArm_L (hand under a handguard)

Origin = where the weapon is held: right = top of the pistol grip, left = underside of the handguard.
gs_common space: +X forward, +Z up, +Y = LEFT (Blender is right handed), meters.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402
from gs_common import box, cyl, export, join_all, material, reset_scene  # noqa: E402


def limb(name, p0, p1, r0, r1, mat, verts=16):
    """Tapered tube from p0 (radius r0) to p1 (radius r1)."""
    a, b = Vector(p0), Vector(p1)
    d = b - a
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r0, radius2=r1, depth=d.length, location=(a + b) / 2)
    o = bpy.context.active_object
    o.name = name
    o.rotation_mode = "QUATERNION"
    o.rotation_quaternion = d.normalized().to_track_quat("Z", "Y")
    o.data.materials.append(mat)
    mod = o.modifiers.new("Bevel", "BEVEL")
    mod.width = 0.004
    mod.segments = 2
    return o


def knuckle(name, loc, r, mat):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=r, location=loc)
    o = bpy.context.active_object
    o.name = name
    o.data.materials.append(mat)
    return o


def sleeve_details(prefix, wrist, elbow, shoulder, m, side, watch=False):
    """Folds that follow the arm, a velcro cuff strap, a shoulder flag patch and (left) a watch."""
    w, e, sh = Vector(wrist), Vector(elbow), Vector(shoulder)
    d = (e - w).normalized()
    for i, t in enumerate((0.22, 0.5, 0.74)):
        # soft, slightly off-centre folds (fabric bunches on one side)
        p = w.lerp(e, t) + Vector((0, side * 0.002, 0.002 * (1 if i % 2 else -1)))
        rr = 0.042 + 0.008 * t + 0.0012
        limb(f"{prefix}Fold{i}", p - d * 0.01, p + d * 0.01, rr * 0.99, rr, m["sleeve"], verts=20)
    # cuff strap with velcro tab
    c = w.lerp(e, 0.06)
    limb(f"{prefix}Strap", c - d * 0.009, c + d * 0.009, 0.0445, 0.0445, m["strap"], verts=20)
    box(f"{prefix}Tab", (0.022, 0.012, 0.006), tuple(c + Vector((0, side * 0.04, 0.02))), m["strap"], bevel=0.002)
    # folds at the elbow and a flag patch on the upper arm
    de = (sh - e).normalized()
    for i, t in enumerate((0.08, 0.2)):
        p = e.lerp(sh, t)
        limb(f"{prefix}EFold{i}", p - de * 0.01, p + de * 0.01, 0.0515 + t * 0.006, 0.052 + t * 0.006, m["sleeve"], verts=20)
    pp = e.lerp(sh, 0.55) + Vector((0, side * 0.052, 0.01))
    box(f"{prefix}Flag", (0.05, 0.004, 0.034), tuple(pp), m["flagred"], bevel=0.001)
    box(f"{prefix}FlagV", (0.008, 0.0045, 0.034), tuple(pp + Vector((-0.006, 0, 0))), m["flagwhite"], bevel=0)
    box(f"{prefix}FlagH", (0.05, 0.0045, 0.008), tuple(pp), m["flagwhite"], bevel=0)
    box(f"{prefix}FlagV2", (0.004, 0.005, 0.034), tuple(pp + Vector((-0.006, 0, 0))), m["flagblue"], bevel=0)
    box(f"{prefix}FlagH2", (0.05, 0.005, 0.004), tuple(pp), m["flagblue"], bevel=0)
    if watch:
        c2 = w.lerp(e, -0.02)
        limb(f"{prefix}WatchBand", c2 - d * 0.009, c2 + d * 0.009, 0.0275, 0.0275, m["knuckle"], verts=20)
        box(f"{prefix}WatchFace", (0.03, 0.03, 0.01), tuple(c2 + Vector((0, 0, 0.026))), m["watch"], bevel=0.004)


def mats():
    return dict(
        glove=material("M_Glove", (0.03, 0.03, 0.03), roughness=0.8),
        knuckle=material("M_Rubber", (0.02, 0.02, 0.02), roughness=0.95),
        sleeve=material("M_Sleeve", (0.18, 0.2, 0.13), roughness=1.0),
        cuff=material("M_Leather", (0.08, 0.06, 0.04), roughness=0.6),
        strap=material("M_Fabric", (0.1, 0.1, 0.1), roughness=1.0),
        watch=material("M_DarkSteel", (0.02, 0.02, 0.022), metallic=0.7, roughness=0.55),
        flagred=material("M_C_7202015_0", (0.73, 0.02, 0.15), roughness=0.8),
        flagwhite=material("M_White", (0.9, 0.9, 0.9), roughness=0.6),
        flagblue=material("M_C_0012645_0", (0.0, 0.13, 0.46), roughness=0.8),
    )


def right_arm():
    reset_scene()
    m = mats()
    R = -1  # right side is -Y
    # palm and back of the hand, wrapped around a grip slanted ~15 degrees
    box("Palm", (0.075, 0.032, 0.09), (-0.032, R * 0.02, -0.055), m["glove"], rot=(0, -15, 0), bevel=0.01)
    box("KnucklePad", (0.02, 0.034, 0.08), (-0.004, R * 0.022, -0.06), m["knuckle"], rot=(0, -15, 0), bevel=0.006)
    # four fingers around the front of the grip, index finger forward on the trigger
    for i, z in enumerate((-0.03, -0.053, -0.075, -0.096)):
        x = 0.012 - (z + 0.03) * 0.25
        if i == 0:
            limb("Index1", (x, R * 0.018, z + 0.005), (x + 0.035, R * 0.008, z + 0.002), 0.0095, 0.008, m["glove"])
            limb("Index2", (x + 0.035, R * 0.008, z + 0.002), (x + 0.045, 0.0, z - 0.012), 0.008, 0.0075, m["glove"])
            continue
        r = 0.0095 - i * 0.0006
        limb(f"F{i}a", (x - 0.005, R * 0.02, z), (x + 0.014, R * 0.0, z), r, r * 0.95, m["glove"])
        limb(f"F{i}b", (x + 0.014, R * 0.0, z), (x - 0.006, -R * 0.02, z), r * 0.95, r * 0.85, m["glove"])
        knuckle(f"K{i}", (x + 0.014, 0.0, z), r * 1.05, m["glove"])
    # thumb over the top of the grip on the left side
    limb("Thumb1", (-0.045, -R * 0.005, -0.02), (-0.015, -R * 0.022, -0.012), 0.011, 0.0095, m["glove"])
    limb("Thumb2", (-0.015, -R * 0.022, -0.012), (0.012, -R * 0.022, -0.016), 0.0095, 0.008, m["glove"])
    # wrist, cuff, forearm in the jacket sleeve, elbow out of view
    wrist = (-0.075, R * 0.028, -0.085)
    elbow = (-0.36, R * 0.115, -0.2)
    limb("Wrist", (-0.05, R * 0.022, -0.07), wrist, 0.026, 0.024, m["glove"])
    limb("Cuff", wrist, (-0.1, R * 0.036, -0.095), 0.03, 0.031, m["cuff"])
    limb("Forearm", (-0.095, R * 0.034, -0.093), elbow, 0.042, 0.05, m["sleeve"], verts=20)
    shoulder = (-0.6, R * 0.2, -0.3)
    limb("Upper", elbow, shoulder, 0.05, 0.058, m["sleeve"], verts=20)
    sleeve_details("R", wrist, elbow, shoulder, m, R)
    # glove: padded back of the hand and fingertips
    box("BackPad", (0.05, 0.006, 0.06), (-0.035, R * 0.038, -0.058), m["knuckle"], rot=(0, -15, 0), bevel=0.004)
    join_all("SM_FPArm_R")
    export("SM_FPArm_R", also_glb=False)


def left_arm():
    reset_scene()
    m = mats()
    L = 1  # left side is +Y
    # palm under the handguard, fingers up the left side, thumb along the right side
    box("Palm", (0.085, 0.045, 0.024), (-0.005, L * 0.006, -0.03), m["glove"], bevel=0.01)
    for i, x in enumerate((-0.03, -0.008, 0.014, 0.034)):
        r = 0.0095 - abs(i - 1.5) * 0.0005
        limb(f"F{i}a", (x, L * 0.026, -0.03), (x + 0.004, L * 0.032, -0.005), r, r * 0.95, m["glove"])
        limb(f"F{i}b", (x + 0.004, L * 0.032, -0.005), (x + 0.008, L * 0.024, 0.012), r * 0.95, r * 0.85, m["glove"])
        knuckle(f"K{i}", (x, L * 0.028, -0.03), r * 1.05, m["glove"])
    limb("Thumb1", (-0.04, -L * 0.02, -0.035), (-0.005, -L * 0.03, -0.012), 0.011, 0.0095, m["glove"])
    limb("Thumb2", (-0.005, -L * 0.03, -0.012), (0.025, -L * 0.028, -0.004), 0.0095, 0.008, m["glove"])
    wrist = (-0.06, L * 0.01, -0.06)
    elbow = (-0.32, L * 0.15, -0.22)
    limb("Wrist", (-0.035, L * 0.006, -0.04), wrist, 0.026, 0.024, m["glove"])
    limb("Cuff", wrist, (-0.085, L * 0.022, -0.075), 0.03, 0.031, m["cuff"])
    limb("Forearm", (-0.08, L * 0.02, -0.073), elbow, 0.042, 0.05, m["sleeve"], verts=20)
    shoulder = (-0.55, L * 0.27, -0.33)
    limb("Upper", elbow, shoulder, 0.05, 0.058, m["sleeve"], verts=20)
    sleeve_details("L", wrist, elbow, shoulder, m, L, watch=True)
    box("BackPad", (0.06, 0.035, 0.006), (-0.008, L * 0.004, -0.044), m["knuckle"], bevel=0.003)
    join_all("SM_FPArm_L")
    export("SM_FPArm_L", also_glb=False)


if __name__ == "__main__":
    right_arm()
    left_arm()
