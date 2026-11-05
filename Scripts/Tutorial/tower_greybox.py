# 마법탑 그레이박스(엔진 기본 도형)를 짓는 재사용 함수 — build_tower_map.py(L_Tower)와
# Scripts/Terrain/build_openbeta.py(L_OpenBeta 의 탑 자리)가 같이 쓴다.
# M2 갈래 T 의 build_tower_map.py 에 있던 배치를 그대로 옮겼다(M3 갈래 W). 좌표 · 크기 · 태그는 바꾸지 않았다.
#
# 탑 기준 좌표(cm): +X = 동쪽(탑 문 · 앞마당), +Y = 북쪽, 탑 1층 = X −1600~1600 · Y −1200~1200, 벽 높이 5m · 지붕은 그 위.
#   북: 시작 방(X −400~600) · 숙소(북서, 침대) · 주방(북동, 아궁이)   가운데: 홀(스승 · 게시판 · 화단) · 서고(서)   남: 수련실 · 시험의 방
#   탑 실내(북 · 가운데)는 전투 금지 볼륨(Zone.NoCombat), 수련실과 탑 밖(앞마당)은 전투 가능.
#   밖(동): 앞마당(생물 자리 · 모닥불 · 침낭 자리) · 초원 · 거점 세울 곳(북동) · 멀리 사냥꾼 거점(남동, 퀘스트 발동 볼륨)
# TowerBuilder(origin, yaw, ground) 로 탑 기준 좌표를 월드로 옮긴다: 돌린 뒤(yaw) 옮기고(origin), 높이는 ground(월드 X, Y) + z.
# 기본값(원점 · 0도 · 땅 높이 0)이면 L_Tower 와 똑같은 좌표다.
# 퀘스트 표(Content/Grimoire/Data/Quest*.csv)는 액터 태그로 사물을 찾는다 — 태그 이름을 바꾸면 표도 고친다.
import math

import unreal

H = 500.0        # 벽 높이
T = 40.0         # 벽 두께
DOOR_H = 300.0   # 문 높이(위는 상인방)


def _load_class(path):
    cls = unreal.load_class(None, path)
    if cls is None:
        raise RuntimeError(f"클래스 없음: {path}")
    return cls


def tag(name):
    result = unreal.GrimSpellDefinition.request_tag_by_name(name)
    if name not in result.export_text():
        raise RuntimeError("GameplayTag 없음: " + name)
    return result


