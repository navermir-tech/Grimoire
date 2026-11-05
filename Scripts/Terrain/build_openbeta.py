# 지형 1차(M3 갈래 W): 오픈 베타 맵 L_OpenBeta(World Partition, 2 × 4 km — 지역 1 마법탑 · 온난 지대)를 처음부터 만든다.
# 레이아웃(수치)은 openbeta_layout.json, 높이 지도 · 식생 계산은 C++(UGrimTerrainEditorLibrary · FGrimTerrainGenerator).
# 다시 만들기(docs/dev/level-design.md 「6」): 구역 높이 · 언덕 군 · 계곡 절벽 · 폭포 · 고개 + 랜드마크 · POI 그레이박스(landmark_greybox.py).
# 마탑 구간(「7」): 작은 언덕이 촘촘한 구간 + 떠 있는 글씨(AGrimDevLabel, 한글) · 종류 블록 · 구간 밖 「공사 중」 경계.
# 시작할 때 설계 검사(경사 분포 · 구역 높이 차 · 길 위 시선 · POI 밀도 · 길 경사) 숫자를 로그에 남긴다 — 기준은 테스트 Grimoire.Terrain.Design.*.
# 돌릴 때마다 L_OpenBeta 를 지우고 새로 만든다 — 같은 레이아웃 · 시드면 같은 지형 · 같은 식생 자리. 손으로 고친 것은 사라진다(docs/systems/terrain.md).
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Terrain\build_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
#
# -nullrhi 로 돌리면 안 된다: 랜드스케이프 편집 레이어가 GPU 로 최종 높이 지도를 합쳐서, RHI 가 없으면 그림 높이 지도가 0(-256 m)으로
# 저장된다(충돌만 맞고 땅이 안 보인다). -AllowCommandletRendering 이 헤드리스 커맨드렛에 RHI 를 켠다(창은 뜨지 않는다, 첫 실행은 셰이더로 몇 분).
#
# 환경 변수 GRIM_TERRAIN_REFRESH_ASSETS=1 이면 Content/Grimoire/Terrain 의 머티리얼 · 폴리지 타입 등도 새로 만든다(기본: 있으면 그대로).
# 좌표(cm): 월드 원점 = 마법탑 가운데, 월드 +X = 북쪽, +Y = 동쪽(하늘의 해돋이 Yaw 90 = +Y, 언리얼 위에서 보기의 위 = +X).
# 레이아웃 JSON 은 지도 좌표(m, 남서 모서리 (0, 0), X = 동쪽, Y = 북쪽) — map_to_world 가 바꾼다.
import json
import math
import os
import shutil
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "Scripts", "Tutorial"))
import landmark_greybox  # noqa: E402
import terrain_assets  # noqa: E402
import tower_greybox  # noqa: E402

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
LAYOUT_PATH = os.path.join(HERE, "openbeta_layout.json")
PREVIEW_PATH = os.path.join(ROOT, "docs", "public", "art", "terrain", "openbeta-preview.png")
SECTOR_PREVIEW_PATH = os.path.join(ROOT, "docs", "public", "art", "terrain", "openbeta-sector-map-v3.png")
HEIGHTMAP_PATH = os.path.join(ROOT, "Saved", "Terrain", "openbeta.r16")
REFRESH_ASSETS = os.environ.get("GRIM_TERRAIN_REFRESH_ASSETS") == "1"

# 성능 설정(근거는 docs/systems/terrain.md 「성능」)
LANDSCAPE_GRID_COMPONENTS = 2     # 스트리밍 프록시 하나 = 2 × 2 구성 요소 = 508 m
WP_CELL_SIZE_CM = 25600           # World Partition 셀 256 m(폴리지 격자와 같다)
WP_LOADING_RANGE_CM = 51200       # 스트리밍 거리 512 m(나무가 사라지는 400 m 보다 멀리)
TOWER_YAW = 0.0                   # 탑 기준 +X(문)가 월드 +X = 북쪽 초원을 본다

lib = unreal.GrimTerrainEditorLibrary
eal = unreal.EditorAssetLibrary
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
counts = {}


def log(message):
    unreal.log(f"build_openbeta: {message}")


def count(kind, n=1):
    counts[kind] = counts.get(kind, 0) + n


