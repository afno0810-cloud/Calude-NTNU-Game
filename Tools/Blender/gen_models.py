"""Builds a 3D model for every item in Tools/ItemCatalog/catalog.py.

    python Tools/Blender/gen_models.py            # everything
    python Tools/Blender/gen_models.py AK47 Kebab # only these ids

Hand-made models (catalog model={"custom": ...}) come from their own scripts (weapon_ag3.py, weapons_more.py ...).
Writes SourceArt/Models/SM_<Id>.fbx/.glb/.blend and Tools/ItemCatalog/model_metrics.json (muzzle and sight
heights that the C++ code uses to place the weapon in first person).
Conventions: meters, +X forward (muzzle/blade), +Z up, origin at the top of the grip.
"""
import json
import math
import os
import sys

import bpy  # noqa: I001  (bpy must come before bmesh)
import bmesh

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "ItemCatalog"))
from gs_common import ROOT, box, cyl, export, join_all, material, prism, reset_scene  # noqa: E402
import catalog  # noqa: E402

METRICS_PATH = os.path.join(ROOT, "Tools", "ItemCatalog", "model_metrics.json")


# ------------------------------------------------------------------ helpers
def M():
    return dict(
        steel=material("M_Steel", (0.05, 0.05, 0.055), metallic=0.85, roughness=0.45),
        dark=material("M_DarkSteel", (0.02, 0.02, 0.022), metallic=0.7, roughness=0.55),
        chrome=material("M_Chrome", (0.6, 0.6, 0.62), metallic=1.0, roughness=0.15),
        wood=material("M_Wood", (0.30, 0.15, 0.07), roughness=0.65),
        lwood=material("M_LightWood", (0.55, 0.38, 0.2), roughness=0.6),
        bake=material("M_Bakelite", (0.12, 0.06, 0.03), roughness=0.5),
        poly=material("M_BlackPolymer", (0.03, 0.03, 0.03), roughness=0.85),
        green=material("M_OliveDrab", (0.16, 0.19, 0.1), roughness=0.8),
        tan=material("M_Tan", (0.45, 0.38, 0.25), roughness=0.85),
        brass=material("M_Brass", (0.6, 0.45, 0.15), metallic=0.9, roughness=0.35),
        blade=material("M_Blade", (0.75, 0.76, 0.78), metallic=1.0, roughness=0.2),
        rubber=material("M_Rubber", (0.02, 0.02, 0.02), roughness=0.95),
        glass=material("M_Glass", (0.6, 0.75, 0.8), roughness=0.05),
        lens=material("M_Lens", (0.05, 0.15, 0.2), metallic=0.5, roughness=0.05),
        white=material("M_White", (0.9, 0.9, 0.9), roughness=0.6),
        red=material("M_Red", (0.7, 0.03, 0.03), roughness=0.5),
        gold=material("M_Gold", (0.75, 0.55, 0.2), metallic=1.0, roughness=0.3),
        paper=material("M_Paper", (0.85, 0.8, 0.7), roughness=0.9),
        fabric=material("M_Fabric", (0.1, 0.1, 0.1), roughness=1.0),
    )


def col(rgb, metallic=0.0, rough=0.5):
    r, g, b = rgb
    return material(f"M_C_{int(r*99):02d}{int(g*99):02d}{int(b*99):02d}_{int(metallic*9)}", tuple(rgb), metallic, rough)


def sph(name, r, loc, mat, scale=(1, 1, 1), segs=20):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segs, ring_count=max(8, segs // 2), radius=r, location=loc)
    o = bpy.context.active_object
    o.name = name
    o.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.data.materials.append(mat)
    return o


def cone(name, r1, r2, depth, loc, mat, axis="Z", verts=24):
    rot = {"X": (0, math.radians(90), 0), "Y": (math.radians(90), 0, 0), "Z": (0, 0, 0)}[axis]
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2, depth=depth, location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    o.data.materials.append(mat)
    return o


def hemi(name, r, loc, mat, scale=(1, 1, 1), segs=24):
    """Upper half sphere (helmets, caps)."""
    o = sph(name, r, loc, mat, scale, segs)
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -1e-4], context="VERTS")
    bm.to_mesh(o.data)
    bm.free()
    return o


def lathe(name, profile, mat, segs=32, loc=(0, 0, 0), scale=(1, 1, 1)):
    """Surface of revolution around Z from a (radius, z) profile, bottom to top. Ends are capped
    when the first/last radius is > 0. Gives smooth bottles, jars, cups, buns."""
    me = bpy.data.meshes.new(name)
    o = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(o)
    bm = bmesh.new()
    rings = []
    for r, z in profile:
        ring = []
        for k in range(segs):
            a = 2 * math.pi * k / segs
            ring.append(bm.verts.new((loc[0] + math.cos(a) * r * scale[0], loc[1] + math.sin(a) * r * scale[1], loc[2] + z * scale[2])))
        rings.append(ring)
    for a, b in zip(rings, rings[1:]):
        for k in range(segs):
            j = (k + 1) % segs
            bm.faces.new((a[k], a[j], b[j], b[k]))
    if profile[0][0] > 1e-5:
        bm.faces.new(rings[0][::-1])
    if profile[-1][0] > 1e-5:
        bm.faces.new(rings[-1])
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    o.data.materials.append(mat)
    for f in me.polygons:
        f.use_smooth = True
    return o


def tube(name, path, radii, mat, segs=16, squash=(1.0, 1.0), caps=True):
    """Tube swept along a polyline (list of xyz) with per-point radius. squash scales the
    cross-section (side, up). Used for bananas, sausages, baguettes, wraps, cables."""
    me = bpy.data.meshes.new(name)
    o = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(o)
    bm = bmesh.new()
    from mathutils import Vector
    P = [Vector(v) for v in path]
    rings = []
    up0 = Vector((0, 0, 1))
    for i, c in enumerate(P):
        t = (P[min(i + 1, len(P) - 1)] - P[max(i - 1, 0)]).normalized()
        side = t.cross(up0)
        if side.length < 1e-4:
            side = t.cross(Vector((0, 1, 0)))
        side.normalize()
        up = side.cross(t).normalized()
        r = radii[i]
        ring = [bm.verts.new(c + side * math.cos(2 * math.pi * k / segs) * r * squash[0] + up * math.sin(2 * math.pi * k / segs) * r * squash[1]) for k in range(segs)]
        rings.append(ring)
    for a, b in zip(rings, rings[1:]):
        for k in range(segs):
            j = (k + 1) % segs
            bm.faces.new((a[k], a[j], b[j], b[k]))
    if caps:
        bm.faces.new(rings[0][::-1])
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    o.data.materials.append(mat)
    for f in me.polygons:
        f.use_smooth = True
    return o


def arc_path(n, length, bend, axis_len="X"):
    """n points along X over `length`, bowed upward in Z by `bend` at the middle."""
    return [((i / (n - 1) - 0.5) * length, 0.0, bend * (1 - (2 * i / (n - 1) - 1) ** 2)) for i in range(n)]


def rotated(o, rot_deg):
    o.rotation_euler = [math.radians(a) for a in rot_deg]
    return o


def finish(id, scale=1.0, metrics=None):
    obj = join_all(f"SM_{id}")
    if scale != 1.0:
        obj.scale = (scale, scale, scale)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    export(f"SM_{id}")
    if metrics is not None:
        return {k: v * scale for k, v in metrics.items()}
    return None


# ------------------------------------------------------------------ guns
def grip(m, mat, w=0.032):
    prism("Grip", [(-0.055, -0.03), (0.005, -0.03), (-0.02, -0.15), (-0.075, -0.14)], w, mat)
    box("TriggerGuard", (0.07, 0.01, 0.006), (0.015, 0, -0.07), m["steel"])
    box("TriggerGuardF", (0.006, 0.01, 0.04), (0.048, 0, -0.05), m["steel"])
    box("Trigger", (0.006, 0.006, 0.025), (0.012, 0, -0.045), m["steel"])


def build_pistol(p, m):
    s, h = p["slide"], p["h"]
    frame = m[p.get("frame", "poly")]
    slide = m[p.get("slidecol", "dark")]
    box("Slide", (s, 0.028, h), (s / 2 - 0.06, 0, 0.012 + h / 2), slide)
    for i in range(6):
        box(f"Serr{i}", (0.003, 0.03, h * 0.7), (-0.05 + i * 0.007, 0, 0.012 + h / 2), m["dark"], bevel=0)
    box("Frame", (s * 0.8, 0.026, 0.022), (s * 0.4 - 0.06, 0, 0.0), frame)
    # --- detail pass: ejection port with brass, front serrations, slide stop, takedown, accessory rail, beavertail
    box("Port", (0.035, 0.03, h * 0.45), (0.0, 0.002, 0.012 + h * 0.72), m["dark"], bevel=0)
    box("PortBrass", (0.022, 0.026, 0.008), (0.0, 0.003, 0.012 + h * 0.7), m["brass"], bevel=0.001)
    for i in range(4):
        box(f"FSerr{i}", (0.003, 0.0305, h * 0.6), (s - 0.11 + i * 0.007, 0, 0.012 + h / 2), m["dark"], bevel=0)
    box("SlideStop", (0.025, 0.004, 0.006), (0.01, -0.0145, 0.008), m["steel"], bevel=0.001)
    cyl("Takedown", 0.004, 0.028, (0.04, 0, 0.005), m["steel"], axis="Y", verts=10)
    box("Rail", (0.04, 0.02, 0.006), (s * 0.62 - 0.06, 0, -0.012), frame)
    for i in range(3):
        box(f"RailSlot{i}", (0.004, 0.021, 0.004), (s * 0.62 - 0.075 + i * 0.012, 0, -0.016), m["dark"], bevel=0)
    box("Beavertail", (0.025, 0.026, 0.01), (-0.07, 0, 0.0), frame, rot=(0, 15, 0))
    box("MagRelease", (0.008, 0.004, 0.008), (-0.004, -0.0145, -0.015), m["steel"])
    # raked grip (about 18 degrees) with a finger-groove front strap, rounded backstrap and stippled side panels
    k = math.tan(math.radians(18))
    front = [(0.0 - k * z, -z) for z in (0.0, 0.018, 0.03, 0.042, 0.055, 0.068, 0.082, 0.096, 0.108)]
    front = [(x + (0.003 if i in (2, 4, 6) else 0.0), z) for i, (x, z) in enumerate(front)]
    back = [(-0.056 - k * z, -z) for z in (0.108, 0.08, 0.04, 0.012)]
    prism("Grip", front + back + [(-0.066, 0.004), (-0.05, 0.006)], 0.029, frame, bevel=0.005)
    for side in (-1, 1):
        prism(f"GripPanel{side}", [(-0.012 - k * 0.02, -0.02), (-0.016 - k * 0.092, -0.092), (-0.048 - k * 0.092, -0.092), (-0.048 - k * 0.02, -0.02)],
              0.002, m["rubber"], y=side * 0.0148, bevel=0.0008)
    cyl("Bore", 0.006, 0.01, (s - 0.06, 0, 0.012 + h * 0.6), m["dark"])
    box("MagBase", (0.05, 0.031, 0.01), (-0.028 - k * 0.112, 0, -0.112), m["dark"], rot=(0, -18, 0), bevel=0.003)
    # trigger guard loop (squared front for a support finger) and a curved trigger
    tube("TriggerGuard", [(0.008, 0, -0.004), (0.052, 0, -0.006), (0.056, 0, -0.012), (0.052, 0, -0.03), (0.042, 0, -0.04), (0.005, 0, -0.04), (-0.012, 0, -0.028)],
         [0.003] * 7, frame, segs=8)
    prism("Trigger", [(0.017, -0.004), (0.023, -0.004), (0.021, -0.016), (0.016, -0.03), (0.012, -0.029), (0.015, -0.016)], 0.006, m["steel"], bevel=0.0006)
    top = 0.012 + h
    box("FrontSight", (0.006, 0.004, 0.006), (s - 0.07, 0, top + 0.003), m["dark"])
    box("RearSight", (0.008, 0.02, 0.007), (-0.045, 0, top + 0.0035), m["dark"])
    return dict(muzzle=s - 0.06, sight=top + 0.006)


def build_revolver(p, m):
    """Stainless double-action revolver: fluted cylinder, full-lug barrel with ejector shroud,
    top strap with rear sight, hammer spur, wooden grip with medallion."""
    b = p["barrel"]
    st = m["chrome"]
    x0 = 0.058  # front of frame / start of barrel
    o = lathe("Barrel", [(0.0, 0.0), (0.0095, 0.0), (0.0095, b - 0.004), (0.0085, b), (0.004, b), (0.004, b - 0.01), (0.0, b - 0.01)], st, segs=28)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (x0, 0, 0.036)
    box("Rib", (b, 0.012, 0.009), (x0 + b / 2, 0, 0.046), st, bevel=0.002)
    for k in range(int(b / 0.008)):
        box(f"RibVent{k}", (0.004, 0.0122, 0.002), (x0 + 0.006 + k * 0.008, 0, 0.0505), m["dark"], bevel=0)
    box("Lug", (b * 0.92, 0.017, 0.02), (x0 + b * 0.46, 0, 0.02), st, bevel=0.004)
    box("Ramp", (0.016, 0.004, 0.014), (x0 + b - 0.01, 0, 0.056), st, rot=(0, -10, 0), bevel=0.001)
    box("RampInsert", (0.006, 0.0042, 0.004), (x0 + b - 0.012, 0, 0.06), m["red"], bevel=0)
    # frame: lower frame, top strap over the cylinder window, recoil shield
    prism("Frame", [(-0.045, -0.002), (0.06, -0.002), (0.06, 0.012), (0.035, 0.012), (0.035, 0.008), (-0.03, 0.008), (-0.04, 0.03), (-0.048, 0.04), (-0.06, 0.038), (-0.065, 0.02)], 0.03, st, bevel=0.002)
    box("TopStrap", (0.08, 0.022, 0.009), (0.022, 0, 0.0545), st, bevel=0.003)
    box("RearSight", (0.012, 0.016, 0.007), (-0.02, 0, 0.061), m["dark"], bevel=0.001)
    box("RecoilShield", (0.008, 0.032, 0.05), (-0.014, 0, 0.033), st, bevel=0.003)
    box("Yoke", (0.008, 0.03, 0.045), (0.054, 0, 0.033), st, bevel=0.002)
    # fluted cylinder with chamber mouths and a latch
    cx, cz = 0.02, 0.032
    o = lathe("Cylinder", [(0.0, -0.022), (0.019, -0.022), (0.021, -0.02), (0.021, 0.02), (0.019, 0.022), (0.0, 0.022)], st, segs=36)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (cx, 0, cz)
    for k in range(6):
        a = k * math.pi / 3 + math.pi / 6
        box(f"Flute{k}", (0.026, 0.006, 0.004), (cx - 0.002, math.cos(a) * 0.0205, cz + math.sin(a) * 0.0205), m["dark"], rot=(math.degrees(a) + 90, 0, 0), bevel=0.0015)
        a2 = k * math.pi / 3
        cyl(f"Chamber{k}", 0.0048, 0.002, (cx + 0.0222, math.cos(a2) * 0.0115, cz + math.sin(a2) * 0.0115), m["dark"], verts=12)
        cyl(f"Case{k}", 0.0047, 0.001, (cx - 0.0225, math.cos(a2) * 0.0115, cz + math.sin(a2) * 0.0115), m["brass"], verts=12)
    cyl("Ejector", 0.004, 0.02, (cx + 0.032, 0, cz), st, verts=12)
    box("Latch", (0.012, 0.004, 0.008), (-0.006, 0.0165, 0.034), m["dark"], bevel=0.001)
    # hammer with checkered spur, trigger and guard
    prism("Hammer", [(-0.05, 0.036), (-0.04, 0.036), (-0.039, 0.05), (-0.046, 0.06), (-0.062, 0.062), (-0.06, 0.056), (-0.05, 0.052)], 0.008, m["steel"], bevel=0.0008)
    prism("Trigger", [(0.008, 0.006), (0.014, 0.006), (0.01, -0.012), (0.004, -0.028), (0.0, -0.026), (0.006, -0.01)], 0.006, m["steel"], bevel=0.0006)
    tube("Guard", [(0.032, 0, 0.0), (0.033, 0, -0.018), (0.024, 0, -0.033), (0.004, 0, -0.037), (-0.012, 0, -0.03), (-0.02, 0, -0.015)], [0.0028] * 6, st, segs=8)
    # wooden grip with finger grooves and a medallion
    pts = [(-0.02, 0.0), (-0.032, -0.03), (-0.03, -0.045), (-0.036, -0.06), (-0.034, -0.075), (-0.04, -0.09), (-0.05, -0.108),
           (-0.09, -0.106), (-0.096, -0.09), (-0.088, -0.05), (-0.072, -0.012), (-0.065, 0.0), (-0.05, 0.012)]
    prism("Grip", pts, 0.036, m["wood"], bevel=0.004)
    for sy in (-1, 1):
        cyl(f"Medallion{sy}", 0.006, 0.002, (-0.065, sy * 0.0185, -0.035), m["gold"], axis="Y", verts=16)
        cyl(f"GripScrew{sy}", 0.0025, 0.002, (-0.06, sy * 0.0186, -0.065), st, axis="Y", verts=10)
    box("ButtCap", (0.045, 0.034, 0.004), (-0.07, 0, -0.107), st, bevel=0.0012)
    return dict(muzzle=x0 + b, sight=0.064)


