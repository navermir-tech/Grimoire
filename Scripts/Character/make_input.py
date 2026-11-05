# 이동 외 조작 입력 에셋을 만든다(M2 갈래 K — 호그와트 레거시 배치, docs/spec/references 「조작」): IMC_Spell 과 그 액션들.
#   기본 공격 = 좌클릭 · 패드 R2/RT 짧게(Tap) / 슬롯 1~4 바로 시전 = 숫자키 1~4 · 패드 R2/RT 누른 채 Y · B · A · X
#   세트 고르기 = 패드 R2/RT 누른 채 십자키(PC F1~F4 는 UI 갈래의 IMC_UI) · 다음/이전 세트 = 마우스 휠(보조)
#   방어(Protego 자리) = Q · 패드 Y, 회피 = Ctrl · 패드 B — 지금은 입력 자리만
# 이동 · 시점 · 점프는 템플릿에서 가져온 Content/Input 을 그대로 쓴다(점프 = Space · 패드 A — 호그와트와 같다).
# 조준 입력(IA_LockOn · IA_AimZoom · IMC_Targeting)은 Scripts/Targeting/make_targeting_input.py 가 맡는다 — 여기서 건드리지 않는다.
# 다시 돌려도 된다(있으면 매핑 · 트리거만 새로 채운다). 옛 액션(IA_SpellCast · IA_SpellNext · IA_SpellPrev)은 지운다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Character\make_input.py" -unattended -nop4 -nosplash -nullrhi
import unreal

FOLDER = "/Game/Grimoire/Input"

# 패드 탭 판정: 이 시간 안에 떼면 기본 공격(더 길면 세트 활성으로 본다).
PAD_TAP_SECONDS = 0.2
# 패드 슬롯 칸 순서(칸 1~4). 호그와트 HUD 의 칸 · 버튼 짝은 공략에서 확인하지 못했다(불확실) — 위에서 시계 방향으로 둔다.
PAD_SLOT_KEYS = ["Gamepad_FaceButton_Top", "Gamepad_FaceButton_Right", "Gamepad_FaceButton_Bottom", "Gamepad_FaceButton_Left"]
# 패드 세트 1~4 = R2/RT + 십자키(위 · 오른쪽 · 아래 · 왼쪽, 순서 불확실)
PAD_SET_KEYS = ["Gamepad_DPad_Up", "Gamepad_DPad_Right", "Gamepad_DPad_Down", "Gamepad_DPad_Left"]

HOLD = "IA_SpellSetHold"

# 액션 이름 → (키 목록, 액션 트리거). 키 이름은 EKeys(FKey) 이름. 트리거: ("tap", 초) · ("chord", 액션 이름).
# 트리거가 없으면 Down — 누르면 Started, 떼면 Completed(물줄기처럼 누르고 있는 마법이 뗌을 받는다).
ACTIONS = {
    "IA_BasicAttack": (["LeftMouseButton"], []),
    "IA_BasicAttackPad": (["Gamepad_RightTrigger"], [("tap", PAD_TAP_SECONDS)]),
    HOLD: (["Gamepad_RightTrigger"], []),
    "IA_SpellSlot1": (["One"], []),
    "IA_SpellSlot2": (["Two"], []),
    "IA_SpellSlot3": (["Three"], []),
    "IA_SpellSlot4": (["Four"], []),
    "IA_SpellSetNext": (["MouseScrollUp"], []),
    "IA_SpellSetPrev": (["MouseScrollDown"], []),
    "IA_Protego": (["Q", "Gamepad_FaceButton_Top"], []),
    "IA_Dodge": (["LeftControl", "Gamepad_FaceButton_Right"], []),
}
for i, key in enumerate(PAD_SLOT_KEYS):
    ACTIONS[f"IA_SpellSlotPad{i + 1}"] = ([key], [("chord", HOLD)])
for i, key in enumerate(PAD_SET_KEYS):
    ACTIONS[f"IA_SpellSetPad{i + 1}"] = ([key], [("chord", HOLD)])

OBSOLETE = ["IA_SpellCast", "IA_SpellNext", "IA_SpellPrev"]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def load_or_create(name, asset_class, factory):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    return asset_tools.create_asset(name, FOLDER, asset_class, factory)


def make_key(name):
    key = unreal.Key()
    key.set_editor_property("key_name", name)
    return key


actions = {}
for name in ACTIONS:
    action = load_or_create(name, unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    actions[name] = action

# 트리거는 액션에 둔다(키마다 따로 두지 않는다) — 패드 슬롯 · 세트는 IA_SpellSetHold 를 Chord 로 건다.
for name, (_, trigger_specs) in ACTIONS.items():
    action = actions[name]
    triggers = []
    for spec in trigger_specs:
        if spec[0] == "pressed":
            triggers.append(unreal.new_object(unreal.InputTriggerPressed, outer=action))
        elif spec[0] == "tap":
            tap = unreal.new_object(unreal.InputTriggerTap, outer=action)
            tap.set_editor_property("tap_release_time_threshold", spec[1])
            triggers.append(tap)
        elif spec[0] == "chord":
            chord = unreal.new_object(unreal.InputTriggerChordAction, outer=action)
            chord.set_editor_property("chord_action", actions[spec[1]])
            triggers.append(chord)
    action.set_editor_property("triggers", triggers)
    eal.save_loaded_asset(action, only_if_is_dirty=False)

imc = load_or_create("IMC_Spell", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()
for name, (keys, _) in ACTIONS.items():
    for key_name in keys:
        imc.map_key(actions[name], make_key(key_name))
eal.save_loaded_asset(imc, only_if_is_dirty=False)

for name in OBSOLETE:
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
        unreal.log(f"make_input: 지움 {path}")

mappings = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
unreal.log(f"make_input: {len(actions)} actions, IMC_Spell mappings = {len(mappings)}")
for m in mappings:
    action = m.get_editor_property("action")
    trig = [t.get_class().get_name() for t in action.get_editor_property("triggers")]
    unreal.log(f"make_input: {action.get_name()} <- {m.get_editor_property('key').get_editor_property('key_name')} {trig}")
