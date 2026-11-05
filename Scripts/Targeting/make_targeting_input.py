# 조준 입력 에셋을 만든다: IA_LockOn(고정 락온 토글) · IA_AimZoom(줌인 조준 자리), IMC_Targeting.
# 기존 IMC_Spell · Content/Input 은 건드리지 않는다 — AGrimPlayerController::DefaultMappingContexts 에 IMC_Targeting 을 더한다.
# 다시 돌려도 된다(있으면 매핑만 새로 채운다).
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Targeting\make_targeting_input.py" -unattended -nop4 -nosplash -nullrhi
import unreal

FOLDER = "/Game/Grimoire/Input"

# 액션 이름 → 키 목록. 키 이름은 EKeys(FKey) 이름.
TARGETING_ACTIONS = {
    # Q17: 호그와트 레거시처럼 PC Caps Lock · 패드 R3(오른쪽 스틱 누르기) 토글
    "IA_LockOn": ["CapsLock", "Gamepad_RightThumbstick"],
    # R24 줌인 조준 — 스펙 조사 중. 자리만(눌러도 아직 아무 동작 없음)
    "IA_AimZoom": ["RightMouseButton", "Gamepad_LeftTrigger"],
}

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
for name in TARGETING_ACTIONS:
    action = load_or_create(name, unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    actions[name] = action
    eal.save_loaded_asset(action)

imc = load_or_create("IMC_Targeting", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()
for name, keys in TARGETING_ACTIONS.items():
    for key_name in keys:
        imc.map_key(actions[name], make_key(key_name))
eal.save_loaded_asset(imc)

mappings = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
unreal.log(f"make_targeting_input: {len(actions)} actions, IMC_Targeting mappings = {len(mappings)}")
for m in mappings:
    unreal.log(f"make_targeting_input: {m.get_editor_property('action').get_name()} <- {m.get_editor_property('key').get_editor_property('key_name')}")
