# M2 갈래 T: 마법탑 튜토리얼 맵 L_Tower 를 그레이박스(엔진 기본 도형)로 새로 만든다.
# 돌릴 때마다 맵을 지우고 처음부터 다시 만든다 — 다시 돌려도 같은 결과(손으로 놓은 액터가 있으면 돌리지 않는다).
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Tutorial\build_tower_map.py" -unattended -nop4 -nosplash -nullrhi
#
# 좌표(cm): +X = 동쪽(탑 문 · 초원), +Y = 북쪽, 탑 1층 = X −1600~1600 · Y −1200~1200, 벽 높이 5m · 지붕은 그 위.
#   북: 시작 방(X −400~600) · 숙소(북서, 침대) · 서고(북동)   가운데: 홀(스승 · 게시판) · 서고(서)   남: 시험의 방
# 퀘스트 표(Content/Grimoire/Data/Quest*.csv)는 액터 태그로 사물을 찾는다 — 태그 이름을 바꾸면 표도 고친다.
import unreal

MAP_PATH = "/Game/Grimoire/Maps/L_Tower"
SPELL_DIR = "/Game/Grimoire/Magic/Spells"

H = 500.0        # 벽 높이
T = 40.0         # 벽 두께
DOOR_H = 300.0   # 문 높이(위는 상인방)

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary

if eal.does_asset_exist(MAP_PATH):
    eal.delete_asset(MAP_PATH)
if not level_subsystem.new_level(MAP_PATH):
    raise RuntimeError(f"new_level 실패: {MAP_PATH}")

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder")
CONE = unreal.load_asset("/Engine/BasicShapes/Cone")
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane")
GRID = unreal.load_asset("/Engine/EngineMaterials/WorldGridMaterial")


def tag(name):
    result = unreal.GrimSpellDefinition.request_tag_by_name(name)
    if name not in result.export_text():
        raise RuntimeError("GameplayTag 없음: " + name)
    return result


def load_class(path):
    cls = unreal.load_class(None, path)
    if cls is None:
        raise RuntimeError(f"클래스 없음: {path}")
    return cls


def spawn(actor_class, x, y, z=0.0, yaw=0.0, label=None, folder=None, tags=()):
    if isinstance(actor_class, str):
        actor_class = load_class(actor_class)
    actor = actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    if tags:
        actor.set_editor_property("tags", [unreal.Name(t) for t in tags])
    return actor


def shape(mesh, label, cx, cy, cz, sx, sy, sz, folder="Tower", yaw=0.0):
    """기본 도형(100cm)을 크기(cm) sx·sy·sz 로. 위치는 가운데."""
    actor = spawn(unreal.StaticMeshActor, cx, cy, cz, yaw=yaw, label=label, folder=folder)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_material(0, GRID)
    actor.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    return actor


def box(label, cx, cy, cz, sx, sy, sz, folder="Tower"):
    return shape(CUBE, label, cx, cy, cz, sx, sy, sz, folder)


def segments(a, b, gaps):
    """a~b 를 gaps(문)로 자른 조각과 문 목록"""
    pieces, cursor = [], a
    for g0, g1 in sorted(gaps):
        if g0 > cursor:
            pieces.append((cursor, g0))
        cursor = max(cursor, g1)
    if cursor < b:
        pieces.append((cursor, b))
    return pieces


def wall_x(label, y, x0, x1, gaps=()):
    """Y 가 고정된 벽(동서로 뻗음)"""
    for i, (a, b) in enumerate(segments(x0, x1, gaps)):
        box(f"{label}_{i}", (a + b) / 2, y, H / 2, b - a, T, H)
    for i, (g0, g1) in enumerate(gaps):
        box(f"{label}_Lintel{i}", (g0 + g1) / 2, y, (DOOR_H + H) / 2, g1 - g0, T, H - DOOR_H)


def wall_y(label, x, y0, y1, gaps=()):
    """X 가 고정된 벽(남북으로 뻗음)"""
    for i, (a, b) in enumerate(segments(y0, y1, gaps)):
        box(f"{label}_{i}", x, (a + b) / 2, H / 2, T, b - a, H)
    for i, (g0, g1) in enumerate(gaps):
        box(f"{label}_Lintel{i}", x, (g0 + g1) / 2, (DOOR_H + H) / 2, T, g1 - g0, H - DOOR_H)


