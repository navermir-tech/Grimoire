# M1 시작 마법 3개(불씨 · 물줄기 · 미풍)의 DataAsset 을 만든다 / 고친다.
# 이 스크립트가 수치의 정본이다 — 에셋을 에디터에서 고쳤다면 여기에도 옮긴다.
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Magic\make_spells.py" -unattended -nop4 -nosplash -nullrhi

import unreal

SPELL_DIR = "/Game/Grimoire/Magic/Spells"

# 수치는 스펙 M1 「제안」값. 양초 규칙(바람 0.3 이상 키움, 2.5 이상 꺼짐)에 맞춰 미풍 세기는 1.0.
SPELLS = [
    dict(
        asset="DA_Spell_Ember",
        name="불씨",
        description="작은 불씨를 날린다. 양초 · 모닥불 · 마른 풀에 불을 붙인다.",
        spell_tag="Spell.Ember",
        element="Element.Fire",
        cooldown_tag="Cooldown.Spell.Ember",
        delivery=unreal.GrimSpellDelivery.PROJECTILE,
        mana_cost=10.0,
        cooldown=1.0,
        projectile_speed=1800.0,
        radius=12.0,
        range=2500.0,
        damage=10.0,
        stimulus=1.0,
        color=unreal.LinearColor(1.0, 0.35, 0.05, 1.0),
    ),
    dict(
        asset="DA_Spell_WaterJet",
        name="물줄기",
        description="물방울을 빠르게 쏜다. 불을 끄고 대상을 적신다.",
        spell_tag="Spell.WaterJet",
        element="Element.Water",
        cooldown_tag="Cooldown.Spell.WaterJet",
        delivery=unreal.GrimSpellDelivery.PROJECTILE,
        mana_cost=8.0,
        cooldown=0.6,
        projectile_speed=2400.0,
        radius=15.0,
        range=1800.0,
        damage=6.0,
        stimulus=1.0,
        color=unreal.LinearColor(0.1, 0.45, 1.0, 1.0),
    ),
    dict(
        asset="DA_Spell_Breeze",
        name="미풍",
        description="부드러운 바람을 앞으로 보낸다. 불꽃을 키우고 가벼운 것을 민다.",
        spell_tag="Spell.Breeze",
        element="Element.Wind",
        cooldown_tag="Cooldown.Spell.Breeze",
        delivery=unreal.GrimSpellDelivery.BEAM,
        mana_cost=6.0,
        cooldown=0.8,
        projectile_speed=0.0,
        radius=40.0,
        range=1200.0,
        damage=3.0,
        stimulus=1.0,
        color=unreal.LinearColor(0.75, 1.0, 0.85, 1.0),
    ),
]


def tag(name):
    result = unreal.GrimSpellDefinition.request_tag_by_name(name)
    # 없는 태그면 빈 태그가 온다 — 텍스트로 내보내 이름이 들어 있는지 본다.
    if name not in result.export_text():
        raise RuntimeError("GameplayTag 없음: " + name)
    return result


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
    for data in SPELLS:
        spell = make_or_load(data["asset"])
        if spell is None:
            raise RuntimeError("에셋을 만들지 못함: " + data["asset"])

        stimulus = unreal.GrimElementalStimulus()
        stimulus.set_editor_property("element", tag(data["element"]))
        stimulus.set_editor_property("intensity", data["stimulus"])
        stimulus.set_editor_property("source", unreal.GrimStimulusSource.SPELL)

        spell.set_editor_property("display_name", unreal.Text(data["name"]))
        spell.set_editor_property("description", unreal.Text(data["description"]))
        spell.set_editor_property("spell_tag", tag(data["spell_tag"]))
        spell.set_editor_property("element", tag(data["element"]))
        spell.set_editor_property("circle", 1)
        spell.set_editor_property("mana_cost", data["mana_cost"])
        spell.set_editor_property("cooldown", data["cooldown"])
        spell.set_editor_property("cooldown_tag", tag(data["cooldown_tag"]))
        spell.set_editor_property("cast_time", 0.0)
        spell.set_editor_property("delivery", data["delivery"])
        spell.set_editor_property("projectile_speed", data["projectile_speed"])
        spell.set_editor_property("radius", data["radius"])
        spell.set_editor_property("range", data["range"])
        spell.set_editor_property("damage", data["damage"])
        spell.set_editor_property("stimulus", stimulus)
        spell.set_editor_property("color", data["color"])

        if not unreal.EditorAssetLibrary.save_loaded_asset(spell, only_if_is_dirty=False):
            raise RuntimeError("저장 실패: " + data["asset"])
        unreal.log("[make_spells] " + data["asset"] + " 저장")


main()