def build_p90(p, m):
    """Bullpen PDW: rounded polymer shell, thumbhole grip, front hand loop, see-through top magazine."""
    poly = m["poly"]
    pts = [(-0.3, -0.035), (-0.29, -0.07), (-0.26, -0.1), (-0.21, -0.105), (-0.16, -0.075), (-0.11, -0.045), (-0.05, -0.04),
           (0.08, -0.04), (0.14, -0.03), (0.18, -0.015), (0.2, 0.01), (0.2, 0.028), (0.175, 0.05), (0.12, 0.06), (-0.27, 0.06), (-0.3, 0.035)]
    prism("Shell", pts, 0.055, poly, bevel=0.008)
    # thumbhole grip and the front loop for the support hand
    prism("Grip", [(-0.06, -0.04), (-0.02, -0.04), (-0.03, -0.12), (-0.07, -0.12), (-0.08, -0.08)], 0.04, poly, bevel=0.006)
    prism("Loop", [(0.06, -0.04), (0.1, -0.04), (0.12, -0.1), (0.11, -0.13), (0.06, -0.13), (0.06, -0.12), (0.1, -0.115), (0.085, -0.06), (0.06, -0.06)], 0.035, poly, bevel=0.004)
    box("Trigger", (0.006, 0.008, 0.022), (0.035, 0, -0.06), m["steel"], bevel=0.001)
    tube("Guard", [(0.06, 0, -0.06), (0.06, 0, -0.08), (0.04, 0, -0.085), (0.005, 0, -0.08), (-0.015, 0, -0.06)], [0.003] * 5, poly, segs=8)
    # magazine on top: smoky translucent body with the rounds visible inside
    box("Mag", (0.32, 0.05, 0.024), (-0.07, 0, 0.073), col((0.12, 0.12, 0.1), 0, 0.15), bevel=0.004)
    for k in range(24):
        box(f"Round{k}", (0.006, 0.044, 0.006), (-0.215 + k * 0.012, 0, 0.073), m["brass"], rot=(0, 0, 70 if k % 2 else -70), bevel=0.001)
    box("MagCatch", (0.02, 0.04, 0.012), (-0.25, 0, 0.068), poly, bevel=0.002)
    # optic rail, ring sight and muzzle
    box("Rail", (0.12, 0.022, 0.008), (0.05, 0, 0.09), m["dark"], bevel=0.001)
    for k in range(8):
        box(f"RailSlot{k}", (0.004, 0.0225, 0.003), (0.0 + k * 0.014, 0, 0.0945), poly, bevel=0)
    o = lathe("Sight", [(0.012, -0.015), (0.016, -0.015), (0.016, 0.015), (0.012, 0.015)], m["dark"], segs=24)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0.06, 0, 0.115)
    box("SightBase", (0.03, 0.02, 0.012), (0.06, 0, 0.098), m["dark"], bevel=0.002)
    cyl("SightLens", 0.012, 0.002, (0.075, 0, 0.115), m["lens"], verts=24)
    o = lathe("Muzzle", [(0.0, 0.0), (0.012, 0.0), (0.012, 0.03), (0.009, 0.035), (0.004, 0.035), (0.0, 0.03)], m["dark"], segs=20)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0.198, 0, 0.022)
    box("Selector", (0.04, 0.058, 0.01), (-0.04, 0, -0.04), m["dark"], bevel=0.002)
    for sy in (-1, 1):
        box(f"Groove{sy}", (0.3, 0.002, 0.004), (-0.05, sy * 0.0276, 0.0), m["dark"], bevel=0)
        box(f"Cocking{sy}", (0.02, 0.006, 0.01), (0.12, sy * 0.03, 0.03), m["dark"], bevel=0.002)
    box("Sling", (0.01, 0.06, 0.008), (-0.28, 0, 0.0), m["steel"], bevel=0.002)
    return dict(muzzle=0.235, sight=0.115)


def build_pistolknife(p, m):
    prism("Blade", [(0.03, -0.03), (0.2, -0.025), (0.25, 0.0), (0.2, 0.012), (0.03, 0.012)], 0.004, m["blade"], bevel=0.001)
    box("Guard", (0.012, 0.03, 0.08), (0.025, 0, -0.005), m["steel"])
    cyl("Barrel", 0.008, 0.14, (0.08, 0, 0.032), m["dark"])
    box("Breech", (0.06, 0.024, 0.03), (-0.01, 0, 0.03), m["dark"])
    prism("Grip", [(-0.06, 0.012), (0.02, 0.012), (0.0, -0.1), (-0.07, -0.095)], 0.03, m["wood"])
    box("Trigger", (0.005, 0.005, 0.02), (0.005, 0, -0.005), m["steel"])
    return dict(muzzle=0.15, sight=0.045)


def build_rifle(p, m):
    x0, x1 = p["rx"]
    rh = p["rh"]
    furn = m[p.get("furn", "poly")]
    top = rh - 0.03
    bz = top - rh * 0.4
    br = p.get("barrel_r", 0.009)
    blen = p["barrel"]
    box("Receiver", (x1 - x0, 0.05, rh), ((x0 + x1) / 2, 0, top - rh / 2), m["steel"], bevel=0.003)
    box("EjectionPort", (0.07, 0.052, 0.015), (0.03, 0, top - 0.02), m["dark"], bevel=0)
    # --- detail pass: split line, bolt in the port, pins, charging handle, selector, magwell, sling swivels
    box("SplitLine", (x1 - x0 - 0.01, 0.0515, 0.002), ((x0 + x1) / 2, 0, top - rh * 0.55), m["dark"], bevel=0)
    box("BoltFace", (0.05, 0.048, 0.011), (0.03, 0.002, top - 0.02), m["chrome"], bevel=0.001)
    box("ForwardAssist", (0.022, 0.012, 0.016), (x0 + 0.05, 0.03, top - 0.018), m["dark"], bevel=0.002)
    for i, px in enumerate((x0 + 0.03, (x0 + x1) / 2 + 0.02, x1 - 0.03)):
        cyl(f"Pin{i}", 0.0035, 0.054, (px, 0, top - rh * 0.72), m["dark"], axis="Y", verts=10)
    box("ChargingHandle", (0.03, 0.01, 0.012), (x1 - 0.04, 0.03, top - 0.012), m["dark"], bevel=0.002)
    box("Selector", (0.016, 0.006, 0.01), (-0.01, -0.028, -0.012), m["steel"], rot=(0, 20, 0), bevel=0.001)
    box("MagRelease", (0.01, 0.006, 0.01), (0.05, 0.028, top - rh - 0.005), m["dark"])
    if p.get("mag", "none") not in ("none", "boxbelt", "drum"):
        box("Magwell", (0.05, 0.034, 0.03), (p.get("magx", 0.08), 0, top - rh - 0.008), m["steel"], bevel=0.003)
    box("SlingSwivel", (0.012, 0.004, 0.018), (x0 - 0.02, 0.026, top - rh * 0.5), m["dark"])
    # barrel and handguard
    hlen, hmat, hstyle = p["hg"]
    hmat = m[hmat]
    cyl("Barrel", br, blen + 0.01, (x1 + blen / 2, 0, bz), m["steel"])
    cyl("BarrelStep", br * 1.25, blen * 0.25, (x1 + blen * 0.125, 0, bz), m["steel"], verts=20)
    cyl("Crown", br * 0.55, 0.002, (x1 + blen + 0.0055, 0, bz), m["dark"], verts=16)
    if hstyle == "rail":
        box("Handguard", (hlen, 0.055, rh * 0.85), (x1 + hlen / 2, 0, bz), hmat)
        box("TopRail", (hlen + (x1 - x0), 0.022, 0.008), ((x0 + x1 + hlen) / 2, 0, top + 0.004), m["dark"])
        for i in range(int((hlen + x1 - x0) / 0.012)):
            box(f"RailSlot{i}", (0.004, 0.024, 0.004), (x0 + 0.006 + i * 0.012, 0, top + 0.008), m["dark"], bevel=0)
        # M-LOK slots along the handguard sides and a vented bottom
        for i in range(int(hlen / 0.045)):
            for side in (-1, 1):
                box(f"MLok{i}_{side}", (0.03, 0.004, 0.009), (x1 + 0.03 + i * 0.045, side * 0.027, bz), m["dark"], bevel=0)
    elif hstyle == "round":
        cyl("Handguard", 0.026, hlen, (x1 + hlen / 2, 0, bz), hmat, verts=16)
    elif hstyle == "ak":
        box("HandguardLow", (hlen, 0.05, 0.04), (x1 + hlen / 2, 0, bz - 0.01), hmat)
        cyl("HandguardTop", 0.016, hlen * 0.7, (x1 + hlen * 0.35, 0, bz + 0.028), hmat)
    elif hstyle == "full":
        box("Forend", (hlen, 0.042, 0.035), (x1 + hlen / 2, 0, bz - 0.012), hmat)
        box("UpperGuard", (hlen * 0.6, 0.03, 0.015), (x1 + hlen * 0.3, 0, bz + 0.015), hmat)
    elif hstyle == "jacket":
        cyl("Jacket", 0.02, hlen, (x1 + hlen / 2, 0, bz), hmat, verts=20)
        for i in range(int(hlen / 0.03)):
            box(f"Vent{i}", (0.014, 0.042, 0.008), (x1 + 0.015 + i * 0.03, 0, bz), m["dark"], bevel=0)
    if p.get("gastube"):
        cyl("GasTube", 0.008, blen * 0.55, (x1 + blen * 0.3, 0, bz + 0.025), m["steel"])
        box("GasBlock", (0.02, 0.03, 0.04), (x1 + blen * 0.58, 0, bz + 0.012), m["steel"])
    if p.get("fins"):
        for i in range(8):
            cyl(f"Fin{i}", br * 2.3, 0.004, (x1 + 0.02 + i * 0.012, 0, bz), m["steel"])
    if p.get("foregrip"):
        prism("Foregrip", [(x1 + 0.03, bz - 0.01), (x1 + 0.08, bz - 0.01), (x1 + 0.075, bz - 0.11), (x1 + 0.035, bz - 0.11)], 0.035, furn)
    # muzzle device
    mx = x1 + blen
    md = p.get("muzzle", "none")
    if md == "brake":
        cyl("Brake", br * 1.5, 0.05, (mx + 0.02, 0, bz), m["steel"]); mx += 0.045
    elif md == "flash":
        cyl("Flash", br * 1.35, 0.06, (mx + 0.025, 0, bz), m["dark"], verts=12)
        for i in range(4):
            box(f"FlashSlot{i}", (0.035, 0.002, br * 3), (mx + 0.035, 0, bz), m["poly"], rot=(i * 45, 0, 0), bevel=0)
        mx += 0.055
    elif md == "comp":
        cyl("Comp", br * 1.6, 0.04, (mx + 0.015, 0, bz), m["steel"]); mx += 0.035
    elif md == "bigbrake":
        box("BigBrake", (0.1, 0.07, 0.045), (mx + 0.045, 0, bz), m["steel"])
        box("BrakePort", (0.03, 0.072, 0.03), (mx + 0.04, 0, bz), m["dark"], bevel=0)
        mx += 0.095
    # grip / stock
    if p.get("wrist"):
        box("TriggerGuard", (0.07, 0.01, 0.006), (0.015, 0, -0.07), m["steel"])
        box("Trigger", (0.006, 0.006, 0.025), (0.012, 0, -0.045), m["steel"])
    else:
        grip(m, furn)
    stock = p.get("stock", "fixed")
    if stock in ("wood", "fixed"):
        smat = m["wood"] if stock == "wood" else furn
        if p.get("wrist"):
            prism("Stock", [(x0 - 0.34, -0.15), (x0 - 0.1, -0.1), (0.02, -0.06), (0.02, -0.03), (x0, top - 0.005),
                            (x0 - 0.34, top - 0.02)], 0.044, smat, bevel=0.006)
        else:
            # comb, heel, slightly curved butt, toe, and a belly that sweeps up into the grip
            L = 0.32
            belly = [(x0 - L + L * t, -0.13 + 0.095 * (t ** 1.6)) for t in (0.0, 0.2, 0.4, 0.6, 0.8, 1.0)]
            prism("Stock", belly + [(x0 + 0.005, -0.035), (x0 + 0.005, top - 0.005), (x0 - 0.08, top - 0.008),
                                    (x0 - 0.2, top - 0.016), (x0 - L, top - 0.018), (x0 - L - 0.004, (top - 0.13) / 2)],
                  0.044, smat, bevel=0.007)
            box("SlingStud", (0.01, 0.008, 0.012), (x0 - L + 0.07, 0, -0.118 + 0.007), m["steel"], bevel=0.002)
        box("ButtPad", (0.015, 0.046, top + 0.12), (x0 - 0.327, 0, (top - 0.13) / 2 - 0.005), m["rubber"])
    elif stock == "thumbhole":
        prism("StockTop", [(x0 - 0.33, top - 0.04), (x0, top - 0.035), (x0, top - 0.005), (x0 - 0.33, top - 0.01)], 0.04, furn)
        prism("StockLow", [(x0 - 0.33, -0.13), (x0 - 0.02, -0.06), (x0, -0.03), (x0 - 0.05, -0.03), (x0 - 0.33, -0.08)], 0.04, furn)
        box("StockRear", (0.03, 0.04, top + 0.13), (x0 - 0.32, 0, (top - 0.13) / 2), furn)
    elif stock == "retract":
        # buffer tube with castle nut, adjustable stock with cheek rest, latch lever and sling loop
        o = lathe("Tube", [(0.0, 0.0), (0.0145, 0.0), (0.0145, 0.2), (0.012, 0.205), (0.0, 0.205)], m["dark"], segs=20)
        o.rotation_euler = (0, math.radians(-90), 0)
        o.location = (x0, 0, top - 0.03)
        cyl("CastleNut", 0.018, 0.012, (x0 - 0.01, 0, top - 0.03), m["steel"], verts=8)
        box("EndPlate", (0.004, 0.03, 0.036), (x0 - 0.002, 0, top - 0.035), m["steel"], bevel=0.001)
        prism("Butt", [(x0 - 0.25, -0.095), (x0 - 0.2, -0.09), (x0 - 0.16, -0.05), (x0 - 0.14, top - 0.045),
                       (x0 - 0.14, top - 0.01), (x0 - 0.2, top + 0.002), (x0 - 0.25, top + 0.002)], 0.042, m["poly"], bevel=0.005)
        box("Cheek", (0.1, 0.044, 0.012), (x0 - 0.2, 0, top - 0.003), m["poly"], bevel=0.004)
        box("ButtPad", (0.012, 0.045, top + 0.1), (x0 - 0.254, 0, (top - 0.095) / 2), m["rubber"], bevel=0.003)
        box("Latch", (0.04, 0.012, 0.008), (x0 - 0.17, 0, -0.035), m["dark"], bevel=0.002)
        box("SlingLoop", (0.012, 0.046, 0.016), (x0 - 0.235, 0, -0.06), m["steel"], bevel=0.002)
    # magazine
    mag = p.get("mag", "none")
    mxp = p.get("magx", 0.08)
    mz = top - rh
    if mag == "straight":
        box("Mag", (0.035, 0.026, 0.17), (mxp, 0, mz - 0.08), m["dark"], rot=(0, -8, 0))
    elif mag in ("curved", "curved_s"):
        # one smooth banana curve (was stacked boxes), with ribs and a floor plate
        n, sl = (5, 0.042) if mag == "curved" else (4, 0.036)
        steps = 16
        total = n * sl
        x, z = mxp, mz
        rear, front, centre = [], [], []
        for i in range(steps + 1):
            ang = math.radians(8 + (i / steps) * (n - 1) * 7)
            nx, nz = math.cos(ang), math.sin(ang)  # perpendicular to the path, pointing forward/down
            rear.append((x - nx * 0.019, z - nz * 0.019 * 0.0 + 0.0))
            front.append((x + nx * 0.019, z + 0.0))
            centre.append((x, z, ang))
            x += math.sin(ang) * total / steps
            z -= math.cos(ang) * total / steps
        prism("Mag", rear + front[::-1], 0.026, m["dark"], bevel=0.002)
        for i in range(2, steps, 3):
            cx, cz, ang = centre[i]
            box(f"MagRib{i}", (0.036, 0.028, 0.004), (cx, 0, cz), m["dark"], rot=(0, -math.degrees(ang), 0), bevel=0.001)
        cx, cz, ang = centre[-1]
        box("MagFloor", (0.044, 0.03, 0.008), (cx, 0, cz), m["dark"], rot=(0, -math.degrees(ang), 0), bevel=0.002)
    elif mag == "drum":
        box("DrumNeck", (0.04, 0.03, 0.04), (mxp, 0, mz - 0.015), m["steel"])
        cyl("Drum", 0.075, 0.05, (mxp, 0, mz - 0.1), m["steel"], axis="Y", verts=32)
        cyl("DrumCap", 0.045, 0.054, (mxp, 0, mz - 0.1), m["dark"], axis="Y", verts=24)
    elif mag == "box12":
        box("Mag", (0.06, 0.03, 0.12), (mxp, 0, mz - 0.055), m["dark"], rot=(0, -6, 0))
    elif mag == "boxbelt":
        box("AmmoBox", (0.1, 0.08, 0.1), (mxp, -0.02, mz - 0.05), m["green"])
    # sights
    sight = p.get("sight", "iron")
    sz = top + 0.015
    if sight == "iron":
        box("RearSight", (0.02, 0.03, 0.02), (x0 + 0.04, 0, top + 0.008), m["dark"])
        box("FrontSightBase", (0.02, 0.02, (top + 0.02) - bz), (x1 + blen - 0.03, 0, (top + 0.02 + bz) / 2), m["steel"])
        box("FrontPost", (0.004, 0.004, 0.015), (x1 + blen - 0.03, 0, top + 0.025), m["dark"])
    elif sight == "drum":
        cyl("DrumSight", 0.012, 0.025, (x0 + 0.03, 0, top + 0.015), m["dark"], axis="Y")
        box("FrontHood", (0.02, 0.025, 0.035), (x1 + blen - 0.02, 0, bz + 0.03), m["dark"])
        sz = top + 0.02
    elif sight == "reddot":
        box("DotBase", (0.05, 0.03, 0.012), (0.05, 0, top + 0.012), m["dark"])
        cyl("DotTube", 0.017, 0.055, (0.05, 0, top + 0.04), m["poly"])
        cyl("DotLens", 0.014, 0.057, (0.05, 0, top + 0.04), m["lens"])
        sz = top + 0.04
    elif sight == "scope":
        for x in (-0.02, 0.1):
            box(f"Ring{x}", (0.015, 0.03, 0.035), (x, 0, top + 0.018), m["dark"])
        cyl("ScopeTube", 0.014, 0.26, (0.04, 0, top + 0.045), m["poly"])
        cone("ScopeFront", 0.014, 0.024, 0.07, (0.2, 0, top + 0.045), m["poly"], axis="X")
        cone("ScopeRear", 0.021, 0.014, 0.05, (-0.105, 0, top + 0.045), m["poly"], axis="X")
        cyl("ScopeLens", 0.022, 0.002, (0.236, 0, top + 0.045), m["lens"])
        cyl("Turret", 0.01, 0.02, (0.04, 0, top + 0.065), m["poly"], axis="Z")
        sz = top + 0.045
    elif sight == "carry":
        box("CarryHandle", (0.16, 0.012, 0.035), (0.02, 0, top + 0.02), m["dark"])
        box("CarrySight", (0.02, 0.025, 0.02), (-0.04, 0, top + 0.045), m["dark"])
        box("FrontSightBase", (0.02, 0.02, 0.06), (x1 + blen - 0.03, 0, bz + 0.03), m["steel"])
        sz = top + 0.05
    if p.get("carryhandle"):
        box("Handle", (0.1, 0.012, 0.012), (x1 + 0.05, 0.035, top + 0.02), m["poly"])
    if p.get("bipod"):
        for side in (-1, 1):
            box(f"Bipod{side}", (0.22, 0.008, 0.012), (x1 + blen * 0.6 - 0.11, side * 0.02, bz - 0.03), m["steel"], rot=(0, -5, 0))
    return dict(muzzle=mx, sight=sz)


