# 랜드마크 · POI 그레이박스(엔진 기본 도형) — build_openbeta.py 가 부른다.
# 레이아웃(openbeta_layout.json)의 Landmarks · Pois 를 읽어 자리마다 실루엣이 서로 다른 모양을 놓는다(docs/dev/level-design.md 「2」「5」).
#   고목 = 둥근 큰 덩어리, 바위 문 = 두 기둥 사이 틈, 선돌 = 고리, 폭포 = 흰 띠, 감시탑 = 부러진 원통, 풍차 = 날개, 부유석 = 공중에 뜬 바위섬, 마법탑 = 가는 수직(tower_greybox).
# 모양만 잡는 자리다 — 놀이(퍼즐 · 채집 · 생물 · 던전)는 각 마일스톤이 같은 표식(Landmark.* · Poi.*) 자리에 채운다.
# 마탑 구간(「7」): 랜드마크 · 구간 POI 위에 「무엇이 들어갈 자리인가」 떠 있는 글씨(AGrimDevLabel) + 종류 색 블록, 구간 경계에 「공사 중」 기둥 · 띠 · 글씨.
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

GIANT_TREE_SCALE = 1.28


def _beam(p, material, name, a, b, width, depth, roll=0.0, block=True, shadow=True, extend=0.6):
    """두 점(로컬 m: 앞 lx, 왼쪽 ly, 땅 위 z) 사이를 잇는 각진 상자 — 뿌리 · 줄기 마디 · 가지. extend = 양 끝을 더 늘려 마디 사이 틈을 메운다"""
    dx, dy, dz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    flat = math.hypot(dx, dy)
    length = math.hypot(flat, dz) + extend
    heading = math.degrees(math.atan2(dy, dx)) if flat > 1e-3 else 0.0
    pitch = math.degrees(math.atan2(dz, max(flat, 1e-3)))
    return p.shape(CUBE, material, name, (a[0] + b[0]) / 2, (a[1] + b[1]) / 2, 0, length, width, depth,
                   yaw=heading, pitch=pitch, roll=roll, block=block, shadow=shadow, z_center=(a[2] + b[2]) / 2)


class _Scaled:
    """Placer 를 감싸 로컬 길이(자리 · 크기 · 높이)를 모두 S 배 — 모양은 그대로 크기만 키운다"""

    def __init__(self, p, scale):
        self.p, self.s = p, scale

    def shape(self, mesh, material, name, lx, ly, z0, sx, sy, sz, z_center=None, **kw):
        s = self.s
        return self.p.shape(mesh, material, name, lx * s, ly * s, z0 * s, sx * s, sy * s, sz * s,
                            z_center=None if z_center is None else z_center * s, **kw)


