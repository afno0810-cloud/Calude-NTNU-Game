"""Shared helpers for the Gloshaugen Strike Blender model scripts.

Run a model script with Blender (no UI needed):
    blender --background --python Tools/Blender/weapon_ag3.py
or with the `bpy` Python module:
    python Tools/Blender/weapon_ag3.py

Conventions: meters, +X = forward (muzzle), +Z = up, origin at the grip.
"""
import math
import os

import bpy
import bmesh

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_DIR = os.path.join(ROOT, "SourceArt", "Models")


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    _materials.clear()
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0


_materials = {}


def material(name, color, metallic=0.0, roughness=0.6):
    """Simple PBR material. Color is (r, g, b) in 0-1."""
    if name in _materials:
        return _materials[name]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (*color, 1.0)
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
    mat.diffuse_color = (*color, 1.0)
    _materials[name] = mat
    return mat


def _finish(obj, mat, bevel=0.0, segments=2):
    if mat:
        obj.data.materials.append(mat)
    if bevel > 0:
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = segments
        mod.limit_method = "ANGLE"
    return obj


def box(name, size, loc, mat=None, rot=(0, 0, 0), bevel=0.002):
    """Box with size (x, y, z) centered at loc. rot in degrees."""
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=[math.radians(r) for r in rot])
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return _finish(obj, mat, bevel)


def cyl(name, radius, length, loc, mat=None, axis="X", verts=24, bevel=0.0):
    """Cylinder along an axis, centered at loc."""
    rot = {"X": (0, math.radians(90), 0), "Y": (math.radians(90), 0, 0), "Z": (0, 0, 0)}[axis]
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=length, location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    return _finish(obj, mat, bevel)


def prism(name, profile_xz, width, mat=None, y=0.0, bevel=0.002):
    """Extrude a 2D side profile (list of (x, z) points, counter-clockwise) along Y. Good for stocks, grips, magazines."""
    mesh = bpy.data.meshes.new(name)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    bm = bmesh.new()
    half = width / 2
    front = [bm.verts.new((x, y - half, z)) for x, z in profile_xz]
    back = [bm.verts.new((x, y + half, z)) for x, z in profile_xz]
    bm.faces.new(front[::-1])
    bm.faces.new(back)
    n = len(profile_xz)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((front[i], front[j], back[j], back[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    return _finish(obj, mat, bevel)


def join_all(name):
    """Apply modifiers and join everything into one object."""
    bpy.ops.object.select_all(action="DESELECT")
    objs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    for o in objs:
        bpy.context.view_layer.objects.active = o
        for m in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=m.name)
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = name
    obj.data.name = name
    # Origin = world origin (the grip), so Unreal and previews agree on the pivot.
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.shade_smooth()
    # Keep hard edges crisp
    try:
        bpy.ops.object.shade_auto_smooth(angle=math.radians(35))
    except Exception:
        pass
    return obj


def export(name, also_blend=True, also_glb=True):
    """Export the scene as FBX (for Unreal) and .glb (for quick previews)."""
    os.makedirs(OUT_DIR, exist_ok=True)
    fbx = os.path.join(OUT_DIR, f"{name}.fbx")
    bpy.ops.export_scene.fbx(
        filepath=fbx,
        use_selection=False,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="X",
        axis_up="Z",
        mesh_smooth_type="FACE",
        add_leaf_bones=False,
        bake_space_transform=True,
    )
    if also_glb:
        glb = os.path.join(OUT_DIR, f"{name}.glb")
        bpy.ops.export_scene.gltf(filepath=glb, export_format="GLB")
    if also_blend:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, f"{name}.blend"))
    print(f"Exported {fbx}")
    return fbx
