# M1.5 갈래 C 「연출 · 동작」 에셋을 만든다 / 고친다. 값은 이 스크립트가 정본이다.
#  - 머티리얼: M_GrimFX_Glow(색 펄스 · 줄기 구슬) · M_GrimFX_Book(그리모어) · M_GrimFX_Decal(그을음 · 젖음)
#  - GameplayCue: GC_Spell_Cast · GC_Spell_Impact · GC_Spell_Channel (C++ 처리기의 블루프린트 자식)
#  - 연출 묶음: VS_Ember · VS_WaterJet · VS_Breeze (UGrimMagicFXVisualSet) — 이펙트는 엔진 기본 Niagara 템플릿(임시)
#  - 애님: ABP_GrimPlayer = 템플릿 ABP_Unarmed 복제 + 부모를 UGrimSpellPoseAnimInstance 로
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\MagicFX\make_magicfx_assets.py" -unattended -nop4 -nosplash -nullrhi
# 결과 요약은 Saved/MagicFX/make_magicfx_assets.txt 에도 남긴다.

import os
import unreal

ROOT = "/Game/Grimoire/MagicFX"
MAT_DIR = ROOT + "/Materials"
CUE_DIR = ROOT + "/Cues"
VS_DIR = ROOT + "/VisualSets"
ANIM_DIR = ROOT + "/Anim"

# 엔진 기본 파티클(임시 — 사용자 지시 2026-11-05). Niagara 플러그인 기본 템플릿 시스템.
NS = "/Niagara/DefaultAssets/Templates/Systems/"
FX_DIRECTIONAL_BURST = NS + "DirectionalBurst.DirectionalBurst"
FX_DIRECTIONAL_BURST_LW = NS + "DirectionalBurstLightweight.DirectionalBurstLightweight"
FX_RADIAL_BURST = NS + "RadialBurst.RadialBurst"
FX_SIMPLE_EXPLOSION = NS + "SimpleExplosion.SimpleExplosion"
FX_FOUNTAIN_LW = NS + "FountainLightweight.FountainLightweight"

# 엔진 기본 소리(임시).
SND_PING = "/Engine/EngineSounds/1kSineTonePing.1kSineTonePing"
SND_NOISE = "/Engine/EngineSounds/WhiteNoise.WhiteNoise"

ABP_TEMPLATE = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"
ABP_GRIM = ANIM_DIR + "/ABP_GrimPlayer"

LOG = []
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def log(msg):
    LOG.append(str(msg))
    unreal.log("[MagicFX] " + str(msg))


def srgb(hex_color):
    """'#RRGGBB' → 선형 LinearColor (스펙의 속성색)."""
    h = hex_color.lstrip("#")
    out = []
    for i in (0, 2, 4):
        c = int(h[i:i + 2], 16) / 255.0
        out.append(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4)
    return unreal.LinearColor(out[0], out[1], out[2], 1.0)


def load(path):
    obj = unreal.load_asset(path)
    if obj is None:
        log("!! 못 찾음: " + path)
    return obj


def get_or_create(name, folder, asset_class, factory):
    path = folder + "/" + name
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path), False
    return asset_tools.create_asset(name, folder, asset_class, factory), True


# ---------------------------------------------------------------- 머티리얼

def fresh_material(name):
    mat, created = get_or_create(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if not created:
        MEL.delete_all_material_expressions(mat)
    return mat


def vec_param(mat, name, value, x, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def scalar_param(mat, name, value, x, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def mul(mat, a, b, x, y, a_out="", b_out=""):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(a, a_out, e, "A")
    MEL.connect_material_expressions(b, b_out, e, "B")
    return e


def make_glow_material():
    """가산 · 무광. Emissive = Color × Glow × Opacity × lerp(1−Fresnel, Fresnel, RimMix)."""
    mat = fresh_material("M_GrimFX_Glow")
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)

    color = vec_param(mat, "Color", unreal.LinearColor(1, 1, 1, 1), -900, -200)
    glow = scalar_param(mat, "Glow", 6.0, -900, 0)
    opacity = scalar_param(mat, "Opacity", 1.0, -900, 120)
    rim = scalar_param(mat, "RimMix", 0.0, -900, 400)

    fresnel = MEL.create_material_expression(mat, unreal.MaterialExpressionFresnel, -900, 250)
    fresnel.set_editor_property("exponent", 2.0)
    inv = MEL.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -700, 200)
    MEL.connect_material_expressions(fresnel, "", inv, "")
    shape = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -500, 250)
    MEL.connect_material_expressions(inv, "", shape, "A")
    MEL.connect_material_expressions(fresnel, "", shape, "B")
    MEL.connect_material_expressions(rim, "", shape, "Alpha")

    c1 = mul(mat, color, glow, -600, -150)
    c2 = mul(mat, c1, opacity, -400, -100)
    c3 = mul(mat, c2, shape, -200, 0)
    MEL.connect_material_property(c3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log("머티리얼 " + mat.get_path_name())
    return mat


def make_book_material():
    """불투명. BaseColor = Color, Emissive = EmissiveColor × Glow (넘길 때 글자 면 발광)."""
    mat = fresh_material("M_GrimFX_Book")
    color = vec_param(mat, "Color", unreal.LinearColor(0.85, 0.76, 0.55, 1), -600, -200)
    emissive = vec_param(mat, "EmissiveColor", unreal.LinearColor(1.0, 0.8, 0.45, 1), -600, 0)
    glow = scalar_param(mat, "Glow", 0.0, -600, 200)
    rough = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 300)
    rough.set_editor_property("r", 0.8)
    e = mul(mat, emissive, glow, -300, 50)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log("머티리얼 " + mat.get_path_name())
    return mat


