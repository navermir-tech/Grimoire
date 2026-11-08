# 레벨 아트 1차(시작 구역): 이미 만든 L_OpenBeta(build_openbeta.py) 위에 Fab 팩 에셋을 입힌다 — 다시 돌려도 같은 결과(시드 고정).
# 무엇을 어디에: docs/dev/level-design.md 「8. 레벨 아트 1차」. 에셋 설정: dress_assets.py.
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Terrain\dress_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
#
# 하는 일
#  1. 맵을 열고 World Partition 액터를 모두 올린다(폴리지 · 랜드스케이프 프록시를 고치려면 올라와 있어야 한다).
#  2. 지난번 입힌 것을 걷는다 — 태그 LevelDress 액터 · FT_Dress_* 인스턴스.
#  3. 랜드스케이프 재질 → M_Terrain_Dress(풀 출력 GT_Dress_Meadow — StyleHex 풀 · 꽃 · 풀포기). 맵 전체에 적용된다.
#  4. 생성기 식생(나무 · 덤불 · 바위) 가운데 시작 구역(레이아웃 Analysis.Sector.Polygon) 안의 것을 StyleHex 메시로 바꾼다.
#     자리 · 크기는 생성기 그대로(같은 시드 = 같은 자리), 구역 밖은 그레이박스 도형 그대로.
#  5. 길가 돌 · 덤불, 쓰러진 나무 · 그루터기 · 큰 바위, 먼 하늘의 손그림 구름 카드(참나무 Megaplants 는 지금 0 — PROP_COUNTS).
#     랜드마크 · POI · 거점 터 · 탑 · 길 · 물가 · 측정 자리는 비운다(아래 clear_of).
#
# 풀 지도: 저장(PreSave)이 굽는 풀 지도는 이 맵에서 높이가 틀어진다 — 게임 · 에디터가 시작할 때 한 번 버리고 런타임에 다시 만든다
# (Config/DefaultEngine.ini [SystemSettings] grass.GrassMap.UseRuntimeGeneration=1 · grass.GrassMap.EvictAll=1, docs/dev/level-design.md 「8」).
# build_openbeta.py 로 맵을 다시 만들면 이것도 다시 돌린다(그레이박스 식생 · 재질로 돌아간다).
# 쓰는 팩은 메인 체크아웃에만 있다 — 워크트리면 Content/<팩> 정션(docs/dev/market-assets.md).
# -nullrhi 로 돌리면 랜드스케이프 재질 인스턴스가 바르게 다시 만들어지지 않는다 — -AllowCommandletRendering.
import json
import math
import os
import random
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dress_assets  # noqa: E402
import landmark_greybox  # noqa: E402
import section_dress  # noqa: E402

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
LAYOUT_PATH = os.path.join(HERE, "openbeta_layout.json")
TAG = "LevelDress"
SEED = 20261107

# build_openbeta.py 의 FOLIAGE_PARTS 와 같아야 한다(레벨 디자인 2차부터 맵 전체를 StyleHex 로 바꾸므로 모두 걷는다)
GREYBOX_PARTS = {
    "Tree": [("FT_Terrain_TreeTrunk", (0, 0, 150), (0.4, 0.4, 3.0)), ("FT_Terrain_TreeCrown", (0, 0, 500), (2.6, 2.6, 4.0))],
    "Bush": [("FT_Terrain_Bush", (0, 0, 40), (1.6, 1.6, 1.1))],
    "Rock": [("FT_Terrain_Rock", (0, 0, 10), (1.6, 1.2, 0.9))],
    "DarkTree": [("FT_Terrain_DarkTrunk", (0, 0, 200), (0.45, 0.45, 4.0)), ("FT_Terrain_DarkCrown", (0, 0, 650), (2.2, 2.2, 7.0))],
}

