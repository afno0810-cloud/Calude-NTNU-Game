"""Render a labelled grid of models:  python contact_sheet.py out.png SM_A SM_B ...  (from SourceArt/Models/*.blend)

TEXTURED=1 renders with the same PBR textures and tints the game uses (from Content/Python/gs_materials.py),
so the preview shows roughly what the item looks like in Unreal.
"""
import math
import os
import re
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
    if os.environ.get("TILT"):
        # optional 3/4 view for flat items: tip the top toward the camera, then turn a little
        from mathutils import Matrix
        obj.data = obj.data.copy()
        obj.data.transform(Matrix.Rotation(math.radians(float(os.environ.get("YAW", "25"))), 4, "Z"))
        obj.data.transform(Matrix.Rotation(math.radians(float(os.environ["TILT"])), 4, "X"))
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



def _instances():
    src = open(os.path.join(ROOT, "Content", "Python", "gs_materials.py")).read()
    out = {}
    for m in re.finditer(r'"(\w+)": \("(\w+)", \(([^)]*)\), ([\d.]+), ([\d.]+), ([\d.]+)', src):
        tint = tuple(float(v) for v in m.group(3).split(","))
        out[m.group(1)] = (m.group(2), tint, float(m.group(4)), float(m.group(5)), float(m.group(6)))
    return out


def _textured(mat, spec):
    texset, tint, tiling, rough, metal = spec
    tex_dir = os.path.join(ROOT, "SourceArt", "Textures")
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    uv = nt.nodes.new("ShaderNodeTexCoord")
    mp = nt.nodes.new("ShaderNodeMapping")
    mp.inputs["Scale"].default_value = (tiling, tiling, tiling)
    nt.links.new(uv.outputs["UV"], mp.inputs[0])

    def img(suffix, colour):
        path = os.path.join(tex_dir, f"T_GS_{texset}{suffix}.jpg")
        if not os.path.exists(path):
            return None
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(path, check_existing=True)
        n.image.colorspace_settings.name = "sRGB" if colour else "Non-Color"
        nt.links.new(mp.outputs[0], n.inputs[0])
        return n

    d, arm, nrm = img("_D", True), img("_ARM", False), img("_N", False)
    if d:
        mix = nt.nodes.new("ShaderNodeMixRGB")
        mix.blend_type = "MULTIPLY"
        mix.inputs[0].default_value = 1.0
        mix.inputs[2].default_value = (*tint, 1)
        nt.links.new(d.outputs[0], mix.inputs[1])
        nt.links.new(mix.outputs[0], bsdf.inputs["Base Color"])
    if arm:
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        nt.links.new(arm.outputs[0], sep.inputs[0])
        r = nt.nodes.new("ShaderNodeMath")
        r.operation = "MULTIPLY"
        r.inputs[1].default_value = rough
        nt.links.new(sep.outputs[1], r.inputs[0])
        nt.links.new(r.outputs[0], bsdf.inputs["Roughness"])
        mt = nt.nodes.new("ShaderNodeMath")
        mt.operation = "MULTIPLY"
        mt.inputs[1].default_value = metal
        nt.links.new(sep.outputs[2], mt.inputs[0])
        nt.links.new(mt.outputs[0], bsdf.inputs["Metallic"])
    if nrm:
        # our normal maps are DirectX (green down); Blender wants OpenGL
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        nt.links.new(nrm.outputs[0], sep.inputs[0])
        inv = nt.nodes.new("ShaderNodeMath")
        inv.operation = "SUBTRACT"
        inv.inputs[0].default_value = 1.0
        nt.links.new(sep.outputs[1], inv.inputs[1])
        comb = nt.nodes.new("ShaderNodeCombineColor")
        nt.links.new(sep.outputs[0], comb.inputs[0])
        nt.links.new(inv.outputs[0], comb.inputs[1])
        nt.links.new(sep.outputs[2], comb.inputs[2])
        nm = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(comb.outputs[0], nm.inputs["Color"])
        nt.links.new(nm.outputs[0], bsdf.inputs["Normal"])


if os.environ.get("TEXTURED"):
    specs = _instances()
    for mat in list(bpy.data.materials):
        base = mat.name.split(".")[0]
        if base.endswith("_In"):   # indoor slots use the same textures as the outdoor material
            base = base[:-3]
        spec = specs.get(base)
        cm = re.match(r"^M_C_(\d\d)(\d\d)(\d\d)_(\d)$", base)
        if not spec and cm:
            r, g, b, met = (int(x) for x in cm.groups())
            metallic = met >= 5
            bv = 0.56 if metallic else 0.75
            spec = ("Metal" if metallic else "Plastic", (r / 99 / bv, g / 99 / bv, b / 99 / bv), 1.2,
                    0.8 if metallic else 1.0, 1.0 if metallic else 0.0)
        if spec and mat.use_nodes:
            _textured(mat, spec)

rows = math.ceil(len(names) / cols)
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = int(os.environ.get("SAMPLES", "16"))
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
scene.render.resolution_x = int(cols * int(os.environ.get("PX", "300")))
scene.render.resolution_y = int(rows * int(os.environ.get("PX", "300")))
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
