# Niagara 시스템을 사양(파이썬 dict)대로 조립한다 — 엔진 실험 플러그인 NiagaraToolsets(UE 5.8)의 도구를
# ToolsetRegistry.execute_tool 로 JSON 호출. 에디터를 띄우지 않고 커맨드렛에서 돈다. 설명은 docs/systems/magic-fx-authoring.md.
#
# 사양 한 덩어리 = 시스템 하나 = 에미터 여럿. 에미터는 엔진 기본 템플릿 하나에서 시작해 모듈 입력 · 렌더러 값을 덮어쓴다.
# 실패한 설정은 멈추지 않고 LOG 에 「!!」로 남긴다(실험 API — 입력 이름이 엔진 판마다 바뀔 수 있다).

import json
import os
import unreal

TR = unreal.ToolsetRegistry
SYS = "NiagaraToolsets.NiagaraToolset_System"
INFO = "NiagaraToolsets.NiagaraToolset_Info"
TEMPLATE_SYSTEM = "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst.DirectionalBurst"
EMITTER_TEMPLATES = "/Niagara/DefaultAssets/Templates/Emitters/"
MODULES = {
    "SpawnRate": "/Niagara/Modules/Emitter/SpawnRate.SpawnRate",
    "ScaleColor": "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor",
    "ScaleSpriteSize": "/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize",
    "SubUVAnimation": "/Niagara/Modules/Update/SubUV/V2/SubUVAnimation.SubUVAnimation",
    "SpriteRotationRate": "/Niagara/Modules/Update/Orientation/SpriteRotationRate.SpriteRotationRate",
}
ES, EU, PS, PU = "EmitterSpawnScript", "EmitterUpdateScript", "ParticleSpawnScript", "ParticleUpdateScript"

LOG = []


def log(msg):
    LOG.append(str(msg))
    unreal.log("[MagicFX] " + str(msg))


TRACE = False


def call(toolset, tool, args):
    if TRACE:
        unreal.log("[MagicFX] > %s %s" % (tool, json.dumps(args)[:300]))
    r = TR.execute_tool(toolset, tool, json.dumps(args))
    err = r.get_editor_property("error")
    if err:
        return None, err
    raw = r.get_value_as_json_string()
    try:
        v = json.loads(raw)
        v = json.loads(v) if isinstance(v, str) else v
    except ValueError:
        v = raw
    if isinstance(v, dict) and "returnValue" in v:
        v = v["returnValue"]
    return v, None


def ref(path):
    return {"refPath": path}


# ---------------------------------------------------------------- 값 만들기

def v_float(x):
    return {"struct": ref("/Script/Niagara.NiagaraFloat"), "value": {"value": float(x)}}


def v_int(x):
    return {"struct": ref("/Script/Niagara.NiagaraInt32"), "value": {"value": int(x)}}


def v_bool(x):
    return {"struct": ref("/Script/Niagara.NiagaraBool"), "value": {"value": -1 if x else 0}}


def v_vec3(x, y, z):
    return {"struct": ref("/Script/CoreUObject.Vector3f"), "value": {"x": x, "y": y, "z": z}}


def v_vec2(x, y):
    return {"struct": ref("/Script/CoreUObject.Vector2f"), "value": {"x": x, "y": y}}


def v_color(r, g, b, a=1.0):
    return {"struct": ref("/Script/CoreUObject.LinearColor"), "value": {"r": r, "g": g, "b": b, "a": a}}


def v_hlsl(expr):
    return {"struct": ref("/Script/NiagaraEditor.NiagaraExt_StackInputData_HlslExpression"), "value": {"hlslExpression": expr}}


class Enum:
    """표시 이름으로 고르는 열거 값 — 실제 이름(NewEnumeratorN)은 그 입력의 UEnum 에서 찾는다."""

    def __init__(self, display):
        self.display = display


# ---------------------------------------------------------------- 시스템

