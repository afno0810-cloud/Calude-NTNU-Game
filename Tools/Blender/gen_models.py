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
    for side in (-1, 1):
        prism(f"GripPanel{side}", [(-0.058, -0.012), (-0.008, -0.012), (-0.024, -0.098), (-0.074, -0.094)], 0.003,
              m["rubber"], y=side * 0.0155)
    cyl("Bore", 0.006, 0.01, (s - 0.06, 0, 0.012 + h * 0.6), m["dark"])
    prism("Grip", [(-0.06, 0.0), (0.0, 0.0), (-0.02, -0.11), (-0.08, -0.105)], 0.03, frame)
    box("MagBase", (0.05, 0.031, 0.01), (-0.05, 0, -0.108), m["dark"], rot=(0, -15, 0))
    box("TriggerGuard", (0.05, 0.01, 0.005), (0.015, 0, -0.035), frame)
    box("Trigger", (0.005, 0.005, 0.02), (0.008, 0, -0.022), m["steel"])
    top = 0.012 + h
    box("FrontSight", (0.006, 0.004, 0.006), (s - 0.07, 0, top + 0.003), m["dark"])
    box("RearSight", (0.008, 0.02, 0.007), (-0.045, 0, top + 0.0035), m["dark"])
    return dict(muzzle=s - 0.06, sight=top + 0.006)


def build_revolver(p, m):
    b = p["barrel"]
    cyl("Barrel", 0.011, b, (0.06 + b / 2, 0, 0.035), m["chrome"])
    box("Rib", (b, 0.008, 0.008), (0.06 + b / 2, 0, 0.047), m["chrome"])
    cyl("Ejector", 0.006, b * 0.7, (0.06 + b * 0.35, 0, 0.018), m["chrome"])
    box("Frame", (0.12, 0.03, 0.05), (0.0, 0, 0.022), m["chrome"])
    cyl("Cylinder", 0.021, 0.045, (0.03, 0, 0.028), m["chrome"], verts=6, bevel=0.003)
    box("Hammer", (0.02, 0.008, 0.02), (-0.06, 0, 0.05), m["steel"], rot=(0, 30, 0))
    prism("Grip", [(-0.06, 0.0), (-0.01, 0.0), (-0.03, -0.11), (-0.09, -0.1)], 0.032, m["wood"])
    box("TriggerGuard", (0.045, 0.01, 0.005), (0.01, 0, -0.015), m["chrome"])
    box("FrontSight", (0.01, 0.004, 0.012), (0.05 + b, 0, 0.052), m["steel"])
    return dict(muzzle=0.06 + b, sight=0.058)


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
            prism("Stock", [(x0 - 0.32, -0.13), (x0 + 0.005, -0.035), (x0 + 0.005, top - 0.005), (x0 - 0.32, top - 0.02)],
                  0.044, smat, bevel=0.006)
        box("ButtPad", (0.015, 0.046, top + 0.12), (x0 - 0.327, 0, (top - 0.13) / 2 - 0.005), m["rubber"])
    elif stock == "thumbhole":
        prism("StockTop", [(x0 - 0.33, top - 0.04), (x0, top - 0.035), (x0, top - 0.005), (x0 - 0.33, top - 0.01)], 0.04, furn)
        prism("StockLow", [(x0 - 0.33, -0.13), (x0 - 0.02, -0.06), (x0, -0.03), (x0 - 0.05, -0.03), (x0 - 0.33, -0.08)], 0.04, furn)
        box("StockRear", (0.03, 0.04, top + 0.13), (x0 - 0.32, 0, (top - 0.13) / 2), furn)
    elif stock == "retract":
        cyl("Tube", 0.014, 0.2, (x0 - 0.1, 0, top - 0.03), m["dark"])
        prism("Butt", [(x0 - 0.24, -0.09), (x0 - 0.17, -0.06), (x0 - 0.17, top - 0.01), (x0 - 0.24, top)], 0.04, m["poly"])
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
    cyl("Bottle", 0.022, 0.065, (0, 0, 0.0325), col(p["color"], 0, 0.3), axis="Z")
    cyl("Cap", 0.024, 0.02, (0, 0, 0.075), col(p["cap"]), axis="Z")
    cyl("Label", 0.0225, 0.03, (0, 0, 0.03), m["white"], axis="Z")


