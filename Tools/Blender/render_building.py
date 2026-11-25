"""Perspective preview of a building blend: python render_building.py in.blend out.png camX camY camZ tgtX tgtY tgtZ [lens]
Coordinates are Blender space (Y is mirrored vs Unreal: Unreal north = Blender +Y)."""
import sys, math, bpy
from mathutils import Vector
a = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
blend, out = a[0], a[1]
cam_p, tgt = Vector(map(float, a[2:5])), Vector(map(float, a[5:8]))
lens = float(a[8]) if len(a) > 8 else 28
bpy.ops.wm.open_mainfile(filepath=blend)
sc = bpy.context.scene
sc.render.engine = "CYCLES"; sc.cycles.device = "CPU"; sc.cycles.samples = 16; sc.cycles.use_denoising = False
sc.render.resolution_x, sc.render.resolution_y = 1400, 850
for d, e in (((50, 10, 140), 3.5), ((-30, 0, -40), 0.8)):
    l = bpy.data.objects.new("L", bpy.data.lights.new("L", "SUN")); l.data.energy = e
    l.rotation_euler = [math.radians(x) for x in d]; sc.collection.objects.link(l)
sc.world = bpy.data.worlds.new("W"); sc.world.use_nodes = True
bg = sc.world.node_tree.nodes["Background"]; bg.inputs[0].default_value = (0.6, 0.7, 0.85, 1); bg.inputs[1].default_value = 0.9
g = bpy.data.meshes.new("G"); g.from_pydata([(-400,-400,-0.05),(400,-400,-0.05),(400,400,-0.05),(-400,400,-0.05)], [], [(0,1,2,3)])
go = bpy.data.objects.new("G", g); sc.collection.objects.link(go)
gm = bpy.data.materials.new("Grass"); gm.use_nodes = True
gm.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.2, 0.3, 0.15, 1); g.materials.append(gm)
cd = bpy.data.cameras.new("C"); cd.lens = lens; cd.clip_end = 2000
c = bpy.data.objects.new("C", cd); sc.collection.objects.link(c); sc.camera = c
c.location = cam_p; c.rotation_euler = (tgt - cam_p).to_track_quat("-Z", "Y").to_euler()
sc.render.filepath = out; bpy.ops.render.render(write_still=True)
