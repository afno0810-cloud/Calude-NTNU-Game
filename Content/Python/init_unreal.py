"""Runs automatically every time the Unreal editor opens this project.

Imports (or re-imports) every model in SourceArt/Models/*.fbx into /Game/Models,
so new models from Claude appear in the game without any clicking.
"""
import os
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceArt", "Models")
DEST = "/Game/Models"


def _needs_import(fbx_path, asset_path):
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return True
    stamp = fbx_path + ".imported"
    if not os.path.exists(stamp):
        return True
    return os.path.getmtime(fbx_path) > os.path.getmtime(stamp)


def import_models():
    if not os.path.isdir(SOURCE_DIR):
        return
    tasks = []
    for name in sorted(os.listdir(SOURCE_DIR)):
        if not name.lower().endswith(".fbx"):
            continue
        fbx = os.path.join(SOURCE_DIR, name)
        asset_name = os.path.splitext(name)[0]
        if not _needs_import(fbx, f"{DEST}/{asset_name}"):
            continue

        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_as_skeletal = False
        options.import_materials = True
        options.import_textures = True
        options.import_animations = False
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.generate_lightmap_u_vs = False
        options.static_mesh_import_data.auto_generate_collision = True

        task = unreal.AssetImportTask()
        task.filename = fbx
        task.destination_path = DEST
        task.destination_name = asset_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        task.options = options
        tasks.append((task, fbx))

    if not tasks:
        return
    unreal.log(f"Gloshaugen: importerer {len(tasks)} modell(er) fra SourceArt/Models")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for t, _ in tasks])
    for task, fbx in tasks:
        with open(fbx + ".imported", "w") as f:
            f.write("ok")


try:
    import_models()
except Exception as e:  # never block the editor from opening
    unreal.log_warning(f"Gloshaugen: modell-import feilet: {e}")