# 측정 · 스크린숏 자리(Scripts/Perf/measure.ps1 · shoot_openbeta.ps1, 월드 cm) — 손 소품을 이 둘레 12 m 안에 두지 않는다
VIEW_SPOTS = [(4000, 0), (13776, 3029), (23246, 2090), (41318, -1871), (56800, -11000), (12500, -30000), (62000, 10000)]

lib = unreal.GrimTerrainEditorLibrary
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
counts = {}


def log(message):
    unreal.log(f"dress_openbeta: {message}")


def count(kind, n=1):
    counts[kind] = counts.get(kind, 0) + n


def first_out(result):
    if isinstance(result, tuple):
        if not result[0]:
            raise RuntimeError("C++ 호출 실패(LogGrimTerrain)")
        return result[1]
    if result is None or result is False:
        raise RuntimeError("C++ 호출 실패(LogGrimTerrain)")
    return result


if "-nullrhi" in unreal.SystemLibrary.get_command_line().lower():
    raise RuntimeError("dress_openbeta: -nullrhi 로는 랜드스케이프 재질 인스턴스가 바르게 저장되지 않는다 — -AllowCommandletRendering 으로 돌린다")

# ───── 1. 레이아웃 · 생성기(땅 높이 · 식생 자리) ─────
layout = first_out(lib.load_layout_from_json(LAYOUT_PATH))
summary = first_out(lib.generate_terrain(layout))
with open(LAYOUT_PATH, encoding="utf-8") as f:
    raw = json.load(f)
origin = raw["WorldOrigin"]
sector = raw["Analysis"]["Sector"]["Polygon"]


def map_to_world(u, v):
    return (v - origin["Y"]) * 100.0, (u - origin["X"]) * 100.0


def world_to_map(x, y):
    return y / 100.0 + origin["X"], x / 100.0 + origin["Y"]


def in_sector_world(x, y):
    u, v = world_to_map(x, y)
    return landmark_greybox.point_in_polygon(u, v, sector)


def _polyline_distance(u, v, points):
    best = 1e9
    for i in range(len(points) - 1):
        a, b = points[i], points[i + 1]
        best = min(best, landmark_greybox._segment_distance(u, v, a["X"], a["Y"], b["X"], b["Y"]))
    return best


def near_water(u, v, margin):
    """연못 · 개울 · 큰 강 물가에서 margin(m) 안인가"""
    for creek in raw["Creeks"]:
        if _polyline_distance(u, v, creek["Points"]) < creek["Width"] / 2 + creek.get("BankWidth", 4) + margin:
            return True
    for pond in raw["Ponds"]:
        if math.hypot(u - pond["Center"]["X"], v - pond["Center"]["Y"]) < pond["Radius"] + pond.get("ShoreWidth", 10) + margin:
            return True
    river = raw["River"]
    return _polyline_distance(u, v, river["Points"]) < river["Width"] / 2 + river.get("BankWidth", 20) + margin


def water_level(name):
    for body in summary.water:
        if str(body.name) == name and body.points:
            return body.points[0].z / 100.0
    return None


def clear_of(u, v, margin=0.0, ignore_pois=False):
    """손 소품을 놓아도 되는 자리인가(지도 m) — 길 · 물 · 탑 · 랜드마크 · POI · 거점 터 · 측정 자리를 비운다"""
    if not landmark_greybox.point_in_polygon(u, v, sector):
        return False
    tower = raw["TowerHill"]
    if math.hypot(u - tower["Center"]["X"], v - tower["Center"]["Y"]) < tower["PlateauRadius"] + 35 + margin:
        return False
    for path in raw["Paths"]:
        if _polyline_distance(u, v, path["Points"]) < path["Width"] / 2 + 6 + margin:
            return False
    for creek in raw["Creeks"]:
        if _polyline_distance(u, v, creek["Points"]) < creek["Width"] / 2 + creek.get("BankWidth", 6) / 2 + 3 + margin:
            return False
    for pond in raw["Ponds"]:
        if math.hypot(u - pond["Center"]["X"], v - pond["Center"]["Y"]) < pond["Radius"] + pond.get("ShoreWidth", 10) + 3 + margin:
            return False
    for spec in raw.get("Landmarks", []):
        if math.hypot(u - spec["Center"]["X"], v - spec["Center"]["Y"]) < 30 + margin:
            return False
    for spec in raw.get("Pois", []):
        if not ignore_pois and math.hypot(u - spec["Center"]["X"], v - spec["Center"]["Y"]) < spec.get("Radius", 12) + 8 + margin:
            return False
    for site in raw.get("BaseSites", []):
        if math.hypot(u - site["Center"]["X"], v - site["Center"]["Y"]) < site["Radius"] + 5 + margin:
            return False
    x, y = map_to_world(u, v)
    for sx, sy in VIEW_SPOTS:
        if math.hypot(x - sx, y - sy) < 1200:
            return False
    return True