def make_decal_material():
    """지연 데칼 · 반투명. 둥근 자국: Opacity = saturate(1 − 2·|uv − 0.5|)^1.5 × Opacity × 데칼 수명 흐림."""
    mat = fresh_material("M_GrimFX_Decal")
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    color = vec_param(mat, "Color", unreal.LinearColor(0.02, 0.015, 0.01, 1), -700, -300)
    rough = scalar_param(mat, "Roughness", 0.9, -700, -100)
    opacity = scalar_param(mat, "Opacity", 0.85, -700, 450)

    uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1300, 150)
    center = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -1300, 280)
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)
    dist = MEL.create_material_expression(mat, unreal.MaterialExpressionDistance, -1100, 200)
    MEL.connect_material_expressions(uv, "", dist, "A")
    MEL.connect_material_expressions(center, "", dist, "B")
    two = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -1100, 320)
    two.set_editor_property("r", 2.0)
    scaled = mul(mat, dist, two, -950, 220)
    inv = MEL.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -800, 220)
    MEL.connect_material_expressions(scaled, "", inv, "")
    sat = MEL.create_material_expression(mat, unreal.MaterialExpressionSaturate, -650, 220)
    MEL.connect_material_expressions(inv, "", sat, "")
    power = MEL.create_material_expression(mat, unreal.MaterialExpressionPower, -500, 220)
    power.set_editor_property("const_exponent", 1.5)
    MEL.connect_material_expressions(sat, "", power, "Base")
    masked = mul(mat, power, opacity, -350, 300)

    final = masked
    lifetime_class = getattr(unreal, "MaterialExpressionDecalLifetimeOpacity", None)
    if lifetime_class is not None:
        life = MEL.create_material_expression(mat, lifetime_class, -500, 450)
        final = mul(mat, masked, life, -200, 350)
    else:
        log("!! DecalLifetimeOpacity 노드 없음 — 자국이 흐려지지 않고 사라진다")

    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log("머티리얼 " + mat.get_path_name())
    return mat


# ---------------------------------------------------------------- GameplayCue

def make_cue(name, parent):
    path = CUE_DIR + "/" + name
    if EAL.does_asset_exist(path):
        bp = EAL.load_asset(path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent.static_class())
        bp = asset_tools.create_asset(name, CUE_DIR, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp)
    cdo = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(path))
    log("큐 %s → 태그 %s" % (path, cdo.get_editor_property("gameplay_cue_tag")))
    return bp


# ---------------------------------------------------------------- 연출 묶음

def pulse(shape, start, end, lifetime, glow, rim=0.0, light=0.0, radius=300.0):
    p = unreal.GrimFXPulseSpec()
    p.set_editor_property("shape", shape)
    p.set_editor_property("start_size", unreal.Vector(*start))
    p.set_editor_property("end_size", unreal.Vector(*end))
    p.set_editor_property("lifetime", lifetime)
    p.set_editor_property("glow", glow)
    p.set_editor_property("rim_mix", rim)
    p.set_editor_property("light_intensity", light)
    p.set_editor_property("light_radius", radius)
    return p


