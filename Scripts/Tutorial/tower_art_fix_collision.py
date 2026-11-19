# 마법탑 구역 아트(TowerArt 태그) 메시의 충돌을 끈다 — BUG-013.
#
# tower_art_dress.py 는 새 메시를 충돌 없이 놓으려 했지만(set_collision_enabled(NO_COLLISION)) 저장된 액터는
# 충돌이 켜진 채였다. 몸통(SM_Tower_Drum)은 단순 충돌이 1층 방을 채워 PlayerStart 를 덮었다 → 엔진이 시작 자리를 버리고
# 폰이 원점 땅 밑에 생겼다. 설계(docs/art/tower-art.md)는 「새 메시는 충돌 없음, 막는 곳은 숨긴 그레이박스 + TA_Block_* 상자」다.
# 그래서 TA_Block_* 를 뺀 TowerArt 메시를 충돌 프로필 NoCollision 으로 바꿔 저장한다(프로필로 바꿔야 불러올 때 되살아나지 않는다).
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script=Scripts/Tutorial/tower_art_fix_collision.py -unattended -nullrhi
#
# 결과는 Saved/Logs 가 아니라 -abslog 로 준 로그의 「[탑 충돌]」 줄.
import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
TAG = "TowerArt"


def log(msg):
    unreal.log_warning(f"[탑 충돌] {msg}")


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
world = unreal.EditorLevelLibrary.get_editor_world()

changed, kept, already = 0, 0, 0
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
    if TAG not in [str(t) for t in actor.tags]:
        continue
    if actor.get_actor_label().startswith("TA_Block_"):
        kept += 1          # 막는 상자는 그대로
        continue
    comp = actor.static_mesh_component
    if str(comp.get_collision_profile_name()) == "NoCollision" and comp.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
        already += 1
        continue
    actor.modify()
    comp.modify()
    comp.set_collision_profile_name("NoCollision")
    comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    changed += 1

log(f"바꿈 {changed} · 이미 없음 {already} · 막는 상자 그대로 {kept}")
if changed:
    if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
        log("저장 실패")
        raise RuntimeError("save_dirty_packages 실패")
    log(f"저장 {MAP_PATH}")