def first_out(result):
    """UFUNCTION 의 bool + out 매개변수가 파이썬에서 (bool, out) 또는 out 으로 오는 두 경우를 받는다"""
    if isinstance(result, tuple):
        ok, value = result[0], result[1]
        if not ok:
            raise RuntimeError("C++ 호출 실패(로그의 LogGrimTerrain 을 본다)")
        return value
    if result is None or result is False:
        raise RuntimeError("C++ 호출 실패(로그의 LogGrimTerrain 을 본다)")
    return result


if "-nullrhi" in unreal.SystemLibrary.get_command_line().lower():
    raise RuntimeError("build_openbeta: -nullrhi 로는 랜드스케이프가 바르게 저장되지 않는다 — -AllowCommandletRendering 으로 돌린다")

# ───── 1. 레이아웃 · 높이 지도 ─────
layout = first_out(lib.load_layout_from_json(LAYOUT_PATH))
summary = first_out(lib.generate_terrain(layout))
with open(LAYOUT_PATH, encoding="utf-8") as f:
    raw = json.load(f)   # 표식 · 길 · 평지 이름은 JSON 을 그대로 읽는 편이 쉽다
origin = raw["WorldOrigin"]


def map_to_world(u, v):
    """지도(u 동, v 북, m) → 월드(cm): 월드 X = 북쪽, Y = 동쪽"""
    return (v - origin["Y"]) * 100.0, (u - origin["X"]) * 100.0


def ground(x, y):
    return lib.get_ground_height(x, y)


log(f"높이 지도 {summary.size_x} × {summary.size_y}, 높이 {summary.min_height:.1f} ~ {summary.max_height:.1f} m, 탑 평지 {summary.tower_plateau_height:.1f} m, "
    f"언덕 {summary.hill_count} · 바위 노두 {summary.outcrop_count}")
log("설계 검사:\n" + lib.run_terrain_analysis())
lib.export_heightmap_r16(HEIGHTMAP_PATH)
lib.export_preview_image(PREVIEW_PATH, 4)
# 마탑 구간 확대 지도(등고선 2 m · 구간 경계 빨간 선 · 언덕 가운데 검은 점)
_sector_poly = raw.get("Analysis", {}).get("Sector", {}).get("Polygon", [])
if _sector_poly:
    lib.export_region_preview_image(SECTOR_PREVIEW_PATH, unreal.Vector2D(380, 150), unreal.Vector2D(1820, 1150), 1.6, 2.0)

# ───── 2. 맵을 지우고 World Partition 맵으로 새로 ─────
# L_OpenBeta 가 에디터 시작 맵(DefaultEngine.ini EditorStartupMap)이라 커맨드렛도 이미 열고 있을 수 있다 —
# 열린 채로 지우면 메모리에 남아 new_level 이 「이미 있다」로 실패한다. 빈 맵으로 바꾸고 치운 뒤 지운다.
unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
unreal.SystemLibrary.collect_garbage()
for asset in (MAP_PATH, MAP_PATH + "_HLODLayer_Instanced", MAP_PATH + "_HLODLayer_Merged"):   # HLOD 층은 new_level 이 다시 만든다
    if eal.does_asset_exist(asset):
        eal.delete_asset(asset)
# 외부 액터 파일은 디스크에서 바로 지운다(delete_directory 는 맵을 불러와 파일을 잡고 놓지 않는다).
# 그 뒤 다시 훑을 때 에셋 레지스트리가 "OpenFile failed" 경고를 많이 남기는데, 지운 파일을 잊는 과정이라 괜찮다.
for sub in ("__ExternalActors__", "__ExternalObjects__"):
    disk = os.path.join(ROOT, "Content", sub, "Grimoire", "Maps", "L_OpenBeta")
    if os.path.isdir(disk):
        shutil.rmtree(disk)
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Grimoire/Maps", "/Game/__ExternalActors__/Grimoire/Maps", "/Game/__ExternalObjects__/Grimoire/Maps"], True)
if not level_subsystem.new_level(MAP_PATH, True):
    raise RuntimeError(f"new_level 실패: {MAP_PATH}")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
log(f"새 World Partition 맵: {world.get_path_name()}")

# ───── 3. 에셋 ─────
assets = terrain_assets.make_all(REFRESH_ASSETS)


def not_spatial(actor):
    """늘 로드(공간 로드 끔) — 시작 자리 · 하늘 · 기후 · 표식 · 바다처럼 어디서나 있어야 하는 것"""
    actor.set_editor_property("is_spatially_loaded", False)
    return actor