def build_bolt(p, m):
    L = p["length"]
    furn = m[p.get("furn", "wood")] if p.get("furn") != "green" else m["green"]
    mx = L - 0.36
    cyl("Receiver", 0.017, 0.24, (0.04, 0, 0.025), m["steel"])
    cyl("Barrel", 0.0095, mx - 0.16, (0.16 + (mx - 0.16) / 2, 0, 0.025), m["steel"])
    cyl("BoltHandle", 0.004, 0.05, (-0.03, 0.035, 0.02), m["steel"], axis="Y")
    sph("BoltKnob", 0.009, (-0.03, 0.06, 0.015), m["steel"])
    fe = mx - 0.12 if p.get("furn") == "wood" else 0.45
    prism("Stock", [(-0.36, -0.14), (-0.1, -0.09), (0.02, -0.06), (0.06, -0.03), (fe, -0.0), (fe, 0.02),
                    (0.15, 0.02), (0.15, 0.01), (-0.08, 0.01), (-0.36, 0.0)], 0.045, furn, bevel=0.006)
    box("ButtPad", (0.015, 0.047, 0.15), (-0.367, 0, -0.07), m["rubber"])
    box("Magwell", (0.08, 0.04, 0.03), (0.06, 0, -0.03), m["steel"])
    box("TriggerGuard", (0.07, 0.01, 0.006), (0.015, 0, -0.07), m["steel"])
    box("Trigger", (0.006, 0.006, 0.025), (0.012, 0, -0.045), m["steel"])
    if p.get("sidebox"):
        box("KragBox", (0.09, 0.025, 0.045), (0.06, 0.035, -0.0), m["steel"])
    if p.get("scope"):
        for x in (-0.02, 0.12):
            box(f"Ring{x}", (0.015, 0.03, 0.04), (x, 0, 0.055), m["dark"])
        cyl("ScopeTube", 0.015, 0.3, (0.05, 0, 0.08), m["poly"])
        cone("ScopeFront", 0.015, 0.027, 0.08, (0.24, 0, 0.08), m["poly"], axis="X")
        cone("ScopeRear", 0.022, 0.015, 0.05, (-0.12, 0, 0.08), m["poly"], axis="X")
        cyl("Turret", 0.011, 0.025, (0.05, 0, 0.1), m["poly"], axis="Z")
        sight = 0.08
    else:
        box("RearSight", (0.04, 0.02, 0.02), (0.2, 0, 0.04), m["steel"])
        box("FrontSight", (0.01, 0.004, 0.02), (mx - 0.02, 0, 0.042), m["steel"])
        sight = 0.05
    if p.get("bipod"):
        for side in (-1, 1):
            box(f"Bipod{side}", (0.008, 0.01, 0.22), (0.4, side * 0.05, -0.1), m["steel"], rot=(side * 18, 10, 0))
    return dict(muzzle=mx, sight=sight)


def build_musket(p, m):
    cyl("Barrel", 0.012, 1.0, (0.55, 0, 0.025), m["steel"])
    cyl("Ramrod", 0.004, 0.85, (0.5, 0, -0.005), m["wood"])
    prism("Stock", [(-0.42, -0.15), (-0.12, -0.08), (0.02, -0.06), (0.06, -0.02), (0.95, 0.005), (0.95, 0.02),
                    (0.08, 0.02), (-0.08, 0.005), (-0.42, 0.0)], 0.048, m["lwood"], bevel=0.008)
    box("LockPlate", (0.12, 0.006, 0.03), (0.04, 0.026, 0.01), m["steel"])
    box("Frizzen", (0.012, 0.012, 0.03), (0.07, 0.03, 0.035), m["steel"], rot=(0, 15, 0))
    box("Hammer", (0.035, 0.008, 0.012), (0.01, 0.03, 0.04), m["steel"], rot=(0, -35, 0))
    for x in (0.3, 0.6, 0.85):
        cyl(f"Band{x}", 0.017, 0.012, (x, 0, 0.018), m["brass"])
    box("ButtPlate", (0.01, 0.05, 0.16), (-0.425, 0, -0.075), m["brass"])
    box("TriggerGuard", (0.09, 0.01, 0.006), (0.02, 0, -0.065), m["brass"])
    box("FrontSight", (0.008, 0.004, 0.008), (1.03, 0, 0.04), m["brass"])
    return dict(muzzle=1.05, sight=0.04)


def build_shotgun(p, m):
    if p["kind"] == "pump":
        box("Receiver", (0.22, 0.045, 0.065), (0.04, 0, 0.0), m["dark"])
        cyl("Barrel", 0.011, 0.47, (0.38, 0, 0.022), m["dark"])
        cyl("Tube", 0.01, 0.36, (0.32, 0, -0.008), m["dark"])
        cyl("Pump", 0.021, 0.16, (0.3, 0, -0.005), m["wood"], verts=16)
        for i in range(5):
            cyl(f"PumpRib{i}", 0.022, 0.006, (0.24 + i * 0.03, 0, -0.005), m["bake"], verts=16)
        prism("Stock", [(-0.38, -0.14), (-0.07, -0.035), (-0.07, 0.03), (-0.38, 0.01)], 0.045, m["wood"], bevel=0.006)
        grip(m, m["wood"])
        sph("Bead", 0.003, (0.6, 0, 0.035), m["brass"])
        return dict(muzzle=0.615, sight=0.04)
    for y in (-0.011, 0.011):
        cyl(f"Barrel{y}", 0.0115, 0.6, (0.35, y, 0.025), m["steel"])
    box("Rib", (0.6, 0.006, 0.006), (0.35, 0, 0.037), m["steel"])
    box("Action", (0.12, 0.048, 0.06), (0.0, 0, 0.005), m["chrome"])
    box("Forend", (0.2, 0.04, 0.03), (0.2, 0, 0.0), m["wood"])
    prism("Stock", [(-0.4, -0.15), (-0.12, -0.08), (0.0, -0.06), (-0.06, -0.025), (-0.06, 0.03), (-0.4, 0.01)], 0.045, m["wood"], bevel=0.006)
    box("TriggerGuard", (0.06, 0.01, 0.006), (0.0, 0, -0.04), m["steel"])
    sph("Bead", 0.003, (0.64, 0, 0.042), m["brass"])
    return dict(muzzle=0.65, sight=0.045)


def build_launcher(p, m):
    cyl("Tube", 0.046, 1.07, (0.03, 0, 0.07), m["green"], verts=32)
    cone("Venturi", 0.046, 0.06, 0.12, (-0.55, 0, 0.07), m["dark"], axis="X", verts=32)
    cyl("MuzzleRing", 0.05, 0.03, (0.56, 0, 0.07), m["dark"], verts=32)
    prism("Grip", [(-0.03, 0.02), (0.02, 0.02), (0.0, -0.1), (-0.05, -0.095)], 0.035, m["poly"])
    prism("FrontGrip", [(0.22, 0.02), (0.27, 0.02), (0.26, -0.09), (0.22, -0.09)], 0.035, m["poly"])
    box("ShoulderPad", (0.2, 0.06, 0.04), (-0.2, 0, 0.005), m["rubber"])
    box("Optic", (0.12, 0.035, 0.05), (0.05, -0.06, 0.12), m["poly"])
    cyl("OpticLens", 0.012, 0.125, (0.05, -0.06, 0.125), m["lens"])
    box("Trigger", (0.006, 0.006, 0.02), (0.02, 0, 0.0), m["steel"])
    for x in (-0.3, 0.35):
        cyl(f"Band{x}", 0.048, 0.02, (x, 0, 0.07), m["dark"], verts=32)
    return dict(muzzle=0.57, sight=0.14)


def build_m79(p, m):
    cyl("Barrel", 0.024, 0.36, (0.24, 0, 0.035), m["steel"])
    box("Receiver", (0.12, 0.05, 0.06), (0.0, 0, 0.01), m["steel"])
    box("Forend", (0.18, 0.045, 0.03), (0.17, 0, 0.0), m["lwood"])
    prism("Stock", [(-0.36, -0.15), (-0.08, -0.07), (0.0, -0.06), (-0.06, -0.02), (-0.06, 0.035), (-0.36, 0.01)], 0.05, m["lwood"], bevel=0.006)
    box("ButtPad", (0.02, 0.05, 0.16), (-0.37, 0, -0.07), m["rubber"])
    box("LeafSight", (0.004, 0.03, 0.05), (0.1, 0, 0.08), m["dark"])
    box("TriggerGuard", (0.06, 0.01, 0.006), (0.0, 0, -0.035), m["steel"])
    return dict(muzzle=0.42, sight=0.075)


def build_crossbow(p, m):
    prism("Stock", [(-0.38, -0.13), (-0.1, -0.06), (0.0, -0.1), (0.04, -0.03), (0.42, -0.0), (0.42, 0.03), (-0.38, 0.015)],
          0.045, m["wood"], bevel=0.006)
    box("Rail", (0.45, 0.012, 0.01), (0.2, 0, 0.035), m["steel"])
    for side in (-1, 1):
        box(f"Limb{side}", (0.03, 0.27, 0.02), (0.38, side * 0.14, 0.025), m["poly"], rot=(0, 0, side * 15))
        box(f"String{side}", (0.003, 0.3, 0.003), (0.27, side * 0.13, 0.035), m["fabric"], rot=(0, 0, side * -28), bevel=0)
    cyl("Bolt", 0.004, 0.36, (0.25, 0, 0.045), m["steel"])
    cone("Tip", 0.007, 0.0, 0.03, (0.44, 0, 0.045), m["steel"], axis="X")
    box("Stirrup", (0.01, 0.1, 0.06), (0.43, 0, 0.0), m["steel"])
    cyl("Scope", 0.012, 0.16, (0.05, 0, 0.075), m["poly"])
    return dict(muzzle=0.45, sight=0.075)


# ------------------------------------------------------------------ melee
def build_knife(p, m):
    b = p["blade"]
    hgt = 0.05 if p.get("wide") else 0.032
    prism("Blade", [(0.02, -hgt * 0.5), (0.02 + b * 0.8, -hgt * 0.5), (0.02 + b, hgt * 0.15), (0.02 + b * 0.85, hgt * 0.5),
                    (0.02, hgt * 0.5)], 0.004, m["blade"], bevel=0.001)
    box("Guard", (0.01, 0.025, hgt + 0.03), (0.015, 0, 0), m["dark"])
    cyl("Handle", 0.015, 0.12, (-0.05, 0, 0), m["rubber"] if not p.get("wide") else m["wood"], verts=12)
    for i in range(5):
        cyl(f"Ring{i}", 0.016, 0.004, (-0.1 + i * 0.025, 0, 0), m["dark"], verts=12)
    sph("Pommel", 0.017, (-0.115, 0, 0), m["dark"])
    return dict(muzzle=0.02 + b, sight=0.0)


def build_axe(p, m):
    h = p["handle"]
    cyl("Handle", 0.017, h, (h / 2 - 0.15, 0, 0), m["lwood"], verts=12)
    end = h - 0.15
    if p["head"] == "viking":
        prism("Head", [(end - 0.04, 0.0), (end + 0.02, 0.0), (end + 0.04, 0.17), (end - 0.08, 0.15), (end - 0.04, 0.05)],
              0.012, m["blade"], bevel=0.002)
        box("Socket", (0.06, 0.036, 0.04), (end - 0.02, 0, 0.0), m["steel"])
    else:
        box("Head", (0.08, 0.07, 0.18), (end - 0.02, 0, 0.0), m["dark"])
    cyl("Wrap", 0.019, 0.15, (-0.07, 0, 0), m["fabric"], verts=12)
    return dict(muzzle=end, sight=0.0)


def build_bat(p, m):
    cone("Bat", 0.017, 0.034, 0.8, (0.3, 0, 0), m["lwood"], axis="X", verts=24)
    sph("End", 0.034, (0.7, 0, 0), m["lwood"], scale=(0.5, 1, 1))
    cyl("Knob", 0.024, 0.02, (-0.1, 0, 0), m["lwood"])
    cyl("Tape", 0.0185, 0.15, (-0.02, 0, 0), m["fabric"])
    return dict(muzzle=0.7, sight=0.0)


def build_crowbar(p, m):
    cyl("Bar", 0.012, 0.65, (0.25, 0, 0), m["red"], verts=6)
    rotated(cyl("Hook", 0.012, 0.12, (0.6, 0, 0.04), m["red"], verts=6), (0, -40, 0))
    rotated(box("Claw", (0.05, 0.03, 0.008), (0.63, 0, 0.09), m["steel"]), (0, -80, 0))
    box("Chisel", (0.06, 0.025, 0.008), (-0.1, 0, -0.01), m["steel"], rot=(0, 10, 0))
    return dict(muzzle=0.62, sight=0.0)


def build_pan(p, m):
    cyl("Handle", 0.012, 0.22, (0.0, 0, 0), m["poly"], verts=12)
    cyl("Pan", 0.13, 0.04, (0.24, 0, 0.0), m["dark"], axis="Y", verts=32)
    cyl("Inside", 0.12, 0.042, (0.24, 0, 0.0), m["steel"], axis="Y", verts=32)
    return dict(muzzle=0.37, sight=0.0)


# ------------------------------------------------------------------ armor
def build_vest(p, m):
    c = col(p["color"], 0, 0.9)
    box("Front", (0.06, 0.36, 0.46), (0.13, 0, 0), c, bevel=0.01)
    box("Back", (0.06, 0.36, 0.48), (-0.13, 0, 0), c, bevel=0.01)
    for s in (-1, 1):
        box(f"Side{s}", (0.22, 0.04, 0.2), (0, s * 0.18, -0.12), c, bevel=0.01)
        box(f"Strap{s}", (0.3, 0.07, 0.025), (0, s * 0.11, 0.24), c, bevel=0.005)
    for i in range(p.get("plates", 0)):
        box(f"Plate{i}", (0.03, 0.29, 0.34), (0.17 if i == 0 else -0.17, 0, 0.03), c, bevel=0.012)
    n = p.get("pouches", 0)
    for i in range(n):
        y = -0.15 + (0.3 / max(1, n - 1)) * i if n > 1 else 0
        box(f"Pouch{i}", (0.05, 0.065, 0.11), (0.19, y, -0.14), c, bevel=0.006)
        box(f"PouchFlap{i}", (0.052, 0.067, 0.02), (0.19, y, -0.085), col((p["color"][0] * 0.7, p["color"][1] * 0.7, p["color"][2] * 0.7)))
    if p.get("collar"):
        box("Collar", (0.3, 0.3, 0.05), (0, 0, 0.26), c, bevel=0.02)
    return None


