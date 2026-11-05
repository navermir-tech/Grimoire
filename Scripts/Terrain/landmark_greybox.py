# 랜드마크 · POI 그레이박스(엔진 기본 도형) — build_openbeta.py 가 부른다.
# 레이아웃(openbeta_layout.json)의 Landmarks · Pois 를 읽어 자리마다 실루엣이 서로 다른 모양을 놓는다(docs/dev/level-design.md 「2」「5」).
#   고목 = 둥근 큰 덩어리, 바위 문 = 두 기둥 사이 틈, 선돌 = 고리, 폭포 = 흰 띠, 감시탑 = 부러진 원통, 풍차 = 날개, 마법탑 = 가는 수직(tower_greybox).
# 모양만 잡는 자리다 — 놀이(퍼즐 · 채집 · 생물 · 던전)는 각 마일스톤이 같은 표식(Landmark.* · Poi.*) 자리에 채운다.
#
# 좌표: 레이아웃 Yaw 는 지도 각도(지도 +X = 동쪽에서 반시계). 「앞」= 그 방향, 「왼쪽」= 반시계 90°.
# 월드(cm): X = 북쪽, Y = 동쪽 → 월드 Yaw = 90 − 지도 각도.
import math
import random

import unreal

CUBE = "/Engine/BasicShapes/Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder"
CONE = "/Engine/BasicShapes/Cone"
SPHERE = "/Engine/BasicShapes/Sphere"


class Placer:
    """지도 좌표 한 자리(u, v)와 방향(지도 각도) 기준의 로컬 미터 좌표로 기본 도형 · 액터를 놓는다."""

    def __init__(self, actor_subsystem, ground_cm, map_to_world, materials, on_spawn):
        self.actors = actor_subsystem
        self.ground_cm = ground_cm          # (월드 x, y) → 땅 높이 cm
        self.map_to_world = map_to_world    # (u, v) → (월드 x, y) cm
        self.materials = materials          # 이름 → 머티리얼
        self.on_spawn = on_spawn            # 놓은 액터마다(늘 로드 · 개수 세기)
        self.meshes = {path: unreal.load_asset(path) for path in (CUBE, CYLINDER, CONE, SPHERE)}
        self.u = self.v = 0.0
        self.angle = 0.0
        self.folder = ""
        self.prefix = ""
        self.base_z = None                  # None 이면 부품마다 그 자리 땅, 아니면 이 높이(cm)를 바닥으로

    def at(self, u, v, angle_deg, folder, prefix, level=False):
        """기준 자리를 정한다. level=True 면 모든 부품이 가운데 땅 높이를 바닥으로 쓴다(쌓는 건물)"""
        self.u, self.v, self.angle = u, v, angle_deg
        self.folder, self.prefix = folder, prefix
        self.base_z = None
        if level:
            x, y = map_to_world_safe(self.map_to_world, u, v)
            self.base_z = self.ground_cm(x, y)
        return self

    def local_to_map(self, lx, ly):
        a = math.radians(self.angle)
        return (self.u + lx * math.cos(a) - ly * math.sin(a), self.v + lx * math.sin(a) + ly * math.cos(a))

    def world_xy(self, lx, ly):
        u, v = self.local_to_map(lx, ly)
        return self.map_to_world(u, v)

    def ground_at(self, lx, ly):
        x, y = self.world_xy(lx, ly)
        return self.base_z if self.base_z is not None else self.ground_cm(x, y)

    def shape(self, mesh, material, name, lx, ly, z0, sx, sy, sz, yaw=0.0, pitch=0.0, roll=0.0, block=True, shadow=True, z_center=None):
        """기본 도형(100 cm)을 크기 sx · sy · sz(m)로. z0 = 바닥이 땅 위 몇 m 인가(z_center 를 주면 가운데 높이)"""
        x, y = self.world_xy(lx, ly)
        ground = self.ground_at(lx, ly)
        z = ground + ((z_center if z_center is not None else z0 + sz * 0.5) * 100.0)
        world_yaw = 90.0 - (self.angle + yaw)
        actor = self.actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(roll, pitch, world_yaw))
        actor.set_actor_label(f"{self.prefix}_{name}")
        actor.set_folder_path(self.folder)
        component = actor.static_mesh_component
        component.set_static_mesh(self.meshes[mesh])
        component.set_material(0, self.materials[material])
        component.set_collision_profile_name("BlockAll" if block else "NoCollision")
        component.set_editor_property("cast_shadow", shadow)
        actor.set_actor_scale3d(unreal.Vector(sx, sy, sz))
        self.on_spawn(actor)
        return actor

    def actor(self, class_path, name, lx, ly, z0=0.0, yaw=0.0, tags=()):
        x, y = self.world_xy(lx, ly)
        z = self.ground_at(lx, ly) + z0 * 100.0
        cls = unreal.load_class(None, class_path)
        if cls is None:
            raise RuntimeError(f"클래스 없음: {class_path}")
        actor = self.actors.spawn_actor_from_class(cls, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 90.0 - (self.angle + yaw)))
        actor.set_actor_label(f"{self.prefix}_{name}")
        actor.set_folder_path(self.folder)
        if tags:
            actor.set_editor_property("tags", [unreal.Name(t) for t in tags])
        self.on_spawn(actor)
        return actor


