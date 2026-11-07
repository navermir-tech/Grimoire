# 볼트 빛 언어(바늘 빛줄기 · 불티 · 고리 · 번쩍임, 가산 발광 MI_Bolt_*)로 Niagara 시스템을 JSON 사양에서 조립한다.
# make_spell_fx.py 가 부른다. 도구 호출 방식은 make_bolt_fx.py 와 같다(NiagaraToolsets JSON 도구 — docs/systems/magic-fx-authoring.md).
#
# 사양 파일(art/fx/specs/spell/*.json):
#   {"folder": "/Game/...", "systems": {"NS_이름": {"user": {"Length": 750.0, "EndPoint": [0, 0, 0]}, "emitters": [에미터, ...]}}}
# 사용자 변수는 수(float) 또는 [x, y, z](vec3). User.Color 는 늘 있다(TintNiagara 가 마법 색으로 덮어쓴다).
# 에미터(모두 선택 키, kind · name · mi 는 필수):
#   kind      "burst"(한 번 터짐, OmnidirectionalBurst) · "still"(한 장 제자리) · "stream"(계속, Fountain)
#   mi        머티리얼 인스턴스 MI_Bolt_<이름> (Streak · Wisp · Spark · Ring · Flare · Burst)
#   aligned   속도 정렬(스프라이트 위 = 진행 방향, 크기 = (폭, 길이))
#   count · rate · lifetime [a, b] · size [a, b]
#   disable   [[단계, 모듈]]           단계: spawn · update · emitter_update
#   set       [[단계, 모듈, 입력, 값]]  값: {"float": x} · {"int": n} · {"vec3": [x, y, z]} · {"hlsl": "식"} · {"link": "User.X", "type": "float"}
#   spawn · update  [[속성, 타입, HLSL 식]]  타입: float · vec2 · vec3 · position · color — 단계 끝에 Set Parameters 로 덮어쓴다
#   renderer  스프라이트 렌더러 값 덮어쓰기(예: {"PivotInUVSpace": {"x": 0.5, "y": 1.0}})
#   defs      {"U": "식"} — HLSL 안의 {U} 를 그 식으로 바꾼다(겹쳐 써도 된다). 무작위 수가 없어 수명 난수를 0~1 로 펴 쓴다.
#   max_life  수명을 식으로 덮어쓸 때 입자 수 어림에 쓸 최대 수명(초)

import json
import os

import unreal

EAL = unreal.EditorAssetLibrary
TOOLSET = "NiagaraToolsets.NiagaraToolset_System"
INFO = "NiagaraToolsets.NiagaraToolset_Info"
EMITTERS = "/Niagara/DefaultAssets/Templates/Emitters/"
STAGES = {"spawn": "ParticleSpawnScript", "update": "ParticleUpdateScript", "emitter_update": "EmitterUpdateScript"}
TYPES = {
    "float": "/Script/Niagara.NiagaraFloat",
    "int": "/Script/Niagara.NiagaraInt32",
    "vec2": "/Script/CoreUObject.Vector2f",
    "vec3": "/Script/CoreUObject.Vector3f",
    "position": "/Script/Niagara.NiagaraPosition",
    "color": "/Script/CoreUObject.LinearColor",
}
DEFAULTS = {
    "float": lambda: {"value": 0.0},
    "int": lambda: {"value": 0},
    "vec2": lambda: {"x": 0.0, "y": 0.0},
    "vec3": lambda: {"x": 0.0, "y": 0.0, "z": 0.0},
    "position": lambda: {"x": 0.0, "y": 0.0, "z": 0.0},
    "color": lambda: {"r": 1.0, "g": 1.0, "b": 1.0, "a": 1.0},
}
LIFE_CYCLE = "/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode"
LOOP = "/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions"

LOG = []


def log(msg):
    LOG.append(str(msg))
    unreal.log("[SpellFX] " + str(msg))


class Failures:
    count = 0


def tool(name, args, toolset=TOOLSET):
    res = unreal.ToolsetRegistry.execute_tool(toolset, name, json.dumps(args))
    if res.error:
        raise RuntimeError("%s: %s" % (name, res.error))
    value = json.loads(res.value) if res.value else None
    return value.get("returnValue") if isinstance(value, dict) else value