# ───── 2. 맵 열기 · 모든 액터 올리기 ─────
if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
    raise RuntimeError(f"맵을 열지 못했다: {MAP_PATH}")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
wpbl = unreal.WorldPartitionBlueprintLibrary
descs = wpbl.get_actor_descs()
if isinstance(descs, tuple):
    descs = descs[1]
wpbl.load_actors([d.guid for d in descs])
all_actors = actor_subsystem.get_all_level_actors()
log(f"액터 {len(descs)}개 올림(올라온 것 {len(all_actors)})")

# ───── 3. 지난번 입힌 것 걷기 ─────
old = [a for a in all_actors if a.actor_has_tag(TAG)]
# 커맨드렛 저장은 지운 액터의 외부 패키지 파일을 지우지 않는다(남으면 다음에 열 때 되살아난다) — 저장 뒤 직접 지운다
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
old_files = []
for a in old:
    package = a.get_package().get_name()   # /Game/__ExternalActors__/Grimoire/Maps/L_OpenBeta/…
    if package.startswith("/Game/__ExternalActors__/"):
        old_files.append(os.path.join(ROOT, "Content", *package[len("/Game/"):].split("/")) + ".uasset")
    actor_subsystem.destroy_actor(a)
log(f"지난 LevelDress 액터 {len(old)}개 지움")
for name in dress_assets.FOLIAGE:
    path = f"{dress_assets.DRESS_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.InstancedFoliageActor.remove_all_instances(world, unreal.load_asset(path))

landscape = None
proxies = []
for a in actor_subsystem.get_all_level_actors():
    if isinstance(a, unreal.LandscapeStreamingProxy):
        proxies.append(a)
    elif isinstance(a, unreal.Landscape):
        landscape = a
if landscape is None:
    raise RuntimeError("랜드스케이프가 없다 — build_openbeta.py 를 먼저 돌린다")
# ───── 4. 에셋 · 5. 랜드스케이프 재질 ─────
# 랜드스케이프가 이미 M_Terrain_Dress 면 재질 · 풀 타입은 건드리지 않는다 — 연 맵의 랜드스케이프를 고쳐 저장하면 저장(PreSave)이 굽는
# 풀 지도의 높이가 틀어진다(언덕 꼭대기 · 탑 평지 풀이 묻히고 골 위에 뜸 — docs/dev/level-design.md 「8」 풀 지도).
# 재질 · 풀을 고치려면 GRIM_DRESS_LANDSCAPE=1 로 이것을 돌린 뒤 build_openbeta.py(처음부터 새 재질로) → 이것을 한 번 더.
dress_material_path = f"{dress_assets.DRESS_DIR}/M_Terrain_Dress"
current = landscape.get_editor_property("landscape_material")
rebuild_landscape = os.environ.get("GRIM_DRESS_LANDSCAPE") == "1" or current is None or current.get_path_name().split(".")[0] != dress_material_path
if rebuild_landscape:
    # 재질을 다시 짜기 전에 그레이박스 재질로 돌려 둔다(쓰는 중인 재질을 고치지 않게)
    landscape.set_editor_property("landscape_material", unreal.load_asset("/Game/Grimoire/Terrain/M_Terrain_Auto"))
