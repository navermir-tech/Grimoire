# 마법탑 아트 시범(docs/art/tower-art.md): 자체 제작 텍스처 · 마스터 재질 · 재질 인스턴스 · 블렌더 키트 / Tripo 메시를 언리얼로 가져온다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Environment\tower_art_import.py" -unattended -nop4 -nosplash -nullrhi
#
# 만드는 것(모두 /Game/Grimoire/Environment/ 아래):
#   Materials/Textures/T_GrimEnv_*            art/env/materials/*.png (art/tools/make_env_textures.py 가 만든 절차적 PBR)
#   Materials/M_GrimEnv_Master                 마스터 재질 — 색조 · 채도 · 조각 변이(정점 색 R) · 큰 무늬 · 위면 이끼 · AO 색(푸른 그늘) · 발광
#   Materials/MI_GrimEnv_<슬롯>                슬롯 이름(Stone · Slate · Brass …)마다 인스턴스 — 모든 탑 에셋 색이 여기서 묶인다
#   Tower/<SM_이름>/StaticMeshes/SM_*          art/env/tower-kit/*.glb (art/tools/blender_tower_kit.py), Nanite, 충돌 없음(충돌은 레벨의 숨긴 상자)
# 환경 변수 GRIM_TOWER_ONLY=<이름,이름> 이면 그 메시만 다시 가져온다. GRIM_TOWER_SKIP_MESH=1 이면 재질만.
import glob
import os

import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
TEX_SRC = os.path.join(ROOT, "art", "env", "materials")
KIT_SRC = os.path.join(ROOT, "art", "env", "tower-kit")
TRIPO_SRC = os.path.join(ROOT, "art", "env")
MAT_DIR = "/Game/Grimoire/Environment/Materials"
TEX_DIR = MAT_DIR + "/Textures"
MESH_DIR = "/Game/Grimoire/Environment/Tower"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
ONLY = [s for s in os.environ.get("GRIM_TOWER_ONLY", "").split(",") if s]


def log(msg):
    unreal.log(f"tower_art_import: {msg}")


# ─────────────── 텍스처 ───────────────

def import_textures():
    tasks = []
    for path in sorted(glob.glob(os.path.join(TEX_SRC, "T_GrimEnv_*.png"))):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", path)
        t.set_editor_property("destination_path", TEX_DIR)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    asset_tools.import_asset_tasks(tasks)
    out = {}
    for t in tasks:
        for p in t.get_editor_property("imported_object_paths"):
            tex = unreal.load_asset(p)
            name = tex.get_name()
            if name.endswith("_N"):
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
                tex.set_editor_property("srgb", False)
                tex.set_editor_property("flip_green_channel", True)   # 만든 노멀은 OpenGL(+Y) — 언리얼은 DirectX
            elif name.endswith("_ORM") or name.endswith("_Mask"):
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
                tex.set_editor_property("srgb", False)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
            eal.save_loaded_asset(tex, False)
            out[name] = tex
    log(f"텍스처 {len(out)}")
    return out


# ─────────────── 마스터 재질 ───────────────

def _expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def _connect(a, a_out, b, b_in):
    if not mel.connect_material_expressions(a, a_out, b, b_in):
        raise RuntimeError(f"연결 실패 {a.get_name()}.{a_out} -> {b.get_name()}.{b_in}")


