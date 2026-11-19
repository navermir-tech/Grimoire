# 초원 꾸밈 1차 묶음 에셋(docs/systems/meadow-dressing.md) — /Game/Grimoire/Dressing 에 우리 에셋만 만든다. 팩은 참조만.
#
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup -P art/tools/blender_meadow_kit.py -- Saved/MeadowKit
#   python art/tools/make_meadow_textures.py
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Dressing\make_meadow_assets.py" -unattended -nop4 -nosplash -AllowCommandletRendering
#
# 만드는 것(다시 돌리면 덮어쓴다 — 값은 이 파일이 정본):
#  - Meshes   SM_Meadow_*            블렌더 메시(Saved/MeadowKit/*.fbx) — 등잔 · 솜꽃 줄기 · 솜 · 곁가지 셋 · 재 자국 · 장작
#  - Materials M_Meadow_Painted(색 하나) · M_Meadow_FloatStone(떠서 도는 돌, WPO) · M_Meadow_Butterfly(날갯짓 스프라이트) · M_Meadow_Puff
#             MI_Meadow_*            팩 재질의 색만 바꾼 인스턴스(마른 풀 · 시든 꽃 · 꽃밭 넷 · 꽃나무 셋 · 돌 · 나무)
#  - FX       EFT_Grim_Ambient(환경 이펙트 — 성능 규칙 24, BUG-006) · NS_Meadow_Butterflies(낮 나비) · NS_Meadow_CottonBurst(솜 흩날림)
#  - Foliage  FT_Meadow_Blossom*(외딴 꽃나무 D26) · FT_Meadow_Flower*(색 꽃밭 D27)
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(HERE, "..", "Terrain"))
sys.path.insert(0, os.path.join(HERE, "..", "MagicFX"))
import dress_assets as da  # noqa: E402  — 재질 노드 도우미(_expr · _connect · _scalar · _vec · _bin · _mask)

ROOT = "/Game/Grimoire/Dressing"
MESH_DIR = ROOT + "/Meshes"
MAT_DIR = ROOT + "/Materials"
TEX_DIR = ROOT + "/Textures"
FX_DIR = ROOT + "/FX"
FOL_DIR = ROOT + "/Foliage"
EFT_PATH = "/Game/Grimoire/Environment/FX/EFT_Grim_Ambient"
KIT_DIR = os.path.join(ROOT_DIR, "Saved", "MeadowKit")
TEX_SRC = os.path.join(ROOT_DIR, "art", "meadow", "textures")

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
SH_FOREST = da.SH_FOREST
SH_GRASS = da.SH_GRASS
SH_ROCKS = da.SH_ROCKS
FAILS = []


def log(msg):
    unreal.log(f"make_meadow_assets: {msg}")


def warn(msg):
    FAILS.append(msg)
    unreal.log_warning(f"make_meadow_assets: !! {msg}")


def load(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"에셋이 없다 — {path} (워크트리면 Content/<팩> 정션, docs/dev/market-assets.md)")
    return asset


def existing_or_new(folder, name, cls, factory):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    return asset_tools.create_asset(name, folder, cls, factory)


# ───── 메시(블렌더 FBX) ─────
MESH_MATERIALS = {   # 메시: 재질(아래에서 만든다)
    "SM_Meadow_StoneLamp": "MI_Meadow_LampStone",
    "SM_Meadow_CottonStems": "MI_Meadow_CottonStem",
    "SM_Meadow_CottonPuffs": "MI_Meadow_CottonPuff",
    "SM_Meadow_Twig1": "MI_Meadow_Wood", "SM_Meadow_Twig2": "MI_Meadow_Wood", "SM_Meadow_Twig3": "MI_Meadow_Wood",
    "SM_Meadow_AshPatch": "MI_Meadow_Ash",
    "SM_Meadow_FireLogs": "MI_Meadow_Char",
}


