# 마법탑 아트 시범(docs/art/tower-art.md): L_OpenBeta 의 탑 구역(탑 실외 · 1층 실내 · 앞마당 · 앞마당 POI)을 자체 제작 에셋으로 입힌다.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Tutorial\tower_art_dress.py" -unattended -nop4 -nosplash -nullrhi
#   되돌리기: 환경 변수 GRIM_TOWER_ART_REVERT=1 로 같은 명령 — 새 액터를 지우고 숨긴 것을 다시 보이게, 뺀 폴리지 인스턴스를 되돌린다.
#
# 순서: tower_art_import.py(에셋) → 이것. build_openbeta.py · dress_openbeta.py 로 맵을 다시 만들면 이것도 다시 돌린다.
# - 게임 액터(거점 · 상점 · 스승 · 게시판 · 양초 · 작물 · 과녁 · 침대 · 모닥불 · 냄비 · 전투 금지 볼륨 · 표식)는 건드리지 않는다.
# - 그레이박스 · 팩 소품은 지우지 않고 **보이지만 않게** 한다(충돌은 그대로 — 길 · 문 · 방 크기가 전과 같다). 태그 TowerArtHidden.
# - 새 메시는 충돌이 없다. 새로 생긴 막는 곳(둥근 기단 · 현관 · 통로 · 책장 · 소품)은 숨긴 상자 충돌을 따로 놓는다.
# - 뺀 팩 폴리지 인스턴스(길가 돌 등)는 Scripts/Tutorial/tower_art_removed.json 에 적는다(되돌리기용).
import json
import math
import os
import random

import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_OpenBeta"
TAG, HIDDEN_TAG = "TowerArt", "TowerArtHidden"
MESH = "/Game/Grimoire/Environment/Tower/{0}/StaticMeshes/{0}"
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
REMOVED_JSON = os.path.join(HERE, "tower_art_removed.json")
REVERT = os.environ.get("GRIM_TOWER_ART_REVERT") == "1"
ZONE_R = 6200.0          # 탑 가운데에서 이 안의 팩 폴리지 인스턴스를 뺀다(평지 45 m + 길 어귀)
SPOT_R = 350.0           # 석등 · 화로 제단 · 돌무더기 둘레에서 뺄 반지름

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
counts = {}
rng = random.Random(20261118)


def log(msg):
    unreal.log(f"tower_art_dress: {msg}")


def count(kind, n=1):
    counts[kind] = counts.get(kind, 0) + n


# ───── 1. 맵 열기 · 모든 액터 올리기 ─────
if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
    raise RuntimeError("맵을 열지 못했다")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
wpbl = unreal.WorldPartitionBlueprintLibrary
descs = wpbl.get_actor_descs()
if isinstance(descs, tuple):
    descs = descs[1]
wpbl.load_actors([d.guid for d in descs])
actors = actor_subsystem.get_all_level_actors()
by_label = {a.get_actor_label(): a for a in actors}
log(f"액터 {len(actors)}")

# ───── 2. 지난번 것 걷기(새 액터 지움 · 숨긴 것 다시 보이게 · 뺀 인스턴스 되돌림) ─────
old_files = []
for a in [a for a in actors if a.actor_has_tag(TAG)]:
    package = a.get_package().get_name()
    if package.startswith("/Game/__ExternalActors__/"):
        old_files.append(os.path.join(ROOT, "Content", *package[len("/Game/"):].split("/")) + ".uasset")
    actor_subsystem.destroy_actor(a)
log(f"지난 TowerArt 액터 {len(old_files)} 지움")


def set_hidden(actor, hidden):
    for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
        comp.set_visibility(not hidden, True)
    actor.set_actor_hidden_in_game(hidden)
    tags = [t for t in actor.tags if str(t) != HIDDEN_TAG]
    if hidden:
        tags.append(unreal.Name(HIDDEN_TAG))
    actor.set_editor_property("tags", tags)


for a in [a for a in actors if a.actor_has_tag(HIDDEN_TAG)]:
    set_hidden(a, False)

FT_DIRS = ["/Game/Grimoire/Terrain/Dress", "/Game/Grimoire/Terrain"]


def foliage_types():
    out = {}
    for d in FT_DIRS:
        for path in eal.list_assets(d, recursive=False):
            asset = unreal.load_asset(path)
            if isinstance(asset, unreal.FoliageType_InstancedStaticMesh):
                mesh = asset.get_editor_property("mesh")
                if mesh:
                    out.setdefault(mesh.get_path_name(), []).append(asset)
    return out