def build_exo(p, m):
    metal = col((0.15, 0.16, 0.18), 0.8, 0.35)
    glow = col((0.1, 0.8, 1.0), 0, 0.2)
    cyl("Spine", 0.025, 0.6, (-0.15, 0, 0.0), metal, axis="Z")
    box("ShoulderBar", (0.06, 0.5, 0.04), (-0.1, 0, 0.28), metal)
    box("ChestPlate", (0.04, 0.3, 0.3), (0.14, 0, 0.08), metal, bevel=0.015)
    box("Battery", (0.08, 0.18, 0.22), (-0.22, 0, 0.05), metal, bevel=0.01)
    box("Glow", (0.005, 0.12, 0.02), (-0.262, 0, 0.1), glow)
    box("ChestGlow", (0.005, 0.2, 0.012), (0.162, 0, 0.12), glow)
    for s in (-1, 1):
        cyl(f"Thigh{s}", 0.02, 0.4, (0, s * 0.13, -0.5), metal, axis="Z")
        cyl(f"Shin{s}", 0.02, 0.42, (0, s * 0.13, -0.95), metal, axis="Z")
        sph(f"Knee{s}", 0.035, (0, s * 0.13, -0.72), metal)
        box(f"Hip{s}", (0.2, 0.04, 0.06), (-0.05, s * 0.13, -0.28), metal)
        cyl(f"Arm{s}", 0.018, 0.3, (-0.05, s * 0.25, 0.12), metal, axis="Z")
    return None


def build_helmet(p, m):
    k = p["kind"]
    if k == "bike":
        c = col((0.9, 0.3, 0.05), 0, 0.3)
        hemi("Shell", 0.13, (0, 0, 0), c, scale=(1.25, 1.0, 0.9))
        for i in range(4):
            box(f"Vent{i}", (0.12, 0.012, 0.03), (0.0, -0.045 + i * 0.03, 0.11), m["poly"], bevel=0)
        return None
    c = m["green"] if k == "combat" else col((0.08, 0.09, 0.08), 0, 0.8)
    hemi("Shell", 0.135, (0, 0, 0), c, scale=(1.12, 1.0, 1.0))
    cyl("Rim", 0.14, 0.02, (0, 0, 0.0), c, axis="Z", verts=32)
    if k == "visor":
        hemi("Visor", 0.14, (0.012, 0, -0.005), col((0.05, 0.08, 0.1), 0.6, 0.05), scale=(1.12, 0.95, 0.55))
        box("NVGMount", (0.03, 0.05, 0.04), (0.15, 0, 0.08), m["poly"])
        for s in (-1, 1):
            box(f"Rail{s}", (0.14, 0.012, 0.02), (0.0, s * 0.135, 0.04), m["poly"])
    return None


# ------------------------------------------------------------------ consumables
def bottle_with_cap(m, color, cap, r=0.028, h=0.2, glassy=True):
    body = col(color, 0, 0.1 if glassy else 0.5)
    cyl("Body", r, h * 0.7, (0, 0, h * 0.35), body, axis="Z")
    cone("Shoulder", r, r * 0.45, h * 0.18, (0, 0, h * 0.79), body)
    cyl("Neck", r * 0.42, h * 0.08, (0, 0, h * 0.92), body, axis="Z")
    cyl("Cap", r * 0.47, h * 0.05, (0, 0, h * 0.98), col(cap), axis="Z")


ITEM_BUILDERS = {}


def item(name):
    def deco(f):
        ITEM_BUILDERS[name] = f
        return f
    return deco


@item("pills")
def _(p, m):
    body = col(p["color"], 0, 0.35)
    lathe("Bottle", [(0.0, 0.0), (0.019, 0.0), (0.0215, 0.003), (0.0215, 0.058), (0.019, 0.064), (0.012, 0.066), (0.012, 0.072)], body, segs=36)
    lathe("Cap", [(0.0, 0.069), (0.0235, 0.069), (0.024, 0.071), (0.024, 0.088), (0.022, 0.0905), (0.0, 0.0905)], col(p["cap"], 0, 0.6), segs=36)
    for k in range(24):
        a = 2 * math.pi * k / 24
        box(f"Rib{k}", (0.0016, 0.0016, 0.016), (math.cos(a) * 0.0242, math.sin(a) * 0.0242, 0.0795), col(p["cap"], 0, 0.6), rot=(0, 0, math.degrees(a)), bevel=0)
    lathe("Label", [(0.0218, 0.012), (0.0218, 0.048)], m["white"], segs=36)
    lathe("LabelBand", [(0.0219, 0.03), (0.0219, 0.037)], col(p["cap"], 0, 0.6), segs=36)


@item("jar")
def _(p, m):
    lathe("Glass", [(0.0, 0.0), (0.031, 0.0), (0.035, 0.004), (0.035, 0.07), (0.031, 0.076), (0.029, 0.078), (0.029, 0.084)], m["glass"], segs=40)
    lathe("Content", [(0.0, 0.002), (0.033, 0.002), (0.0352, 0.006), (0.0352, 0.052), (0.0, 0.056)], col(p["color"], 0, 0.9), segs=40)
    lathe("Lid", [(0.0, 0.078), (0.032, 0.078), (0.0325, 0.08), (0.0325, 0.092), (0.03, 0.094), (0.0, 0.094)], m["dark"], segs=40)
    lathe("Label", [(0.0354, 0.058), (0.0354, 0.07)], m["paper"], segs=40)


@item("tin")
def _(p, m):
    c = col(p["color"], 0.5, 0.35)
    lathe("Tin", [(0.0, 0.0), (0.032, 0.0), (0.0335, 0.0015), (0.0335, 0.016), (0.0, 0.016)], c, segs=40)
    lathe("Lid", [(0.0, 0.0135), (0.0345, 0.0135), (0.035, 0.015), (0.035, 0.022), (0.034, 0.0235), (0.0, 0.0235)], m["white"], segs=40)
    lathe("LidPrint", [(0.0, 0.0237), (0.022, 0.0237), (0.0, 0.0237)], c, segs=40)
    lathe("Ring", [(0.022, 0.0236), (0.024, 0.0238), (0.026, 0.0236)], col(p["color"], 0.6, 0.3), segs=40)


@item("syringe")
def _(p, m):
    def lx(name, prof, mat, segs=24):
        o = lathe(name, prof, mat, segs=segs)
        o.rotation_euler = (0, math.radians(90), 0)
        return o
    lx("Barrel", [(0.0075, -0.045), (0.0085, -0.044), (0.0085, 0.04), (0.006, 0.046), (0.0035, 0.048)], m["glass"])
    lx("Liquid", [(0.0, -0.02), (0.0078, -0.02), (0.0078, 0.039), (0.005, 0.044), (0.0, 0.044)], col(p["color"], 0, 0.15))
    lx("Stopper", [(0.0, -0.026), (0.0078, -0.026), (0.0079, -0.023), (0.0078, -0.02), (0.0, -0.02)], m["rubber"])
    lx("Plunger", [(0.0025, -0.095), (0.0025, -0.026)], m["white"], 8)
    lx("Thumb", [(0.0, -0.1), (0.011, -0.1), (0.011, -0.096), (0.0025, -0.095)], m["white"])
    box("Flange", (0.004, 0.034, 0.011), (-0.046, 0, 0), m["white"], bevel=0.0015)
    for i in range(8):
        box(f"Tick{i}", (0.0006, 0.0015, 0.003), (-0.03 + i * 0.008, 0, 0.0086), m["dark"], bevel=0)
    lx("Hub", [(0.0035, 0.047), (0.003, 0.058), (0.0015, 0.06)], col((0.2, 0.5, 0.9), 0, 0.4), 12)
    lx("Needle", [(0.00045, 0.058), (0.00045, 0.092), (0.0, 0.095)], m["chrome"], 6)


@item("blister")
def _(p, m):
    box("Foil", (0.095, 0.052, 0.0012), (0, 0, 0.0006), m["chrome"], bevel=0.0004)
    box("Print", (0.093, 0.05, 0.0002), (0, 0, -0.0001), col((0.95, 0.95, 0.95)), bevel=0)
    for i in range(5):
        for j in range(2):
            x, y = -0.034 + i * 0.017, -0.012 + j * 0.024
            sph(f"Dome{i}{j}", 0.0075, (x, y, 0.0012), m["glass"], scale=(1.3, 1, 0.65))
            if not (i == 4 and j == 0):
                sph(f"Pill{i}{j}", 0.0058, (x, y, 0.0026), col(p["color"], 0, 0.45), scale=(1.4, 1, 0.55))
    box("Torn", (0.012, 0.012, 0.0003), (0.034, -0.012, 0.0014), col((0.7, 0.7, 0.72), 0.8, 0.4), rot=(0, 0, 18), bevel=0)


@item("dropper")
def _(p, m):
    lathe("Bottle", [(0.0, 0.0), (0.0145, 0.0), (0.016, 0.002), (0.016, 0.042), (0.012, 0.05), (0.008, 0.053), (0.008, 0.056)], col((0.32, 0.15, 0.04), 0, 0.08), segs=32)
    lathe("Label", [(0.0162, 0.01), (0.0162, 0.034)], m["paper"], segs=32)
    lathe("LabelStripe", [(0.0163, 0.02), (0.0163, 0.025)], col(p["color"], 0, 0.5), segs=32)
    lathe("Collar", [(0.0, 0.054), (0.0095, 0.054), (0.0095, 0.066), (0.006, 0.068), (0.0, 0.068)], m["dark"], segs=24)
    lathe("Bulb", [(0.0, 0.067), (0.006, 0.067), (0.0075, 0.072), (0.0075, 0.082), (0.0055, 0.088), (0.0, 0.09)], col(p["color"], 0, 0.4), segs=24)


@item("can")
def _(p, m):
    r, h = p.get("r", 0.033), p.get("h", 0.12)
    c = col(p["color"], 0.6, 0.3)
    lathe("Can", [(0.0, 0.0), (r * 0.82, 0.0), (r * 0.86, 0.002), (r, 0.012), (r, h - 0.012), (r * 0.82, h - 0.002), (r * 0.84, h), (r * 0.86, h - 0.0005)], c, segs=40)
    lathe("Bottom", [(0.0, 0.0005), (r * 0.82, 0.0005), (r * 0.86, 0.002), (r, 0.012)], m["chrome"], segs=40)
    lathe("Top", [(0.0, h - 0.005), (r * 0.8, h - 0.005), (r * 0.82, h - 0.002), (r * 0.84, h), (r * 0.86, h - 0.0005), (r, h - 0.012)], m["chrome"], segs=40)
    if p.get("band"):
        lathe("Band", [(r + 0.0004, h * 0.3), (r + 0.0004, h * 0.7)], col(p["band"], 0.3, 0.4), segs=40)
    if h > 0.1:
        box("Tab", (0.018, 0.009, 0.001), (r * 0.35, 0, h - 0.004), m["chrome"], bevel=0.0005)


@item("vial")
def _(p, m):
    lathe("Glass", [(0.0, 0.0), (0.009, 0.0), (0.0115, 0.003), (0.0115, 0.056), (0.008, 0.062), (0.0065, 0.064), (0.0075, 0.066)], m["glass"], segs=32)
    lathe("Liquid", [(0.0, 0.0015), (0.0105, 0.003), (0.0117, 0.006), (0.0117, 0.046), (0.0, 0.047)], col(p["color"], 0, 0.05), segs=32)
    lathe("Crimp", [(0.0, 0.064), (0.0082, 0.064), (0.0085, 0.066), (0.0085, 0.073), (0.006, 0.0745), (0.0, 0.0745)], m["chrome"], segs=32)
    lathe("Septum", [(0.0, 0.0748), (0.0045, 0.0748), (0.0, 0.0752)], col(p["color"], 0, 0.5), segs=20)
    lathe("Label", [(0.0118, 0.015), (0.0118, 0.035)], m["white"], segs=32)


@item("mushroom")
def _(p, m):
    for i, (x, y, s) in enumerate([(0, 0, 1.0), (0.03, 0.02, 0.7), (-0.025, 0.025, 0.55)]):
        cyl(f"Stem{i}", 0.008 * s, 0.05 * s, (x, y, 0.025 * s), m["white"], axis="Z")
        hemi(f"Cap{i}", 0.028 * s, (x, y, 0.048 * s), m["red"], scale=(1, 1, 0.7))
        for k in range(4):
            a = k * 1.57 + i
            sph(f"Dot{i}{k}", 0.004 * s, (x + math.cos(a) * 0.017 * s, y + math.sin(a) * 0.017 * s, 0.06 * s), m["white"])


@item("patch")
def _(p, m):
    c = col(p["color"], 0, 0.7)
    box("PatchA", (0.052, 0.036, 0.0012), (0, 0, 0.0006), c, bevel=0.0005)
    box("PatchB", (0.036, 0.052, 0.0012), (0, 0, 0.0006), c, bevel=0.0005)
    for sx in (-1, 1):
        for sy in (-1, 1):
            sph(f"Corner{sx}{sy}", 0.008, (sx * 0.018, sy * 0.018, 0.0005), c, scale=(1, 1, 0.08), segs=16)
    box("Center", (0.032, 0.032, 0.0016), (0, 0, 0.0008), col((0.93, 0.9, 0.85), 0, 0.3), bevel=0.0005)
    box("Liner", (0.06, 0.03, 0.0005), (0.012, 0.022, -0.0002), m["white"], rot=(0, 0, 8), bevel=0)


@item("canister")
def _(p, m):
    c = col(p["color"], 0.7, 0.3)
    lathe("Can", [(0.0, 0.0), (0.02, 0.0), (0.025, 0.004), (0.025, 0.11), (0.022, 0.125), (0.014, 0.135), (0.008, 0.138), (0.008, 0.142)], c, segs=36)
    lathe("Band", [(0.0253, 0.04), (0.0253, 0.08)], col((0.15, 0.3, 0.7), 0.2, 0.4), segs=36)
    lathe("Valve", [(0.0, 0.141), (0.0085, 0.141), (0.0085, 0.152), (0.005, 0.156), (0.005, 0.166), (0.0, 0.166)], m["chrome"], segs=20)
    box("Handle", (0.04, 0.008, 0.006), (0.018, 0, 0.162), m["poly"], bevel=0.002)
    box("Nozzle", (0.008, 0.008, 0.02), (-0.008, 0, 0.17), m["poly"], bevel=0.002)


@item("plaster")
def _(p, m):
    skin = col((0.86, 0.68, 0.52), 0, 0.7)
    box("Strip", (0.076, 0.022, 0.0015), (0, 0, 0.00075), skin, bevel=0.0007)
    for sx in (-1, 1):
        sph(f"End{sx}", 0.011, (sx * 0.038, 0, 0.0006), skin, scale=(1, 1, 0.07), segs=24)
    box("Pad", (0.026, 0.016, 0.0022), (0, 0, 0.0012), m["white"], bevel=0.0006)
    for i in range(4):
        for j in range(2):
            for sx in (-1, 1):
                cyl(f"Hole{i}{j}{sx}", 0.0006, 0.0002, (sx * (0.022 + i * 0.006), -0.004 + j * 0.008, 0.0016), col((0.76, 0.6, 0.45)), axis="Z", verts=6)


@item("bandage")
def _(p, m):
    o = lathe("Roll", [(0.008, -0.035), (0.03, -0.035), (0.031, -0.032), (0.031, 0.032), (0.03, 0.035), (0.008, 0.035)], m["white"], segs=32)
    o.rotation_euler = (math.radians(90), 0, 0)
    o.location = (0, 0, 0.031)
    o = lathe("Core", [(0.0075, -0.0352), (0.0075, 0.0352)], m["paper"], segs=20)
    o.rotation_euler = (math.radians(90), 0, 0)
    o.location = (0, 0, 0.031)
    prism("Tail", [(0.0, 0.0), (0.09, 0.0), (0.09, 0.0018), (0.0, 0.0018)], 0.068, m["white"], bevel=0)
    box("Clip", (0.012, 0.02, 0.003), (0.085, 0, 0.0025), m["chrome"], bevel=0.0005)


@item("medkit")
def _(p, m):
    box("Box", (0.3, 0.1, 0.2), (0, 0, 0.1), m["red"], bevel=0.01)
    box("CrossV", (0.04, 0.102, 0.12), (0, 0, 0.1), m["white"], bevel=0)
    box("CrossH", (0.12, 0.102, 0.04), (0, 0, 0.1), m["white"], bevel=0)
    box("Handle", (0.12, 0.02, 0.02), (0, 0, 0.215), m["poly"])


@item("surgery")
def _(p, m):
    box("Case", (0.4, 0.14, 0.26), (0, 0, 0.13), m["green"], bevel=0.015)
    box("CrossV", (0.05, 0.142, 0.15), (0, 0, 0.13), m["white"], bevel=0)
    box("CrossH", (0.15, 0.142, 0.05), (0, 0, 0.13), m["white"], bevel=0)
    for x in (-0.14, 0.14):
        box(f"Latch{x}", (0.03, 0.15, 0.02), (x, 0, 0.24), m["chrome"])
    box("Handle", (0.14, 0.025, 0.025), (0, 0, 0.275), m["poly"])


@item("cup")
def _(p, m):
    lathe("Cup", [(0.0, 0.0), (0.028, 0.0), (0.03, 0.003), (0.04, 0.115), (0.042, 0.118)], m["white"], segs=40)
    lathe("Sleeve", [(0.0335, 0.035), (0.0385, 0.095)], col((0.45, 0.3, 0.18), 0, 0.9), segs=40)
    lathe("Lid", [(0.0, 0.117), (0.0435, 0.117), (0.044, 0.122), (0.04, 0.126), (0.036, 0.133), (0.0, 0.133)], m["poly"], segs=40)
    box("Sip", (0.012, 0.004, 0.004), (0.027, 0, 0.134), m["poly"], bevel=0.001)


