"""Render a quick side + 3/4 preview PNG of a model: python render_preview.py SourceArt/Models/SM_AG3.blend out.png"""
import sys, math, bpy
from mathutils import Vector

blend, out = sys.argv[-2], sys.argv[-1]
bpy.ops.wm.open_mainfile(filepath=blend)
scene = bpy.context.scene
obj = [o for o in scene.objects if o.type == "MESH"][0]
pts = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
center = sum(pts, Vector()) / 8
size = max((max(p[i] for p in pts) - min(p[i] for p in pts)) for i in range(3))

scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 24
scene.cycles.use_denoising = False
for d, e in (((60, 0, 30), 4.0), ((-40, 0, -150), 2.0), ((70, 0, 200), 2.5)):
    l = bpy.data.objects.new("L", bpy.data.lights.new("L", "SUN"))
    l.data.energy = e
    l.rotation_euler = [math.radians(a) for a in d]
    scene.collection.objects.link(l)
scene.render.resolution_x, scene.render.resolution_y = 1400, 700
scene.render.film_transparent = False
scene.world = bpy.data.worlds.new("W")
scene.world.use_nodes = True
scene.world.node_tree.nodes["Background"].inputs[0].default_value = (0.55, 0.6, 0.65, 1)
scene.world.node_tree.nodes["Background"].inputs[1].default_value = 0.8

view = sys.argv[-3] if len(sys.argv) > 3 and sys.argv[-3] in ("side", "34") else "side"
cam_data = bpy.data.cameras.new("Cam")
cam_data.type = "ORTHO"
cam_data.ortho_scale = size * (1.15 if view == "side" else 1.5)
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
if view == "side":
    cam.location = center + Vector((0, -size * 2, 0))
else:
    cam.location = center + Vector((size * 0.9, -size * 1.4, size * 0.7))
direction = center - cam.location
cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
scene.render.filepath = out
bpy.ops.render.render(write_still=True)