def instances_of(mesh_path):
    """그 메시를 쓰는 모든 폴리지 인스턴스 월드 변환"""
    out = []
    for ifa in [a for a in actor_subsystem.get_all_level_actors() if isinstance(a, unreal.InstancedFoliageActor)]:
        for comp in ifa.get_components_by_class(unreal.InstancedStaticMeshComponent):
            m = comp.get_editor_property("static_mesh")
            if m and m.get_path_name() == mesh_path:
                out += [comp.get_instance_transform(i, True) for i in range(comp.get_instance_count())]
    return out


def to_json(t):
    r = t.rotation.rotator()
    return [t.translation.x, t.translation.y, t.translation.z, r.roll, r.pitch, r.yaw, t.scale3d.x, t.scale3d.y, t.scale3d.z]


def from_json(v):
    return unreal.Transform(unreal.Vector(v[0], v[1], v[2]), unreal.Rotator(v[3], v[4], v[5]), unreal.Vector(v[6], v[7], v[8]))


fts = foliage_types()
if os.path.exists(REMOVED_JSON):
    with open(REMOVED_JSON, encoding="utf-8") as f:
        prev = json.load(f).get("foliage", {})
    for ft_path, items in prev.items():
        ft = unreal.load_asset(ft_path)
        if ft and items:
            unreal.InstancedFoliageActor.add_instances(world, ft, [from_json(v) for v in items])
    log(f"지난번 뺀 폴리지 되돌림 {sum(len(v) for v in prev.values())}")

if REVERT:
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    for path in old_files:
        if os.path.exists(path):
            os.remove(path)
    if os.path.exists(REMOVED_JSON):
        os.remove(REMOVED_JSON)
    log("되돌림 끝")
    raise SystemExit(0)

# ───── 3. 기준 높이 · 자리 ─────
ground = by_label["Tut_Yard"].get_actor_location().z          # 탑 평지(표식은 땅 높이)
log(f"탑 평지 높이 {ground:.0f} cm")


def ground_at(x, y, fallback):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, fallback + 5000), unreal.Vector(x, y, fallback - 5000),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True)
    for hit in hits or []:
        t = hit.to_tuple()
        actor = t[9] if len(t) > 9 else None
        if actor and "Landscape" in actor.get_class().get_name():
            return t[4].z
    return fallback


# ───── 4. 숨기기 — 그레이박스 · 팩 소품(충돌은 남긴다) ─────
hidden_labels = []


def hide(actor):
    set_hidden(actor, True)
    hidden_labels.append(actor.get_actor_label())


HIDE_FOLDERS = ["TowerSite/Tower", "TowerSite/Meadow/Trees", "TowerSite/Meadow/Shelter"]
for a in actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    folder = str(a.get_folder_path())
    label = a.get_actor_label()
    if any(folder == f or folder.startswith(f + "/") for f in HIDE_FOLDERS) or label.startswith(("Yard_CampStone_", "Meadow_CandleStone_")):
        hide(a)
lantern_posts = sorted([a for a in actors if a.get_actor_label().startswith("Lantern_") and a.get_actor_label().endswith("_Post")], key=lambda a: a.get_actor_label())
for a in actors:
    label = a.get_actor_label()
    if isinstance(a, unreal.StaticMeshActor) and label.startswith("Lantern_") and "YardLanterns" in str(a.get_folder_path()):
        hide(a)
poi_spots = {}
for name in ("Poi_Yard_Brazier", "Poi_Yard_Cairn"):
    marker = next((a for a in actors if a.get_actor_label() == name and "Marker" in a.get_class().get_name()), None)
    if marker is None:
        raise RuntimeError(f"표식 없음: {name}")
    poi_spots[name] = marker.get_actor_location()
    for a in actors:
        folder = str(a.get_folder_path())
        if isinstance(a, unreal.StaticMeshActor) and (folder.endswith(f"/{name}") or f"/{name}/" in folder):
            hide(a)
log(f"숨김 {len(hidden_labels)}")


# ───── 5. 놓기 도우미 ─────
def mesh(name):
    m = unreal.load_asset(name if name.startswith("/") else MESH.format(name))
    if m is None:
        raise RuntimeError(f"메시 없음 {name} — tower_art_import.py 를 먼저")
    return m