assets = dress_assets.make_all(rebuild_landscape)
if rebuild_landscape:
    landscape.set_editor_property("landscape_material", assets["landscape"])
    for p in proxies:
        if p.get_editor_property("landscape_material") != assets["landscape"]:
            p.set_editor_property("landscape_material", assets["landscape"])
    unreal.log_warning("dress_openbeta: 랜드스케이프 재질을 바꿨다 — 이 저장이 굽는 풀 지도는 높이가 틀어진다. "
                       "build_openbeta.py 로 맵을 다시 만든 뒤(새 재질로 처음부터) 이 스크립트를 한 번 더 돌린다")
log(f"랜드스케이프 재질 {'→ 바꿈' if rebuild_landscape else '그대로'}: {assets['landscape'].get_name()} (프록시 {len(proxies)}개)")

# ───── 6. 식생: 맵 전체 StyleHex(레벨 디자인 2차) — 자리 · 크기 · 기울기는 생성기(포아송 원판 · 숲 무늬 · 무리 · 등성이 / 골),
# 종은 생성기 Variant(같은 종끼리 무리 짓는 노이즈)로 고른다. 그레이박스 도형(FT_Terrain_*)은 모두 걷는다 ─────
for rule, parts in GREYBOX_PARTS.items():
    for type_name, offset, scale in parts:
        unreal.InstancedFoliageActor.remove_all_instances(world, unreal.load_asset(f"/Game/Grimoire/Terrain/{type_name}"))
# 구간 구도 자리(section_dress.py)와 랜드마크 둘레는 식생을 비운다 — 풍차 · 고목 실루엣이 나무에 묻히지 않게, 밭 · 곳간 · 봉화대 속에 나무가 나지 않게
# (지도 u, v, 반지름 m, 비울 규칙)
_lm = {l["Kind"]: l["Center"] for l in raw["Landmarks"]}
_poi = {p["Name"]: p["Center"] for p in raw["Pois"]}
ALL_RULES = ("Tree", "DarkTree", "Bush", "Rock")
CLEAR_ZONES = [
    (_lm["Windmill"]["X"], _lm["Windmill"]["Y"], 42, ("Tree", "DarkTree")),
    (_lm["GiantTree"]["X"], _lm["GiantTree"]["Y"], 48, ("Tree", "DarkTree")),
    (_lm["GiantTree"]["X"], _lm["GiantTree"]["Y"], 24, ALL_RULES),
    (_poi["Poi_Windmill_Field"]["X"], _poi["Poi_Windmill_Field"]["Y"], 36, ALL_RULES),
    (_poi["Poi_Windmill_Field"]["X"] + 28, _poi["Poi_Windmill_Field"]["Y"] + 22, 12, ALL_RULES),
    (_poi["Poi_Beacon_Top"]["X"] - 7, _poi["Poi_Beacon_Top"]["Y"] + 4, 10, ALL_RULES),
    (_poi["Poi_Meadow_Bees"]["X"] + 4, _poi["Poi_Meadow_Bees"]["Y"] - 3, 7, ALL_RULES),
]
# 측정 · 스크린숏 자리 둘레 14 m 는 나무 · 덤불을 비운다(카메라 바로 앞을 나무가 막지 않게 — 같은 자리 전 · 후 비교)
CLEAR_ZONES += [(*world_to_map(sx, sy), 14, ("Tree", "DarkTree", "Bush")) for sx, sy in VIEW_SPOTS]


def cleared(rule, x, y):
    u, v = world_to_map(x, y)
    return any(rule in rules and math.hypot(u - cu, v - cv) < r for cu, cv, r, rules in CLEAR_ZONES)


