# 볼트(튕기기) 이펙트 — 사용자 참고 그림(art/fx/refs/user-ref-bolt-*.png)과 GPT 시안(art/fx/raw/sheet_*.png) 대로.
# 값은 이 스크립트가 정본이다. 만드는 것(/Game/Grimoire/MagicFX/Bolt):
#  - 텍스처 T_Bolt_*        ← art/fx/bolt/*.png (make_bolt_textures.py 가 GPT 그림에서 만든 회색조, 밝기 = 알파)
#  - 머티리얼 M_Bolt_Add     가산 스프라이트: 텍스처 × 파티클 색, 밝은 곳은 흰빛(뜨거운 심), 깊이 흐림
#    인스턴스 MI_Bolt_<텍스처>
#  - Niagara NS_Bolt_Cast(완드 끝 튐) · NS_Bolt_Trail(바늘 빛줄기 · 꼬리 · 불티) · NS_Bolt_Impact(불티 폭발 · 고리 · 번쩍임)
#    색은 사용자 파라미터 User.Color(UGrimSpellVisualSet::TintNiagara 가 마법 색으로 덮어쓴다).
#    Niagara 는 엔진 실험 플러그인 NiagaraToolsets(ToolsetRegistry 의 JSON 도구)로 데이터에서 만든다.
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\MagicFX\make_bolt_fx.py" -unattended -nop4 -nosplash -nullrhi
# 결과 요약은 Saved/MagicFX/make_bolt_fx.txt.

import json
import os

import unreal

ROOT = "/Game/Grimoire/MagicFX/Bolt"
SRC_TEX = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "art", "fx", "bolt")

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
LOG = []


def log(msg):
    LOG.append(str(msg))
    unreal.log("[BoltFX] " + str(msg))


# ---------------------------------------------------------------- 텍스처

def import_textures():
    tasks = []
    for name in sorted(os.listdir(SRC_TEX)):
        if not name.endswith(".png"):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.abspath(os.path.join(SRC_TEX, name)))
        t.set_editor_property("destination_path", ROOT + "/Textures")
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
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
            tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
            tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
            EAL.save_loaded_asset(tex)
            out[tex.get_name()] = tex
            log("텍스처 " + path)
    return out


# ---------------------------------------------------------------- 머티리얼

def expr(mat, cls, x, y, **props):
    e = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, b_in="", a_out=""):
    MEL.connect_material_expressions(a, a_out, b, b_in)


