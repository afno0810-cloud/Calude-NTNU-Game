"""Imports SourceArt/Sounds/S_GS_*.wav into /Game/Sounds (called from init_unreal.py).

The game looks sounds up by name (GSAudio.cpp), so a recorded sound imported as /Game/Sounds/S_GS_Shot_Rifle
replaces ours; a newer .wav from the generator is only re-imported when the file changed.
"""
import os
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SND_DIR = os.path.join(PROJECT_DIR, "SourceArt", "Sounds")
DEST = "/Game/Sounds"
eal = unreal.EditorAssetLibrary


def run():
    if not os.path.isdir(SND_DIR):
        return
    tasks = []
    for fn in sorted(os.listdir(SND_DIR)):
        if not fn.lower().endswith(".wav"):
            continue
        name = os.path.splitext(fn)[0]
        path = os.path.join(SND_DIR, fn)
        stamp = path + ".imported"
        if eal.does_asset_exist(f"{DEST}/{name}") and os.path.exists(stamp) and os.path.getmtime(stamp) >= os.path.getmtime(path):
            continue
        t = unreal.AssetImportTask()
        t.filename = path
        t.destination_path = DEST
        t.destination_name = name
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append((t, path, name))
    if not tasks:
        return
    unreal.log(f"Gloshaugen: importerer {len(tasks)} lyd(er)")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for t, _, _ in tasks])
    for _, path, name in tasks:
        snd = eal.load_asset(f"{DEST}/{name}")
        if snd and name.startswith("S_GS_Amb_"):
            snd.set_editor_property("looping", True)
            eal.save_loaded_asset(snd)
        with open(path + ".imported", "w") as f:
            f.write("ok")