@item("jar")
def _(p, m):
    cyl("Glass", 0.035, 0.08, (0, 0, 0.04), m["glass"], axis="Z")
    cyl("Content", 0.0355, 0.05, (0, 0, 0.03), col(p["color"], 0, 0.9), axis="Z")
    cyl("Lid", 0.037, 0.015, (0, 0, 0.087), m["dark"], axis="Z")


@item("tin")
def _(p, m):
    cyl("Tin", 0.034, 0.022, (0, 0, 0.011), col(p["color"], 0.5, 0.4), axis="Z", verts=32)
    cyl("Lid", 0.035, 0.006, (0, 0, 0.022), m["white"], axis="Z", verts=32)


@item("syringe")
def _(p, m):
    cyl("Barrel", 0.008, 0.09, (0, 0, 0), m["glass"])
    cyl("Liquid", 0.0083, 0.06, (0.012, 0, 0), col(p["color"], 0, 0.2))
    cyl("Plunger", 0.003, 0.05, (-0.065, 0, 0), m["white"])
    cyl("Thumb", 0.011, 0.004, (-0.09, 0, 0), m["white"])
    box("Flange", (0.004, 0.035, 0.012), (-0.045, 0, 0), m["white"])
    cyl("Hub", 0.004, 0.012, (0.051, 0, 0), m["white"])
    cyl("Needle", 0.0008, 0.035, (0.075, 0, 0), m["chrome"])


@item("blister")
def _(p, m):
    box("Foil", (0.09, 0.05, 0.003), (0, 0, 0.0015), m["chrome"], bevel=0)
    for i in range(5):
        for j in range(2):
            sph(f"Pill{i}{j}", 0.006, (-0.032 + i * 0.016, -0.011 + j * 0.022, 0.004), col(p["color"]), scale=(1.4, 1, 0.6))


@item("dropper")
def _(p, m):
    cyl("Bottle", 0.016, 0.05, (0, 0, 0.025), col((0.3, 0.15, 0.05), 0, 0.1), axis="Z")
    cyl("Neck", 0.008, 0.012, (0, 0, 0.056), m["dark"], axis="Z")
    sph("Bulb", 0.01, (0, 0, 0.072), col(p["color"]), scale=(1, 1, 1.5))


@item("can")
def _(p, m):
    r, h = p.get("r", 0.033), p.get("h", 0.12)
    cyl("Can", r, h, (0, 0, h / 2), col(p["color"], 0.6, 0.3), axis="Z", verts=32)
    if p.get("band"):
        cyl("Band", r + 0.0005, h * 0.35, (0, 0, h / 2), col(p["band"], 0.3, 0.4), axis="Z", verts=32)
    cyl("Top", r * 0.9, 0.004, (0, 0, h + 0.001), m["chrome"], axis="Z", verts=32)


@item("vial")
def _(p, m):
    cyl("Glass", 0.012, 0.07, (0, 0, 0.035), m["glass"], axis="Z")
    cyl("Liquid", 0.0122, 0.05, (0, 0, 0.026), col(p["color"], 0, 0.1), axis="Z")
    cyl("Cap", 0.013, 0.015, (0, 0, 0.075), m["dark"], axis="Z")


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
    box("Patch", (0.06, 0.06, 0.003), (0, 0, 0.0015), col(p["color"]), bevel=0.001)
    box("Center", (0.03, 0.03, 0.004), (0, 0, 0.002), m["white"], bevel=0)


