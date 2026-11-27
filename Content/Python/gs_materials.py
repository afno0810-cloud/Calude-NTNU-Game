"""Gloshaugen materials, run from init_unreal.py when the editor opens.

1. Imports SourceArt/Textures/T_GS_* into /Game/GS/Textures (only new or changed files).
2. Builds the master materials (once):
     M_GS_Surface  - UV mapped (our meshes have 2 m box UVs), base colour x tint x large-scale variation,
                     normal, AO / roughness / metallic from the ARM texture.
     M_GS_World    - same, but projected from above in world space (ground, roads, paths).
     M_GS_Glass    - thin, reflective, see-through window glass.
     M_GS_Foliage  - masked two-sided leaves for trees and bushes.
3. Makes one material instance per surface type: MI_GS_<Name>.
4. Puts materials on every model in /Game/Models by material slot name. Scanned materials you have added
   (Poly Haven, Megascans, Fab) win over ours when their names match; see SCANNED_KEYWORDS.
"""
import os
import re
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
TEX_DIR = os.path.join(PROJECT_DIR, "SourceArt", "Textures")
GS = "/Game/GS"
TEX = GS + "/Textures"
MAT = GS + "/Materials"
MODELS = "/Game/Models"
VERSION = 10  # bump to rebuild the master materials (10: cheaper, prettier glass for Helgasetr, K19)

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def log(msg):
    unreal.log(f"Gloshaugen: {msg}")


# ------------------------------------------------------------------ saving that survives a locked file (runde 5a)
# Another Unreal process (another team's headless run, the editor) can hold a .uasset open. Then one save fails;
# try again three times with 2 s between, and remember the asset as "locked" for the import report (init_unreal.run_all).
LOCKED = []
FAILED = []


def save_asset(asset, tries=3, wait=2.0):
    import time
    name = asset.get_name() if asset else "?"
    for k in range(tries):
        try:
            if eal.save_loaded_asset(asset, only_if_is_dirty=False):
                return True
        except Exception as e:
            unreal.log_warning(f"Gloshaugen: lagring av {name} feilet ({k + 1}/{tries}): {e}")
        if k + 1 < tries:
            time.sleep(wait)
    unreal.log_warning(f"Gloshaugen: {name} er låst eller kan ikke lagres etter {tries} forsøk")
    if name not in LOCKED:
        LOCKED.append(name)
    return False


def _guard(what, fn, *a):
    """Runs one asset step; an error is logged and recorded, and the loop goes on with the next asset."""
    try:
        return fn(*a)
    except Exception as e:
        unreal.log_warning(f"Gloshaugen: {what} feilet: {e}")
        FAILED.append(f"{what}: {e}")
        return None


# ------------------------------------------------------------------ textures
def import_textures():
    if not os.path.isdir(TEX_DIR):
        return
    tasks = []
    for fn in sorted(os.listdir(TEX_DIR)):
        if not fn.lower().endswith((".jpg", ".png")):
            continue
        name = os.path.splitext(fn)[0]
        path = os.path.join(TEX_DIR, fn)
        stamp = path + ".imported"
        if eal.does_asset_exist(f"{TEX}/{name}") and os.path.exists(stamp) and os.path.getmtime(stamp) >= os.path.getmtime(path):
            continue
        t = unreal.AssetImportTask()
        t.filename = path
        t.destination_path = TEX
        t.destination_name = name
        t.automated = True
        t.replace_existing = True
        t.save = False
        tasks.append((t, path, name))
    if not tasks:
        return
    log(f"importerer {len(tasks)} tekstur(er)")
    tools.import_asset_tasks([t for t, _, _ in tasks])
    for _, path, name in tasks:
        tex = eal.load_asset(f"{TEX}/{name}")
        if tex:
            if name.endswith("_N"):
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
                tex.set_editor_property("srgb", False)
                tex.set_editor_property("flip_green_channel", False)
            elif name.endswith("_ARM") or name.endswith("_M"):
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
                tex.set_editor_property("srgb", False)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
            save_asset(tex)
        with open(path + ".imported", "w") as f:
            f.write("ok")


def tex(name):
    p = f"{TEX}/{name}"
    return eal.load_asset(p) if eal.does_asset_exist(p) else None