for rule in [r["Name"] for r in raw["Foliage"]]:
    if rule not in dress_assets.RULE_SWAP:
        continue
    pick, part_scale = dress_assets.RULE_SWAP[rule]
    part = unreal.Transform(unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0), unreal.Vector(part_scale, part_scale, part_scale))
    result = lib.get_foliage_instances(rule, part)
    transforms, variants = (result[1], result[2]) if isinstance(result, tuple) and len(result) == 3 else result
    buckets = {}
    for t, var in zip(transforms, variants):
        if cleared(rule, t.translation.x, t.translation.y):
            count(f"Cleared.{rule}")
            continue
        name = pick(var, t.translation.x, t.translation.y)
        buckets.setdefault(name, []).append(t)
    for name, ts in buckets.items():
        unreal.InstancedFoliageActor.add_instances(world, assets["foliage"][name], ts)
        count(f"Dress.{name}", len(ts))
    log(f"식생 {rule}: {sum(len(v) for v in buckets.values())}개 → " + ", ".join(f"{k}={len(v)}" for k, v in sorted(buckets.items())))

rng = random.Random(SEED)


def ground_transform(x, y, yaw, scale, sink_cm=0.0, align=False):
    z = lib.get_ground_height(x, y) - sink_cm
    pitch = roll = 0.0
    if align:   # 비탈을 따라 기울인다(쓰러진 나무)
        d = 150.0
        gx = (lib.get_ground_height(x + d, y) - lib.get_ground_height(x - d, y)) / (2 * d)
        gy = (lib.get_ground_height(x, y + d) - lib.get_ground_height(x, y - d)) / (2 * d)
        c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        pitch = math.degrees(math.atan(gx * c + gy * s))
        roll = -math.degrees(math.atan(-gx * s + gy * c))
    return unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll, pitch, yaw), unreal.Vector(scale, scale, scale))


# ───── 7. 길가: 큰길 · 연못 길 · 동쪽 길의 시작 구역 구간, 양옆 5.5 ~ 9 m 에 작은 돌 · 덤불 ─────
edge = {"FT_Dress_RockSmall1": [], "FT_Dress_RockSmall3": [], "FT_Dress_RockSmall5": [], "FT_Dress_Bush3": []}
for path in raw["Paths"]:
    pts = path["Points"]
    for i in range(len(pts) - 1):
        a, b = pts[i], pts[i + 1]
        length = math.hypot(b["X"] - a["X"], b["Y"] - a["Y"])
        if length < 1:
            continue
        nx, ny = -(b["Y"] - a["Y"]) / length, (b["X"] - a["X"]) / length
        step = 8.0
        for k in range(int(length / step)):
            if rng.random() > 0.55:
                continue
            t = (k + rng.random()) * step / length
            side = 1 if rng.random() < 0.5 else -1
            off = path["Width"] / 2 + rng.uniform(3.5, 7.0)
            u = a["X"] + (b["X"] - a["X"]) * t + nx * off * side
            v = a["Y"] + (b["Y"] - a["Y"]) * t + ny * off * side
            if near_water(u, v, 1.0):
                continue
            if any(_polyline_distance(u, v, p["Points"]) < p["Width"] / 2 + 3.0 for p in raw["Paths"]):
                continue
            tower = raw["TowerHill"]
            if math.hypot(u - tower["Center"]["X"], v - tower["Center"]["Y"]) < tower["PlateauRadius"] + 10:
                continue
            if any(math.hypot(u - s["Center"]["X"], v - s["Center"]["Y"]) < s.get("Radius", 12) + 3 for s in raw.get("Pois", [])):
                continue
            if any(math.hypot(u - pnd["Center"]["X"], v - pnd["Center"]["Y"]) < pnd["Radius"] + 2 for pnd in raw["Ponds"]):
                continue
            x, y = map_to_world(u, v)
            kind = rng.choice(["FT_Dress_RockSmall1", "FT_Dress_RockSmall3", "FT_Dress_RockSmall5", "FT_Dress_Bush3", "FT_Dress_Bush3"])
            scale = rng.uniform(0.35, 0.7) if kind != "FT_Dress_Bush3" else rng.uniform(0.35, 0.6)
            edge[kind].append(ground_transform(x, y, rng.uniform(0, 360), scale, sink_cm=8.0))
