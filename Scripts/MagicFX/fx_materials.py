# 마법 이펙트 텍스처 들여오기 · 파티클 머티리얼(make_fx_niagara.py 가 부른다). 값은 이 스크립트가 정본이다.
#  - 텍스처: art/fx/textures/T_FX_*.png → /Game/Grimoire/MagicFX/Textures (회색조, 밝기 = 알파)
#  - M_GrimFX_PAdd       가산 스프라이트: 텍스처 × 파티클 색, 밝은 곳은 흰빛으로(뜨거운 심)
#  - M_GrimFX_PFireSubUV 반투명 불꽃 플립북: 파티클 알파가 줄면 흐려지지 않고 가장자리부터 깎인다(침식), 가장자리 짙은 붉은빛
#  - M_GrimFX_PSmoke     반투명 연기 플립북: 파티클 색 그대로(어두운 연기), 침식
#  - M_GrimFX_RibbonAdd  가산 리본 꼬리: 결 텍스처를 길이 방향으로 흘리고 노이즈로 끊는다, 꼬리 끝으로 갈수록 가늘게

import os
import unreal

ROOT = "/Game/Grimoire/MagicFX"
TEX_DIR = ROOT + "/Textures"
PMAT_DIR = ROOT + "/Materials/Particles"
SRC_TEX = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "art", "fx", "textures")

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
LOG = []


def log(msg):
    LOG.append(str(msg))
    unreal.log("[MagicFX] " + str(msg))


# ---------------------------------------------------------------- 텍스처

def import_textures():
    tasks = []
    for name in sorted(os.listdir(SRC_TEX)):
        if not name.endswith(".png"):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.abspath(os.path.join(SRC_TEX, name)))
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
            if not isinstance(tex, unreal.Texture2D):
                continue
            # 회색조 마스크 — 선형, 압축은 알파 없는 단일 채널.
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
            is_tile = "Noise" in path or "Ribbon" in path
            addr = unreal.TextureAddress.TA_WRAP if is_tile else unreal.TextureAddress.TA_CLAMP
            tex.set_editor_property("address_x", addr)
            tex.set_editor_property("address_y", addr if "Ribbon" not in path else unreal.TextureAddress.TA_CLAMP)
            EAL.save_loaded_asset(tex)
            out[tex.get_name()] = tex
            log("텍스처 " + path)
    return out


# ---------------------------------------------------------------- 머티리얼 노드 도우미

