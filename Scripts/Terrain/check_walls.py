# 레벨 디자인 3차 — 돌담에 뜬 돌이 없는지 맵에 놓인 인스턴스를 재서 확인한다(docs/qa/level-art.md QA-LVL-022).
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Terrain\check_walls.py" -unattended -nop4 -nosplash -nullrhi
# 돌(FT_Dress_Wall*)마다 바운드 바닥 가운데를 구하고, ① 그 자리 랜드스케이프(라인 트레이스)에 닿았는가(바닥 ≤ 땅 + 허용) 또는
# ② 바로 아래의 다른 돌 바운드 안에 들어 있는가(받쳐 있는가)를 본다. 어느 쪽도 아니면 뜬 돌이다. 결과: Saved/Terrain/ld3_wall_check.json, 로그 끝에 PASS/FAIL.
import json
import math
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
GROUND_TOLERANCE_CM = 8.0      # 둥근 돌 바닥 가운데가 땅에서 이만큼까지 떠 있어도 닿은 것으로 본다(앉힐 때 묻기 16~22 % — 실제로는 땅속)
SUPPORT_TOLERANCE_CM = 6.0

unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
wpbl = unreal.WorldPartitionBlueprintLibrary
descs = wpbl.get_actor_descs()
if isinstance(descs, tuple):
    descs = descs[1]
wpbl.load_actors([d.guid for d in descs])

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
stones = []
for a in actors:
    if not isinstance(a, unreal.InstancedFoliageActor):
        continue
    for comp in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        mesh = comp.get_editor_property("static_mesh")
        if mesh is None or not mesh.get_name().startswith("SM_Stone_Small"):
            continue
        # 돌담 돌은 폴리지 타입 FT_Dress_Wall* — 같은 메시를 쓰는 길가 돌과 끝 컬 거리로 가른다(돌담 14,000 cm, 나머지 12,000 cm)
        if comp.get_editor_property("instance_end_cull_distance") != 14000:
            continue
        b = mesh.get_bounds()
        n = comp.get_instance_count()
        for i in range(n):
            res = comp.get_instance_transform(i, True)
            t = res[1] if isinstance(res, tuple) else res
            sz = t.scale3d.z
            sx, sy = abs(t.scale3d.x), abs(t.scale3d.y)
            ez = b.box_extent.z * abs(sz)
            bottom = t.translation.z + (b.origin.z - b.box_extent.z) * sz
            stones.append((t.translation.x, t.translation.y, bottom, bottom + 2 * ez, max(b.box_extent.x * sx, b.box_extent.y * sy)))
unreal.log(f"check_walls: 돌 {len(stones)} 개")

# 칸으로 묶어 아래 돌을 찾는다
CELL = 200.0
grid = {}
for s in stones:
    grid.setdefault((int(s[0] // CELL), int(s[1] // CELL)), []).append(s)


def ground_z(x, y):
    start = unreal.Vector(x, y, 30000.0)
    end = unreal.Vector(x, y, -5000.0)
    hit = unreal.SystemLibrary.line_trace_single(world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    return hit.to_tuple()[4].z if hit.to_tuple()[0] else None


float_list = []
landed = supported = traced = 0
for s in stones:
    x, y, bottom, top, radius = s
    z = ground_z(x, y)
    if z is not None:
        traced += 1
    if z is not None and bottom <= z + GROUND_TOLERANCE_CM:
        landed += 1
        continue
    cx, cy = int(x // CELL), int(y // CELL)
    held = False
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for o in grid.get((cx + dx, cy + dy), []):
                if o is s:
                    continue
                d = math.hypot(o[0] - x, o[1] - y)
                if d < radius + o[4] and o[2] < bottom and o[3] + SUPPORT_TOLERANCE_CM >= bottom:
                    held = True
                    break
            if held:
                break
        if held:
            break
    if held:
        supported += 1
    else:
        float_list.append({"x": round(x), "y": round(y), "bottom": round(bottom, 1), "ground": round(z, 1) if z is not None else None})
info = {"stones": len(stones), "traced": traced, "on_ground": landed, "on_stone": supported, "floating": len(float_list), "examples": float_list[:20]}
path = os.path.join(ROOT, "Saved", "Terrain", "ld3_wall_check.json")
os.makedirs(os.path.dirname(path), exist_ok=True)
with open(path, "w", encoding="utf-8") as f:
    json.dump(info, f, ensure_ascii=False, indent=1)
unreal.log(f"check_walls: {json.dumps({k: v for k, v in info.items() if k != 'examples'}, ensure_ascii=False)}")
unreal.log("check_walls: " + ("PASS" if not float_list and traced > len(stones) * 0.95 else "FAIL"))