@item("canister")
def _(p, m):
    cyl("Can", 0.025, 0.12, (0, 0, 0.06), col(p["color"], 0.7, 0.3), axis="Z")
    hemi("Top", 0.025, (0, 0, 0.12), col(p["color"], 0.7, 0.3))
    cyl("Valve", 0.006, 0.02, (0, 0, 0.15), m["chrome"], axis="Z")
    box("Nozzle", (0.03, 0.008, 0.008), (0.012, 0, 0.158), m["poly"])


@item("plaster")
def _(p, m):
    box("Strip", (0.08, 0.022, 0.002), (0, 0, 0.001), col((0.85, 0.7, 0.55)), bevel=0)
    box("Pad", (0.025, 0.018, 0.003), (0, 0, 0.0015), m["white"], bevel=0)


@item("bandage")
def _(p, m):
    cyl("Roll", 0.03, 0.07, (0, 0, 0.03), m["white"], axis="Y", verts=24)
    box("Tail", (0.08, 0.07, 0.002), (0.04, 0, 0.0), m["white"], bevel=0)


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
    cone("Cup", 0.03, 0.04, 0.12, (0, 0, 0.06), m["white"])
    cyl("Sleeve", 0.037, 0.045, (0, 0, 0.065), col((0.45, 0.3, 0.18)), axis="Z")
    cyl("Lid", 0.042, 0.012, (0, 0, 0.125), m["poly"], axis="Z")


@item("bottle")
def _(p, m):
    bottle_with_cap(m, p["color"], p["cap"])


@item("carton")
def _(p, m):
    box("Carton", (0.07, 0.07, 0.19), (0, 0, 0.095), col(p["color"]), bevel=0.002)
    box("Band", (0.072, 0.072, 0.05), (0, 0, 0.1), col(p["band"]), bevel=0)
    prism("Gable", [(-0.035, 0.19), (0.035, 0.19), (0.0, 0.23)], 0.07, col(p["color"]))
    cyl("Spout", 0.01, 0.015, (0.015, 0, 0.215), m["poly"], axis="Z")


@item("baguette")
def _(p, m):
    c = col((0.78, 0.52, 0.22), 0, 0.8)
    cyl("Bread", 0.03, 0.5, (0, 0, 0.03), c, verts=16)
    sph("EndA", 0.03, (0.25, 0, 0.03), c)
    sph("EndB", 0.03, (-0.25, 0, 0.03), c)
    for i in range(5):
        box(f"Cut{i}", (0.05, 0.012, 0.01), (-0.18 + i * 0.09, 0, 0.058), col((0.9, 0.8, 0.6)), rot=(0, 0, 30), bevel=0)


@item("lunchbox")
def _(p, m):
    box("Paper", (0.16, 0.1, 0.05), (0, 0, 0.025), col((0.92, 0.9, 0.85)), bevel=0.008)
    box("String", (0.162, 0.006, 0.052), (0, 0, 0.025), col((0.6, 0.1, 0.1)), bevel=0)
    box("Brunost", (0.05, 0.05, 0.01), (0.04, 0.0, 0.055), col((0.7, 0.4, 0.15)), bevel=0.002)


@item("pizzabox")
def _(p, m):
    box("Box", (0.3, 0.3, 0.035), (0, 0, 0.0175), col((0.9, 0.9, 0.88)), bevel=0.003)
    box("Print", (0.24, 0.24, 0.002), (0, 0, 0.036), col((0.75, 0.1, 0.1)), bevel=0)
    cyl("Pizza", 0.09, 0.003, (0, 0, 0.038), col((0.95, 0.7, 0.2)), axis="Z", verts=32)