def map_to_world_safe(fn, u, v):
    return fn(u, v)


# ───────────── 랜드마크 ─────────────

def giant_tree(p):
    """거대 고목 — 굵은 줄기 + 뿌리 둔덕 + 둥근 잎 덩어리 여럿(꼭대기 약 47 m)"""
    p.shape(SPHERE, "Bark", "RootMound", 0, 0, 0, 16, 16, 5, z_center=0.3)
    for i in range(5):
        a = i * 72.0 + 15.0
        p.shape(CYLINDER, "Bark", f"Root_{i}", 5.5 * math.cos(math.radians(a)), 5.5 * math.sin(math.radians(a)), 0, 2.2, 2.2, 9, yaw=a, pitch=-62, z_center=1.2)
    p.shape(CYLINDER, "Bark", "Trunk", 0, 0, -1, 7, 7, 27)
    p.shape(CYLINDER, "Bark", "Branch_0", 3.5, 2, 0, 2, 2, 14, yaw=30, pitch=-40, z_center=22)
    p.shape(CYLINDER, "Bark", "Branch_1", -3, -2.5, 0, 2, 2, 13, yaw=210, pitch=-42, z_center=21)
    p.shape(SPHERE, "GiantCrown", "Crown", 0, 0, 0, 32, 30, 24, z_center=33)
    for i, (lx, ly, z, d) in enumerate([(10, 7, 27, 20), (-9, 8, 28, 19), (2, -11, 29, 21), (-7, -6, 36, 17), (4, 3, 41, 15)]):
        p.shape(SPHERE, "GiantCrown", f"Crown_{i}", lx, ly, 0, d, d, d * 0.8, z_center=z)


def rock_gate(p):
    """북쪽 고개 바위 문 — 틈(길) 양옆 두 바위 기둥이 안으로 기운다(높이 약 40 m)"""
    for side, sign in (("West", 1.0), ("East", -1.0)):
        ly = 37.0 * sign
        p.shape(CUBE, "DarkStone", f"Pillar_{side}", 0, ly, -2, 13, 14, 40, yaw=6 * sign, roll=-5 * sign)
        p.shape(CUBE, "DarkStone", f"PillarTop_{side}", 1.5, ly - 3 * sign, 0, 9, 10, 9, yaw=-18 * sign, roll=-9 * sign, z_center=38)
        p.shape(SPHERE, "Stone", f"Boulder_{side}_0", -7, ly + 4 * sign, 0, 9, 8, 6, z_center=1.5)
        p.shape(SPHERE, "Stone", f"Boulder_{side}_1", 8, ly + 6 * sign, 0, 7, 7, 5, z_center=1.0)


def stone_circle(p):
    """선돌 고리 — 반지름 14 m 에 선돌 12(하나는 쓰러짐), 가운데 높은 선돌 + 제단(높이 약 9 m)"""
    rng = random.Random("StoneCircle")
    for i in range(12):
        a = i * 30.0
        lx, ly = 14.0 * math.cos(math.radians(a)), 14.0 * math.sin(math.radians(a))
        if i == 7:
            p.shape(CUBE, "Stone", f"Menhir_{i:02d}_Fallen", lx * 1.15, ly * 1.15, 0, 6.5, 2.2, 1.4, yaw=a + 70, z_center=0.5)
            continue
        h = rng.uniform(5.5, 7.5)
        p.shape(CUBE, "Stone", f"Menhir_{i:02d}", lx, ly, -0.8, 1.4, 2.2, h, yaw=a, roll=rng.uniform(-6, 6), pitch=rng.uniform(-4, 4))
    p.shape(CYLINDER, "Stone", "Altar", 0, 0, -0.3, 4, 4, 1.1)
    p.shape(CUBE, "DarkStone", "CenterStone", 0, 0, 0.6, 1.8, 1.8, 8.5)