def giant_tree(p):
    """거대 고목 — 큐브 목업(레벨 디자인 2차, docs/dev/level-design.md 「9」). 원기둥 하나가 아니라 덩어리를 쪼개 실루엣을 만든다:
    판뿌리(버팀뿌리) 9 갈래가 계단처럼 퍼지고(동쪽 한 갈래는 전망 길), 마디마다 비틀리며 기우는 줄기 5 마디 + 세로 골 덩이,
    줄기 14 ~ 19 m 에서 바깥 · 위로 갈라지는 큰 가지 6(마디 3 + 잔가지), 가지 끝 · 가운데 · 꼭대기 · 아래 가장자리(치마)에 잎 덩어리 묶음(큰 상자 + 둘레 작은 상자).
    아래 치수(꼭대기 약 47 m, 잎 지름 약 58 m)를 GIANT_TREE_SCALE 배 — 꼭대기 약 60 m, 잎 지름 약 75 m(콘셉트 lm-giant-tree — 넓고 낮게 퍼진 참나무형,
    500 m 언덕 위에서 멀리서도 덩어리가 읽히게). 레이아웃 Landmark_GiantTree 의 Height 와 맞춘다. 시드 고정이라 다시 지어도 같다."""
    rng = random.Random("GiantTreeBlockout")
    p = _Scaled(p, GIANT_TREE_SCALE)

    # ── 판뿌리: 줄기 밑에서 바깥으로 낮아지며 퍼진다. 마디 3~4, 마디마다 조금씩 휜다 ──
    root_dirs = [8, 47, 88, 128, 170, 208, 247, 290, 330]
    for i, a0 in enumerate(root_dirs):
        reach = rng.uniform(11.0, 16.0) if i != 0 else 21.0   # 0 번(동쪽 앞) = 뿌리를 딛고 오르는 전망 길
        pts = []
        heading = a0 + rng.uniform(-6, 6)
        for k, (t, z) in enumerate([(0.0, 5.0), (0.28, 2.6), (0.58, 1.1), (1.0, -0.2)]):
            r = 2.8 + (reach - 2.8) * t
            heading += rng.uniform(-7, 7) if k else 0.0
            pts.append((r * math.cos(math.radians(heading)), r * math.sin(math.radians(heading)), z))
        for k in range(len(pts) - 1):
            w = (3.4 - k * 0.8) * (1.15 if i == 0 else 1.0)
            h = max(3.4 - k * 1.05, 0.9)
            _beam(p, "Bark", f"Root_{i}_{k}", pts[k], pts[k + 1], w, h, roll=rng.uniform(-8, 8), extend=1.2)
        # 뿌리 사이 흙 · 이끼 덩이(판뿌리 밑동을 땅에 묻는다)
        r = reach * 0.45
        p.shape(CUBE, "Dirt", f"RootSoil_{i}", r * math.cos(math.radians(a0 + 20)), r * math.sin(math.radians(a0 + 20)), -0.6,
                rng.uniform(3.5, 5.0), rng.uniform(2.5, 3.5), 1.2, yaw=a0 + rng.uniform(-20, 20), roll=rng.uniform(-6, 6), shadow=False)

    # ── 줄기: 7 마디, 위로 갈수록 가늘고 마디마다 비틀리며 살짝 기운다 ──
    widths = [10.5, 9.2, 8.0, 7.0, 6.2]
    seg_h = 3.9
    centers = []
    twist = 0.0
    for k, w in enumerate(widths):
        z = k * seg_h + seg_h / 2
        cx = 1.6 * math.sin(z / 9.0) - 0.6
        cy = 1.1 * math.sin(z / 6.5 + 0.8)
        centers.append((cx, cy, z))
        twist += rng.uniform(8, 15)
        p.shape(CUBE, "Bark", f"Trunk_{k}", cx, cy, 0, w, w * rng.uniform(0.82, 0.95), seg_h + 0.9,
                yaw=twist, pitch=rng.uniform(-3, 3), roll=rng.uniform(-3, 3), z_center=z)
        if k <= 3:   # 아래 네 마디: 모서리에 세로 골 덩이(버팀벽처럼 — 네모 실루엣을 깬다)
            for j in range(4):
                a = twist + 45 + j * 90 + rng.uniform(-12, 12)
                r = w * 0.52
                p.shape(CUBE, "Bark", f"Flute_{k}_{j}", cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a)), 0,
                        w * 0.32, w * 0.26, seg_h + 1.2, yaw=a, pitch=rng.uniform(-4, 4), z_center=z - 0.4)
    top = centers[-1]

    # ── 큰 가지 6: 줄기 위쪽(17~24 m)에서 바깥 · 위로, 끝으로 갈수록 눕는다(참나무형). 끝에 잔가지 2 ──
    limb_specs = [(18, 18.5, 19.0), (80, 15.5, 21.0), (135, 19.0, 17.5), (195, 16.5, 20.5), (250, 14.0, 19.0), (310, 17.5, 18.0)]
    tips = []
    mids = []
    for i, (heading, z0, reach) in enumerate(limb_specs):
        base = min(centers, key=lambda c: abs(c[2] - z0))
        pos = (base[0], base[1], z0)
        pitches = [rng.uniform(36, 44), rng.uniform(22, 30), rng.uniform(8, 16)]
        lengths = [reach * 0.42, reach * 0.34, reach * 0.30]
        thick = [3.4, 2.5, 1.7]
        h = heading + rng.uniform(-6, 6)
        for k in range(3):
            h += rng.uniform(-10, 10)
            dz = lengths[k] * math.sin(math.radians(pitches[k]))
            dr = lengths[k] * math.cos(math.radians(pitches[k]))
            nxt = (pos[0] + dr * math.cos(math.radians(h)), pos[1] + dr * math.sin(math.radians(h)), pos[2] + dz)
            _beam(p, "Bark", f"Limb_{i}_{k}", pos, nxt, thick[k], thick[k] * 0.9, roll=rng.uniform(0, 30), extend=1.0)
            pos = nxt
            if k == 1:
                mids.append(pos)
        for f in range(2):   # 잔가지
            fh = h + (35 if f else -35) + rng.uniform(-10, 10)
            end = (pos[0] + 6 * math.cos(math.radians(fh)), pos[1] + 6 * math.sin(math.radians(fh)), pos[2] + rng.uniform(1.0, 3.0))
            _beam(p, "Bark", f"Twig_{i}_{f}", pos, end, 1.1, 1.0, extend=0.6)
        tips.append(pos)
    leader = (top[0] + 1.0, top[1] - 0.5, 31.0)
    _beam(p, "Bark", "Leader", (top[0], top[1], top[2]), leader, 3.0, 2.8)
    tips.append(leader)

    # ── 잎 덩어리: 가지 끝마다 큰 묶음(큰 상자 + 둘레 작은 상자 8, 아래가 평평한 구름처럼) + 가지 가운데 작은 묶음 + 꼭대기 둥근 지붕 묶음 3.
    # 위쪽 상자는 밝은 잎 색(빛 받는 면 흉내). 잎 덩어리 높이 약 22 ~ 48 m(× 배율) — 멀리서 우산이 아니라 둥근 덩어리로 읽히게 위아래로 두껍게 ──
    def cluster(tag, cx, cy, cz, core, sats, spread, size):
        p.shape(CUBE, "GiantCrown", f"Crown_{tag}_Core", cx, cy, 0, *core, yaw=rng.uniform(0, 90), roll=rng.uniform(-6, 6), pitch=rng.uniform(-5, 5), z_center=cz)
        for j in range(sats):
            a = j * (360.0 / sats) + rng.uniform(-20, 20)
            r = rng.uniform(*spread)
            sz = rng.uniform(*size)
            dz = rng.uniform(-3.5, 5.0)
            material = "Herb" if dz > 2.5 else "GiantCrown"
            p.shape(CUBE, material, f"Crown_{tag}_{j}", cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a)), 0,
                    sz, sz * rng.uniform(0.8, 1.1), sz * rng.uniform(0.6, 0.85),
                    yaw=rng.uniform(0, 90), pitch=rng.uniform(-14, 14), roll=rng.uniform(-14, 14), z_center=cz + dz, block=(sz > 6.5))

    for i, (tx, ty, tz) in enumerate(tips[:-1]):
        cluster(f"Tip{i}", tx, ty, tz + 2.5, (rng.uniform(13, 16), rng.uniform(11, 14), rng.uniform(8, 10)), 8, (5.0, 9.0), (5.0, 9.0))
    for i, (mx, my, mz) in enumerate(mids):
        cluster(f"Mid{i}", mx, my, mz + 1.5, (rng.uniform(8, 10), rng.uniform(7, 9), rng.uniform(5, 6.5)), 4, (3.5, 6.0), (4.0, 6.5))
    lx, ly, lz = tips[-1]
    cluster("Top", lx, ly, lz + 6.0, (18.0, 16.0, 10.0), 8, (6.0, 10.0), (6.0, 9.5))
    for k in range(3):   # 꼭대기 둘레 둥근 지붕
        a = k * 120 + rng.uniform(-20, 20)
        cluster(f"Dome{k}", lx + 11 * math.cos(math.radians(a)), ly + 11 * math.sin(math.radians(a)), lz + 2.0,
                (12.0, 11.0, 8.0), 5, (4.0, 7.0), (4.5, 7.5))
    # 아래 가장자리 치마 묶음 6 — 잎 덩어리 아래 모서리가 둥글게 처져 우산 · 버섯이 아니라 둥근 덩어리로 읽히게
    for k in range(6):
        a = k * 60 + 30 + rng.uniform(-15, 15)
        r = rng.uniform(19.0, 23.0)
        cluster(f"Skirt{k}", r * math.cos(math.radians(a)), r * math.sin(math.radians(a)), rng.uniform(20.5, 23.0),
                (rng.uniform(9, 11), rng.uniform(8, 10), rng.uniform(5.5, 7.0)), 3, (3.5, 6.0), (4.0, 6.5))
    # 가지 사이 빈 곳을 메우는 가운데 덩이 — 층마다 크기 · 방향을 달리해 밑면이 평평한 천장이 되지 않게
    p.shape(CUBE, "GiantCrown", "Crown_Fill_Low", 0.5, 0, 0, 22, 20, 6.0, yaw=20, z_center=25.0)
    p.shape(CUBE, "GiantCrown", "Crown_Fill_Mid", -1.0, 1.0, 0, 30, 26, 8.0, yaw=55, z_center=30.5)
    p.shape(CUBE, "Herb", "Crown_Fill_Top", 0.5, -0.5, 0, 22, 20, 7.0, yaw=35, z_center=38.5)


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