@item("kebab")
def _(p, m):
    cone("Pita", 0.045, 0.015, 0.18, (0, 0, 0.09), col((0.9, 0.75, 0.5)), verts=20)
    sph("Filling", 0.042, (0, 0, 0.18), col((0.55, 0.3, 0.15)), scale=(1, 1, 0.6))
    sph("Salad", 0.02, (0.02, 0.01, 0.2), col((0.2, 0.6, 0.15)))
    cone("Foil", 0.04, 0.012, 0.09, (0, 0, 0.045), m["chrome"], verts=20)


@item("hotdog")
def _(p, m):
    box("Lompe", (0.17, 0.08, 0.01), (0, 0, 0.005), col((0.9, 0.85, 0.7)), bevel=0.004)
    cyl("Sausage", 0.012, 0.16, (0, 0, 0.022), col((0.6, 0.25, 0.15)))
    box("Ketchup", (0.13, 0.005, 0.004), (0, 0, 0.035), col((0.8, 0.05, 0.05)), bevel=0)


@item("taco")
def _(p, m):
    pts = [(math.cos(a) * 0.07, 0.07 - math.sin(a) * 0.07) for a in [i * math.pi / 10 for i in range(11)]]
    inner = [(x * 0.85, 0.07 - (0.07 - z) * 0.85) for x, z in reversed(pts)]
    prism("Shell", pts + inner, 0.08, col((0.95, 0.75, 0.3)))
    box("Meat", (0.1, 0.06, 0.03), (0, 0, 0.04), col((0.45, 0.25, 0.12)))
    box("Salad", (0.1, 0.06, 0.012), (0, 0, 0.06), col((0.3, 0.7, 0.2)))
    box("Cheese", (0.09, 0.05, 0.008), (0, 0, 0.068), col((1.0, 0.8, 0.2)))


@item("bun")
def _(p, m):
    sph("Bun", 0.05, (0, 0, 0.03), col(p["color"], 0, 0.7), scale=(1, 1, 0.6))
    if p.get("top"):
        cyl("Glaze", 0.035, 0.006, (0, 0, 0.058), m["white"], axis="Z")
        sph("Cream", 0.017, (0, 0, 0.062), col((1.0, 0.85, 0.3)), scale=(1, 1, 0.6))
        sph("Coco", 0.03, (0, 0, 0.059), col((0.95, 0.95, 0.9)), scale=(1, 1, 0.12))
    if p.get("swirl"):
        for i in range(3):
            cyl(f"Swirl{i}", 0.044 - i * 0.013, 0.004, (0, 0, 0.058 + i * 0.003), col((0.45, 0.25, 0.1)), axis="Z")


@item("waffle")
def _(p, m):
    c = col((0.9, 0.65, 0.3), 0, 0.7)
    for i in range(5):
        a = i * 2 * math.pi / 5
        cyl(f"Heart{i}", 0.045, 0.012, (math.cos(a) * 0.04, math.sin(a) * 0.04, 0.006), c, axis="Z")
    sph("Jam", 0.025, (0, 0, 0.014), col((0.7, 0.05, 0.1)), scale=(1, 1, 0.3))


@item("bar")
def _(p, m):
    box("Wrapper", (0.13, 0.045, 0.018), (0, 0, 0.009), col(p["color"]), bevel=0.003)
    box("Stripe", (0.131, 0.012, 0.019), (0, 0, 0.009), m["white"], bevel=0)


@item("apple")
def _(p, m):
    sph("Apple", 0.04, (0, 0, 0.04), col((0.7, 0.05, 0.05), 0, 0.3), scale=(1, 1, 0.9))
    cyl("Stem", 0.002, 0.02, (0, 0, 0.082), m["wood"], axis="Z")
    box("Leaf", (0.02, 0.01, 0.002), (0.01, 0, 0.088), col((0.2, 0.6, 0.1)))