# ------------------------------------------------------------------ master materials
def _expr(m, cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def _new_material(name):
    path = f"{MAT}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return tools.create_asset(name, MAT, unreal.Material, unreal.MaterialFactoryNew())


def ensure_weather_mpc():
    """MPC_GS_Weather: Wetness and SnowCover (0..1), set every frame by AGSEnvironment."""
    path = f"{GS}/MPC_GS_Weather"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    mpc = tools.create_asset("MPC_GS_Weather", GS, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    params = []
    for n in ("Wetness", "SnowCover"):
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", 0.0)
        params.append(p)
    mpc.set_editor_property("scalar_parameters", params)
    save_asset(mpc)
    return mpc


def _surface_master(name, world):
    m = _new_material(name)
    S = unreal.MaterialSamplerType
    if world:
        wp = _expr(m, unreal.MaterialExpressionWorldPosition, -1400, 0)
        mask = _expr(m, unreal.MaterialExpressionComponentMask, -1250, 0, r=True, g=True, b=False, a=False)
        mel.connect_material_expressions(wp, "", mask, "")
        tile = _expr(m, unreal.MaterialExpressionScalarParameter, -1250, 120, parameter_name="TileCm", default_value=200.0)
        uv = _expr(m, unreal.MaterialExpressionDivide, -1100, 0)
        mel.connect_material_expressions(mask, "", uv, "A")
        mel.connect_material_expressions(tile, "", uv, "B")
    else:
        tc = _expr(m, unreal.MaterialExpressionTextureCoordinate, -1250, 0)
        tiling = _expr(m, unreal.MaterialExpressionScalarParameter, -1250, 120, parameter_name="Tiling", default_value=1.0)
        uv = _expr(m, unreal.MaterialExpressionMultiply, -1100, 0)
        mel.connect_material_expressions(tc, "", uv, "A")
        mel.connect_material_expressions(tiling, "", uv, "B")

    d = _expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, -300, parameter_name="BaseColorTex",
              texture=tex("T_GS_Concrete_D"), sampler_type=S.SAMPLERTYPE_COLOR)
    n = _expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0, parameter_name="NormalTex",
              texture=tex("T_GS_Concrete_N"), sampler_type=S.SAMPLERTYPE_NORMAL)
    a = _expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 300, parameter_name="ARMTex",
              texture=tex("T_GS_Concrete_ARM"), sampler_type=S.SAMPLERTYPE_MASKS)
    for s in (d, n, a):
        mel.connect_material_expressions(uv, "", s, "UVs")

    # Large-scale variation so the 2 m tile never shows as a repeating pattern
    mscale = _expr(m, unreal.MaterialExpressionConstant, -1100, -450, r=0.113)
    muv = _expr(m, unreal.MaterialExpressionMultiply, -950, -450)
    mel.connect_material_expressions(uv, "", muv, "A")
    mel.connect_material_expressions(mscale, "", muv, "B")
    mac = _expr(m, unreal.MaterialExpressionTextureSample, -800, -600, texture=tex("T_GS_Macro_M"), sampler_type=S.SAMPLERTYPE_MASKS)
    mel.connect_material_expressions(muv, "", mac, "UVs")
    lo = _expr(m, unreal.MaterialExpressionScalarParameter, -650, -700, parameter_name="VariationDark", default_value=0.8)
    hi = _expr(m, unreal.MaterialExpressionScalarParameter, -650, -620, parameter_name="VariationBright", default_value=1.12)
    var = _expr(m, unreal.MaterialExpressionLinearInterpolate, -500, -600)
    mel.connect_material_expressions(lo, "", var, "A")
    mel.connect_material_expressions(hi, "", var, "B")
    mel.connect_material_expressions(mac, "R", var, "Alpha")

    tint = _expr(m, unreal.MaterialExpressionVectorParameter, -650, -450, parameter_name="Tint",
                 default_value=unreal.LinearColor(1, 1, 1, 1))
    c1 = _expr(m, unreal.MaterialExpressionMultiply, -500, -350)
    mel.connect_material_expressions(d, "RGB", c1, "A")
    mel.connect_material_expressions(tint, "", c1, "B")
    c2 = _expr(m, unreal.MaterialExpressionMultiply, -350, -350)
    mel.connect_material_expressions(c1, "", c2, "A")
    mel.connect_material_expressions(var, "", c2, "B")
    rmul = _expr(m, unreal.MaterialExpressionScalarParameter, -650, 420, parameter_name="RoughnessScale", default_value=1.0)
    r = _expr(m, unreal.MaterialExpressionMultiply, -500, 350)
    mel.connect_material_expressions(a, "G", r, "A")
    mel.connect_material_expressions(rmul, "", r, "B")

    # Weather: rain darkens and makes everything glossy (wet streets reflect with ray tracing),
    # snow settles on surfaces that face up.
    mpc = ensure_weather_mpc()
    wet = _expr(m, unreal.MaterialExpressionCollectionParameter, -350, 800, collection=mpc, parameter_name="Wetness")
    snow = _expr(m, unreal.MaterialExpressionCollectionParameter, -350, 900, collection=mpc, parameter_name="SnowCover")
    # Weather = 1 outdoors, 0 indoors (the MI_GS_<Name>_In instances): floors, walls and furniture under a roof
    # never get wet or snowed on. Everything below uses the scaled values.
    weather = _expr(m, unreal.MaterialExpressionScalarParameter, -650, 850, parameter_name="Weather", default_value=1.0)
    wet_raw, snow_raw = wet, snow
    wet = _expr(m, unreal.MaterialExpressionMultiply, -500, 800)
    mel.connect_material_expressions(wet_raw, "", wet, "A")
    mel.connect_material_expressions(weather, "", wet, "B")
    snow = _expr(m, unreal.MaterialExpressionMultiply, -500, 900)
    mel.connect_material_expressions(snow_raw, "", snow, "A")
    mel.connect_material_expressions(weather, "", snow, "B")
    dark = _expr(m, unreal.MaterialExpressionConstant, -350, 1000, r=0.55)
    one = _expr(m, unreal.MaterialExpressionConstant, -350, 1060, r=1.0)
    wdark = _expr(m, unreal.MaterialExpressionLinearInterpolate, -200, 900)
    mel.connect_material_expressions(one, "", wdark, "A")
    mel.connect_material_expressions(dark, "", wdark, "B")
    mel.connect_material_expressions(wet, "", wdark, "Alpha")
    c3 = _expr(m, unreal.MaterialExpressionMultiply, -50, -350)
    mel.connect_material_expressions(c2, "", c3, "A")
    mel.connect_material_expressions(wdark, "", c3, "B")
    # snow mask: up-facing (world normal Z) x cover
    vn = _expr(m, unreal.MaterialExpressionVertexNormalWS, -350, 1150)
    vz = _expr(m, unreal.MaterialExpressionComponentMask, -200, 1150, r=False, g=False, b=True, a=False)
    mel.connect_material_expressions(vn, "", vz, "")
    up = _expr(m, unreal.MaterialExpressionSmoothStep, -50, 1150)
    k1 = _expr(m, unreal.MaterialExpressionConstant, -200, 1230, r=0.55)
    k2 = _expr(m, unreal.MaterialExpressionConstant, -200, 1290, r=0.85)
    mel.connect_material_expressions(k1, "", up, "Min")
    mel.connect_material_expressions(k2, "", up, "Max")
    mel.connect_material_expressions(vz, "", up, "Value")
    smask = _expr(m, unreal.MaterialExpressionMultiply, 100, 1100)
    mel.connect_material_expressions(up, "", smask, "A")
    mel.connect_material_expressions(snow, "", smask, "B")
    white = _expr(m, unreal.MaterialExpressionConstant3Vector, 100, 1000, constant=unreal.LinearColor(0.85, 0.87, 0.9, 1))
    c4 = _expr(m, unreal.MaterialExpressionLinearInterpolate, 250, -350)
    mel.connect_material_expressions(c3, "", c4, "A")
    mel.connect_material_expressions(white, "", c4, "B")
    mel.connect_material_expressions(smask, "", c4, "Alpha")
    mel.connect_material_property(c4, "", unreal.MaterialProperty.MP_BASE_COLOR)
    wetr = _expr(m, unreal.MaterialExpressionConstant, -350, 500, r=0.08)
    r2 = _expr(m, unreal.MaterialExpressionLinearInterpolate, -200, 400)
    mel.connect_material_expressions(r, "", r2, "A")
    mel.connect_material_expressions(wetr, "", r2, "B")
    wet85 = _expr(m, unreal.MaterialExpressionMultiply, -350, 600)
    mel.connect_material_expressions(wet, "", wet85, "A")
    k85 = _expr(m, unreal.MaterialExpressionConstant, -500, 600, r=0.85)
    mel.connect_material_expressions(k85, "", wet85, "B")
    mel.connect_material_expressions(wet85, "", r2, "Alpha")
    snowr = _expr(m, unreal.MaterialExpressionConstant, 100, 500, r=0.75)
    r3 = _expr(m, unreal.MaterialExpressionLinearInterpolate, 250, 400)
    mel.connect_material_expressions(r2, "", r3, "A")
    mel.connect_material_expressions(snowr, "", r3, "B")
    mel.connect_material_expressions(smask, "", r3, "Alpha")
    mel.connect_material_property(r3, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mmul = _expr(m, unreal.MaterialExpressionScalarParameter, -650, 520, parameter_name="MetallicScale", default_value=1.0)
    mt = _expr(m, unreal.MaterialExpressionMultiply, -500, 480)
    mel.connect_material_expressions(a, "B", mt, "A")
    mel.connect_material_expressions(mmul, "", mt, "B")
    mel.connect_material_property(mt, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(a, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    # normal strength: lerp between flat and the texture
    flat = _expr(m, unreal.MaterialExpressionConstant3Vector, -650, 100, constant=unreal.LinearColor(0, 0, 1, 1))
    ns = _expr(m, unreal.MaterialExpressionScalarParameter, -650, 200, parameter_name="NormalStrength", default_value=1.0)
    nl = _expr(m, unreal.MaterialExpressionLinearInterpolate, -450, 50)
    mel.connect_material_expressions(flat, "", nl, "A")
    mel.connect_material_expressions(n, "RGB", nl, "B")
    mel.connect_material_expressions(ns, "", nl, "Alpha")
    mel.connect_material_property(nl, "", unreal.MaterialProperty.MP_NORMAL)

    # emissive (ceiling light panels, screens): colour x strength, 0 by default
    ec = _expr(m, unreal.MaterialExpressionVectorParameter, -650, 650, parameter_name="EmissiveColor",
               default_value=unreal.LinearColor(1, 0.95, 0.85, 1))
    es = _expr(m, unreal.MaterialExpressionScalarParameter, -650, 750, parameter_name="EmissiveStrength", default_value=0.0)
    em = _expr(m, unreal.MaterialExpressionMultiply, -500, 680)
    mel.connect_material_expressions(ec, "", em, "A")
    mel.connect_material_expressions(es, "", em, "B")
    mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    m.set_editor_property("used_with_instanced_static_meshes", True)
    m.set_editor_property("used_with_nanite", True)
    mel.recompile_material(m)
    save_asset(m)
    return m


def _glass_master():
    """Window glass, cheap and still glassy (K19): translucency-volume lighting (no per-pixel forward shading), no
    refraction, no screen-space or Lumen reflection on the translucent surface. Fresnel does the look: nearly clear
    when you look straight through, a greenish, more reflective sheet at grazing angles, like the photos."""
    m = _new_material("M_GS_Glass")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    try:
        m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    except Exception:
        pass
    for prop, val in (("screen_space_reflections", False), ("contact_shadows", False), ("output_translucent_velocity", False)):
        try:
            m.set_editor_property(prop, val)
        except Exception:
            pass
    m.set_editor_property("two_sided", True)
    col = _expr(m, unreal.MaterialExpressionVectorParameter, -700, -150, parameter_name="Tint",
                default_value=unreal.LinearColor(0.06, 0.09, 0.1, 1))
    edge = _expr(m, unreal.MaterialExpressionVectorParameter, -700, -300, parameter_name="EdgeTint",
                 default_value=unreal.LinearColor(0.10, 0.16, 0.15, 1))
    fr = _expr(m, unreal.MaterialExpressionFresnel, -700, 0)
    try:
        fr.set_editor_property("exponent", 4.0)
        fr.set_editor_property("base_reflect_fraction", 0.04)
    except Exception:
        pass
    bc = _expr(m, unreal.MaterialExpressionLinearInterpolate, -400, -200)
    mel.connect_material_expressions(col, "", bc, "A")
    mel.connect_material_expressions(edge, "", bc, "B")
    mel.connect_material_expressions(fr, "", bc, "Alpha")
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    op_lo = _expr(m, unreal.MaterialExpressionScalarParameter, -700, 120, parameter_name="Opacity", default_value=0.16)
    op_hi = _expr(m, unreal.MaterialExpressionScalarParameter, -700, 200, parameter_name="OpacityEdge", default_value=0.6)
    op = _expr(m, unreal.MaterialExpressionLinearInterpolate, -400, 100)
    mel.connect_material_expressions(op_lo, "", op, "A")
    mel.connect_material_expressions(op_hi, "", op, "B")
    mel.connect_material_expressions(fr, "", op, "Alpha")
    mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    ro = _expr(m, unreal.MaterialExpressionConstant, -500, 280, r=0.03)
    mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
    sp = _expr(m, unreal.MaterialExpressionConstant, -500, 340, r=1.0)
    mel.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(m)
    save_asset(m)


def _foliage_master():
    m = _new_material("M_GS_Foliage")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    try:
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    except Exception:
        pass
    S = unreal.MaterialSamplerType
    d = _expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, -150, parameter_name="BaseColorTex",
              texture=tex("T_GS_Leaves_D"), sampler_type=S.SAMPLERTYPE_COLOR)
    n = _expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 150, parameter_name="NormalTex",
              texture=tex("T_GS_Leaves_N"), sampler_type=S.SAMPLERTYPE_NORMAL)
    tint = _expr(m, unreal.MaterialExpressionVectorParameter, -500, -300, parameter_name="Tint",
                 default_value=unreal.LinearColor(1, 1, 1, 1))
    c = _expr(m, unreal.MaterialExpressionMultiply, -300, -150)
    mel.connect_material_expressions(d, "RGB", c, "A")
    mel.connect_material_expressions(tint, "", c, "B")
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(d, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.connect_material_property(n, "RGB", unreal.MaterialProperty.MP_NORMAL)
    sss = _expr(m, unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(c, "", sss, "A")
    k = _expr(m, unreal.MaterialExpressionConstant, -500, 50, r=0.6)
    mel.connect_material_expressions(k, "", sss, "B")
    try:
        mel.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    except Exception:
        pass
    ro = _expr(m, unreal.MaterialExpressionConstant, -300, 250, r=0.65)
    mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
    # gentle wind sway: own small graph (SimpleGrassWind moved in UE 5.8 and loading the missing
    # asset logged an error on every editor start).
    try:
        t = _expr(m, unreal.MaterialExpressionTime, -1300, 400)
        wp = _expr(m, unreal.MaterialExpressionWorldPosition, -1300, 480)
        mask = _expr(m, unreal.MaterialExpressionComponentMask, -1150, 480, r=True, g=False, b=False, a=False)
        mel.connect_material_expressions(wp, "", mask, "")
        sc = _expr(m, unreal.MaterialExpressionMultiply, -1000, 480, const_b=0.004)
        mel.connect_material_expressions(mask, "", sc, "A")
        ts = _expr(m, unreal.MaterialExpressionMultiply, -1000, 400, const_b=1.7)
        mel.connect_material_expressions(t, "", ts, "A")
        ph = _expr(m, unreal.MaterialExpressionAdd, -850, 430)
        mel.connect_material_expressions(ts, "", ph, "A")
        mel.connect_material_expressions(sc, "", ph, "B")
        sn = _expr(m, unreal.MaterialExpressionSine, -700, 430)
        mel.connect_material_expressions(ph, "", sn, "")
        uv = _expr(m, unreal.MaterialExpressionTextureCoordinate, -850, 560)
        vm = _expr(m, unreal.MaterialExpressionComponentMask, -700, 560, r=False, g=True, b=False, a=False)
        mel.connect_material_expressions(uv, "", vm, "")
        om = _expr(m, unreal.MaterialExpressionOneMinus, -550, 560)
        mel.connect_material_expressions(vm, "", om, "")
        amp = _expr(m, unreal.MaterialExpressionMultiply, -550, 430)
        mel.connect_material_expressions(sn, "", amp, "A")
        mel.connect_material_expressions(om, "", amp, "B")
        dirv = _expr(m, unreal.MaterialExpressionConstant3Vector, -550, 330, constant=unreal.LinearColor(2.0, 1.2, 0.0, 0.0))
        wpo = _expr(m, unreal.MaterialExpressionMultiply, -400, 400)
        mel.connect_material_expressions(amp, "", wpo, "A")
        mel.connect_material_expressions(dirv, "", wpo, "B")
        mel.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    except Exception as e:
        log(f"vind i bladmateriale hoppet over: {e}")
    m.set_editor_property("used_with_instanced_static_meshes", True)
    m.set_editor_property("used_with_nanite", True)
    mel.recompile_material(m)
    save_asset(m)


def _fx_master(name, additive):
    """Soft round sprite for smoke, dust, fire and flashes (used by AGSShortFX on a camera-facing plane).
    Params: Color (rgb), Glow (emissive multiplier), Opacity (0..1), Soft (edge falloff power)."""
    m = _new_material(name)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE if additive else unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    col = _expr(m, unreal.MaterialExpressionVectorParameter, -900, -200, parameter_name="Color",
                default_value=unreal.LinearColor(0.5, 0.5, 0.5, 1))
    glow = _expr(m, unreal.MaterialExpressionScalarParameter, -900, -60, parameter_name="Glow", default_value=1.0)
    op = _expr(m, unreal.MaterialExpressionScalarParameter, -900, 40, parameter_name="Opacity", default_value=1.0)
    soft = _expr(m, unreal.MaterialExpressionScalarParameter, -900, 120, parameter_name="Soft", default_value=1.6)
    # radial mask: r^2 = dot(uv-0.5, uv-0.5) * 4, mask = saturate(1 - r^2) ^ Soft
    uv = _expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, 250)
    sub = _expr(m, unreal.MaterialExpressionSubtract, -1150, 250, const_b=0.5)
    mel.connect_material_expressions(uv, "", sub, "A")
    dot = _expr(m, unreal.MaterialExpressionDotProduct, -1000, 250)
    mel.connect_material_expressions(sub, "", dot, "A")
    mel.connect_material_expressions(sub, "", dot, "B")
    r2 = _expr(m, unreal.MaterialExpressionMultiply, -850, 250, const_b=4.0)
    mel.connect_material_expressions(dot, "", r2, "A")
    inv = _expr(m, unreal.MaterialExpressionOneMinus, -700, 250)
    mel.connect_material_expressions(r2, "", inv, "")
    sat = _expr(m, unreal.MaterialExpressionSaturate, -560, 250)
    mel.connect_material_expressions(inv, "", sat, "")
    pw = _expr(m, unreal.MaterialExpressionPower, -420, 250)
    mel.connect_material_expressions(sat, "", pw, "Base")
    mel.connect_material_expressions(soft, "", pw, "Exponent")
    mask = pw
    if not additive:
        # break the round blob up a little with world-space noise so smoke looks like smoke
        try:
            nz = _expr(m, unreal.MaterialExpressionNoise, -700, 420, scale=0.08, levels=3, output_min=0.45, output_max=1.0)
            mm = _expr(m, unreal.MaterialExpressionMultiply, -300, 330)
            mel.connect_material_expressions(pw, "", mm, "A")
            mel.connect_material_expressions(nz, "", mm, "B")
            mask = mm
        except Exception as e:
            log(f"stoey i roeyk hoppet over: {e}")
    a = _expr(m, unreal.MaterialExpressionMultiply, -200, 150)
    mel.connect_material_expressions(mask, "", a, "A")
    mel.connect_material_expressions(op, "", a, "B")
    e = _expr(m, unreal.MaterialExpressionMultiply, -500, -150)
    mel.connect_material_expressions(col, "", e, "A")
    mel.connect_material_expressions(glow, "", e, "B")
    if additive:
        e2 = _expr(m, unreal.MaterialExpressionMultiply, -100, -100)
        mel.connect_material_expressions(e, "", e2, "A")
        mel.connect_material_expressions(a, "", e2, "B")
        mel.connect_material_property(e2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mel.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        try:
            df = _expr(m, unreal.MaterialExpressionDepthFade, 0, 150, fade_distance_default=25.0)
            mel.connect_material_expressions(a, "", df, "Opacity")
            mel.connect_material_property(df, "", unreal.MaterialProperty.MP_OPACITY)
        except Exception:
            mel.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    save_asset(m)


def _blood_master():
    """Wet blood lying on a surface (AGSShortFX stains, on a flat plane). Lit and glossy, ragged edge.
    Params: Color (rgb), Roughness, Opacity (1 = whole stain, towards 0 it erodes away), Soft (edge falloff power)."""
    m = _new_material("M_GS_Blood")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("opacity_mask_clip_value", 0.3)
    col = _expr(m, unreal.MaterialExpressionVectorParameter, -900, -200, parameter_name="Color",
                default_value=unreal.LinearColor(0.11, 0.004, 0.004, 1))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = _expr(m, unreal.MaterialExpressionScalarParameter, -900, -60, parameter_name="Roughness", default_value=0.12)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    op = _expr(m, unreal.MaterialExpressionScalarParameter, -900, 40, parameter_name="Opacity", default_value=1.0)
    soft = _expr(m, unreal.MaterialExpressionScalarParameter, -900, 120, parameter_name="Soft", default_value=0.6)
    # radial mask, as in _fx_master: saturate(1 - r^2) ^ Soft
    uv = _expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, 250)
    sub = _expr(m, unreal.MaterialExpressionSubtract, -1150, 250, const_b=0.5)
    mel.connect_material_expressions(uv, "", sub, "A")
    dot = _expr(m, unreal.MaterialExpressionDotProduct, -1000, 250)
    mel.connect_material_expressions(sub, "", dot, "A")
    mel.connect_material_expressions(sub, "", dot, "B")
    r2 = _expr(m, unreal.MaterialExpressionMultiply, -850, 250, const_b=4.0)
    mel.connect_material_expressions(dot, "", r2, "A")
    inv = _expr(m, unreal.MaterialExpressionOneMinus, -700, 250)
    mel.connect_material_expressions(r2, "", inv, "")
    sat = _expr(m, unreal.MaterialExpressionSaturate, -560, 250)
    mel.connect_material_expressions(inv, "", sat, "")
    pw = _expr(m, unreal.MaterialExpressionPower, -420, 250)
    mel.connect_material_expressions(sat, "", pw, "Base")
    mel.connect_material_expressions(soft, "", pw, "Exponent")
    mask = pw
    try:
        # World-space noise moves the edge in and out (between about half and the whole radius), so every stain
        # gets its own ragged outline and a growing pool creeps unevenly. The middle is always solid.
        nz = _expr(m, unreal.MaterialExpressionNoise, -700, 420, scale=0.15, levels=4, output_min=-0.55, output_max=0.12)
        mm = _expr(m, unreal.MaterialExpressionAdd, -300, 330)
        mel.connect_material_expressions(pw, "", mm, "A")
        mel.connect_material_expressions(nz, "", mm, "B")
        mask = mm
    except Exception as e:
        log(f"stoey i blod hoppet over: {e}")
    a = _expr(m, unreal.MaterialExpressionMultiply, -200, 150)
    mel.connect_material_expressions(mask, "", a, "A")
    mel.connect_material_expressions(op, "", a, "B")
    mel.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(m)
    save_asset(m)


def build_masters():
    stamp = os.path.join(PROJECT_DIR, "Saved", "GSMaterials.version")
    have = 0
    if os.path.exists(stamp):
        try:
            have = int(open(stamp).read().strip() or 0)
        except ValueError:
            have = 0
    if have == VERSION and eal.does_asset_exist(f"{MAT}/M_GS_Surface"):
        return False
    log("bygger hovedmaterialer")
    _surface_master("M_GS_Surface", world=False)
    _surface_master("M_GS_World", world=True)
    _glass_master()
    _foliage_master()
    for nm, add in (("M_GS_FX", False), ("M_GS_FXAdd", True)):
        try:
            _fx_master(nm, add)
        except Exception as e:
            log(f"{nm} feilet: {e}")
    os.makedirs(os.path.dirname(stamp), exist_ok=True)
    with open(stamp, "w") as f:
        f.write(str(VERSION))
    return True


# ------------------------------------------------------------------ material instances
# name: (texture set, tint rgb, tiling, roughness scale, metallic scale, master)
INSTANCES = {
    "Brick": ("Brick", (1, 1, 1), 1, 1, 0, "Surface"),
    "BrickBrown": ("BrickBrown", (1, 1, 1), 1, 1, 0, "Surface"),
    "BrickYellow": ("BrickYellow", (1, 1, 1), 1, 1, 0, "Surface"),
    "Stone": ("Stone", (1, 1, 1), 1, 1, 0, "Surface"),
    "StoneDark": ("StoneDark", (1, 1, 1), 1, 1, 0, "Surface"),
    "Trim": ("Trim", (1, 1, 1), 1, 1, 0, "Surface"),
    "Slate": ("Slate", (1, 1, 1), 1.5, 1, 0, "Surface"),
    "Plaster": ("Plaster", (1, 1, 1), 1, 1, 0, "Surface"),
    "PlasterWhite": ("Plaster", (1.15, 1.15, 1.17), 1, 1, 0, "Surface"),
    "Concrete": ("Concrete", (1, 1, 1), 1, 1, 0, "Surface"),
    "ConcreteSmooth": ("ConcreteSmooth", (1, 1, 1), 1, 1, 0, "Surface"),
    "Floor": ("Floor", (1, 1, 1), 1, 1, 0, "Surface"),
    "Wood": ("Wood", (1, 1, 1), 2, 1, 0, "Surface"),
    "WoodLight": ("Floor", (1.2, 1.15, 1.05), 2, 1, 0, "Surface"),
    "Marble": ("Marble", (1, 1, 1), 1, 1, 0, "Surface"),
    "Terrazzo": ("Terrazzo", (1, 1, 1), 1, 1, 0, "Surface"),
    "Copper": ("Copper", (1, 1, 1), 1, 1, 1, "Surface"),
    "Metal": ("Metal", (1, 1, 1), 2, 1, 1, "Surface"),
    "MetalDark": ("Metal", (0.18, 0.18, 0.19), 2, 1.2, 1, "Surface"),
    "Steel": ("Metal", (0.9, 0.9, 0.92), 2, 0.8, 1, "Surface"),
    "Gunmetal": ("Metal", (0.12, 0.12, 0.13), 3, 1.3, 1, "Surface"),
    "Fabric": ("Fabric", (0.2, 0.3, 0.55), 4, 1, 0, "Surface"),
    "Tarp": ("Fabric", (0.10, 0.28, 0.16), 3, 0.9, 0, "Surface"),
    "FabricRed": ("Fabric", (0.75, 0.12, 0.12), 4, 1, 0, "Surface"),
    "Plastic": ("Plastic", (0.08, 0.08, 0.09), 2, 1, 0, "Surface"),
    "PlasticWhite": ("Plastic", (1.13, 1.13, 1.15), 2, 1, 0, "Surface"),
    "Polymer": ("Plastic", (0.06, 0.06, 0.065), 3, 1.2, 0, "Surface"),
    "Rubber": ("Rubber", (1, 1, 1), 3, 1, 0, "Surface"),
    "PaintRed": ("PaintedMetal", (0.9, 0.06, 0.05), 2, 0.9, 0, "Surface"),
    "PaintBlue": ("PaintedMetal", (0.06, 0.25, 0.7), 2, 0.9, 0, "Surface"),
    "PaintYellow": ("PaintedMetal", (1.1, 0.85, 0.1), 2, 0.9, 0, "Surface"),
    "PaintGreen": ("PaintedMetal", (0.12, 0.42, 0.24), 2, 0.9, 0, "Surface"),
    "PaintOrange": ("PaintedMetal", (1.2, 0.5, 0.08), 2, 0.9, 0, "Surface"),
    "NTNUBlue": ("PaintedMetal", (0.0, 0.3, 0.68), 2, 0.9, 0, "Surface"),
    # Helgasetr (round 4b, from the photos): yellow stairs and Ryddekroken, orange mezzanine floor, white painted steel truss
    "HelgYellow": ("PaintedMetal", (1.15, 0.62, 0.06), 2, 0.8, 0, "Surface"),
    "HelgOrange": ("Plastic", (1.1, 0.42, 0.08), 2, 1.0, 0, "Surface"),
    "PaintWhite": ("PaintedMetal", (1.1, 1.1, 1.08), 2, 0.9, 0, "Surface"),
    "RobotWhite": ("Plastic", (1.15, 1.15, 1.18), 2, 0.8, 0, "Surface"),
    "RobotBody": ("PaintedMetal", (1.1, 0.9, 0.1), 2, 0.9, 0, "Surface"),
    "Asphalt": ("Asphalt", (1, 1, 1), 1, 1, 0, "Surface"),
    "Paving": ("Paving", (1, 1, 1), 1, 1, 0, "Surface"),
    "Gravel": ("Gravel", (1, 1, 1), 1, 1, 0, "Surface"),
    "Soil": ("Gravel", (0.45, 0.32, 0.22), 2, 1, 0, "Surface"),
    "Bark": ("Bark", (1, 1, 1), 1, 1, 0, "Surface"),
    "BirchBark": ("BirchBark", (1, 1, 1), 1, 1, 0, "Surface"),
    # weapons and items (Tools/Blender/gen_models.py and weapon_*.py use M_<name>)
    "M_Wood": ("GunWood", (1, 1, 1), 1.0, 1.0, 0, "Surface"),
    "M_LightWood": ("GunWood", (1.7, 1.6, 1.5), 1.0, 1.0, 0, "Surface"),
    "M_Steel": ("Metal", (0.2, 0.2, 0.21), 1.2, 0.75, 1, "Surface"),
    "M_DarkSteel": ("Metal", (0.08, 0.08, 0.085), 1.2, 1.0, 1, "Surface"),
    "M_Chrome": ("Metal", (0.95, 0.95, 0.97), 1.2, 0.25, 1, "Surface"),
    "M_Blade": ("Metal", (0.85, 0.86, 0.88), 1.2, 0.5, 1, "Surface"),
    "M_BladeDark": ("Metal", (0.2, 0.2, 0.21), 1.2, 0.9, 1, "Surface"),
    "M_Gold": ("Metal", (1.0, 0.75, 0.3), 1.2, 0.5, 1, "Surface"),
    "M_Brass": ("Metal", (0.85, 0.62, 0.3), 1.2, 0.6, 1, "Surface"),
    "M_BlackPolymer": ("Plastic", (0.05, 0.05, 0.055), 1.2, 1.3, 0, "Surface"),
    "M_Bakelite": ("Plastic", (0.25, 0.1, 0.05), 1.2, 0.8, 0, "Surface"),
    "M_OliveDrab": ("PaintedMetal", (0.22, 0.25, 0.14), 1.2, 1.1, 0, "Surface"),
    "M_Tan": ("PaintedMetal", (0.55, 0.45, 0.3), 1.2, 1.1, 0, "Surface"),
    "M_Leather": ("Leather", (1, 1, 1), 1.2, 1, 0, "Surface"),
    "M_KatanaWrap": ("Fabric", (0.05, 0.05, 0.08), 4, 1, 0, "Surface"),
    "M_Fabric": ("Fabric", (0.3, 0.32, 0.25), 2, 1, 0, "Surface"),
    "M_Rubber": ("Rubber", (1, 1, 1), 1.2, 1, 0, "Surface"),
    "M_Paper": ("Plastic", (1.2, 1.18, 1.1), 1, 1.5, 0, "Surface"),
    "M_White": ("Plastic", (1.15, 1.15, 1.15), 1.2, 1, 0, "Surface"),
    "M_Red": ("PaintedMetal", (0.9, 0.05, 0.04), 1.2, 0.9, 0, "Surface"),
    "Leather": ("Leather", (1, 1, 1), 1, 1, 0, "Surface"),
    # lights and screens (glow; Lumen uses them to light the rooms)
    "LightPanel": ("Plastic", (1.2, 1.2, 1.2), 1, 1, 0, "Surface", 12.0),
    "ScreenOn": ("Plastic", (0.1, 0.3, 0.8), 1, 0.3, 0, "Surface", 3.0, (0.25, 0.55, 1.0)),
    "Bulb": ("Plastic", (1.2, 1.1, 0.9), 1, 1, 0, "Surface", 20.0, (1.0, 0.85, 0.6)),
    # car paint (GSCampusBuilder picks one per parked car)
    "CarPaint_Red": ("PaintedMetal", (0.55, 0.03, 0.03), 4, 0.35, 0, "Surface"),
    "CarPaint_Black": ("PaintedMetal", (0.02, 0.02, 0.022), 4, 0.3, 0, "Surface"),
    "CarPaint_White": ("PaintedMetal", (1.05, 1.05, 1.05), 4, 0.35, 0, "Surface"),
    "CarPaint_Silver": ("Metal", (0.75, 0.76, 0.78), 6, 0.6, 1, "Surface"),
    "CarPaint_Blue": ("PaintedMetal", (0.04, 0.12, 0.38), 4, 0.35, 0, "Surface"),
    "CarPaint_Grey": ("PaintedMetal", (0.25, 0.26, 0.27), 4, 0.35, 0, "Surface"),
    "CarPaint_Green": ("PaintedMetal", (0.06, 0.2, 0.12), 4, 0.35, 0, "Surface"),
    "CarPaint": ("PaintedMetal", (0.55, 0.03, 0.03), 4, 0.35, 0, "Surface"),
    "Plate": ("Plastic", (1.2, 1.2, 1.2), 1, 0.6, 0, "Surface"),
    # world projected (ground and roads, made from plain boxes by GSCampusBuilder)
    "W_Grass": ("Grass", (1, 1, 1), 400, 1, 0, "World"),
    "W_Asphalt": ("Asphalt", (1, 1, 1), 300, 1, 0, "World"),
    "W_Paving": ("Paving", (1, 1, 1), 200, 1, 0, "World"),
    "W_Gravel": ("Gravel", (1, 1, 1), 200, 1, 0, "World"),
    "W_Concrete": ("ConcreteSmooth", (1, 1, 1), 200, 1, 0, "World"),
}


def _fill_instance(mi, parent, spec):
    """Sets parent, textures, tint and scales on a material instance from its INSTANCES entry."""
    texset, tint, tiling, rough, metal, master = spec[:6]
    glow = spec[6] if len(spec) > 6 else 0.0
    glow_col = spec[7] if len(spec) > 7 else (1.0, 0.95, 0.85)
    mel.set_material_instance_parent(mi, parent)
    for param, suffix in (("BaseColorTex", "_D"), ("NormalTex", "_N"), ("ARMTex", "_ARM")):
        t = tex(f"T_GS_{texset}{suffix}")
        if t:
            mel.set_material_instance_texture_parameter_value(mi, param, t)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(tint[0], tint[1], tint[2], 1))
    mel.set_material_instance_scalar_parameter_value(mi, "TileCm" if master == "World" else "Tiling", float(tiling))
    mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", float(rough))
    mel.set_material_instance_scalar_parameter_value(mi, "MetallicScale", float(metal))
    if master == "Surface":
        mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", float(glow))
        mel.set_material_instance_vector_parameter_value(mi, "EmissiveColor", unreal.LinearColor(glow_col[0], glow_col[1], glow_col[2], 1))


def _ensure_indoor(name, spec, force):
    """MI_GS_<Name>_In: the same material without rain and snow (Weather = 0), for everything under a roof.
    It has MI_GS_<Name> as its parent, so it follows the textures and the colour of the outdoor one. If the
    engine will not take an instance as parent, it becomes an instance of the master with the same values."""
    path = f"{MAT}/MI_GS_{name}_In"
    exists = eal.does_asset_exist(path)
    if exists and not force:
        return 0
    outdoor = eal.load_asset(f"{MAT}/MI_GS_{name}")
    if not outdoor:
        return 0
    mi = eal.load_asset(path) if exists else tools.create_asset(
        f"MI_GS_{name}_In", MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if not mi:
        return 0
    mel.set_material_instance_parent(mi, outdoor)
    if mi.get_editor_property("parent") != outdoor:
        master = eal.load_asset(f"{MAT}/M_GS_{spec[5]}")
        if not master:
            return 0
        _fill_instance(mi, master, spec)
        log(f"MI_GS_{name}_In: fikk ikke instans som forelder, bruker hovedmaterialet")
    mel.set_material_instance_scalar_parameter_value(mi, "Weather", 0.0)
    save_asset(mi)
    return 1


def build_instances(force):
    made = 0
    for name, spec in INSTANCES.items():
        master = spec[5]
        path = f"{MAT}/MI_GS_{name}"
        exists = eal.does_asset_exist(path)
        if not exists or force:
            mi = eal.load_asset(path) if exists else tools.create_asset(
                f"MI_GS_{name}", MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            parent = eal.load_asset(f"{MAT}/M_GS_{master}")
            if not mi or not parent:
                continue
            _fill_instance(mi, parent, spec)
            save_asset(mi)
            made += 1
        if master == "Surface":
            try:
                made += _ensure_indoor(name, spec, force)
            except Exception as e:  # never block the rest
                log(f"MI_GS_{name}_In feilet: {e}")
    for name, master, extra in (("Glass", "Glass", None), ("GlassDark", "Glass", (0.02, 0.03, 0.035)),
                                ("Leaves", "Foliage", None), ("LeavesAutumn", "Foliage", (1.8, 1.1, 0.4)),
                                ("Needles", "Foliage", (0.55, 0.75, 0.6))):
        path = f"{MAT}/MI_GS_{name}"
        if eal.does_asset_exist(path) and not force:
            continue
        mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
            f"MI_GS_{name}", MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        parent = eal.load_asset(f"{MAT}/M_GS_{master}")
        if not mi or not parent:
            continue
        mel.set_material_instance_parent(mi, parent)
        if extra:
            mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(extra[0], extra[1], extra[2], 1))
        save_asset(mi)
        made += 1
    if made:
        log(f"{made} materialer laget")


# ------------------------------------------------------------------ apply to models
# Our slot name -> words to look for in scanned materials you added (Poly Haven, Megascans, Fab).
# Scanned materials that REPLACE ours by name (whole material swap). Only for slots with nothing better below.
SCANNED_KEYWORDS = {
    "Copper": ["copper", "patina"],
    "Brick": ["red_brick", "brick_wall"],
    "BrickBrown": ["dark_brick", "brown_brick"],
    "BrickYellow": ["yellow_brick"],
    "Fabric": ["fabric", "denim", "cloth"],
    "Bark": ["bark", "pine_bark", "tree_bark"],
    "M_Wood": ["walnut", "dark_wood", "wood_table"],
    "M_Leather": ["leather"],
    "Leather": ["leather"],
}

# Scanned TEXTURES adopted into our own instances (Poly Haven on Andreas' PC, /Game/PolyHaven/Materials).
# We keep our masters, so world-space tiling on the ground, wet streets, snow and tint still work;
# only the photo textures change. First name that exists wins.
ADOPT = {
    "Stone": ["mi_granite_wall", "mi_stone_wall_04", "mi_castle_wall_slates"],
    "StoneDark": ["mi_stone_wall_04", "mi_castle_wall_slates", "mi_granite_wall"],
    "Trim": ["mi_granite_tile", "mi_grey_plaster"],
    "Slate": ["mi_roof_slates_02"],
    "Plaster": ["mi_painted_plaster_wall", "mi_grey_plaster"],
    "PlasterWhite": ["mi_white_plaster_rough_01", "mi_painted_plaster_wall"],
    "Marble": ["mi_marble_01", "mi_marble_tiles"],
    "Terrazzo": ["mi_terrazzo_tiles"],
    "Floor": ["mi_herringbone_parquet", "mi_wood_floor", "mi_laminate_floor"],
    "Metal": ["mi_metal_plate"],
    "Concrete": ["mi_concrete_wall_008"],
    "ConcreteSmooth": ["mi_smooth_concrete_floor"],
    "Asphalt": ["mi_asphalt_02", "mi_clean_asphalt"],
    "Paving": ["mi_brick_pavement", "mi_cobblestone_floor_01"],
    "Soil": ["mi_forest_ground_04"],
    "W_Grass": ["mi_grass_ground", "mi_sparse_grass", "mi_leafy_grass"],
    "W_Asphalt": ["mi_clean_asphalt", "mi_asphalt_02"],
    "W_Paving": ["mi_brick_pavement", "mi_cobblestone_floor_01"],
    "W_Concrete": ["mi_smooth_concrete_floor", "mi_concrete_wall_008"],
}
# Parameter names scanned masters use (Poly Haven M_PH_Base: BaseColor / Normal / ARM).
_SCAN_PARAMS = {
    "BaseColorTex": ["BaseColor", "Base Color", "Albedo", "Diffuse", "Color"],
    "NormalTex": ["Normal", "NormalMap"],
    "ARMTex": ["ARM", "ORM", "Packed"],
}


def adopt_scanned_textures(scanned):
    by_name = {}
    for low, path, is_mi in scanned:
        if is_mi:
            by_name.setdefault(low, path)
    n = 0
    for name, candidates in ADOPT.items():
        src_path = next((by_name[c] for c in candidates if c in by_name), None)
        if not src_path:
            continue
        src = eal.load_asset(src_path)
        mi = eal.load_asset(f"{MAT}/MI_GS_{name}")
        if not src or not mi:
            continue
        changed = False
        got = 0
        for ours, theirs in _SCAN_PARAMS.items():
            tex_val = None
            for pn in theirs:
                try:
                    tex_val = mel.get_material_instance_texture_parameter_value(src, pn)
                except Exception:
                    tex_val = None
                if tex_val:
                    break
            if not tex_val:
                continue
            got += 1
            cur = mel.get_material_instance_texture_parameter_value(mi, ours)
            if not cur or cur.get_path_name() != tex_val.get_path_name():
                mel.set_material_instance_texture_parameter_value(mi, ours, tex_val)
                changed = True
        if changed and got:
            # photo textures carry their own colour: neutral tint, no extra roughness scaling
            mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(1, 1, 1, 1))
            mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", 1.0)
            save_asset(mi)
            n += 1
            log(f"{name}: bruker teksturene fra {src_path}")
    if n:
        log(f"{n} materialer bruker nå skannede teksturer")


def _scanned_materials():
    """Material instances / materials outside our own folders (Poly Haven, Megascans, Fab)."""
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    found = []
    for cls in ("MaterialInstanceConstant", "Material"):
        flt = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", cls)],
                              package_paths=["/Game"], recursive_paths=True)
        for data in reg.get_assets(flt):
            path = str(data.package_name)
            if path.startswith(MODELS) or path.startswith(GS) or "/Python" in path:
                continue
            found.append((str(data.asset_name).lower(), f"{path}.{data.asset_name}", cls == "MaterialInstanceConstant"))
    return found