class SystemBuilder:
    def __init__(self, folder, name):
        self.folder = folder
        self.name = name
        self.path = "%s/%s.%s" % (folder, name, name)
        self.errors = 0
        self.enum_cache = {}

    def fail(self, what, err):
        self.errors += 1
        log("!! %s %s: %s" % (self.name, what, str(err)[:400]))

    def sys_ref(self):
        return ref(self.path)

    def item(self, emitter="", script="", module="", renderer=-1, inputs=()):
        return {"system": self.sys_ref(), "emitterName": emitter, "scriptName": script, "moduleName": module,
                "rendererIndex": renderer, "inputNameStack": list(inputs)}

    def create(self):
        pkg = "%s/%s" % (self.folder, self.name)
        # 이미 있으면 지우지 않고 에미터만 비워 다시 채운다 — 같은 세션에서 지우고 같은 이름으로 새로 만들면 저장 때 엔진이 죽는다(5.8).
        if not unreal.EditorAssetLibrary.does_asset_exist(pkg):
            v, err = call(SYS, "CreateNiagaraSystem", {"assetName": self.name, "assetPath": self.folder, "templateSystem": ref(TEMPLATE_SYSTEM)})
            if err:
                raise RuntimeError("시스템 생성 실패 %s: %s" % (self.name, err))
        summary, _ = call(SYS, "GetSystemSummary", {"system": self.sys_ref()})
        for e in (summary or {}).get("emitters", []):
            call(SYS, "RemoveEmitter", {"emitterToRemove": self.item(e["emitterName"])})

    def add_emitter(self, template, name):
        v, err = call(SYS, "AddEmitter", {"system": self.sys_ref(), "templateEmitter": ref(EMITTER_TEMPLATES + template + "." + template), "emitterName": name})
        if err:
            self.fail("에미터 " + name, err)
            return None
        return v.get("emitterName", name)

    def add_module(self, emitter, script, key):
        v, err = call(SYS, "AddModule", {"moduleLocationRef": self.item(emitter, script), "moduleAsset": ref(MODULES[key])})
        if err:
            self.fail("모듈 %s/%s" % (emitter, key), err)
            return None
        return v.get("moduleName", key)

    def module_inputs(self, emitter, script, module):
        v, err = call(SYS, "GetModuleInputValues", {"moduleRef": self.item(emitter, script, module)})
        return [i["name"] for i in (v or {}).get("inputs", [])] if not err else []

    def enum_value(self, emitter, script, module, inputs, display):
        cur, err = call(SYS, "GetStackInputData", {"stackInputRef": self.item(emitter, script, module, inputs=inputs)})
        if err or not isinstance(cur, dict):
            raise RuntimeError("열거 입력 읽기 실패: %s" % err)
        enum_path = cur.get("value", {}).get("enum", {}).get("refPath")
        if not enum_path:
            raise RuntimeError("열거 입력이 아님: %s" % json.dumps(cur)[:200])
        if enum_path not in self.enum_cache:
            info, err = call(INFO, "UEnum_Info", {"enum": ref(enum_path)})
            if isinstance(info, str):
                info = json.loads(info)
            self.enum_cache[enum_path] = info
        info = self.enum_cache[enum_path]
        # {"Name": "ENiagara…", "Values": [{"Name": "NewEnumerator1", "DisplayName": "Self", …}]} → "ENiagara…::NewEnumerator1"
        for entry in info.get("Values", []):
            if entry.get("DisplayName") == display:
                return {"struct": ref("/Script/NiagaraEditor.NiagaraExt_StackInputData_Enum"),
                        "value": {"enum": ref(enum_path), "enumName": "%s::%s" % (info.get("Name"), entry.get("Name"))}}
        raise RuntimeError("열거 값 %r 없음: %s" % (display, [e.get("DisplayName") for e in info.get("Values", [])]))

    def set(self, emitter, script, module, input_name, value):
        inputs = input_name if isinstance(input_name, (list, tuple)) else [input_name]
        try:
            if isinstance(value, Enum):
                value = self.enum_value(emitter, script, module, inputs, value.display)
        except RuntimeError as e:
            self.fail("%s/%s/%s" % (emitter, module, "/".join(inputs)), e)
            return False
        _, err = call(SYS, "SetStackInputData", {"stackInputRef": self.item(emitter, script, module, inputs=inputs), "inputData": value})
        if err:
            self.fail("%s/%s/%s" % (emitter, module, "/".join(inputs)), err)
            return False
        return True

    def set_renderer(self, emitter, index, props):
        _, err = call(SYS, "SetRendererData", {"renderer": self.item(emitter, renderer=index), "rendererData": {"propertyValues": json.dumps(props)}})
        if err:
            self.fail("렌더러 %s" % emitter, err)

    def set_emitter_data(self, emitter, props):
        _, err = call(SYS, "SetEmitterData", {"emitter": self.item(emitter), "emitterData": {"propertyValues": json.dumps(props)}})
        if err:
            self.fail("에미터 값 %s" % emitter, err)

    def save(self):
        if TRACE:
            unreal.log("[MagicFX] > 저장 " + self.path)
        system = unreal.load_asset(self.path)
        unreal.GrimMagicFXEditorLibrary.compile_niagara_system_and_wait(system, True)
        unreal.EditorAssetLibrary.save_loaded_asset(system)
        state, _ = call(SYS, "GetSystemCompileState", {"system": self.sys_ref()})
        issues = []
        if isinstance(state, dict):
            for s in state.get("scripts", []):
                for ev in s.get("compileEvents", []):
                    if ev.get("severity") in ("Error", "Warning"):
                        issues.append("%s %s: %s" % (s.get("emitterName"), ev.get("severity"), ev.get("message", "")[:200]))
            log("시스템 %s 컴파일=%s 오류=%s 설정 실패=%d" % (self.path, state.get("aggregateStatus"), state.get("bHasErrors"), self.errors))
        for i in issues[:12]:
            log("   " + i)
        return system