class TowerBuilder:
    """탑 기준 좌표(cm)로 액터를 놓는다. origin = 탑 가운데의 월드 위치(X, Y), yaw = 탑을 돌린 각도(도),
    ground(x, y) = 그 월드 XY 의 땅 높이(cm). folder_root 를 주면 모든 폴더 앞에 붙인다."""

    def __init__(self, origin=(0.0, 0.0), yaw=0.0, ground=None, folder_root=""):
        self.actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.origin = origin
        self.yaw = yaw
        self.ground = ground
        self.folder_root = folder_root
        self.spawned = []
        self.cube = unreal.load_asset("/Engine/BasicShapes/Cube")
        self.cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder")
        self.cone = unreal.load_asset("/Engine/BasicShapes/Cone")
        self.plane = unreal.load_asset("/Engine/BasicShapes/Plane")
        self.grid = unreal.load_asset("/Engine/EngineMaterials/WorldGridMaterial")

    def to_world(self, x, y, z):
        if self.yaw == 0.0 and self.origin == (0.0, 0.0) and self.ground is None:
            return x, y, z
        c, s = math.cos(math.radians(self.yaw)), math.sin(math.radians(self.yaw))
        wx = self.origin[0] + x * c - y * s
        wy = self.origin[1] + x * s + y * c
        base = self.ground(wx, wy) if self.ground else 0.0
        return wx, wy, base + z

    def folder(self, name):
        if not name:
            return name
        return f"{self.folder_root}/{name}" if self.folder_root else name

    def spawn(self, actor_class, x, y, z=0.0, yaw=0.0, label=None, folder=None, tags=()):
        if isinstance(actor_class, str):
            actor_class = _load_class(actor_class)
        wx, wy, wz = self.to_world(x, y, z)
        actor = self.actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(wx, wy, wz), unreal.Rotator(0, 0, yaw + self.yaw))
        if label:
            actor.set_actor_label(label)
        if folder:
            actor.set_folder_path(self.folder(folder))
        if tags:
            actor.set_editor_property("tags", [unreal.Name(t) for t in tags])
        self.spawned.append(actor)
        return actor

    def shape(self, mesh, label, cx, cy, cz, sx, sy, sz, folder="Tower", yaw=0.0):
        """기본 도형(100cm)을 크기(cm) sx·sy·sz 로. 위치는 가운데."""
        actor = self.spawn(unreal.StaticMeshActor, cx, cy, cz, yaw=yaw, label=label, folder=folder)
        component = actor.static_mesh_component
        component.set_static_mesh(mesh)
        component.set_material(0, self.grid)
        actor.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
        return actor

    def box(self, label, cx, cy, cz, sx, sy, sz, folder="Tower"):
        return self.shape(self.cube, label, cx, cy, cz, sx, sy, sz, folder)

    def wall_x(self, label, y, x0, x1, gaps=()):
        """Y 가 고정된 벽(동서로 뻗음)"""
        for i, (a, b) in enumerate(segments(x0, x1, gaps)):
            self.box(f"{label}_{i}", (a + b) / 2, y, H / 2, b - a, T, H)
        for i, (g0, g1) in enumerate(gaps):
            self.box(f"{label}_Lintel{i}", (g0 + g1) / 2, y, (DOOR_H + H) / 2, g1 - g0, T, H - DOOR_H)

    def wall_y(self, label, x, y0, y1, gaps=()):
        """X 가 고정된 벽(남북으로 뻗음)"""
        for i, (a, b) in enumerate(segments(y0, y1, gaps)):
            self.box(f"{label}_{i}", x, (a + b) / 2, H / 2, T, b - a, H)
        for i, (g0, g1) in enumerate(gaps):
            self.box(f"{label}_Lintel{i}", x, (g0 + g1) / 2, (DOOR_H + H) / 2, T, g1 - g0, H - DOOR_H)

    def point_light(self, label, x, y, z=400.0, intensity=40.0, radius=1600.0):
        light = self.spawn(unreal.PointLight, x, y, z, label=label, folder="Tower/Lights")
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_intensity(intensity)
        component.set_attenuation_radius(radius)
        return light

    def marker(self, label, x, y, z=0.0):
        return self.spawn(unreal.TargetPoint, x, y, z, label=label, folder="Markers", tags=[label])


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


def build_ground_and_sky(b):
    """L_Tower 전용: 평평한 땅 · 하늘 · 기후 볼륨(Meadow). 지형 맵은 랜드스케이프 · 자기 하늘 · 기후를 따로 놓는다."""
    ground = b.spawn(unreal.StaticMeshActor, 2000, 0, 0, label="Ground", folder="Meadow")
    ground.static_mesh_component.set_static_mesh(b.plane)
    ground.static_mesh_component.set_material(0, b.grid)
    ground.set_actor_scale3d(unreal.Vector(300, 300, 1))

    b.spawn("/Script/Grimoire.GrimSkyController", 0, 0, 500, label="Sky", folder="Environment")

    meadow = b.spawn("/Script/Grimoire.GrimClimateRegionVolume", 2000, 0, 0, label="Climate_Meadow", folder="Environment")
    meadow.set_editor_property("region_id", "Meadow")
    meadow.set_editor_property("blend_distance", 1000.0)
    meadow.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(12000, 12000, 3000))