_COLOR_SLOT = re.compile(r"^M_C_(\d\d)(\d\d)(\d\d)_(\d)$")


def ensure_color_instance(base):
    """Items made with col((r, g, b), metallic) get slot M_C_rrggbb_m (0-99 per channel). Give each its own
    textured instance: plastic micro detail for paint/plastic, brushed metal when metallic."""
    m = _COLOR_SLOT.match(base)
    if not m:
        return None
    r, g, b, met = (int(x) for x in m.groups())
    name = f"MI_GS_{base[2:]}"
    path = f"{MAT}/{name}"
    if eal.does_asset_exist(path):
        return f"{path}.{name}"
    metallic = met >= 5
    texset, base_val = ("Metal", 0.56) if metallic else ("Plastic", 0.75)
    mi = tools.create_asset(name, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    parent = eal.load_asset(f"{MAT}/M_GS_Surface")
    if not mi or not parent:
        return None
    mel.set_material_instance_parent(mi, parent)
    for param, suffix in (("BaseColorTex", "_D"), ("NormalTex", "_N"), ("ARMTex", "_ARM")):
        t = tex(f"T_GS_{texset}{suffix}")
        if t:
            mel.set_material_instance_texture_parameter_value(mi, param, t)
    mel.set_material_instance_vector_parameter_value(
        mi, "Tint", unreal.LinearColor(r / 99 / base_val, g / 99 / base_val, b / 99 / base_val, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Tiling", 1.2)
    mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", 0.8 if metallic else 1.0)
    mel.set_material_instance_scalar_parameter_value(mi, "MetallicScale", 1.0 if metallic else 0.0)
    save_asset(mi)
    return f"{path}.{name}"


# Furniture that only ever stands indoors: every Surface slot on these models gets the _In instance.
# (Building models mark their indoor slots themselves, with names that end in _In.)
INDOOR_MODELS = {"SM_Prop_" + n for n in (
    "Desk", "Chair", "OfficeChair", "PC", "Printer3D", "Locker", "Bookshelf", "Whiteboard", "Plant", "VendingMachine",
    "CoffeeMachine", "Sofa", "TrashBin", "FireExtinguisher", "FilingCabinet", "ServerRack", "LabBench", "Stool",
    "CafeTable", "LectureSeats", "Lectern", "Projector")}


def pick_material(slot, scanned, indoor=False):
    base = slot.split(".")[0]
    if base.endswith("_In"):
        base, indoor = base[:-3], True
    if base in ("M_Glass", "M_Lens"):
        g = "Glass" if base == "M_Glass" else "GlassDark"
        return f"{MAT}/MI_GS_{g}.MI_GS_{g}"
    if base.startswith("M_C_"):
        return ensure_color_instance(base)
    for word in SCANNED_KEYWORDS.get(base, []):
        hits = [m for m in scanned if word in m[0]]
        if hits:
            hits.sort(key=lambda m: (not m[2], len(m[0])))
            return hits[0][1]
    if indoor and base in INSTANCES and INSTANCES[base][5] == "Surface" and eal.does_asset_exist(f"{MAT}/MI_GS_{base}_In"):
        return f"{MAT}/MI_GS_{base}_In.MI_GS_{base}_In"
    if base in INSTANCES or base in ("Glass", "GlassDark", "Leaves", "LeavesAutumn", "Needles"):
        return f"{MAT}/MI_GS_{base}.MI_GS_{base}"
    return None


def _apply_one(name, short, scanned):
    mesh = eal.load_asset(name)
    if not isinstance(mesh, unreal.StaticMesh):
        return False
    changed = False
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        slot = str(sm.get_editor_property("material_slot_name"))
        path = pick_material(slot, scanned, short in INDOOR_MODELS)
        if not path:
            continue
        cur = sm.get_editor_property("material_interface")
        if cur and cur.get_path_name() == path:
            continue
        mat = eal.load_asset(path)
        if mat:
            mesh.set_material(i, mat)
            changed = True
    if changed:
        save_asset(mesh)
    return changed


def apply_to_models(only=None):
    scanned = _scanned_materials()
    n = 0
    for name in eal.list_assets(MODELS, recursive=False):
        short = name.split("/")[-1].split(".")[0]
        if only is not None and short not in only:
            continue
        if _guard(f"materialer på {short}", _apply_one, name, short, scanned):
            n += 1
    if n:
        log(f"materialer lagt på {n} modeller ({len(scanned)} skannede materialer funnet)")


# ------------------------------------------------------------------ physical materials (runde 5a, kontroll 4b funn 2)
# PM_GS_<Norwegian>[_<English>]: GSAudio::SurfaceAt reads the Norwegian word (Stein, Tre, Stal, Betong, Stoff, Glass,
# Gips, Lino, Gress, Jord), Interaktiv's GSBreakable the English one (Wood, Metal, Concrete, Plaster, Glass).
# Set on every MI_GS_<Name> and MI_GS_<Name>_In, so a hit on a merged Blender part (complex collision) gets the
# physical material of the triangle it hit, and a cube ISM the one of its material: bullet holes, impact sound and
# footsteps follow the material slot.
PHYS = GS + "/Physics"
PHYS_MATS = {  # name: (friction, density g/cm3)
    "PM_GS_Stein": (0.7, 2.4), "PM_GS_Betong_Concrete": (0.8, 2.4), "PM_GS_Gips_Plaster": (0.6, 0.9),
    "PM_GS_Tre_Wood": (0.6, 0.7), "PM_GS_Stal_Metal": (0.4, 7.8), "PM_GS_Stoff": (0.9, 0.3),
    "PM_GS_Glass": (0.3, 2.5), "PM_GS_Lino": (0.8, 1.3), "PM_GS_Gress": (0.9, 1.0), "PM_GS_Jord": (0.9, 1.6),
}
PM_FOR = {}
for _pm, _names in (
        ("PM_GS_Stein", ("Brick", "BrickBrown", "BrickYellow", "Stone", "StoneDark", "Slate", "Marble", "Terrazzo",
                         "Asphalt", "Paving", "W_Asphalt", "W_Paving")),
        ("PM_GS_Betong_Concrete", ("Concrete", "ConcreteSmooth", "W_Concrete")),
        ("PM_GS_Gips_Plaster", ("Plaster", "PlasterWhite", "Trim", "Plastic", "PlasticWhite", "Polymer", "LightPanel",
                                "RobotWhite", "M_White", "M_Paper", "M_BlackPolymer", "M_Bakelite", "Plate", "ScreenOn", "Bulb")),
        ("PM_GS_Tre_Wood", ("Wood", "WoodLight", "M_Wood", "M_LightWood", "Bark", "BirchBark")),
        ("PM_GS_Stal_Metal", ("Copper", "Metal", "MetalDark", "Steel", "Gunmetal", "PaintRed", "PaintBlue", "PaintYellow",
                              "PaintGreen", "PaintOrange", "NTNUBlue", "HelgYellow", "PaintWhite", "RobotBody",
                              "CarPaint_Red", "CarPaint_Black", "CarPaint_White", "CarPaint_Silver", "CarPaint_Blue",
                              "CarPaint_Grey", "CarPaint_Green", "CarPaint", "M_Steel", "M_DarkSteel", "M_Chrome",
                              "M_Blade", "M_BladeDark", "M_Gold", "M_Brass", "M_OliveDrab", "M_Tan", "M_Red")),
        ("PM_GS_Stoff", ("Fabric", "Tarp", "FabricRed", "Leather", "M_Leather", "M_KatanaWrap", "M_Fabric")),
        ("PM_GS_Glass", ("Glass", "GlassDark")),
        ("PM_GS_Lino", ("Floor", "HelgOrange", "Rubber", "M_Rubber")),
        ("PM_GS_Gress", ("W_Grass", "Leaves", "LeavesAutumn", "Needles")),
        ("PM_GS_Jord", ("Gravel", "Soil", "W_Gravel"))):
    for _n in _names:
        PM_FOR[_n] = _pm


def _ensure_pm(name):
    path = f"{PHYS}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    pm = tools.create_asset(name, PHYS, unreal.PhysicalMaterial, unreal.PhysicalMaterialFactoryNew())
    if not pm:
        return None
    fr, dens = PHYS_MATS[name]
    pm.set_editor_property("friction", fr)
    pm.set_editor_property("density", dens)
    save_asset(pm)
    log(f"{name} laget")
    return pm


def _set_pm(mi_name, pm):
    path = f"{MAT}/{mi_name}"
    if not eal.does_asset_exist(path):
        return 0
    mi = eal.load_asset(path)
    if not mi:
        return 0
    cur = mi.get_editor_property("phys_material")
    if cur and cur.get_name() == pm.get_name():
        return 0
    mi.set_editor_property("phys_material", pm)
    save_asset(mi)
    return 1


def ensure_physical_materials():
    pms = {}
    for name in PHYS_MATS:
        pm = _guard(f"fysisk materiale {name}", _ensure_pm, name)
        if pm:
            pms[name] = pm
    n = 0
    for mi, pm_name in PM_FOR.items():
        pm = pms.get(pm_name)
        if not pm:
            continue
        for suffix in ("", "_In"):
            n += _guard(f"PM på MI_GS_{mi}{suffix}", _set_pm, f"MI_GS_{mi}{suffix}", pm) or 0
    if n:
        log(f"fysiske materialer (PM_GS_*) satt på {n} materialer")


def phys_material_of(mat):
    """Name of the physical material a material gives (for the import report), or '-'."""
    try:
        pm = mat.get_physical_material() if mat else None
        return pm.get_name() if pm else "-"
    except Exception:
        return "-"


# ------------------------------------------------------------------ the map
def ensure_map():
    """Creates /Game/Maps/Gloshaugen with editor-preview lighting if it does not exist.
    The game removes these lights at start and spawns its own sky, weather and day/night."""
    path = "/Game/Maps/Gloshaugen"
    if eal.does_asset_exist(path):
        return
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not les.new_level(path):
        return
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(0, -40, 35))
    if sun:
        c = sun.get_component_by_class(unreal.DirectionalLightComponent)
        c.set_editor_property("atmosphere_sun_light", True)
        c.set_editor_property("intensity", 10.0)
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    if sky:
        sc = sky.get_component_by_class(unreal.SkyLightComponent)
        sc.set_editor_property("real_time_capture", True)
        sc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    eas.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(0, 0, 0))
    eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, -4000, 200))
    les.save_current_level()
    log("kartet Content/Maps/Gloshaugen er laget")


def run():
    import_textures()
    forced = build_masters()
    try:
        # the blood material is built once on its own (not part of VERSION, so the others are not rebuilt for it)
        if not eal.does_asset_exist(f"{MAT}/M_GS_Blood"):
            log("bygger blodmateriale")
            _blood_master()
    except Exception as e:  # never block the rest
        log(f"M_GS_Blood feilet: {e}")
    _guard("materialinstanser", build_instances, forced)
    _guard("fysiske materialer", ensure_physical_materials)
    try:
        adopt_scanned_textures(_scanned_materials())
    except Exception as e:  # never block the rest
        unreal.log_warning(f"Gloshaugen: skannede teksturer feilet: {e}")
    _guard("materialer på modellene", apply_to_models)
