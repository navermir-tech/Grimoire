# 기본 공격(R26, M2 갈래 K)의 마법 정의 DA_Spell_BasicAttack 을 만든다 / 고친다. 값은 이 스크립트가 정본이다.
# 무속성(Element.None) · Bolt · 마나 0 · 쿨다운 0 · 숙련도 없음(bTracksProficiency = False — 그리모어 맨 앞, 슬롯에 안 들어감).
# 연타 리듬 · 강타 · 주먹질 수치는 DefaultGame.ini [/Script/Grimoire.GrimBasicAttackSettings].
# 연출 묶음 VS_BasicAttack 은 Scripts/MagicFX/make_magicfx_assets.py 가 먼저 만들어야 한다(GRIM_FX_STEPS=visual_sets 로 그것만).
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   $env:GRIM_FX_STEPS = "visual_sets"
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\MagicFX\make_magicfx_assets.py" -unattended -nop4 -nosplash -nullrhi
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Magic\make_basic_attack.py" -unattended -nop4 -nosplash -nullrhi

import unreal

SPELL_DIR = "/Game/Grimoire/Magic/Spells"
ASSET = "DA_Spell_BasicAttack"
VISUAL_SET = "/Game/Grimoire/MagicFX/VisualSets/VS_BasicAttack"

VALUES = dict(
    display_name="기본 공격",
    description="완드를 휘둘러 순수 마력을 쏜다. 마나도 쿨타임도 없다 — 이어 치면 네 번째가 세게 밀어낸다. 사물에는 충격만 준다.",
    circle=1,
    mana_cost=0.0,
    channel_mana_per_second=0.0,
    cooldown=0.0,
    cast_windup=0.0,           # 연타 리듬(0.3초)을 해치지 않게 바로 나간다
    channeled=False,
    radius=8.0,
    range=2500.0,
    bolt_speed=9000.0,         # 가장 빠른 섬광
    homing_strength=60.0,      # 락온 대상 쪽으로 살짝 휨(마법과 같다)
    damage=5.0,                # 작다(강타는 × HeavyDamageMultiplier)
    stimulus=1.0,
    color=unreal.LinearColor(0.63, 0.77, 1.0, 1.0),
    tracks_proficiency=False,
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

    stimulus = unreal.GrimElementalStimulus()
    stimulus.set_editor_property("element", tag("Element.None"))   # 반응 규칙표에 무속성 행이 없다 → 속성 상태는 그대로(충격만)
    stimulus.set_editor_property("intensity", VALUES["stimulus"])
    stimulus.set_editor_property("source", unreal.GrimStimulusSource.SPELL)

    spell.set_editor_property("display_name", unreal.Text(VALUES["display_name"]))
    spell.set_editor_property("description", unreal.Text(VALUES["description"]))
    spell.set_editor_property("spell_tag", tag("Spell.BasicAttack"))
    spell.set_editor_property("element", tag("Element.None"))
    spell.set_editor_property("circle", VALUES["circle"])
    spell.set_editor_property("ability_class", unreal.GrimBasicAttackAbility.static_class())
    spell.set_editor_property("mana_cost", VALUES["mana_cost"])
    spell.set_editor_property("channel_mana_per_second", VALUES["channel_mana_per_second"])
    spell.set_editor_property("cooldown", VALUES["cooldown"])
    spell.set_editor_property("cooldown_tag", unreal.GameplayTag())
    spell.set_editor_property("cast_windup", VALUES["cast_windup"])
    spell.set_editor_property("channeled", VALUES["channeled"])
    spell.set_editor_property("delivery", unreal.GrimSpellDelivery.BOLT)
    spell.set_editor_property("radius", VALUES["radius"])
    spell.set_editor_property("range", VALUES["range"])
    spell.set_editor_property("projectile_class", unreal.GrimBasicAttackProjectile.static_class())
    spell.set_editor_property("bolt_speed", VALUES["bolt_speed"])
    spell.set_editor_property("homing_strength", VALUES["homing_strength"])
    spell.set_editor_property("damage", VALUES["damage"])
    spell.set_editor_property("stimulus", stimulus)
    spell.set_editor_property("color", VALUES["color"])
    spell.set_editor_property("tracks_proficiency", VALUES["tracks_proficiency"])

    visual = unreal.EditorAssetLibrary.load_asset(VISUAL_SET)
    if visual is None:
        unreal.log_warning("[make_basic_attack] 연출 묶음 없음(먼저 make_magicfx_assets.py): " + VISUAL_SET)
    else:
        spell.set_editor_property("visual_set", visual)

    if not unreal.EditorAssetLibrary.save_loaded_asset(spell, only_if_is_dirty=False):
        raise RuntimeError("저장 실패: " + ASSET)
    unreal.log("[make_basic_attack] %s 저장 — 연출 %s, 어빌리티 %s" % (ASSET, visual, spell.get_editor_property("ability_class")))


main()
