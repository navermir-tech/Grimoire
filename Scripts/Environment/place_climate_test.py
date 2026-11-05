# M2 날씨 시험 배치: 맵에 기후 지역 볼륨 3개(설산 · 화산 황야 · 폭풍 해안)와 지붕 하나(양초 하나는 지붕 아래, 하나는 밖)를 놓는다.
# 통합 때 L_M1_Sandbox 에 돌린다(갈래 W 는 맵에 놓지 않았다). 볼륨 밖은 기본 지역(초원, PlayerStart 근처).
# 여러 번 돌려도 같은 결과가 되도록, 이 스크립트가 놓은 액터(태그 ClimateTest)를 먼저 지우고 다시 놓는다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Environment\place_climate_test.py" -unattended -nop4 -nosplash -nullrhi
#
# 다른 맵에 놓으려면 환경 변수 GRIM_CLIMATE_TEST_MAP=/Game/... 을 준다.
import os
import unreal

MAP_PATH = os.environ.get("GRIM_CLIMATE_TEST_MAP", "/Game/Grimoire/Maps/L_M1_Sandbox")
TAG = "ClimateTest"
FOLDER = "ClimateTest"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"load_level 실패: {MAP_PATH}")

removed = 0
for actor in actor_subsystem.get_all_level_actors():
    if TAG in [str(t) for t in actor.get_editor_property("tags")]:
        actor_subsystem.destroy_actor(actor)
        removed += 1


def spawn(class_path, x, y, z=0.0, label=None):
    cls = unreal.load_class(None, class_path)
    if cls is None:
        raise RuntimeError(f"클래스 없음: {class_path}")
    actor = actor_subsystem.spawn_actor_from_class(cls, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 0))
    actor.set_editor_property("tags", [TAG])
    if label:
        actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    return actor


def region(region_id, x, y, half_xy, half_z=2000.0, blend=1000.0):
    vol = spawn("/Script/Grimoire.GrimClimateRegionVolume", x, y, 0.0, label=f"Climate_{region_id}")
    vol.set_editor_property("region_id", region_id)
    vol.set_editor_property("blend_distance", blend)
    box = vol.get_component_by_class(unreal.BoxComponent)
    box.set_box_extent(unreal.Vector(half_xy, half_xy, half_z))
    return vol


# 바닥은 100m(±50m). PlayerStart 는 원점 근처 = 초원. 각 지역은 가장자리 1m~ 에서 10m 에 걸쳐 섞인다.
region("SnowPeak", 0, -3500, 1500)
region("VolcanicWaste", 0, 3500, 1500)
region("StormCoast", -3500, 0, 1500)

# 지붕: 양초 하나 위 3m 에 3m x 3m 판(상자). 비가 와도 지붕 아래 양초는 켜진 채, 밖의 양초는 꺼져야 한다.
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
grid = unreal.load_asset("/Engine/EngineMaterials/WorldGridMaterial")
roof = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(2500, 0, 300), unreal.Rotator(0, 0, 0))
roof.static_mesh_component.set_static_mesh(cube)
roof.static_mesh_component.set_material(0, grid)
roof.set_actor_scale3d(unreal.Vector(3, 3, 0.1))
roof.set_editor_property("tags", [TAG])
roof.set_actor_label("ClimateTest_Roof")
roof.set_folder_path(FOLDER)

spawn("/Script/Grimoire.GrimCandle", 2500, 0, label="Candle_Sheltered")
spawn("/Script/Grimoire.GrimCandle", 2500, 600, label="Candle_Outdoor")

if not level_subsystem.save_current_level():
    raise RuntimeError("save_current_level 실패")

unreal.log(f"place_climate_test: removed {removed}, saved {MAP_PATH}")