def stack_ref(system, emitter="", script="", module="", renderer=-1, inputs=()):
    return {"system": system, "emitterName": emitter, "scriptName": script, "moduleName": module,
            "rendererIndex": renderer, "inputNameStack": list(inputs)}


ENUMS = {}


def v_enum(enum_path, display):
    if enum_path not in ENUMS:
        info = tool("UEnum_Info", {"enum": {"refPath": enum_path}}, INFO)
        ENUMS[enum_path] = json.loads(info) if isinstance(info, str) else info
    info = ENUMS[enum_path]
    for entry in info.get("Values", []):
        if entry.get("DisplayName") == display:
            return {"struct": {"refPath": "/Script/NiagaraEditor.NiagaraExt_StackInputData_Enum"},
                    "value": {"enum": {"refPath": enum_path}, "enumName": "%s::%s" % (info.get("Name"), entry.get("Name")),
                              "displayName": display}}
    raise RuntimeError("열거 값 없음 %s %s" % (enum_path, display))


def value_of(spec):
    """사양의 값 표기 → 툴셋 입력 데이터."""
    if "float" in spec:
        return {"struct": {"refPath": TYPES["float"]}, "value": {"value": float(spec["float"])}}
    if "int" in spec:
        return {"struct": {"refPath": TYPES["int"]}, "value": {"value": int(spec["int"])}}
    if "vec3" in spec:
        x, y, z = spec["vec3"]
        return {"struct": {"refPath": TYPES["vec3"]}, "value": {"x": x, "y": y, "z": z}}
    if "hlsl" in spec:
        return {"struct": {"refPath": "/Script/NiagaraEditor.NiagaraExt_StackInputData_HlslExpression"},
                "value": {"hlslExpression": spec["hlsl"]}}
    if "link" in spec:
        return {"struct": {"refPath": "/Script/NiagaraEditor.NiagaraExt_StackInputData_Linked"},
                "value": {"linkedVariable": {"name": spec["link"],
                                             "type": {"classStructOrEnum": {"refPath": TYPES[spec.get("type", "float")]}}}}}
    raise RuntimeError("모르는 값 표기 %s" % spec)


class Emitter:
    def __init__(self, system, template, name):
        self.system = system
        top = tool("AddEmitter", {"system": system, "templateEmitter": {"refPath": EMITTERS + template + "." + template},
                                  "emitterName": name})
        self.name = top.get("emitterName", name)

    def fail(self, what, err):
        Failures.count += 1
        log("!! %s %s: %s" % (self.name, what, str(err)[:300]))

    def set(self, script, module, input_name, value):
        try:
            tool("SetStackInputData", {"stackInputRef": stack_ref(self.system, self.name, script, module, inputs=[input_name]),
                                       "inputData": value})
        except RuntimeError as e:
            self.fail("%s.%s" % (module, input_name), e)

    def disable(self, script, module):
        try:
            tool("SetModuleEnabled", {"moduleRef": stack_ref(self.system, self.name, script, module), "bEnabled": False})
        except RuntimeError as e:
            self.fail("끄기 " + module, e)

    def override(self, script, params):
        """params: [(속성, 타입 키, HLSL)] — 단계 끝 Set Parameters 모듈 하나로."""
        entries = [{"variable": {"name": n, "type": {"classStructOrEnum": {"refPath": TYPES[t]}}},
                    "defaultValue": {"struct": {"refPath": TYPES[t]}, "value": DEFAULTS[t]()}} for n, t, _ in params]
        try:
            top = tool("AddSetParametersModule", {"moduleLocationRef": stack_ref(self.system, self.name, script), "parameters": entries})
        except RuntimeError as e:
            self.fail("덮어쓰기 " + script, e)
            return
        module = top["moduleName"]
        for n, _, expression in params:
            self.set(script, module, n, value_of({"hlsl": expression}))

    def renderer(self, values):
        try:
            tool("SetRendererData", {"renderer": stack_ref(self.system, self.name, renderer=0),
                                     "rendererData": {"propertyValues": json.dumps(values)}})
        except RuntimeError as e:
            self.fail("렌더러", e)

    def life_cycle(self, loop):
        try:
            self.set(STAGES["emitter_update"], "EmitterState", "Life Cycle Mode", v_enum(LIFE_CYCLE, "Self"))
            self.set(STAGES["emitter_update"], "EmitterState", "Loop Behavior", v_enum(LOOP, loop))
        except RuntimeError as e:
            self.fail("수명 주기", e)