@item("banana")
def _(p, m):
    c = col((0.95, 0.8, 0.1), 0, 0.6)
    for i in range(8):
        a = math.radians(-50 + i * 14)
        rotated(cyl(f"Seg{i}", 0.017 - abs(i - 3.5) * 0.0012, 0.03, (math.sin(a) * 0.11, 0, 0.11 - math.cos(a) * 0.11), c, axis="X", verts=6), (0, -math.degrees(a), 0))
    cyl("Tip", 0.005, 0.02, (0.09, 0, 0.05), m["bake"])


@item("noodlecup")
def _(p, m):
    cone("Cup", 0.035, 0.048, 0.09, (0, 0, 0.045), col((0.95, 0.9, 0.8)))
    cyl("Band", 0.045, 0.03, (0, 0, 0.06), col((0.9, 0.2, 0.1)), axis="Z")
    cyl("Lid", 0.049, 0.004, (0, 0, 0.092), m["chrome"], axis="Z")


@item("salmon")
def _(p, m):
    prism("Fillet", [(-0.1, 0.0), (0.1, 0.0), (0.12, 0.012), (0.08, 0.025), (-0.1, 0.02)], 0.07, col((0.95, 0.45, 0.3), 0, 0.3))
    for i in range(6):
        box(f"Fat{i}", (0.004, 0.072, 0.02), (-0.08 + i * 0.03, 0, 0.012), col((0.98, 0.8, 0.7)), rot=(0, 20, 0), bevel=0)
    box("Pack", (0.25, 0.09, 0.005), (0, 0, -0.0025), col((0.1, 0.2, 0.5)), bevel=0)


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
    prism("Cheese", [(-0.07, 0.0), (0.07, 0.0), (0.07, 0.07), (0.02, 0.09), (-0.07, 0.09)], 0.08, col((0.72, 0.4, 0.15), 0, 0.4))
    box("Slicer", (0.16, 0.03, 0.004), (0.08, 0.0, 0.095), m["chrome"], rot=(0, -15, 0))


@item("grenade")
def _(p, m):
    k = p["kind"]
    if k == "frag":
        sph("Body", 0.032, (0, 0, 0.04), m["green"], scale=(1, 1, 1.2))
    else:
        cyl("Body", 0.03, 0.11, (0, 0, 0.055), m["dark"] if k == "flash" else col((0.5, 0.5, 0.5)), axis="Z")
        if k == "flash":
            for i in range(6):
                a = i * math.pi / 3
                cyl(f"Hole{i}", 0.006, 0.01, (math.cos(a) * 0.029, math.sin(a) * 0.029, 0.07), m["poly"], axis="Z")
        else:
            cyl("Stripe", 0.031, 0.015, (0, 0, 0.08), col((0.9, 0.85, 0.1)), axis="Z")
    top = 0.085 if k == "frag" else 0.115
    cyl("Fuse", 0.011, 0.02, (0, 0, top), m["steel"], axis="Z")
    box("Spoon", (0.01, 0.014, 0.07), (0.033, 0, top - 0.035), m["steel"], rot=(0, 8, 0))
    cyl("Ring", 0.012, 0.002, (-0.015, 0, top + 0.012), m["steel"], axis="Y")


@item("molotov")
def _(p, m):
    bottle_with_cap(m, (0.25, 0.45, 0.15), (0.9, 0.85, 0.7), r=0.03, h=0.24)
    box("Rag", (0.02, 0.03, 0.08), (0.01, 0, 0.27), col((0.8, 0.75, 0.6)), rot=(0, 15, 0))


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
    prism("Body", [(-0.11, 0.0), (0.11, 0.0), (0.1, 0.09), (-0.1, 0.09)], 0.035, m["green"])
    box("Front", (0.2, 0.002, 0.07), (0, -0.018, 0.045), col((0.25, 0.28, 0.15)), bevel=0)
    for s in (-1, 1):
        for t in (-1, 1):
            rotated(cyl(f"Leg{s}{t}", 0.003, 0.07, (s * 0.07, t * 0.02, -0.02), m["steel"], axis="Z"), (t * 25, 0, 0))
    cyl("Wire", 0.0008, 0.5, (0.35, 0, 0.02), m["chrome"])