@item("bottle")
def _(p, m):
    body = col(p["color"], 0, 0.1)
    prof = [(0.0, 0.0), (0.024, 0.0), (0.028, 0.004), (0.028, 0.03)]
    for i in range(5):  # grip rings
        z = 0.035 + i * 0.012
        prof += [(0.028, z), (0.0255, z + 0.004), (0.028, z + 0.008)]
    prof += [(0.028, 0.14), (0.024, 0.16), (0.013, 0.178), (0.0115, 0.186), (0.0125, 0.188), (0.0115, 0.19)]
    lathe("Body", prof, body, segs=36)
    lathe("Label", [(0.0283, 0.095), (0.0283, 0.135)], m["white"], segs=36)
    lathe("Cap", [(0.0, 0.189), (0.0128, 0.189), (0.0128, 0.204), (0.011, 0.206), (0.0, 0.206)], col(p["cap"], 0, 0.5), segs=36)


@item("carton")
def _(p, m):
    box("Carton", (0.07, 0.07, 0.19), (0, 0, 0.095), col(p["color"]), bevel=0.002)
    box("Band", (0.072, 0.072, 0.05), (0, 0, 0.1), col(p["band"]), bevel=0)
    prism("Gable", [(-0.035, 0.19), (0.035, 0.19), (0.0, 0.23)], 0.07, col(p["color"]))
    cyl("Spout", 0.01, 0.015, (0.015, 0, 0.215), m["poly"], axis="Z")


@item("baguette")
def _(p, m):
    c = col((0.78, 0.5, 0.2), 0, 0.8)
    n = 15
    path = [((i / (n - 1) - 0.5) * 0.52, 0, 0.026 + 0.004 * math.sin(i / (n - 1) * math.pi)) for i in range(n)]
    rad = [0.013 + 0.02 * math.sin(math.pi * i / (n - 1)) ** 0.35 for i in range(n)]
    tube("Bread", path, rad, c, segs=18, squash=(1.0, 0.85))
    for i in range(5):
        box(f"Cut{i}", (0.055, 0.009, 0.006), (-0.17 + i * 0.085, 0, 0.051), col((0.93, 0.82, 0.6), 0, 0.9), rot=(0, 0, 25), bevel=0.002)


@item("lunchbox")
def _(p, m):
    # Matpakke: brødskiver i matpapir, opprullet papir på toppen, skive med brunost.
    paper = col((0.93, 0.92, 0.88), 0, 0.9)
    box("Paper", (0.15, 0.1, 0.045), (0, 0, 0.0225), paper, bevel=0.01)
    tube("Fold", [(-0.075, 0, 0.046), (0.075, 0, 0.046)], [0.006, 0.006], paper, segs=10)
    for k in range(3):
        box(f"Crease{k}", (0.002, 0.101, 0.046), (-0.05 + k * 0.05, 0, 0.0225), col((0.85, 0.84, 0.8), 0, 0.9), bevel=0)
    box("Bread", (0.1, 0.09, 0.012), (0.03, -0.065, 0.006), col((0.72, 0.5, 0.28), 0, 0.85), rot=(0, 0, 12), bevel=0.004)
    box("Crumb", (0.09, 0.08, 0.0122), (0.03, -0.065, 0.0061), col((0.85, 0.72, 0.5), 0, 0.9), rot=(0, 0, 12), bevel=0.003)
    box("Brunost", (0.07, 0.06, 0.003), (0.03, -0.065, 0.0135), col((0.66, 0.36, 0.12), 0, 0.4), rot=(0, 0, 18), bevel=0.0008)


@item("pizzabox")
def _(p, m):
    # Grandiosa: frozen pizza in its plastic-wrapped carton.
    box("Box", (0.3, 0.3, 0.03), (0, 0, 0.015), col((0.92, 0.9, 0.86), 0, 0.6), bevel=0.003)
    box("Front", (0.29, 0.29, 0.002), (0, 0, 0.0305), col((0.8, 0.08, 0.06), 0, 0.5), bevel=0)
    cyl("PizzaPic", 0.11, 0.002, (0.02, -0.01, 0.0315), col((0.9, 0.55, 0.15), 0, 0.6), axis="Z", verts=40)
    cyl("Crust", 0.112, 0.0018, (0.02, -0.01, 0.0312), col((0.85, 0.62, 0.3), 0, 0.7), axis="Z", verts=40)
    for i in range(9):
        a = i * 2.39
        r = 0.03 + (i % 3) * 0.025
        cyl(f"Pep{i}", 0.011, 0.0012, (0.02 + math.cos(a) * r, -0.01 + math.sin(a) * r, 0.0328), col((0.65, 0.12, 0.06), 0, 0.5), axis="Z", verts=16)
    box("Logo", (0.22, 0.05, 0.0015), (0, 0.11, 0.0318), col((0.98, 0.85, 0.1), 0, 0.5), bevel=0)
    box("LogoText", (0.18, 0.025, 0.0016), (0, 0.11, 0.0322), col((0.15, 0.25, 0.6), 0, 0.5), bevel=0)


@item("kebab")
def _(p, m):
    # Rolled durum standing up: foil wrapped bottom half, open top with meat, salad, tomato, sauce.
    pita = col((0.88, 0.72, 0.48), 0, 0.85)
    lathe("Pita", [(0.0, 0.0), (0.03, 0.0), (0.036, 0.02), (0.038, 0.17), (0.036, 0.18)], pita, segs=28)
    lathe("Foil", [(0.0, -0.001), (0.031, -0.001), (0.037, 0.02), (0.0395, 0.085), (0.042, 0.095)], m["chrome"], segs=14)
    for k in range(7):
        a = k * 2 * math.pi / 7
        sph(f"Meat{k}", 0.013, (math.cos(a) * 0.017, math.sin(a) * 0.017, 0.178 + (k % 3) * 0.003), col((0.45, 0.24, 0.1), 0, 0.7), scale=(1.4, 0.8, 0.6))
    for k in range(5):
        a = k * 2 * math.pi / 5 + 0.4
        box(f"Lettuce{k}", (0.022, 0.012, 0.003), (math.cos(a) * 0.02, math.sin(a) * 0.02, 0.19), col((0.35, 0.7, 0.2), 0, 0.6), rot=(25, 0, math.degrees(a)), bevel=0)
    for k in range(3):
        a = k * 2.1
        cyl(f"Tomato{k}", 0.009, 0.004, (math.cos(a) * 0.014, math.sin(a) * 0.014, 0.193), col((0.8, 0.1, 0.06), 0, 0.3), axis="Z", verts=14)
    sph("Sauce", 0.016, (0.004, -0.004, 0.197), col((0.95, 0.93, 0.85), 0, 0.3), scale=(1.6, 1.2, 0.35))


@item("hotdog")
def _(p, m):
    # Hot dog in lompe (potato flatbread) wrapped around the sausage, ketchup and mustard zigzag.
    lompe = col((0.92, 0.85, 0.66), 0, 0.85)
    o = lathe("Lompe", [(0.0, -0.08), (0.02, -0.08), (0.027, -0.072), (0.028, 0.0), (0.027, 0.072), (0.02, 0.08), (0.0, 0.08)], lompe, segs=24, scale=(0.55, 1.25, 1))
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0, 0, 0.016)
    path = [(x, 0, 0.027 + 0.004 * (1 - (x / 0.095) ** 2)) for x in [i * 0.019 - 0.095 for i in range(11)]]
    rad = [0.0085, 0.0115] + [0.0125] * 7 + [0.0115, 0.0085]
    tube("Sausage", path, rad, col((0.62, 0.26, 0.14), 0, 0.35), segs=18)
    zz = [(-0.07 + i * 0.012, (0.006 if i % 2 else -0.006), 0.0405) for i in range(12)]
    tube("Ketchup", zz, [0.0022] * len(zz), col((0.75, 0.04, 0.03), 0, 0.2), segs=8)
    zz2 = [(-0.064 + i * 0.012, (-0.005 if i % 2 else 0.005), 0.0415) for i in range(11)]
    tube("Mustard", zz2, [0.0018] * len(zz2), col((0.9, 0.7, 0.05), 0, 0.3), segs=8)


@item("taco")
def _(p, m):
    n = 15
    pts = [(math.cos(a) * 0.07, 0.072 - math.sin(a) * 0.07) for a in [i * math.pi / (n - 1) for i in range(n)]]
    inner = [(x * 0.92, 0.072 - (0.072 - z) * 0.92) for x, z in reversed(pts)]
    prism("Shell", pts + inner, 0.085, col((0.93, 0.72, 0.3), 0, 0.75), bevel=0.001)
    for i in range(7):
        x = -0.045 + i * 0.015
        sph(f"Meat{i}", 0.013, (x, 0.012 * ((i % 2) * 2 - 1) * 0.5, 0.045 + 0.006 * math.cos(i)), col((0.42, 0.22, 0.1), 0, 0.8), scale=(1, 1.4, 0.9))
    for i in range(9):
        x = -0.055 + i * 0.0135
        box(f"Lettuce{i}", (0.02, 0.07, 0.004), (x, 0, 0.062 + 0.004 * math.sin(i * 1.7)), col((0.35, 0.72, 0.22), 0, 0.6), rot=(8 * math.sin(i), 0, 20 * math.cos(i)), bevel=0.001)
    for i in range(5):
        cyl(f"Tomato{i}", 0.006, 0.006, (-0.04 + i * 0.02, 0.015 * math.sin(i * 2), 0.069), col((0.85, 0.1, 0.06), 0, 0.3), axis="Z", verts=10)
    for i in range(14):
        box(f"Cheese{i}", (0.018, 0.0025, 0.0025), (-0.05 + (i * 0.0077), 0.02 * math.sin(i * 2.3), 0.072), col((1.0, 0.75, 0.15), 0, 0.5), rot=(0, 0, i * 47), bevel=0)


@item("bun")
def _(p, m):
    prof = []
    for i in range(13):
        t = i / 12
        a = math.pi / 2 * t
        prof.append((0.05 * math.cos(a) * (1 - 0.06 * t), 0.004 + 0.042 * math.sin(a) ** 0.8))
    prof = [(0.0, 0.0), (0.044, 0.0)] + prof[:-1] + [(0.0, 0.046)]
    lathe("Bun", prof, col(p["color"], 0, 0.55), segs=36)
    if p.get("top"):
        lathe("Glaze", [(0.0, 0.0455), (0.03, 0.044), (0.033, 0.041), (0.031, 0.0415), (0.0, 0.0465)], col((0.97, 0.96, 0.92), 0, 0.3), segs=36)
        sph("Cream", 0.017, (0, 0, 0.046), col((1.0, 0.82, 0.3), 0, 0.3), scale=(1, 1, 0.55))
        for i in range(60):
            a = i * 2.39996
            r = 0.026 * math.sqrt((i + 0.5) / 60) + 0.008
            box(f"Coco{i}", (0.003, 0.0012, 0.0012), (math.cos(a) * r, math.sin(a) * r, 0.045 - (r / 0.034) ** 2 * 0.004), m["white"], rot=(0, 0, i * 37), bevel=0)
    if p.get("swirl"):
        path = [(math.cos(t) * (0.006 + t * 0.0028), math.sin(t) * (0.006 + t * 0.0028), 0.043 - t * 0.0006) for t in [i * 0.25 for i in range(56)]]
        tube("Swirl", path, [0.0035] * len(path), col((0.42, 0.22, 0.08), 0, 0.6), segs=8)
        for i in range(40):
            a = i * 2.39996
            r = 0.04 * math.sqrt((i + 0.5) / 40)
            box(f"Sugar{i}", (0.0018, 0.0018, 0.0018), (math.cos(a) * r, math.sin(a) * r, 0.046 - (r / 0.05) ** 2 * 0.012), m["white"], bevel=0)


@item("waffle")
def _(p, m):
    # Norwegian heart waffle: five hearts around a centre, with a grid pattern and jam + sour cream.
    c = col((0.88, 0.62, 0.28), 0, 0.7)
    cyl("Core", 0.05, 0.0105, (0, 0, 0.00525), c, axis="Z", verts=30)
    for i in range(5):
        a = i * 2 * math.pi / 5
        cx, cy = math.cos(a) * 0.045, math.sin(a) * 0.045
        for sgn in (-1, 1):
            b = a + sgn * 0.55
            hz = 0.0112 + (i * 2 + (sgn > 0)) * 0.00012
            cyl(f"Lobe{i}{sgn}", 0.024, hz, (cx + math.cos(b) * 0.016, cy + math.sin(b) * 0.016, hz / 2), c, axis="Z", verts=24)
        box(f"Point{i}", (0.03, 0.03, 0.0108), (math.cos(a) * 0.028, math.sin(a) * 0.028, 0.0054), c, rot=(0, 0, math.degrees(a) + 45), bevel=0.002)
    for k in range(-4, 5):
        L = 2 * math.sqrt(max(0.0, 0.058 ** 2 - (k * 0.013) ** 2))
        box(f"GridX{k}", (L, 0.0025, 0.002), (0, k * 0.013, 0.0136), col((0.7, 0.45, 0.18), 0, 0.8), bevel=0)
        box(f"GridY{k}", (0.0025, L, 0.002), (k * 0.013, 0, 0.0136), col((0.7, 0.45, 0.18), 0, 0.8), bevel=0)
    sph("Cream", 0.02, (0.01, 0.01, 0.016), col((0.97, 0.95, 0.9), 0, 0.4), scale=(1, 1, 0.45))
    sph("Jam", 0.016, (-0.012, -0.006, 0.019), col((0.55, 0.03, 0.08), 0, 0.15), scale=(1, 1, 0.4))


@item("bar")
def _(p, m):
    c = col(p["color"], 0, 0.35)
    box("Wrapper", (0.12, 0.045, 0.016), (0, 0, 0.008), c, bevel=0.004)
    for sx in (-1, 1):
        x0 = 0.058 if sx > 0 else -0.07
        prism(f"Crimp{sx}", [(x0, 0.001), (x0 + 0.012, 0.0), (x0 + 0.012, 0.016), (x0, 0.015)], 0.05, c, bevel=0.001)
        for k in range(6):
            box(f"Tooth{sx}{k}", (0.002, 0.051, 0.0025), (sx * 0.066, 0, 0.002 + k * 0.0026), col([v * 0.8 for v in p["color"]], 0, 0.4), bevel=0)
    box("Stripe", (0.121, 0.0125, 0.0165), (0, 0, 0.008), m["white"], bevel=0.001)
    box("Logo", (0.04, 0.03, 0.001), (-0.025, 0, 0.0165), m["gold"], bevel=0)


@item("apple")
def _(p, m):
    prof = []
    for i in range(17):
        t = i / 16
        a = -math.pi / 2 + math.pi * t
        r = 0.04 * math.cos(a) * (1 + 0.08 * math.sin(a))
        z = 0.038 + 0.037 * math.sin(a)
        dip = 0.008 * math.exp(-((r / 0.012) ** 2))
        prof.append((max(r, 0.0), z - (dip if t > 0.5 else -dip)))
    lathe("Apple", prof, col((0.72, 0.06, 0.05), 0, 0.28), segs=36)
    rotated(cyl("Stem", 0.0018, 0.022, (0.002, 0, 0.078), m["wood"], axis="Z", verts=8), (0, 12, 0))
    sph("Leaf", 0.012, (0.012, 0.004, 0.084), col((0.22, 0.55, 0.12), 0, 0.5), scale=(1.4, 0.55, 0.12))


@item("banana")
def _(p, m):
    n = 17
    path = []
    rad = []
    for i in range(n):
        t = i / (n - 1)
        a = math.radians(-55 + 110 * t)
        path.append((math.sin(a) * 0.1, 0, 0.1 - math.cos(a) * 0.1 + 0.018))
        rad.append(0.003 + 0.016 * math.sin(math.pi * min(1, max(0, (t - 0.0) / 0.92))) ** 0.6)
    tube("Peel", path, rad, col((0.93, 0.78, 0.12), 0, 0.55), segs=5, squash=(1.0, 1.0))
    a = math.radians(-60)
    rotated(cyl("Stem", 0.004, 0.025, (math.sin(a) * 0.1 - 0.006, 0, 0.1 - math.cos(a) * 0.1 + 0.03), col((0.35, 0.3, 0.1), 0, 0.8), axis="Z", verts=6), (0, -35, 0))
    a = math.radians(57)
    sph("Tip", 0.004, (math.sin(a) * 0.1, 0, 0.1 - math.cos(a) * 0.1 + 0.018), m["bake"])


@item("noodlecup")
def _(p, m):
    lathe("Cup", [(0.0, 0.0), (0.033, 0.0), (0.035, 0.003), (0.047, 0.085), (0.049, 0.088), (0.047, 0.09)], col((0.95, 0.92, 0.85), 0, 0.6), segs=40)
    lathe("Band", [(0.0405, 0.04), (0.045, 0.072)], col((0.9, 0.2, 0.1), 0, 0.5), segs=40)
    lathe("Lid", [(0.0, 0.0905), (0.0495, 0.0905), (0.05, 0.092), (0.0, 0.092)], m["chrome"], segs=40)
    box("LidTab", (0.02, 0.02, 0.0008), (0.055, 0, 0.0915), m["chrome"], rot=(0, 0, 45), bevel=0)
    cyl("LidPrint", 0.03, 0.0005, (0, 0, 0.0925), col((0.9, 0.2, 0.1), 0, 0.5), axis="Z", verts=32)


