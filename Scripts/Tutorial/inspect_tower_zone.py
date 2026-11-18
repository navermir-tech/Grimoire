# 탑 구역(탑 가운데에서 반경 R m) 안에 지금 무엇이 놓여 있는지 적는다 — 탑 아트 시범(docs/art/tower-art.md) 전 · 후 조사용.
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Tutorial\inspect_tower_zone.py" -unattended -nop4 -nosplash -nullrhi
# 결과: Saved/TowerArt/zone_<GRIM_TOWER_INSPECT_TAG 기본 before>.json (액터 · 메시 · 재질 · 폴리지 인스턴스 수)
import json
import math
import os

import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
RADIUS = float(os.environ.get("GRIM_TOWER_INSPECT_RADIUS", "70")) * 100.0
CX, CY = (float(v) for v in os.environ.get("GRIM_TOWER_INSPECT_CENTER", "0,0").split(","))   # 월드 cm
TAG = os.environ.get("GRIM_TOWER_INSPECT_TAG", "before")
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT = os.path.join(ROOT, "Saved", "TowerArt", f"zone_{TAG}.json")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError("load_level 실패")

wp = unreal.WorldPartitionBlueprintLibrary
box = unreal.Box(unreal.Vector(CX - RADIUS, CY - RADIUS, -100000.0), unreal.Vector(CX + RADIUS, CY + RADIUS, 100000.0))
descs = wp.get_intersecting_actor_descs(box)
guids = [d.guid for d in descs]
wp.load_actors(guids)

rows = []
foliage = {}
for actor in actor_subsystem.get_all_level_actors():
    loc = actor.get_actor_location()
    cls = actor.get_class().get_name()
    if isinstance(actor, unreal.InstancedFoliageActor):
        for comp in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
            mesh = comp.get_editor_property("static_mesh")
            n = 0
            near = []
            for i in range(comp.get_instance_count()):
                t = comp.get_instance_transform(i, True)
                d = math.hypot(t.translation.x - CX, t.translation.y - CY)
                if d < RADIUS:
                    n += 1
                    if d < 6000.0:
                        near.append([round(t.translation.x), round(t.translation.y), round(t.translation.z)])
            if n:
                key = mesh.get_path_name() if mesh else "None"
                entry = foliage.setdefault(key, {"count": 0, "within60m": []})
                entry["count"] += n
                entry["within60m"] += near
        continue
    if math.hypot(loc.x - CX, loc.y - CY) > RADIUS:
        continue
    meshes = []
    for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = comp.get_editor_property("static_mesh")
        mats = [m.get_path_name() if m else "None" for m in comp.get_materials()]
        inst = comp.get_instance_count() if isinstance(comp, unreal.InstancedStaticMeshComponent) else 0
        meshes.append({"mesh": mesh.get_path_name() if mesh else "None", "materials": mats, "instances": inst})
    scale = actor.get_actor_scale3d()
    rows.append({
        "label": actor.get_actor_label(), "class": cls, "folder": str(actor.get_folder_path()),
        "loc": [round(loc.x), round(loc.y), round(loc.z)], "yaw": round(actor.get_actor_rotation().yaw, 1),
        "scale": [round(scale.x, 2), round(scale.y, 2), round(scale.z, 2)],
        "tags": [str(t) for t in actor.tags], "meshes": meshes,
    })

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump({"radius_m": RADIUS / 100.0, "actors": rows, "foliage": foliage}, f, ensure_ascii=False, indent=1)
unreal.log(f"inspect_tower_zone: 액터 {len(rows)} · 폴리지 메시 {len(foliage)} → {OUT}")
