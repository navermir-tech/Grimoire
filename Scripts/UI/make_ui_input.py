# UI 입력 에셋을 만든다(M2 갈래 G): IA_ToggleGrimoire(그리모어 화면) · IA_SpellSet1~4(단축 슬롯 세트), IMC_UI.
# 조작은 호그와트 레거시 배치를 따른다: 그리모어 = T(패드 십자키 →, 호그와트의 마법 배치 메뉴 자리), 세트 1~4 = F1~F4.
# 컨트롤러 파일은 고치지 않는다 — UGrimUISubsystem 이 로컬 플레이어에 IMC_UI 를 더한다(DefaultGame.ini [/Script/Grimoire.GrimUISettings]).
# 다시 돌려도 된다(있으면 매핑만 새로 채운다).
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\UI\make_ui_input.py" -unattended -nop4 -nosplash -nullrhi
import unreal

FOLDER = "/Game/Grimoire/UI"

# 액션 이름 → 키 목록. 키 이름은 EKeys(FKey) 이름.
UI_ACTIONS = {
    "IA_ToggleGrimoire": ["T", "Gamepad_DPad_Right"],
    "IA_SpellSet1": ["F1"],
    "IA_SpellSet2": ["F2"],
    "IA_SpellSet3": ["F3"],
    "IA_SpellSet4": ["F4"],
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


eal.make_directory(FOLDER)
actions = {}
for name in UI_ACTIONS:
    action = load_or_create(name, unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    actions[name] = action
    eal.save_loaded_asset(action)

imc = load_or_create("IMC_UI", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()
for name, keys in UI_ACTIONS.items():
    for key_name in keys:
        imc.map_key(actions[name], make_key(key_name))
eal.save_loaded_asset(imc, only_if_is_dirty=False)  # unmap_all · map_key 는 dirty 표시를 하지 않아 강제로 저장한다

mappings = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
unreal.log(f"make_ui_input: {len(actions)} actions, IMC_UI mappings = {len(mappings)}")
for m in mappings:
    unreal.log(f"make_ui_input: {m.get_editor_property('action').get_name()} <- {m.get_editor_property('key').get_editor_property('key_name')}")