def point_light(label, x, y, z=400.0, intensity=40.0, radius=1600.0):
    light = spawn(unreal.PointLight, x, y, z, label=label, folder="Tower/Lights")
    component = light.get_component_by_class(unreal.PointLightComponent)
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_intensity(intensity)
    component.set_attenuation_radius(radius)
    return light


def marker(label, x, y, z=0.0):
    return spawn(unreal.TargetPoint, x, y, z, label=label, folder="Markers", tags=[label])


# ───── 땅 · 하늘 · 기후 ─────
ground = spawn(unreal.StaticMeshActor, 2000, 0, 0, label="Ground", folder="Meadow")
ground.static_mesh_component.set_static_mesh(PLANE)
ground.static_mesh_component.set_material(0, GRID)
ground.set_actor_scale3d(unreal.Vector(300, 300, 1))

spawn("/Script/Grimoire.GrimSkyController", 0, 0, 500, label="Sky", folder="Environment")

meadow = spawn("/Script/Grimoire.GrimClimateRegionVolume", 2000, 0, 0, label="Climate_Meadow", folder="Environment")
meadow.set_editor_property("region_id", "Meadow")
meadow.set_editor_property("blend_distance", 1000.0)
meadow.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(12000, 12000, 3000))

# ───── 탑 1층 ─────
X0, X1, Y0, Y1 = -1600.0, 1600.0, -1200.0, 1200.0
wall_x("Wall_North", Y1, X0 - T / 2, X1 + T / 2)
wall_x("Wall_South", Y0, X0 - T / 2, X1 + T / 2)
wall_y("Wall_West", X0, Y0, Y1)
wall_y("Wall_East", X1, Y0, Y1, gaps=[(-200, 200)])                          # 탑 문(동)
wall_x("Wall_HallNorth", 400, X0, X1, gaps=[(-1100, -900), (50, 250), (1000, 1200)])  # 숙소↔서고(서) · 시작 방 · 서고(북동)
wall_x("Wall_HallSouth", -400, X0, X1, gaps=[(500, 800)])                   # 시험의 방 문
wall_y("Wall_LibraryEast", -400, -400, 400, gaps=[(-100, 100)])             # 서고(서) ↔ 홀
wall_y("Wall_StartWest", -400, 400, Y1)                                     # 숙소 | 시작 방
wall_y("Wall_StartEast", 600, 400, Y1)                                      # 시작 방 | 서고(북동)

box("Roof", 0, 0, H + 15, X1 - X0 + 2 * T + 40, Y1 - Y0 + 2 * T + 40, 30)
shape(CYLINDER, "TowerSpire", 0, 0, H + 30 + 1500, 2000, 2000, 3000)
shape(CONE, "TowerSpireTop", 0, 0, H + 30 + 3000 + 400, 2200, 2200, 800)

# 시작 방: 깨어나는 자리(장식 깔개) · 탁자 위 양초 · PlayerStart
box("Start_Mat", -150, 1000, 5, 200, 100, 10, folder="Tower/StartRoom")
box("Start_Table", 300, 1000, 40, 120, 70, 80, folder="Tower/StartRoom")
spawn("/Script/Grimoire.GrimCandle", 300, 1000, 80, label="Tut_Candle", folder="Tower/StartRoom", tags=["Tut_Candle"])
spawn(unreal.PlayerStart, -150, 800, 100, yaw=-50, label="PlayerStart", folder="Tower/StartRoom")
point_light("Light_StartRoom", 100, 800)

# 숙소(북서): 침대 둘
for i, x in enumerate([-1300, -800]):
    spawn("/Script/Grimoire.GrimBed", x, 1000, 0, label=f"Tut_Bed_{i}", folder="Tower/Dormitory", tags=["Tut_Bed"])
point_light("Light_Dormitory", -1000, 800)

# 서고(서 · 북동): 책장
for i, y in enumerate([-250, 0, 250]):
    box(f"Library_Shelf_{i}", -1520, y, 125, 60, 160, 250, folder="Tower/Library")
for i, x in enumerate([800, 1100, 1400]):
    box(f"Study_Shelf_{i}", x, 1120, 125, 160, 60, 250, folder="Tower/Library")
point_light("Light_Library", -1000, 0)
point_light("Light_Study", 1100, 800)

# 홀: 스승 · 게시판
spawn("/Script/Grimoire.GrimTowerNPC", 300, 0, 92, yaw=90, label="Npc_Master", folder="Tower/Hall", tags=["Npc_Master"])
spawn("/Script/Grimoire.GrimQuestBoard", 1100, 330, 100, yaw=-90, label="QuestBoard", folder="Tower/Hall", tags=["QuestBoard"])
point_light("Light_Hall", 600, 0)

