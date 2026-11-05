# 시작 마법 3개(불씨 = Bolt · 물줄기 = Stream · 미풍 = Cone)와 M2 스킬북 마법(화염구)의 DataAsset 을 만든다 / 고친다 (docs/systems/spell-feel · magic).
# 이 스크립트가 수치의 정본이다 — 에셋을 에디터에서 고쳤다면 여기에도 옮긴다.
# 연출 묶음(VisualSet)은 연출 갈래(MagicFX)가 채운다 — 이 스크립트는 건드리지 않는다(있으면 그대로 둔다).
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Magic\make_spells.py" -unattended -nop4 -nosplash -nullrhi

import os

import unreal

SPELL_DIR = "/Game/Grimoire/Magic/Spells"

# 수치는 spell-feel 「제안」값. 양초 규칙(바람 0.3~2.5 키움, 2.5 이상 꺼짐)에 맞춰 미풍은 거리별 세기:
# 1.5m 안 2.8(돌풍 — 끔) → 10m 0.6(키움). 시작점(완드 끝)에서 약 2.7m 안이면 2.5 를 넘는다.
SPELLS = [
    dict(
        asset="DA_Spell_Ember",
        name="플라미스",
        description="빠른 불꽃 화살을 쏜다. 양초 · 모닥불 · 마른 풀에 불을 붙인다.",
        spell_tag="Spell.Ember",
        element="Element.Fire",
        cooldown_tag="Cooldown.Spell.Ember",
        delivery=unreal.GrimSpellDelivery.BOLT,
        mana_cost=10.0,
        cooldown=0.6,
        cast_windup=0.1,
        radius=10.0,
        range=2500.0,
        damage=10.0,
        stimulus=1.0,
        color=unreal.LinearColor(0.88, 0.35, 0.17, 1.0),   # 불 #E0582B
        bolt_speed=7500.0,
        homing_strength=60.0,
    ),
    dict(
        asset="DA_Spell_WaterJet",
        name="아쿠엘",
        description="누르고 있는 동안 물줄기를 뿜는다. 불을 끄고 대상을 적신다.",
        spell_tag="Spell.WaterJet",
        element="Element.Water",
        cooldown_tag="Cooldown.Spell.WaterJet",
        delivery=unreal.GrimSpellDelivery.STREAM,
        channeled=True,
        mana_cost=2.0,
        channel_mana_per_second=10.0,
        cooldown=0.3,
        cast_windup=0.05,
        radius=15.0,
        range=1200.0,
        damage=4.0,
        stimulus=1.0,
        color=unreal.LinearColor(0.23, 0.48, 0.84, 1.0),   # 물 #3A7BD5
        stream_launch_speed=1400.0,
        stream_gravity_scale=1.0,
        stream_tick_interval=0.1,
        stream_segments=8,
        stream_splash_radius=40.0,
        stream_stimulus_interval=1.0,
    ),
    dict(
        asset="DA_Spell_Breeze",
        name="벤투스",
        description="바람을 원뿔로 쓸어 보낸다. 멀리서는 불꽃을 키우고, 바로 앞에서는 돌풍이 되어 끈다.",
        spell_tag="Spell.Breeze",
        element="Element.Wind",
        cooldown_tag="Cooldown.Spell.Breeze",
        delivery=unreal.GrimSpellDelivery.CONE,
        mana_cost=6.0,
        cooldown=0.8,
        cast_windup=0.1,
        radius=0.0,
        range=1000.0,
        damage=3.0,
        stimulus=1.0,
        color=unreal.LinearColor(0.49, 0.79, 0.65, 1.0),   # 바람 #7CC9A6
        cone_half_angle=30.0,
        cone_range=1000.0,
        cone_falloff=dict(near_distance=150.0, near_intensity=2.8, far_intensity=0.6, exponent=1.0),
    ),
    # M2 시험용 스킬북 마법(2서클) — 불씨를 Bolt 그대로 수치만 키웠다. 스킬북으로만 배운다(StartingSpells 에 없음).
    dict(
        asset="DA_Spell_Fireball",
        name="이그니스",
        description="화염 구체를 쏜다. 플라미스보다 느리지만 더 세게 태운다. 2서클 스킬북으로 배운다.",
        spell_tag="Spell.Fireball",
        element="Element.Fire",
        cooldown_tag="Cooldown.Spell.Fireball",
        delivery=unreal.GrimSpellDelivery.BOLT,
        circle=2,
        mana_cost=25.0,
        cooldown=2.0,
        cast_windup=0.15,
        radius=25.0,
        range=3000.0,
        damage=30.0,
        stimulus=2.0,
        color=unreal.LinearColor(1.0, 0.45, 0.1, 1.0),
        bolt_speed=5000.0,
        homing_strength=40.0,
    ),
]