def build_tower(b):
    """탑 1층 · 방 · NPC · 시험의 방(탑 기준 좌표 안쪽)"""
    X0, X1, Y0, Y1 = -1600.0, 1600.0, -1200.0, 1200.0
    b.wall_x("Wall_North", Y1, X0 - T / 2, X1 + T / 2)
    b.wall_x("Wall_South", Y0, X0 - T / 2, X1 + T / 2)
    b.wall_y("Wall_West", X0, Y0, Y1)
    b.wall_y("Wall_East", X1, Y0, Y1, gaps=[(-200, 200)])                          # 탑 문(동)
    b.wall_x("Wall_HallNorth", 400, X0, X1, gaps=[(-1100, -900), (50, 250), (1000, 1200)])  # 숙소↔서고(서) · 시작 방 · 주방
    b.wall_x("Wall_HallSouth", -400, X0, X1, gaps=[(500, 800)])                   # 시험의 방 문
    b.wall_y("Wall_LibraryEast", -400, -400, 400, gaps=[(-100, 100)])             # 서고(서) ↔ 홀
    b.wall_y("Wall_StartWest", -400, 400, Y1)                                     # 숙소 | 시작 방
    b.wall_y("Wall_StartEast", 600, 400, Y1)                                      # 시작 방 | 주방

    b.box("Roof", 0, 0, H + 15, X1 - X0 + 2 * T + 40, Y1 - Y0 + 2 * T + 40, 30)
    b.shape(b.cylinder, "TowerSpire", 0, 0, H + 30 + 1500, 2000, 2000, 3000)
    b.shape(b.cone, "TowerSpireTop", 0, 0, H + 30 + 3000 + 400, 2200, 2200, 800)

    # 시작 방: 깨어나는 자리(장식 깔개) · 탁자 위 양초 · PlayerStart
    b.box("Start_Mat", -150, 1000, 5, 200, 100, 10, folder="Tower/StartRoom")
    b.box("Start_Table", 300, 1000, 40, 120, 70, 80, folder="Tower/StartRoom")
    b.spawn("/Script/Grimoire.GrimCandle", 300, 1000, 80, label="Tut_Candle", folder="Tower/StartRoom", tags=["Tut_Candle"])
    b.spawn(unreal.PlayerStart, -150, 800, 100, yaw=-50, label="PlayerStart", folder="Tower/StartRoom")
    b.point_light("Light_StartRoom", 100, 800)

    # 숙소(북서): 침대 둘
    for i, x in enumerate([-1300, -800]):
        b.spawn("/Script/Grimoire.GrimBed", x, 1000, 0, label=f"Tut_Bed_{i}", folder="Tower/Dormitory", tags=["Tut_Bed"])
    b.point_light("Light_Dormitory", -1000, 800)

    # 서고(서): 책장
    for i, y in enumerate([-250, 0, 250]):
        b.box(f"Library_Shelf_{i}", -1520, y, 125, 60, 160, 250, folder="Tower/Library")
    b.point_light("Light_Library", -1000, 0)

    # 주방(북동): 아궁이(모닥불 액터 — 플라미스로 켠다) · 조리대. 요리(M4)는 이벤트로.
    b.marker("Tut_Kitchen", 1100, 800)
    b.box("Kitchen_Hearth", 1300, 1050, 25, 200, 140, 50, folder="Tower/Kitchen")
    b.spawn("/Script/Grimoire.GrimCampfire", 1300, 1050, 50, label="Tut_Stove", folder="Tower/Kitchen", tags=["Tut_Stove"])
    # 아궁이 위 냄비(M3 갈래 I — 요리 시설). 원점 = 냄비 바닥, 결과(스튜)는 주방 쪽(+X 앞) 아궁이 위에 놓인다.
    b.spawn("/Script/Grimoire.GrimCookingPot", 1300, 1050, 105, yaw=-128.7, label="Tut_CookingPot", folder="Tower/Kitchen", tags=["Tut_CookingPot"])
    b.box("Kitchen_Counter", 800, 1120, 45, 260, 70, 90, folder="Tower/Kitchen")
    b.point_light("Light_Kitchen", 1100, 800)

    # 홀: 스승 · 게시판
    b.spawn("/Script/Grimoire.GrimTowerNPC", 300, 0, 92, yaw=90, label="Npc_Master", folder="Tower/Hall", tags=["Npc_Master"])
    b.spawn("/Script/Grimoire.GrimQuestBoard", 1100, 330, 100, yaw=-90, label="QuestBoard", folder="Tower/Hall", tags=["QuestBoard"])
    # 홀 화단: 작물 자리 셋(아쿠엘로 물 주기 — 작물 M4 전 임시)
    b.box("Hall_Planter", 1050, -300, 15, 520, 120, 30, folder="Tower/Hall")
    for i, x in enumerate([880, 1050, 1220]):
        b.spawn("/Script/Grimoire.GrimCropPlot", x, -300, 30, label=f"Tut_Crop_{i}", folder="Tower/Hall", tags=["Tut_Crop"])
    b.point_light("Light_Hall", 600, 0)

    # 전투 금지(중립 지대): 북쪽 방들 · 홀과 서고. 수련실(남)은 뺀다.
    for label, cy in [("NoCombat_North", 800), ("NoCombat_Middle", 0)]:
        zone = b.spawn("/Script/Grimoire.GrimNoCombatVolume", 0, cy, 250, label=label, folder="Tower/Zones")
        zone.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(1600, 400, 300))

    # 수련실(남, 시험의 방과 같은 방): 기본 공격으로 부술 과녁(세 번 맞으면 부서짐, 튜토리얼 단계에서만 켜짐)
    training = b.spawn("/Script/Grimoire.GrimTrialTarget", 300, -1000, 120, label="Tut_TrainingTarget", folder="Tower/TrialRoom", tags=["Tut_TrainingTarget", "Tut_TrainingProp"])
    training.set_editor_property("move_amplitude", unreal.Vector(0, 0, 0))
    training.set_editor_property("hits_to_break", 3)

    # 시험의 방(남): 2서클 — 양초 셋(3m 간격, 번지지 않게), 3서클 — 움직이는 표적 + 젖은 양초
    b.marker("Trial_Room", 650, -800)
    for i, x in enumerate([-1300, -1000, -700]):
        b.box(f"Trial2_Pedestal_{i}", x, -800, 40, 40, 40, 80, folder="Tower/TrialRoom")
        b.spawn("/Script/Grimoire.GrimCandle", x, -800, 80, label=f"Trial2_Candle_{i}", folder="Tower/TrialRoom", tags=["Trial2_Candle", "Trial2_Prop"])
    b.box("Trial3_Pedestal", -200, -1000, 40, 40, 40, 80, folder="Tower/TrialRoom")
    wet = b.spawn("/Script/Grimoire.GrimCandle", -200, -1000, 80, label="Trial3_WetCandle", folder="Tower/TrialRoom", tags=["Trial3_WetCandle", "Trial3_Prop"])
    wet.get_component_by_class(unreal.GrimElementalStateComponent).set_editor_property("initial_states", {tag("State.Wet"): 2.0})
    target = b.spawn("/Script/Grimoire.GrimTrialTarget", 1200, -800, 150, label="Trial3_Target", folder="Tower/TrialRoom", tags=["Trial3_Target", "Trial3_Prop"])
    # 움직이는 표적의 진폭은 표적 액터 기준(로컬)이 아니라 월드 벡터 — 탑을 돌리면 같이 돌린다
    target.set_editor_property("move_amplitude", _rotate_vector(b, unreal.Vector(0, 300, 0)))
    target.set_editor_property("move_period_seconds", 4.0)
    b.point_light("Light_TrialRoom", 0, -800, intensity=60.0, radius=2200.0)