@item("binoculars")
def _(p, m):
    for s in (-1, 1):
        cyl(f"Tube{s}", 0.025, 0.12, (0, s * 0.035, 0), m["poly"])
        cyl(f"Lens{s}", 0.021, 0.122, (0, s * 0.035, 0), m["lens"])
        cyl(f"Eye{s}", 0.018, 0.03, (-0.07, s * 0.033, 0), m["rubber"])
    box("Bridge", (0.05, 0.04, 0.02), (-0.01, 0, 0), m["dark"])
    cyl("Focus", 0.008, 0.03, (-0.02, 0, 0.015), m["dark"], axis="Y")


@item("goggles")
def _(p, m):
    box("Housing", (0.05, 0.08, 0.04), (0, 0, 0), m["green"], bevel=0.008)
    for s in (-1, 1):
        cyl(f"Tube{s}", 0.017, 0.06, (0.045, s * 0.03, 0), m["green"])
        cyl(f"Lens{s}", 0.014, 0.062, (0.045, s * 0.03, 0), col((0.1, 0.6, 0.2), 0.5, 0.05))
    box("Mount", (0.03, 0.03, 0.05), (-0.01, 0, 0.04), m["poly"])
    box("Strap", (0.01, 0.2, 0.02), (-0.04, 0, 0.0), m["fabric"])


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
    box("Body", (0.035, 0.06, 0.13), (0, 0, 0.065), m["poly"], bevel=0.006)
    cyl("Antenna", 0.005, 0.12, (0, 0.015, 0.19), m["rubber"], axis="Z")
    cyl("Knob", 0.006, 0.012, (0, -0.015, 0.136), m["dark"], axis="Z")
    box("Screen", (0.002, 0.04, 0.025), (0.018, 0, 0.1), col((0.3, 0.6, 0.3), 0, 0.1), bevel=0)
    box("Grill", (0.002, 0.045, 0.04), (0.018, 0, 0.045), m["dark"], bevel=0)


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
    cyl("Can", 0.018, 0.17, (0, 0, 0), m["poly"])
    cyl("Mount", 0.012, 0.03, (-0.095, 0, 0), m["dark"])


@item("reddot")
def _(p, m):
    box("Base", (0.05, 0.03, 0.012), (0, 0, 0.006), m["dark"])
    cyl("Tube", 0.017, 0.055, (0, 0, 0.032), m["poly"])
    cyl("Lens", 0.014, 0.057, (0, 0, 0.032), m["lens"])


@item("scope")
def _(p, m):
    cyl("Tube", 0.014, 0.26, (0, 0, 0.04), m["poly"])
    cone("Front", 0.014, 0.024, 0.07, (0.16, 0, 0.04), m["poly"], axis="X")
    cone("Rear", 0.021, 0.014, 0.05, (-0.145, 0, 0.04), m["poly"], axis="X")
    for x in (-0.06, 0.06):
        box(f"Ring{x}", (0.015, 0.03, 0.035), (x, 0, 0.0175), m["dark"])
    cyl("Turret", 0.01, 0.02, (0, 0, 0.06), m["poly"], axis="Z")


@item("extmag")
def _(p, m):
    for i in range(6):
        ang = 4 + i * 5
        box(f"Mag{i}", (0.038, 0.026, 0.05), (math.sin(math.radians(ang)) * 0.05 * i, 0, -0.045 * i), m["dark"], rot=(0, -ang, 0))
    box("Base", (0.045, 0.03, 0.012), (0.16, 0, -0.27), m["poly"], rot=(0, -30, 0))


@item("laser")
def _(p, m):
    box("Body", (0.06, 0.03, 0.03), (0, 0, 0.015), m["poly"], bevel=0.004)
    cyl("Emitter", 0.006, 0.01, (0.033, 0.006, 0.015), m["red"])
    cyl("Flash", 0.009, 0.01, (0.033, -0.006, 0.015), m["glass"])