def _scalar(m, name, v, x, y, group="Grim"):
    n = _expr(m, unreal.MaterialExpressionScalarParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", v)
    n.set_editor_property("group", group)
    return n


def _vector(m, name, rgb, x, y, group="Grim"):
    n = _expr(m, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    n.set_editor_property("group", group)
    return n


def _mask(m, src, chans, x, y, out=""):
    n = _expr(m, unreal.MaterialExpressionComponentMask, x, y)
    for c in "rgba":
        n.set_editor_property(c, c in chans)
    _connect(src, out, n, "")
    return n


def _bin(m, cls, a, b, x, y, a_out="", b_out=""):
    n = _expr(m, cls, x, y)
    _connect(a, a_out, n, "A")
    _connect(b, b_out, n, "B")
    return n


def _const(m, v, x, y):
    n = _expr(m, unreal.MaterialExpressionConstant, x, y)
    n.set_editor_property("r", v)
    return n


def _lerp(m, a, b, alpha, x, y, a_out="", b_out="", alpha_out=""):
    n = _expr(m, unreal.MaterialExpressionLinearInterpolate, x, y)
    _connect(a, a_out, n, "A")
    _connect(b, b_out, n, "B")
    _connect(alpha, alpha_out, n, "Alpha")
    return n


def _tex_param(m, name, tex, sampler, uv, x, y):
    n = _expr(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("texture", tex)
    n.set_editor_property("sampler_type", sampler)
    n.set_editor_property("group", "Grim Textures")
    if uv is not None:
        _connect(uv, "", n, "UVs")
    return n


def make_master(tex):
    path = MAT_DIR + "/M_GrimEnv_Master"
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = asset_tools.create_asset("M_GrimEnv_Master", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", False)
    X = -2400
    # UV(1 UV = 2 m 를 메시가 이미 준다) × UVScale
    tc = _expr(m, unreal.MaterialExpressionTextureCoordinate, X, 0)
    uv = _bin(m, unreal.MaterialExpressionMultiply, tc, _scalar(m, "UVScale", 1.0, X, 120), X + 200, 0)
    bc = _tex_param(m, "BaseColorTex", tex["T_GrimEnv_Stone_BC"], unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, uv, X + 400, -300)
    nrm = _tex_param(m, "NormalTex", tex["T_GrimEnv_Stone_N"], unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, uv, X + 400, 300)
    orm = _tex_param(m, "ORMTex", tex["T_GrimEnv_Stone_ORM"], unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, uv, X + 400, 700)
    # 큰 무늬(월드 XY / MacroScale) — R 넓은 얼룩, B 이끼 자리
    wp = _expr(m, unreal.MaterialExpressionWorldPosition, X, 1100)
    wxy = _mask(m, wp, "rg", X + 200, 1100)
    muv = _bin(m, unreal.MaterialExpressionDivide, wxy, _scalar(m, "MacroScale", 1800.0, X + 200, 1220), X + 400, 1100)
    macro = _tex_param(m, "MacroTex", tex["T_GrimEnv_Macro_Mask"], unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, muv, X + 600, 1100)
    vc = _expr(m, unreal.MaterialExpressionVertexColor, X, -900)

    # 바탕색: 채도 빼기 → 색조 → 조각 변이 → 큰 무늬 → AO 색(푸른 그늘)
    X2 = -1700
    desat = _expr(m, unreal.MaterialExpressionDesaturation, X2, -300)
    _connect(bc, "RGB", desat, "")
    _connect(_scalar(m, "Desaturate", 0.0, X2 - 200, -180), "", desat, "Fraction")
    tinted = _bin(m, unreal.MaterialExpressionMultiply, desat, _vector(m, "Tint", (1, 1, 1), X2 - 200, -80), X2 + 200, -300)
    # 정점 색 직접 칠하기(잎 · 꽃 · 책): 회색 텍스처 밝기 × 정점 RGB
    vc_rgb = _mask(m, vc, "rgb", X2 - 200, -900)
    lum = _expr(m, unreal.MaterialExpressionDesaturation, X2, -700)
    _connect(bc, "RGB", lum, "")
    _connect(_const(m, 1.0, X2 - 200, -620), "", lum, "Fraction")
    direct = _bin(m, unreal.MaterialExpressionMultiply, _bin(m, unreal.MaterialExpressionMultiply, lum, vc_rgb, X2 + 200, -800), _const(m, 1.15, X2 + 200, -680), X2 + 400, -800)
    use_vc = _scalar(m, "UseVertexColor", 0.0, X2 + 400, -620)
    base = _lerp(m, tinted, direct, use_vc, X2 + 600, -300)
    # 조각 변이: 1 + (VC.R − 0.5) × 2 × VariationAmount   (정점 색 직접 칠하기면 끈다)
    var_amt = _bin(m, unreal.MaterialExpressionMultiply, _scalar(m, "VariationAmount", 0.12, X2, 100),
                   _bin(m, unreal.MaterialExpressionSubtract, _const(m, 1.0, X2, 200), use_vc, X2 + 200, 200), X2 + 400, 100)
    vr = _bin(m, unreal.MaterialExpressionSubtract, vc, _const(m, 0.5, X2, 300), X2 + 200, 300, a_out="R")
    var = _bin(m, unreal.MaterialExpressionAdd, _const(m, 1.0, X2 + 400, 400),
               _bin(m, unreal.MaterialExpressionMultiply, _bin(m, unreal.MaterialExpressionMultiply, vr, _const(m, 2.0, X2 + 400, 300), X2 + 600, 300), var_amt, X2 + 800, 200), X2 + 1000, 300)
    mr = _bin(m, unreal.MaterialExpressionSubtract, macro, _const(m, 0.5, X2, 1100), X2 + 200, 1100, a_out="R")
    macro_f = _bin(m, unreal.MaterialExpressionAdd, _const(m, 1.0, X2 + 400, 1200),
                   _bin(m, unreal.MaterialExpressionMultiply, mr, _scalar(m, "MacroStrength", 0.25, X2 + 200, 1220), X2 + 400, 1100), X2 + 600, 1100)
    col = _bin(m, unreal.MaterialExpressionMultiply, _bin(m, unreal.MaterialExpressionMultiply, base, var, X2 + 1200, -300), macro_f, X2 + 1400, -300)
    # 위를 보는 면에 이끼(큰 무늬 B 얼룩) — 「Detailed Environment Art」
    vn = _expr(m, unreal.MaterialExpressionVertexNormalWS, X2, 1500)
    up = _mask(m, vn, "b", X2 + 200, 1500)
    up_ramp = _expr(m, unreal.MaterialExpressionSaturate, X2 + 800, 1500)
    _connect(_bin(m, unreal.MaterialExpressionMultiply, _bin(m, unreal.MaterialExpressionSubtract, up, _const(m, 0.55, X2 + 200, 1620), X2 + 400, 1500), _const(m, 3.0, X2 + 400, 1620), X2 + 600, 1500), "", up_ramp, "")
    moss_mask = _bin(m, unreal.MaterialExpressionMultiply, _bin(m, unreal.MaterialExpressionMultiply, up_ramp, macro, X2 + 1000, 1500, b_out="B"), _scalar(m, "MossAmount", 0.0, X2 + 800, 1650), X2 + 1200, 1500)
    moss_sat = _expr(m, unreal.MaterialExpressionSaturate, X2 + 1400, 1500)
    _connect(moss_mask, "", moss_sat, "")
    col_moss = _lerp(m, col, _vector(m, "MossColor", (0.20, 0.30, 0.10), X2 + 1400, 1300), moss_sat, X2 + 1600, -300)
    # AO 색: 오목한 곳을 검정이 아니라 푸른 그늘로(Painterly — 따뜻한 빛 · 푸른 그림자)
    ao_tint = _lerp(m, _vector(m, "AOTint", (0.62, 0.66, 0.82), X2 + 1600, 400), _const(m, 1.0, X2 + 1600, 520), orm, X2 + 1800, 400, alpha_out="R")
    final_col = _bin(m, unreal.MaterialExpressionMultiply, col_moss, ao_tint, X2 + 2000, -300)
    mel.connect_material_property(final_col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # 거칠기 · 금속 · AO
    rough = _bin(m, unreal.MaterialExpressionMultiply, orm, _scalar(m, "RoughnessScale", 1.0, X2 + 1600, 800), X2 + 1800, 800, a_out="G")
    rough_moss = _lerp(m, rough, _const(m, 0.95, X2 + 1800, 920), moss_sat, X2 + 2000, 800)
    rough_sat = _expr(m, unreal.MaterialExpressionSaturate, X2 + 2200, 800)
    _connect(rough_moss, "", rough_sat, "")
    mel.connect_material_property(rough_sat, "", unreal.MaterialProperty.MP_ROUGHNESS)
    metal = _bin(m, unreal.MaterialExpressionMultiply, orm, _scalar(m, "MetallicScale", 0.0, X2 + 1600, 1000), X2 + 1800, 1000, a_out="B")
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(orm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    # 노멀 세기: (n.xy × NormalStrength, n.z)
    nxy = _bin(m, unreal.MaterialExpressionMultiply, _mask(m, nrm, "rg", X2, 600), _scalar(m, "NormalStrength", 1.0, X2, 720), X2 + 200, 600)
    nz = _mask(m, nrm, "b", X2, 820)
    nn = _expr(m, unreal.MaterialExpressionAppendVector, X2 + 400, 650)
    _connect(nxy, "", nn, "A")
    _connect(nz, "", nn, "B")
    mel.connect_material_property(nn, "", unreal.MaterialProperty.MP_NORMAL)
    # 발광(창 · 유리 구): EmissiveColor × EmissiveStrength
    em = _bin(m, unreal.MaterialExpressionMultiply, _vector(m, "EmissiveColor", (1.0, 0.62, 0.3), X2 + 1600, 1200), _scalar(m, "EmissiveStrength", 0.0, X2 + 1600, 1320), X2 + 1800, 1200)
    mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m, False)
    log("마스터 재질 M_GrimEnv_Master")
    return m


# 슬롯 이름 → (텍스처 세트, 파라미터). 색은 콘셉트 tower-exterior: 양피지색 돌 · 남색 슬레이트 · 놋쇠 · 잉크빛 창틀
MATERIALS = {
    "Stone": ("Stone", dict(Tint=(0.90, 0.77, 0.58), VariationAmount=0.2, MossAmount=0.6, MacroStrength=0.35, NormalStrength=1.4)),
    "StoneDark": ("Stone", dict(Tint=(0.60, 0.55, 0.48), VariationAmount=0.16, MossAmount=0.8, MacroStrength=0.35, NormalStrength=1.4)),
    "Flagstone": ("Stone", dict(Tint=(0.70, 0.65, 0.57), VariationAmount=0.24, MossAmount=0.0, MacroStrength=0.4, RoughnessScale=0.95, NormalStrength=1.3)),
    "Slate": ("Slate", dict(Tint=(0.27, 0.33, 0.70), VariationAmount=0.12, MossAmount=0.25, MacroStrength=0.2, UVScale=1.0)),
    "Wood": ("Wood", dict(Tint=(1.0, 0.92, 0.86), VariationAmount=0.14, MacroStrength=0.1)),
    "WoodInk": ("Wood", dict(Tint=(0.30, 0.36, 0.62), VariationAmount=0.06, Desaturate=0.6, MacroStrength=0.1)),
    "Brass": ("Metal", dict(Tint=(1.0, 0.76, 0.40), MetallicScale=1.0, VariationAmount=0.06, RoughnessScale=0.85, MacroStrength=0.1)),
    "Iron": ("Metal", dict(Tint=(0.20, 0.20, 0.22), MetallicScale=0.85, VariationAmount=0.05, RoughnessScale=1.4, MacroStrength=0.0)),
    "Glass": ("Leaf", dict(Tint=(0.07, 0.08, 0.11), RoughnessScale=0.2, VariationAmount=0.0, EmissiveColor=(1.0, 0.62, 0.30), EmissiveStrength=1.6, MacroStrength=0.0)),
    "Cloth": ("Cloth", dict(UseVertexColor=1.0, MacroStrength=0.0)),
    "Banner": ("Cloth", dict(BaseColorTex="T_GrimEnv_Banner_BC", VariationAmount=0.0, MacroStrength=0.0)),
    "Rug": ("Cloth", dict(BaseColorTex="T_GrimEnv_Rug_BC", VariationAmount=0.0, MacroStrength=0.0)),
    "Leaf": ("Leaf", dict(UseVertexColor=1.0, MacroStrength=0.0, RoughnessScale=1.1)),
    "Soil": ("Stone", dict(Tint=(0.36, 0.26, 0.18), Desaturate=0.3, VariationAmount=0.1, RoughnessScale=1.2, MacroStrength=0.3)),
    "Rune": ("Metal", dict(Tint=(1.0, 0.78, 0.42), MetallicScale=1.0, RoughnessScale=0.7, VariationAmount=0.0, EmissiveColor=(1.0, 0.7, 0.3), EmissiveStrength=0.15, MacroStrength=0.0)),
}


def make_instances(master, tex):
    out = {}
    for slot, (tset, params) in MATERIALS.items():
        name = f"MI_GrimEnv_{slot}"
        path = f"{MAT_DIR}/{name}"
        mi = eal.load_asset(path) if eal.does_asset_exist(path) else asset_tools.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", tex[params.get("BaseColorTex", f"T_GrimEnv_{tset}_BC")])
        mel.set_material_instance_texture_parameter_value(mi, "NormalTex", tex[f"T_GrimEnv_{tset}_N"])
        mel.set_material_instance_texture_parameter_value(mi, "ORMTex", tex[f"T_GrimEnv_{tset}_ORM"])
        for k, v in params.items():
            if k == "BaseColorTex":
                continue
            if isinstance(v, tuple):
                mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
            else:
                mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
        mel.update_material_instance(mi)
        eal.save_loaded_asset(mi, False)
        out[slot] = mi
    log(f"재질 인스턴스 {len(out)}")
    return out


# ─────────────── 메시 ───────────────

def kit_sources():
    srcs = {os.path.splitext(os.path.basename(p))[0]: p for p in glob.glob(os.path.join(KIT_SRC, "SM_*.glb"))}
    # Tripo 결과(정리된 GLB 가 있으면) — art/env/tower-yard-<이름>/SM_*.glb
    for p in glob.glob(os.path.join(TRIPO_SRC, "tower-yard-*", "SM_*.glb")):
        srcs[os.path.splitext(os.path.basename(p))[0]] = p
    return srcs


def import_mesh(name, src, mis):
    folder = f"{MESH_DIR}"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", folder)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    asset_tools.import_asset_tasks([task])
    paths = list(task.get_editor_property("imported_object_paths"))
    mesh = next((unreal.load_asset(p) for p in paths if isinstance(unreal.load_asset(p), unreal.StaticMesh)), None)
    if mesh is None:
        raise RuntimeError(f"정적 메시가 안 나왔다: {name} {paths}")
    tripo = "tower-yard-" in src.replace("\\", "/")
    # 재질: 슬롯 이름 → 우리 인스턴스. Tripo 메시는 자기 PBR 텍스처 재질을 그대로 두되 부모를 마스터 재질 계열로 맞추지는 않는다(아래 tripo_material)
    mats = mesh.get_editor_property("static_materials")
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property("material_slot_name"))
        key = slot.split(".")[0]
        if key in mis:
            sm.set_editor_property("material_interface", mis[key])
            mats[i] = sm
        elif tripo:
            pass
        else:
            unreal.log_warning(f"tower_art_import: {name} 슬롯 {slot} 에 맞는 인스턴스 없음")
    mesh.set_editor_property("static_materials", mats)
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", nanite)
    unreal.EditorStaticMeshLibrary.remove_collisions(mesh)  # 커맨드렛엔 StaticMeshEditorSubsystem 이 없다
    body = mesh.get_editor_property("body_setup")
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    eal.save_loaded_asset(mesh, False)
    # 가져오기가 만든 재질 · 텍스처(키트는 슬롯 이름만 든 빈 재질) — 키트는 지운다, Tripo 는 텍스처를 쓴다
    if not tripo:
        for p in paths:
            a = unreal.load_asset(p)
            if isinstance(a, (unreal.MaterialInterface, unreal.Texture)):
                eal.delete_asset(p.split(".")[0])
    box = mesh.get_bounding_box()
    size = box.max - box.min
    log(f"{name}: {size.x:.0f}×{size.y:.0f}×{size.z:.0f} cm (최소 {box.min.x:.0f}, {box.min.y:.0f}, {box.min.z:.0f}) 슬롯 {[str(s.get_editor_property('material_slot_name')) for s in mats]}")
    return mesh


def axis_check(mesh_path):
    """블렌더 +Y 가 언리얼 +Y 인지 — 아궁이(SM_TowerIn_Hearth) 쇠 막대는 블렌더 y = +0.3 m"""
    mesh = unreal.load_asset(mesh_path)
    if mesh is None:
        return
    for i, sm in enumerate(mesh.get_editor_property("static_materials")):
        if "Iron" in str(sm.get_editor_property("material_slot_name")):
            try:
                result = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, i)
                verts = result[0]
                ys = [v.y for v in verts]
                log(f"축 검사: 아궁이 쇠 막대 평균 Y = {sum(ys) / max(len(ys), 1):.1f} cm (블렌더 +30 cm)")
            except Exception as e:  # noqa: BLE001
                log(f"축 검사 못 함: {e}")


tex = import_textures()
master = make_master(tex)
mis = make_instances(master, tex)
if os.environ.get("GRIM_TOWER_SKIP_MESH") != "1":
    count = 0
    for name, src in sorted(kit_sources().items()):
        if ONLY and name not in ONLY:
            continue
        import_mesh(name, src, mis)
        count += 1
    log(f"메시 {count}")
    axis_check(f"{MESH_DIR}/SM_TowerIn_Hearth/StaticMeshes/SM_TowerIn_Hearth")
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
log("끝")