for name, transforms in edge.items():
    if transforms:
        unreal.InstancedFoliageActor.add_instances(world, assets["foliage"][name], transforms)
    count(f"PathEdge.{name}", len(transforms))
log("길가: " + ", ".join(f"{k}={len(v)}" for k, v in edge.items()))


# ───── 8. 손 소품(공간 로드, 태그 LevelDress) ─────
def place_prop(kind, index, mesh_path, mesh_kind, transform, folder, block=True, shadow=True, spatial=True):
    mesh = unreal.load_asset(mesh_path)
    if mesh is None:
        raise RuntimeError(f"팩 에셋이 없다: {mesh_path}")
    cls = unreal.SkeletalMeshActor if mesh_kind == "skeletal" else unreal.StaticMeshActor
    actor = actor_subsystem.spawn_actor_from_class(cls, transform.translation, transform.rotation.rotator())
    actor.set_actor_scale3d(transform.scale3d)
    actor.set_actor_label(f"Dress_{kind}_{index:02d}")
    actor.set_folder_path(f"LevelDress/{folder}")
    actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(f"{TAG}.{kind}")])
    if mesh_kind == "skeletal":
        comp = actor.skeletal_mesh_component
        comp.set_skinned_asset_and_update(mesh)
    else:
        comp = actor.static_mesh_component
        comp.set_static_mesh(mesh)
    comp.set_collision_profile_name("BlockAll" if block else "NoCollision")
    comp.set_editor_property("cast_shadow", shadow)
    actor.set_editor_property("is_spatially_loaded", spatial)
    count(f"Prop.{kind}")
    loc = transform.translation
    log(f"  {actor.get_actor_label()} ({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f}) {mesh_path.rsplit('/', 1)[-1]}")
    return actor


def sample_spots(n, margin, max_slope, tries=4000):
    spots = []
    for _ in range(tries):
        if len(spots) >= n:
            break
        u = rng.uniform(420, 1780)
        v = rng.uniform(100, 1080)
        if not clear_of(u, v, margin):
            continue
        x, y = map_to_world(u, v)
        if lib.get_slope_degrees(x, y) > max_slope or lib.get_ground_height(x, y) < 150:
            continue
        if any(math.hypot(u - su, v - sv) < 45 for su, sv, _, _ in spots):
            continue
        spots.append((u, v, x, y))
    return spots


# 개수, 여유 m, 경사 상한. Oak(Megaplants)은 0 — r.Nanite.Foliage(프로젝트 설정)가 꺼져 있으면 잎 없이 줄기만 보인다(Q125).
# Stump 도 0 — 메시 기본 머티리얼의 부모(Universal_Mat)가 다른 스캔의 4K 텍스처를 끌고 와 한 종류에 134 MB(docs/dev/level-design.md 「8」)
PROP_COUNTS = {"FallenTrunk": (7, 4, 9), "Stump": (0, 2, 14), "Boulder": (6, 4, 18), "Oak": (0, 6, 10)}
for kind, (n, margin, max_slope) in PROP_COUNTS.items():
    variants = dress_assets.PROPS[kind]
    for i, (u, v, x, y) in enumerate(sample_spots(n, margin, max_slope)):
        mesh_path, mesh_kind = variants[i % len(variants)]
        yaw = rng.uniform(0, 360)
        if kind == "FallenTrunk":   # 스캔 2 m × 15 cm(나뭇가지 크기) → 굵은 둥치로 키운다
            t = ground_transform(x, y, yaw, rng.uniform(4.5, 5.5), sink_cm=25.0, align=True)
            place_prop(kind, i, mesh_path, mesh_kind, t, "FallenTrunks", block=True, shadow=True)
        elif kind == "Stump":
            t = ground_transform(x, y, yaw, rng.uniform(1.0, 1.4), sink_cm=5.0)
            place_prop(kind, i, mesh_path, mesh_kind, t, "Stumps", block=True, shadow=False)
        elif kind == "Boulder":
            t = ground_transform(x, y, yaw, rng.uniform(0.8, 1.3), sink_cm=40.0)
            place_prop(kind, i, mesh_path, mesh_kind, t, "Boulders", block=True, shadow=True)
        elif kind == "Oak":   # Megaplants 참나무 — Q125 비교용, 길에서 보이는 자리
            t = ground_transform(x, y, yaw, rng.uniform(0.9, 1.1), sink_cm=10.0)
            place_prop(kind, i, mesh_path, mesh_kind, t, "Oaks", block=False, shadow=True)
    log(f"손 소품 {kind}: {counts.get('Prop.' + kind, 0)}개")

