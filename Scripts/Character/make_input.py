# 마법 입력 에셋을 만든다: IA_SpellCast · IA_SpellNext · IA_SpellPrev, IMC_Spell.
# 이동 · 시점 · 점프 입력은 템플릿에서 가져온 Content/Input 을 그대로 쓴다.
# 다시 돌려도 된다(있으면 매핑만 새로 채운다).
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Character\make_input.py" -unattended -nop4 -nosplash -nullrhi
import unreal

FOLDER = "/Game/Grimoire/Input"

# 액션 이름 → 키 목록. 키 이름은 EKeys(FKey) 이름.
SPELL_ACTIONS = {
    "IA_SpellCast": ["LeftMouseButton", "Gamepad_RightTrigger"],
    "IA_SpellNext": ["MouseScrollUp", "E", "Gamepad_RightShoulder"],
    "IA_SpellPrev": ["MouseScrollDown", "Q", "Gamepad_LeftShoulder"],
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
for name in SPELL_ACTIONS:
    action = load_or_create(name, unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    actions[name] = action
    eal.save_loaded_asset(action)

imc = load_or_create("IMC_Spell", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()
for name, keys in SPELL_ACTIONS.items():
    for key_name in keys:
        imc.map_key(actions[name], make_key(key_name))
eal.save_loaded_asset(imc)

unreal.log(f"make_input: {len(actions)} actions, IMC_Spell mappings = {len(imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))}")