def spawn(cls, x, y, z, yaw=0.0, label=None, folder=None, tags=()):
    actor = actor_subsystem.spawn_actor_from_class(cls, unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    if tags:
        actor.set_editor_property("tags", [unreal.Name(t) for t in tags])
    return actor


def mesh_actor(mesh, material, label, x, y, z, sx, sy, sz, folder, yaw=0.0, pitch=0.0, tags=()):
    actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, pitch, yaw))
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    if tags:
        actor.set_editor_property("tags", [unreal.Name(t) for t in tags])
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_material(0, material)
    component.set_collision_profile_name("NoCollision")
    component.set_editor_property("cast_shadow", False)
    actor.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    return actor


def box_volume(cls, label, center, extent, yaw=0.0, folder=None, tags=()):
    volume = lib.spawn_box_volume(world, cls, center, extent, yaw)
    if volume is None:
        raise RuntimeError(f"볼륨을 만들지 못했다: {label}")
    volume.set_actor_label(label)
    if folder:
        volume.set_folder_path(folder)
    if tags:
        volume.set_editor_property("tags", [unreal.Name(t) for t in tags])
    return volume


# ───── 4. 랜드스케이프 ─────
landscape = lib.create_landscape(world, assets["landscape"], assets["layers"], LANDSCAPE_GRID_COMPONENTS, False)
if landscape is None:
    raise RuntimeError("랜드스케이프를 만들지 못했다")
count("Landscape")

map_w, map_h = summary.map_size.x, summary.map_size.y
center_x, center_y = map_to_world(map_w / 2, map_h / 2)
extent_x, extent_y = map_h * 50.0, map_w * 50.0     # 월드 X(북남) 반 길이 = 지도 세로, Y(동서) = 지도 가로

# ───── 5. 물(바다 · 강은 해수면 평면 하나, 연못 · 개울은 따로) + 물 볼륨(헤엄) ─────
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder")
water_mi = assets["water"]
sea_level = raw.get("SeaLevel", 0.0) * 100.0
margin = 2000.0 * 100.0
sea = mesh_actor(PLANE, water_mi, "Water_Sea", center_x, center_y, sea_level, (extent_x * 2 + 2 * margin) / 100.0, (extent_y * 2 + 2 * margin) / 100.0, 1.0, "Water", tags=["Water", "Water.Sea"])
not_spatial(sea)
count("WaterPlane")
sea_depth = (raw.get("SeaDepth", 25.0) + 15.0) * 100.0
sea_volume = box_volume(unreal.PhysicsVolume, "WaterVolume_SeaRiver", unreal.Vector(center_x, center_y, sea_level - sea_depth / 2),
                        unreal.Vector(extent_x + 5000, extent_y + 5000, sea_depth / 2), folder="Water", tags=["Water", "Water.Sea", "Water.River"])
sea_volume.set_editor_property("water_volume", True)
not_spatial(sea_volume)
count("WaterVolume")

