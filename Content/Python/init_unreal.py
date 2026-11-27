"""Runs automatically every time the Unreal editor opens this project.

1. Imports (or re-imports) every model in SourceArt/Models/*.fbx into /Game/Models,
   so new models from Claude appear in the game without any clicking.
2. Buildings get "complex as simple" collision, so you can walk inside them.
3. Materials: our own PBR textures (SourceArt/Textures) become MI_GS_* materials, and every model gets
   them by material slot name. Scanned materials you add (Poly Haven, Megascans, Fab) win when their
   names match (see gs_materials.py).
4. Creates the map Content/Maps/Gloshaugen the first time.
"""
import os
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceArt", "Models")
DEST = "/Game/Models"

# Meshes you walk inside: exact per-triangle collision instead of convex hulls.
# (SM_Glass_Helgasetr: the fixed facade and roof glass of Helgasetr. Not the SM_Mod_Helg_ modules: they keep a simple box, K15.)
COMPLEX_COLLISION_PREFIXES = ("SM_Hovedbygningen", "SM_Bygg_", "SM_Glass_Helgasetr", "SM_Prop_BusStop", "SM_Prop_BikeRack", "SM_Prop_PicnicTable", "SM_Prop_LectureSeats")
# Meshes that get Megascans materials (they have tiling UVs from Tools/Blender/meshkit.py).
MEGASCANS_PREFIXES = ("SM_Hovedbygningen", "SM_Bygg_", "SM_Prop_")  # built as Nanite (not glass, cars or trees: they use translucent or masked materials)

def _is_building(name):
    return name.startswith(COMPLEX_COLLISION_PREFIXES)


def _needs_import(fbx_path, asset_path):
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return True
    stamp = fbx_path + ".imported"
    if not os.path.exists(stamp):
        return True
    return os.path.getmtime(fbx_path) > os.path.getmtime(stamp)


def import_models():
    if not os.path.isdir(SOURCE_DIR):
        return []
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
        options.static_mesh_import_data.generate_lightmap_u_vs = True
        options.static_mesh_import_data.auto_generate_collision = not _is_building(asset_name)
        if asset_name.startswith(MEGASCANS_PREFIXES):
            try:
                options.static_mesh_import_data.set_editor_property("build_nanite", True)
            except Exception:
                pass

        task = unreal.AssetImportTask()
        task.filename = fbx
        task.destination_path = DEST
        task.destination_name = asset_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        task.options = options
        tasks.append((task, fbx, asset_name))

    if not tasks:
        return []
    unreal.log(f"Gloshaugen: importerer {len(tasks)} modell(er) fra SourceArt/Models")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for t, _, _ in tasks])
    for task, fbx, _ in tasks:
        with open(fbx + ".imported", "w") as f:
            f.write("ok")
    return [n for _, _, n in tasks]


def fix_building_collision(names):
    for name in names:
        if not _is_building(name):
            continue
        mesh = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{name}")
        if not mesh:
            continue
        body = mesh.get_editor_property("body_setup")
        if body:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            unreal.log(f"Gloshaugen: {name} bruker presis kollisjon")


def fix_distance_fields(names):
    """Helgasetr's building parts (<= ca. 26 m each) get twice the distance-field resolution, so Lumen does not leak
    light through 15-40 cm walls and slabs (K17)."""
    try:
        sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    except Exception as e:
        unreal.log_warning(f"Gloshaugen: StaticMeshEditorSubsystem mangler, avstandsfelt ikke endret: {e}")
        return
    for name in names:
        if not name.startswith("SM_Bygg_Helgasetr_"):
            continue
        mesh = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{name}")
        if not mesh:
            continue
        try:
            settings = sub.get_lod_build_settings(mesh, 0)
            if abs(settings.get_editor_property("distance_field_resolution_scale") - 2.0) < 1e-3:
                continue
            settings.set_editor_property("distance_field_resolution_scale", 2.0)
            sub.set_lod_build_settings(mesh, 0, settings)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            unreal.log(f"Gloshaugen: {name} fikk dobbel oppløsning på avstandsfeltet")
        except Exception as e:
            unreal.log_warning(f"Gloshaugen: avstandsfelt for {name} feilet (set_lod_build_settings): {e}")


