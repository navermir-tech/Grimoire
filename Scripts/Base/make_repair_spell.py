# 「수리」 마법(무속성 1서클, R18 · R28)의 DataAsset DA_Spell_Repair 를 만든다 / 고친다 (docs/systems/m4-plan 「데이터」).
# 볼트가 맞힌 것이 지은 것이면 가방의 재료로 고치고, 작업대면 시전자의 장비를, 거점 핵이면 거점 수리를 한다 — AGrimRepairProjectile.
# 스킬북 아이템 Skillbook_Repair(Items.csv)가 이 마법을 가르친다. 연출 묶음(VisualSet)은 연출 갈래 몫 — 있으면 그대로 둔다.
#
# 실행(이 프로젝트의 에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Base\make_repair_spell.py" -unattended -nop4 -nosplash -nullrhi

import unreal

SPELL_DIR = "/Game/Grimoire/Magic/Spells"
ASSET = "DA_Spell_Repair"

VALUES = dict(
    name="수리",
    description="무속성 1서클. 재료(나무 · 돌 · 털 …)를 가방에 들고 닳은 조각 · 시설에 걸면 재료 1개당 최대 내구도의 25%를 고친다. "
                "작업대에 걸면 입은 장비를, 거점 핵에 걸면 저장고의 재료로 거점 전체를 고친다(마나 더 듦).",
    circle=1,
    mana_cost=15.0,
    cooldown=1.5,
    cast_windup=0.12,
    radius=25.0,
    range=2000.0,
    bolt_speed=3500.0,
    homing_strength=120.0,
    color=unreal.LinearColor(0.75, 0.72, 0.6, 1.0),
)


def tag(name):
    result = unreal.GrimSpellDefinition.request_tag_by_name(name)
    if name not in result.export_text():
        raise RuntimeError("GameplayTag 없음: " + name)
    return result


def make_or_load():
    path = SPELL_DIR + "/" + ASSET
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.GrimSpellDefinition)
    return tools.create_asset(ASSET, SPELL_DIR, unreal.GrimSpellDefinition, factory)


def main():
    unreal.EditorAssetLibrary.make_directory(SPELL_DIR)
    spell = make_or_load()
    if spell is None:
        raise RuntimeError("에셋을 만들지 못함: " + ASSET)

    element = tag("Element.None")
    stimulus = unreal.GrimElementalStimulus()
    stimulus.set_editor_property("element", element)
    stimulus.set_editor_property("intensity", 0.0)
    stimulus.set_editor_property("source", unreal.GrimStimulusSource.SPELL)

    spell.set_editor_property("display_name", unreal.Text(VALUES["name"]))
    spell.set_editor_property("description", unreal.Text(VALUES["description"]))
    spell.set_editor_property("spell_tag", tag("Spell.Repair"))
    spell.set_editor_property("element", element)
    spell.set_editor_property("circle", VALUES["circle"])
    spell.set_editor_property("mana_cost", VALUES["mana_cost"])
    spell.set_editor_property("channel_mana_per_second", 0.0)
    spell.set_editor_property("cooldown", VALUES["cooldown"])
    spell.set_editor_property("cooldown_tag", tag("Cooldown.Spell.Repair"))
    spell.set_editor_property("cast_windup", VALUES["cast_windup"])
    spell.set_editor_property("channeled", False)
    spell.set_editor_property("delivery", unreal.GrimSpellDelivery.BOLT)
    spell.set_editor_property("radius", VALUES["radius"])
    spell.set_editor_property("range", VALUES["range"])
    spell.set_editor_property("projectile_class", unreal.GrimRepairProjectile.static_class())
    spell.set_editor_property("bolt_speed", VALUES["bolt_speed"])
    spell.set_editor_property("homing_strength", VALUES["homing_strength"])
    spell.set_editor_property("damage", 0.0)
    spell.set_editor_property("stimulus", stimulus)
    spell.set_editor_property("color", VALUES["color"])
    spell.set_editor_property("tracks_proficiency", True)

    if not unreal.EditorAssetLibrary.save_loaded_asset(spell, only_if_is_dirty=False):
        raise RuntimeError("저장 실패: " + ASSET)
    unreal.log("[make_repair_spell] " + ASSET + " 저장")


main()