def watchtower(p):
    """폐허 감시탑 — 부러진 원통(한쪽만 높게 남은 벽 · 성가퀴) + 떨어진 덩어리(높이 약 18 m)"""
    p.shape(CYLINDER, "Stone", "Body", 0, 0, -1.5, 9, 9, 15.5)
    for i, a in enumerate(range(0, 180, 30)):
        h = 2.6 if i % 2 == 0 else 1.6
        p.shape(CUBE, "Stone", f"Merlon_{i}", 4.0 * math.cos(math.radians(a)), 4.0 * math.sin(math.radians(a)), 14.0, 1.6, 2.0, h, yaw=a)
    p.shape(CUBE, "Stone", "BrokenWall", 4.0 * math.cos(math.radians(75)), 4.0 * math.sin(math.radians(75)), 14.0, 1.4, 3.4, 4.0, yaw=75)
    p.shape(CUBE, "Dark", "Door", 4.4, 0, 0, 0.6, 2.0, 3.0)
    p.shape(CYLINDER, "Stone", "Fallen", -9, -7, 0, 6, 6, 5, yaw=35, pitch=90, z_center=2.6)
    for i, (lx, ly) in enumerate([(-6, 5), (7, -6), (-11, -2)]):
        p.shape(SPHERE, "Stone", f"Rubble_{i}", lx, ly, 0, 3, 2.6, 1.6, z_center=0.4)


def windmill(p):
    """풍차 — 돌 몸통 + 원뿔 지붕 + 앞(레이아웃 Yaw 방향)을 보는 + 자 날개(날개 끝 약 21 m). 돌아가는 것은 아직 없음"""
    p.shape(CYLINDER, "Stone", "Body", 0, 0, -0.5, 7, 7, 12.5)
    p.shape(CONE, "Roof", "Roof", 0, 0, 12.0, 8.5, 8.5, 5)
    p.shape(CUBE, "Dark", "Door", 3.6, 0, 0, 0.5, 1.6, 2.4)
    p.shape(CYLINDER, "Wood", "Hub", 4.4, 0, 0, 1.3, 1.3, 2.2, pitch=90, z_center=11.0)
    for i, (dy, dz, roll) in enumerate([(0, 5.6, 0), (0, -5.6, 0), (5.6, 0, 90), (-5.6, 0, 90)]):
        p.shape(CUBE, "Sail", f"Blade_{i}", 5.4, dy, 0, 0.25, 1.8, 10, roll=roll, block=False, z_center=11.0 + dz)
        p.shape(CUBE, "Wood", f"Spar_{i}", 5.25, dy, 0, 0.3, 0.3, 10.5, roll=roll, block=False, z_center=11.0 + dz)


def waterfall(p, falls, world_to_local_height):
    """폭포 — 위 끝 → 아래 끝으로 비스듬한 흰 띠 + 아래 물보라 + 물 볼륨 띠(헤엄 · 젖음). falls = 생성기 Falls(월드 cm)"""
    top, bottom = falls.points[0], falls.points[1]
    dx, dy, dz = bottom.x - top.x, bottom.y - top.y, bottom.z - top.z
    flat = math.hypot(dx, dy)
    length = math.hypot(flat, dz)
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, max(flat, 1.0)))
    mid = unreal.Vector((top.x + bottom.x) / 2, (top.y + bottom.y) / 2, (top.z + bottom.z) / 2 + 60.0)
    sheet = p.actors.spawn_actor_from_class(unreal.StaticMeshActor, mid, unreal.Rotator(0, pitch, yaw))
    sheet.set_actor_label(f"{p.prefix}_Sheet")
    sheet.set_folder_path(p.folder)
    c = sheet.static_mesh_component
    c.set_static_mesh(p.meshes[CUBE])
    c.set_material(0, p.materials["Falls"])
    c.set_collision_profile_name("NoCollision")
    sheet.set_actor_scale3d(unreal.Vector(length / 100.0 + 2.0, falls.size / 100.0, 0.6))
    p.on_spawn(sheet)
    foam = p.actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(bottom.x, bottom.y, bottom.z + 50.0), unreal.Rotator(0, 0, yaw))
    foam.set_actor_label(f"{p.prefix}_Foam")
    foam.set_folder_path(p.folder)
    c = foam.static_mesh_component
    c.set_static_mesh(p.meshes[SPHERE])
    c.set_material(0, p.materials["Falls"])
    c.set_collision_profile_name("NoCollision")
    foam.set_actor_scale3d(unreal.Vector(falls.size / 100.0 * 2.2, falls.size / 100.0 * 2.2, 1.6))
    p.on_spawn(foam)
    return mid, yaw, flat, -dz


