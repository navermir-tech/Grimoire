# 계약 마법(길들이기) 파크툼 · 파크툼 + 속성의 DataAsset 을 만든다 / 고친다 (docs/systems/creatures 「길들이기」, M3 갈래 T).
# 어떤 파크툼이 있는지(이름 · 설명 · 속성 · 서클 · 색 · 만들지 여부)는 Content/Grimoire/Data/PactumSpells.csv 가 정본이고,
# 전달 방식 · 비용 같은 공통 수치는 아래 COMMON 이 정본이다. bImplemented=False 행(글라키에스 · 플로라 · 상투스 · 룩스 · 말레디카)은 표에만 있다.
# 연출 묶음(VisualSet)은 연출 갈래가 채운다 — 이 스크립트는 건드리지 않는다(있으면 그대로 둔다).
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Magic\make_pactum_spells.py" -unattended -nop4 -nosplash -nullrhi

import csv
import os

import unreal

SPELL_DIR = "/Game/Grimoire/Magic/Spells"
TABLE = os.path.join(unreal.Paths.project_content_dir(), "Grimoire", "Data", "PactumSpells.csv")

# 모든 파크툼이 같이 쓰는 값: 피해 없는 빠른 볼트(유도 240°/s · 반지름 25 — 도망치는 약한 생물에도 맞도록 공격 볼트보다 훨씬 세게 휜다). 자극 세기 0(양초 · 풀에 아무 일 없음).
COMMON = dict(
    delivery=unreal.GrimSpellDelivery.BOLT,
    mana_cost=12.0,
    cooldown=2.5,
    cast_windup=0.12,
    radius=25.0,
    range=2200.0,
    damage=0.0,
    stimulus=0.0,
    bolt_speed=4500.0,
    homing_strength=240.0,
    # 숙련도(Q26) 추가 효과: S 등급에서 포획 확률 +10%p (등급 배율 ExtraEffectScale 이 곱해진다)
    extra_effect_tag="Taming.CaptureBonus",
    extra_effect_max=0.1,
)

ONLY = [name.strip() for name in os.environ.get("GRIM_SPELLS", "").split(",") if name.strip()]


def tag(name):
    result = unreal.GrimSpellDefinition.request_tag_by_name(name)
    if name not in result.export_text():
        raise RuntimeError("GameplayTag 없음: " + name)
    return result


def parse_color(text):
    # "(R=0.85,G=0.78,B=0.55,A=1.0)"
    values = {}
    for part in text.strip("()").split(","):
        key, value = part.split("=")
        values[key.strip()] = float(value)
    return unreal.LinearColor(values["R"], values["G"], values["B"], values.get("A", 1.0))


def make_or_load(asset_name):
    path = SPELL_DIR + "/" + asset_name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.GrimSpellDefinition)
    return tools.create_asset(asset_name, SPELL_DIR, unreal.GrimSpellDefinition, factory)


def main():
    unreal.EditorAssetLibrary.make_directory(SPELL_DIR)
    with open(TABLE, encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    made = 0
    for row in rows:
        spell_id = row["Name"]
        asset = "DA_Spell_" + spell_id
        if row["bImplemented"].strip().lower() != "true":
            unreal.log("[make_pactum_spells] " + spell_id + " — 표에만(만들지 않음)")
            continue
        if ONLY and asset not in ONLY:
            continue
        suffix = spell_id[len("Pactum"):].lstrip("_")
        spell_tag = "Spell.Pactum" + ("." + suffix if suffix else "")
        cooldown_tag = "Cooldown.Spell.Pactum" + ("." + suffix if suffix else "")

        spell = make_or_load(asset)
        if spell is None:
            raise RuntimeError("에셋을 만들지 못함: " + asset)

        element = tag(row["Element"])
        stimulus = unreal.GrimElementalStimulus()
        stimulus.set_editor_property("element", element)
        stimulus.set_editor_property("intensity", COMMON["stimulus"])
        stimulus.set_editor_property("source", unreal.GrimStimulusSource.SPELL)

        extra = unreal.GrimProficiencyExtraEffect()
        extra.set_editor_property("effect_tag", tag(COMMON["extra_effect_tag"]))
        extra.set_editor_property("max_value", COMMON["extra_effect_max"])

        spell.set_editor_property("display_name", unreal.Text(row["DisplayName"]))
        spell.set_editor_property("description", unreal.Text(row["Description"]))
        spell.set_editor_property("spell_tag", tag(spell_tag))
        spell.set_editor_property("element", element)
        spell.set_editor_property("circle", int(row["Circle"]))
        spell.set_editor_property("mana_cost", COMMON["mana_cost"])
        spell.set_editor_property("channel_mana_per_second", 0.0)
        spell.set_editor_property("cooldown", COMMON["cooldown"])
        spell.set_editor_property("cooldown_tag", tag(cooldown_tag))
        spell.set_editor_property("cast_windup", COMMON["cast_windup"])
        spell.set_editor_property("channeled", False)
        spell.set_editor_property("delivery", COMMON["delivery"])
        spell.set_editor_property("radius", COMMON["radius"])
        spell.set_editor_property("range", COMMON["range"])
        spell.set_editor_property("projectile_class", unreal.GrimPactumProjectile.static_class())
        spell.set_editor_property("bolt_speed", COMMON["bolt_speed"])
        spell.set_editor_property("homing_strength", COMMON["homing_strength"])
        spell.set_editor_property("damage", COMMON["damage"])
        spell.set_editor_property("stimulus", stimulus)
        spell.set_editor_property("color", parse_color(row["Color"]))
        spell.set_editor_property("tracks_proficiency", True)
        spell.set_editor_property("proficiency_extra_effect", extra)

        if not unreal.EditorAssetLibrary.save_loaded_asset(spell, only_if_is_dirty=False):
            raise RuntimeError("저장 실패: " + asset)
        made += 1
        unreal.log("[make_pactum_spells] " + asset + " 저장 (" + row["Element"] + ", " + row["Circle"] + "서클)")
    unreal.log("[make_pactum_spells] 끝 — " + str(made) + "개")


main()