def floating_rock(p):
    """부유석(마탑 구간) — 언덕 꼭대기 위 공중에 뜬 바위섬(땅 위 약 38~51 m — 작은 언덕들 너머로 보이게) + 아래로 뾰족한 밑동 + 떠 있는 조각 + 마나 빛줄기 + 땅의 부서진 조각"""
    p.shape(SPHERE, "FloatRock", "Island", 0, 0, 0, 18, 16, 9, block=True, z_center=46.5)
    p.shape(CONE, "FloatRock", "Underside", 0, 0, 0, 15, 13, 10, pitch=180, block=True, z_center=39.5)
    p.shape(CUBE, "Herb", "Grass", 0, 0, 0, 12, 10, 0.6, block=False, z_center=50.6)
    p.shape(CUBE, "Stone", "Chest", 2.0, 1.0, 0, 1.4, 0.9, 0.9, z_center=51.3)
    for i, (lx, ly, z, d) in enumerate([(13, 4, 41, 4.5), (-11, 9, 44, 3.5), (-6, -14, 36, 3.0), (9, -10, 30, 2.4), (-4, 6, 22, 2.0)]):
        p.shape(SPHERE, "FloatRock", f"Fragment_{i}", lx, ly, 0, d, d * 0.9, d * 0.8, block=False, z_center=z)
    p.shape(CYLINDER, "ManaGlow", "ManaBeam", 0, 0, 0, 1.6, 1.6, 36, block=False, shadow=False, z_center=17.0)
    for i, (lx, ly) in enumerate([(7, 3), (-6, 6), (2, -8)]):
        p.shape(SPHERE, "FloatRock", f"Shard_{i}", lx, ly, 0, 2.6, 2.2, 1.6, z_center=0.4)


