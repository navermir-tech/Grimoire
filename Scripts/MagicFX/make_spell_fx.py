# 마법 이펙트를 볼트 빛 언어로 사양 파일에서 만든다(2026-11-07, Q85 · Q86) — 값은 사양 파일(art/fx/specs/spell/*.json)이 정본.
#  - NS_Bolt_Launch(볼트 발사 빛줄기) · NS_Stream_Trail/Impact(물줄기) · NS_Cone_Trail/Impact(미풍)
#  - Effect Type EFT_Grim_Spell 을 만들고(없으면) 만든 시스템과 /Game/Grimoire/MagicFX 아래 모든 Niagara 시스템에 붙인다(성능 규칙 24).
#  머티리얼 · 텍스처는 make_bolt_fx.py 가 만든 MI_Bolt_* 를 그대로 쓴다 — 그것부터 돌린다. 연출 묶음 연결은 make_magicfx_assets.py.
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\MagicFX\make_spell_fx.py" -unattended -nop4 -nosplash -nullrhi
# 일부만: GRIM_FX_SPEC=stream (쉼표 여럿, 파일 이름) · GRIM_FX_SYSTEMS=NS_Stream_Trail. 결과는 Saved/MagicFX/make_spell_fx.txt —
# 시스템마다 「설정 실패=0」 · 「입자 어림」, 끝에 Effect Type 붙인 수.

import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import unreal  # noqa: E402
import spell_fx_spec as fx  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SPEC_DIR = os.path.join(ROOT, "art", "fx", "specs", "spell")
BOLT_MI = "/Game/Grimoire/MagicFX/Bolt/Materials/MI_Bolt_"
EFT_PATH = "/Game/Grimoire/MagicFX/EFT_Grim_Spell"
EAL = unreal.EditorAssetLibrary
log = fx.log

# 마법 Effect Type 값(성능 규칙 24). 마법 하나는 짧게 터지는 것이 대부분 — 멀면 그리지 않고, 같은 종류가 너무 많으면 먼 것부터 끈다.
EFT_VALUES = {
    "update_frequency": "LOW",           # 거리 · 개수 확인 주기
    "cull_reaction": "DEACTIVATE_IMMEDIATE",
    "max_distance": 8000.0,              # 80 m 밖의 마법 연출은 그리지 않는다(볼트 사거리 30 m)
    "max_instances": 160,                # 이 Effect Type 전체(모든 마법 이펙트 동시에)
    "max_system_instances": 48,          # 시스템 하나(예: NS_Bolt_Impact) 동시에
}


def set_prop(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as e:  # noqa: BLE001 — 실험적인 이름은 로그만 남긴다
        fx.Failures.count += 1
        log("!! %s.%s: %s" % (obj.__class__.__name__, name, e))
        return False


def ensure_effect_type():
    if EAL.does_asset_exist(EFT_PATH):
        eft = EAL.load_asset(EFT_PATH)
    else:
        folder, name = EFT_PATH.rsplit("/", 1)
        eft = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.NiagaraEffectType, unreal.NiagaraEffectTypeFactoryNew())
    set_prop(eft, "update_frequency", getattr(unreal.NiagaraScalabilityUpdateFrequency, EFT_VALUES["update_frequency"]))
    set_prop(eft, "cull_reaction", getattr(unreal.NiagaraCullReaction, EFT_VALUES["cull_reaction"]))
    # 중요도 = 거리(가까운 것부터 남김). 처리기 클래스가 파이썬에 없어 C++ 도우미로.
    if not unreal.GrimMagicFXEditorLibrary.set_distance_significance(eft):
        fx.Failures.count += 1
        log("!! 중요도 처리기를 못 붙임")

    arr = eft.get_editor_property("system_scalability_settings")
    settings = list(arr.get_editor_property("settings"))
    if not settings:
        settings = [unreal.NiagaraSystemScalabilitySettings()]
    for s in settings:
        set_prop(s, "cull_by_distance", True)
        set_prop(s, "max_distance", EFT_VALUES["max_distance"])
        set_prop(s, "cull_max_instance_count", True)
        set_prop(s, "max_instances", EFT_VALUES["max_instances"])
        set_prop(s, "cull_per_system_max_instance_count", True)
        set_prop(s, "max_system_instances", EFT_VALUES["max_system_instances"])
    arr.set_editor_property("settings", settings)
    set_prop(eft, "system_scalability_settings", arr)
    EAL.save_loaded_asset(eft)
    s0 = eft.get_editor_property("system_scalability_settings").get_editor_property("settings")[0]
    log("Effect Type %s 중요도=%s 거리 컬=%s %.0fcm, 전체 %d · 시스템당 %d" % (
        EFT_PATH, eft.get_editor_property("significance_handler"), s0.get_editor_property("cull_by_distance"), s0.get_editor_property("max_distance"),
        s0.get_editor_property("max_instances"), s0.get_editor_property("max_system_instances")))
    return eft


