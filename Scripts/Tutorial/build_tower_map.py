# M2 갈래 T: 마법탑 튜토리얼 맵 L_Tower 를 그레이박스(엔진 기본 도형)로 새로 만든다.
# 돌릴 때마다 맵을 지우고 처음부터 다시 만든다 — 다시 돌려도 같은 결과(손으로 놓은 액터가 있으면 돌리지 않는다).
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Tutorial\build_tower_map.py" -unattended -nop4 -nosplash -nullrhi
#
# 배치(좌표 · 방 · 태그)는 tower_greybox.py 에 있다 — L_OpenBeta(Scripts/Terrain/build_openbeta.py)도 같은 함수로 탑을 짓는다.
# 좌표(cm): +X = 동쪽(탑 문 · 초원), +Y = 북쪽, 탑 1층 = X −1600~1600 · Y −1200~1200, 벽 높이 5m · 지붕은 그 위.
# 퀘스트 표(Content/Grimoire/Data/Quest*.csv)는 액터 태그로 사물을 찾는다 — 태그 이름을 바꾸면 표도 고친다.
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tower_greybox  # noqa: E402

MAP_PATH = "/Game/Grimoire/Maps/L_Tower"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary

# 맵이 있으면 열어서 액터를 모두 지우고(기본 맵이라 에셋째 지우면 잠겨 있을 수 있다), 없으면 새로 만든다.
if eal.does_asset_exist(MAP_PATH):
    if not level_subsystem.load_level(MAP_PATH):
        raise RuntimeError(f"load_level 실패: {MAP_PATH}")
    for old_actor in actor_subsystem.get_all_level_actors():
        if not isinstance(old_actor, unreal.WorldSettings):
            actor_subsystem.destroy_actor(old_actor)
elif not level_subsystem.new_level(MAP_PATH):
    raise RuntimeError(f"new_level 실패: {MAP_PATH}")

builder = tower_greybox.TowerBuilder()
tower_greybox.build_ground_and_sky(builder)   # 땅 · 하늘 · 기후
tower_greybox.build_tower(builder)            # 탑 1층
tower_greybox.build_yard(builder)             # 탑 밖: 앞마당(동) · 초원

if not level_subsystem.save_current_level():
    raise RuntimeError("save_current_level 실패")

unreal.log(f"build_tower_map: saved {MAP_PATH}")