# ------------------------------------------------------------------ Fab models replace Claude's models
# Item id -> extra words to look for in mesh names (the id itself, lower case, is always tried first).
ALIASES = {
    "M1911": ["1911", "colt"], "Glock17": ["glock"], "DesertEagle": ["deagle", "desert_eagle", "deserteagle"],
    "Makarov": ["makarov"], "Revolver": ["revolver", "magnum"], "Luger": ["luger", "p08"],
    "MP5": ["mp5"], "Uzi": ["uzi"], "Thompson": ["thompson", "tommy"], "PPSh": ["ppsh"], "MP40": ["mp40"],
    "AK47": ["ak47", "ak_47", "akm", "ak-47"], "HK416": ["hk416", "m4a1", "m4", "ar15", "ar-15", "m16"],
    "AG3": ["ag3", "g3"], "FAL": ["fal"], "Garand": ["garand", "m1_rifle"], "Kar98k": ["kar98", "k98"],
    "Krag": ["krag"], "SVD": ["svd", "dragunov"], "L96": ["l96", "awm", "awp"], "Barrett": ["barrett", "m82", "m107"],
    "Pump": ["pump", "remington", "870", "mossberg"], "DoubleBarrel": ["double_barrel", "doublebarrel", "over_under", "overunder"],
    "OverUnder": ["over_under", "overunder", "o_u", "trap"], "Saiga": ["saiga"], "MG3": ["mg3", "mg42"], "Bren": ["bren"],
    "PKM": ["pkm", "pkp"], "Minimi": ["minimi", "m249"], "CarlGustaf": ["carl_gustaf", "carlgustaf", "gustaf", "m3_maaws"],
    "M72": ["m72", "law"], "M79": ["m79"], "Crossbow": ["crossbow"], "Knife": ["knife", "combat_knife"],
    "Machete": ["machete"], "Katana": ["katana"], "VikingSword": ["viking_sword", "sword"],
    "VikingAxe": ["viking_axe", "battle_axe", "axe"], "DaneAxe": ["double_axe", "battle_axe", "labrys"],
    "BaseballBat": ["baseball", "bat"], "Crowbar": ["crowbar"], "Sledgehammer": ["sledge", "hammer"],
    "FryingPan": ["frying_pan", "pan"], "Musket": ["musket", "flintlock"],
    "Apple": ["apple"], "Banana": ["banana"], "Baguette": ["baguette", "bread"], "Coffee": ["coffee", "cup"],
    "Water": ["water_bottle", "bottle"], "Medkit": ["medkit", "first_aid", "firstaid"],
    "FragGrenade": ["frag", "grenade"], "Binoculars": ["binocular"], "Flashlight": ["flashlight", "torch"],
    "Radio": ["walkie", "radio"], "Drone": ["drone"],
}
OVERRIDES_FILE = os.path.join(PROJECT_DIR, "Config", "GSMeshOverrides.ini")
SKIP_WORDS = ("magazine", "_mag", "scope", "sight", "bullet", "shell", "casing", "attachment", "lod", "collision")


def _catalog_ids():
    import sys
    tools = os.path.join(PROJECT_DIR, "Tools", "ItemCatalog")
    if tools not in sys.path:
        sys.path.insert(0, tools)
    import catalog  # noqa
    return [(i["id"], i.get("cat", "")) for i in catalog.ITEMS]


def _fab_meshes():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    flt = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")],
                          package_paths=["/Game"], recursive_paths=True)
    out = []
    for data in registry.get_assets(flt):
        path = str(data.package_name)
        if path.startswith(DEST):
            continue
        name = str(data.asset_name).lower()
        if any(w in name for w in SKIP_WORDS):
            continue
        out.append((name, f"{path}.{data.asset_name}"))
    return out


def _read_overrides():
    manual, auto = {}, {}
    if os.path.exists(OVERRIDES_FILE):
        with open(OVERRIDES_FILE, encoding="utf-8") as f:
            for line in f:
                line = line.rstrip("\n")
                if not line.strip() or line.lstrip().startswith(";") or "=" not in line:
                    continue
                key = line.split("=", 1)[0].strip()
                (auto if line.rstrip().endswith("; auto") else manual)[key] = line
    return manual, auto