LANDMARK_BUILDERS = {
    "GiantTree": (giant_tree, True),
    "RockGate": (rock_gate, False),
    "StoneCircle": (stone_circle, False),
    "Watchtower": (watchtower, True),
    "Windmill": (windmill, True),
}


# ───────────── POI 소품(자리와 실루엣만) ─────────────

def _rocks(p, rng, n, spread, name="Rock", size=(1.4, 2.6)):
    for i in range(n):
        a = rng.uniform(0, 360)
        r = rng.uniform(0.3, 1.0) * spread
        s = rng.uniform(*size)
        p.shape(SPHERE, "Rock", f"{name}_{i}", r * math.cos(math.radians(a)), r * math.sin(math.radians(a)), 0, s * 1.2, s, s * 0.7, yaw=rng.uniform(0, 180), z_center=s * 0.15)


def poi_puzzle(p, rng, poi):
    """속성 퍼즐 — 무너진 벽 ㄱ 자 + 받침 위 꺼진 화로(모닥불, 꺼진 채) + 마른 풀(덩굴)"""
    p.shape(CUBE, "Stone", "WallA", -3.0, 1.5, -0.3, 5.5, 0.8, 2.8)
    p.shape(CUBE, "Stone", "WallB", -5.4, -0.8, -0.3, 0.8, 4.0, 1.9)
    p.shape(CYLINDER, "Stone", "Pedestal", 0, 0, -0.2, 1.4, 1.4, 1.1)
    p.actor("/Script/Grimoire.GrimCampfire", "Brazier", 0, 0, 0.9, tags=["Poi_Brazier"])
    for i, (lx, ly) in enumerate([(2.6, 2.0), (3.0, -1.6), (-1.5, -2.6)]):
        p.actor("/Script/Grimoire.GrimDryGrass", f"DryGrass_{i}", lx, ly, 0.0, yaw=rng.uniform(0, 360), tags=["Poi_DryGrass"])


def poi_resource(p, rng, poi):
    """자원 군락 — 바위 셋 + 작은 풀 덩이(약초 · 광석 자리)"""
    _rocks(p, rng, 3, poi.radius * 0.6)
    for i in range(7):
        a = rng.uniform(0, 360)
        r = rng.uniform(0.2, 0.9) * poi.radius * 0.8
        p.shape(CONE, "Herb", f"Herb_{i}", r * math.cos(math.radians(a)), r * math.sin(math.radians(a)), -0.1, 0.9, 0.9, rng.uniform(0.9, 1.6), block=False, shadow=False)


def poi_creature(p, rng, poi):
    """생물 자리 — 흙 둔덕 + 굴 입구 + 뼈 + 바위"""
    p.shape(SPHERE, "Dirt", "Mound", 0, 0, 0, 7.5, 6.5, 2.6, z_center=0.2)
    p.shape(CYLINDER, "Dark", "Hole", 2.9, 0, 0, 1.8, 1.8, 0.5, pitch=65, z_center=0.7)
    for i, (lx, ly, yaw) in enumerate([(4.5, 1.6, 20), (4.0, -2.0, 110)]):
        p.shape(CYLINDER, "Bone", f"Bone_{i}", lx, ly, 0, 0.18, 0.18, 1.3, yaw=yaw, pitch=90, block=False, shadow=False, z_center=0.1)
    _rocks(p, rng, 2, 5.5)


def poi_ruin(p, rng, poi):
    """작은 폐허 · 보물 — 높이가 다른 벽 셋 + 쓰러진 기둥 + 돌 상자"""
    p.shape(CUBE, "Stone", "WallA", 0, 3.0, -0.3, 5.0, 0.8, 3.4)
    p.shape(CUBE, "Stone", "WallB", -2.8, 0.8, -0.3, 0.8, 3.8, 2.2)
    p.shape(CUBE, "Stone", "WallC", 1.6, -2.6, -0.3, 2.6, 0.8, 1.3)
    p.shape(CYLINDER, "Stone", "FallenColumn", 3.2, -0.4, 0, 0.9, 0.9, 4.2, yaw=rng.uniform(0, 180), pitch=90, z_center=0.4)
    p.shape(CUBE, "DarkStone", "StoneChest", 0.4, 0.9, 0, 1.4, 0.9, 0.85)