def tripo_or_blender(base):
    """Tripo 결과(SM_<이름>)가 있으면 그것, 없으면 블렌더 대체(SM_<이름>_Blender)"""
    return base if eal.does_asset_exist(MESH.format(base)) else base + "_Blender"


def place(name, x, y, z, yaw=0.0, folder="Exterior", scale=(1.0, 1.0, 1.0), label=None, shadow=True):
    actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh(name))
    comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    comp.set_editor_property("cast_shadow", shadow)
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_label(label or f"TA_{name.split('/')[-1][3:]}_{counts.get(name, 0):02d}")
    actor.set_folder_path(f"TowerArt/{folder}")
    actor.set_editor_property("tags", [unreal.Name(TAG)])
    actor.set_editor_property("is_spatially_loaded", False)   # 탑은 먼 데서도 보이는 랜드마크 · 시작 자리 — 늘 로드(그레이박스와 같다)
    count(name)
    return actor


CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")


def blocker(label, x, y, z_bottom, sx, sy, sz, yaw=0.0):
    """숨긴 충돌 상자(cm). 새 메시엔 충돌이 없어서 막아야 하는 곳에만"""
    actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z_bottom + sz / 2), unreal.Rotator(0, 0, yaw))
    comp = actor.static_mesh_component
    comp.set_static_mesh(CUBE)
    comp.set_visibility(False, True)
    comp.set_editor_property("cast_shadow", False)
    actor.set_actor_hidden_in_game(True)
    actor.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    actor.set_actor_label(f"TA_Block_{label}")
    actor.set_folder_path("TowerArt/Collision")
    actor.set_editor_property("tags", [unreal.Name(TAG)])
    actor.set_editor_property("is_spatially_loaded", False)
    count("Blocker")
    return actor


def grass_exclusion(label, x, y, z, ex, ey, ez):
    cls = unreal.load_class(None, "/Script/Grimoire.GrimGrassExclusionVolume")
    actor = actor_subsystem.spawn_actor_from_class(cls, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 0))
    actor.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(ex, ey, ez))
    actor.set_actor_label(f"TA_NoGrass_{label}")
    actor.set_folder_path("TowerArt/Grass")
    actor.set_editor_property("tags", [unreal.Name(TAG)])
    actor.set_editor_property("is_spatially_loaded", False)
    count("GrassExclusion")


def light(label, x, y, z, intensity, radius, color=(1.0, 0.72, 0.45)):
    actor = actor_subsystem.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 0))
    comp = actor.get_component_by_class(unreal.PointLightComponent)
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_intensity(intensity)
    comp.set_attenuation_radius(radius)
    comp.set_light_color(unreal.LinearColor(color[0], color[1], color[2], 1.0))
    actor.set_actor_label(f"TA_Light_{label}")
    actor.set_folder_path("TowerArt/Lights")
    actor.set_editor_property("tags", [unreal.Name(TAG)])
    actor.set_editor_property("is_spatially_loaded", False)
    count("Light")


# ───── 6. 탑 실외(블렌더 +Y 는 언리얼 −Y 로 들어온다 — 좌우 대칭 조각은 그대로, 비대칭은 yaw 로 맞춘다) ─────
for name in ("SM_Tower_Drum", "SM_Tower_DrumRoof", "SM_Tower_Shaft", "SM_Tower_Observatory", "SM_Tower_Roof", "SM_Tower_Armillary", "SM_Tower_Porch", "SM_Tower_Vestibule"):
    place(name, 0, 0, ground, folder="Exterior/Tower")


def shaft_r(z_m):
    t = (z_m - 5.5) / (29.6 - 5.5)
    return 7.4 - 0.8 * max(0.0, min(1.0, t))


for a_deg, z_top, r in [(20, 5.25, 20.92), (-20, 5.25, 20.92), (22.5, 16.9, None), (202.5, 16.9, None), (112.5, 24.9, None), (292.5, 24.9, None)]:
    rr = (r if r else shaft_r(z_top - 1.5) + 0.08) * 100
    a = math.radians(a_deg)
    place("SM_Tower_Banner", rr * math.cos(a), rr * math.sin(a), ground + z_top * 100, yaw=a_deg - 90, folder="Exterior/Banners")