# ---------------------------------------------------------------- 에미터 사양 → 도구 호출

def build_emitter(b, spec):
    """spec 키(모두 선택): template · name · material · loop('once'|'infinite') · duration · burst · rate · lifetime(a,b)
    · color(r,g,b,a) · size(a,b) · size_xy((x,y),(x,y)) · rotation(bool) · spin(deg/s) · speed(a,b) · cone · axis(x,y,z)
    · radius · gravity(z) · drag · alpha(hlsl) · rgb(hlsl) · grow(hlsl float) · subuv(cols) · local(bool) · align · ribbon_width
    · renderer(dict 덮어쓰기) · raw[(script, module, input, value)]"""
    name = b.add_emitter(spec["template"], spec["name"])
    if not name:
        return
    s = spec
    tpl = s["template"]

    # 수명 주기: 스스로 돈다, 한 번/계속.
    b.set(name, EU, "EmitterState", "Life Cycle Mode", Enum("Self"))
    b.set(name, EU, "EmitterState", "Loop Behavior", Enum("Infinite" if s.get("loop") == "infinite" else "Once"))
    if "duration" in s:
        b.set(name, EU, "EmitterState", "Loop Duration", v_float(s["duration"]))

    # 낳기: 한 번에(burst) 또는 초당(rate).
    if "burst" in s:
        b.set(name, EU, "SpawnBurst_Instantaneous", "Spawn Count", v_int(s["burst"]))
    if "rate" in s:
        if tpl != "Fountain":
            b.add_module(name, EU, "SpawnRate")
        b.set(name, EU, "SpawnRate", "SpawnRate", v_float(s["rate"]))

    init = "InitializeParticle"
    if "lifetime" in s:
        lo, hi = s["lifetime"]
        b.set(name, PS, init, "Lifetime Mode", Enum("Random"))
        b.set(name, PS, init, "Lifetime Min", v_float(lo))
        b.set(name, PS, init, "Lifetime Max", v_float(hi))
    if "color" in s:
        b.set(name, PS, init, "Color Mode", Enum("Direct Set"))
        b.set(name, PS, init, "Color", v_color(*s["color"]))
    if "size" in s:
        lo, hi = s["size"]
        b.set(name, PS, init, "Sprite Size Mode", Enum("Random Uniform"))
        b.set(name, PS, init, "Uniform Sprite Size Min", v_float(lo))
        b.set(name, PS, init, "Uniform Sprite Size Max", v_float(hi))
    if "size_xy" in s:
        lo, hi = s["size_xy"]
        b.set(name, PS, init, "Sprite Size Mode", Enum("Random Non-Uniform"))
        b.set(name, PS, init, "Sprite Size Min", v_vec2(*lo))
        b.set(name, PS, init, "Sprite Size Max", v_vec2(*hi))
    if s.get("rotation"):
        b.set(name, PS, init, "Sprite Rotation Mode", Enum("Random"))
        b.set(name, PS, init, "Sprite Rotation Angle Min", v_float(0))
        b.set(name, PS, init, "Sprite Rotation Angle Max", v_float(360))
    if "ribbon_width" in s:
        b.set(name, PS, init, "Ribbon Width Mode", Enum("Direct Set"))
        b.set(name, PS, init, "Ribbon Width", v_float(s["ribbon_width"]))

    if "radius" in s:
        b.set(name, PS, "ShapeLocation", "Sphere Radius", v_float(s["radius"]))
    if "speed" in s:
        lo, hi = s["speed"]
        if not (b.set(name, PS, "AddVelocity", ["Velocity Speed", "Minimum"], v_float(lo))
                and b.set(name, PS, "AddVelocity", ["Velocity Speed", "Maximum"], v_float(hi))):
            b.set(name, PS, "AddVelocity", "Velocity Speed", v_float((lo + hi) * 0.5))
    if "cone" in s:
        b.set(name, PS, "AddVelocity", "Cone Angle", v_float(s["cone"]))
    if "axis" in s:
        b.set(name, PS, "AddVelocity", "Cone Axis", v_vec3(*s["axis"]))
    if "gravity" in s:
        b.set(name, PU, "GravityForce", "Gravity", v_vec3(0, 0, s["gravity"]))
    if "drag" in s:
        b.set(name, PU, "Drag", "Drag", v_float(s["drag"]))

    # 수명에 따라: 알파(침식 머티리얼은 알파가 줄면 가장자리부터 깎인다) · 색 · 크기.
    if "alpha" in s or "rgb" in s:
        if tpl in ("SingleLoopingParticle", "LocationBasedRibbon", "SimpleSpriteBurst"):
            b.add_module(name, PU, "ScaleColor")
        if "alpha" in s:
            b.set(name, PU, "ScaleColor", "Scale Alpha", v_hlsl(s["alpha"]))
        if "rgb" in s:
            b.set(name, PU, "ScaleColor", "Scale RGB", v_hlsl(s["rgb"]))
    if "grow" in s:
        if tpl == "OmnidirectionalBurst":
            mod = "ScaleSpriteSize"
        else:
            mod = b.add_module(name, PU, "ScaleSpriteSize")
        if mod and tpl != "OmnidirectionalBurst":
            # 새로 붙인 모듈은 곡선 모드가 기본 — 균일 배율 모드로 바꾸고 식을 넣는다.
            b.set(name, PU, mod, "Scale Sprite Size Mode", Enum("Uniform"))
            b.set(name, PU, mod, "Uniform Scale Factor", v_hlsl(s["grow"]))
        elif mod:
            b.set(name, PU, mod, "Scale Factor", v_hlsl("float2(1.0, 1.0) * (%s)" % s["grow"]))
    if "spin" in s:
        mod = b.add_module(name, PU, "SpriteRotationRate")
        if mod:
            b.set(name, PU, mod, "Rotation Rate", v_float(s["spin"]))
    if "subuv" in s:
        mod = b.add_module(name, PU, "SubUVAnimation")
        if mod:
            # 칸 수는 스프라이트 렌더러의 SubImageSize 를 읽는다 — 수명 동안 한 바퀴.
            b.set(name, PU, mod, "SubUV Animation Mode", Enum("Linear"))

    for script, module, input_name, value in s.get("raw", []):
        b.set(name, script, module, input_name, value)

    if s.get("local"):
        b.set_emitter_data(name, {"bLocalSpace": True})

    props = {}
    if "material" in s:
        props["Material"] = ref(s["material"])
    if "subuv" in s:
        props["SubImageSize"] = {"x": s["subuv"], "y": s["subuv"]}
        props["bSubImageBlend"] = True
    if "align" in s and os.environ.get("GRIM_FX_NOALIGN") != "1":
        props["Alignment"] = s["align"]
    if tpl != "LocationBasedRibbon":
        props["SortMode"] = "None"
        props["bCastShadows"] = False
    props.update(s.get("renderer", {}))
    if props:
        b.set_renderer(name, 0, props)


def build_system(folder, name, emitters):
    b = SystemBuilder(folder, name)
    b.create()
    only = [x for x in os.environ.get("GRIM_FX_EMITTERS", "").split(",") if x]
    for e in emitters:
        if not only or e["name"] in only:
            build_emitter(b, e)
    return b.save()