for body in summary.water:
    kind = str(body.kind)
    name = str(body.name)
    if kind == "Pond":
        p = body.points[0]
        radius = body.size
        disk = mesh_actor(CYLINDER, water_mi, f"Water_{name}", p.x, p.y, p.z - 5.0, radius * 2 / 100.0, radius * 2 / 100.0, 0.1, "Water/Ponds", tags=["Water", "Water.Pond", name])
        count("WaterPlane")
        depth = body.depth + 100.0
        volume = box_volume(unreal.PhysicsVolume, f"WaterVolume_{name}", unreal.Vector(p.x, p.y, p.z - depth / 2),
                            unreal.Vector(radius, radius, depth / 2), folder="Water/Ponds", tags=["Water", "Water.Pond", name])
        volume.set_editor_property("water_volume", True)
        count("WaterVolume")
    elif kind == "Creek":
        points = list(body.points)
        for i in range(len(points) - 1):
            a, b = points[i], points[i + 1]
            dx, dy, dz = b.x - a.x, b.y - a.y, b.z - a.z
            flat = math.hypot(dx, dy)
            if flat < 1.0:
                continue
            yaw = math.degrees(math.atan2(dy, dx))
            pitch = math.degrees(math.atan2(dz, flat))
            length = math.hypot(flat, dz) + 150.0   # 이음매가 벌어지지 않게 겹친다
            mesh_actor(PLANE, water_mi, f"Water_{name}_{i:03d}", (a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2,
                       length / 100.0, body.size / 100.0, 1.0, f"Water/{name}", yaw=yaw, pitch=pitch, tags=["Water", "Water.Creek", name])
            count("WaterPlane")

# ───── 6. 식생(인스턴스드 폴리지 — 규칙은 C++ 생성기, 밀도는 레이아웃) ─────
FOLIAGE_PARTS = {
    # 규칙 이름: [(폴리지 타입, 부품 위치(cm, 크기 1 기준), 부품 크기)]
    "Tree": [("FT_Terrain_TreeTrunk", (0, 0, 150), (0.4, 0.4, 3.0)), ("FT_Terrain_TreeCrown", (0, 0, 500), (2.6, 2.6, 4.0))],
    "DarkTree": [("FT_Terrain_DarkTrunk", (0, 0, 200), (0.45, 0.45, 4.0)), ("FT_Terrain_DarkCrown", (0, 0, 650), (2.2, 2.2, 7.0))],
    "Bush": [("FT_Terrain_Bush", (0, 0, 40), (1.6, 1.6, 1.1))],
    "Rock": [("FT_Terrain_Rock", (0, 0, 10), (1.6, 1.2, 0.9))],
}
for rule in raw["Foliage"]:
    parts = FOLIAGE_PARTS.get(rule["Name"])
    if not parts:
        unreal.log_warning(f"build_openbeta: 식생 규칙 {rule['Name']} 의 부품이 없다 — 건너뜀")
        continue
    for type_name, offset, scale in parts:
        part = unreal.Transform(unreal.Vector(*offset), unreal.Rotator(0, 0, 0), unreal.Vector(*scale))
        transforms = lib.get_foliage_transforms(rule["Name"], part)
        if transforms:
            unreal.InstancedFoliageActor.add_instances(world, assets[type_name], transforms)
        count(f"Foliage.{type_name}", len(transforms))
    log(f"식생 {rule['Name']}: {counts.get('Foliage.' + parts[0][0], 0)}그루/개")

# ───── 7. 표식(탑 자리 · 앞마당 · 서식 구역 · 거점 평지 · 길) ─────
MARKER_CLASS = unreal.load_class(None, "/Script/Grimoire.GrimTerrainMarker")


def marker(name, tag_name, u, v, radius_m, folder):
    x, y = map_to_world(u, v)
    actor = spawn(MARKER_CLASS, x, y, ground(x, y) + 50.0, label=name, folder=folder, tags=[tag_name, name])
    actor.set_editor_property("marker_tag", tower_greybox.tag(tag_name))
    actor.set_editor_property("radius", radius_m * 100.0)
    not_spatial(actor)
    count("Marker")
    return actor


for spec in raw["Markers"]:
    marker(spec["Name"], spec["Tag"], spec["Center"]["X"], spec["Center"]["Y"], spec["Radius"], "Markers/Sites" if spec["Tag"].startswith("Terrain") else "Markers/Habitats")
for site in raw["BaseSites"]:
    marker(site["Name"], "Terrain.Site.Base", site["Center"]["X"], site["Center"]["Y"], site["Radius"], "Markers/BaseSites")
for path in raw["Paths"]:
    for i, p in enumerate(path["Points"]):
        marker(f"{path['Name']}_{i:02d}", "Terrain.Path", p["X"], p["Y"], path["Width"] / 2, f"Markers/{path['Name']}")
LANDMARK_MARKER_RADIUS_M = {"Tower": 45, "GiantTree": 30, "RockGate": 45, "StoneCircle": 18, "Waterfall": 15, "Watchtower": 12, "Windmill": 10, "FloatingRock": 12}
for spec in raw.get("Landmarks", []):
    marker(spec["Name"], f"Landmark.{spec['Kind']}", spec["Center"]["X"], spec["Center"]["Y"], LANDMARK_MARKER_RADIUS_M.get(spec["Kind"], 20), "Markers/Landmarks")
for spec in raw.get("Pois", []):
    marker(spec["Name"], f"Poi.{spec['Kind']}", spec["Center"]["X"], spec["Center"]["Y"], spec.get("Radius", 12), f"Markers/Poi/{spec.get('Cluster', '')}")

# ───── 7b. 랜드마크 · POI 그레이박스 · 다리(늘 로드 — 멀리서 보여야 하는 것) ─────


def _spawned(actor):
    not_spatial(actor)
    count("GreyboxActor")


placer = landmark_greybox.Placer(actor_subsystem, ground, map_to_world, assets["landmark_materials"], _spawned)
falls_by_name = {str(body.name): body for body in summary.water if str(body.kind) == "Falls"}


def falls_volume(label, center, extent, yaw):
    volume = box_volume(unreal.PhysicsVolume, label, center, extent, yaw=yaw, folder="Water/Falls", tags=["Water", "Water.Falls"])
    volume.set_editor_property("water_volume", True)
    not_spatial(volume)
    count("WaterVolume")


built = landmark_greybox.build_landmarks(placer, raw, falls_by_name, falls_volume)
log(f"랜드마크 그레이박스: {', '.join(built)} (마법탑은 tower_greybox)")
poi_kinds = landmark_greybox.build_pois(placer, raw)
log("POI 소품: " + ", ".join(f"{k}={v}" for k, v in sorted(poi_kinds.items())))
count("Bridge", landmark_greybox.build_bridges(placer, raw, None))

# ───── 7c. 마탑 구간(docs/dev/level-design.md 「7」): 떠 있는 글씨(랜드마크 · 구간 POI) + 종류 블록 · 공사 중 경계 ─────
labels = landmark_greybox.build_dev_labels(placer, raw)
posts, signs = landmark_greybox.build_construction_fence(placer, raw)
count("DevLabel", labels + signs)
count("ConstructionPost", posts)
log(f"개발 표시: 글씨 {labels}(랜드마크 · 구간 POI) + 공사 중 글씨 {signs}, 공사 중 기둥 {posts}")

# ───── 8. 마법탑(L_Tower 와 같은 그레이박스를 탑 자리에) ─────
tower_x, tower_y = map_to_world(raw["TowerHill"]["Center"]["X"], raw["TowerHill"]["Center"]["Y"])
builder = tower_greybox.TowerBuilder(origin=(tower_x, tower_y), yaw=TOWER_YAW, ground=ground, folder_root="TowerSite")
tower_greybox.build_tower(builder)
tower_greybox.build_yard(builder)
# 탑 · 앞마당은 늘 로드 — 시작 자리가 플레이어보다 먼저 있어야 하고, 튜토리얼 액터(L_Tower 에서 온)는
# 셀이 내렸다 다시 올라오는 것(EndPlay → BeginPlay)을 겪어 본 적이 없다. 114개라 늘 둬도 싸다.
for actor in builder.spawned:
    not_spatial(actor)
count("TowerActor", len(builder.spawned))

# ───── 9. 하늘 · 기후(Meadow 하나로 전체) · 지도 끝 막기 ─────
plateau_z = summary.tower_plateau_height * 100.0
not_spatial(spawn(unreal.load_class(None, "/Script/Grimoire.GrimSkyController"), tower_x, tower_y, plateau_z + 500.0, label="Sky", folder="Environment"))
climate = spawn(unreal.load_class(None, "/Script/Grimoire.GrimClimateRegionVolume"), center_x, center_y, 0.0, label="Climate_Meadow", folder="Environment")
climate.set_editor_property("region_id", "Meadow")
climate.set_editor_property("blend_distance", 1000.0)
climate.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(extent_x + 10000, extent_y + 10000, 50000))
not_spatial(climate)

wall_h = 100000.0
south_x, west_y = map_to_world(0, 0)
north_x, east_y = map_to_world(map_w, map_h)
for label, cx, cy, ex, ey in [
    ("MapEdge_West", center_x, west_y - 500, extent_x + 1000, 500),
    ("MapEdge_East", center_x, east_y + 500, extent_x + 1000, 500),
    ("MapEdge_North", north_x + 500, center_y, 500, extent_y + 1000),
    ("MapEdge_South", south_x - 500, center_y, 500, extent_y + 1000),
]:
    not_spatial(box_volume(unreal.BlockingVolume, label, unreal.Vector(cx, cy, 0.0), unreal.Vector(ex, ey, wall_h), folder="Environment/MapEdge"))
    count("MapEdge")

# ───── 10. World Partition 설정 기록 · 저장 ─────
wp_result = lib.configure_world_partition(world, WP_CELL_SIZE_CM, WP_LOADING_RANGE_CM)
if not wp_result:
    raise RuntimeError("World Partition 격자를 설정하지 못했다")
log(f"World Partition: {wp_result}")

if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
    raise RuntimeError("저장 실패")

log("개수 " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))
log(f"saved {MAP_PATH}")
