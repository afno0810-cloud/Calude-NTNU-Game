"""Render a labelled grid of models:  python contact_sheet.py out.png SM_A SM_B ...  (from SourceArt/Models/*.blend)"""
import math
import os
import sys

import bpy
from mathutils import Vector

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
out, names = sys.argv[1], sys.argv[2:]
cols = int(os.environ.get("COLS", "5"))
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
cell = 1.3
for i, n in enumerate(names):
    path = os.path.join(ROOT, "SourceArt", "Models", n + ".blend")
    with bpy.data.libraries.load(path) as (src, dst):
        dst.objects = [o for o in src.objects]
    obj = [o for o in dst.objects if o and o.type == "MESH"][0]
    scene.collection.objects.link(obj)
    pts = [Vector(c) for c in obj.bound_box]
    lo = Vector([min(p[k] for p in pts) for k in range(3)])
    hi = Vector([max(p[k] for p in pts) for k in range(3)])
    size = max(hi.x - lo.x, hi.z - lo.z, (hi.y - lo.y) * 0.8)
    s = 1.0 / size
    obj.scale = (s, s, s)
    cx, cz = i % cols, i // cols
    centre = (lo + hi) / 2
    obj.location = Vector((cx * cell, 0, -cz * cell)) - centre * s
    if (hi.y - lo.y) > (hi.x - lo.x) * 1.3 and (hi.z - lo.z) < (hi.y - lo.y):
        pass
    bpy.ops.object.text_add(location=(cx * cell, 0, -cz * cell - 0.6))
    t = bpy.context.active_object
    t.data.body = n.replace("SM_", "")
    t.data.size = 0.11
    t.data.align_x = "CENTER"
    t.rotation_euler = (math.radians(90), 0, 0)
    mat = bpy.data.materials.new("Txt")
    mat.diffuse_color = (0, 0, 0, 1)
    mat.use_nodes = True
    mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.02, 0.02, 0.02, 1)
    t.data.materials.append(mat)

rows = math.ceil(len(names) / cols)
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 16
scene.cycles.use_denoising = False
for d, e in (((60, 0, 30), 4.0), ((-40, 0, -150), 2.0), ((70, 0, 200), 2.5)):
    l = bpy.data.objects.new("L", bpy.data.lights.new("L", "SUN"))
    l.data.energy = e
    l.rotation_euler = [math.radians(a) for a in d]
    scene.collection.objects.link(l)
scene.world = bpy.data.worlds.new("W")
scene.world.use_nodes = True
scene.world.node_tree.nodes["Background"].inputs[0].default_value = (0.72, 0.75, 0.78, 1)
w, h = cols * cell, rows * cell
scene.render.resolution_x = int(cols * 300)
scene.render.resolution_y = int(rows * 300)
cam_data = bpy.data.cameras.new("Cam")
cam_data.type = "ORTHO"
cam_data.ortho_scale = max(w, h)
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
center = Vector(((cols - 1) * cell / 2, 0, -(rows - 1) * cell / 2 - 0.05))
cam.location = center + Vector((0, -20, 3.5))
cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()
scene.render.filepath = out
bpy.ops.render.render(write_still=True)
