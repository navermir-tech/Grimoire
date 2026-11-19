# 초원 꾸밈 1차 묶음 배치(docs/spec/meadow-dressing.md 「4. 분포 규칙」 · docs/systems/meadow-dressing.md) — L_OpenBeta 마탑 구간.
# 손 배치 없이 시드 고정 규칙으로 고른다. 다시 돌리면 지난번 것(태그 MeadowDress 액터 · FT_Meadow_* 인스턴스)을 걷고 같은 자리에 다시 놓는다.
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Dressing\meadow_place.py" -unattended -nop4 -nosplash -AllowCommandletRendering
#   (먼저 make_meadow_assets.py. dress_openbeta.py · build_openbeta.py 로 맵을 다시 만들면 이것도 다시)
#
# 비우는 곳: 길 · 개울 · 연못 · 랜드마크 · POI · 거점 터 · 측정 자리(dress_openbeta 의 clear_of 와 같은 규칙) + 이 스크립트가 더하는 것
#   - 마법탑과 그 둘레 TOWER_KEEP_OUT m(탑 실외 · 앞마당 · 석등 줄 — 다른 브랜치가 고치는 중)
#   - 전망대 터(레이아웃 Pads) 둘레, 돌담 돌 · 바위 · 나무 둥치(맵의 폴리지 인스턴스 자리에서 잰다)
# 반복 규칙: 같은 종류의 멈추는 것은 SPACING m 안에 하나, 셋 이상 곧게 늘어놓지 않는다(등잔 줄만 예외), 시드 고정.
import json
import math
import os
import random
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(HERE, "..", "Terrain"))
import landmark_greybox  # noqa: E402
import section_dress  # noqa: E402

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
LAYOUT_PATH = os.path.join(HERE, "..", "Terrain", "openbeta_layout.json")
TAG = "MeadowDress"
SEED = 20261118
TOWER_KEEP_OUT = 260.0      # m — 탑 평지 45 m · 앞마당 POI 218 m · 석등 줄(큰길 52~88 m)을 넉넉히 덮는다
VIEW_SPOTS = [(4000, 0), (13776, 3029), (23246, 2090), (41318, -1871), (56800, -11000), (12500, -30000), (62000, 10000)]   # dress_openbeta 와 같다
FOL = "/Game/Grimoire/Dressing/Foliage"
CELL = 256.0                # 채집 셀 한 변(m)
MIN_GROUND_CM = 400.0       # 이보다 낮은 땅(바닷가 · 해수면 근처)은 비운다

# 몇 곳 · 간격(m) — 기획안 표의 제안(마탑 구간 102 ha 기준)
COUNT = {"D26": 10, "D27": 30, "D16": 18, "D04_sites": 7, "D20": 10, "D01": 16, "D02": 24, "D03": 12, "D21": 20, "D29": 14, "D10_branch": 40, "D10_pebble": 40}
SPACING = {"D26": 150, "D27": 60, "D04_sites": 150, "D20": 120, "D01": 120, "D02": 120, "D03": 120, "D21": 70, "D10": 8}

lib = unreal.GrimTerrainEditorLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
rng = random.Random(SEED)
report = {"counts": {}, "placed": []}


def log(msg):
    unreal.log(f"meadow_place: {msg}")


def first_out(result):
    if isinstance(result, tuple):
        if not result[0]:
            raise RuntimeError("C++ 호출 실패(LogGrimTerrain)")
        return result[1]
    if result is None or result is False:
        raise RuntimeError("C++ 호출 실패(LogGrimTerrain)")
    return result


# ───── 레이아웃 · 땅 ─────
layout = first_out(lib.load_layout_from_json(LAYOUT_PATH))
summary = first_out(lib.generate_terrain(layout))
with open(LAYOUT_PATH, encoding="utf-8") as f:
    raw = json.load(f)
origin = raw["WorldOrigin"]
sector = raw["Analysis"]["Sector"]["Polygon"]
tower = raw["TowerHill"]["Center"]
COAST_V = raw.get("CoastLine", 0) + 20   # 해안선 북쪽 20 m 까지(그 위는 땅 높이 MIN_GROUND_CM 로 거른다)
paths = [(section_dress.Polyline(section_dress.catmull_rom(p["Points"])), p["Width"], p["Name"]) for p in raw["Paths"]]