class G:
    """머티리얼 그래프를 짧게 쓰는 도우미."""

    def __init__(self, mat):
        self.mat = mat
        self.y = 0

    def node(self, cls, x=-600, **props):
        self.y += 120
        e = MEL.create_material_expression(self.mat, cls, x, self.y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(self, a, b, b_in="", a_out=""):
        MEL.connect_material_expressions(a, a_out, b, b_in)

    def op(self, cls, a, b, a_out="", b_out=""):
        e = self.node(cls, -300)
        self.link(a, e, "A", a_out)
        if isinstance(b, (int, float)):
            e.set_editor_property("const_b", float(b))
        else:
            self.link(b, e, "B", b_out)
        return e

    def mul(self, a, b, a_out="", b_out=""):
        return self.op(unreal.MaterialExpressionMultiply, a, b, a_out, b_out)

    def add(self, a, b, a_out="", b_out=""):
        return self.op(unreal.MaterialExpressionAdd, a, b, a_out, b_out)

    def sub(self, a, b, a_out="", b_out=""):
        return self.op(unreal.MaterialExpressionSubtract, a, b, a_out, b_out)

    def const(self, v):
        return self.node(unreal.MaterialExpressionConstant, r=float(v))

    def scalar(self, name, v):
        return self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=float(v))

    def one_minus(self, a, a_out=""):
        e = self.node(unreal.MaterialExpressionOneMinus, -300)
        self.link(a, e, "", a_out)
        return e

    def saturate(self, a):
        e = self.node(unreal.MaterialExpressionSaturate, -200)
        self.link(a, e)
        return e

    def power(self, a, exp):
        e = self.node(unreal.MaterialExpressionPower, -200, const_exponent=float(exp))
        self.link(a, e, "Base")
        return e

    def lerp(self, a, b, alpha, alpha_out=""):
        e = self.node(unreal.MaterialExpressionLinearInterpolate, -200)
        self.link(a, e, "A")
        self.link(b, e, "B")
        self.link(alpha, e, "Alpha", alpha_out)
        return e

    def tex_param(self, name, tex, uv=None, sub_uv=False):
        cls = unreal.MaterialExpressionParticleSubUV if sub_uv else unreal.MaterialExpressionTextureSampleParameter2D
        if sub_uv:
            e = self.node(cls, -900, texture=tex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
            e.set_editor_property("blend", True)
        else:
            e = self.node(cls, -900, parameter_name=name, texture=tex,
                          sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
        if uv is not None:
            self.link(uv, e, "UVs")
        return e


def fresh(name, blend, usage):
    path = PMAT_DIR + "/" + name
    if EAL.does_asset_exist(path):
        mat = EAL.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset(name, PMAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", blend)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    # 쓰임 표시(Used with Niagara …)는 속성으로 직접 켠다 — set_material_usage 는 커맨드렛에서 저장에 남지 않아
    # 게임에서 「needed to set usage flag」 와 함께 엔진 기본 흰 스프라이트로 그려졌다(2026-11-06).
    for flag in usage:
        mat.set_editor_property(USAGE_PROPERTY[flag], True)
    return mat, G(mat)


def finish(mat, emissive, opacity=None):
    # 노출 되돌리기: 발광 × (1 / 화면 노출). 해 질 녘 · 실내처럼 어두운 곳에서 자동 노출이 오르면 이펙트 색이 통째로
    # 하얗게 밀렸다(2026-11-06 — 어두운 연기가 회색, 불꽃이 흰색). 이제 1.0 = 화면에서 꽉 찬 색, 자동 노출과 상관없다.
    inv = MEL.create_material_expression(mat, unreal.MaterialExpressionEyeAdaptationInverse, -100, 0)
    if not (MEL.connect_material_expressions(emissive, "", inv, "LightValue")
            or MEL.connect_material_expressions(emissive, "", inv, "LightValueInput")):
        log("!! EyeAdaptationInverse 입력 연결 실패 " + mat.get_name())
        inv = emissive
    MEL.connect_material_property(inv, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if opacity is not None:
        MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log("머티리얼 " + mat.get_path_name())
    return mat


SPRITE = ["sprites"]
RIBBON = ["ribbons"]
MESH = ["meshes"]
USAGE_PROPERTY = {"sprites": "used_with_niagara_sprites", "ribbons": "used_with_niagara_ribbons", "meshes": "used_with_niagara_mesh_particles"}


def hot_core(g, color, mask, glow_param, core_param):
    """색 × 마스크 × 밝기, 마스크가 진한 곳은 흰빛으로 — 심이 하얗고 가장자리가 속성색."""
    hot = g.power(mask, 3.0)
    white = g.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(1, 0.97, 0.9, 1))
    core_mix = g.mul(hot, core_param)
    tinted = g.lerp(color, white, core_mix)
    lit = g.mul(tinted, mask)
    return g.mul(lit, glow_param)


def make_add_sprite(textures):
    mat, g = fresh("M_GrimFX_PAdd", unreal.BlendMode.BLEND_ADDITIVE, SPRITE + MESH)
    pc = g.node(unreal.MaterialExpressionParticleColor, -900)
    tex = g.tex_param("Mask", textures["T_FX_SoftGlow"])
    mask = g.mul(tex, pc, "", "A")
    out = hot_core(g, pc, mask, g.scalar("Glow", 2.5), g.scalar("CoreWhite", 0.35))
    return finish(mat, out)


def make_fire_subuv(textures):
    """불꽃 덩이: 반투명(가산이 아니다) — 밝은 낮 하늘 앞에서 가산은 흰색으로 날아간다(2026-11-06 화면 확인).
    가장자리는 짙은 주황 · 붉은빛으로 배경을 덮고, 진한 곳만 노란 흰빛."""
    mat, g = fresh("M_GrimFX_PFireSubUV", unreal.BlendMode.BLEND_TRANSLUCENT, SPRITE)
    pc = g.node(unreal.MaterialExpressionParticleColor, -900)
    tex = g.tex_param("Flipbook", textures["T_FX_FireFlipbook_4x4"], sub_uv=True)
    inv = g.one_minus(pc, "A")
    d = g.sub(tex, inv)
    mask = g.saturate(g.mul(d, 1.0 / 0.35))
    edge = g.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0.35, 0.03, 0.005, 1))
    body = g.lerp(edge, pc, g.power(mask, 0.6))
    hot = g.mul(g.power(mask, 6.0), g.scalar("CoreWhite", 0.4))
    white = g.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(1, 0.75, 0.35, 1))
    lit = g.mul(g.lerp(body, white, hot), g.scalar("Glow", 1.5))
    opacity = g.mul(g.power(mask, 0.8), g.scalar("Opacity", 0.95))
    return finish(mat, lit, opacity)