def new_system(folder, name, user):
    """있으면 지우지 않고 비워 다시 채운다(같은 세션에서 지우고 같은 이름으로 만들면 저장 때 엔진이 죽는다)."""
    path = folder + "/" + name
    if EAL.does_asset_exist(path):
        system = {"refPath": EAL.load_asset(path).get_path_name()}
        old_vars = tool("GetUserVariables", {"system": system}).get("userVariables", [])
        if old_vars:
            tool("RemoveUserVariables", {"system": system, "variablesToRemove": [{"name": v["name"], "type": v["type"]} for v in old_vars]})
    else:
        system = tool("CreateNiagaraSystem", {"assetName": name, "assetPath": folder,
                                              "templateSystem": {"refPath": "/Niagara/DefaultAssets/DefaultSystem.DefaultSystem"}})
    for em in tool("GetSystemSummary", {"system": system})["emitters"]:
        tool("RemoveEmitter", {"emitterToRemove": stack_ref(system, em["emitterName"])})

    variables = [{"name": "User.Color", "type": {"classStructOrEnum": {"refPath": TYPES["color"]}},
                  "defaultValue": {"struct": {"refPath": TYPES["color"]}, "value": {"r": 0.6, "g": 0.75, "b": 1.0, "a": 1.0}},
                  "description": "속성 색(UGrimSpellVisualSet::TintNiagara 가 덮어쓴다)"}]
    for key, default in (user or {}).items():
        if isinstance(default, (list, tuple)):
            t, value = "vec3", {"x": default[0], "y": default[1], "z": default[2]}
        else:
            t, value = "float", {"value": float(default)}
        variables.append({"name": "User." + key, "type": {"classStructOrEnum": {"refPath": TYPES[t]}},
                          "defaultValue": {"struct": {"refPath": TYPES[t]}, "value": value}, "description": "C++ 가 넣는 값(기본값은 미리 보기용)"})
    tool("AddUserVariables", {"system": system, "variablesToAdd": variables})
    return system


TEMPLATES = {"burst": "OmnidirectionalBurst", "still": "OmnidirectionalBurst", "stream": "Fountain"}


def expand(expression, defs):
    for _ in range(6):
        before = expression
        for key, value in defs.items():
            expression = expression.replace("{%s}" % key, "(%s)" % value)
        if expression == before:
            break
    return expression


def estimate_particles(spec):
    """동시에 살아 있을 입자 수 어림: 한 장 = 1, 터짐 = 개수, 계속 = 초당 × 최대 수명."""
    if spec["kind"] == "still":
        return 1
    if spec["kind"] == "burst":
        return int(spec.get("count", 10))
    return int(round(spec.get("rate", 50.0) * spec.get("max_life", spec.get("lifetime", [0.2, 0.2])[1])))


