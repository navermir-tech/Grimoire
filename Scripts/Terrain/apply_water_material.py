# 개울 · 연못 물 표면만 다시 만든다(맵을 다시 만들지 않고) — 생성기로 물 표면 메시(SM_Water_*)를 다시 굽고, MI_Dress_Water 를 만들어 메시와 맵의 물 표면 액터(Water_*_Surface)에 입힌다.
# 재질 값은 dress_assets.py 의 WATER_SCALARS · WATER_COLORS. 돌릴 때마다 같은 결과. 무엇을 왜: docs/systems/terrain.md 「물」.
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Terrain\apply_water_material.py" -unattended -nop4 -nosplash -nullrhi
#
# Water Materials 팩은 메인 체크아웃에만 있다 — 워크트리면 Content\WaterMaterials 정션(docs/dev/market-assets.md).
import json
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import dress_assets  # noqa: E402

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
LAYOUT_PATH = os.path.join(HERE, "openbeta_layout.json")
MESH_DIR = "/Game/Grimoire/Terrain/Water"
SURFACE_PREFIX = "Water_"
SURFACE_SUFFIX = "_Surface"


def first_out(result):
    return result[-1] if isinstance(result, tuple) else result


def main():
    eal = unreal.EditorAssetLibrary
    lib = unreal.GrimTerrainEditorLibrary
    material = dress_assets.make_water_material()
    unreal.log(f"apply_water_material: {material.get_path_name()} (부모 {material.get_editor_property('parent').get_name()})")

    layout = first_out(lib.load_layout_from_json(LAYOUT_PATH))
    first_out(lib.generate_terrain(layout))
    with open(LAYOUT_PATH, encoding="utf-8") as f:
        raw = json.load(f)
    changed = []
    for creek in raw["Creeks"]:
        if not creek.get("bNatural"):
            continue
        bodies = [creek["Name"]] + ([creek["JoinPond"]] if creek.get("JoinPond") else [])
        path = f"{MESH_DIR}/SM_Water_{creek['Name']}"
        if eal.does_asset_exist(path):   # 있는 메시를 다시 빌드하면 ensure — 지우고 새로
            eal.delete_asset(path)
        result = lib.build_water_surface_mesh(path, bodies, material, 1.0)
        mesh = result[0] if isinstance(result, tuple) else result
        if mesh is None:
            raise RuntimeError(f"물 표면 메시 실패: {creek['Name']}")
        eal.save_loaded_asset(mesh, only_if_is_dirty=False)
        changed.append(mesh.get_name())
    unreal.log(f"apply_water_material: 메시 {changed}")

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError("맵을 열지 못했다")
    wpbl = unreal.WorldPartitionBlueprintLibrary
    descs = wpbl.get_actor_descs()
    if isinstance(descs, tuple):
        descs = descs[1]
    wanted = [d for d in descs if str(d.label).startswith(SURFACE_PREFIX) and str(d.label).endswith(SURFACE_SUFFIX)]
    unreal.log(f"apply_water_material: 물 표면 액터 {[str(d.label) for d in wanted]}")
    if not wanted:
        raise RuntimeError("물 표면 액터가 없다")
    wpbl.load_actors([d.guid for d in wanted])
    packages = []
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actor_subsystem.get_all_level_actors():
        label = actor.get_actor_label()
        if not (label.startswith(SURFACE_PREFIX) and label.endswith(SURFACE_SUFFIX)):
            continue
        actor.modify()
        actor.static_mesh_component.set_material(0, material)
        packages.append(actor.get_package())
        unreal.log(f"apply_water_material: 액터 {label}")
    if len(packages) != len(wanted):
        raise RuntimeError(f"올린 액터 수가 다르다: {len(packages)} / {len(wanted)}")
    if not unreal.EditorLoadingAndSavingUtils.save_packages(packages, False):
        raise RuntimeError("액터 저장 실패")
    unreal.log("apply_water_material: 끝")


main()