def decal(material, color, roughness, opacity, size, lifetime, fade):
    d = unreal.GrimImpactDecalSpec()
    d.set_editor_property("material", material)
    d.set_editor_property("color", color)
    d.set_editor_property("roughness", roughness)
    d.set_editor_property("opacity", opacity)
    d.set_editor_property("size", size)
    d.set_editor_property("lifetime", lifetime)
    d.set_editor_property("fade_time", fade)
    return d


SHAPE = unreal.GrimFXPulseShape
NONE_PULSE = None  # 아래에서 pulse(SHAPE.NONE, ...) 로 채운다


def make_visual_set(name, values):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.GrimMagicFXVisualSet.static_class())
    vs, _ = get_or_create(name, VS_DIR, unreal.GrimMagicFXVisualSet, factory)
    for key, value in values.items():
        vs.set_editor_property(key, value)
    EAL.save_loaded_asset(vs)
    log("연출 묶음 %s 색=%s 섬광=%s 몸통=%s 터짐=%s" % (
        vs.get_path_name(), vs.get_editor_property("element_color"),
        vs.get_editor_property("muzzle_flash"), vs.get_editor_property("trail"), vs.get_editor_property("impact")))
    return vs


def make_visual_sets(decal_mat):
    shake = unreal.GrimSpellCameraShake.static_class()
    ping = load(SND_PING)
    noise = load(SND_NOISE)
    off = pulse(SHAPE.NONE, (0, 0, 0), (0, 0, 0), 0.1, 0.0)
    no_decal = decal(None, unreal.LinearColor(0, 0, 0, 1), 0.9, 0.0, 60.0, 1.0, 0.0)

    # 불씨 — 주황, 작은 불꽃 덩어리 · 화염 터짐 · 그을음. 튕기기(Bolt).
    make_visual_set("VS_Ember", {
        "element_color": srgb("#E0582B"),
        "cast_motion": unreal.GrimCastMotion.AUTO,
        "muzzle_flash": load(FX_DIRECTIONAL_BURST),
        "trail": load(FX_FOUNTAIN_LW),
        "impact": load(FX_SIMPLE_EXPLOSION),
        "muzzle_flash_scale": 0.3,
        "trail_scale": 0.3,
        "impact_scale": 0.5,
        "cast_sound": ping, "cast_sound_volume": 0.12, "cast_sound_pitch": 1.6,
        "impact_sound": noise, "impact_sound_volume": 0.3, "impact_sound_pitch": 1.5, "impact_sound_duration": 0.15,
        "loop_sound": None,
        "camera_shake": shake, "camera_shake_scale": 1.0,
        "muzzle_pulse": pulse(SHAPE.SPHERE, (4, 4, 4), (24, 24, 24), 0.12, 10.0, 0.0, 4000.0, 250.0),
        "cast_wave": off,
        "impact_pulse": pulse(SHAPE.SPHERE, (10, 10, 10), (90, 90, 90), 0.3, 8.0, 0.0, 6000.0, 450.0),
        "impact_decal": decal(decal_mat, unreal.LinearColor(0.02, 0.015, 0.01, 1), 0.95, 0.9, 70.0, 12.0, 4.0),
        "stream_bead_count": 0,
    })

    # 물줄기 — 파랑, 포물선 물방울 구슬 · 물보라 원반 · 젖은 자국. 뻗고 유지(Stream).
    make_visual_set("VS_WaterJet", {
        "element_color": srgb("#3A7BD5"),
        "cast_motion": unreal.GrimCastMotion.AUTO,
        "muzzle_flash": load(FX_DIRECTIONAL_BURST_LW),
        "trail": load(FX_FOUNTAIN_LW),
        "impact": load(FX_DIRECTIONAL_BURST),
        "muzzle_flash_scale": 0.25,
        "trail_scale": 0.5,
        "impact_scale": 0.4,
        "cast_sound": ping, "cast_sound_volume": 0.08, "cast_sound_pitch": 0.8,
        "loop_sound": noise, "loop_sound_volume": 0.1, "loop_sound_pitch": 0.6,
        "impact_sound": noise, "impact_sound_volume": 0.2, "impact_sound_pitch": 1.0, "impact_sound_duration": 0.2,
        "camera_shake": shake, "camera_shake_scale": 0.6,
        "muzzle_pulse": pulse(SHAPE.SPHERE, (4, 4, 4), (16, 16, 16), 0.1, 6.0, 0.2, 2500.0, 200.0),
        "cast_wave": off,
        "impact_pulse": pulse(SHAPE.DISC, (10, 10, 2), (70, 70, 6), 0.35, 4.0, 0.3, 1500.0, 300.0),
        "impact_decal": decal(decal_mat, unreal.LinearColor(0.01, 0.015, 0.03, 1), 0.1, 0.7, 90.0, 15.0, 6.0),
        "stream_bead_count": 12,
        "stream_bead_size": 7.0,
        "stream_bead_glow": 3.0,
        "stream_flight_time": 0.6,
        "stream_default_launch_speed": 1400.0,
        "stream_default_gravity_scale": 1.0,
    })

    # 미풍 — 연두, 반투명 공기 고리(원뿔 껍질) · 둥근 바람 터짐 · 자국 없음. 쓸기(Cone).
    make_visual_set("VS_Breeze", {
        "element_color": srgb("#7CC9A6"),
        "cast_motion": unreal.GrimCastMotion.AUTO,
        "muzzle_flash": load(FX_DIRECTIONAL_BURST_LW),
        "trail": load(FX_RADIAL_BURST),
        "impact": load(FX_RADIAL_BURST),
        "muzzle_flash_scale": 0.25,
        "trail_scale": 0.6,
        "impact_scale": 0.5,
        "cast_sound": ping, "cast_sound_volume": 0.08, "cast_sound_pitch": 0.5,
        "loop_sound": None,
        "impact_sound": noise, "impact_sound_volume": 0.25, "impact_sound_pitch": 0.7, "impact_sound_duration": 0.3,
        "camera_shake": shake, "camera_shake_scale": 0.8,
        "muzzle_pulse": pulse(SHAPE.SPHERE, (6, 6, 6), (30, 30, 30), 0.15, 3.0, 1.0, 0.0, 200.0),
        # 끝 크기는 마법의 원뿔 사거리 · 반각이 덮어쓴다 — 시작 크기는 그 비율(10%)로 쓰인다.
        "cast_wave": pulse(SHAPE.CONE, (60, 60, 60), (600, 600, 600), 0.35, 2.5, 1.0, 0.0, 300.0),
        "impact_pulse": pulse(SHAPE.SPHERE, (20, 20, 20), (120, 120, 120), 0.3, 2.0, 1.0, 0.0, 300.0),
        "impact_decal": no_decal,
        "stream_bead_count": 0,
    })