def make_smoke(textures):
    mat, g = fresh("M_GrimFX_PSmoke", unreal.BlendMode.BLEND_TRANSLUCENT, SPRITE)
    pc = g.node(unreal.MaterialExpressionParticleColor, -900)
    tex = g.tex_param("Flipbook", textures["T_FX_SmokeFlipbook_4x4"], sub_uv=True)
    inv = g.one_minus(pc, "A")
    d = g.sub(tex, inv)
    mask = g.saturate(g.mul(d, 1.0 / 0.5))
    opacity = g.mul(mask, g.scalar("Density", 0.6))
    return finish(mat, pc, opacity)


def make_ribbon(textures):
    """리본 UV: U = 길이(0 머리 → 1 꼬리), V = 폭."""
    mat, g = fresh("M_GrimFX_RibbonAdd", unreal.BlendMode.BLEND_ADDITIVE, RIBBON)
    pc = g.node(unreal.MaterialExpressionParticleColor, -900)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1200)
    pan = g.node(unreal.MaterialExpressionPanner, -1000, speed_x=-1.6, speed_y=0.0)
    g.link(uv, pan, "Coordinate")
    streak = g.tex_param("Streak", textures["T_FX_RibbonStreak"], pan)
    pan2 = g.node(unreal.MaterialExpressionPanner, -1000, speed_x=-0.7, speed_y=0.15)
    uv2 = g.node(unreal.MaterialExpressionTextureCoordinate, -1200, u_tiling=1.5, v_tiling=0.6)
    g.link(uv2, pan2, "Coordinate")
    nz = g.tex_param("Noise", textures["T_FX_Noise"], pan2)
    # 꼬리 끝으로 갈수록 가늘게: 폭 마스크 = 1 − |V − 0.5|·2 를 (1 − U) 로 좁힌다.
    mask_u = g.node(unreal.MaterialExpressionComponentMask, -800, r=True, g=False, b=False, a=False)
    g.link(uv, mask_u)
    tail = g.one_minus(mask_u)
    body = g.mul(streak, g.add(g.mul(nz, 0.8), 0.45))
    shaped = g.mul(body, g.power(tail, 1.5))
    mask = g.saturate(g.mul(shaped, pc, "", "A"))
    out = hot_core(g, pc, mask, g.scalar("Glow", 3.0), g.scalar("CoreWhite", 0.6))
    return finish(mat, out)


def make_all():
    for folder in (TEX_DIR, PMAT_DIR):
        if not EAL.does_directory_exist(folder):
            EAL.make_directory(folder)
    textures = import_textures()
    add = make_add_sprite(textures)
    mats = {
        "add": add,
        "fire": make_fire_subuv(textures),
        "smoke": make_smoke(textures),
        "ribbon": make_ribbon(textures),
    }
    # 가산 스프라이트의 모양만 바꾼 인스턴스(별 섬광 · 고리 · 불티).
    for key, tex in (("flare4", "T_FX_Flare4"), ("flare6", "T_FX_Flare6"), ("ring", "T_FX_Ring"), ("spark", "T_FX_Spark")):
        mats[key] = make_instance("MI_GrimFX_PAdd_" + key.capitalize(), add, {"Mask": textures[tex]})
    return textures, mats


def make_instance(name, parent, tex_params):
    path = PMAT_DIR + "/" + name
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = asset_tools.create_asset(name, PMAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", parent)
    for key, tex in tex_params.items():
        MEL.set_material_instance_texture_parameter_value(mi, key, tex)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    log("인스턴스 " + path)
    return mi