# 막는 상자: 둥근 기단(문 구멍 ±6.6° 비움) · 현관 옆벽 · 앞벽 · 통로 옆벽
R_mid, n_seg = 2043.0, 34
a0, a1 = math.radians(6.8), math.radians(353.2)
for k in range(n_seg):
    a = a0 + (a1 - a0) * (k + 0.5) / n_seg
    chord = 2 * R_mid * math.sin((a1 - a0) / n_seg / 2) + 12
    blocker(f"Drum_{k:02d}", R_mid * math.cos(a), R_mid * math.sin(a), ground, 80, chord, 620, yaw=math.degrees(a))
for s in (-1, 1):
    blocker(f"PorchSide_{'N' if s > 0 else 'S'}", 2185, s * 330, ground, 450, 60, 540)
    blocker(f"PorchFront_{'N' if s > 0 else 'S'}", 2430, s * 265, ground, 60, 190, 540)
    blocker(f"Vestibule_{'N' if s > 0 else 'S'}", 1820, s * 240, ground, 410, 40, 500)
light("Porch", 2200, 0, ground + 470, 8.0, 1000)
light("Vestibule", 1800, 0, ground + 420, 5.0, 700)

# ───── 7. 1층 실내 ─────
wall_seq = 0


def wall_modules(length):
    mods, rem = [], length
    while rem >= 399:
        mods.append(400)
        rem -= 400
    while rem >= 199:
        mods.append(200)
        rem -= 200
    while rem >= 99:
        mods.append(100)
        rem -= 100
    if rem > 35 or not mods:
        mods.append(100)
    return mods, length / sum(mods)


for a in actors:
    if not isinstance(a, unreal.StaticMeshActor) or str(a.get_folder_path()) != "TowerSite/Tower":
        continue
    label = a.get_actor_label()
    if not label.startswith("Wall_"):
        continue
    loc, sc = a.get_actor_location(), a.get_actor_scale3d()
    along_x = sc.x >= sc.y
    length = (sc.x if along_x else sc.y) * 100.0
    height = sc.z * 100.0
    yaw = 0.0 if along_x else 90.0
    if "_Lintel" in label:
        w = int(round(length / 100.0)) * 100
        place(f"SM_TowerIn_Lintel{w}", loc.x, loc.y, ground + 300, yaw=yaw, folder="Interior/Walls")
        place(f"SM_TowerIn_DoorFrame{w}", loc.x, loc.y, ground, yaw=yaw, folder="Interior/Walls")
        continue
    mods, f = wall_modules(length)
    start = -length / 2
    for m in mods:
        c = start + f * m / 2
        start += f * m
        name = {400: "SM_TowerIn_Wall400A" if wall_seq % 2 == 0 else "SM_TowerIn_Wall400B", 200: "SM_TowerIn_Wall200", 100: "SM_TowerIn_Wall100"}[m]
        wall_seq += 1
        x, y = (loc.x + c, loc.y) if along_x else (loc.x, loc.y + c)
        place(name, x, y, ground, yaw=yaw, folder="Interior/Walls", scale=(f, 1.0, height / 500.0))

for i in range(8):
    for j in range(6):
        x, y = -1400 + 400 * i, -1000 + 400 * j
        if j >= 4:
            place("SM_TowerIn_FloorWood", x, y, ground, folder="Interior/Floor")
        else:
            place("SM_TowerIn_FloorFlagA" if (i + j) % 2 else "SM_TowerIn_FloorFlagB", x, y, ground, yaw=90.0 * rng.randrange(4), folder="Interior/Floor")
        place("SM_TowerIn_Ceiling", x, y, ground, folder="Interior/Ceiling", shadow=True)


def at(label):
    return by_label[label].get_actor_location()


# 서고 책장: 그레이박스 셋(서벽, 앞 = 동) + 북벽 둘 · 남벽 셋(막는 상자 같이)
for i in range(3):
    p = at(f"Library_Shelf_{i}")
    place("SM_TowerIn_Bookcase", -1548, p.y, ground, yaw=0, folder="Interior/Library")
for k, (x, y, yaw) in enumerate([(-1350, 350, -90), (-650, 350, -90), (-1350, -350, 90), (-1000, -350, 90), (-650, -350, 90)]):
    place("SM_TowerIn_Bookcase", x, y, ground, yaw=yaw, folder="Interior/Library")
    blocker(f"Bookcase_{k}", x, y, ground, 175 if yaw else 60, 60 if yaw else 175, 270)