@item("salmon")
def _(p, m):
    box("Tray", (0.24, 0.1, 0.004), (0, 0, 0.002), col((0.85, 0.7, 0.25), 0.8, 0.3), bevel=0.001)
    for i in range(6):
        x = -0.085 + i * 0.034
        prism(f"Slice{i}", [(x - 0.022, 0.004), (x + 0.02, 0.004), (x + 0.024, 0.008), (x - 0.018, 0.009)], 0.085, col((0.95, 0.42, 0.25), 0, 0.25), bevel=0.0008)
        for k in range(3):
            box(f"Fat{i}{k}", (0.0012, 0.086, 0.0052), (x - 0.01 + k * 0.011, 0, 0.0068), col((0.99, 0.82, 0.72), 0, 0.3), rot=(0, 0, 0), bevel=0)
    box("Label", (0.06, 0.04, 0.0006), (0.07, 0.025, 0.0122), col((0.1, 0.2, 0.5), 0, 0.5), bevel=0)


@item("pizzaslice")
def _(p, m):
    prism("Slice", [(0.0, 0.0), (0.15, -0.0), (0.15, 0.012), (0.0, 0.012)], 0.002, col((0.95, 0.7, 0.2)))
    bpy.ops.object.delete()
    me = bpy.data.meshes.new("Slice")
    o = bpy.data.objects.new("Slice", me)
    bpy.context.collection.objects.link(o)
    bm = bmesh.new()
    top = [bm.verts.new(v) for v in [(0, 0, 0.012), (0.15, -0.06, 0.012), (0.15, 0.06, 0.012)]]
    bot = [bm.verts.new(v) for v in [(0, 0, 0), (0.15, -0.06, 0), (0.15, 0.06, 0)]]
    bm.faces.new(top)
    bm.faces.new(bot[::-1])
    for i in range(3):
        j = (i + 1) % 3
        bm.faces.new((bot[i], bot[j], top[j], top[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    o.data.materials.append(col((0.95, 0.7, 0.2)))
    cyl("Crust", 0.012, 0.13, (0.15, 0, 0.008), col((0.75, 0.45, 0.15)), axis="Y")
    for i, (x, y) in enumerate([(0.06, 0.0), (0.11, 0.025), (0.1, -0.03)]):
        cyl(f"Pep{i}", 0.01, 0.003, (x, y, 0.0135), col((0.7, 0.1, 0.05)), axis="Z")


@item("cheese")
def _(p, m):
    # Brunost block with a cut face and an ostehøvel resting on top.
    c = col((0.62, 0.33, 0.11), 0, 0.35)
    box("Cheese", (0.12, 0.08, 0.075), (0, 0, 0.0375), c, bevel=0.006)
    box("CutFace", (0.002, 0.07, 0.065), (0.0605, 0, 0.0365), col((0.7, 0.4, 0.15), 0, 0.3), bevel=0)
    prism("Curl", [(0.04, 0.075), (0.065, 0.075), (0.07, 0.085), (0.05, 0.083)], 0.05, col((0.7, 0.4, 0.15), 0, 0.3), bevel=0.001)
    box("Wrap", (0.09, 0.082, 0.077), (-0.016, 0, 0.0385), col((0.85, 0.1, 0.08), 0, 0.4), bevel=0.005)
    box("Blade", (0.06, 0.045, 0.0015), (0.035, 0, 0.078), m["chrome"], rot=(0, -4, 0), bevel=0.0005)
    box("Slot", (0.004, 0.035, 0.0018), (0.045, 0, 0.0785), m["dark"], rot=(0, -4, 0), bevel=0)
    rotated(tube("Handle", [(0.0, 0, 0.0), (0.08, 0, 0.0)], [0.006, 0.0075], m["wood"], segs=12), (0, -20, 0)).location = (0.065, 0, 0.08)


@item("grenade")
def _(p, m):
    k = p["kind"]
    steel = m["steel"]
    if k == "frag":
        prof = []
        for i in range(15):
            a = -math.pi / 2 + math.pi * i / 14
            prof.append((0.032 * math.cos(a), 0.038 + 0.04 * math.sin(a)))
        prof = [(0.0, prof[0][1])] + prof[1:-1] + [(0.0115, 0.076)]
        lathe("Body", prof, m["green"], segs=36)
        lathe("Seam", [(0.0322, 0.036), (0.0322, 0.04)], col((0.2, 0.24, 0.13), 0.2, 0.6), segs=36)
        top = 0.076
        for i in range(3):
            box(f"Print{i}", (0.012 - i * 0.002, 0.001, 0.003), (0.0, -0.0318, 0.05 - i * 0.006), col((0.85, 0.8, 0.3)), bevel=0)
    else:
        body = m["dark"] if k == "flash" else col((0.45, 0.47, 0.4), 0.4, 0.5)
        lathe("Body", [(0.0, 0.0), (0.028, 0.0), (0.03, 0.003), (0.03, 0.108), (0.027, 0.112), (0.012, 0.114)], body, segs=36)
        if k == "flash":
            for row in range(2):
                for i in range(6):
                    a = i * math.pi / 3 + row * math.pi / 6
                    box(f"Hole{row}{i}", (0.004, 0.012, 0.016), (math.cos(a) * 0.029, math.sin(a) * 0.029, 0.035 + row * 0.04), m["poly"], rot=(0, 0, math.degrees(a)), bevel=0.001)
        else:
            lathe("Stripe", [(0.0303, 0.07), (0.0303, 0.085)], col((0.9, 0.85, 0.1)), segs=36)
            lathe("Text", [(0.0302, 0.03), (0.0302, 0.05)], col((0.15, 0.15, 0.12)), segs=36)
            for i in range(4):
                a = i * math.pi / 2
                cyl(f"Vent{i}", 0.003, 0.002, (math.cos(a) * 0.015, math.sin(a) * 0.015, 0.001), m["poly"], axis="Z", verts=8)
        top = 0.114
    lathe("Fuse", [(0.0, top - 0.004), (0.0105, top - 0.004), (0.0105, top + 0.01), (0.0085, top + 0.013), (0.006, top + 0.022), (0.0, top + 0.022)], steel, segs=24)
    prism("Spoon", [(0.008, top + 0.02), (0.016, top + 0.018), (0.034, top - 0.01), (0.036, top - 0.06), (0.031, top - 0.062), (0.029, top - 0.012), (0.012, top + 0.012), (0.008, top + 0.014)], 0.011, steel, bevel=0.0006)
    ring = [(-0.012 + math.cos(t) * 0.011, 0.0, top + 0.014 + math.sin(t) * 0.011) for t in [i * 2 * math.pi / 20 for i in range(21)]]
    tube("Ring", ring, [0.0012] * len(ring), m["chrome"], segs=6, caps=False)
    tube("Pin", [(-0.003, 0, top + 0.012), (0.012, 0, top + 0.012)], [0.001, 0.001], m["chrome"], segs=6)


@item("molotov")
def _(p, m):
    glass = col((0.22, 0.4, 0.14), 0, 0.05)
    lathe("Bottle", [(0.0, 0.0), (0.026, 0.0), (0.03, 0.004), (0.03, 0.15), (0.027, 0.17), (0.016, 0.195), (0.012, 0.215), (0.012, 0.23), (0.014, 0.232), (0.012, 0.236)], glass, segs=32)
    lathe("Fuel", [(0.0, 0.003), (0.0302, 0.004), (0.0302, 0.11), (0.0, 0.112)], col((0.75, 0.5, 0.1), 0, 0.1), segs=32)
    lathe("Label", [(0.0305, 0.05), (0.0305, 0.1)], m["paper"], segs=32)
    rag = col((0.78, 0.72, 0.58), 0, 1.0)
    tube("RagNeck", [(0, 0, 0.212), (0, 0, 0.24)], [0.0145, 0.0135], rag, segs=12)
    tube("RagTail", [(0.006, 0, 0.238), (0.014, 0.002, 0.252), (0.02, 0.0, 0.262), (0.03, -0.004, 0.258), (0.036, -0.006, 0.244)], [0.007, 0.0075, 0.007, 0.006, 0.004], rag, segs=8, squash=(1.0, 0.45))
    tube("Twine", [(math.cos(t) * 0.0142, math.sin(t) * 0.0142, 0.218) for t in [i * 2 * math.pi / 16 for i in range(17)]], [0.0012] * 17, col((0.4, 0.3, 0.18), 0, 1.0), segs=5, caps=False)


@item("c4")
def _(p, m):
    box("Block", (0.12, 0.07, 0.04), (0, 0, 0.02), col((0.85, 0.82, 0.7)), bevel=0.005)
    box("Tape", (0.122, 0.072, 0.012), (0, 0, 0.02), m["poly"], bevel=0)
    box("Detonator", (0.04, 0.03, 0.02), (0.02, 0, 0.05), m["poly"])
    sph("Led", 0.004, (0.035, 0, 0.061), m["red"])
    for s in (-1, 1):
        rotated(cyl(f"Wire{s}", 0.0015, 0.08, (-0.02, s * 0.01, 0.05), col((0.8, 0.1, 0.1) if s < 0 else (0.1, 0.2, 0.8))), (0, 60, 0))


@item("mine")
def _(p, m):
    # Directional mine: curved box on scissor legs, sight on top, tripwire spool.
    n = 11
    outer = [(math.sin(t) * 0.12, -0.018 + (1 - math.cos(t)) * 0.12) for t in [-0.7 + 1.4 * i / (n - 1) for i in range(n)]]
    me = bpy.data.meshes.new("Body")
    o = bpy.data.objects.new("Body", me)
    bpy.context.collection.objects.link(o)
    bm = bmesh.new()
    ring = []
    for x, y in outer:
        ring.append([bm.verts.new((x, y + dy, z)) for dy, z in ((0.0, 0.0), (0.0, 0.085), (0.032, 0.085), (0.032, 0.0))])
    for a, b in zip(ring, ring[1:]):
        for k in range(4):
            j = (k + 1) % 4
            bm.faces.new((a[k], b[k], b[j], a[j]))
    bm.faces.new(ring[0])
    bm.faces.new(ring[-1][::-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    o.data.materials.append(m["green"])
    for f in me.polygons:
        f.use_smooth = False
    box("Text", (0.09, 0.001, 0.02), (0, -0.0135, 0.06), col((0.85, 0.82, 0.6)), bevel=0)
    box("Sight", (0.012, 0.012, 0.012), (0, 0.008, 0.092), m["dark"], bevel=0.002)
    for s in (-1, 1):
        cyl(f"Fuze{s}", 0.005, 0.014, (s * 0.04, 0.008, 0.091), m["dark"], axis="Z", verts=12)
        for t in (-1, 1):
            rotated(cyl(f"Leg{s}{t}", 0.0022, 0.075, (s * 0.075, 0.008 + t * 0.016, -0.025), m["steel"], axis="Z", verts=8), (t * 22, 0, 0))
    cyl("Spool", 0.012, 0.012, (0.0, 0.03, 0.02), m["poly"], axis="Y", verts=16)
    tube("Wire", [(0.0, 0.03, 0.02), (0.1, 0.08, 0.0), (0.22, 0.09, 0.0)], [0.0006] * 3, m["chrome"], segs=4)


@item("binoculars")
def _(p, m):
    for s in (-1, 1):
        y = s * 0.034
        o = lathe(f"Tube{s}", [(0.017, -0.06), (0.0185, -0.045), (0.022, -0.02), (0.025, 0.03), (0.026, 0.062), (0.024, 0.065)], m["green"], segs=28)
        o.rotation_euler = (0, math.radians(90), 0)
        o.location = (0.0, y, 0.0)
        o = lathe(f"Armor{s}", [(0.0228, -0.01), (0.0258, 0.03), (0.0262, 0.05)], m["rubber"], segs=28)
        o.rotation_euler = (0, math.radians(90), 0)
        o.location = (0.0, y, 0.0)
        cyl(f"Lens{s}", 0.021, 0.004, (0.063, y, 0.0), m["lens"], verts=28)
        o = lathe(f"Eyecup{s}", [(0.0, -0.08), (0.016, -0.08), (0.017, -0.077), (0.016, -0.06)], m["rubber"], segs=24)
        o.rotation_euler = (0, math.radians(90), 0)
        o.location = (0.0, y, 0.0)
        cyl(f"Ocular{s}", 0.009, 0.002, (-0.08, y, 0.0), m["lens"], verts=20)
    for x in (-0.03, 0.035):
        box(f"Bridge{x}", (0.022, 0.05, 0.012), (x, 0, 0.004), m["dark"], bevel=0.003)
    cyl("Hinge", 0.008, 0.11, (0.0, 0, 0.006), m["dark"], verts=16)
    cyl("Focus", 0.01, 0.018, (-0.04, 0, 0.012), m["rubber"], axis="Y", verts=20)
    box("Strap", (0.01, 0.012, 0.006), (-0.055, 0.058, 0.0), m["poly"], bevel=0.002)


@item("goggles")
def _(p, m):
    box("Housing", (0.05, 0.07, 0.035), (0, 0, 0), m["green"], bevel=0.009)
    box("Battery", (0.03, 0.03, 0.018), (-0.005, 0.045, 0.012), m["green"], bevel=0.005)
    cyl("BattCap", 0.008, 0.006, (-0.005, 0.063, 0.012), m["dark"], axis="Y", verts=16)
    for s in (-1, 1):
        o = lathe(f"Tube{s}", [(0.014, 0.02), (0.016, 0.03), (0.0175, 0.065), (0.019, 0.072), (0.018, 0.075)], m["green"], segs=24)
        o.rotation_euler = (0, math.radians(90), 0)
        o.location = (0, s * 0.018, 0)
        cyl(f"Lens{s}", 0.015, 0.003, (0.075, s * 0.018, 0), col((0.1, 0.5, 0.25), 0.6, 0.05), verts=24)
        o = lathe(f"Eyecup{s}", [(0.0, -0.045), (0.016, -0.045), (0.018, -0.04), (0.015, -0.02)], m["rubber"], segs=24)
        o.rotation_euler = (0, math.radians(90), 0)
        o.location = (0, s * 0.03, 0)
        cyl(f"Knob{s}", 0.005, 0.008, (0.01, s * 0.04, -0.012), m["dark"], axis="Y", verts=12)
    box("Mount", (0.028, 0.03, 0.04), (-0.012, 0, 0.035), m["poly"], bevel=0.004)
    box("Dovetail", (0.04, 0.02, 0.008), (-0.012, 0, 0.058), m["dark"], bevel=0.002)
    box("Strap", (0.012, 0.2, 0.022), (-0.045, 0, -0.005), m["fabric"], bevel=0.003)


@item("drone")
def _(p, m):
    box("Body", (0.1, 0.07, 0.04), (0, 0, 0.02), m["poly"], bevel=0.01)
    cyl("Cam", 0.015, 0.02, (0.055, 0, 0.0), m["lens"])
    for i in range(4):
        a = math.radians(45 + i * 90)
        x, y = math.cos(a) * 0.11, math.sin(a) * 0.11
        rotated(box(f"Arm{i}", (0.15, 0.015, 0.012), (x / 2, y / 2, 0.025), m["dark"]), (0, 0, math.degrees(a)))
        cyl(f"Motor{i}", 0.014, 0.025, (x, y, 0.035), m["dark"], axis="Z")
        cyl(f"Prop{i}", 0.06, 0.002, (x, y, 0.05), col((0.85, 0.3, 0.05)), axis="Z", verts=16)
    sph("Led", 0.005, (-0.05, 0, 0.03), m["red"])


@item("grapple")
def _(p, m):
    box("Body", (0.2, 0.05, 0.06), (0.05, 0, 0.0), col((0.85, 0.4, 0.05)), bevel=0.01)
    prism("Grip", [(-0.04, -0.03), (0.0, -0.03), (-0.02, -0.12), (-0.06, -0.11)], 0.032, m["poly"])
    cyl("Barrel", 0.016, 0.1, (0.2, 0, 0.01), m["dark"])
    cyl("Shaft", 0.006, 0.08, (0.28, 0, 0.01), m["steel"])
    for i in range(3):
        a = i * 120
        rotated(box(f"Prong{i}", (0.008, 0.006, 0.05), (0.3, 0, 0.01), m["steel"]), (a, -35, 0))
    cyl("Spool", 0.025, 0.04, (0.0, 0, 0.05), m["dark"], axis="Y")


@item("parachute")
def _(p, m):
    box("Pack", (0.12, 0.32, 0.42), (0, 0, 0.21), col((0.15, 0.2, 0.12)), bevel=0.03)
    for s in (-1, 1):
        box(f"Strap{s}", (0.03, 0.05, 0.45), (0.07, s * 0.11, 0.2), m["fabric"])
    sph("Handle", 0.015, (0.08, 0.13, 0.15), m["red"])


@item("keycard")
def _(p, m):
    box("Card", (0.085, 0.054, 0.002), (0, 0, 0.001), col(p.get("color", (0.0, 0.25, 0.45))), bevel=0.0008)
    box("Stripe", (0.085, 0.012, 0.0022), (0, 0.015, 0.001), m["white"], bevel=0)
    box("Photo", (0.02, 0.025, 0.0024), (-0.025, -0.008, 0.001), col((0.8, 0.7, 0.6)), bevel=0)
    box("Chip", (0.012, 0.01, 0.0025), (0.02, -0.01, 0.001), m["gold"], bevel=0)


@item("lockpick")
def _(p, m):
    box("Wallet", (0.12, 0.07, 0.01), (0, 0, 0.005), col((0.2, 0.12, 0.06)), bevel=0.004)
    for i in range(5):
        box(f"Pick{i}", (0.1, 0.004, 0.001), (0.03, -0.024 + i * 0.012, 0.012), m["chrome"], bevel=0)


@item("flashlight")
def _(p, m):
    cyl("Body", 0.013, 0.13, (0, 0, 0), m["dark"])
    cone("Head", 0.013, 0.02, 0.04, (0.085, 0, 0), m["dark"], axis="X")
    cyl("Glass", 0.018, 0.003, (0.106, 0, 0), m["glass"])
    cyl("Button", 0.005, 0.006, (-0.067, 0, 0), m["rubber"])


@item("radio")
def _(p, m):
    box("Body", (0.035, 0.058, 0.12), (0, 0, 0.06), m["poly"], bevel=0.008)
    box("Grip", (0.037, 0.06, 0.05), (0, 0, 0.03), m["rubber"], bevel=0.008)
    lathe("Antenna", [(0.0, 0.118), (0.0065, 0.118), (0.0065, 0.135), (0.0045, 0.145), (0.0042, 0.23), (0.003, 0.236), (0.0, 0.237)], m["rubber"], segs=16, loc=(0, 0.014, 0))
    cyl("Knob", 0.0065, 0.014, (0, -0.016, 0.126), m["dark"], axis="Z", verts=18)
    for k in range(12):
        a = k * math.pi / 6
        box(f"KnobRib{k}", (0.0012, 0.0012, 0.012), (math.cos(a) * 0.0067, -0.016 + math.sin(a) * 0.0067, 0.126), m["dark"], bevel=0)
    box("Screen", (0.002, 0.04, 0.022), (0.0177, 0, 0.1), col((0.35, 0.65, 0.35), 0, 0.1), bevel=0.0005)
    box("Bezel", (0.0015, 0.046, 0.028), (0.0172, 0, 0.1), m["dark"], bevel=0.001)
    for r in range(3):
        for c in range(3):
            box(f"Key{r}{c}", (0.003, 0.011, 0.007), (0.018, -0.014 + c * 0.014, 0.075 - r * 0.011), col((0.25, 0.25, 0.27)), bevel=0.001)
    for i in range(6):
        box(f"Grill{i}", (0.002, 0.04, 0.0018), (0.0185, 0, 0.03 - i * 0.004), m["dark"], bevel=0)
    box("PTT", (0.012, 0.004, 0.025), (0.0, 0.031, 0.075), m["dark"], bevel=0.0015)
    box("Clip", (0.004, 0.026, 0.07), (-0.021, 0, 0.065), m["dark"], bevel=0.0015)
    cyl("Led", 0.002, 0.003, (0.006, 0.006, 0.121), col((0.9, 0.1, 0.05), 0, 0.2), axis="Z", verts=8)


@item("ammobox")
def _(p, m):
    box("Box", (0.28, 0.14, 0.18), (0, 0, 0.09), m["green"], bevel=0.006)
    box("Lid", (0.29, 0.15, 0.025), (0, 0, 0.19), m["green"], bevel=0.004)
    box("Handle", (0.12, 0.02, 0.015), (0, 0, 0.21), m["dark"])
    box("Stencil", (0.15, 0.142, 0.03), (0, 0, 0.1), col((0.85, 0.8, 0.3)), bevel=0)


@item("plate")
def _(p, m):
    box("Plate", (0.25, 0.3, 0.025), (0, 0, 0.0125), col((0.3, 0.3, 0.28)), bevel=0.02)
    box("Label", (0.1, 0.06, 0.026), (0, 0.05, 0.013), m["white"], bevel=0)


@item("backpack")
def _(p, m):
    s = p["size"]
    c = col((0.15, 0.25, 0.35) if s < 1 else (0.35, 0.18, 0.08), 0, 0.95)
    box("Main", (0.18 * s, 0.32 * s, 0.45 * s), (0, 0, 0.225 * s), c, bevel=0.04 * s)
    box("Flap", (0.2 * s, 0.33 * s, 0.1 * s), (0.01, 0, 0.42 * s), c, bevel=0.03 * s)
    box("Pocket", (0.06 * s, 0.24 * s, 0.18 * s), (0.11 * s, 0, 0.14 * s), c, bevel=0.02 * s)
    for sd in (-1, 1):
        box(f"Strap{sd}", (0.025, 0.05, 0.4 * s), (-0.1 * s, sd * 0.1 * s, 0.22 * s), m["fabric"])
        cyl(f"SideBottle{sd}", 0.03, 0.15, (0, sd * 0.18 * s, 0.12 * s), col((0.3, 0.6, 0.8)), axis="Z")


@item("suppressor")
def _(p, m):
    # can centred on the bore axis (z = 0); the mount starts 11 cm behind the centre
    o = lathe("Can", [(0.0, -0.085), (0.015, -0.085), (0.018, -0.08), (0.018, 0.078), (0.016, 0.085), (0.005, 0.087), (0.005, 0.083), (0.0, 0.083)], m["poly"], segs=32)
    o.rotation_euler = (0, math.radians(90), 0)
    o = lathe("Mount", [(0.0, -0.112), (0.0105, -0.112), (0.0125, -0.105), (0.0125, -0.084), (0.0, -0.084)], m["dark"], segs=24)
    o.rotation_euler = (0, math.radians(90), 0)
    for k in range(6):
        o = lathe(f"Band{k}", [(0.0183, -0.06 + k * 0.025), (0.0183, -0.055 + k * 0.025)], m["dark"], segs=32)
        o.rotation_euler = (0, math.radians(90), 0)
    for k in range(8):
        a = k * math.pi / 4
        box(f"Flat{k}", (0.012, 0.006, 0.003), (-0.098, math.cos(a) * 0.0125, math.sin(a) * 0.0125), m["steel"], rot=(math.degrees(a) + 90, 0, 0), bevel=0.0005)


@item("reddot")
def _(p, m):
    # optical axis 3.2 cm above the base
    box("Base", (0.05, 0.03, 0.01), (0, 0, 0.005), m["dark"], bevel=0.002)
    box("Clamp", (0.02, 0.006, 0.014), (0, -0.017, 0.007), m["dark"], bevel=0.001)
    cyl("Knob", 0.005, 0.008, (0, -0.021, 0.006), m["steel"], axis="Y", verts=12)
    o = lathe("Tube", [(0.0145, -0.028), (0.017, -0.026), (0.017, 0.026), (0.0185, 0.028), (0.0185, 0.032), (0.0155, 0.032), (0.0145, 0.03)], m["poly"], segs=28)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0, 0, 0.032)
    box("Saddle", (0.04, 0.022, 0.014), (0, 0, 0.016), m["poly"], bevel=0.003)
    cyl("Lens", 0.0145, 0.002, (0.02, 0, 0.032), m["lens"], verts=28)
    cyl("Turret", 0.007, 0.012, (0.0, 0.019, 0.032), m["dark"], axis="Y", verts=16)
    cyl("TurretTop", 0.007, 0.01, (0.0, 0, 0.051), m["dark"], axis="Z", verts=16)
    sph("Dot", 0.0012, (0.0, 0, 0.032), col((1, 0.05, 0.05)))


@item("scope")
def _(p, m):
    # 4x: axis 4 cm above the base
    o = lathe("Tube", [(0.02, -0.15), (0.021, -0.145), (0.021, -0.1), (0.0135, -0.08), (0.0135, 0.08), (0.022, 0.12), (0.022, 0.16), (0.0195, 0.163)], m["poly"], segs=32)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0, 0, 0.04)
    cyl("LensF", 0.0195, 0.002, (0.162, 0, 0.04), m["lens"], verts=32)
    cyl("LensR", 0.018, 0.002, (-0.15, 0, 0.04), m["lens"], verts=32)
    for x in (-0.05, 0.05):
        box(f"RingBase{x}", (0.018, 0.026, 0.02), (x, 0, 0.01), m["dark"], bevel=0.002)
        o = lathe(f"Ring{x}", [(0.0138, -0.009), (0.0165, -0.009), (0.0165, 0.009), (0.0138, 0.009)], m["dark"], segs=28)
        o.rotation_euler = (0, math.radians(90), 0)
        o.location = (x, 0, 0.04)
    cyl("TurretTop", 0.011, 0.016, (0, 0, 0.06), m["dark"], axis="Z", verts=20)
    cyl("TurretSide", 0.011, 0.016, (0, 0.02, 0.04), m["dark"], axis="Y", verts=20)
    o = lathe("Zoom", [(0.022, -0.1), (0.022, -0.085)], m["rubber"], segs=32)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0, 0, 0.04)


@item("extmag")
def _(p, m):
    # Extended curved rifle magazine (40 rounds): one smooth arc body with ribs and a floor plate.
    n = 14
    L = 0.3
    pts_f, pts_b = [], []
    for i in range(n):
        t = i / (n - 1)
        a = math.radians(5 + 28 * t)
        cx = 0.12 * (1 - math.cos(a)) * 1.9
        cz = -L * t
        pts_f.append((cx + 0.02 * math.cos(a), cz - 0.02 * math.sin(a) * 0.3))
        pts_b.append((cx - 0.02 * math.cos(a), cz + 0.02 * math.sin(a) * 0.3))
    prism("Body", pts_f + pts_b[::-1], 0.024, m["dark"], bevel=0.002)
    for i in range(1, n - 1):
        x0, z0 = pts_f[i]
        x1, z1 = pts_b[i]
        if i % 2 == 0:
            box(f"Rib{i}", (abs(x0 - x1) * 0.8, 0.0255, 0.006), ((x0 + x1) / 2, 0, (z0 + z1) / 2), m["dark"], rot=(0, -math.degrees(math.radians(5 + 28 * i / (n - 1))), 0), bevel=0.001)
    xe, ze = (pts_f[-1][0] + pts_b[-1][0]) / 2, (pts_f[-1][1] + pts_b[-1][1]) / 2
    box("Floor", (0.05, 0.03, 0.012), (xe, 0, ze - 0.004), m["poly"], rot=(0, -33, 0), bevel=0.003)
    box("Lips", (0.03, 0.018, 0.012), (0.0, 0, 0.006), m["dark"], bevel=0.001)
    sph("Round", 0.005, (0.008, 0, 0.012), m["brass"], scale=(3, 1, 1))


@item("laser")
def _(p, m):
    box("Body", (0.06, 0.03, 0.026), (0, 0, 0.013), m["poly"], bevel=0.005)
    box("Clamp", (0.04, 0.034, 0.006), (0, 0, 0.028), m["dark"], bevel=0.0015)
    o = lathe("Bezel", [(0.0, 0.0), (0.009, 0.0), (0.0095, 0.008), (0.008, 0.01), (0.0, 0.01)], m["dark"], segs=20)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0.03, -0.006, 0.013)
    cyl("Emitter", 0.004, 0.002, (0.041, -0.006, 0.013), col((1, 0.05, 0.05)), verts=16)
    cyl("Light", 0.0085, 0.002, (0.031, 0.008, 0.013), m["glass"], verts=20)
    box("Switch", (0.012, 0.006, 0.01), (-0.024, 0.0175, 0.012), m["rubber"], bevel=0.002)


@item("compass")
def _(p, m):
    lathe("Case", [(0.0, 0.0), (0.028, 0.0), (0.031, 0.003), (0.031, 0.011), (0.029, 0.013), (0.027, 0.0125)], m["brass"], segs=40)
    cyl("Face", 0.0268, 0.001, (0, 0, 0.0095), m["white"], axis="Z", verts=40)
    for k in range(36):
        a = k * math.pi / 18
        L = 0.005 if k % 9 == 0 else 0.0025
        box(f"Tick{k}", (L, 0.0006, 0.0004), (math.cos(a) * (0.0245 - L / 2), math.sin(a) * (0.0245 - L / 2), 0.0101), m["dark"], rot=(0, 0, math.degrees(a)), bevel=0)
    prism("NeedleN", [(0.0, 0.0103), (0.02, 0.0108), (0.0, 0.0113)], 0.005, m["red"], bevel=0)
    prism("NeedleS", [(0.0, 0.0103), (-0.02, 0.0108), (0.0, 0.0113)], 0.005, m["dark"], bevel=0)
    cyl("Pivot", 0.0018, 0.003, (0, 0, 0.0112), m["brass"], axis="Z", verts=12)
    cyl("Glass", 0.0275, 0.0008, (0, 0, 0.0122), m["glass"], axis="Z", verts=40)
    lathe("Bow", [(0.004, 0.0), (0.004, 0.006)], m["brass"], segs=16, loc=(0.034, 0, 0.004))
    ring = [(0.04 + math.cos(t) * 0.007, math.sin(t) * 0.007, 0.006) for t in [i * 2 * math.pi / 20 for i in range(21)]]
    tube("Ring", ring, [0.0012] * len(ring), m["brass"], segs=6, caps=False)


@item("defib")
def _(p, m):
    box("Case", (0.25, 0.1, 0.25), (0, 0, 0.125), col((0.95, 0.75, 0.05)), bevel=0.02)
    box("Panel", (0.15, 0.102, 0.1), (0, 0, 0.15), col((0.1, 0.6, 0.2)), bevel=0)
    box("Handle", (0.12, 0.025, 0.025), (0, 0, 0.265), m["poly"])
    for s in (-1, 1):
        box(f"Pad{s}", (0.06, 0.012, 0.09), (s * 0.08, -0.06, 0.08), m["white"], bevel=0.005)


@item("trophy")
def _(p, m):
    g = m["gold"]
    box("Base", (0.1, 0.1, 0.035), (0, 0, 0.0175), m["dark"], bevel=0.004)
    box("Plate", (0.06, 0.001, 0.016), (0, -0.0505, 0.0175), m["brass"], bevel=0)
    lathe("Stem", [(0.0, 0.035), (0.03, 0.035), (0.028, 0.042), (0.012, 0.048), (0.009, 0.07), (0.016, 0.078), (0.009, 0.086), (0.008, 0.11), (0.02, 0.12)], g, segs=36)
    prof = [(0.02, 0.118)]
    for i in range(12):
        t = i / 11
        prof.append((0.02 + 0.05 * math.sin(t * math.pi / 2) ** 0.8, 0.12 + 0.11 * t))
    prof += [(0.073, 0.232), (0.066, 0.23), (0.0, 0.2)]
    lathe("Cup", prof, g, segs=40)
    for s in (-1, 1):
        path = [(0, s * (0.058 + 0.03 * math.sin(t)), 0.2 - 0.06 * (1 - math.cos(t)) - 0.01 * t) for t in [i * math.pi / 10 for i in range(11)]]
        tube(f"Handle{s}", path, [0.0045] * len(path), g, segs=10)




# ------------------------------------------------------------------ new weapons (round 3)
def build_overunder(p, m):
    """Over/under competition shotgun: engraved silver receiver, walnut stock with palm swell, vented rib."""
    walnut = col((0.32, 0.14, 0.05), 0, 0.35)
    for z in (0.012, 0.037):
        cyl(f"Barrel{z}", 0.0115, 0.72, (0.42, 0, z), m["dark"], verts=24)
    box("Rib", (0.72, 0.008, 0.004), (0.42, 0, 0.051), m["dark"], bevel=0)
    for i in range(24):
        box(f"RibPost{i}", (0.004, 0.006, 0.006), (0.08 + i * 0.03, 0, 0.0485), m["dark"], bevel=0)
    box("Receiver", (0.11, 0.046, 0.07), (0.0, 0, 0.02), m["chrome"], bevel=0.008)
    for i in range(7):  # engraving scrolls
        cyl(f"Scroll{i}", 0.006 + (i % 3) * 0.002, 0.0485, (-0.035 + i * 0.012, 0, 0.012 + (i % 2) * 0.02), col((0.45, 0.45, 0.47), 1, 0.4), axis="Y", verts=12)
    box("TopLever", (0.04, 0.012, 0.006), (-0.06, 0, 0.057), m["steel"], rot=(0, 0, 18))
    prism("Forend", [(0.06, -0.012), (0.33, -0.006), (0.34, 0.002), (0.06, 0.002)], 0.05, walnut, bevel=0.008)
    for i in range(10):
        box(f"Checker{i}", (0.012, 0.0505, 0.002), (0.12 + i * 0.018, 0, -0.008), col((0.22, 0.1, 0.04)), bevel=0)
    prism("Stock", [(-0.42, -0.16), (-0.25, -0.1), (-0.12, -0.075), (-0.07, -0.085), (-0.03, -0.04), (-0.055, -0.01),
                    (-0.055, 0.045), (-0.2, 0.035), (-0.42, 0.02)], 0.042, walnut, bevel=0.01)
    box("ButtPad", (0.016, 0.045, 0.19), (-0.428, 0, -0.07), m["rubber"], rot=(0, -8, 0))
    box("TriggerGuard", (0.065, 0.01, 0.006), (-0.005, 0, -0.03), m["chrome"])
    box("Trigger", (0.006, 0.006, 0.022), (-0.01, 0, -0.02), col((0.7, 0.55, 0.2), 1, 0.3))
    sph("Bead", 0.0035, (0.775, 0, 0.056), m["white"])
    return dict(muzzle=0.78, sight=0.055)


def build_m72(p, m):
    """M72 LAW: thin telescoping olive tube with flip-up sights."""
    cyl("Tube", 0.035, 0.66, (0.06, 0, 0.06), m["green"], verts=24)
    cyl("Inner", 0.031, 0.24, (0.48, 0, 0.06), col((0.12, 0.14, 0.08), 0, 0.7), verts=24)
    for x in (-0.25, 0.37):
        cyl(f"Cap{x}", 0.037, 0.03, (x, 0, 0.06), m["poly"], verts=24)
    box("Trigger", (0.05, 0.03, 0.035), (0.1, 0, 0.11), m["poly"])
    box("FrontSight", (0.008, 0.04, 0.04), (0.34, 0, 0.115), m["poly"])
    box("RearSight", (0.008, 0.035, 0.05), (-0.05, 0, 0.12), m["poly"])
    box("Sling", (0.6, 0.004, 0.02), (0.05, -0.04, 0.03), m["fabric"], bevel=0)
    box("Label", (0.12, 0.0705, 0.03), (0.15, 0, 0.06), col((0.85, 0.8, 0.3)), bevel=0)
    return dict(muzzle=0.6, sight=0.14)


def build_daneaxe(p, m):
    """Double-bladed battle axe with leather wrapped handle."""
    h = p.get("handle", 0.85)
    cyl("Handle", 0.017, h, (h / 2 - 0.15, 0, 0), col((0.25, 0.14, 0.07), 0, 0.6), verts=12)
    for i in range(8):
        rotated(cyl(f"Leather{i}", 0.0195, 0.025, (-0.13 + i * 0.026, 0, 0), col((0.18, 0.09, 0.04), 0, 0.9), verts=12), (0, 0, 0))
    end = h - 0.15
    steel = col((0.55, 0.56, 0.58), 1, 0.25)
    for s in (1, -1):
        prism(f"Blade{s}", [(end - 0.03, 0.0), (end + 0.03, 0.0), (end + 0.09, s * 0.19), (end - 0.1, s * 0.19), (end - 0.05, s * 0.04)][::s],
              0.01, steel, bevel=0.002)
    box("Socket", (0.08, 0.04, 0.05), (end, 0, 0.0), m["dark"])
    cone("Spike", 0.016, 0.0, 0.08, (end + 0.08, 0, 0.0), m["dark"], axis="X")
    sph("Pommel", 0.022, (-0.16, 0, 0), m["dark"])
    return dict(muzzle=end + 0.1, sight=0.0)


def build_spear(p, m):
    cyl("Shaft", 0.014, 1.7, (0.55, 0, 0), m["lwood"], verts=12)
    prism("Head", [(1.38, -0.025), (1.6, 0.0), (1.38, 0.025), (1.36, 0.0)], 0.006, m["blade"], bevel=0.001)
    cyl("Socket", 0.017, 0.08, (1.35, 0, 0), m["dark"], verts=12)
    cyl("Wrap", 0.016, 0.2, (0.0, 0, 0), m["fabric"], verts=12)
    return dict(muzzle=1.6, sight=0.0)


def build_bow(p, m):
    """Recurve longbow lying along X with the string towards -X."""
    wood = col((0.4, 0.22, 0.1), 0, 0.5)
    for s in (-1, 1):
        rotated(box(f"Limb{s}", (0.022, 0.03, 0.62), (0.03, 0, s * 0.36), wood, bevel=0.006), (0, s * -10, 0))
        rotated(box(f"Tip{s}", (0.03, 0.02, 0.08), (0.1, 0, s * 0.69), wood, bevel=0.004), (0, s * 35, 0))
    box("Grip", (0.04, 0.035, 0.14), (0.0, 0, 0.0), m["fabric"], bevel=0.008)
    box("String", (0.002, 0.002, 1.36), (-0.03, 0, 0.0), m["white"], bevel=0)
    cyl("Arrow", 0.004, 0.75, (0.3, 0, 0.02), m["lwood"])
    cone("ArrowTip", 0.008, 0.0, 0.04, (0.69, 0, 0.02), m["steel"], axis="X")
    return dict(muzzle=0.7, sight=0.03)


def build_flare(p, m):
    """Orange signal pistol with a fat barrel."""
    org = col((0.95, 0.35, 0.05), 0, 0.4)
    cyl("Barrel", 0.02, 0.18, (0.08, 0, 0.03), org, verts=20)
    cyl("Bore", 0.015, 0.182, (0.08, 0, 0.03), m["dark"], verts=20)
    box("Frame", (0.08, 0.035, 0.05), (-0.03, 0, 0.02), org, bevel=0.006)
    prism("Grip", [(-0.06, 0.0), (0.0, 0.0), (-0.02, -0.11), (-0.08, -0.1)], 0.032, org)
    box("Hammer", (0.02, 0.01, 0.02), (-0.07, 0, 0.05), m["dark"], rot=(0, 30, 0))
    box("TriggerGuard", (0.05, 0.01, 0.005), (0.0, 0, -0.02), org)
    return dict(muzzle=0.17, sight=0.055)


def build_printed(p, m):
    """3D-printed pistol: chunky white and orange plastic, visible print layers."""
    white = col((0.92, 0.92, 0.9), 0, 0.55)
    orange = col((1.0, 0.45, 0.08), 0, 0.5)
    box("Body", (0.17, 0.03, 0.05), (0.02, 0, 0.03), white, bevel=0.004)
    for i in range(9):
        box(f"Layer{i}", (0.172, 0.031, 0.0015), (0.02, 0, 0.007 + i * 0.005), col((0.85, 0.85, 0.83)), bevel=0)
    cyl("Barrel", 0.01, 0.06, (0.12, 0, 0.04), orange)
    prism("Grip", [(-0.05, 0.005), (0.0, 0.005), (-0.02, -0.1), (-0.07, -0.095)], 0.03, orange)
    box("Trigger", (0.006, 0.006, 0.02), (0.01, 0, -0.008), white)
    return dict(muzzle=0.15, sight=0.06)


def build_printknife(p, m):
    orange = col((1.0, 0.45, 0.08), 0, 0.5)
    prism("Blade", [(0.02, -0.015), (0.15, -0.012), (0.18, 0.004), (0.15, 0.016), (0.02, 0.016)], 0.006, col((0.92, 0.92, 0.9), 0, 0.5))
    box("Handle", (0.11, 0.025, 0.03), (-0.04, 0, 0.0), orange, bevel=0.006)
    return dict(muzzle=0.18, sight=0.0)


# ------------------------------------------------------------------ ammo, attachments, filament, new food (round 3)
@item("cal")
def _(p, m):
    """Ammo box for one caliber: cardboard box with a coloured band and a few rounds on top."""
    c = col(p["color"], 0, 0.7)
    box("Box", (0.12, 0.08, 0.06), (0, 0, 0.03), col((0.75, 0.65, 0.45), 0, 0.85), bevel=0.003)
    box("Band", (0.122, 0.082, 0.02), (0, 0, 0.035), c, bevel=0)
    L, r = p.get("len", 0.03), p.get("r", 0.005)
    for i in range(p.get("n", 5)):
        y = -0.03 + i * 0.015
        if p.get("shell"):
            cyl(f"Hull{i}", r, L * 0.8, (0.0, y, 0.066), col((0.8, 0.1, 0.08), 0, 0.5))
            cyl(f"Base{i}", r * 1.05, L * 0.2, (-L * 0.5, y, 0.066), m["brass"])
        else:
            cyl(f"Case{i}", r, L, (0.0, y, 0.066), m["brass"])
            cone(f"Tip{i}", r * 0.95, r * 0.2, L * 0.5, (L * 0.75, y, 0.066), col(p.get("tip", (0.6, 0.35, 0.15)), 0.9, 0.3), axis="X")


@item("specialammo")
def _(p, m):
    """Special ammo: dark tin with a bright stripe and an icon."""
    c = col(p["color"], 0, 0.4)
    cyl("Tin", 0.045, 0.07, (0, 0, 0.035), m["dark"], axis="Z", verts=24)
    cyl("Stripe", 0.0455, 0.018, (0, 0, 0.04), c, axis="Z", verts=24)
    cyl("Lid", 0.047, 0.008, (0, 0, 0.072), m["steel"], axis="Z", verts=24)
    for i in range(3):
        cyl(f"Round{i}", 0.005, 0.045, (0.0, -0.012 + i * 0.012, 0.09), m["brass"])
        cone(f"RoundTip{i}", 0.0048, 0.001, 0.02, (0.032, -0.012 + i * 0.012, 0.09), c, axis="X")


@item("holo")
def _(p, m):
    # window centre 3.7 cm above the base
    box("Base", (0.075, 0.036, 0.012), (0, 0, 0.006), m["dark"], bevel=0.002)
    box("Body", (0.05, 0.04, 0.022), (-0.012, 0, 0.022), m["poly"], bevel=0.004)
    box("Battery", (0.03, 0.044, 0.016), (-0.022, 0, 0.012), m["poly"], bevel=0.004)
    for sy in (-1, 1):
        prism(f"Hood{sy}", [(-0.01, 0.028), (0.035, 0.028), (0.035, 0.06), (0.02, 0.062), (-0.01, 0.05)], 0.004, m["poly"], y=sy * 0.021, bevel=0.001)
    box("HoodTop", (0.045, 0.046, 0.004), (0.012, 0, 0.061), m["poly"], bevel=0.0015)
    box("Window", (0.002, 0.038, 0.03), (0.03, 0, 0.044), m["lens"], bevel=0)
    box("WindowR", (0.002, 0.038, 0.026), (-0.006, 0, 0.044), m["lens"], bevel=0)
    for k in range(3):
        box(f"Button{k}", (0.006, 0.004, 0.004), (-0.025 + k * 0.009, -0.0205, 0.025), m["rubber"], bevel=0.0008)
    cyl("Lever", 0.004, 0.01, (0.02, 0.022, 0.006), m["steel"], axis="Y", verts=10)
    box("Reticle", (0.0006, 0.004, 0.004), (0.031, 0, 0.037), col((1, 0.1, 0.1)), bevel=0)


@item("scope8")
def _(p, m):
    # 8x: axis 5 cm above the base
    o = lathe("Tube", [(0.021, -0.21), (0.022, -0.205), (0.022, -0.14), (0.015, -0.115), (0.015, 0.1), (0.029, 0.17), (0.029, 0.26), (0.026, 0.264)], m["poly"], segs=36)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0, 0, 0.05)
    cyl("Lens", 0.027, 0.002, (0.263, 0, 0.05), m["lens"], verts=32)
    cyl("LensR", 0.019, 0.002, (-0.21, 0, 0.05), m["lens"], verts=32)
    o = lathe("Shade", [(0.0295, 0.26), (0.0295, 0.32), (0.028, 0.322)], m["poly"], segs=36)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0, 0, 0.05)
    for x in (-0.07, 0.07):
        box(f"RingBase{x}", (0.022, 0.032, 0.03), (x, 0, 0.015), m["dark"], bevel=0.002)
        o = lathe(f"Ring{x}", [(0.0152, -0.011), (0.0185, -0.011), (0.0185, 0.011), (0.0152, 0.011)], m["dark"], segs=28)
        o.rotation_euler = (0, math.radians(90), 0)
        o.location = (x, 0, 0.05)
    cyl("TurretTop", 0.013, 0.022, (0.0, 0, 0.074), m["dark"], axis="Z", verts=24)
    cyl("TurretSide", 0.013, 0.022, (0.0, 0.026, 0.05), m["dark"], axis="Y", verts=24)
    cyl("Parallax", 0.011, 0.016, (0.0, -0.024, 0.05), m["dark"], axis="Y", verts=24)
    for k in range(12):
        a = k * math.pi / 6
        box(f"Click{k}", (0.0015, 0.0015, 0.02), (math.cos(a) * 0.0132, math.sin(a) * 0.0132, 0.074), m["steel"], bevel=0)


