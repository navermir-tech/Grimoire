# M3 생물 시험 배치: 서식 구역 3곳(재칼로프 초원 · 켈피 물가 · 검은 개 밤길) + 마른 풀 무더기(씨앗 퍼뜨리기) + 켜진 모닥불(검은 개가 피한다).
# 원본 맵은 건드리지 않는다 — 기본은 L_M1_Sandbox 를 사본(/Game/Grimoire/Creatures/Test/L_CreatureTest)으로 떠서 거기에 놓고 사본만 저장한다.
# 사본은 시험용이라 커밋하지 않는다(지우려면 Content/Grimoire/Creatures/Test 폴더를 지운다). 통합 때는 대상 맵을 정해 같은 스크립트로 놓는다.
# 여러 번 돌려도 같은 결과가 되도록, 이 스크립트가 놓은 액터(태그 CreatureTest)를 먼저 지우고 다시 놓는다.
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Creatures\place_creature_test.py" -unattended -nop4 -nosplash -nullrhi
#
# 환경 변수:
#   GRIM_CREATURE_SOURCE_MAP  사본을 뜰 맵(기본 /Game/Grimoire/Maps/L_M1_Sandbox)
#   GRIM_CREATURE_TEST_MAP    놓을 맵(기본 위 사본). SOURCE 와 같게 주면 그 맵에 바로 놓고 저장한다 — 통합 때만.
# 실행해 보기: UnrealEditor.exe Grimoire.uproject /Game/Grimoire/Creatures/Test/L_CreatureTest -game  (밤 확인은 콘솔 Grim.Creature.Time 0)
import os
import unreal

SOURCE_MAP = os.environ.get("GRIM_CREATURE_SOURCE_MAP", "/Game/Grimoire/Maps/L_M1_Sandbox")
TEST_MAP = os.environ.get("GRIM_CREATURE_TEST_MAP", "/Game/Grimoire/Creatures/Test/L_CreatureTest")
TAG = "CreatureTest"
FOLDER = "CreatureTest"
SPECIES_DIR = "/Game/Grimoire/Creatures/Species"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary

if TEST_MAP != SOURCE_MAP:
    if eal.does_asset_exist(TEST_MAP):
        eal.delete_asset(TEST_MAP)
    eal.make_directory(TEST_MAP.rsplit("/", 1)[0])
    if not eal.duplicate_asset(SOURCE_MAP, TEST_MAP):
        raise RuntimeError(f"맵 사본 실패: {SOURCE_MAP} → {TEST_MAP}")

if not level_subsystem.load_level(TEST_MAP):
    raise RuntimeError(f"load_level 실패: {TEST_MAP}")

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


def species(asset):
    loaded = eal.load_asset(f"{SPECIES_DIR}/{asset}")
    if loaded is None:
        raise RuntimeError(f"종 에셋 없음(먼저 make_creature_species.py): {asset}")
    return loaded


def habitat(label, x, y, half_xy, entries, habitat_tags, seed=0):
    vol = spawn("/Script/Grimoire.GrimHabitatVolume", x, y, 0.0, label=label)
    rows = []
    for asset, cap in entries:
        entry = unreal.GrimHabitatSpeciesEntry()
        entry.set_editor_property("species", species(asset))
        entry.set_editor_property("max_count", cap)
        rows.append(entry)
    vol.set_editor_property("species", rows)
    vol.set_editor_property("habitat_tags", unreal.GrimCreatureSpecies.make_tag_container(habitat_tags))
    vol.set_editor_property("random_seed", seed)
    box = vol.get_component_by_class(unreal.BoxComponent)
    box.set_box_extent(unreal.Vector(half_xy, half_xy, 500.0))
    return vol


# 바닥은 100m(±50m), PlayerStart 는 원점 둘레. 구역은 플레이어에게서 15~30m.
habitat("Habitat_Jackalope_Meadow", 2500, 1500, 1000, [("DA_Creature_Jackalope", 6)], ["Habitat.Meadow", "Habitat.Field"], seed=31)
habitat("Habitat_Kelpie_Waterside", -2500, 1500, 800, [("DA_Creature_Kelpie", 3)], ["Habitat.Waterside"], seed=32)
habitat("Habitat_BlackDog_Road", 0, -3000, 1000, [("DA_Creature_BlackDog", 2)], ["Habitat.Road"], seed=33)

# 재칼로프 구역 안 마른 풀 무더기(씨앗 퍼뜨리기 자극을 받는다)
for i, (dx, dy) in enumerate([(-300, -200), (0, -250), (300, -150), (-200, 200), (150, 250), (350, 100)]):
    spawn("/Script/Grimoire.GrimDryGrass", 2500 + dx, 1500 + dy, 0.0, label=f"DryGrass_{i}")

# 검은 개 구역 가장자리 모닥불(처음부터 켜짐) — 검은 개가 피하고 그림자 걸음이 풀린다
campfire = spawn("/Script/Grimoire.GrimCampfire", 0, -1800, 0.0, label="Campfire_Lit")
state = campfire.get_component_by_class(unreal.GrimElementalStateComponent)
if state is not None:
    state.set_editor_property("initial_states", {unreal.GrimCreatureSpecies.request_tag("State.Burning"): 1.0})

if not level_subsystem.save_current_level():
    raise RuntimeError("save_current_level 실패")

unreal.log(f"place_creature_test: removed {removed}, saved {TEST_MAP} (source {SOURCE_MAP})")
