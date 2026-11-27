"""Renders the Helgasetr model parts for the controller (called by building_helgasetr.py --render, or on its own):

    blender --background --python Tools/Blender/render_helgasetr.py

Loads every SourceArt/Models/<part>.blend, puts the ring lamps where the data has them, a low sun from the south and
writes .claude/lag/grafikk/kontroll/r04b-*.png: plan sections of Plan 1-4 seen from above and four views inside
(compare with SourceArt/Reference/Helgasetr bilde 01, 06, 22, 26). Camera positions are given in game meters.
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import helgasetr_plan as H  # noqa: E402
from gs_common import OUT_DIR  # noqa: E402

ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
DEST = os.path.join(ROOT, ".claude", "lag", "grafikk", "kontroll")


def g2b(p):
    """Game meters (Y south) -> Blender (the FBX/.blend is mirrored in Y)."""
    return Vector((p[0], -p[1], p[2]))


def load_parts(names):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    lamp = None
    for name in names:
        path = os.path.join(OUT_DIR, name + ".blend")
        if not os.path.exists(path):
            continue
        with bpy.data.libraries.load(path, link=False) as (src, dst):
            dst.objects = [n for n in src.objects if n == name]
        for o in dst.objects:
            if o is None:
                continue
            if name == "SM_Prop_Helg_Hengelampe":
                lamp = o
                continue
            bpy.context.scene.collection.objects.link(o)
            if name.startswith("SM_Glass_"):
                o.visible_shadow = False
    if lamp is not None:
        for (x, y, z, kind, r, _shadow) in H.make_plan()["lights"]:
            if kind == 0:
                c = lamp.copy()
                c.location = g2b((x, y, z))
                c.scale = (r / 1.2,) * 3
                bpy.context.scene.collection.objects.link(c)
    for m in bpy.data.materials:
        if not m.use_nodes:
            continue
        bsdf = m.node_tree.nodes.get("Principled BSDF")
        if not bsdf:
            continue
        if m.name.startswith("Glass"):
            bsdf.inputs["Alpha"].default_value = 0.12
            try:
                m.surface_render_method = "BLENDED"
            except Exception:
                pass
        if m.name.startswith("LightPanel"):
            bsdf.inputs["Emission Color"].default_value = (1.0, 0.95, 0.85, 1.0)
            bsdf.inputs["Emission Strength"].default_value = 8.0


def setup_scene():
    sc = bpy.context.scene
    try:
        sc.render.engine = "BLENDER_EEVEE"
    except Exception:
        sc.render.engine = "BLENDER_EEVEE_NEXT"
    sc.render.resolution_x, sc.render.resolution_y = 1280, 720
    try:
        sc.eevee.taa_render_samples = 32
        sc.eevee.use_raytracing = True
    except Exception:
        pass
    world = bpy.data.worlds.new("Sky")
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    bg.inputs["Color"].default_value = (0.55, 0.68, 0.85, 1.0)
    bg.inputs["Strength"].default_value = 1.2
    sc.world = world
    sun_data = bpy.data.lights.new("Sun", "SUN")
    sun_data.energy = 4.0
    sun_data.angle = math.radians(0.6)
    sun = bpy.data.objects.new("Sun", sun_data)
    # the sun from the south-south-west, 32 degrees up (a Trondheim afternoon); light travels north and down
    d = Vector((0.35, 1.0, -math.tan(math.radians(32)))).normalized()   # Blender: +Y = game north
    sun.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    sc.collection.objects.link(sun)
    sc.view_settings.view_transform = "AgX"
    return sc


def camera(sc, name, pos, look, lens=18.0, ortho=None):
    cam_data = bpy.data.cameras.new(name)
    cam = bpy.data.objects.new(name, cam_data)
    sc.collection.objects.link(cam)
    p, t = g2b(pos), g2b(look)
    cam.location = p
    cam.rotation_euler = (t - p).to_track_quat("-Z", "Y").to_euler()
    if ortho:
        cam_data.type = "ORTHO"
        cam_data.ortho_scale = ortho
        cam_data.clip_start = 0.05
    else:
        cam_data.lens = lens
        cam_data.clip_start = 0.05
    cam_data.clip_end = 400.0
    sc.camera = cam
    return cam


def shot(sc, name):
    os.makedirs(DEST, exist_ok=True)
    sc.render.filepath = os.path.join(DEST, name + ".png")
    bpy.ops.render.render(write_still=True)
    print("Rendret", sc.render.filepath)


def render_all(names):
    load_parts(names)
    sc = setup_scene()
    for lv in (H.P1, H.P2, H.P3, H.P4):
        z = H.LEVELS[lv] + 2.4
        camera(sc, "plan%d" % (lv + 1), (0.0, 1.8, z), (0.0, 1.8, z - 10.0), ortho=72.0)
        sc.render.resolution_x, sc.render.resolution_y = 1100, 1400
        shot(sc, "r04b-plan%d" % (lv + 1))
    sc.render.resolution_x, sc.render.resolution_y = 1280, 720
    z4 = H.LEVELS[H.P4] + 1.65
    views = [
        ("r04b-p4-nord-mot-sor", (9.5, -18.9, z4), (-2.0, 8.0, z4 - 3.0), 16.0),
        ("r04b-mesanin-mot-kantina", (2.0, 16.0, H.LEVELS[H.P2] + 1.65), (0.0, -10.0, 0.5), 16.0),
        ("r04b-p4-vestkorridor", (-14.4, 17.0, z4), (-14.2, -17.0, z4), 22.0),
        ("r04b-kantina-mot-taket", (2.0, 4.0, 1.65), (0.0, -6.0, 24.0), 14.0),
        ("r04b-atriet-ovenfra", (-12.0, 16.5, 21.0), (2.0, -6.0, 3.0), 14.0),
    ]
    for name, pos, look, lens in views:
        camera(sc, name, pos, look, lens=lens)
        shot(sc, name)


if __name__ == "__main__":
    import building_helgasetr as B   # noqa: E402  (only for the part list; it builds nothing on import)
    render_all([n for n, _ in B.PARTS] + ["SM_Prop_Helg_Hengelampe"])