def link_fab_models():
    """Finds Fab/Megascans meshes whose names match an item and writes Config/GSMeshOverrides.ini.
    Lines you edit by hand (remove '; auto' at the end) are never overwritten."""
    meshes = _fab_meshes()
    manual, _ = _read_overrides()
    lines = ["; Fab/Megascans models that replace Claude's models. Format:",
             ";   Id=/Game/Path/Mesh.Mesh | yaw=0 pitch=0 roll=0 | scale=1 | offset=x,y,z",
             "; Lines ending in '; auto' are rewritten every time Unreal starts. Remove '; auto' to keep your own fix.",
             "; Weapon points forward along +X. If a gun points backwards, set yaw=180."]
    lines += list(manual.values())
    for item_id, cat in _catalog_ids():
        if item_id in manual:
            continue
        words = [item_id.lower()] + ALIASES.get(item_id, [])
        hit = None
        for w in words:
            cands = [m for m in meshes if w in m[0]]
            if cands:
                cands.sort(key=lambda m: len(m[0]))
                hit = cands[0][1]
                break
        if not hit:
            continue
        theirs = unreal.EditorAssetLibrary.load_asset(hit)
        ours = unreal.EditorAssetLibrary.load_asset(f"{DEST}/SM_{item_id}")
        if not theirs:
            continue
        tb = theirs.get_bounds()
        te = tb.box_extent
        yaw = 90.0 if te.y > te.x * 1.2 else 0.0
        their_len = max(te.x, te.y) * 2.0
        if ours:
            ob = ours.get_bounds()
            our_len = max(ob.box_extent.x, ob.box_extent.y) * 2.0
            our_c = ob.origin
        else:
            our_len, our_c = their_len, unreal.Vector(0, 0, 0)
        scale = our_len / their_len if their_len > 0.1 else 1.0
        tc = tb.origin
        if yaw == 90.0:
            tx, ty = -tc.y, tc.x
        else:
            tx, ty = tc.x, tc.y
        off = (our_c.x - tx * scale, our_c.y - ty * scale, our_c.z - tc.z * scale)
        lines.append(f"{item_id}={hit} | yaw={yaw:g} pitch=0 roll=0 | scale={scale:.3f} | "
                     f"offset={off[0]:.1f},{off[1]:.1f},{off[2]:.1f} ; auto")
        unreal.log(f"Gloshaugen: {item_id} bruker Fab-modellen {hit}")
    with open(OVERRIDES_FILE, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def _running_as_game():
    """True when the project runs as a game (-game, Standalone, packaged), not as the editor.
    The asset import below uses editor-only libraries and crashes the engine in game mode."""
    try:
        cmd = unreal.SystemLibrary.get_command_line().lower()
    except Exception:
        return False
    return " -game" in (" " + cmd) or "-server" in cmd


_map_handle = None

if _running_as_game():
    unreal.log("Gloshaugen: kjoerer som spill (-game), hopper over import i editoren")
else:
    try:
        imported = import_models()
        fix_building_collision(imported)
        fix_distance_fields(imported)
    except Exception as e:  # never block the editor from opening
        unreal.log_warning(f"Gloshaugen: modell-import feilet: {e}")
    try:
        import gs_materials
        gs_materials.run()  # our PBR textures and materials; scanned Poly Haven / Megascans materials win by name
    except Exception as e:
        unreal.log_warning(f"Gloshaugen: materialer feilet: {e}")
    try:
        import gs_sounds
        gs_sounds.run()  # procedural sound effects; recorded sounds with the same names win
    except Exception as e:
        unreal.log_warning(f"Gloshaugen: lyd-import feilet: {e}")
    try:
        link_fab_models()
    except Exception as e:
        unreal.log_warning(f"Gloshaugen: Fab-kobling feilet: {e}")


    # The map is made one tick after startup, when the editor is ready to open levels.


    def _make_map(_dt):
        global _map_handle
        if _map_handle is not None:
            unreal.unregister_slate_post_tick_callback(_map_handle)
            _map_handle = None
        try:
            import gs_materials
            gs_materials.ensure_map()
        except Exception as e:
            unreal.log_warning(f"Gloshaugen: kunne ikke lage kartet: {e}")


    try:
        _map_handle = unreal.register_slate_post_tick_callback(_make_map)
    except Exception as e:
        unreal.log_warning(f"Gloshaugen: {e}")