@item("drum")
def _(p, m):
    cyl("Drum", 0.075, 0.05, (0, 0, 0.0), m["dark"], axis="Y", verts=32)
    cyl("Cap", 0.03, 0.054, (0, 0, 0.0), m["steel"], axis="Y", verts=24)
    box("Neck", (0.04, 0.026, 0.08), (0.02, 0, 0.09), m["dark"])


@item("comp")
def _(p, m):
    o = lathe("Body", [(0.0058, -0.03), (0.0125, -0.03), (0.0135, -0.026), (0.0135, 0.026), (0.012, 0.03), (0.0058, 0.03)], m["dark"], segs=24)
    o.rotation_euler = (0, math.radians(90), 0)
    o.location = (0, 0, 0.013)
    for i in range(3):
        for sy in (-1, 1):
            box(f"Port{i}{sy}", (0.006, 0.006, 0.012), (-0.01 + i * 0.012, sy * 0.0115, 0.017), m["poly"], bevel=0.0008)
        box(f"TopPort{i}", (0.006, 0.01, 0.006), (-0.01 + i * 0.012, 0, 0.0262), m["poly"], bevel=0.0008)


@item("vgrip")
def _(p, m):
    box("Clamp", (0.05, 0.03, 0.012), (0, 0, 0.1015), m["dark"], bevel=0.002)
    cyl("Screw", 0.004, 0.034, (0.0, 0, 0.1), m["steel"], axis="Y", verts=10)
    lathe("Grip", [(0.0, 0.0), (0.016, 0.0), (0.0175, 0.004), (0.0165, 0.03), (0.0145, 0.06), (0.014, 0.09), (0.0155, 0.096), (0.0, 0.096)], m["poly"], segs=24)
    for i in range(4):
        lathe(f"Ridge{i}", [(0.0168 - i * 0.0006, 0.012 + i * 0.02), (0.0168 - i * 0.0006, 0.018 + i * 0.02)], m["rubber"], segs=24)
    cyl("Cap", 0.012, 0.003, (0, 0, -0.0005), m["dark"], axis="Z", verts=20)