def m2w(u, v):
    return (v - origin["Y"]) * 100.0, (u - origin["X"]) * 100.0


def ground(u, v):
    x, y = m2w(u, v)
    return lib.get_ground_height(x, y)


def slope(u, v):
    x, y = m2w(u, v)
    return lib.get_slope_degrees(x, y)


def seg_dist(px, py, a, b):
    return landmark_greybox._segment_distance(px, py, a[0], a[1], b[0], b[1])


def path_edge_distance(u, v):
    """가장 가까운 길 가장자리까지(m, 길 위면 음수)"""
    best = 1e9
    for poly, width, _ in paths:
        pts = poly.pts
        for i in range(len(pts) - 1):
            a, b = pts[i], pts[i + 1]
            # 멀리 있는 조각은 건너뛴다(두 끝이 같은 쪽으로 150 m 넘게) — 결과 거리에는 영향 없다
            if (u - a[0] > 150 and u - b[0] > 150) or (a[0] - u > 150 and b[0] - u > 150) or (v - a[1] > 150 and v - b[1] > 150) or (a[1] - v > 150 and b[1] - v > 150):
                best = min(best, 150.0)
                continue
            best = min(best, seg_dist(u, v, a, b) - width / 2)
    return best


def polyline_distance(u, v, points):
    return min(landmark_greybox._segment_distance(u, v, points[i]["X"], points[i]["Y"], points[i + 1]["X"], points[i + 1]["Y"]) for i in range(len(points) - 1))


def water_distance(u, v):
    """물가(개울 · 연못 · 강 둑 바깥)까지 m"""
    best = 1e9
    for creek in raw["Creeks"]:
        best = min(best, polyline_distance(u, v, creek["Points"]) - creek["Width"] / 2 - creek.get("BankWidth", 4))
    for pond in raw["Ponds"]:
        best = min(best, math.hypot(u - pond["Center"]["X"], v - pond["Center"]["Y"]) - pond["Radius"] - pond.get("ShoreWidth", 10))
    river = raw["River"]
    best = min(best, polyline_distance(u, v, river["Points"]) - river["Width"] / 2 - river.get("BankWidth", 20))
    return best


def tower_distance(u, v):
    return math.hypot(u - tower["X"], v - tower["Y"])


def clear_of(u, v, margin=0.0):
    """dress_openbeta.clear_of 와 같은 규칙 + 탑 둘레 · 전망대 터 · 바닷가 모래"""
    if not landmark_greybox.point_in_polygon(u, v, sector) or tower_distance(u, v) < TOWER_KEEP_OUT:
        return False
    # 구간 다각형이 남쪽 바닷가까지 들어온다 — 해안선(레이아웃 CoastLine) 둘레 모래와 낮은 땅은 비운다(첫 배치에서 꽃이 모래 위에 놓였다)
    if v < COAST_V or ground(u, v) < MIN_GROUND_CM:
        return False
    if path_edge_distance(u, v) < 6 + margin:
        return False
    for creek in raw["Creeks"]:
        if polyline_distance(u, v, creek["Points"]) < creek["Width"] / 2 + creek.get("BankWidth", 6) / 2 + 3 + margin:
            return False
    for pond in raw["Ponds"]:
        if math.hypot(u - pond["Center"]["X"], v - pond["Center"]["Y"]) < pond["Radius"] + pond.get("ShoreWidth", 10) + 3 + margin:
            return False
    for spec in raw.get("Landmarks", []):
        if math.hypot(u - spec["Center"]["X"], v - spec["Center"]["Y"]) < 30 + margin:
            return False
    for spec in raw.get("Pois", []):
        if math.hypot(u - spec["Center"]["X"], v - spec["Center"]["Y"]) < spec.get("Radius", 12) + 8 + margin:
            return False
    for site in raw.get("BaseSites", []):
        if math.hypot(u - site["Center"]["X"], v - site["Center"]["Y"]) < site["Radius"] + 5 + margin:
            return False
    for pad in raw.get("Pads", []):
        if math.hypot(u - pad["Center"]["X"], v - pad["Center"]["Y"]) < pad.get("Radius", 12) + 6 + margin:
            return False
    x, y = m2w(u, v)
    return all(math.hypot(x - sx, y - sy) >= 1200 for sx, sy in VIEW_SPOTS)