# ───── 8b. 구간 구도(콘셉트 아트) · 하늘 구역 이름 — section_dress.py ─────
dresser = section_dress.SectionDresser({
    "raw": raw, "lib": lib, "actors": actor_subsystem, "world": world, "foliage": assets["foliage"], "map_to_world": map_to_world,
    "clear_of": clear_of, "in_sector": lambda u, v: landmark_greybox.point_in_polygon(u, v, sector), "near_water": near_water,
    "water_level": water_level, "count": count,
})
walls, posts, lamps = dresser.approach_road()
log(f"큰길: 돌담 돌 {walls} · 돌 표지 {posts} · 등불 {lamps}")
crest_rocks, crest_trees = dresser.ridge_crests()
log(f"능선 꼭대기: 바위 {crest_rocks} · 외딴 나무 {crest_trees}")
log(f"풍차 마당 {dresser.windmill_farm()} · 봉화대 {dresser.beacon()} · 연못 징검돌 {dresser.pond_stones()} · 부유석 디딤돌 {dresser.floating_steps()} · 둥치 {dresser.meadow_stump()}")
log(f"구간 폴리지 인스턴스 {dresser.flush_instances()}")
log(f"하늘 구역 이름 {dresser.section_labels()}")

# 먼 하늘 손그림 구름 카드 — 늘 로드(하늘 몫, 성능 규칙 6). 탑에서 4 ~ 6.5 km 둘레, 탑을 본다
for i in range(12):
    ang = i * 30.0 + rng.uniform(-10, 10)
    dist = rng.uniform(400000, 650000)
    x, y = dist * math.cos(math.radians(ang)), dist * math.sin(math.radians(ang))
    z = rng.uniform(70000, 120000)
    size = rng.uniform(900, 1500)   # Plane 1 m → 0.9 ~ 1.5 km
    yaw = ang + 180.0
    actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, 90.0, yaw))
    actor.set_actor_scale3d(unreal.Vector(size * rng.uniform(0.45, 0.6), size, 1.0))
    actor.set_actor_label(f"Dress_Cloud_{i:02d}")
    actor.set_folder_path("LevelDress/Clouds")
    actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(f"{TAG}.Cloud")])
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
    comp.set_material(0, assets["clouds"][i % len(assets["clouds"])])
    comp.set_collision_profile_name("NoCollision")
    comp.set_editor_property("cast_shadow", False)
    actor.set_editor_property("is_spatially_loaded", False)
    count("Prop.Cloud")

# ───── 9. 저장 ─────
if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
    raise RuntimeError("저장 실패")
removed = 0
for path in old_files:
    if os.path.exists(path):
        os.remove(path)
        removed += 1
log(f"지난 LevelDress 외부 액터 파일 {removed}개 지움")
log("개수 " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))
log(f"saved {MAP_PATH}")