# 일부만 다시 만들 때: 환경 변수 GRIM_SPELLS=DA_Spell_Fireball(쉼표로 여럿). 비우면 전부.
ONLY = [name.strip() for name in os.environ.get("GRIM_SPELLS", "").split(",") if name.strip()]

# 정의에 없는 키의 기본값(전달 방식마다 쓰지 않는 필드도 깨끗이 맞춘다)
DEFAULTS = dict(
    circle=1,
    channeled=False,
    channel_mana_per_second=0.0,
    cast_windup=0.0,
    bolt_speed=0.0,
    homing_strength=0.0,
    stream_launch_speed=0.0,
    stream_gravity_scale=1.0,
    stream_tick_interval=0.1,
    stream_segments=8,
    stream_splash_radius=0.0,
    stream_stimulus_interval=1.0,
    cone_half_angle=30.0,
    cone_range=0.0,
    cone_falloff=dict(near_distance=0.0, near_intensity=1.0, far_intensity=1.0, exponent=1.0),
)


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
        if ONLY and data["asset"] not in ONLY:
            continue
        spell = make_or_load(data["asset"])
        if spell is None:
            raise RuntimeError("에셋을 만들지 못함: " + data["asset"])

        values = dict(DEFAULTS)
        values.update(data)

        stimulus = unreal.GrimElementalStimulus()
        stimulus.set_editor_property("element", tag(values["element"]))
        stimulus.set_editor_property("intensity", values["stimulus"])
        stimulus.set_editor_property("source", unreal.GrimStimulusSource.SPELL)

        falloff = unreal.GrimConeFalloff()
        for key, value in values["cone_falloff"].items():
            falloff.set_editor_property(key, value)

        spell.set_editor_property("display_name", unreal.Text(values["name"]))
        spell.set_editor_property("description", unreal.Text(values["description"]))
        spell.set_editor_property("spell_tag", tag(values["spell_tag"]))
        spell.set_editor_property("element", tag(values["element"]))
        spell.set_editor_property("circle", values["circle"])
        spell.set_editor_property("mana_cost", values["mana_cost"])
        spell.set_editor_property("channel_mana_per_second", values["channel_mana_per_second"])
        spell.set_editor_property("cooldown", values["cooldown"])
        spell.set_editor_property("cooldown_tag", tag(values["cooldown_tag"]))
        spell.set_editor_property("cast_windup", values["cast_windup"])
        spell.set_editor_property("channeled", values["channeled"])
        spell.set_editor_property("delivery", values["delivery"])
        spell.set_editor_property("radius", values["radius"])
        spell.set_editor_property("range", values["range"])
        spell.set_editor_property("bolt_speed", values["bolt_speed"])
        spell.set_editor_property("homing_strength", values["homing_strength"])
        spell.set_editor_property("stream_launch_speed", values["stream_launch_speed"])
        spell.set_editor_property("stream_gravity_scale", values["stream_gravity_scale"])
        spell.set_editor_property("stream_tick_interval", values["stream_tick_interval"])
        spell.set_editor_property("stream_segments", values["stream_segments"])
        spell.set_editor_property("stream_splash_radius", values["stream_splash_radius"])
        spell.set_editor_property("stream_stimulus_interval", values["stream_stimulus_interval"])
        spell.set_editor_property("cone_half_angle", values["cone_half_angle"])
        spell.set_editor_property("cone_range", values["cone_range"])
        spell.set_editor_property("cone_falloff", falloff)
        spell.set_editor_property("damage", values["damage"])
        spell.set_editor_property("stimulus", stimulus)
        spell.set_editor_property("color", values["color"])

        if not unreal.EditorAssetLibrary.save_loaded_asset(spell, only_if_is_dirty=False):
            raise RuntimeError("저장 실패: " + values["asset"])
        unreal.log("[make_spells] " + values["asset"] + " 저장 (" + str(values["delivery"]) + ")")


main()