def rel_height(u, v, r=30.0):
    """둘레보다 얼마나 높은가(m) — 언덕 꼭대기 · 안부 찾기"""
    h = ground(u, v)
    ring = [ground(u + r * math.cos(a), v + r * math.sin(a)) for a in (i * math.pi / 4 for i in range(8))]
    return (h - sum(ring) / len(ring)) / 100.0


class Grid:
    """점 자리 해시(m) — 가까운 것 찾기"""

    def __init__(self, size=20.0):
        self.size = size
        self.cells = {}

    def add(self, u, v, r=0.0, data=None):
        self.cells.setdefault((int(u // self.size), int(v // self.size)), []).append((u, v, r, data))

    def near(self, u, v, radius):
        k = int(math.ceil(radius / self.size))
        cu, cv = int(u // self.size), int(v // self.size)
        for i in range(cu - k, cu + k + 1):
            for j in range(cv - k, cv + k + 1):
                for item in self.cells.get((i, j), ()):
                    if math.hypot(item[0] - u, item[1] - v) <= radius + item[2]:
                        yield item

    def any_near(self, u, v, radius):
        return next(self.near(u, v, radius), None) is not None


# ───── 맵 열기 · 지난번 것 걷기 ─────
if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
    raise RuntimeError(f"맵을 열지 못했다: {MAP_PATH}")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
wpbl = unreal.WorldPartitionBlueprintLibrary
descs = wpbl.get_actor_descs()
if isinstance(descs, tuple):
    descs = descs[1]
wpbl.load_actors([d.guid for d in descs])
all_actors = actors.get_all_level_actors()
old_files = []
for a in [a for a in all_actors if a.actor_has_tag(TAG)]:
    package = a.get_package().get_name()
    if package.startswith("/Game/__ExternalActors__/"):
        old_files.append(os.path.join(ROOT, "Content", *package[len("/Game/"):].split("/")) + ".uasset")
    actors.destroy_actor(a)
log(f"지난 {TAG} 액터 {len(old_files)}개 걷음")
for asset_path in unreal.EditorAssetLibrary.list_assets(FOL, False, False):
    ft = unreal.load_asset(asset_path.split(".")[0])
    if ft:
        unreal.InstancedFoliageActor.remove_all_instances(world, ft)

# ───── 맵에 이미 있는 것(폴리지 인스턴스 자리) ─────
trees, rocks = Grid(30.0), Grid(10.0)
n_trees = n_rocks = 0
for ifa in [a for a in actors.get_all_level_actors() if isinstance(a, unreal.InstancedFoliageActor)]:
    for comp in ifa.get_components_by_class(unreal.InstancedStaticMeshComponent):
        mesh = comp.get_editor_property("static_mesh")
        name = mesh.get_name() if mesh else ""
        is_tree = name.startswith("SM_Tree_")
        is_block = name in ("Cube", "Cylinder", "Sphere", "Cone")      # 블록아웃 소품(짐수레 · 울타리 · 건초 …)
        is_rock = name.startswith("SM_Stone_") or "Boulder" in name or "Trunk" in name or is_block
        if not (is_tree or is_rock):
            continue
        for i in range(comp.get_instance_count()):
            t = comp.get_instance_transform(i, True)
            t = t[1] if isinstance(t, tuple) else t
            loc = t.translation
            u, v = loc.y / 100.0 + origin["X"], loc.x / 100.0 + origin["Y"]
            if not landmark_greybox.point_in_polygon(u, v, sector):
                continue
            if is_tree:
                trees.add(u, v, 0.0, t.scale3d.z)
                n_trees += 1
            else:
                if is_block:
                    r = max(0.5, 0.6 * max(t.scale3d.x, t.scale3d.y))
                else:
                    r = max(0.4, t.scale3d.x * (0.9 if "Small" in name or "Pebble" in name else 2.5))
                rocks.add(u, v, r, name)
                n_rocks += 1
log(f"구간 안 나무 {n_trees} · 바위 · 돌 · 통나무 {n_rocks}")

placed = Grid(20.0)       # 이 스크립트가 놓은 것(종류 섞어) — 서로 3 m 안에 겹치지 않게
by_kind = {}


def free(u, v, r=3.0, rock_margin=1.5, tree_margin=2.0):
    if placed.any_near(u, v, r):
        return False
    if rocks.any_near(u, v, rock_margin):
        return False
    return not trees.any_near(u, v, tree_margin)


def far_from_same(kind, u, v, spacing):
    return all(math.hypot(u - a, v - b) >= spacing for a, b in by_kind.get(kind, []))


def claim(kind, u, v, r=3.0):
    placed.add(u, v, r, kind)
    by_kind.setdefault(kind, []).append((u, v))


def candidates(n, seed_offset, margin=2.0):
    """구간 안 무작위 자리(시드 고정)"""
    r = random.Random(SEED + seed_offset)
    xs = [p["X"] for p in sector]
    ys = [p["Y"] for p in sector]
    out = []
    for _ in range(n):
        u, v = r.uniform(min(xs), max(xs)), r.uniform(min(ys), max(ys))
        if clear_of(u, v, margin):
            out.append((u, v))
    return out


def zone_color(u, v):
    """구역마다 주색 하나(분포 규칙 「색 예산」): 부유석 쪽 서쪽 분홍 · 봉화 언덕 가운데 흰빛 · 풍차 쪽 동쪽 노랑"""
    if u < 820:
        return "Pink"
    if u < 1200:
        return "White"
    return "Yellow"


# ───── 놓기 ─────
def spawn_prop(prop_id, u, v, yaw, z_off_cm=0.0, scale=1.0, label=None, journal=None):
    x, y = m2w(u, v)
    z = ground(u, v) + z_off_cm
    actor = actors.spawn_actor_from_class(unreal.GrimReactiveProp, unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    actor.set_editor_property("prop_id", prop_id)
    if journal:
        actor.set_editor_property("journal_override", journal)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    kind = prop_id.split("_")[0]
    n = report["counts"].get(prop_id, 0)
    actor.set_actor_label(label or f"Meadow_{prop_id}_{n:02d}")
    actor.set_folder_path(f"MeadowDress/{prop_id}")
    actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(f"{TAG}.{kind}")])
    try:
        actor.rerun_construction_scripts()
    except AttributeError:
        pass   # set_editor_property 가 이미 다시 지었다(PostEditChange)
    report["counts"][prop_id] = n + 1
    report["placed"].append({"id": prop_id, "label": actor.get_actor_label(), "u": round(u, 1), "v": round(v, 1), "x": round(x), "y": round(y), "z": round(z)})
    return actor


foliage = {}


def add_foliage(ft_name, u, v, yaw, scale, z_off_cm=0.0):
    if ft_name not in foliage:
        foliage[ft_name] = []
    x, y = m2w(u, v)
    foliage[ft_name].append(unreal.Transform(unreal.Vector(x, y, ground(u, v) + z_off_cm), unreal.Rotator(0, 0, yaw),
                                             unreal.Vector(scale, scale, scale)))


# D26 외딴 꽃나무 — 능선 · 안부 · 길 굽이 끝(둘레보다 높은 곳), 구역마다 한 색, 150 m 간격
cands = sorted(((rel_height(u, v), u, v) for u, v in candidates(2500, 26, 6.0) if slope(u, v) < 18 and not trees.any_near(u, v, 25)), reverse=True)
for h, u, v in cands:
    if len(by_kind.get("D26", [])) >= COUNT["D26"]:
        break
    if h < 0.8 or not far_from_same("D26", u, v, SPACING["D26"]) or not free(u, v, 6, 4, 12):
        continue
    add_foliage(f"FT_Meadow_Blossom{zone_color(u, v)}", u, v, rng.uniform(0, 360), rng.uniform(0.75, 0.95), -20)
    claim("D26", u, v, 6)
    report["placed"].append({"id": "D26", "color": zone_color(u, v), "u": round(u, 1), "v": round(v, 1), "x": round(m2w(u, v)[0]), "y": round(m2w(u, v)[1]), "z": round(ground(u, v))})
report["counts"]["D26"] = len(by_kind.get("D26", []))

# D27 색 꽃밭 · 꽃길 — 길에서 3~40 m 열린 풀밭에 굽은 띠(8~18 m), 구역 색(물가는 푸른빛), 60 m 간격
bands = []
for u, v in candidates(3000, 27, 2.0):
    if len(bands) >= COUNT["D27"]:
        break
    edge = path_edge_distance(u, v)
    if not (3 <= edge <= 40) or slope(u, v) > 16 or not far_from_same("D27", u, v, SPACING["D27"]) or not free(u, v, 8, 2, 6):
        continue
    color = "Blue" if water_distance(u, v) < 25 else zone_color(u, v)
    length = rng.uniform(8, 18)
    heading = rng.uniform(0, math.tau)
    bend = rng.uniform(-0.08, 0.08)
    n = int(length / 1.1)
    for k in range(n):
        t = k / max(1, n - 1) - 0.5
        a = heading + bend * k
        bu = u + math.cos(a) * t * length + rng.uniform(-0.8, 0.8)
        bv = v + math.sin(a) * t * length + rng.uniform(-0.8, 0.8)
        if path_edge_distance(bu, bv) < 1.5:
            continue
        for _ in range(rng.choice((1, 2))):   # 띠 폭 1~2 포기
            add_foliage(f"FT_Meadow_Flower{color}{rng.choice((1, 2))}", bu + rng.uniform(-0.7, 0.7), bv + rng.uniform(-0.7, 0.7),
                        rng.uniform(0, 360), rng.uniform(0.9, 1.4), -3)
    claim("D27", u, v, length / 2)
    bands.append((u, v, color))
    report["placed"].append({"id": "D27", "color": color, "u": round(u, 1), "v": round(v, 1), "length": round(length, 1), "x": round(m2w(u, v)[0]), "y": round(m2w(u, v)[1]), "z": round(ground(u, v))})
report["counts"]["D27"] = len(bands)

# D16 낮 나비 — 꽃밭 띠 위(앞에서부터 18 곳)
for u, v, color in bands[:COUNT["D16"]]:
    spawn_prop("D16_Butterflies", u + rng.uniform(-2, 2), v + rng.uniform(-2, 2), rng.uniform(0, 360))

# D04 돌 등잔 — 길 굽이 바깥쪽에 3~5 개 줄(줄 사이 9~11 m), 탑에서 300~900 m(가르치는 순서 「300~700 m」를 탑 비움만큼 민다), 굽이끼리 150 m
bends = []
for poly, width, name in paths:
    step = 3.0
    arc = 12.0
    while arc < poly.length - 12:
        u, v, d0 = poly.at(arc - 12)
        _, _, d1 = poly.at(arc + 12)
        turn = math.degrees(math.acos(max(-1.0, min(1.0, d0[0] * d1[0] + d0[1] * d1[1]))))
        cu, cv, _ = poly.at(arc)
        if turn > 10 and 300 <= tower_distance(cu, cv) <= 1000:
            cross = d0[0] * d1[1] - d0[1] * d1[0]
            bends.append((turn, arc, poly, width, -1 if cross > 0 else 1, name))
        arc += step
bends.sort(key=lambda b: -b[0])
sites = 0
for turn, arc, poly, width, outer, name in bends:
    if sites >= COUNT["D04_sites"]:
        break
    cu, cv, _ = poly.at(arc)
    if not far_from_same("D04_site", cu, cv, SPACING["D04_sites"]) or not landmark_greybox.point_in_polygon(cu, cv, sector):
        continue
    n = rng.choice((3, 4, 4, 5))
    lamps = []
    for k in range(n):
        a = arc + (k - (n - 1) / 2) * rng.uniform(9, 11)
        pu, pv, (dx, dy) = poly.at(a)
        off = width / 2 + 2.4 + rng.uniform(-0.3, 0.4)
        lu, lv = pu - dy * outer * off, pv + dx * outer * off
        if not landmark_greybox.point_in_polygon(lu, lv, sector) or tower_distance(lu, lv) < TOWER_KEEP_OUT or water_distance(lu, lv) < 3 or lv < COAST_V \
                or rocks.any_near(lu, lv, 0.8) or placed.any_near(lu, lv, 2.0) or slope(lu, lv) > 22:
            continue
        lamps.append((lu, lv, math.degrees(math.atan2(dy, dx))))
    if len(lamps) < 3:
        continue
    for lu, lv, heading in lamps:
        spawn_prop("D04_StoneLamp", lu, lv, 90.0 - heading + rng.uniform(-8, 8), -4)
        claim("D04", lu, lv, 1.5)
    claim("D04_site", cu, cv, 0)
    sites += 1
report["counts"]["D04_sites"] = sites

# D20 식은 모닥불 — 큰 나무 밑(6~15 m) 또는 물가(15~40 m), 평지, 탑에서 300 m 밖, 120 m 간격. 넷에는 책장 한 장을 곁에(장면 S1)
fires = []
for u, v in candidates(8000, 20, 4.0):
    if len(fires) >= COUNT["D20"]:
        break
    near_tree = trees.any_near(u, v, 20) and not trees.any_near(u, v, 6)
    wd = water_distance(u, v)
    if not (near_tree or 15 <= wd <= 40) or slope(u, v) > 8 or not (15 <= path_edge_distance(u, v) <= 60):
        continue
    if not far_from_same("D20", u, v, SPACING["D20"]) or not free(u, v, 5, 3, 4):
        continue
    spawn_prop("D20_Campfire", u, v, rng.uniform(0, 360), -5)
    claim("D20", u, v, 4)
    fires.append((u, v))

pages = 0


def page_journal():
    return f"Page_{pages + 1:02d}"


for u, v in fires[:4]:
    for _ in range(20):
        a, r = rng.uniform(0, math.tau), rng.uniform(4, 8)
        pu, pv = u + r * math.cos(a), v + r * math.sin(a)
        if clear_of(pu, pv, 0) and free(pu, pv, 2, 1, 1.5):
            spawn_prop("D21_Page", pu, pv, rng.uniform(0, 360), journal=page_journal())
            claim("D21", pu, pv, 1)
            pages += 1
            break

# D01 바람솜꽃 — 바람받이 언덕 · 안부(둘레보다 높은 곳), 탑에서 가까운 쪽 먼저(가르치는 순서), 120 m 간격
cands = sorted(((tower_distance(u, v), u, v) for u, v in candidates(3000, 1, 3.0) if rel_height(u, v, 25) > 0.6 and slope(u, v) < 22))
for _, u, v in cands:
    if len(by_kind.get("D01", [])) >= COUNT["D01"]:
        break
    if not far_from_same("D01", u, v, SPACING["D01"]) or not free(u, v, 4, 2, 3):
        continue
    spawn_prop("D01_WindCotton", u, v, rng.uniform(0, 360), -3, rng.uniform(0.9, 1.15))
    claim("D01", u, v, 2)

# D02 마른 풀 더미 — 길가(가장자리에서 3~12 m) 먼저, 모자라면 들판. 물에서 10 m 밖(불 쓰는 것). 120 m 간격
# 120 m 간격으로 모자라면 90 m 로(밭 귀퉁이 · 돌담 잔해 밑이 따로 없는 땅이라 — 문서에 적는다)
for pass_edge, spacing in (((3, 12), SPACING["D02"]), ((12, 150), SPACING["D02"]), ((3, 150), 90)):
    for u, v in sorted(candidates(4000, 2 + pass_edge[1], 0.0), key=lambda p: tower_distance(*p)):
        if len(by_kind.get("D02", [])) >= COUNT["D02"]:
            break
        edge = path_edge_distance(u, v)
        if not (pass_edge[0] <= edge <= pass_edge[1]) or water_distance(u, v) < 10 or slope(u, v) > 20:
            continue
        if not far_from_same("D02", u, v, spacing) or not free(u, v, 4, 1.5, 3):
            continue
        spawn_prop("D02_DryGrass", u, v, rng.uniform(0, 360), -4, rng.uniform(0.9, 1.2))
        claim("D02", u, v, 2)

# D03 목마른 들꽃 — 물에서 먼(60 m 밖) 양지(남쪽 비탈 · 평지), 탑에서 가까운 쪽 먼저, 120 m 간격
for u, v in sorted(candidates(3000, 3, 2.0), key=lambda p: tower_distance(*p)):
    if len(by_kind.get("D03", [])) >= COUNT["D03"]:
        break
    if water_distance(u, v) < 60 or slope(u, v) > 15 or trees.any_near(u, v, 12):
        continue
    if not far_from_same("D03", u, v, SPACING["D03"]) or not free(u, v, 4, 1.5, 3):
        continue
    spawn_prop("D03_ThirstyFlower", u, v, rng.uniform(0, 360), -2)
    claim("D03", u, v, 2)

# D21 흩날리는 책장 — 길 밖 비탈(8~28°) · 바위 틈(바위 2~5 m), 70 m 간격, 모두 20 쪽
for u, v in candidates(4000, 21, 3.0):
    if pages >= COUNT["D21"]:
        break
    s = slope(u, v)
    near_rock = rocks.any_near(u, v, 5) and not rocks.any_near(u, v, 1.5)
    if not (8 <= s <= 28 or near_rock) or path_edge_distance(u, v) < 15:
        continue
    if not far_from_same("D21", u, v, SPACING["D21"]) or not free(u, v, 3, 1.2, 2):
        continue
    spawn_prop("D21_Page", u, v, rng.uniform(0, 360), journal=page_journal())
    claim("D21", u, v, 1)
    pages += 1

# D29 떠 있는 돌멩이 — 부유석 쪽으로 150 m 에 걸쳐 14 개, 다가갈수록 촘촘하게(줄 금지 — 옆으로 흔든다), 땅 위 30~80 cm
float_rock = next(l for l in raw["Landmarks"] if l["Kind"] == "FloatingRock")["Center"]
fu, fv = float_rock["X"], float_rock["Y"]
# 출발: 부유석에서 탑 쪽으로(동남) — 길에서 부유석을 향해 올라가게
du, dv = tower["X"] - fu, tower["Y"] - fv
dn = math.hypot(du, dv)
du, dv = du / dn, dv / dn
stones = 0
for k in range(COUNT["D29"] * 30):
    if stones >= COUNT["D29"]:
        break
    t = (stones + 0.5) / COUNT["D29"]
    dist = 35 + 150 * (1 - t) ** 1.7            # 부유석에 가까울수록 촘촘하다
    turn = math.radians(rng.uniform(-30, 30))   # 한 줄로 늘어서지 않게 방향도 흔든다(반복 규칙 4)
    cu, cv = du * math.cos(turn) - dv * math.sin(turn), du * math.sin(turn) + dv * math.cos(turn)
    side = rng.uniform(-14, 14)
    u = fu + cu * dist - cv * side
    v = fv + cv * dist + cu * side
    if not landmark_greybox.point_in_polygon(u, v, sector) or v < COAST_V or path_edge_distance(u, v) < 3 or water_distance(u, v) < 3 or not free(u, v, 4, 1.2, 1.5):
        continue
    # 부유석 구간 이야기 POI(룬 · 수정 · 폐허) 곁은 괜찮다(장면 S7) — 그 터 위만 피한다
    if any(math.hypot(u - p["Center"]["X"], v - p["Center"]["Y"]) < p.get("Radius", 12) for p in raw.get("Pads", []) + raw.get("Pois", [])):
        continue
    if any(math.hypot(u - l["Center"]["X"], v - l["Center"]["Y"]) < 22 for l in raw["Landmarks"]):
        continue
    spawn_prop("D29_FloatStone", u, v, rng.uniform(0, 360), rng.uniform(30, 80), rng.uniform(0.6, 1.5))
    claim("D29", u, v, 1)
    stones += 1

# D10 곁가지 · 돌멩이 — 큰 나무 밑(줄기에서 2~6 m) 40 · 바위 옆(1~3 m) 40, 8 m 간격, 256 m 셀마다 관리자 하나
points = []
tree_list = [item for cell in trees.cells.values() for item in cell]
rng.shuffle(tree_list)
for tu, tv, _, _ in tree_list:
    if sum(1 for p in points if p[0].startswith("D10_Branch")) >= COUNT["D10_branch"]:
        break
    a, r = rng.uniform(0, math.tau), rng.uniform(2.0, 6.0)
    u, v = tu + r * math.cos(a), tv + r * math.sin(a)
    if not clear_of(u, v, -3.0) or slope(u, v) > 25 or any(math.hypot(u - p[1], v - p[2]) < SPACING["D10"] for p in points) or placed.any_near(u, v, 2):
        continue
    # 풀(키 30~50 cm)에 묻히지 않게 조금 크게 · 살짝 들어 — 첫 그림에서 거의 안 보였다
    points.append((f"D10_Branch{rng.randint(1, 3)}", u, v, rng.uniform(0, 360), rng.uniform(1.4, 1.8), 3.0))
rock_list = [item for cell in rocks.cells.values() for item in cell if not str(item[3]).startswith("SM_Stone_Pebble")]
rng.shuffle(rock_list)
for ru, rv, rr, _ in rock_list:
    if sum(1 for p in points if p[0].startswith("D10_Pebble")) >= COUNT["D10_pebble"]:
        break
    a, r = rng.uniform(0, math.tau), rr + rng.uniform(1.0, 3.0)
    u, v = ru + r * math.cos(a), rv + r * math.sin(a)
    if not clear_of(u, v, -3.0) or slope(u, v) > 28 or rocks.any_near(u, v, 0.6) or any(math.hypot(u - p[1], v - p[2]) < SPACING["D10"] for p in points) \
            or placed.any_near(u, v, 2):
        continue
    points.append((f"D10_Pebble{rng.randint(1, 2)}", u, v, rng.uniform(0, 360), rng.uniform(0.5, 0.75), 4.0))
cells = {}
xs = [p["X"] for p in sector]
ys = [p["Y"] for p in sector]
for kind, u, v, yaw, scale, z_off in points:
    key = (int((u - min(xs)) // CELL), int((v - min(ys)) // CELL))
    x, y = m2w(u, v)
    t = unreal.Transform(unreal.Vector(x, y, ground(u, v) + z_off), unreal.Rotator(rng.uniform(-6, 6), rng.uniform(-6, 6), yaw), unreal.Vector(scale, scale, scale))
    cells.setdefault(key, []).append(unreal.GrimHarvestPoint(kind=kind, transform=t))
    report["placed"].append({"id": "D10", "kind": kind, "cell": f"{key[0]}_{key[1]}", "u": round(u, 1), "v": round(v, 1), "x": round(x), "y": round(y), "z": round(ground(u, v))})
for (cu, cv), pts in sorted(cells.items()):
    u = min(xs) + (cu + 0.5) * CELL
    v = min(ys) + (cv + 0.5) * CELL
    x, y = m2w(u, v)
    cell = actors.spawn_actor_from_class(unreal.GrimHarvestCell, unreal.Vector(x, y, ground(u, v)), unreal.Rotator(0, 0, 0))
    cell.set_editor_property("points", pts)
    cell.set_actor_label(f"Meadow_HarvestCell_{cu}_{cv}")
    cell.set_folder_path("MeadowDress/HarvestCells")
    cell.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(f"{TAG}.D10")])
    cell.set_editor_property("is_spatially_loaded", False)
report["counts"]["D10_points"] = len(points)
report["counts"]["D10_cells"] = len(cells)
report["counts"]["D21"] = pages

# ───── 폴리지 넣기 · 저장 ─────
for ft_name, transforms in foliage.items():
    ft = unreal.load_asset(f"{FOL}/{ft_name}")
    if ft is None:
        raise RuntimeError(f"폴리지 타입이 없다: {ft_name} — make_meadow_assets.py 먼저")
    unreal.InstancedFoliageActor.add_instances(world, ft, transforms)
report["counts"]["foliage_instances"] = sum(len(t) for t in foliage.values())

if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
    raise RuntimeError("저장 실패")
removed = 0
for path in old_files:
    if os.path.exists(path):
        os.remove(path)
        removed += 1
out_dir = os.path.join(ROOT, "Saved", "MeadowPlace")
os.makedirs(out_dir, exist_ok=True)
with open(os.path.join(out_dir, "report.json"), "w", encoding="utf-8") as f:
    json.dump(report, f, ensure_ascii=False, indent=1)
log(f"지난 외부 액터 파일 {removed}개 지움")
log("개수 " + ", ".join(f"{k}={v}" for k, v in sorted(report["counts"].items())))
log(f"saved {MAP_PATH} — 보고서 Saved/MeadowPlace/report.json")
