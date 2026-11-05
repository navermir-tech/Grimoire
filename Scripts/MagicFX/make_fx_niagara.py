# 마법 이펙트 자체 제작(2026-11-06): 텍스처 들여오기 → 파티클 머티리얼 → Niagara 시스템을 사양대로 조립.
# 사양은 GPT 콘셉트 시트(art/fx/raw/<마법>.png, 다듬은 것은 docs/public/art/fx/)를 보고 이 파일에 적는다 — 값은 이 파일이 정본이다.
# 연출 묶음(VS_*)이 이 시스템을 가리키게 하는 것은 make_magicfx_assets.py.
#
# 실행(에디터를 닫고, 저장소 루트에서. 먼저 python Scripts/MagicFX/make_fx_textures.py):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\MagicFX\make_fx_niagara.py" -unattended -nop4 -nosplash -nullrhi
# 결과는 Saved/MagicFX/make_fx_niagara.txt. 일부만: GRIM_FX_SYSTEMS=NS_GrimEmber_Cast,NS_GrimEmber_Trail (비우면 전부)
# 다른 사양 파일로: GRIM_FX_SPEC=<json 절대 경로> (형식은 load_spec, 예: art/fx/specs/example.json)
# 함정: 스크립트는 끝까지 돌고 저장도 되지만, 엔진 종료 때 UObjectArray 단언으로 종료 코드 3 이 날 수 있다(실험 플러그인).
#       결과 파일의 「설정 실패=0」 과 컴파일 상태로 판단한다.

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import unreal  # noqa: E402
import fx_materials  # noqa: E402
import fx_niagara_builder as nb  # noqa: E402
from fx_niagara_builder import ES, EU, PS, PU, Enum, v_float  # noqa: E402,F401

FX_DIR = "/Game/Grimoire/MagicFX/Niagara"
M = fx_materials.PMAT_DIR + "/"
MAT_ADD = M + "M_GrimFX_PAdd.M_GrimFX_PAdd"
MAT_FIRE = M + "M_GrimFX_PFireSubUV.M_GrimFX_PFireSubUV"
MAT_SMOKE = M + "M_GrimFX_PSmoke.M_GrimFX_PSmoke"
MAT_RIBBON = M + "M_GrimFX_RibbonAdd.M_GrimFX_RibbonAdd"
MAT_FLARE4 = M + "MI_GrimFX_PAdd_Flare4.MI_GrimFX_PAdd_Flare4"
MAT_FLARE6 = M + "MI_GrimFX_PAdd_Flare6.MI_GrimFX_PAdd_Flare6"
MAT_RING = M + "MI_GrimFX_PAdd_Ring.MI_GrimFX_PAdd_Ring"
MAT_SPARK = M + "MI_GrimFX_PAdd_Spark.MI_GrimFX_PAdd_Spark"

FADE = "1.0 - Particles.NormalizedAge"
FADE2 = "pow(1.0 - Particles.NormalizedAge, 2.0)"
VEL = "VelocityAligned"

# 색(선형). 불 #E0582B · 순수 마력 #CFE3FF(art/costumes-tutorial · make_magicfx_assets 와 같은 속성색)을 기준으로,
# 파티클 색은 밝게(머티리얼이 진한 곳을 흰빛으로 바꿔 심을 만든다).
FIRE = (1.0, 0.26, 0.04, 1.0)
FIRE_DEEP = (0.9, 0.16, 0.03, 1.0)
EMBER_SPARK = (1.0, 0.45, 0.12, 1.0)
SMOKE = (0.09, 0.065, 0.05, 1.0)
MANA = (0.62, 0.78, 1.0, 1.0)
MANA_HALO = (0.55, 0.55, 1.0, 0.7)
MANA_CORE = (1.3, 1.45, 1.9, 1.0)


