# M3 갈래 J: 메인 메뉴 맵 L_MainMenu 를 만든다(빈 레벨 + 월드 설정 GameMode Override = AGrimMenuGameMode).
# 화면은 C++ 위젯(UGrimMainMenuWidget)이 다 덮으므로 레벨에는 아무것도 놓지 않는다. 다시 돌려도 같은 결과.
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Online\make_main_menu_map.py" -unattended -nop4 -nosplash -nullrhi
#
# 기본 맵(DefaultEngine.ini GameDefaultMap)은 여기서 바꾸지 않는다 — 통합 때 L_MainMenu 로.
import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_MainMenu"
GAME_MODE = "/Script/Grimoire.GrimMenuGameMode"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eal = unreal.EditorAssetLibrary

if eal.does_asset_exist(MAP_PATH):
    if not level_subsystem.load_level(MAP_PATH):
        raise RuntimeError(f"load_level 실패: {MAP_PATH}")
    for old_actor in actor_subsystem.get_all_level_actors():
        if not isinstance(old_actor, unreal.WorldSettings):
            actor_subsystem.destroy_actor(old_actor)
elif not level_subsystem.new_level(MAP_PATH):
    raise RuntimeError(f"new_level 실패: {MAP_PATH}")

game_mode = unreal.load_class(None, GAME_MODE)
if game_mode is None:
    raise RuntimeError(f"클래스 없음: {GAME_MODE} — 먼저 C++ 를 빌드한다")

world = editor_subsystem.get_editor_world()
world_settings = world.get_world_settings()
world_settings.set_editor_property("default_game_mode", game_mode)

if not level_subsystem.save_current_level():
    raise RuntimeError("save_current_level 실패")

unreal.log(f"make_main_menu_map: saved {MAP_PATH} (GameMode Override = {game_mode.get_name()})")