# ---------------------------------------------------------------- 애님

def make_anim_blueprint():
    if not EAL.does_asset_exist(ABP_GRIM):
        if not EAL.does_asset_exist(ABP_TEMPLATE):
            log("!! 템플릿 ABP 없음: " + ABP_TEMPLATE)
            return
        EAL.duplicate_asset(ABP_TEMPLATE, ABP_GRIM)
        log("복제 %s → %s" % (ABP_TEMPLATE, ABP_GRIM))
    bp = EAL.load_asset(ABP_GRIM)
    parent = unreal.GrimSpellPoseAnimInstance.static_class()
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp)
    cls = EAL.load_blueprint_class(ABP_GRIM)
    cdo = unreal.get_default_object(cls) if cls else None
    log("애님 BP %s 생성 클래스=%s 부모가 GrimSpellPoseAnimInstance=%s" % (
        ABP_GRIM, cls, isinstance(cdo, unreal.GrimSpellPoseAnimInstance)))


# ---------------------------------------------------------------- 실행

def main():
    for folder in (MAT_DIR, CUE_DIR, VS_DIR, ANIM_DIR):
        if not EAL.does_directory_exist(folder):
            EAL.make_directory(folder)

    make_glow_material()
    make_book_material()
    decal_mat = make_decal_material()

    make_cue("GC_Spell_Cast", unreal.GrimCueNotify_SpellCast)
    make_cue("GC_Spell_Impact", unreal.GrimCueNotify_SpellImpact)
    make_cue("GC_Spell_Channel", unreal.GrimCueNotify_SpellChannel)

    make_visual_sets(decal_mat)
    make_anim_blueprint()

    # 설정(DefaultGame.ini)이 이 에셋들을 가리키는지 — 설정 클래스는 파이썬에 노출되지 않아 이름으로 찾는다.
    try:
        settings = unreal.find_object(None, "/Script/Grimoire.Default__GrimMagicFXSettings")
        log("설정 DefaultVisualSets = %s" % settings.get_editor_property("default_visual_sets"))
        log("설정 GlowMaterial = %s" % settings.get_editor_property("glow_material"))
    except Exception as e:  # noqa: BLE001 — 확인용 출력일 뿐
        log("설정 확인 생략: %s" % e)

    out_dir = os.path.join(unreal.Paths.project_saved_dir(), "MagicFX")
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "make_magicfx_assets.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(LOG))


main()
