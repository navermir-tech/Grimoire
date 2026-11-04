# M1 통합: L_M1_Sandbox 에 하늘 · 양초 · 모닥불 · 마른 풀을 놓고 임시 조명을 지운다.
# 여러 번 돌려도 같은 결과가 되도록, 이 스크립트가 놓은 액터(태그 M1Props)를 먼저 지우고 다시 놓는다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Integration\place_m1_props.py" -unattended -nop4 -nosplash -nullrhi
import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_M1_Sandbox"
TEMP_LIGHT_TAG = "M1TempLighting"   # 갈래 A 의 임시 조명
PROPS_TAG = "M1Props"               # 이 스크립트가 놓는 것

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"load_level 실패: {MAP_PATH}")

removed = 0
for actor in actor_subsystem.get_all_level_actors():
    tags = [str(t) for t in actor.get_editor_property("tags")]
    if TEMP_LIGHT_TAG in tags or PROPS_TAG in tags:
        actor_subsystem.destroy_actor(actor)
        removed += 1


def spawn(class_path, x, y, z=0.0, yaw=0.0, label=None, folder="M1Props"):
    cls = unreal.load_class(None, class_path)
    if cls is None:
        raise RuntimeError(f"클래스 없음: {class_path}")
    actor = actor_subsystem.spawn_actor_from_class(cls, unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    actor.set_editor_property("tags", [PROPS_TAG])
    if label:
        actor.set_actor_label(label)
    actor.set_folder_path(folder)
    return actor


# 하늘 — 해 · 달 · 대기 · 스카이라이트 · 안개를 스스로 가진다 (갈래 D)
spawn("/Script/Grimoire.GrimSkyController", 0, 0, 500, label="Sky", folder="Environment")

# 양초 3개 — PlayerStart(원점 3m 안) 앞쪽에, 서로 1.2m 넘게 떨어뜨린다(가까우면 서로 옮겨붙는다)
for i, (x, y) in enumerate([(500, -250), (500, 0), (500, 250)]):
    spawn("/Script/Grimoire.GrimCandle", x, y, label=f"Candle_{i}")

# 모닥불 1개 — 꺼진 채로 둔다. 불씨로 켜는 것이 시험 항목
spawn("/Script/Grimoire.GrimCampfire", -450, 0, label="Campfire")

# 마른 풀 한 줄 — 1m 간격, 불이 번지는 것을 본다
for i in range(5):
    spawn("/Script/Grimoire.GrimDryGrass", 0, 600 + i * 100, label=f"DryGrass_{i}")

if not level_subsystem.save_current_level():
    raise RuntimeError("save_current_level 실패")

unreal.log(f"place_m1_props: removed {removed}, saved {MAP_PATH}")
