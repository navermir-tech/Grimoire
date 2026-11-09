# 레벨 디자인 4차 — 돌담 검사(docs/qa/level-art.md QA-LVL-03x). 맵에 놓인 인스턴스를 잰다.
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Terrain\check_walls.py" -unattended -nop4 -nosplash -nullrhi
# 납작하게 눌린 돌(담의 단 · 갓돌, FT_Dress_Wall*)만 본다 — 봉화대 · 무더기처럼 둥근 돌은 담이 아니다.
#   뜬 돌    바닥이 땅에도 아래 돌에도 닿지 않는 돌
#   틈       같은 단 안에서 옆 돌과 6 cm 넘게 벌어진 곳(양쪽에 돌이 있는데 비어 있는 자리)
#   돌탑     위쪽 단(땅 위 50 cm 넘는 곳)의 돌인데 같은 단에 옆 돌이 하나도 없는 것 — 기둥처럼 따로 솟은 돌
# 결과: Saved/Terrain/ld4_wall_check.json, 로그 끝에 PASS/FAIL.
import json
import math
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
GROUND_TOLERANCE_CM = 6.0      # 바닥 가운데가 땅에서 이만큼까지 떠 있어도 닿은 것으로 본다
SUPPORT_TOLERANCE_CM = 5.0
GAP_LIMIT_CM = 6.0
SAME_COURSE_CM = 7.0
TOWER_HEIGHT_CM = 50.0
FLAT_RATIO = 1.5               # 긴 변 ÷ 높이가 이보다 크면 담 돌(세워 놓은 갓돌을 빼지 않게 낮다)

unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
wpbl = unreal.WorldPartitionBlueprintLibrary
descs = wpbl.get_actor_descs()
if isinstance(descs, tuple):
    descs = descs[1]
wpbl.load_actors([d.guid for d in descs])

# 돌담이 쌓을 때 남긴 돌 자리(Saved/Terrain/ld4_wall_report.json) — FT_Dress_Wall* 은 봉화대 · 무더기에도 쓰이므로 담 돌만 고른다
with open(os.path.join(ROOT, "Saved", "Terrain", "ld4_wall_report.json"), encoding="utf-8") as report_file:
    wall_report = json.load(report_file)
with open(os.path.join(HERE, "openbeta_layout.json"), encoding="utf-8") as layout_file:
    origin = json.load(layout_file)["WorldOrigin"]
known = {(round((row["v"] - origin["Y"]) * 100.0), round((row["u"] - origin["X"]) * 100.0)) for row in wall_report["stones"]}

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
        for i in range(comp.get_instance_count()):
            res = comp.get_instance_transform(i, True)
            t = res[1] if isinstance(res, tuple) else res
            sx, sy, sz = abs(t.scale3d.x), abs(t.scale3d.y), abs(t.scale3d.z)
            ex, ey, ez = b.box_extent.x * sx, b.box_extent.y * sy, b.box_extent.z * sz
            if max(ex, ey) / max(ez, 0.01) < FLAT_RATIO or (round(t.translation.x), round(t.translation.y)) not in known:
                continue
            bottom = t.translation.z + (b.origin.z - b.box_extent.z) * t.scale3d.z
            yaw = math.radians(t.rotation.rotator().yaw)
            rise = math.tan(math.radians(t.rotation.rotator().pitch))
            stones.append({"x": t.translation.x, "y": t.translation.y, "bottom": bottom, "top": bottom + 2 * ez, "hl": ex, "ht": ey, "ax": math.cos(yaw), "ay": math.sin(yaw), "rise": rise})
unreal.log(f"check_walls: 담 돌 {len(stones)} 개 / 쌓을 때 {len(known)} 개")

CELL = 200.0
grid = {}
for s in stones:
    grid.setdefault((int(s["x"] // CELL), int(s["y"] // CELL)), []).append(s)


def neighbours(s):
    cx, cy = int(s["x"] // CELL), int(s["y"] // CELL)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for o in grid.get((cx + dx, cy + dy), []):
                if o is not s:
                    yield o


def ground_z(x, y):
    start = unreal.Vector(x, y, 30000.0)
    end = unreal.Vector(x, y, -5000.0)
    hit = unreal.SystemLibrary.line_trace_single(world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    return hit.to_tuple()[4].z if hit.to_tuple()[0] else None


floating, gaps, towers = [], [], []
landed = supported = traced = 0
for s in stones:
    z = ground_z(s["x"], s["y"])
    if z is not None:
        traced += 1
    if z is not None and s["bottom"] <= z + GROUND_TOLERANCE_CM:
        landed += 1
    else:
        held = False
        for o in neighbours(s):
            d = math.hypot(o["x"] - s["x"], o["y"] - s["y"])
            if d < max(s["hl"], s["ht"]) + max(o["hl"], o["ht"]) and o["bottom"] < s["bottom"] and o["top"] + SUPPORT_TOLERANCE_CM >= s["bottom"]:
                held = True
                break
        if held:
            supported += 1
        else:
            floating.append({"x": round(s["x"]), "y": round(s["y"]), "bottom": round(s["bottom"], 1), "ground": round(z, 1) if z is not None else None})
    # 같은 단의 이웃
    ahead, behind = None, None
    for o in neighbours(s):
        along = (o["x"] - s["x"]) * s["ax"] + (o["y"] - s["y"]) * s["ay"]
        # 같은 단: 돌 기울기만큼 비탈을 따라 높이가 달라지는 것을 뺀다
        if abs((o["bottom"] - s["bottom"]) - along * s["rise"]) > SAME_COURSE_CM:
            continue
        cross = abs(-(o["x"] - s["x"]) * s["ay"] + (o["y"] - s["y"]) * s["ax"])
        if cross > 25.0:
            continue
        reach = s["hl"] + o["hl"]
        if along > 0 and (ahead is None or along - reach < ahead):
            ahead = along - reach
        if along < 0 and (behind is None or -along - reach < behind):
            behind = -along - reach
    if ahead is not None and behind is not None and min(ahead, behind) > GAP_LIMIT_CM and min(ahead, behind) < 150.0:
        gaps.append({"x": round(s["x"]), "y": round(s["y"]), "gap": round(min(ahead, behind), 1)})
    height = s["bottom"] - z if z is not None else 0.0
    if height > TOWER_HEIGHT_CM and ahead is None and behind is None:
        towers.append({"x": round(s["x"]), "y": round(s["y"]), "height": round(height, 1)})

info = {"stones": len(stones), "traced": traced, "on_ground": landed, "on_stone": supported, "floating": len(floating), "gaps_over_6cm": len(gaps), "towers": len(towers),
        "examples": {"floating": floating[:15], "gaps": gaps[:15], "towers": towers[:15]}}
path = os.path.join(ROOT, "Saved", "Terrain", "ld4_wall_check.json")
os.makedirs(os.path.dirname(path), exist_ok=True)
with open(path, "w", encoding="utf-8") as f:
    json.dump(info, f, ensure_ascii=False, indent=1)
unreal.log(f"check_walls: {json.dumps({k: v for k, v in info.items() if k != 'examples'}, ensure_ascii=False)}")
ok = not floating and not towers and len(gaps) <= len(stones) * 0.02 and traced > len(stones) * 0.95
unreal.log("check_walls: " + ("PASS" if ok else "FAIL"))