@item("compass")
def _(p, m):
    cyl("Case", 0.03, 0.012, (0, 0, 0.006), m["brass"], axis="Z", verts=32)
    cyl("Face", 0.027, 0.013, (0, 0, 0.0065), m["white"], axis="Z", verts=32)
    box("NeedleN", (0.022, 0.004, 0.002), (0.011, 0, 0.0135), m["red"], bevel=0)
    box("NeedleS", (0.022, 0.004, 0.002), (-0.011, 0, 0.0135), m["dark"], bevel=0)


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
    box("Base", (0.1, 0.1, 0.04), (0, 0, 0.02), m["dark"], bevel=0.004)
    cyl("Stem", 0.012, 0.08, (0, 0, 0.08), g, axis="Z")
    cone("Cup", 0.02, 0.07, 0.12, (0, 0, 0.18), g, verts=32)
    for s in (-1, 1):
        cyl(f"Handle{s}", 0.025, 0.008, (0, s * 0.075, 0.18), g, axis="Y")




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
    box("Base", (0.07, 0.035, 0.012), (0, 0, 0.006), m["dark"])
    box("Body", (0.045, 0.04, 0.03), (-0.012, 0, 0.027), m["poly"], bevel=0.004)
    box("Hood", (0.012, 0.046, 0.045), (0.028, 0, 0.035), m["poly"], bevel=0.003)
    box("Window", (0.003, 0.034, 0.03), (0.028, 0, 0.037), m["lens"], bevel=0)
    box("Reticle", (0.0035, 0.004, 0.004), (0.028, 0, 0.04), col((1, 0.1, 0.1)), bevel=0)


@item("scope8")
def _(p, m):
    cyl("Tube", 0.016, 0.36, (0, 0, 0.05), m["poly"])
    cone("Objective", 0.016, 0.03, 0.09, (0.22, 0, 0.05), m["poly"], axis="X")
    cyl("Ocular", 0.02, 0.06, (-0.2, 0, 0.05), m["poly"])
    cyl("Turret", 0.01, 0.03, (0.0, 0, 0.072), m["dark"], axis="Z")
    cyl("TurretSide", 0.01, 0.03, (0.0, 0.026, 0.05), m["dark"], axis="Y")
    for x in (-0.08, 0.08):
        box(f"Ring{x}", (0.02, 0.04, 0.045), (x, 0, 0.03), m["dark"])
    cyl("Lens", 0.028, 0.002, (0.266, 0, 0.05), m["lens"])


@item("drum")
def _(p, m):
    cyl("Drum", 0.075, 0.05, (0, 0, 0.0), m["dark"], axis="Y", verts=32)
    cyl("Cap", 0.03, 0.054, (0, 0, 0.0), m["steel"], axis="Y", verts=24)
    box("Neck", (0.04, 0.026, 0.08), (0.02, 0, 0.09), m["dark"])


@item("comp")
def _(p, m):
    cyl("Body", 0.013, 0.06, (0, 0, 0.013), m["dark"], verts=16)
    for i in range(3):
        box(f"Port{i}", (0.006, 0.02, 0.006), (-0.012 + i * 0.012, 0, 0.026), m["steel"], bevel=0)


@item("vgrip")
def _(p, m):
    box("Clamp", (0.05, 0.03, 0.015), (0, 0, 0.1), m["dark"])
    cyl("Grip", 0.015, 0.1, (0, 0, 0.045), m["poly"], axis="Z", verts=16)
    for i in range(4):
        cyl(f"Ridge{i}", 0.0155, 0.004, (0, 0, 0.015 + i * 0.02), m["rubber"], axis="Z", verts=16)


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
                       pistol=build_pistol, revolver=build_revolver, pistolknife=build_pistolknife, rifle=build_rifle,
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