LANDMARK_BUILDERS = {
    "GiantTree": (giant_tree, True),
    "RockGate": (rock_gate, False),
    "StoneCircle": (stone_circle, False),
    "Watchtower": (watchtower, True),
    "Windmill": (windmill, True),
    "FloatingRock": (floating_rock, True),
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
        if spec.get("bNoGreybox"):
            continue   # 레벨 디자인 3차: 실제 소품이 놓인 POI(짐수레 — ld3_dress.py) — 블록을 겹치지 않는다
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
            # 탑 몸은 tower_greybox(L_Tower 와 같음). 여기서는 꼭대기에서 하늘로 솟는 마나 빛줄기만 — 작은 언덕 골 안에서도 방향을 잡게(높이 = 레이아웃 Height)
            top = 38.0
            height = max(spec.get("Height", top) - top, 0.0)
            if height > 1.0:
                placer.at(u, v, 0.0, f"Landmarks/{name}", name)
                placer.shape(CYLINDER, "ManaGlow", "ManaBeam", 0, 0, 0, 2.2, 2.2, height, block=False, shadow=False, z_center=top + height * 0.5)
                placer.shape(SPHERE, "ManaGlow", "ManaBeamTop", 0, 0, 0, 5.0, 5.0, 5.0, block=False, shadow=False, z_center=top + height)
                built.append(name)
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


# ───────────── 개발 표시(마탑 구간 — docs/dev/level-design.md 「7」): 종류 블록 · 떠 있는 글씨 · 공사 중 ─────────────

DEV_LABEL_CLASS = "/Script/Grimoire.GrimDevLabel"

# POI 종류 → (한글 이름, 블록 색 머티리얼, 글씨 제목 색)
POI_KIND_INFO = {
    "Puzzle": ("속성 퍼즐", "KindPuzzle", (1.00, 0.55, 0.15)),
    "Resource": ("자원", "KindResource", (0.50, 0.95, 0.35)),
    "Creature": ("생물", "KindCreature", (1.00, 0.45, 0.40)),
    "Ruin": ("폐허 · 보물", "KindRuin", (0.82, 0.74, 1.00)),
    "Viewpoint": ("전망대", "KindViewpoint", (1.00, 0.90, 0.30)),
    "Story": ("이야기 흔적", "KindStory", (0.55, 0.80, 1.00)),
    "BaseSite": ("거점 터", "KindBaseSite", (0.60, 0.70, 1.00)),
    "Dungeon": ("던전 입구", "KindDungeon", (0.85, 0.60, 1.00)),
}
TIER_NAMES = {"Global": "전역 랜드마크", "District": "구역 랜드마크"}
LANDMARK_LABEL_COLOR = (1.00, 0.35, 0.25)
WARNING_LABEL_COLOR = (1.00, 0.62, 0.10)


def point_in_polygon(u, v, polygon):
    inside = False
    n = len(polygon)
    for i in range(n):
        ax, ay = polygon[i - 1]["X"], polygon[i - 1]["Y"]
        bx, by = polygon[i]["X"], polygon[i]["Y"]
        if (by > v) != (ay > v) and u < (ax - bx) * (v - by) / (ay - by) + bx:
            inside = not inside
    return inside


def _segment_distance(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    length2 = dx * dx + dy * dy
    t = 0.0 if length2 <= 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def distance_to_paths(u, v, raw):
    best = 1e9
    for path in raw.get("Paths", []):
        pts = path["Points"]
        for i in range(len(pts) - 1):
            best = min(best, _segment_distance(u, v, pts[i]["X"], pts[i]["Y"], pts[i + 1]["X"], pts[i + 1]["Y"]))
    return best


def spawn_label(placer, name, folder, u, v, z_above_ground_m, title, body, color, scale, wrap=900.0, base_z_cm=None):
    """떠 있는 글씨(AGrimDevLabel). 글씨 아래 가운데가 (u, v, 땅 + z) — 한글은 Slate 폴백 폰트로 그린다"""
    x, y = placer.map_to_world(u, v)
    ground = base_z_cm if base_z_cm is not None else placer.ground_cm(x, y)
    cls = unreal.load_class(None, DEV_LABEL_CLASS)
    if cls is None:
        raise RuntimeError(f"클래스 없음: {DEV_LABEL_CLASS}")
    actor = placer.actors.spawn_actor_from_class(cls, unreal.Vector(x, y, ground + z_above_ground_m * 100.0), unreal.Rotator(0, 0, 0))
    actor.set_actor_label(name)
    actor.set_folder_path(folder)
    actor.set_editor_property("title", unreal.Text(title))
    actor.set_editor_property("body", unreal.Text(body))
    actor.set_editor_property("title_color", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    actor.set_editor_property("world_scale", scale)
    actor.set_editor_property("wrap_width", wrap)
    actor.set_editor_property("tags", [unreal.Name("DevLabel")])
    placer.on_spawn(actor)
    return actor


def build_dev_labels(placer, raw):
    """랜드마크(전부) · 마탑 구간 POI 의 종류 블록(바닥 원판 + 기둥) + 떠 있는 글씨. 돌려준 값 = 글씨 수"""
    sector = raw.get("Analysis", {}).get("Sector", {})
    polygon = sector.get("Polygon", [])
    count = 0
    for spec in raw.get("Landmarks", []):
        u, v = spec["Center"]["X"], spec["Center"]["Y"]
        title = f"{TIER_NAMES.get(spec.get('Tier'), '랜드마크')} — {spec.get('Label', spec['Name'])}"
        spawn_label(placer, f"Label_{spec['Name']}", "DevLabels/Landmarks", u, v, spec.get("LabelHeight", spec.get("Height", 20.0) + 7.0),
                    title, spec.get("Note", ""), LANDMARK_LABEL_COLOR, 4.0, wrap=1000.0)
        count += 1
    for spec in raw.get("Pois", []):
        u, v = spec["Center"]["X"], spec["Center"]["Y"]
        if not polygon or not point_in_polygon(u, v, polygon):
            continue
        kind_name, material, color = POI_KIND_INFO.get(spec["Kind"], (spec["Kind"], "Stone", (1, 1, 1)))
        name = spec["Name"]
        placer.at(u, v, 0.0, f"DevLabels/Poi/{name}", f"Block_{name}")
        radius = min(spec.get("Radius", 10.0), 14.0)   # 거점 터(28 m)도 원판은 작게 — 멀리서 연못처럼 보이지 않게
        # 블록: 자리 크기(반지름) 원판(반쯤 묻힘) + 종류 색 기둥 — 멀리서도 「무엇이 들어갈 자리」인지 색으로 읽힌다
        if not spec.get("bNoGreybox"):
            placer.shape(CYLINDER, material, "Footprint", 0, 0, 0, radius * 2.0, radius * 2.0, 1.0, block=False, shadow=False, z_center=0.0)
            placer.shape(CYLINDER, material, "Post", 0, 0, -0.5, 0.45, 0.45, 7.0, block=False)
        spawn_label(placer, f"Label_{name}", f"DevLabels/Poi/{name}", u, v, 6.8,
                    f"{kind_name} — {spec.get('Label', name)}", spec.get("Note", ""), color, 1.8, wrap=820.0)
        count += 1
    return count


def build_construction_fence(placer, raw, spacing_m=40.0, sign_every=4):
    """마탑 구간 밖을 알리는 「공사 중」 표시 — Analysis.Sector.Fence 꺾은선을 따라 기둥(주황 · 검정 띠) + 줄 + 글씨.
    길이 지나는 곳은 비우고 그 옆에 글씨를 둔다. 모두 막지 않음(지나갈 수 있다). 돌려준 값 = (기둥, 글씨)"""
    fence = raw.get("Analysis", {}).get("Sector", {}).get("Fence", [])
    if len(fence) < 2:
        return 0, 0
    # 꺾은선을 spacing 마다 점으로
    points = []
    carry = 0.0
    for i in range(len(fence) - 1):
        ax, ay, bx, by = fence[i]["X"], fence[i]["Y"], fence[i + 1]["X"], fence[i + 1]["Y"]
        length = math.hypot(bx - ax, by - ay)
        d = carry
        while d <= length:
            t = d / length
            points.append((ax + t * (bx - ax), ay + t * (by - ay), math.degrees(math.atan2(by - ay, bx - ax))))
            d += spacing_m
        carry = d - length
    posts, signs = 0, 0
    placed = []
    title = "공사 중 — 마탑 구간 밖"
    body = "이 너머는 아직 다듬지 않은 땅(지형 2차 그대로). 마탑 구간을 완성한 뒤 한 구역씩 넓힌다."
    for index, (u, v, angle) in enumerate(points):
        near_path = distance_to_paths(u, v, raw) < 14.0
        if near_path:
            placed.append(None)
            # 길 바로 옆(길을 비켜 12 m)에 글씨 — 지나가는 사람이 꼭 본다
            side = math.radians(angle)
            su, sv = u + 18.0 * math.cos(side), v + 18.0 * math.sin(side)
            spawn_label(placer, f"Label_Construction_Path_{index:02d}", "Construction", su, sv, 6.5, title, body, WARNING_LABEL_COLOR, 2.4, wrap=820.0)
            signs += 1
            continue
        placer.at(u, v, angle, "Construction/Posts", f"ConstructionPost_{index:02d}")
        placer.shape(CYLINDER, "Warning", "Pole", 0, 0, -0.6, 0.7, 0.7, 7.6, block=False)
        placer.shape(CYLINDER, "WarningDark", "Ring_0", 0, 0, 2.0, 0.78, 0.78, 0.8, block=False)
        placer.shape(CYLINDER, "WarningDark", "Ring_1", 0, 0, 4.6, 0.78, 0.78, 0.8, block=False)
        placer.shape(CUBE, "Warning", "Cap", 0, 0, 7.0, 1.2, 1.2, 0.5, block=False)
        x, y = placer.map_to_world(u, v)
        placed.append((u, v, placer.ground_cm(x, y)))
        posts += 1
        if index % sign_every == 0:
            spawn_label(placer, f"Label_Construction_{index:02d}", "Construction", u, v, 8.2, title, body, WARNING_LABEL_COLOR, 2.4, wrap=820.0)
            signs += 1
    # 띠: 이웃한 두 기둥 사이(둘 다 있을 때만) — 주황 줄 위, 검정 줄 아래
    for index in range(len(placed) - 1):
        a, b = placed[index], placed[index + 1]
        if a is None or b is None:
            continue
        (au, av, az), (bu, bv, bz) = a, b
        length = math.hypot(bu - au, bv - av)
        if length < 1.0:
            continue
        angle = math.degrees(math.atan2(bv - av, bu - au))
        placer.at((au + bu) / 2, (av + bv) / 2, angle, "Construction/Tape", f"ConstructionTape_{index:02d}")
        placer.base_z = (az + bz) / 2
        pitch = math.degrees(math.atan2((bz - az) / 100.0, length))
        placer.shape(CUBE, "Warning", "Upper", 0, 0, 0, length, 0.14, 0.45, pitch=pitch, block=False, shadow=False, z_center=5.6)
        placer.shape(CUBE, "WarningDark", "Lower", 0, 0, 0, length, 0.14, 0.35, pitch=pitch, block=False, shadow=False, z_center=3.2)
    return posts, signs


def build_natural_bridges(placer, raw, summary, map_to_world):
    """레벨 디자인 3차: 자연 개울 위 다리 — 생성기가 실제 곡선 교점에서 계산한 자리 · 방향 · 상판 높이대로, 수평으로 양쪽 둑에 얹는다.
    가로 널 · 세로 보 · 돌 교대(둑 위) · 난간을 모두 길 방향에 맞춘다(비뚤지 않게 — 널 길이만 조금씩 다르다)"""
    import random
    count = 0
    for bridge in summary.bridges:
        rng = random.Random(hash(str(bridge.name)) & 0xffff)
        rng.seed(int(bridge.center.x + bridge.center.y))
        span = bridge.span / 100.0
        width = bridge.width / 100.0
        # 지도 각도 = 90 − 월드 Yaw (Placer 는 지도 각도)
        angle = 90.0 - bridge.yaw
        u = bridge.center.y / 100.0 + raw["WorldOrigin"]["X"]
        v = bridge.center.x / 100.0 + raw["WorldOrigin"]["Y"]
        name = str(bridge.name)
        placer.at(u, v, angle, "Bridges", name)
        placer.base_z = bridge.center.z
        deck = 0.0      # 상판 윗면 = base_z
        beam_len = span + 2.4
        # 세로 보 셋(둑 위까지 간다)
        for ly in (-width * 0.36, 0.0, width * 0.36):
            placer.shape(CUBE, "Bark", f"Beam_{ly:+.1f}", 0, ly, 0, beam_len, 0.3, 0.3, z_center=deck - 0.28)
        # 가로 널 — 수평 · 직각, 길이만 조금씩(끝이 들쭉날쭉 — 손으로 켠 널)
        x = -beam_len * 0.5
        k = 0
        while x < beam_len * 0.5:
            w = rng.uniform(0.34, 0.46)
            placer.shape(CUBE, "Wood", f"Plank_{k:02d}", x + w * 0.5, rng.uniform(-0.05, 0.05), 0, w - 0.03, width + rng.uniform(-0.12, 0.2), 0.08, z_center=deck - 0.04)
            x += w
            k += 1
        # 돌 교대: 양쪽 둑에 보 끝을 받치는 돌 턱
        for sgn in (-1.0, 1.0):
            ax = sgn * (span * 0.5 + 0.1)
            bank = placer.ground_cm(*placer.world_xy(ax, 0.0))
            top = bridge.center.z - 40.0
            height = max(top - bank + 80.0, 80.0) / 100.0
            placer.base_z = bridge.center.z
            placer.shape(CUBE, "Stone", f"Abutment_{sgn:+.0f}", ax, 0, 0, 1.3, width + 1.3, height, z_center=(top / 100.0 - bridge.center.z / 100.0) - height * 0.5)
            for ly in (-(width * 0.5 + 0.35), width * 0.5 + 0.35):
                placer.shape(CUBE, "Stone", f"Wing_{sgn:+.0f}_{ly:+.1f}", ax - sgn * 0.45, ly, 0, 2.2, 0.7, height * 0.8, z_center=(top / 100.0 - bridge.center.z / 100.0) - height * 0.4)
        # 난간: 기둥(양쪽 3 개) + 위 가로대 — 직각
        for side in (1, -1):
            ly = side * (width * 0.5 + 0.04)
            for j in range(3):
                lx = (j - 1) * span * 0.42
                placer.shape(CUBE, "Bark", f"Post_{side:+d}_{j}", lx, ly, 0, 0.14, 0.14, 1.05, z_center=deck + 0.5)
            placer.shape(CUBE, "Wood", f"Rail_{side:+d}", 0, ly, 0, span * 0.92, 0.09, 0.11, block=False, z_center=deck + 1.02)
        count += 1
        unreal.log(f"다리 {name}: 길이 {span:.1f} m 폭 {width:.1f} m, 상판 {bridge.center.z / 100.0:.2f} m, 물 {bridge.water_z / 100.0:.2f} m, 길과 개울 사이각 {bridge.crossing_angle:.0f}°")
    return count


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
            if creek.get("bNatural"):
                continue   # 자연 개울 다리는 build_natural_bridges(생성기가 곡선 교점에서 계산)
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
