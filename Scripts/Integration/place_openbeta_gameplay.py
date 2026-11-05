# M3 통합: L_OpenBeta(지형 1차) 위에 게임플레이 액터를 놓는다 — 지형 표식(AGrimTerrainMarker)의 자리를 읽어서.
#  - 생물 서식 구역(AGrimHabitatVolume): Habitat.Jackalope · Habitat.Kelpie · Habitat.BlackDog 표식마다 하나, 반지름 = 표식 반지름
#  - 아궁이 냄비(AGrimCookingPot): 탑 주방 아궁이(태그 Tut_Stove) 위
# 지형을 다시 만들지 않고 이 액터들만 더한다(지형을 다시 만들면 이 스크립트를 다시 돌린다).
# 여러 번 돌려도 같은 결과 — 태그 M3Gameplay 액터를 먼저 지운다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Integration\place_openbeta_gameplay.py" -unattended -nop4 -nosplash -nullrhi
import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
TAG = "M3Gameplay"
HABITATS = {
    "Habitat.Jackalope": ("/Game/Grimoire/Creatures/Species/DA_Creature_Jackalope", 8),
    "Habitat.Kelpie": ("/Game/Grimoire/Creatures/Species/DA_Creature_Kelpie", 3),
    "Habitat.BlackDog": ("/Game/Grimoire/Creatures/Species/DA_Creature_BlackDog", 2),
}

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"load_level 실패: {MAP_PATH}")

removed = 0
for actor in actor_subsystem.get_all_level_actors():
    if TAG in [str(t) for t in actor.get_editor_property("tags")]:
        actor_subsystem.destroy_actor(actor)
        removed += 1


def always_loaded(actor):
    # World Partition 셀이 내렸다 올렸다 하지 않게(서식 구역은 개체군 기록을 쥐고, 냄비는 탑과 같이 늘 있다).
    try:
        actor.set_editor_property("is_spatially_loaded", False)
    except Exception as error:  # 이름이 바뀐 엔진 버전 대비
        unreal.log_warning(f"is_spatially_loaded 설정 실패: {error}")


markers = [a for a in actor_subsystem.get_all_level_actors() if a.get_class().get_name() == "GrimTerrainMarker"]
placed = 0
for marker in markers:
    tag = str(marker.get_editor_property("marker_tag").get_editor_property("tag_name"))
    if tag not in HABITATS:
        continue
    species_path, max_count = HABITATS[tag]
    species = unreal.load_asset(species_path)
    if species is None:
        raise RuntimeError(f"종 없음: {species_path}")
    radius = marker.get_editor_property("radius")
    volume = actor_subsystem.spawn_actor_from_class(unreal.load_class(None, "/Script/Grimoire.GrimHabitatVolume"), marker.get_actor_location(), unreal.Rotator(0, 0, 0))
    entry = unreal.GrimHabitatSpeciesEntry()
    entry.set_editor_property("species", species)
    entry.set_editor_property("max_count", max_count)
    volume.set_editor_property("species", [entry])
    bounds = volume.get_editor_property("bounds")
    bounds.set_box_extent(unreal.Vector(radius, radius, 3000.0))
    volume.set_editor_property("tags", [TAG])
    volume.set_actor_label(f"Habitat_{tag.split('.')[-1]}")
    volume.set_folder_path("Gameplay/Habitats")
    always_loaded(volume)
    placed += 1

stove = next((a for a in actor_subsystem.get_all_level_actors() if "Tut_Stove" in [str(t) for t in a.get_editor_property("tags")]), None)
if stove is None:
    raise RuntimeError("아궁이(Tut_Stove)가 없다 — 탑을 먼저 지어야 한다")
location = stove.get_actor_location() + unreal.Vector(0.0, 0.0, 55.0)
pot = actor_subsystem.spawn_actor_from_class(unreal.load_class(None, "/Script/Grimoire.GrimCookingPot"), location, unreal.Rotator(0, 0, -128.7))
pot.set_editor_property("tags", [TAG, "Tut_CookingPot"])
pot.set_actor_label("Tut_CookingPot")
pot.set_folder_path("Tower/Kitchen")
always_loaded(pot)

# World Partition 맵은 액터가 각자 외부 패키지(__ExternalActors__)라 레벨만 저장하면 빠진다 — 더러운 패키지를 모두 저장.
if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
    raise RuntimeError("저장 실패")
unreal.log(f"place_openbeta_gameplay: removed {removed}, habitats {placed}, pot at {location}")