def make_additive_material(default_tex):
    """Emissive = Mask × ParticleAlpha × (ParticleColor × Intensity + Mask^CorePower × CoreWhite) × DepthFade.
    파티클 색은 속성 색 그대로(0~1) — 밝기는 Intensity, 심의 흰빛은 CoreWhite."""
    path = ROOT + "/Materials/M_Bolt_Add"
    if EAL.does_asset_exist(path):
        mat = EAL.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset("M_Bolt_Add", ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    for flag in ("used_with_niagara_sprites", "used_with_niagara_ribbons", "used_with_niagara_mesh_particles"):
        mat.set_editor_property(flag, True)

    tex = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1200, 0,
               parameter_name="Mask", texture=default_tex,
               sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    pc = expr(mat, unreal.MaterialExpressionParticleColor, -1200, 300)
    intensity = expr(mat, unreal.MaterialExpressionScalarParameter, -1200, 400, parameter_name="Intensity", default_value=4.0)
    core_pow = expr(mat, unreal.MaterialExpressionScalarParameter, -1200, 500, parameter_name="CorePower", default_value=3.0)
    core_white = expr(mat, unreal.MaterialExpressionScalarParameter, -1200, 600, parameter_name="CoreWhite", default_value=4.0)
    fade_dist = expr(mat, unreal.MaterialExpressionScalarParameter, -1200, 750, parameter_name="DepthFade", default_value=16.0)

    power = expr(mat, unreal.MaterialExpressionPower, -900, 450)
    link(tex, power, "Base", "R")
    link(core_pow, power, "Exp")
    hot = expr(mat, unreal.MaterialExpressionMultiply, -750, 500)
    link(power, hot, "A")
    link(core_white, hot, "B")
    tint = expr(mat, unreal.MaterialExpressionMultiply, -750, 300)
    link(pc, tint, "A", "RGB")
    link(intensity, tint, "B")
    col = expr(mat, unreal.MaterialExpressionAdd, -600, 350)
    link(tint, col, "A")
    link(hot, col, "B")

    mask = expr(mat, unreal.MaterialExpressionMultiply, -750, 100)
    link(tex, mask, "A", "R")
    link(pc, mask, "B", "A")
    lit = expr(mat, unreal.MaterialExpressionMultiply, -450, 200)
    link(col, lit, "A")
    link(mask, lit, "B")

    depth = expr(mat, unreal.MaterialExpressionDepthFade, -600, 750)
    link(fade_dist, depth, "FadeDistance")
    final = expr(mat, unreal.MaterialExpressionMultiply, -250, 300)
    link(lit, final, "A")
    link(depth, final, "B")
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log("머티리얼 " + path)
    return mat


def make_instance(parent, key, tex, intensity, core_white):
    name = "MI_Bolt_" + key
    path = ROOT + "/Materials/" + name
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = asset_tools.create_asset(name, ROOT + "/Materials", unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", parent)
    MEL.set_material_instance_texture_parameter_value(mi, "Mask", tex)
    MEL.set_material_instance_scalar_parameter_value(mi, "Intensity", intensity)
    MEL.set_material_instance_scalar_parameter_value(mi, "CoreWhite", core_white)
    EAL.save_loaded_asset(mi)
    log("인스턴스 " + path)
    return mi


# ---------------------------------------------------------------- Niagara (NiagaraToolsets JSON 도구)

TOOLSET = "NiagaraToolsets.NiagaraToolset_System"
T_FLOAT = "/Script/Niagara.NiagaraFloat"
T_VEC2 = "/Script/CoreUObject.Vector2f"
T_VEC3 = "/Script/CoreUObject.Vector3f"
T_COLOR = "/Script/CoreUObject.LinearColor"
T_INT = "/Script/Niagara.NiagaraInt32"
EMITTERS = "/Niagara/DefaultAssets/Templates/Emitters/"
SPAWN, UPDATE, EMITTER_UPDATE = "ParticleSpawnScript", "ParticleUpdateScript", "EmitterUpdateScript"


def tool(name, args, toolset=TOOLSET):
    res = unreal.ToolsetRegistry.execute_tool(toolset, name, json.dumps(args))
    if res.error:
        raise RuntimeError("%s: %s" % (name, res.error))
    value = json.loads(res.value) if res.value else None
    return value.get("returnValue") if isinstance(value, dict) else value


ENUMS = {}


def v_enum(enum_path, display):
    """Niagara 열거 입력 — 표시 이름(예: "Self", "Infinite")으로 고른다. 값 이름(NewEnumeratorN)은 Info 도구에서 찾는다."""
    if enum_path not in ENUMS:
        info = tool("UEnum_Info", {"enum": {"refPath": enum_path}}, "NiagaraToolsets.NiagaraToolset_Info")
        ENUMS[enum_path] = json.loads(info) if isinstance(info, str) else info
    info = ENUMS[enum_path]
    for entry in info.get("Values", []):
        if entry.get("DisplayName") == display:
            return {"struct": {"refPath": "/Script/NiagaraEditor.NiagaraExt_StackInputData_Enum"},
                    "value": {"enum": {"refPath": enum_path}, "enumName": "%s::%s" % (info.get("Name"), entry.get("Name")),
                              "displayName": display}}
    raise RuntimeError("열거 값 없음 %s %s: %s" % (enum_path, display, [e.get("DisplayName") for e in info.get("Values", [])]))


LIFE_CYCLE = "/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode"
LOOP = "/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions"


def stack_ref(system, emitter="", script="", module="", renderer=-1, inputs=()):
    return {"system": system, "emitterName": emitter, "scriptName": script, "moduleName": module,
            "rendererIndex": renderer, "inputNameStack": list(inputs)}


def v_float(x):
    return {"struct": {"refPath": T_FLOAT}, "value": {"value": x}}


def v_int(x):
    return {"struct": {"refPath": T_INT}, "value": {"value": x}}


def v_vec3(x, y, z):
    return {"struct": {"refPath": T_VEC3}, "value": {"x": x, "y": y, "z": z}}


def v_hlsl(expression):
    return {"struct": {"refPath": "/Script/NiagaraEditor.NiagaraExt_StackInputData_HlslExpression"},
            "value": {"hlslExpression": expression}}


def v_link(name, type_path):
    return {"struct": {"refPath": "/Script/NiagaraEditor.NiagaraExt_StackInputData_Linked"},
            "value": {"linkedVariable": {"name": name, "type": {"classStructOrEnum": {"refPath": type_path}}}}}


DEFAULTS = {
    T_FLOAT: lambda: {"value": 0.0},
    T_VEC2: lambda: {"x": 0.0, "y": 0.0},
    T_VEC3: lambda: {"x": 0.0, "y": 0.0, "z": 0.0},
    T_COLOR: lambda: {"r": 1.0, "g": 1.0, "b": 1.0, "a": 1.0},
}


class Emitter:
    """시스템 안 이미터 하나. 템플릿 이미터를 붙이고, 템플릿 모듈 값을 바꾸거나 끄고,
    파티클 속성을 HLSL 식으로 덮어쓰는 Set Parameters 모듈을 단계 끝에 더한다."""

    def __init__(self, system, template, name):
        self.system = system
        top = tool("AddEmitter", {"system": system, "templateEmitter": {"refPath": EMITTERS + template + "." + template},
                                  "emitterName": name})
        self.name = top.get("emitterName", name)

    def set(self, script, module, input_name, value):
        try:
            tool("SetStackInputData", {"stackInputRef": stack_ref(self.system, self.name, script, module, inputs=[input_name]),
                                       "inputData": value})
        except RuntimeError as e:
            log("!! %s.%s.%s: %s" % (self.name, module, input_name, e))

    def disable(self, script, module):
        try:
            tool("SetModuleEnabled", {"moduleRef": stack_ref(self.system, self.name, script, module), "bEnabled": False})
        except RuntimeError as e:
            log("!! 끄기 %s.%s: %s" % (self.name, module, e))

    def override(self, script, params):
        """params: [(속성 이름, 타입, HLSL 식)] — 단계 끝에서 차례로 쓴다."""
        entries = [{"variable": {"name": n, "type": {"classStructOrEnum": {"refPath": t}}},
                    "defaultValue": {"struct": {"refPath": t}, "value": DEFAULTS[t]()}} for n, t, _ in params]
        try:
            top = tool("AddSetParametersModule", {"moduleLocationRef": stack_ref(self.system, self.name, script),
                                                  "parameters": entries})
        except RuntimeError as e:
            log("!! 덮어쓰기 %s.%s: %s" % (self.name, script, e))
            return None
        module = top["moduleName"]
        for n, _, expression in params:
            self.set(script, module, n, v_hlsl(expression))
        return module

    def renderer(self, material, aligned):
        values = {"Material": {"refPath": material.get_path_name()},
                  "Alignment": "VelocityAligned" if aligned else "Unaligned",
                  "FacingMode": "FaceCamera", "SortMode": "None", "bCastShadows": False}
        try:
            tool("SetRendererData", {"renderer": stack_ref(self.system, self.name, renderer=0),
                                     "rendererData": {"propertyValues": json.dumps(values)}})
        except RuntimeError as e:
            log("!! 렌더러 %s: %s" % (self.name, e))

    def life_cycle(self, loop):
        try:
            self.set(EMITTER_UPDATE, "EmitterState", "Life Cycle Mode", v_enum(LIFE_CYCLE, "Self"))
            self.set(EMITTER_UPDATE, "EmitterState", "Loop Behavior", v_enum(LOOP, loop))
        except RuntimeError as e:
            log("!! 수명 주기 %s: %s" % (self.name, e))

    def color_from_user(self):
        self.set(SPAWN, "InitializeParticle", "Color", v_link("User.Color", T_COLOR))

    def common(self, lifetime, size):
        self.set(SPAWN, "InitializeParticle", "Lifetime Min", v_float(lifetime[0]))
        self.set(SPAWN, "InitializeParticle", "Lifetime Max", v_float(lifetime[1]))
        self.set(SPAWN, "InitializeParticle", "Uniform Sprite Size Min", v_float(size[0]))
        self.set(SPAWN, "InitializeParticle", "Uniform Sprite Size Max", v_float(size[1]))
        self.color_from_user()


def new_system(name):
    folder = ROOT + "/Systems"
    path = folder + "/" + name
    if EAL.does_asset_exist(path):
        # 연출 묶음이 가리키는 에셋은 지우면 에디터가 죽는다 — 그대로 두고 안만 비운다.
        system = {"refPath": EAL.load_asset(path).get_path_name()}
        old_vars = tool("GetUserVariables", {"system": system}).get("userVariables", [])
        if old_vars:
            tool("RemoveUserVariables", {"system": system, "variablesToRemove": [
                {"name": v["name"], "type": v["type"]} for v in old_vars]})
    else:
        # 빈 시스템 템플릿이 없다 — 기본 시스템(분수 이미터 하나)으로 만들고 그 이미터를 뺀다.
        system = tool("CreateNiagaraSystem", {"assetName": name, "assetPath": folder,
                                              "templateSystem": {"refPath": "/Niagara/DefaultAssets/DefaultSystem.DefaultSystem"}})
    for em in tool("GetSystemSummary", {"system": system})["emitters"]:
        tool("RemoveEmitter", {"emitterToRemove": stack_ref(system, em["emitterName"])})
    color = {"name": "User.Color", "type": {"classStructOrEnum": {"refPath": T_COLOR}},
             "defaultValue": {"struct": {"refPath": T_COLOR}, "value": {"r": 0.6, "g": 0.75, "b": 1.0, "a": 1.0}},
             "description": "속성 색(UGrimSpellVisualSet::TintNiagara 가 덮어쓴다)"}
    tool("AddUserVariables", {"system": system, "variablesToAdd": [color]})
    return system


def burst(system, name, count, lifetime, size, material, aligned):
    """한 번 터지는 이미터(OmnidirectionalBurst 템플릿)."""
    e = Emitter(system, "OmnidirectionalBurst", name)
    e.life_cycle("Once")
    e.set(EMITTER_UPDATE, "SpawnBurst_Instantaneous", "Spawn Count", v_int(count))
    e.common(lifetime, size)
    e.renderer(material, aligned)
    return e


def still(system, name, lifetime, material):
    """한 번 · 한 장 · 제자리(번쩍임 · 고리 · 빛살). 크기 · 색은 update 덮어쓰기가 정한다."""
    e = burst(system, name, 1, (lifetime, lifetime), (1.0, 1.0), material, False)
    e.disable(SPAWN, "ShapeLocation")
    e.disable(UPDATE, "ScaleSpriteSize")
    e.disable(UPDATE, "ScaleColor")
    e.override(SPAWN, [("Particles.Velocity", T_VEC3, "float3(0, 0, 0)")])
    return e


def stream(system, name, rate, lifetime, size, material, aligned):
    """계속 뿜는 이미터(Fountain 템플릿)."""
    e = Emitter(system, "Fountain", name)
    # 스스로 끝없이 돈다 — 시스템 수명(기본 시스템 템플릿)을 따르면 투사체가 나는 동안 꺼질 수 있다.
    e.life_cycle("Infinite")
    e.set(EMITTER_UPDATE, "SpawnRate", "SpawnRate", v_float(rate))
    e.common(lifetime, size)
    e.renderer(material, aligned)
    return e


FADE = "float4(User.Color.rgb, 1.0 - Particles.NormalizedAge)"
FADE_SQ = "float4(User.Color.rgb, (1.0 - Particles.NormalizedAge) * (1.0 - Particles.NormalizedAge))"


def make_trail(mi):
    """비행: 바늘 빛줄기(심) + 머리 번쩍임 + 깃털 꼬리 + 흩어지는 불티. 투사체에 붙어 따라간다(월드 공간 · 투사체 속도를 물려받음).
    속도 정렬 스프라이트의 「위」= 진행 방향 — 크기는 (폭, 길이)."""
    system = new_system("NS_Bolt_Trail")

    core = stream(system, "Core", 160.0, (0.045, 0.06), (1.0, 1.0), mi["Streak"], True)
    for script, m in ((SPAWN, "ShapeLocation"), (SPAWN, "AddVelocity"), (UPDATE, "GravityForce"), (UPDATE, "Drag"),
                      (UPDATE, "ScaleColor")):
        core.disable(script, m)
    core.override(SPAWN, [("Particles.Velocity", T_VEC3, "Engine.Owner.Velocity"),
                          ("Particles.SpriteSize", T_VEC2, "float2(16.0, 120.0)")])
    core.override(UPDATE, [("Particles.Color", T_COLOR, FADE)])

    head = stream(system, "Head", 120.0, (0.03, 0.03), (26.0, 26.0), mi["Flare"], False)
    for script, m in ((SPAWN, "ShapeLocation"), (SPAWN, "AddVelocity"), (UPDATE, "GravityForce"), (UPDATE, "Drag"),
                      (UPDATE, "ScaleColor")):
        head.disable(script, m)
    head.override(SPAWN, [("Particles.Velocity", T_VEC3, "Engine.Owner.Velocity")])

    tail = stream(system, "Tail", 150.0, (0.12, 0.2), (1.0, 1.0), mi["Wisp"], True)
    for script, m in ((SPAWN, "ShapeLocation"), (SPAWN, "AddVelocity"), (UPDATE, "GravityForce"), (UPDATE, "ScaleColor")):
        tail.disable(script, m)
    tail.override(SPAWN, [("Particles.Velocity", T_VEC3, "Engine.Owner.Velocity * 0.75")])
    tail.override(UPDATE, [("Particles.Color", T_COLOR, FADE_SQ),
                           ("Particles.SpriteSize", T_VEC2, "float2(34.0, 230.0) * (1.0 - 0.5 * Particles.NormalizedAge)")])

    sparks = stream(system, "Sparks", 160.0, (0.25, 0.55), (3.0, 6.0), mi["Spark"], True)
    sparks.disable(SPAWN, "ShapeLocation")
    sparks.set(SPAWN, "AddVelocity", "Cone Angle", v_float(180.0))
    sparks.set(UPDATE, "GravityForce", "Gravity", v_vec3(0.0, 0.0, -120.0))
    sparks.set(UPDATE, "Drag", "Drag", v_float(2.5))
    sparks.override(SPAWN, [("Particles.Velocity", T_VEC3, "Particles.Velocity * 0.5 + Engine.Owner.Velocity * 0.08"),
                            ("Particles.SpriteSize", T_VEC2, "Particles.SpriteSize * float2(0.5, 3.0)")])
    sparks.override(UPDATE, [("Particles.Color", T_COLOR, FADE)])
    return finish(system)


def make_cast(mi):
    """시전: 완드 끝의 작은 방사형 불티 튐 + 번쩍임 + 몇 가닥 실안개. 시스템 Z 축 = 겨눈 방향(GrimSpellCueNotifies 의 AxisRotation)."""
    system = new_system("NS_Bolt_Cast")
    sparks = burst(system, "Sparks", 18, (0.12, 0.28), (3.0, 6.0), mi["Spark"], True)
    sparks.disable(UPDATE, "ScaleSpriteSize")
    sparks.set(UPDATE, "GravityForce", "Gravity", v_vec3(0.0, 0.0, -60.0))
    sparks.set(UPDATE, "Drag", "Drag", v_float(4.0))
    sparks.override(SPAWN, [("Particles.Velocity", T_VEC3, "Particles.Velocity * 1.2 + Engine.Owner.SystemZAxis * 380.0"),
                            ("Particles.SpriteSize", T_VEC2, "Particles.SpriteSize * float2(0.5, 3.5)")])

    flare = still(system, "Flare", 0.1, mi["Flare"])
    flare.override(UPDATE, [("Particles.SpriteSize", T_VEC2, "float2(40.0, 40.0) * (1.0 - 0.5 * Particles.NormalizedAge)"),
                            ("Particles.Color", T_COLOR, FADE)])

    wisps = burst(system, "Wisps", 5, (0.18, 0.3), (1.0, 1.0), mi["Wisp"], True)
    wisps.disable(SPAWN, "ShapeLocation")
    wisps.disable(UPDATE, "ScaleSpriteSize")
    wisps.disable(UPDATE, "ScaleColor")
    wisps.override(SPAWN, [("Particles.Velocity", T_VEC3, "Particles.Velocity * 0.35 + Engine.Owner.SystemZAxis * 160.0")])
    wisps.override(UPDATE, [("Particles.SpriteSize", T_VEC2, "float2(14.0, 46.0) * (0.6 + 0.8 * Particles.NormalizedAge)"),
                            ("Particles.Color", T_COLOR, FADE_SQ)])
    return finish(system)


def make_impact(mi):
    """적중: 방사형 불티 폭발 + 퍼지는 고리 + 번쩍임 + 빛살."""
    system = new_system("NS_Bolt_Impact")
    sparks = burst(system, "Sparks", 40, (0.25, 0.6), (3.0, 7.0), mi["Spark"], True)
    sparks.disable(UPDATE, "ScaleSpriteSize")
    sparks.set(UPDATE, "GravityForce", "Gravity", v_vec3(0.0, 0.0, -300.0))
    sparks.set(UPDATE, "Drag", "Drag", v_float(3.0))
    sparks.override(SPAWN, [("Particles.Velocity", T_VEC3, "Particles.Velocity * 1.4"),
                            ("Particles.SpriteSize", T_VEC2, "Particles.SpriteSize * float2(0.5, 4.0)")])

    ring = still(system, "Ring", 0.3, mi["Ring"])
    ring.override(UPDATE, [("Particles.SpriteSize", T_VEC2, "float2(1.0, 1.0) * (30.0 + 190.0 * sqrt(Particles.NormalizedAge))"),
                           ("Particles.Color", T_COLOR, FADE)])

    flare = still(system, "Flare", 0.18, mi["Flare"])
    flare.override(UPDATE, [("Particles.SpriteSize", T_VEC2, "float2(90.0, 90.0) * (1.0 - 0.4 * Particles.NormalizedAge)"),
                            ("Particles.Color", T_COLOR, FADE_SQ)])

    rays = still(system, "Burst", 0.14, mi["Burst"])
    rays.override(UPDATE, [("Particles.SpriteSize", T_VEC2, "float2(150.0, 150.0) * (0.6 + 0.7 * Particles.NormalizedAge)"),
                           ("Particles.Color", T_COLOR, FADE)])
    return finish(system)


def finish(system):
    try:
        summary = tool("GetSystemSummary", {"system": system})
        log("시스템 %s 이미터 %s 사용자 변수 %s" % (system["refPath"], [e["emitterName"] for e in summary["emitters"]],
                                             summary.get("userVariables")))
    except RuntimeError as e:
        log("!! 요약 " + str(e))
    # 확인용: 이미터마다 모듈 입력 값을 Saved/MagicFX/<시스템>.json 으로.
    try:
        dump = {e["emitterName"]: tool("GetEmitterInputValues", {"emitterRef": stack_ref(system, e["emitterName"])})
                for e in summary["emitters"]}
        out_dir = os.path.join(unreal.Paths.project_saved_dir(), "MagicFX")
        os.makedirs(out_dir, exist_ok=True)
        name = system["refPath"].rsplit(".", 1)[-1]
        with open(os.path.join(out_dir, name + ".json"), "w", encoding="utf-8") as f:
            json.dump(dump, f, ensure_ascii=False, indent=1)
    except (RuntimeError, NameError) as e:
        log("!! 값 덤프 " + str(e))
    obj = unreal.load_asset(system["refPath"])
    EAL.save_loaded_asset(obj)
    return obj


# ---------------------------------------------------------------- 실행

def main():
    for folder in ("/Textures", "/Materials", "/Systems"):
        if not EAL.does_directory_exist(ROOT + folder):
            EAL.make_directory(ROOT + folder)
    tex = import_textures()
    parent = make_additive_material(tex.get("T_Bolt_Streak"))
    # 인스턴스: (밝기 Intensity, 심 흰빛 CoreWhite)
    looks = {"Streak": (4.0, 6.0), "Wisp": (2.2, 1.0), "Spark": (5.0, 4.0), "Ring": (3.0, 2.0),
             "Flare": (1.2, 1.5), "Burst": (3.0, 3.0)}
    mi = {}
    for key, (intensity, core) in looks.items():
        t = tex.get("T_Bolt_" + key)
        if t is None:
            log("!! 텍스처 없음 T_Bolt_%s — 빛줄기로 대신" % key)
            t = tex["T_Bolt_Streak"]
        mi[key] = make_instance(parent, key, t, intensity, core)
    for make in (make_trail, make_cast, make_impact):
        try:
            make(mi)
        except Exception as e:  # noqa: BLE001 — 하나가 실패해도 나머지는 만든다
            import traceback
            log("!! %s 실패: %s\n%s" % (make.__name__, e, traceback.format_exc()))

    out_dir = os.path.join(unreal.Paths.project_saved_dir(), "MagicFX")
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "make_bolt_fx.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(LOG))


main()