def one_shot(name, material, size, life, color, alpha=FADE2, grow="1.0", **extra):
    """제자리에서 한 장 번쩍 — 섬광 · 고리 · 광륜."""
    spec = dict(template="OmnidirectionalBurst", name=name, material=material, burst=1, lifetime=(life, life),
                size=(size, size), color=color, speed=(0, 0), radius=0.0, gravity=0.0, drag=0.0, alpha=alpha, grow=grow)
    spec.update(extra)
    return spec


SYSTEMS = {
    # ---------------------------------------------------------------- 불씨 (시트 art/fx/raw/ember.png)
    # 1 시전: 별 섬광 + 앞으로 튀는 불꽃 혀 + 불티.
    "NS_GrimEmber_Cast": [
        one_shot("Flash", MAT_FLARE6, 70, 0.14, (1.0, 0.38, 0.08, 1.0), grow="0.7 + 0.8 * Particles.NormalizedAge", rotation=True),
        one_shot("Glow", MAT_ADD, 24, 0.16, FIRE),
        dict(template="DirectionalBurst", name="Licks", material=MAT_FIRE, subuv=4, burst=7, lifetime=(0.16, 0.28),
             size=(18, 30), color=FIRE, speed=(220, 520), cone=28, axis=(0, 0, 1), gravity=40, drag=7, alpha=FADE,
             rotation=True, raw=[(PU, "ScaleSpriteSizeBySpeed", "Velocity Threshold", v_float(4000))]),
        dict(template="DirectionalBurst", name="Sparks", material=MAT_SPARK, burst=14, lifetime=(0.2, 0.45),
             size_xy=((2.0, 5.0), (3.0, 9.0)), color=EMBER_SPARK, speed=(350, 900), cone=40, axis=(0, 0, 1),
             gravity=-500, drag=2.5, alpha=FADE, align=VEL),
    ],
    # 2 · 3 몸통 + 꼬리(투사체에 붙는다, 투사체 앞 = 로컬 +X): 흰 심 · 너울 불꽃 껍질 · 풀리는 불꽃 띠 · 불티 · 옅은 연기.
    "NS_GrimEmber_Trail": [
        dict(template="SingleLoopingParticle", name="Core", material=MAT_ADD, burst=1, lifetime=(4, 4), size=(30, 30),
             color=(1.0, 0.5, 0.15, 1.0), local=True, grow="0.9 + 0.12 * sin(Engine.Time * 40.0)"),
        dict(template="Fountain", name="Shell", material=MAT_FIRE, subuv=4, loop="infinite", rate=140, lifetime=(0.16, 0.3),
             size=(26, 40), color=FIRE, speed=(10, 60), cone=180, axis=(-1, 0, 0), gravity=60, drag=3, alpha=FADE,
             rotation=True, radius=5.0),
        dict(template="LocationBasedRibbon", name="Tail", material=MAT_RIBBON, loop="infinite", rate=120, lifetime=(0.3, 0.3),
             color=FIRE_DEEP, ribbon_width=26, alpha=FADE),
        dict(template="Fountain", name="Sparks", material=MAT_SPARK, loop="infinite", rate=45, lifetime=(0.3, 0.6),
             size_xy=((1.5, 4.0), (2.5, 7.0)), color=EMBER_SPARK, speed=(40, 160), cone=70, axis=(-1, 0, 0),
             gravity=-250, drag=1.5, alpha=FADE, align=VEL, radius=6.0),
        dict(template="Fountain", name="Smoke", material=MAT_SMOKE, subuv=4, loop="infinite", rate=12, lifetime=(0.6, 0.9),
             size=(14, 22), color=SMOKE, speed=(5, 25), cone=180, axis=(0, 0, 1), gravity=50, drag=1.0, alpha=FADE,
             grow="1.0 + 1.5 * Particles.NormalizedAge", rotation=True, radius=4.0),
    ],
    # 4 터짐: 큰 섬광 · 바깥으로 말리는 불꽃 꽃잎 · 사방 불티 · 오르는 연기(그을음 링은 C++ 데칼).
    "NS_GrimEmber_Impact": [
        one_shot("Flash", MAT_FLARE6, 200, 0.14, (1.0, 0.42, 0.1, 1.0), grow="0.6 + 0.9 * Particles.NormalizedAge", rotation=True),
        one_shot("Glow", MAT_ADD, 160, 0.3, FIRE),
        dict(template="OmnidirectionalBurst", name="Petals", material=MAT_FIRE, subuv=4, burst=22, lifetime=(0.5, 0.8),
             size=(50, 85), color=FIRE, speed=(180, 420), radius=8.0, gravity=80, drag=4.5, alpha=FADE,
             grow="0.7 + 0.9 * Particles.NormalizedAge", rotation=True),
        dict(template="DirectionalBurst", name="Sparks", material=MAT_SPARK, burst=32, lifetime=(0.4, 0.9),
             size_xy=((2.0, 5.0), (3.5, 11.0)), color=EMBER_SPARK, speed=(400, 1100), cone=75, axis=(0, 0, 1),
             gravity=-900, drag=1.5, alpha=FADE, align=VEL),
        dict(template="OmnidirectionalBurst", name="Smoke", material=MAT_SMOKE, subuv=4, burst=5, lifetime=(1.4, 2.0),
             size=(30, 50), color=SMOKE, speed=(40, 90), radius=15.0, gravity=140, drag=1.2,
             alpha="0.8 * sin(3.1416 * pow(Particles.NormalizedAge, 0.7))",
             grow="0.8 + 2.2 * Particles.NormalizedAge", rotation=True),
    ],

    # ---------------------------------------------------------------- 기본 공격 (시트 art/fx/raw/basic-attack.png)
    # 1 시전: 얇은 마법진 고리 + 별 섬광 + 앞으로 짧은 빛살.
    "NS_GrimBasic_Cast": [
        one_shot("Ring", MAT_RING, 50, 0.2, MANA, grow="0.8 + 0.6 * Particles.NormalizedAge"),
        one_shot("Flash", MAT_FLARE4, 55, 0.12, MANA_CORE, rotation=True),
        dict(template="DirectionalBurst", name="Streaks", material=MAT_SPARK, burst=3, lifetime=(0.08, 0.14),
             size_xy=((2.0, 14.0), (3.0, 24.0)), color=MANA, speed=(900, 1400), cone=6, axis=(0, 0, 1),
             gravity=0, drag=8, alpha=FADE, align=VEL),
    ],
    # 2 · 3 몸통 + 꼬리: 밝은 심 · 보랏빛 광륜 · 가는 빛 리본 · 빛 알갱이.
    "NS_GrimBasic_Trail": [
        dict(template="SingleLoopingParticle", name="Core", material=MAT_ADD, burst=1, lifetime=(4, 4), size=(26, 26),
             color=MANA_CORE, local=True),
        dict(template="SingleLoopingParticle", name="Halo", material=MAT_ADD, burst=1, lifetime=(4, 4), size=(70, 70),
             color=MANA_HALO, local=True, grow="0.92 + 0.1 * sin(Engine.Time * 25.0)"),
        dict(template="LocationBasedRibbon", name="Tail", material=MAT_RIBBON, loop="infinite", rate=120, lifetime=(0.18, 0.18),
             color=MANA, ribbon_width=11, alpha=FADE),
        dict(template="Fountain", name="Glints", material=MAT_FLARE4, loop="infinite", rate=28, lifetime=(0.25, 0.45),
             size=(2.5, 4.5), color=MANA_CORE, speed=(10, 40), cone=180, axis=(-1, 0, 0), gravity=0, drag=3,
             alpha=FADE, radius=4.0),
    ],
    # 4 터짐: 별 섬광 + 퍼지는 빛 고리 + 흩어지는 빛 알갱이. 작고 깔끔하게.
    "NS_GrimBasic_Impact": [
        one_shot("Flash", MAT_FLARE4, 170, 0.16, MANA_CORE, grow="0.7 + 0.6 * Particles.NormalizedAge", rotation=True),
        one_shot("Ring", MAT_RING, 45, 0.3, MANA, alpha=FADE, grow="1.0 + 3.5 * Particles.NormalizedAge"),
        one_shot("Glow", MAT_ADD, 80, 0.22, MANA_HALO),
        dict(template="OmnidirectionalBurst", name="Glints", material=MAT_FLARE4, burst=16, lifetime=(0.3, 0.55),
             size=(5, 9), color=MANA_CORE, speed=(150, 400), radius=2.0, gravity=0, drag=5, alpha=FADE),
    ],
}


