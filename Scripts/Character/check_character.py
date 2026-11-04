# 갈래 A 에셋 점검(읽기만): 손 소켓 hand_r, 입력 에셋, 샌드박스 맵 로드 · PlayerStart 수.
# 실패하면 "check_character: FAIL" 줄을 남긴다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Character\check_character.py" -unattended -nop4 -nosplash -nullrhi
import unreal

eal = unreal.EditorAssetLibrary
failures = []


def check(ok, what):
    unreal.log(f"check_character: {'OK  ' if ok else 'FAIL'} {what}")
    if not ok:
        failures.append(what)


mesh = eal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
check(mesh is not None, "SKM_Manny_Simple 로드")
if mesh:
    # 본 이름은 소켓처럼 부착 대상으로 쓸 수 있다. 메시를 끼운 임시 컴포넌트로 본 · 소켓을 함께 본다.
    probe = unreal.new_object(unreal.SkeletalMeshComponent)
    probe.set_skeletal_mesh_asset(mesh)
    check(probe.does_socket_exist("hand_r"), f"hand_r (socket={mesh.find_socket('hand_r') is not None}, bone_index={probe.get_bone_index('hand_r')})")

check(eal.load_blueprint_class("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed") is not None, "ABP_Unarmed")

for path in ["/Game/Input/Actions/IA_Move", "/Game/Input/Actions/IA_Look", "/Game/Input/Actions/IA_MouseLook",
             "/Game/Input/Actions/IA_Jump", "/Game/Input/IMC_Default", "/Game/Input/IMC_MouseLook",
             "/Game/Grimoire/Input/IA_SpellCast", "/Game/Grimoire/Input/IA_SpellNext",
             "/Game/Grimoire/Input/IA_SpellPrev", "/Game/Grimoire/Input/IMC_Spell"]:
    check(eal.load_asset(path) is not None, path)

imc = eal.load_asset("/Game/Grimoire/Input/IMC_Spell")
if imc:
    mappings = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
    for m in mappings:
        unreal.log(f"check_character:      IMC_Spell {m.action.get_name()} <- {m.key.get_editor_property('key_name')}")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
check(level_subsystem.load_level("/Game/Grimoire/Maps/L_M1_Sandbox"), "L_M1_Sandbox 로드")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
check(len(starts) >= 2, f"PlayerStart {len(starts)}개")

unreal.log(f"check_character: {'PASS' if not failures else 'FAIL ' + ', '.join(failures)}")