p = at("Kitchen_Hearth")
place("SM_TowerIn_Hearth", p.x, p.y, ground, yaw=180, folder="Interior/Kitchen")       # 갓(블렌더 +Y → 언리얼 −Y)을 북벽 쪽으로
p = at("Kitchen_Counter")
place("SM_TowerIn_Counter", p.x, p.y, ground, yaw=180, folder="Interior/Kitchen")      # 서랍(블렌더 −Y → 언리얼 +Y)을 방 쪽(−Y)으로
p = at("Start_Table")
place("SM_TowerIn_Desk", p.x, p.y, ground, folder="Interior/StartRoom")
p = at("Start_Mat")
place("SM_TowerIn_Rug", p.x, p.y, ground, folder="Interior/StartRoom")
p = at("Hall_Planter")
place("SM_TowerIn_Planter", p.x, p.y, ground, folder="Interior/Hall")
for label in ["Trial2_Pedestal_0", "Trial2_Pedestal_1", "Trial2_Pedestal_2", "Trial3_Pedestal"]:
    p = at(label)
    place("SM_TowerIn_Pedestal", p.x, p.y, ground, folder="Interior/TrialRoom")
place("SM_TowerIn_RuneCircle", 550, 0, ground, folder="Interior/Hall")
place("SM_TowerIn_Rug", 1150, 0, ground, yaw=0, folder="Interior/Hall", scale=(1.6, 1.4, 1.0))     # 동문 → 홀 길 깔개
for x, y in [(600, 372), (-150, 372), (0, -372), (1200, -372)]:
    place("SM_TowerIn_Banner", x, y, ground + 470, yaw=0, folder="Interior/Hall")

# ───── 8. 앞마당 ─────
COURT = (3560.0, 0.0)
place("SM_Tower_Court", COURT[0], COURT[1], ground, folder="Yard")
for a_deg in (60, -60, 120, -120):
    place("SM_Tower_FlowerBed", COURT[0], COURT[1], ground, yaw=a_deg, folder="Yard")
for k, (x, y) in enumerate([(2600, 500), (2600, -500), (4560, 450), (4560, -450)]):
    place("SM_Tower_OrbLamp", x, y, ground, folder="Yard/Lamps")
    blocker(f"OrbLamp_{k}", x, y, ground, 50, 50, 170)
    light(f"OrbLamp_{k}", x, y, ground + 160, 2.0, 600)
p = at("Shelter_Roof")
place("SM_Tower_Shelter", p.x, p.y, ground, folder="Yard")
shed_a = math.radians(25)
place("SM_Tower_WoodShed", 2195 * math.cos(shed_a), 2195 * math.sin(shed_a), ground, yaw=25, folder="Yard")
blocker("WoodShed", 2195 * math.cos(shed_a), 2195 * math.sin(shed_a), ground, 160, 360, 240, yaw=25)
for k, (x, y) in enumerate([(2245, 1250), (2315, 1185), (2170, 1180), (2160, -600), (2225, -665)]):
    place("SM_Tower_Barrel", x, y, ground, yaw=rng.uniform(0, 360), folder="Yard")
    blocker(f"Barrel_{k}", x, y, ground, 60, 60, 90)
well = tripo_or_blender("SM_Tower_Well")
place(well, 2450, -1600, ground, yaw=-30, folder="Yard")
blocker("Well", 2450, -1600, ground, 190, 190, 100, yaw=-30)
# 숨긴 모닥불 돌 · 양초 받침 자리
ROCK = "/Game/Grimoire/Environment/Rocks/SM_Rock_MeadowA/StaticMeshes/SM_Rock_MeadowA"     # 첫 자체 제작 에셋(초원 바위 A, Tripo)
if eal.does_asset_exist(ROCK):
    bb = unreal.load_asset(ROCK).get_bounding_box()
    k = 55.0 / max(bb.max.x - bb.min.x, bb.max.y - bb.min.y, 1.0)          # 모닥불 돌 폭 약 55 cm
    for i in range(3):
        p = at(f"Yard_CampStone_{i}")
        place(ROCK, p.x, p.y, ground, yaw=rng.uniform(0, 360), folder="Yard", scale=(k, k, k * 0.8), label=f"TA_CampStone_{i}")
for i in range(2):
    p = at(f"Meadow_CandleStone_{i}")
    place("SM_TowerIn_Pedestal", p.x, p.y, ground, folder="Yard", scale=(1.0, 1.0, 0.75))