def load_spec(path):
    """JSON 사양 파일 → (폴더, {시스템: [에미터 dict]}). 다른 마법 · 다른 세션이 이 파일을 고치지 않고 이펙트를 만들 때.
    형식 예: art/fx/specs/example.json — 에미터 키는 fx_niagara_builder.build_emitter 설명과 같다(튜플 대신 리스트).
    "material" 에는 에셋 경로 또는 이 스크립트의 별칭(add · fire · smoke · ribbon · flare4 · flare6 · ring · spark).
    "raw" 는 [스크립트, 모듈, 입력, {"float": 1.0} | {"int": 1} | {"bool": true} | {"color": [r,g,b,a]} | {"vec3": [x,y,z]} | {"hlsl": "식"}]."""
    import json
    aliases = {"add": MAT_ADD, "fire": MAT_FIRE, "smoke": MAT_SMOKE, "ribbon": MAT_RIBBON,
               "flare4": MAT_FLARE4, "flare6": MAT_FLARE6, "ring": MAT_RING, "spark": MAT_SPARK}
    makers = {"float": nb.v_float, "int": nb.v_int, "bool": nb.v_bool, "hlsl": nb.v_hlsl,
              "color": lambda c: nb.v_color(*c), "vec3": lambda v: nb.v_vec3(*v), "vec2": lambda v: nb.v_vec2(*v)}
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    systems = {}
    for name, emitters in data["systems"].items():
        out = []
        for e in emitters:
            e = dict(e)
            if e.get("material") in aliases:
                e["material"] = aliases[e["material"]]
            raw = []
            for script, module, input_name, value in e.get("raw", []):
                (kind, v), = value.items()
                raw.append((script, module, input_name, makers[kind](v)))
            e["raw"] = raw
            for key, v in list(e.items()):
                if isinstance(v, list) and key != "raw":
                    e[key] = tuple(tuple(x) if isinstance(x, list) else x for x in v)
            out.append(e)
        systems[name] = out
    return data.get("folder", FX_DIR), systems


def main():
    nb.TRACE = os.environ.get("GRIM_FX_TRACE") == "1"
    only = [s.strip() for s in os.environ.get("GRIM_FX_SYSTEMS", "").split(",") if s.strip()]
    spec_path = os.environ.get("GRIM_FX_SPEC", "")
    folder, systems = load_spec(spec_path) if spec_path else (FX_DIR, SYSTEMS)
    # 공용 머티리얼 · 텍스처: 기본은 다시 만든다. 사양 파일이나 일부 시스템만 만들 때는 GRIM_FX_MATERIALS=1 일 때만.
    default_materials = "0" if (only or spec_path) else "1"
    if os.environ.get("GRIM_FX_MATERIALS", default_materials) == "1":
        fx_materials.make_all()
        nb.LOG.extend(fx_materials.LOG)
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.make_directory(folder)
    for name, emitters in systems.items():
        if only and name not in only:
            continue
        try:
            nb.build_system(folder, name, emitters)
        except RuntimeError as e:
            nb.log("!! %s" % e)

    out_dir = os.path.join(unreal.Paths.project_saved_dir(), "MagicFX")
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "make_fx_niagara.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(nb.LOG))


main()