@item("spool")
def _(p, m):
    c = col(p.get("color", (1.0, 0.45, 0.08)), 0, 0.45)
    cyl("Core", 0.03, 0.06, (0, 0, 0), m["white"], axis="Y", verts=24)
    cyl("Filament", 0.08, 0.055, (0, 0, 0), c, axis="Y", verts=32)
    for y in (-0.031, 0.031):
        cyl(f"Flange{y}", 0.1, 0.006, (0, y, 0), col((0.1, 0.1, 0.1)), axis="Y", verts=32)
    cyl("Hole", 0.026, 0.07, (0, 0, 0), m["dark"], axis="Y", verts=16)


@item("lefse")
def _(p, m):
    cyl("Lefse", 0.12, 0.004, (0, 0, 0.002), col((0.92, 0.85, 0.65), 0, 0.9), axis="Z", verts=32)
    for i in range(9):
        a = i * 0.7
        cyl(f"Spot{i}", 0.012, 0.0045, (math.cos(a) * 0.06 * (i % 3 + 1) / 2, math.sin(a) * 0.06 * (i % 3 + 1) / 2, 0.0025),
            col((0.55, 0.38, 0.2)), axis="Z", verts=10)
    box("Fold", (0.12, 0.24, 0.004), (0.06, 0, 0.008), col((0.9, 0.83, 0.62), 0, 0.9), rot=(0, -8, 0))
    box("Butter", (0.08, 0.15, 0.002), (0.05, 0, 0.0055), col((0.95, 0.85, 0.4), 0, 0.4))


WEAPON_BUILDERS = dict(overunder=build_overunder, m72=build_m72, daneaxe=build_daneaxe, spear=build_spear, bow=build_bow,
                       flare=build_flare, printed=build_printed, printknife=build_printknife,
                       pistol=build_pistol, revolver=build_revolver, p90=build_p90, pistolknife=build_pistolknife, rifle=build_rifle,
                       bolt=build_bolt, musket=build_musket, shotgun=build_shotgun, launcher=build_launcher,
                       m79=build_m79, crossbow=build_crossbow, knife=build_knife, axe=build_axe, bat=build_bat,
                       crowbar=build_crowbar, pan=build_pan, vest=build_vest, exo=build_exo, helmet=build_helmet)


def build(it, metrics):
    p = it["model"]
    if "custom" in p:
        metrics[it["id"]] = dict(mesh=p["custom"], muzzle=p["muzzle"], sight=p["sight"])
        return
    reset_scene()
    m = M()
    arch = p["arch"]
    if arch in WEAPON_BUILDERS:
        met = WEAPON_BUILDERS[arch](p, m)
    else:
        ITEM_BUILDERS[arch](p, m)
        met = None
    met = finish(it["id"], p.get("scale", 1.0), met)
    if met is not None and "muzzle" in met:
        # barrel axis height at the muzzle: centre of the vertices at the very front of the gun
        obj = bpy.data.objects.get(f"SM_{it['id']}")
        if obj is not None:
            xs = [v.co.x for v in obj.data.vertices]
            front = max(xs)
            tip = [v.co.z for v in obj.data.vertices if v.co.x > front - 0.012]
            if tip:
                met["bore"] = round((max(tip) + min(tip)) / 2, 4)
    metrics[it["id"]] = dict(mesh=f"SM_{it['id']}", **(met or {}))


def main(only):
    metrics = {}
    if os.path.exists(METRICS_PATH):
        metrics = json.load(open(METRICS_PATH))
    for it in catalog.ITEMS:
        if only and it["id"] not in only:
            continue
        build(it, metrics)
        print("built", it["id"])
    with open(METRICS_PATH, "w") as f:
        json.dump(metrics, f, indent=1, sort_keys=True)


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if "--" in sys.argv:
        args = sys.argv[sys.argv.index("--") + 1:]
    main(set(args))