def build_emitter(system, spec, mi):
    kind = spec["kind"]
    defs = spec.get("defs", {})
    e = Emitter(system, TEMPLATES[kind], spec["name"])
    spawn, update, emitter_update = STAGES["spawn"], STAGES["update"], STAGES["emitter_update"]
    if kind == "stream":
        # 스스로 끝없이 돈다 — 시스템 수명을 따르면 줄기가 나는 동안 꺼질 수 있다. 끄는 것은 C++(Deactivate).
        e.life_cycle("Infinite")
        e.set(emitter_update, "SpawnRate", "SpawnRate", value_of({"float": spec.get("rate", 50.0)}))
    else:
        e.life_cycle("Once")
        e.set(emitter_update, "SpawnBurst_Instantaneous", "Spawn Count", value_of({"int": 1 if kind == "still" else spec.get("count", 10)}))

    lo, hi = spec.get("lifetime", [0.2, 0.2])
    e.set(spawn, "InitializeParticle", "Lifetime Min", value_of({"float": lo}))
    e.set(spawn, "InitializeParticle", "Lifetime Max", value_of({"float": hi}))
    s_lo, s_hi = spec.get("size", [1.0, 1.0])
    e.set(spawn, "InitializeParticle", "Uniform Sprite Size Min", value_of({"float": s_lo}))
    e.set(spawn, "InitializeParticle", "Uniform Sprite Size Max", value_of({"float": s_hi}))
    e.set(spawn, "InitializeParticle", "Color", value_of({"link": "User.Color", "type": "color"}))

    disabled = [tuple(x) for x in spec.get("disable", [])]
    if kind == "still":
        disabled += [("spawn", "ShapeLocation"), ("update", "ScaleSpriteSize"), ("update", "ScaleColor")]
    for stage, module in dict.fromkeys(disabled):
        e.disable(STAGES[stage], module)
    for stage, module, input_name, value in spec.get("set", []):
        if "hlsl" in value:
            value = {"hlsl": expand(value["hlsl"], defs)}
        e.set(STAGES[stage], module, input_name, value_of(value))

    spawn_params = list(spec.get("spawn", []))
    if kind == "still" and not any(p[0] == "Particles.Velocity" for p in spawn_params):
        spawn_params.insert(0, ["Particles.Velocity", "vec3", "float3(0, 0, 0)"])
    if spawn_params:
        e.override(spawn, [(n, t, expand(x, defs)) for n, t, x in spawn_params])
    if spec.get("update"):
        e.override(update, [(n, t, expand(x, defs)) for n, t, x in spec["update"]])

    values = {"Material": {"refPath": mi[spec["mi"]].get_path_name()},
              "Alignment": "VelocityAligned" if spec.get("aligned") else "Unaligned",
              "FacingMode": "FaceCamera", "SortMode": "None", "bCastShadows": False}
    values.update(spec.get("renderer", {}))
    e.renderer(values)


def finish(system, effect_type):
    name = system["refPath"].rsplit(".", 1)[-1]
    summary = tool("GetSystemSummary", {"system": system})
    try:
        dump = {em["emitterName"]: tool("GetEmitterInputValues", {"emitterRef": stack_ref(system, em["emitterName"])})
                for em in summary["emitters"]}
        out_dir = os.path.join(unreal.Paths.project_saved_dir(), "MagicFX")
        os.makedirs(out_dir, exist_ok=True)
        with open(os.path.join(out_dir, name + ".json"), "w", encoding="utf-8") as f:
            json.dump(dump, f, ensure_ascii=False, indent=1)
    except RuntimeError as e:
        log("!! 값 덤프 %s: %s" % (name, e))
    obj = unreal.load_asset(system["refPath"])
    if effect_type is not None:
        obj.set_editor_property("effect_type", effect_type)
    unreal.GrimMagicFXEditorLibrary.compile_niagara_system_and_wait(obj, True)
    EAL.save_loaded_asset(obj)
    try:
        state = tool("GetSystemCompileState", {"system": system})
        issues = ["%s %s: %s" % (sc.get("emitterName"), ev.get("severity"), ev.get("message", "")[:200])
                  for sc in (state or {}).get("scripts", []) for ev in sc.get("compileEvents", [])
                  if ev.get("severity") in ("Error", "Warning")]
        log("  %s 컴파일=%s 오류=%s" % (name, state.get("aggregateStatus"), state.get("bHasErrors")))
        for issue in issues[:12]:
            Failures.count += 1
            log("  !! " + issue)
    except RuntimeError as e:
        log("!! 컴파일 상태 %s: %s" % (name, e))
    log("시스템 %s 이미터 %s 사용자 변수 %s" % (system["refPath"], [em["emitterName"] for em in summary["emitters"]],
                                         [v.get("name") for v in summary.get("userVariables", [])] if isinstance(summary.get("userVariables"), list) else summary.get("userVariables")))
    return obj


def build_system(folder, name, spec, mi, effect_type):
    before = Failures.count
    total = sum(estimate_particles(em) for em in spec["emitters"])
    log("%s 입자 어림 %d (상한 2,000 — 성능 규칙 24)" % (name, total))
    if total > 2000:
        Failures.count += 1
        log("!! %s 입자 상한 넘음" % name)
    system = new_system(folder, name, spec.get("user"))
    for em in spec["emitters"]:
        try:
            build_emitter(system, em, mi)
        except RuntimeError as e:
            Failures.count += 1
            log("!! %s/%s: %s" % (name, em.get("name"), e))
    obj = finish(system, effect_type)
    log("  %s 설정 실패=%d" % (name, Failures.count - before))
    return obj