def attach_effect_type_everywhere(eft, skip):
    """/Game/Grimoire/MagicFX 아래 Niagara 시스템 모두에 붙인다(이번에 만든 것은 이미 붙음)."""
    count = 0
    for path in EAL.list_assets("/Game/Grimoire/MagicFX", recursive=True, include_folder=False):
        obj_path = path.split(".")[0]
        if obj_path.rsplit("/", 1)[-1] in skip:
            continue
        data = unreal.EditorAssetLibrary.find_asset_data(path)
        if str(data.asset_class_path.asset_name) != "NiagaraSystem":
            continue
        system = EAL.load_asset(obj_path)
        if system.get_editor_property("effect_type") != eft:
            set_prop(system, "effect_type", eft)
            unreal.GrimMagicFXEditorLibrary.compile_niagara_system_and_wait(system, True)
            EAL.save_loaded_asset(system)
        count += 1
        log("Effect Type 붙음 %s" % obj_path)
    return count


def main():
    only_specs = [x for x in os.environ.get("GRIM_FX_SPEC", "").split(",") if x]
    only_systems = [x for x in os.environ.get("GRIM_FX_SYSTEMS", "").split(",") if x]
    eft = ensure_effect_type()
    mi = {}
    for key in ("Streak", "Wisp", "Spark", "Ring", "Flare", "Burst"):
        mi[key] = EAL.load_asset(BOLT_MI + key)
        if mi[key] is None:
            raise RuntimeError("머티리얼 인스턴스 없음 %s%s — make_bolt_fx.py 를 먼저 돌린다" % (BOLT_MI, key))

    built = []
    for spec_path in sorted(glob.glob(os.path.join(SPEC_DIR, "*.json"))):
        stem = os.path.splitext(os.path.basename(spec_path))[0]
        if only_specs and stem not in only_specs:
            continue
        with open(spec_path, encoding="utf-8") as f:
            spec = json.load(f)
        folder = spec["folder"]
        if not EAL.does_directory_exist(folder):
            EAL.make_directory(folder)
        for name, system in spec["systems"].items():
            if only_systems and name not in only_systems:
                continue
            try:
                fx.build_system(folder, name, system, mi, eft)
                built.append(name)
            except Exception as e:  # noqa: BLE001 — 하나가 실패해도 나머지는 만든다
                import traceback
                fx.Failures.count += 1
                log("!! %s 실패: %s\n%s" % (name, e, traceback.format_exc()))

    attached = attach_effect_type_everywhere(eft, set(built))
    log("끝: 만든 시스템 %d개 %s, 그 밖에 Effect Type 붙인 시스템 %d개, 설정 실패 합계 %d" % (len(built), built, attached, fx.Failures.count))

    out_dir = os.path.join(unreal.Paths.project_saved_dir(), "MagicFX")
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "make_spell_fx.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(fx.LOG))


main()