def poi_viewpoint(p, rng, poi):
    """전망대 — 오를 수 있는 넓적 바위 + 깃대(빨간 깃발)"""
    p.shape(CYLINDER, "Stone", "Rock", 0, 0, -0.5, 4.8, 4.8, 2.0)
    p.shape(CYLINDER, "Wood", "FlagPole", 1.0, 1.0, 1.4, 0.16, 0.16, 7.0)
    p.shape(CUBE, "Flag", "Flag", 1.0, 2.2, 0, 0.08, 2.2, 1.3, block=False, z_center=7.6)


def poi_story(p, rng, poi):
    """이야기 흔적 — 꺼진 모닥불(돌 고리) + 상자 · 짐 + 깔개"""
    p.actor("/Script/Grimoire.GrimCampfire", "Campfire", 0, 0, 0.0, tags=["Poi_Campfire"])
    for i in range(6):
        a = i * 60.0
        p.shape(SPHERE, "Rock", f"RingStone_{i}", 1.1 * math.cos(math.radians(a)), 1.1 * math.sin(math.radians(a)), 0, 0.5, 0.45, 0.35, block=False, z_center=0.1)
    p.shape(CUBE, "Wood", "Crate_0", 2.2, 1.0, 0, 0.9, 0.9, 0.9, yaw=rng.uniform(0, 90))
    p.shape(CUBE, "Wood", "Crate_1", 2.5, -0.4, 0, 0.7, 0.7, 0.6, yaw=rng.uniform(0, 90))
    p.shape(CUBE, "Sail", "Bedroll", -2.2, 1.5, 0, 2.0, 0.9, 0.18, yaw=rng.uniform(-20, 20), block=False)


def poi_base_site(p, rng, poi):
    """거점 터 — 네 귀퉁이 말뚝 + 가운데 깃대(평지는 생성기가 고른다)"""
    r = min(poi.radius * 0.45, 12.0)
    for i, (sx, sy) in enumerate([(1, 1), (1, -1), (-1, 1), (-1, -1)]):
        p.shape(CYLINDER, "Wood", f"Stake_{i}", r * sx, r * sy, -0.2, 0.22, 0.22, 1.7)
    p.shape(CYLINDER, "Wood", "FlagPole", 0, 0, -0.2, 0.18, 0.18, 6.0)
    p.shape(CUBE, "Flag", "Flag", 0, 1.2, 0, 0.08, 2.2, 1.3, block=False, z_center=5.2)


def poi_dungeon(p, rng, poi):
    """던전 입구 — 돌 기둥 둘 + 상인방 + 검은 입구 + 바위"""
    for side, ly in (("L", 2.5), ("R", -2.5)):
        p.shape(CUBE, "DarkStone", f"Pillar_{side}", 0, ly, -0.5, 1.5, 1.5, 5.5)
    p.shape(CUBE, "DarkStone", "Lintel", 0, 0, 4.8, 1.8, 6.6, 1.3)
    p.shape(CUBE, "Dark", "Opening", -0.5, 0, -0.3, 0.4, 3.5, 4.6, block=False)
    _rocks(p, rng, 3, 6.0, size=(1.8, 3.2))


POI_BUILDERS = {
    "Puzzle": poi_puzzle,
    "Resource": poi_resource,
    "Creature": poi_creature,
    "Ruin": poi_ruin,
    "Viewpoint": poi_viewpoint,
    "Story": poi_story,
    "BaseSite": poi_base_site,
    "Dungeon": poi_dungeon,
}


class _Poi:
    def __init__(self, spec):
        self.name = spec["Name"]
        self.kind = spec["Kind"]
        self.cluster = spec.get("Cluster", "")
        self.u = spec["Center"]["X"]
        self.v = spec["Center"]["Y"]
        self.yaw = spec.get("Yaw", 0.0)
        self.radius = spec.get("Radius", 12.0)