def build_yard(b, include_trees=True):
    """탑 밖: 앞마당(동) · 초원 소품 · 쉼터 · 거점 자리 · 사냥꾼 거점 · 나무"""
    b.marker("Tut_Yard", 2600, 0)
    b.marker("Tut_CreatureSpot", 3300, -700)      # 소형 생물이 나올 자리(생물 M3)
    b.spawn("/Script/Grimoire.GrimCampfire", 3000, 400, 0, label="Tut_Campfire", folder="Meadow/Yard", tags=["Tut_Campfire"])
    b.marker("Tut_CampSite", 3000, 650)            # 튜토리얼이 침낭을 놓는 곳(모닥불 곁)
    for i, (x, y) in enumerate([(2900, 280), (3120, 300), (2950, 520)]):
        b.box(f"Yard_CampStone_{i}", x, y, 15, 50, 50, 30, folder="Meadow/Yard")
    for i in range(5):
        b.spawn("/Script/Grimoire.GrimDryGrass", 3600, -300 - i * 100, 0, label=f"Meadow_DryGrass_{i}", folder="Meadow", tags=["Meadow_DryGrass"])
    for i, y in enumerate([1000, 1300]):
        b.box(f"Meadow_CandleStone_{i}", 2600, y, 30, 40, 40, 60, folder="Meadow")
        b.spawn("/Script/Grimoire.GrimCandle", 2600, y, 60, label=f"Meadow_Candle_{i}", folder="Meadow", tags=["Meadow_Candle"])

    # 지붕 있는 쉼터 + 침낭(밖에서도 잘 수 있다 — 스펙 「탑 튜토리얼에서 침낭 하나」)
    SX, SY = 4000.0, 1500.0
    for i, (dx, dy) in enumerate([(-180, -180), (180, -180), (-180, 180), (180, 180)]):
        b.box(f"Shelter_Post_{i}", SX + dx, SY + dy, 150, 20, 20, 300, folder="Meadow/Shelter")
    b.box("Shelter_Roof", SX, SY, 310, 420, 420, 20, folder="Meadow/Shelter")

    # 거점 세울 곳(북동): 돌판. 거점 핵 · 건축은 M4 — 지금은 자리만.
    b.marker("Tut_BaseSite", 5500, 2500)
    b.box("BaseSite_Slab", 5500, 2500, 5, 600, 600, 10, folder="Meadow/BaseSite")

    # 사냥꾼 거점(먼 남동): 천막 · 상자 · 꺼진 모닥불 + 첫 등장 퀘스트 발동 볼륨(튜토리얼을 끝낸 뒤에만 시작된다)
    HX, HY = 11000.0, -7000.0
    b.marker("Hunter_Camp", HX, HY)
    for i, (dx, dy) in enumerate([(-500, -300), (400, -400), (0, 500)]):
        b.shape(b.cone, f"Hunter_Tent_{i}", HX + dx, HY + dy, 150, 400, 400, 300, folder="Hunter/Camp")
    for i, (dx, dy) in enumerate([(-200, 100), (250, 150), (300, -100)]):
        b.box(f"Hunter_Crate_{i}", HX + dx, HY + dy, 40, 80, 80, 80, folder="Hunter/Camp")
    b.spawn("/Script/Grimoire.GrimCampfire", HX, HY, 0, label="Hunter_Campfire", folder="Hunter/Camp", tags=["Hunter_Campfire"])
    trigger = b.spawn("/Script/Grimoire.GrimQuestTriggerVolume", HX, HY, 200, label="Hunter_QuestTrigger", folder="Hunter/Camp")
    trigger.set_editor_property("quest_to_start", "Hunter_FirstEncounter")
    trigger.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(1200, 1200, 400))

    # 나무(원통 + 원뿔)
    if include_trees:
        for i, (x, y) in enumerate([(2500, -1800), (3500, 2500), (4500, -800), (6000, 1000), (5000, 2800), (-2500, 2500), (-2600, -2200), (1000, 2800), (1200, -2900), (7000, -1500)]):
            b.shape(b.cylinder, f"Tree_{i}_Trunk", x, y, 150, 40, 40, 300, folder="Meadow/Trees")
            b.shape(b.cone, f"Tree_{i}_Crown", x, y, 450, 260, 260, 400, folder="Meadow/Trees")


def _rotate_vector(b, v):
    if b.yaw == 0.0:
        return v
    c, s = math.cos(math.radians(b.yaw)), math.sin(math.radians(b.yaw))
    return unreal.Vector(v.x * c - v.y * s, v.x * s + v.y * c, v.z)