# 시험의 방(남): 2서클 — 양초 셋(3m 간격, 번지지 않게), 3서클 — 움직이는 표적 + 젖은 양초
marker("Trial_Room", 650, -800)
for i, x in enumerate([-1300, -1000, -700]):
    box(f"Trial2_Pedestal_{i}", x, -800, 40, 40, 40, 80, folder="Tower/TrialRoom")
    spawn("/Script/Grimoire.GrimCandle", x, -800, 80, label=f"Trial2_Candle_{i}", folder="Tower/TrialRoom", tags=["Trial2_Candle", "Trial2_Prop"])
box("Trial3_Pedestal", -200, -1000, 40, 40, 40, 80, folder="Tower/TrialRoom")
wet = spawn("/Script/Grimoire.GrimCandle", -200, -1000, 80, label="Trial3_WetCandle", folder="Tower/TrialRoom", tags=["Trial3_WetCandle", "Trial3_Prop"])
wet.get_component_by_class(unreal.GrimElementalStateComponent).set_editor_property("initial_states", {tag("State.Wet"): 2.0})
target = spawn("/Script/Grimoire.GrimTrialTarget", 1200, -800, 150, label="Trial3_Target", folder="Tower/TrialRoom", tags=["Trial3_Target", "Trial3_Prop"])
target.set_editor_property("move_amplitude", unreal.Vector(0, 300, 0))
target.set_editor_property("move_period_seconds", 4.0)
point_light("Light_TrialRoom", 0, -800, intensity=60.0, radius=2200.0)

# ───── 탑 밖 초원(동) ─────
marker("Tut_Exit", 2200, 0)
spawn("/Script/Grimoire.GrimCampfire", 3000, 400, 0, label="Tut_Campfire", folder="Meadow", tags=["Tut_Campfire"])
for i in range(5):
    spawn("/Script/Grimoire.GrimDryGrass", 3600, -300 - i * 100, 0, label=f"Meadow_DryGrass_{i}", folder="Meadow", tags=["Meadow_DryGrass"])
for i, y in enumerate([1000, 1300]):
    box(f"Meadow_CandleStone_{i}", 2600, y, 30, 40, 40, 60, folder="Meadow")
    spawn("/Script/Grimoire.GrimCandle", 2600, y, 60, label=f"Meadow_Candle_{i}", folder="Meadow", tags=["Meadow_Candle"])

# 지붕 있는 쉼터 + 침낭(밖에서도 잘 수 있다 — 스펙 「탑 튜토리얼에서 침낭 하나」)
SX, SY = 4000.0, 1500.0
for i, (dx, dy) in enumerate([(-180, -180), (180, -180), (-180, 180), (180, 180)]):
    box(f"Shelter_Post_{i}", SX + dx, SY + dy, 150, 20, 20, 300, folder="Meadow/Shelter")
box("Shelter_Roof", SX, SY, 310, 420, 420, 20, folder="Meadow/Shelter")
bedroll = load_class("/Script/Grimoire.GrimBed")
roll = spawn(bedroll, SX, SY, 0, label="Shelter_Bedroll", folder="Meadow/Shelter", tags=["Shelter_Bedroll"])
roll.set_editor_property("bedroll", True)

# 화염구 스킬북: 남동쪽 바위 위(2서클이 되어야 읽힌다)
box("Skillbook_Rock", 5500, -2500, 30, 160, 160, 60, folder="Meadow")
book = spawn("/Script/Grimoire.GrimSkillbook", 5500, -2500, 90, label="Tut_Skillbook", folder="Meadow", tags=["Tut_Skillbook"])
book.set_editor_property("teaches_spell", unreal.load_asset(f"{SPELL_DIR}/DA_Spell_Fireball"))

# 나무(원통 + 원뿔)
for i, (x, y) in enumerate([(2500, -1800), (3500, 2500), (4500, -800), (6000, 1000), (5000, 2800), (-2500, 2500), (-2600, -2200), (1000, 2800), (1200, -2900), (7000, -1500)]):
    shape(CYLINDER, f"Tree_{i}_Trunk", x, y, 150, 40, 40, 300, folder="Meadow/Trees")
    shape(CONE, f"Tree_{i}_Crown", x, y, 450, 260, 260, 400, folder="Meadow/Trees")

if not level_subsystem.save_current_level():
    raise RuntimeError("save_current_level 실패")

unreal.log(f"build_tower_map: saved {MAP_PATH}")