def build_pois(placer, raw):
    """레이아웃 Pois 마다 종류별 소품. 돌려준 값 = 종류 → 개수"""
    kinds = {}
    for spec in raw.get("Pois", []):
        poi = _Poi(spec)
        builder = POI_BUILDERS.get(poi.kind)
        if builder is None:
            unreal.log_warning(f"landmark_greybox: 모르는 POI 종류 {poi.kind} ({poi.name}) — 표식만")
            continue
        placer.at(poi.u, poi.v, poi.yaw, f"Poi/{poi.cluster}/{poi.name}", poi.name)
        builder(placer, random.Random(poi.name), poi)
        kinds[poi.kind] = kinds.get(poi.kind, 0) + 1
    return kinds


def build_landmarks(placer, raw, falls_by_name, spawn_water_volume):
    """레이아웃 Landmarks 마다 모양. 마법탑은 tower_greybox 가 따로 짓는다. 폭포는 생성기가 찾은 Falls 를 쓴다"""
    built = []
    for spec in raw.get("Landmarks", []):
        kind = spec["Kind"]
        name = spec["Name"]
        u, v = spec["Center"]["X"], spec["Center"]["Y"]
        if kind == "Tower":
            continue
        if kind == "Waterfall":
            # 생성기는 개울마다 큰 단차를 폭포로 낸다(강으로 떨어지는 어귀 등) — 랜드마크 자리에 가장 가까운 것
            wx, wy = placer.map_to_world(u, v)
            falls = min(falls_by_name.values(), key=lambda b: math.hypot(b.points[0].x - wx, b.points[0].y - wy), default=None)
            if falls is None:
                unreal.log_warning("landmark_greybox: 생성기가 폭포(Falls)를 찾지 못했다 — 폭포 모양 없음")
                continue
            placer.at(u, v, spec.get("Yaw", 0.0), f"Landmarks/{name}", name)
            mid, yaw, flat, drop = waterfall(placer, falls, None)
            spawn_water_volume(f"WaterVolume_{name}", unreal.Vector(mid.x, mid.y, mid.z - 60.0),
                               unreal.Vector(flat / 2.0 + 300.0, falls.size / 2.0 + 150.0, drop / 2.0 + 200.0), yaw)
            built.append(name)
            continue
        entry = LANDMARK_BUILDERS.get(kind)
        if entry is None:
            unreal.log_warning(f"landmark_greybox: 모르는 랜드마크 종류 {kind} ({name})")
            continue
        builder, level = entry
        placer.at(u, v, spec.get("Yaw", 0.0), f"Landmarks/{name}", name, level=level)
        builder(placer)
        built.append(name)
    return built


def build_bridges(placer, raw, path_points_by_name):
    """길이 개울을 건너는 곳마다 널다리(길 꺾은선 × 개울 꺾은선 교차 — 꺾은점 기준이라 수 m 어긋날 수 있다)"""
    def segments(points):
        return [((points[i]["X"], points[i]["Y"]), (points[i + 1]["X"], points[i + 1]["Y"])) for i in range(len(points) - 1)]

    def intersect(a, b, c, d):
        (x1, y1), (x2, y2), (x3, y3), (x4, y4) = a, b, c, d
        den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)
        if abs(den) < 1e-9:
            return None
        t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den
        s = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / den
        if 0.0 <= t <= 1.0 and 0.0 <= s <= 1.0:
            return (x1 + t * (x2 - x1), y1 + t * (y2 - y1)), (x2 - x1, y2 - y1)
        return None

    count = 0
    for path in raw.get("Paths", []):
        for creek in raw.get("Creeks", []):
            for a, b in segments(path["Points"]):
                for c, d in segments(creek["Points"]):
                    hit = intersect(a, b, c, d)
                    if hit is None:
                        continue
                    (u, v), (du, dv) = hit
                    angle = math.degrees(math.atan2(dv, du))
                    name = f"Bridge_{path['Name']}_{creek['Name']}_{count}"
                    placer.at(u, v, angle, "Bridges", name)
                    # 양쪽 둑 높이 중 높은 쪽에 맞춘다
                    bank = max(placer.ground_at(-9, 0), placer.ground_at(9, 0))
                    placer.base_z = bank
                    placer.shape(CUBE, "Wood", "Deck", 0, 0, 0.05, 18.0, path["Width"] + 1.2, 0.45)
                    for side in (1, -1):
                        placer.shape(CUBE, "Wood", f"Rail_{side}", 0, side * (path["Width"] * 0.5 + 0.5), 0.5, 18.0, 0.18, 0.18, block=False)
                    count += 1
    return count
