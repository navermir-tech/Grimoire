# M1 샌드박스 맵 L_M1_Sandbox 를 새로 만든다: 바닥 · 임시 조명 · PlayerStart 4개 · 거리감용 상자.
# 실행할 때마다 맵을 처음부터 다시 만든다 — 통합 단계에서 손으로 놓은 액터가 있으면 돌리지 않는다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Character\make_sandbox_map.py" -unattended -nop4 -nosplash -nullrhi
import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_M1_Sandbox"

# 갈래 D 의 하늘 액터가 들어오면 지울 임시 조명. 이 태그로 찾는다.
TEMP_LIGHT_TAG = "M1TempLighting"

FLOOR_SIZE_M = 100  # 한 변(미터). 기본 Plane 은 1m.

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary

if eal.does_asset_exist(MAP_PATH):
    eal.delete_asset(MAP_PATH)

if not level_subsystem.new_level(MAP_PATH):
    raise RuntimeError(f"new_level 실패: {MAP_PATH}")


def spawn(actor_class, location, rotation=unreal.Rotator(0, 0, 0), label=None, folder=None):
    actor = actor_subsystem.spawn_actor_from_class(actor_class, location, rotation)
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    return actor


def tag_temp(actor):
    actor.set_editor_property("tags", [TEMP_LIGHT_TAG])


# 바닥
plane = unreal.load_asset("/Engine/BasicShapes/Plane")
grid = unreal.load_asset("/Engine/EngineMaterials/WorldGridMaterial")
floor = spawn(unreal.StaticMeshActor, unreal.Vector(0, 0, 0), label="Floor", folder="Geometry")
floor_mesh = floor.static_mesh_component
floor_mesh.set_static_mesh(plane)
floor_mesh.set_material(0, grid)
floor.set_actor_scale3d(unreal.Vector(FLOOR_SIZE_M, FLOOR_SIZE_M, 1))

# 거리감 · 엄폐 확인용 상자 몇 개
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
for i, (x, y, s) in enumerate([(800, 0, 1.0), (800, 400, 2.0), (-600, -700, 1.5), (0, 1200, 3.0)]):
    box = spawn(unreal.StaticMeshActor, unreal.Vector(x, y, 50 * s), label=f"Box_{i}", folder="Geometry")
    box.static_mesh_component.set_static_mesh(cube)
    box.static_mesh_component.set_material(0, grid)
    box.set_actor_scale3d(unreal.Vector(s, s, s))

# 임시 조명 (갈래 D 하늘 액터로 교체 예정)
sun = spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, -45, -30), label="TempSun", folder="TempLighting")
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
tag_temp(sun)
for actor_class, label in [(unreal.SkyAtmosphere, "TempSkyAtmosphere"), (unreal.ExponentialHeightFog, "TempFog")]:
    tag_temp(spawn(actor_class, unreal.Vector(0, 0, 500), label=label, folder="TempLighting"))
sky_light = spawn(unreal.SkyLight, unreal.Vector(0, 0, 500), label="TempSkyLight", folder="TempLighting")
sky_light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky_light.light_component.set_editor_property("real_time_capture", True)
tag_temp(sky_light)

# PlayerStart 4개 — 원 위에 서로 마주 보게
for i, (x, y) in enumerate([(0, -300), (0, 300), (-300, 0), (300, 0)]):
    yaw = unreal.MathLibrary.deg_atan2(-y, -x) if (x or y) else 0.0
    spawn(unreal.PlayerStart, unreal.Vector(x, y, 100), unreal.Rotator(0, 0, yaw), label=f"PlayerStart_{i}", folder="PlayerStarts")

if not level_subsystem.save_current_level():
    raise RuntimeError("save_current_level 실패")

unreal.log(f"make_sandbox_map: saved {MAP_PATH}")