# 길 등불 줄(콘셉트 tower-yard 석등) — 숨긴 회색 상자 자리마다
lantern = tripo_or_blender("SM_Tower_StoneLantern")
spots = []
for post in lantern_posts:
    p = post.get_actor_location()
    g = p.z - 32.5            # section_dress: 기둥 상자 가운데 = 땅 − 0.3 m + 1.25 m / 2
    place(lantern, p.x, p.y, g, yaw=post.get_actor_rotation().yaw, folder="Yard/Lanterns")
    blocker(f"Lantern_{len(spots)}", p.x, p.y, g, 55, 55, 180, yaw=post.get_actor_rotation().yaw)
    spots.append((p.x, p.y))
# 앞마당 POI: 꺼진 화로 제단 · 길손의 돌무더기
for name, mesh_base, yaw, size in (("Poi_Yard_Brazier", "SM_Tower_Brazier", 200, (240, 240, 100)), ("Poi_Yard_Cairn", "SM_Tower_Cairn", 250, (110, 110, 100))):
    p = poi_spots[name]
    g = ground_at(p.x, p.y, p.z)
    place(tripo_or_blender(mesh_base), p.x, p.y, g, yaw=yaw, folder="Yard/POI")
    blocker(name, p.x, p.y, g, *size, yaw=yaw)
    spots.append((p.x, p.y))
    log(f"{name}: ({p.x:.0f}, {p.y:.0f}) 땅 {g:.0f} (표식 {p.z:.0f})")

# 풀 막기: 1층 · 통로 · 현관 · 마당
grass_exclusion("Interior", 0, 0, ground + 250, 1660, 1260, 500)
grass_exclusion("PorchVestibule", 2150, 0, ground + 250, 520, 390, 500)
grass_exclusion("Court", COURT[0], COURT[1], ground + 100, 900, 900, 300)

# ───── 9. 팩 폴리지 인스턴스 빼기(탑 둘레 · 새 소품 자리) ─────
removed = {}


def in_zone(t):
    x, y = t.translation.x, t.translation.y
    if math.hypot(x, y) < ZONE_R:
        return True
    return any(math.hypot(x - sx, y - sy) < SPOT_R for sx, sy in spots)


skipped_shared = []
for mesh_path, types in fts.items():
    if "/Grimoire/" in mesh_path:
        continue          # 우리 메시는 팩이 아니다
    if len(types) > 1:
        # 한 메시를 폴리지 타입 여럿이 쓰면 인스턴스가 어느 타입 것인지 파이썬에서 가를 수 없다(IFA 에 타입별 조회가 없다).
        # 지우고 다시 넣으면 다른 타입 인스턴스가 첫 타입으로 복사돼 돌릴 때마다 불어난다(2026-11-18) — 건드리지 않는다
        skipped_shared.append(f"{mesh_path.split('.')[-1]}({', '.join(t.get_name() for t in types)})")
        continue
    all_t = instances_of(mesh_path)
    gone = [t for t in all_t if in_zone(t)]
    if not gone:
        continue
    keep = [t for t in all_t if not in_zone(t)]
    ft = types[0]
    unreal.InstancedFoliageActor.remove_all_instances(world, ft)
    if keep:
        unreal.InstancedFoliageActor.add_instances(world, ft, keep)
    removed[ft.get_path_name()] = [to_json(t) for t in gone]
    log(f"폴리지 {ft.get_name()} ({mesh_path.split('.')[-1]}): {len(gone)} 뺌, {len(keep)} 남김")

if skipped_shared:
    log("폴리지 타입 여럿이 쓰는 메시는 두었다: " + "; ".join(skipped_shared))

with open(REMOVED_JSON, "w", encoding="utf-8") as f:
    json.dump({"_comment": "tower_art_dress.py 가 탑 구역에서 숨긴 액터 · 뺀 팩 폴리지 인스턴스(되돌리기: GRIM_TOWER_ART_REVERT=1)",
               "hidden": sorted(hidden_labels), "foliage": removed, "foliage_skipped_shared_mesh": skipped_shared}, f, ensure_ascii=False, indent=1)

# ───── 10. 저장 ─────
if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
    raise RuntimeError("저장 실패")
gone_files = 0
for path in old_files:
    if os.path.exists(path):
        os.remove(path)
        gone_files += 1
log(f"지난 외부 액터 파일 {gone_files} 지움")
log("개수 " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))
log("끝")