def import_meshes(materials):
    if not os.path.isdir(KIT_DIR):
        raise RuntimeError(f"{KIT_DIR} 이 없다 — 먼저 블렌더로 art/tools/blender_meadow_kit.py 를 돌린다")
    tasks = []
    for name in MESH_MATERIALS:
        fbx = os.path.join(KIT_DIR, name + ".fbx")
        if not os.path.exists(fbx):
            raise RuntimeError(f"{fbx} 가 없다")
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", fbx)
        t.set_editor_property("destination_path", MESH_DIR)
        t.set_editor_property("destination_name", name)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("automated", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    asset_tools.import_asset_tasks(tasks)
    out = {}
    for name, mat in MESH_MATERIALS.items():
        path = f"{MESH_DIR}/{name}"
        mesh = eal.load_asset(path)
        if not isinstance(mesh, unreal.StaticMesh):
            warn(f"메시를 못 들여옴: {path}")
            continue
        # static_materials 배열의 원소를 고쳐 다시 넣는 길은 복사본만 바뀌어 칸이 비었다(2026-11-18) — set_material 로
        mesh.set_material(0, materials[mat])
        # 소품 메시는 충돌을 쓰지 않는다(액터의 HitBox · ISM 은 충돌 없음)
        unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
        nanite = mesh.get_editor_property("nanite_settings")
        nanite.set_editor_property("enabled", False)
        mesh.set_editor_property("nanite_settings", nanite)
        eal.save_loaded_asset(mesh)
        b = mesh.get_bounds()
        slot = mesh.get_material(0)
        if slot is None or slot.get_name() != mat:
            warn(f"{name}: 재질 칸이 {slot} — {mat} 이어야 한다")
        log(f"메시 {name} 크기(cm) {2 * b.box_extent.x:.0f}×{2 * b.box_extent.y:.0f}×{2 * b.box_extent.z:.0f}")
        out[name] = mesh
    # FBX 가 같이 만든 빈 재질(칸 이름) 지우기
    for asset_path in eal.list_assets(MESH_DIR, False, False):
        asset = eal.load_asset(asset_path)
        if isinstance(asset, (unreal.Material, unreal.MaterialInstance, unreal.Texture)):
            eal.delete_asset(asset_path.split(".")[0])
    return out


# ───── 텍스처 ─────
def import_textures():
    tasks = []
    for name in sorted(os.listdir(TEX_SRC)):
        if name.endswith(".png"):
            t = unreal.AssetImportTask()
            t.set_editor_property("filename", os.path.join(TEX_SRC, name))
            t.set_editor_property("destination_path", TEX_DIR)
            t.set_editor_property("replace_existing", True)
            t.set_editor_property("automated", True)
            t.set_editor_property("save", False)
            tasks.append(t)
    asset_tools.import_asset_tasks(tasks)
    out = {}
    for t in tasks:
        for path in t.get_editor_property("imported_object_paths"):
            tex = unreal.load_asset(path)
            if isinstance(tex, unreal.Texture2D):
                tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
                tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
                tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
                eal.save_loaded_asset(tex)
                out[tex.get_name()] = tex
    log(f"텍스처 {sorted(out)}")
    return out


# ───── 재질 ─────
def fresh_material(name):
    m = existing_or_new(MAT_DIR, name, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    return m


def make_painted():
    """색 하나 · 거칠기 · 은은한 빛 — 블렌더 소품(솜 · 줄기 · 재 · 숯)"""
    m = fresh_material("M_Meadow_Painted")
    color = da._vec(m, "Color", (0.8, 0.8, 0.8), -600, 0)
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(da._scalar(m, "Roughness", 0.9, -400, 200), "", unreal.MaterialProperty.MP_ROUGHNESS)
    glow = da._bin(m, unreal.MaterialExpressionMultiply, color, da._scalar(m, "EmissiveStrength", 0.0, -600, 320), -200, 300)
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def make_float_stone():
    """떠 있는 돌(D29): 팩 돌 텍스처 + 위아래로 천천히 떠오르며 돈다(WPO — 틱 없음). 자리마다 위상이 다르다(물체 위치)"""
    m = fresh_material("M_Meadow_FloatStone")
    tex = da._expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, -200)
    tex.set_editor_property("parameter_name", "BaseTexture")
    tex.set_editor_property("texture", load(f"{SH_ROCKS}/Textures/T_Stone_Small_1_BC"))
    tint = da._vec(m, "Tint", (0.95, 0.95, 1.0), -900, 60)
    base = da._bin(m, unreal.MaterialExpressionMultiply, tex, tint, -500, -100, "RGB", "")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(da._scalar(m, "Roughness", 0.85, -500, 120), "", unreal.MaterialProperty.MP_ROUGHNESS)
    # 은은한 푸른 기(마법) — 아래쪽만
    glow = da._bin(m, unreal.MaterialExpressionMultiply, da._vec(m, "GlowColor", (0.15, 0.3, 0.8), -900, 260), da._scalar(m, "GlowStrength", 0.15, -900, 400), -500, 300)
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    obj = da._expr(m, unreal.MaterialExpressionObjectPositionWS, -1500, 600)
    phase_dir = da._expr(m, unreal.MaterialExpressionConstant3Vector, -1500, 700)
    phase_dir.set_editor_property("constant", unreal.LinearColor(0.013, 0.017, 0.0, 0.0))
    phase = da._bin(m, unreal.MaterialExpressionDotProduct, obj, phase_dir, -1300, 650)
    time = da._expr(m, unreal.MaterialExpressionTime, -1500, 850)
    t_bob = da._bin(m, unreal.MaterialExpressionMultiply, time, da._scalar(m, "BobSpeed", 0.9, -1500, 930), -1300, 850)
    sin = da._expr(m, unreal.MaterialExpressionSine, -1000, 850)
    da._connect(da._bin(m, unreal.MaterialExpressionAdd, t_bob, phase, -1150, 850), "", sin, "")
    bob = da._bin(m, unreal.MaterialExpressionMultiply, sin, da._scalar(m, "BobHeight", 7.0, -1000, 950), -850, 850)
    zero = da._expr(m, unreal.MaterialExpressionConstant2Vector, -850, 760)
    bob3 = da._bin(m, unreal.MaterialExpressionAppendVector, zero, bob, -700, 800)
    # 돌기: 물체 가운데를 축으로(바퀴/초)
    spin = da._bin(m, unreal.MaterialExpressionMultiply, time, da._scalar(m, "SpinTurnsPerSecond", 0.04, -1500, 1100), -1300, 1050)
    angle = da._bin(m, unreal.MaterialExpressionAdd, spin, phase, -1150, 1050)
    rot = da._expr(m, unreal.MaterialExpressionRotateAboutAxis, -900, 1050)
    axis = da._expr(m, unreal.MaterialExpressionConstant3Vector, -1150, 1150)
    axis.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 0.0))
    da._connect(axis, "", rot, "NormalizedRotationAxis")
    da._connect(angle, "", rot, "RotationAngle")
    da._connect(obj, "", rot, "PivotPoint")
    da._connect(da._expr(m, unreal.MaterialExpressionWorldPosition, -1150, 1250), "", rot, "Position")
    wpo = da._bin(m, unreal.MaterialExpressionAdd, rot, bob3, -500, 900)
    mel.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def make_butterfly(tex):
    """나비 스프라이트: 마스크 · 양면, 날개 폭을 시간에 따라 접었다 편다(U 를 가운데 기준으로 늘림). 색은 파티클 난수로 셋 중 하나쯤"""
    m = fresh_material("M_Meadow_Butterfly")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_niagara_sprites", True)
    rnd = da._expr(m, unreal.MaterialExpressionParticleRandom, -1700, 300)
    time = da._expr(m, unreal.MaterialExpressionTime, -1700, 150)
    flap_t = da._bin(m, unreal.MaterialExpressionMultiply, time, da._scalar(m, "FlapSpeed", 16.0, -1700, 220), -1500, 150)
    phase = da._bin(m, unreal.MaterialExpressionMultiply, rnd, da._scalar(m, "PhaseSpread", 6.283, -1700, 380), -1500, 300)
    sin = da._expr(m, unreal.MaterialExpressionSine, -1200, 200)
    da._connect(da._bin(m, unreal.MaterialExpressionAdd, flap_t, phase, -1350, 200), "", sin, "")
    width = da._expr(m, unreal.MaterialExpressionAbs, -1050, 200)
    da._connect(sin, "", width, "")
    width = da._bin(m, unreal.MaterialExpressionMax, width, da._scalar(m, "MinWidth", 0.18, -1050, 300), -900, 220)
    uv = da._expr(m, unreal.MaterialExpressionTextureCoordinate, -1500, -100)
    u = da._mask(m, uv, "r", -1300, -150)
    v = da._mask(m, uv, "g", -1300, -50)
    half = da._expr(m, unreal.MaterialExpressionConstant, -1300, 40)
    half.set_editor_property("r", 0.5)
    u2 = da._bin(m, unreal.MaterialExpressionAdd, da._bin(m, unreal.MaterialExpressionDivide, da._bin(m, unreal.MaterialExpressionSubtract, u, half, -1100, -150), width, -800, -150), half, -650, -150)
    uv2 = da._bin(m, unreal.MaterialExpressionAppendVector, u2, v, -500, -100)
    sample = da._expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -350, -100)
    sample.set_editor_property("parameter_name", "WingTexture")
    sample.set_editor_property("texture", tex)
    sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    da._connect(uv2, "", sample, "UVs")
    # 색: 귤빛 → 노랑 → 흰빛
    t1 = da._expr(m, unreal.MaterialExpressionSaturate, -900, 450)
    da._connect(da._bin(m, unreal.MaterialExpressionMultiply, rnd, da._scalar(m, "Two", 2.0, -1100, 520), -1000, 450), "", t1, "")
    t2 = da._expr(m, unreal.MaterialExpressionSaturate, -900, 600)
    da._connect(da._bin(m, unreal.MaterialExpressionSubtract, da._bin(m, unreal.MaterialExpressionMultiply, rnd, da._scalar(m, "Two2", 2.0, -1100, 680), -1000, 600),
                        da._scalar(m, "One", 1.0, -1100, 760), -950, 650), "", t2, "")
    c1 = da._vec(m, "ColorA", (1.0, 0.45, 0.08), -900, 750)
    c2 = da._vec(m, "ColorB", (1.0, 0.85, 0.2), -900, 850)
    c3 = da._vec(m, "ColorC", (0.95, 0.95, 1.0), -900, 950)
    tint = da._lerp(m, da._lerp(m, c1, c2, t1, -600, 800), c3, t2, -450, 850)
    color = da._bin(m, unreal.MaterialExpressionMultiply, sample, tint, -150, 100, "RGB", "")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(da._bin(m, unreal.MaterialExpressionMultiply, color, da._scalar(m, "Glow", 0.25, -150, 300), 50, 250), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.connect_material_property(da._scalar(m, "Roughness", 0.8, -150, 400), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def make_puff(tex):
    """솜 스프라이트: 반투명 · 빛 안 받음, 파티클 색 × 텍스처"""
    m = fresh_material("M_Meadow_Puff")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("used_with_niagara_sprites", True)
    sample = da._expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    sample.set_editor_property("parameter_name", "PuffTexture")
    sample.set_editor_property("texture", tex)
    pc = da._expr(m, unreal.MaterialExpressionParticleColor, -600, 250)
    emissive = da._bin(m, unreal.MaterialExpressionMultiply, da._bin(m, unreal.MaterialExpressionMultiply, sample, pc, -350, 0, "RGB", "RGB"),
                       da._scalar(m, "Brightness", 1.1, -350, 150), -150, 50)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(da._bin(m, unreal.MaterialExpressionMultiply, sample, pc, -150, 250, "A", "A"), "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def mi(name, parent, vectors=None, scalars=None, switches=None):
    inst = existing_or_new(MAT_DIR, name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(inst, parent)
    # set_* 의 돌려준 값은 믿을 수 없다(5.8 에서 늘 False) — 이름이 부모에 있는지로 본다
    vector_names = [str(n) for n in mel.get_vector_parameter_names(inst)]
    scalar_names = [str(n) for n in mel.get_scalar_parameter_names(inst)]
    for param, rgb in (vectors or {}).items():
        a = rgb[3] if len(rgb) > 3 else 1.0
        if param not in vector_names:
            warn(f"{name}: 벡터 파라미터 {param} 없음({vector_names})")
        mel.set_material_instance_vector_parameter_value(inst, param, unreal.LinearColor(rgb[0], rgb[1], rgb[2], a))
    for param, value in (scalars or {}).items():
        if param not in scalar_names:
            warn(f"{name}: 스칼라 파라미터 {param} 없음({scalar_names})")
        mel.set_material_instance_scalar_parameter_value(inst, param, value)
    for param, on in (switches or {}).items():
        mel.set_material_instance_static_switch_parameter_value(inst, param, on)
    if switches:
        mel.update_material_instance(inst)
    eal.save_loaded_asset(inst)
    return inst


# 외딴 꽃나무(D26) — 구역마다 한 색(분포 규칙 「색 예산」). StyleHex 활엽수 잎 재질의 색만
BLOSSOMS = {   # 이름: (Color Base, Color Top, Subsurface)
    "Pink": ((0.42, 0.10, 0.20), (1.00, 0.52, 0.68), (0.35, 0.08, 0.16)),
    "White": ((0.45, 0.45, 0.42), (1.00, 0.97, 0.93), (0.30, 0.30, 0.28)),
    "Yellow": ((0.42, 0.28, 0.02), (1.00, 0.82, 0.20), (0.32, 0.22, 0.03)),
}
# 색 꽃밭(D27) — 꽃 아틀라스의 Custom Color(곱하기)
FLOWERS = {
    "Pink": (1.0, 0.42, 0.66),
    "Yellow": (1.0, 0.80, 0.18),
    "White": (1.0, 0.98, 0.94),
    "Blue": (0.45, 0.60, 1.0),
}


def make_materials():
    out = {}
    painted = make_painted()
    out["MI_Meadow_CottonPuff"] = mi("MI_Meadow_CottonPuff", painted, {"Color": (0.93, 0.93, 0.88)}, {"Roughness": 1.0, "EmissiveStrength": 0.12})
    out["MI_Meadow_CottonStem"] = mi("MI_Meadow_CottonStem", painted, {"Color": (0.16, 0.26, 0.07)}, {"Roughness": 0.85})
    out["MI_Meadow_Ash"] = mi("MI_Meadow_Ash", painted, {"Color": (0.045, 0.04, 0.035)}, {"Roughness": 1.0})
    out["MI_Meadow_Char"] = mi("MI_Meadow_Char", painted, {"Color": (0.09, 0.055, 0.03)}, {"Roughness": 0.95})
    out["MI_Meadow_LampStone"] = mi("MI_Meadow_LampStone", load(f"{SH_ROCKS}/Materials/Mat_Stones/MI_Stone_Small_1"))
    out["MI_Meadow_Wood"] = mi("MI_Meadow_Wood", load(f"{SH_FOREST}/Materials/Mat_Trees/MI_Tree_Broadleaf_Trunk"))
    out["MI_Meadow_DryGrass"] = mi("MI_Meadow_DryGrass", load(f"{SH_GRASS}/Materials/Mat_Foliage/MI_Fol_Grass_AlphaCut"),
                                   {"Top Color": (0.66, 0.50, 0.17), "Base Color": (0.30, 0.20, 0.07)})
    flower_parent = load(f"{SH_FOREST}/Materials/Mat_Foliage/MI_Fol_Flower_Atlas")
    # 꽃 아틀라스의 Custom Color 는 「Desaturate Texture?」 를 켜야 꽃잎까지 물든다(끄면 잎 쪽만 — 첫 그림에서 색이 안 바뀌었다)
    tint = {"Desaturate Texture?": True}
    out["MI_Meadow_Wilted"] = mi("MI_Meadow_Wilted", flower_parent, {"Custom Color": (0.55, 0.36, 0.14)}, {"Texture Brightness": 0.8}, tint)   # 갈색으로 바랜 꽃(첫 그림에서 피기 전후가 잘 안 갈렸다)
    for color, rgb in FLOWERS.items():
        out[f"MI_Meadow_Flower_{color}"] = mi(f"MI_Meadow_Flower_{color}", flower_parent, {"Custom Color": rgb}, {"Texture Brightness": 1.15}, tint)
    leaf_parent = load(f"{SH_FOREST}/Materials/Mat_Trees/MI_Tree_Leaves_Broadleaf")
    for color, (base, top, sss) in BLOSSOMS.items():
        out[f"MI_Meadow_Blossom_{color}"] = mi(f"MI_Meadow_Blossom_{color}", leaf_parent, {"Color Base": base, "Color Top": top, "Subsurface Color": sss})
    out["M_Meadow_FloatStone"] = make_float_stone()
    return out


# ───── 이펙트 ─────
EFT_VALUES = {
    "update_frequency": "LOW",
    "cull_reaction": "DEACTIVATE_IMMEDIATE_RESUME",   # 멀어지면 끄고 다가오면 다시 켠다(늘 도는 환경 이펙트)
    "max_distance": 6000.0,                            # 60 m 밖의 환경 이펙트는 그리지 않는다(나비 · 솜)
    "max_instances": 40,
    "max_system_instances": 24,
}


def ensure_ambient_effect_type():
    if eal.does_asset_exist(EFT_PATH):
        eft = eal.load_asset(EFT_PATH)
    else:
        folder, name = EFT_PATH.rsplit("/", 1)
        eft = asset_tools.create_asset(name, folder, unreal.NiagaraEffectType, unreal.NiagaraEffectTypeFactoryNew())
    eft.set_editor_property("update_frequency", getattr(unreal.NiagaraScalabilityUpdateFrequency, EFT_VALUES["update_frequency"]))
    eft.set_editor_property("cull_reaction", getattr(unreal.NiagaraCullReaction, EFT_VALUES["cull_reaction"]))
    if not unreal.GrimMagicFXEditorLibrary.set_distance_significance(eft):
        warn("EFT 중요도 처리기를 못 붙임")
    arr = eft.get_editor_property("system_scalability_settings")
    settings = list(arr.get_editor_property("settings")) or [unreal.NiagaraSystemScalabilitySettings()]
    for s in settings:
        s.set_editor_property("cull_by_distance", True)
        s.set_editor_property("max_distance", EFT_VALUES["max_distance"])
        s.set_editor_property("cull_max_instance_count", True)
        s.set_editor_property("max_instances", EFT_VALUES["max_instances"])
        s.set_editor_property("cull_per_system_max_instance_count", True)
        s.set_editor_property("max_system_instances", EFT_VALUES["max_system_instances"])
    arr.set_editor_property("settings", settings)
    eft.set_editor_property("system_scalability_settings", arr)
    eal.save_loaded_asset(eft)
    log(f"Effect Type {EFT_PATH}: 거리 컬 {EFT_VALUES['max_distance']:.0f} cm · 전체 {EFT_VALUES['max_instances']} · 시스템당 {EFT_VALUES['max_system_instances']}")
    return eft


def make_fx(butterfly_mat, puff_mat, eft):
    import fx_niagara_builder as fx
    fx.MODULES.setdefault("CurlNoise", "/Niagara/Modules/Update/Forces/CurlNoiseForce.CurlNoiseForce")
    fade = "saturate(min(Particles.NormalizedAge * 6.0, (1.0 - Particles.NormalizedAge) * 6.0))"
    # 낮 나비: 곳당 늘 8 마리쯤(초당 1.3 × 수명 5~7초) — 입자 곳당 16 이하(성능 규칙 24)
    butterflies = [{
        "template": "Fountain", "name": "Butterflies", "loop": "infinite", "rate": 1.3, "lifetime": (5.0, 7.0),
        "size": (15.0, 21.0), "speed": (20.0, 45.0), "cone": 75.0, "axis": (0, 0, 1), "radius": 140.0, "gravity": -2.0, "drag": 0.6,
        "alpha": fade, "material": butterfly_mat.get_path_name(), "local": False,
        "raw": [(fx.PU, "Drag", "Drag", fx.v_float(0.6))],
    }]
    built = fx.build_system(FX_DIR, "NS_Meadow_Butterflies", butterflies)
    # 날갯짓 몸통 흔들림: 컬 노이즈(실패해도 나비는 난다)
    b = fx.SystemBuilder(FX_DIR, "NS_Meadow_Butterflies")
    mod = b.add_module("Butterflies", fx.PU, "CurlNoise")
    if mod:
        b.set("Butterflies", fx.PU, mod, "Noise Strength", fx.v_float(90.0))
        b.set("Butterflies", fx.PU, mod, "Noise Frequency", fx.v_float(0.004))
        b.save()
    # 솜 흩날림: 한 번, 솜 14 송이가 바람 쪽으로 떠오르며 퍼진다
    cotton = [{
        "template": "DirectionalBurst", "name": "Cotton", "loop": "once", "duration": 4.0, "burst": 14, "lifetime": (2.5, 4.0),
        "size": (5.0, 9.0), "speed": (60.0, 150.0), "cone": 55.0, "axis": (0.3, 0, 1), "gravity": 12.0, "drag": 1.4,
        "color": (1.0, 1.0, 0.97, 0.95), "alpha": fade, "material": puff_mat.get_path_name(), "rotation": True, "spin": 40.0,
    }]
    fx.build_system(FX_DIR, "NS_Meadow_CottonBurst", cotton)
    out = {}
    for name in ("NS_Meadow_Butterflies", "NS_Meadow_CottonBurst"):
        system = eal.load_asset(f"{FX_DIR}/{name}")
        if system is None:
            warn(f"Niagara 를 못 만듦: {name}")
            continue
        system.set_editor_property("effect_type", eft)
        eal.save_loaded_asset(system)
        out[name] = system
    if fx.LOG:
        for line in fx.LOG:
            if "!!" in line:
                warn("FX " + line)
    log(f"Niagara {sorted(out)}")
    return out


# ───── 폴리지 타입 ─────
def make_foliage(materials):
    out = {}
    lib = unreal.GrimTerrainEditorLibrary
    trunk = load(f"{SH_FOREST}/Materials/Mat_Trees/MI_Tree_Broadleaf_Trunk")
    tree = load(f"{SH_FOREST}/Meshes/Trees/SM_Tree_Broadleaf_2")
    for color in BLOSSOMS:
        name = f"FT_Meadow_Blossom{color}"
        ft = lib.create_foliage_type_asset(f"{FOL_DIR}/{name}", tree, None, 36000, 48000, True, True)
        ft.set_editor_property("mesh", tree)
        ft.set_editor_property("cull_distance", unreal.Int32Interval(36000, 48000))
        ft.set_editor_property("cast_shadow", True)
        ft.set_editor_property("override_materials", [materials[f"MI_Meadow_Blossom_{color}"], trunk])
        eal.save_loaded_asset(ft)
        out[name] = ft
    flower = load(f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Flower_Clump_1")
    flower2 = load(f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Flower_Clump_3")
    for color in FLOWERS:
        for k, mesh in ((1, flower), (2, flower2)):
            name = f"FT_Meadow_Flower{color}{k}"
            ft = lib.create_foliage_type_asset(f"{FOL_DIR}/{name}", mesh, None, 6300, 9000, False, False)
            ft.set_editor_property("mesh", mesh)
            ft.set_editor_property("cull_distance", unreal.Int32Interval(6300, 9000))
            ft.set_editor_property("cast_shadow", False)
            ft.set_editor_property("override_materials", [materials[f"MI_Meadow_Flower_{color}"]])
            eal.save_loaded_asset(ft)
            out[name] = ft
    log(f"폴리지 타입 {len(out)}")
    return out


def main():
    textures = import_textures()
    materials = make_materials()
    meshes = import_meshes(materials)
    butterfly = make_butterfly(textures["T_Meadow_Butterfly"])
    puff = make_puff(textures["T_Meadow_Puff"])
    eft = ensure_ambient_effect_type()
    make_fx(butterfly, puff, eft)
    make_foliage(materials)
    log(f"메시 {len(meshes)} · 재질 {len(materials)} · 실패 {len(FAILS)}")
    for f in FAILS:
        unreal.log_warning(f"make_meadow_assets: 실패 — {f}")


main()
