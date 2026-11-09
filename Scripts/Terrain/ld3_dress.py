# 레벨 디자인 3차(docs/dev/level-design.md 「10」) — dress_openbeta.py 가 부른다(태그 · 폴리지 타입은 section_dress 와 같은 규칙, 다시 돌리면 걷고 다시 놓는다).
#
#   돌담      ld4_walls        이유가 있는 자리에만(레이아웃 StoneWalls) — 납작한 돌이 가로로 이어진 단 + 갓돌
#   개울      creek_dress      소 · 여울 · 굽이에 맞춘 돌 · 자갈 · 갈대 · 고사리(생성기의 폭 · 소 값을 그대로 읽는다)
#   샘        spring           바위 틈(뒤 · 옆 바위 + 내민 바위) 아래 작은 웅덩이, 이끼 돌 · 고사리, 물이 솟는 동심원
#   짐수레    cart             큰길 굽이 길섶 — 바퀴 빠져 기울고 짐이 도랑 쪽으로 쏟아진 이야기
#   능선      ridge_rocks      등성이 바위(실루엣) · 급사면 바위 · 밑동 너덜
#   빈 곳     fill_*           작은 콘셉트는 여러 자리에 여러 변형으로, 큰 콘셉트는 1 ~ 2 번 더
#
# 좌표: u = 동, v = 북(지도 m). 놓기는 모두 폴리지 인스턴스(SectionDresser.instances)로 모은다.
import json
import math
import os
import random

import unreal

import dress_assets
import ld4_walls
import section_dress

WALL_STONES = ["FT_Dress_Wall1", "FT_Dress_Wall2", "FT_Dress_Wall3", "FT_Dress_Wall4", "FT_Dress_Wall5"]
MOSS_SMALL = ["FT_Dress_MossSmall1", "FT_Dress_MossSmall3", "FT_Dress_MossSmall5"]
PEBBLES = ["FT_Dress_Pebble1", "FT_Dress_Pebble2", "FT_Dress_Pebble3"]
FERNS = ["FT_Dress_Fern1", "FT_Dress_Fern1", "FT_Dress_Tuft1"]   # 넓은 잎 풀(Plant_19) — 꽃대가 달린 Plant 메시는 라벤더처럼 보여 쓰지 않는다
REEDS = ["FT_Dress_Reed1", "FT_Dress_Reed2", "FT_Dress_Reed3"]
TUFTS = ["FT_Dress_Tuft1", "FT_Dress_Tuft2"]
FLOWERS = ["FT_Dress_Flower1", "FT_Dress_Flower2", "FT_Dress_Flower3"]
ROCKS_BIG = ["FT_Dress_RockLarge", "FT_Dress_RockCluster1", "FT_Dress_RockCluster2", "FT_Dress_Boulder1", "FT_Dress_Boulder2"]
ROCKS_SMALL = ["FT_Dress_RockSmall1", "FT_Dress_RockSmall2", "FT_Dress_RockSmall3", "FT_Dress_RockSmall4", "FT_Dress_RockSmall5"]


def noise1(x, seed=0.0):
    """부드러운 1차원 값 노이즈(−1 ~ 1)"""
    def h(i):
        return math.fmod(abs(math.sin(i * 127.1 + seed * 311.7) * 43758.5453), 1.0) * 2.0 - 1.0
    i = math.floor(x)
    t = x - i
    t = t * t * (3 - 2 * t)
    return h(i) * (1 - t) + h(i + 1) * t


class Level3:
    def __init__(self, dresser, ctx):
        self.d = dresser
        self.c = ctx
        self.lib = ctx["lib"]
        self.raw = ctx["raw"]
        self.summary = ctx["summary"]
        self.m2w = ctx["map_to_world"]
        self.w2m = ctx["world_to_map"]
        self.rng = random.Random(20261111)
        self.bounds = {}
        for name, row in dress_assets.FOLIAGE.items():
            mesh = unreal.load_asset(row[0])
            b = mesh.get_bounds()
            self.bounds[name] = (b.origin.x, b.origin.y, b.origin.z, b.box_extent.x, b.box_extent.y, b.box_extent.z)
        self.bridge_spots = [(*self.w2m(b.center.x, b.center.y), b.span / 200.0 + 3.0) for b in self.summary.bridges]
        self.wall_report = []
        self.taken = []    # (u, v, r) — 소품 묶음끼리 겹치지 않게
        self.counts = {}

    # ── 땅 · 놓기 ──
    def ground(self, u, v):
        x, y = self.m2w(u, v)
        return self.lib.get_ground_height(x, y)

    def slope(self, u, v):
        x, y = self.m2w(u, v)
        return self.lib.get_slope_degrees(x, y)

    def downhill(self, u, v, d=2.5):
        gx = self.ground(u + d, v) - self.ground(u - d, v)
        gy = self.ground(u, v + d) - self.ground(u, v - d)
        n = math.hypot(gx, gy) or 1.0
        return -gx / n, -gy / n

    def bump(self, kind, n=1):
        self.counts[kind] = self.counts.get(kind, 0) + n

    def put(self, ft, u, v, z_cm, map_yaw, scale, pitch=0.0, roll=0.0):
        """피벗을 (u, v, z_cm) 에. map_yaw = 메시 +X 의 지도 각도(동 = 0, 북 = 90). pitch + = 진행 방향이 올라감(UE 회전 그대로)"""
        x, y = self.m2w(u, v)
        s = scale if isinstance(scale, tuple) else (scale, scale, scale)
        self.d.instances.setdefault(ft, []).append(
            unreal.Transform(unreal.Vector(x, y, z_cm), unreal.Rotator(roll, pitch, 90.0 - map_yaw), unreal.Vector(*s)))

    def sit(self, ft, u, v, map_yaw, scale, embed=0.18, pitch=0.0, roll=0.0, lift_cm=0.0):
        """바운드 바닥을 그 자리 땅에 묻어 앉힌다(embed = 높이의 비율만큼 땅속)"""
        s = scale if isinstance(scale, tuple) else (scale, scale, scale)
        _, _, oz, _, _, ez = self.bounds[ft]
        height = 2.0 * ez * s[2]
        self.put(ft, u, v, self.ground(u, v) - embed * height - s[2] * (oz - ez) + lift_cm, map_yaw, s, pitch, roll)

    def prim(self, ft, u, v, z_m, sx, sy, sz, map_yaw=0.0, pitch=0.0, roll=0.0):
        """기본 도형(폴리지) — z_m = 도형 가운데가 그 자리 땅 위 몇 m. 크기는 m"""
        self.put(ft, u, v, self.ground(u, v) + z_m * 100.0, map_yaw, (sx, sy, sz), pitch, roll)

    def frame(self, u, v, angle):
        a = math.radians(angle)
        ca, sa = math.cos(a), math.sin(a)
        return lambda lx, ly: (u + lx * ca - ly * sa, v + lx * sa + ly * ca)

    def plant_ok(self, u, v):
        """풀 · 돌을 놓아도 되는가 — 다리 위 · 길 위에는 놓지 않는다"""
        import landmark_greybox
        if any(math.hypot(u - a, v - b) < r for a, b, r in self.bridge_spots):
            return False
        return landmark_greybox.distance_to_paths(u, v, self.raw) > 2.4

    def free(self, u, v, r, margin=0.0):
        return all(math.hypot(u - a, v - b) >= r + c + margin for a, b, c in self.taken)

    def claim(self, u, v, r):
        self.taken.append((u, v, r))

    def find_flat(self, u, v, radius, max_slope, clear_margin=2.0, tries=60):
        """(u, v) 둘레에서 경사 max_slope 아래이고 비어 있는 자리"""
        best = None
        for k in range(tries):
            a = self.rng.uniform(0, math.tau)
            r = 0.0 if k == 0 else radius * math.sqrt(self.rng.random())
            pu, pv = u + r * math.cos(a), v + r * math.sin(a)
            if not self.c["clear_of"](pu, pv, clear_margin):
                continue
            s = self.slope(pu, pv)
            if s < max_slope and (best is None or s < best[0]):
                best = (s, pu, pv)
                if s < max_slope * 0.5:
                    break
        return None if best is None else (best[1], best[2])

    # ── 돌담(4차 — ld4_walls.py, 자리 · 이유는 레이아웃 StoneWalls) ──
    def _wall_ok(self, u, v):
        tower = self.raw["TowerHill"]
        if math.hypot(u - tower["Center"]["X"], v - tower["Center"]["Y"]) < tower["PlateauRadius"] + 8:
            return False
        import landmark_greybox
        return self.c["in_sector"](u, v) and not self.c["near_water"](u, v, 1.5) and landmark_greybox.distance_to_paths(u, v, self.raw) > 3.4 and self.slope(u, v) < 33

    def write_wall_report(self, root, walls):
        path = os.path.join(root, "Saved", "Terrain", "ld4_wall_report.json")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"walls": walls, "stones": self.wall_stones}, f, ensure_ascii=False)
        return {"walls": len(walls), "stones": len(self.wall_stones), "length_m": round(sum(w["length_m"] for w in walls), 1)}

    # ── 개울 ──
    def _water_bodies(self):
        return [b for b in self.summary.water if b.surface_mesh and str(b.kind) == "Creek"]

    def creek_dress(self):
        rng = self.rng
        rocks = pebbles = plants = 0
        for body in self._water_bodies():
            pts = list(body.points)
            widths = list(body.widths)
            pools = list(body.pools)
            uv = [self.w2m(p.x, p.y) for p in pts]
            for i in range(2, len(uv) - 2):
                u, v = uv[i]
                du, dv = uv[i + 1][0] - uv[i - 1][0], uv[i + 1][1] - uv[i - 1][1]
                n = math.hypot(du, dv) or 1.0
                du, dv = du / n, dv / n
                half = widths[i] / 200.0
                pool = pools[i]
                level = pts[i].z
                # 곡률 — 굽이 바깥쪽에는 큰 돌, 안쪽에는 자갈
                pu, pv = uv[i - 1]
                qu, qv = uv[i + 1]
                cross = (u - pu) * (qv - v) - (v - pv) * (qu - u)
                bend_side = 1.0 if cross > 0 else -1.0   # 왼쪽으로 꺾이면 왼쪽이 안쪽
                # 여울: 물 위로 머리를 내민 돌 — 물길 가운데쪽
                if pool < 0.45 and rng.random() < 0.34:
                    off = rng.uniform(-0.75, 0.75) * half
                    ru, rv = u - dv * off, v + du * off
                    if not self.plant_ok(ru, rv):
                        continue
                    s = rng.uniform(0.2, 0.42) * (0.7 + half * 0.4)
                    ft = rng.choice(MOSS_SMALL)
                    _, _, oz, _, _, ez = self.bounds[ft]
                    h = 2.0 * ez * s
                    self.put(ft, ru, rv, level - h * rng.uniform(0.35, 0.6) - s * (oz - ez), rng.uniform(0, 360), (s * rng.uniform(1.0, 1.5), s, s * rng.uniform(0.6, 0.9)), rng.uniform(-14, 14), rng.uniform(-14, 14))
                    rocks += 1
                # 소 가장자리: 잠긴 자갈(물 표면 아래)
                if pool > 0.6 and rng.random() < 0.3:
                    off = rng.uniform(0.5, 0.95) * half * rng.choice((-1, 1))
                    self.sit(rng.choice(PEBBLES), u - dv * off, v + du * off, rng.uniform(0, 360), rng.uniform(1.2, 3.0), embed=0.3)
                    pebbles += 1
                # 굽이 바깥 둑의 큰 돌, 때때로
                if rng.random() < 0.11:
                    sign = -bend_side
                    off = half + rng.uniform(0.6, 2.2)
                    ru, rv = u - dv * sign * off, v + du * sign * off
                    if self.slope(ru, rv) < 42 and self.plant_ok(ru, rv):
                        ft = rng.choice(MOSS_SMALL + ["FT_Dress_MossLarge", "FT_Dress_Boulder2"])
                        s = rng.uniform(0.5, 1.0) if ft in MOSS_SMALL else rng.uniform(0.32, 0.6)
                        self.sit(ft, ru, rv, rng.uniform(0, 360), (s * rng.uniform(1, 1.3), s, s * rng.uniform(0.7, 1.0)), embed=rng.uniform(0.25, 0.4), pitch=rng.uniform(-10, 10), roll=rng.uniform(-10, 10))
                        rocks += 1
                # 안쪽 모래톱의 자갈 · 흩어진 돌
                if rng.random() < 0.3:
                    off = half + rng.uniform(0.2, 1.8)
                    ru, rv = u - dv * bend_side * off, v + du * bend_side * off
                    self.sit(rng.choice(PEBBLES + ["FT_Dress_Scatter"]), ru, rv, rng.uniform(0, 360), rng.uniform(1.4, 3.2), embed=0.25)
                    pebbles += 1
                # 물가 식물: 소 둘레는 갈대, 비탈 쪽은 고사리, 여울은 풀포기
                for sgn in (-1, 1):
                    if rng.random() < 0.42:
                        off = half + rng.uniform(0.25, 1.9 + pool)
                        ru, rv = u - dv * sgn * off, v + du * sgn * off
                        steep = self.slope(ru, rv)
                        if steep > 40 or not self.plant_ok(ru, rv):
                            continue
                        if self.ground(ru, rv) < level - 4.0:
                            continue   # 물속에 풀을 심지 않는다
                        if pool > 0.45 and rng.random() < 0.7:
                            ft, s = rng.choice(REEDS), (rng.uniform(1.0, 1.5), rng.uniform(1.0, 1.5), rng.uniform(1.9, 3.1))
                        elif steep > 14 or rng.random() < 0.28:
                            ft, s = rng.choice(FERNS), rng.uniform(0.7, 1.35)
                        else:
                            ft, s = rng.choice(TUFTS), rng.uniform(0.8, 1.5)
                        self.sit(ft, ru, rv, rng.uniform(0, 360), s, embed=0.04)
                        plants += 1
            # V 자 상류 비탈: 맨흙(경사 28° 넘어 풀 지도가 비는 곳)에 고사리 · 풀포기를 빽빽이 — 비탈이 물길을 감싼다
            for i in range(0, len(uv) - 2, 1):
                if i * 2.0 > 130.0:
                    break
                u, v = uv[i]
                du, dv = uv[i + 1][0] - uv[i][0], uv[i + 1][1] - uv[i][1]
                n = math.hypot(du, dv) or 1.0
                for sgn in (-1, 1):
                    for k in range(3):
                        off = widths[i] / 200.0 + rng.uniform(0.8, 7.5)
                        ru, rv = u - dv / n * sgn * off + rng.uniform(-1, 1), v + du / n * sgn * off + rng.uniform(-1, 1)
                        if self.ground(ru, rv) < pts[i].z + 8.0 or not self.plant_ok(ru, rv):
                            continue
                        ft = rng.choice(FERNS) if rng.random() < 0.45 else rng.choice(TUFTS)
                        self.sit(ft, ru, rv, rng.uniform(0, 360), rng.uniform(0.9, 1.7), embed=0.05)
                        plants += 1
            self.claim(uv[0][0], uv[0][1], 12)
        # 연못 물가: 갈대 띠(한쪽으로 몰린다) · 돌 · 고사리, 물이 드는 어귀
        for pond in self.raw["Ponds"]:
            if not pond.get("Irregularity"):
                continue
            cu, cv, r = pond["Center"]["X"], pond["Center"]["Y"], pond["Radius"]
            for k in range(190):
                a = rng.uniform(0, math.tau)
                dense = 0.5 + 0.5 * math.sin(a * 2.0 + 1.3)
                if rng.random() > 0.3 + 0.7 * dense:
                    continue
                d = r * (1.0 + rng.uniform(-0.12, 0.34))
                u, v = cu + d * math.cos(a), cv + d * math.sin(a)
                g = self.ground(u, v)
                level = self.c["water_level"](pond["Name"]) * 100.0
                if g < level - 28:
                    continue
                s = rng.uniform(1.1, 1.7)
                self.sit(rng.choice(REEDS) if rng.random() < 0.72 else rng.choice(TUFTS), u, v, rng.uniform(0, 360), (s, s, s * rng.uniform(1.9, 3.2)), embed=0.05)
                plants += 1
        self.bump("CreekRock", rocks)
        self.bump("CreekPebble", pebbles)
        self.bump("CreekPlant", plants)
        return rocks, pebbles, plants

    # ── 샘 ──
    def spring(self, body, big=True):
        """물머리 — 샘은 비탈 밑 후미진 곳에서 바위 틈으로 솟는다: 뒤 · 옆 바위 벽 + 위로 내민 바위 아래 작은 웅덩이"""
        pts = list(body.points)
        cu, cv = self.w2m(pts[0].x, pts[0].y)
        nu, nv = self.w2m(pts[3].x, pts[3].y)
        down_a = math.atan2(nv - cv, nu - cu)
        out = lambda lx, ly: (cu + lx * math.cos(down_a) - ly * math.sin(down_a), cv + lx * math.sin(down_a) + ly * math.cos(down_a))
        rng = self.rng
        level = pts[0].z
        pool = (body.widths[0] / 200.0)
        back_angle = math.degrees(down_a) + 180.0
        # 뒤 바위 벽: 큰 바위 셋이 말굽꼴로 둘러싼다 — 앞(하류)만 열린다
        for (lx, ly, ft, s, yaw) in [(-pool - 1.3, 0.2, "FT_Dress_MossBoulder", 1.05, 20), (-pool - 0.2, 3.0, "FT_Dress_MossLarge", 1.5, 110), (-pool - 0.2, -3.0, "FT_Dress_MossLarge", 1.35, 250),
                                     (-pool - 2.6, 2.3, "FT_Dress_RockCluster1", 1.5, 60), (-pool - 2.6, -2.4, "FT_Dress_RockCluster2", 1.25, 200)]:
            u, v = out(lx, ly)
            self.sit(ft, u, v, back_angle + yaw + rng.uniform(-12, 12), (s, s * rng.uniform(0.85, 1.1), s * rng.uniform(0.85, 1.05)), embed=0.22, pitch=rng.uniform(-8, 8), roll=rng.uniform(-8, 8))
        # 내민 바위: 납작한 판이 뒤쪽 바위 위에 걸쳐 웅덩이 위로 나온다 — 그 그늘 밑에서 물이 나온다
        u, v = out(-pool * 0.35, 0.0)
        ft = "FT_Dress_MossLarge"
        _, _, oz, _, _, ez = self.bounds[ft]
        s = (1.05, 1.15, 0.5)
        self.put(ft, u, v, level + 1.25 * 100.0 - s[2] * (oz - ez) * 0 , back_angle + 8, s, -9.0, 4.0)
        # 웅덩이 가장자리 이끼 돌(앞쪽은 띄엄띄엄 — 물이 빠져나가는 틈)
        for k in range(16):
            a = math.tau * k / 16 + rng.uniform(-0.1, 0.1)
            if abs(math.atan2(math.sin(a), math.cos(a))) < 0.5:
                continue   # 하류 쪽 틈
            r = pool + rng.uniform(0.0, 0.5)
            u, v = out(r * math.cos(a), r * math.sin(a))
            ft = rng.choice(MOSS_SMALL)
            s = rng.uniform(0.32, 0.6)
            self.sit(ft, u, v, rng.uniform(0, 360), (s * 1.25, s, s * 0.75), embed=0.3, pitch=rng.uniform(-10, 10), roll=rng.uniform(-10, 10))
        # 물속 자갈 · 물 솟는 자리
        for k in range(10):
            a = rng.uniform(0, math.tau)
            r = rng.uniform(0.3, pool * 0.85)
            u, v = out(r * math.cos(a), r * math.sin(a))
            self.sit(rng.choice(PEBBLES), u, v, rng.uniform(0, 360), rng.uniform(1.4, 2.6), embed=0.3)
        # 둘레 고사리 · 갈대 — 바위 틈마다 무성하게
        for k in range(46 if big else 26):
            a = rng.uniform(0, math.tau)
            r = pool + rng.uniform(1.2, 7.5)
            u, v = out(r * math.cos(a), r * math.sin(a))
            if self.slope(u, v) > 50:
                continue
            s = rng.uniform(0.9, 1.9)
            if rng.random() < 0.62:
                self.sit(rng.choice(FERNS), u, v, rng.uniform(0, 360), s, embed=0.05)
            else:
                self.sit(rng.choice(REEDS), u, v, rng.uniform(0, 360), (s, s, s * rng.uniform(1.7, 2.6)), embed=0.05)
        # 물 솟는 동심원 + 뒤편 나무 몇 — 그늘
        u, v = out(0.0, 0.0)
        x, y = self.m2w(u, v)
        for k in range(3 if big else 1):
            a = rng.uniform(math.pi * 0.7, math.pi * 1.3)
            tu, tv = out(10 * math.cos(a), 10 * math.sin(a) + rng.uniform(-3, 3))
            self.sit(rng.choice(["FT_Dress_Broadleaf1", "FT_Dress_Broadleaf2"]), tu, tv, rng.uniform(0, 360), rng.uniform(0.62, 0.82), embed=0.02)
        return (x, y, level + 2.0, pool)

    def spring_ripple(self, x, y, z, pool, material):
        actor = self.c["actors"].spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 0))
        actor.set_actor_label("Spring_Ripple")
        actor.set_folder_path("LevelDress/Sections/Spring")
        actor.set_actor_scale3d(unreal.Vector(pool * 2.0, pool * 2.0, 1.0))   # Plane 1 m × 배율
        comp = actor.static_mesh_component
        comp.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
        comp.set_material(0, material)
        comp.set_collision_profile_name("NoCollision")
        comp.set_editor_property("cast_shadow", False)
        actor.set_editor_property("tags", [unreal.Name("LevelDress"), unreal.Name("LevelDress.Blockout")])

    # ── 짐수레 ──
    def cart(self, u, v, road_angle, ditch_side, variant):
        """road_angle = 길 방향(지도 각도), ditch_side = 길에서 바깥쪽 +1/−1. 바퀴가 빠져 기운 수레 — 짐이 도랑 쪽으로 쏟아졌다"""
        rng = random.Random(4100 + variant * 7)
        f = self.frame(u, v, road_angle)
        roll_tilt = 15.0 * ditch_side     # 도랑 쪽으로 기운다
        pitch_tilt = -3.0
        z_body = 0.85
        def body(ft, lx, ly, z, sx, sy, sz, yaw=0.0, pitch=0.0, roll=0.0):
            pu, pv = f(lx, ly)
            self.prim(ft, pu, pv, z - ly * ditch_side * math.sin(math.radians(15.0)), sx, sy, sz, road_angle + yaw, pitch_tilt + pitch, roll_tilt + roll)
        body("FT_Block_Wood", 0, 0, z_body, 2.6, 1.3, 0.14)                          # 바닥판
        for side, ly in (("A", 0.6), ("B", -0.6)):                                    # 옆널
            body("FT_Block_Wood", 0, ly, z_body + 0.22, 2.6, 0.08, 0.32)
        body("FT_Block_Wood", 1.28, 0, z_body + 0.22, 0.08, 1.3, 0.32)               # 앞널
        # 뒤널은 경첩이 빠져 땅에 떨어졌다
        pu, pv = f(-1.75, 0.2 * ditch_side)
        self.prim("FT_Block_Wood", pu, pv, 0.08, 0.08, 1.2, 0.34, road_angle + rng.uniform(-14, 14), rng.uniform(70, 88), 6.0)
        # 멀쩡한 바퀴(높은 쪽 둘 — 길 쪽, 기울어 들림)
        for lx in (0.9, -0.9):
            pu, pv = f(lx, -ditch_side * 0.74)
            self.prim("FT_Block_WoodCyl", pu, pv, 0.6, 1.1, 1.1, 0.11, road_angle, 0.0, 90.0 + roll_tilt)
        # 도랑 쪽 뒤 바퀴는 끼인 채 진창에 반쯤 묻혔다
        pu, pv = f(-0.9, ditch_side * 0.78)
        self.prim("FT_Block_WoodCyl", pu, pv, 0.34, 1.1, 1.1, 0.11, road_angle + 8.0, 0.0, 90.0 + roll_tilt * 1.6)
        # 도랑 쪽 앞 바퀴는 빠져 땅에 눕고, 부러진 차축이 땅을 찍었다
        pu, pv = f(1.9, ditch_side * 1.6)
        self.prim("FT_Block_WoodCyl", pu, pv, 0.07, 1.1, 1.1, 0.11, road_angle, 5.0, 3.0)
        pu, pv = f(0.95, ditch_side * 0.68)
        self.prim("FT_Block_Wood", pu, pv, 0.28, 0.1, 0.95, 0.1, road_angle + 12.0, 0.0, roll_tilt * 2.2)
        # 끌채 — 앞이 땅에 닿았다
        pu, pv = f(2.5, -0.08 * ditch_side)
        self.prim("FT_Block_Wood", pu, pv, 0.34, 2.4, 0.1, 0.1, road_angle + 4.0, -18.0, 0.0)
        # 쏟아진 짐(도랑 쪽으로 — 상자 · 자루 · 통 · 깨진 마법 재료 병)
        crates = 3 + variant % 2
        for k in range(crates):
            lx, ly = rng.uniform(-1.4, 1.6), ditch_side * rng.uniform(1.05, 3.2)
            pu, pv = f(lx, ly)
            s = rng.uniform(0.5, 0.85)
            self.prim("FT_Block_Wood", pu, pv, s * 0.42, s, s, s * rng.uniform(0.7, 1.0), rng.uniform(0, 360), rng.uniform(-22, 22), rng.uniform(-22, 22))
        for k in range(4):
            lx, ly = rng.uniform(-2.0, 1.4), ditch_side * rng.uniform(0.9, 2.8)
            pu, pv = f(lx, ly)
            s = rng.uniform(0.5, 0.78)
            self.prim("FT_Block_Sack", pu, pv, s * 0.3, s * 1.2, s * 0.85, s * 0.6, rng.uniform(0, 360), rng.uniform(-10, 10), rng.uniform(-10, 10))
        pu, pv = f(-1.3 + variant * 0.3, ditch_side * 3.2)
        self.prim("FT_Block_WoodCyl", pu, pv, 0.34, 0.68, 0.68, 0.88, road_angle + 70.0, 0.0, 88.0)                # 굴러간 통
        for k in range(7):
            lx, ly = rng.uniform(-1.0, 2.0), ditch_side * rng.uniform(1.3, 3.8)
            pu, pv = f(lx, ly)
            self.prim("FT_Block_Shard", pu, pv, 0.1, 0.1, 0.1, 0.2, rng.uniform(0, 360), rng.uniform(-40, 40), rng.uniform(-40, 40))   # 빛나는 깨진 병 조각
        # 끌린 자국: 도랑 쪽에 흩어진 잔돌 · 눌린 풀
        for k in range(10):
            pu, pv = f(rng.uniform(-4.0, 3.0), ditch_side * rng.uniform(0.2, 1.4))
            self.sit("FT_Dress_Scatter", pu, pv, rng.uniform(0, 360), rng.uniform(1.0, 2.2), embed=0.3)
        self.claim(u, v, 8)
        self.bump("Cart")

    def carts(self):
        for name, side in (("Poi_Meadow_Cart", -1), ("Poi_Pasture_Cart", 1)):
            poi = next(p for p in self.raw["Pois"] if p["Name"] == name)
            u, v = poi["Center"]["X"], poi["Center"]["Y"]
            # 길 위 가장 가까운 점 · 방향 — 길섶(반폭 + 3 m)에 놓는다
            best = None
            for line_name, line in self.d.paths.items():
                arc = 0.0
                while arc < line.length:
                    pu, pv, d = line.at(arc)
                    dist = math.hypot(pu - u, pv - v)
                    if best is None or dist < best[0]:
                        best = (dist, pu, pv, d, line_name)
                    arc += 2.0
            _, pu, pv, (dx, dy), line_name = best
            road_angle = math.degrees(math.atan2(dy, dx))
            width = next(p["Width"] for p in self.raw["Paths"] if p["Name"] == line_name)
            off = width * 0.5 + 3.4
            self.cart(pu - dy * side * off, pv + dx * side * off, road_angle, side, 0 if name == "Poi_Meadow_Cart" else 1)

    # ── 능선 바위 ──
    def ridge_rocks(self):
        rng = self.rng
        tors = outcrops = scree = 0
        for ridge in self.raw.get("Ridgelines", []):
            if not ridge.get("bSharpCrest"):
                continue
            line = section_dress.Polyline(section_dress.catmull_rom(ridge["Points"]))
            is_main = ridge["Name"].startswith("Ridge_") and not ridge["Name"].startswith("Ridge_Hog")
            scarp = ridge.get("ScarpSide", 0)
            arc = rng.uniform(5, 25)
            while arc < line.length:
                u, v, (dx, dy) = line.at(arc)
                # 등성이 위 바위 — 실루엣이 끊기지 않게, 주능선일수록 자주
                if self.c["clear_of"](u, v, 3.0) and rng.random() < (0.72 if is_main else 0.35):
                    ft = rng.choice(ROCKS_BIG[:3])
                    s = rng.uniform(0.75, 1.5) if is_main else rng.uniform(0.45, 0.8)
                    self.sit(ft, u + rng.uniform(-2, 2), v + rng.uniform(-2, 2), math.degrees(math.atan2(dy, dx)) + rng.uniform(-30, 30), (s * rng.uniform(1.0, 1.35), s, s * rng.uniform(0.9, 1.4)), embed=0.28, pitch=rng.uniform(-14, 14), roll=rng.uniform(-14, 14))
                    tors += 1
                # 급사면: 등성이에서 내려가며 경사 재어 바위(≥ 30°) · 너덜(밑동, 15 ~ 30°)
                if scarp:
                    width = ridge["LeftWidth"] if scarp > 0 else ridge["RightWidth"]
                    nu, nv = -dy * scarp, dx * scarp
                    for k in range(5):
                        d = rng.uniform(3.0, width * 0.92)
                        pu, pv = u + nu * d + rng.uniform(-5, 5), v + nv * d + rng.uniform(-5, 5)
                        if not self.c["in_sector"](pu, pv) or not self.c["clear_of"](pu, pv, 1.5):
                            continue
                        s0 = self.slope(pu, pv)
                        if s0 >= 30.0 and rng.random() < 0.65:
                            ft = rng.choice(ROCKS_BIG)
                            s = rng.uniform(0.55, 1.2)
                            self.sit(ft, pu, pv, rng.uniform(0, 360), (s * rng.uniform(1.0, 1.4), s, s * rng.uniform(0.75, 1.1)), embed=0.3, pitch=rng.uniform(-18, 18), roll=rng.uniform(-18, 18))
                            outcrops += 1
                        elif 13.0 <= s0 < 30.0 and d > width * 0.45:
                            # 너덜 — 밑동으로 흘러내린 돌 무리
                            for j in range(rng.randint(3, 7)):
                                qu, qv = pu + rng.gauss(0, 2.4), pv + rng.gauss(0, 2.4)
                                if self.c["clear_of"](qu, qv, 0.5):
                                    self.sit(rng.choice(WALL_STONES + PEBBLES), qu, qv, rng.uniform(0, 360), rng.uniform(0.3, 0.7), embed=0.3, pitch=rng.uniform(-20, 20), roll=rng.uniform(-20, 20))
                                    scree += 1
                arc += rng.uniform(14, 28) if is_main else rng.uniform(22, 40)
        self.bump("RidgeTor", tors)
        self.bump("RidgeOutcrop", outcrops)
        self.bump("Scree", scree)
        return tors, outcrops, scree

    # ── 작은 콘셉트(여러 자리 · 여러 변형) ──
    def cairn(self, u, v, rng):
        n = rng.randint(4, 8)
        z = 0.0
        size = rng.uniform(0.55, 0.8)
        for k in range(n):
            pu, pv = u + rng.uniform(-0.12, 0.12), v + rng.uniform(-0.12, 0.12)
            ft = rng.choice(WALL_STONES)
            s = size * (1.0 - k / (n + 3.0)) * rng.uniform(0.85, 1.1)
            _, _, oz, _, _, ez = self.bounds[ft]
            h = 2.0 * ez * s * 0.72
            self.put(ft, pu, pv, self.ground(u, v) + z * 100.0 - (0.14 * h) - s * 0.72 * (oz - ez), rng.uniform(0, 360), (s, s, s * 0.72), rng.uniform(-9, 9), rng.uniform(-9, 9))
            z += h / 100.0 * 0.82
        for k in range(rng.randint(2, 4)):
            a = rng.uniform(0, math.tau)
            self.sit(rng.choice(TUFTS + FERNS), u + 0.9 * math.cos(a), v + 0.9 * math.sin(a), rng.uniform(0, 360), rng.uniform(0.7, 1.2), embed=0.05)

    def log_fallen(self, u, v, rng):
        yaw = rng.uniform(0, 360)
        down = self.downhill(u, v)
        yaw = math.degrees(math.atan2(-down[0], down[1])) + rng.uniform(-25, 25)     # 비탈을 가로질러 누운다
        self.sit(rng.choice(["FT_Dress_Trunk1", "FT_Dress_Trunk2"]), u, v, yaw, rng.uniform(3.6, 5.4), embed=0.3, pitch=rng.uniform(-3, 3))
        for k in range(rng.randint(4, 9)):
            a = rng.uniform(0, math.tau)
            d = rng.uniform(1.0, 3.5)
            self.sit(rng.choice(FERNS + TUFTS), u + d * math.cos(a), v + d * math.sin(a), rng.uniform(0, 360), rng.uniform(0.6, 1.2), embed=0.05)

    def stump(self, u, v, rng):
        r = rng.uniform(0.7, 1.15)
        tilt = (rng.uniform(-3, 3), rng.uniform(-3, 3))
        self.prim("FT_Block_BarkCyl", u, v, 0.28, r, r, 0.75, rng.uniform(0, 360), tilt[0], tilt[1])
        for k in range(rng.randint(3, 7)):
            a = rng.uniform(0, math.tau)
            d = r * rng.uniform(0.55, 0.85)
            pu, pv = u + d * math.cos(a), v + d * math.sin(a)
            s = rng.uniform(0.1, 0.2)
            self.prim("FT_Block_Mushroom", pu, pv, rng.uniform(0.05, 0.42), s, s, s * 0.45)
        for k in range(rng.randint(3, 6)):
            a = rng.uniform(0, math.tau)
            d = rng.uniform(0.7, 1.6)
            self.sit(rng.choice(FERNS), u + d * math.cos(a), v + d * math.sin(a), rng.uniform(0, 360), rng.uniform(0.6, 1.1), embed=0.05)

    def flower_patch(self, u, v, rng):
        n = rng.randint(14, 30)
        pick = rng.sample(FLOWERS, 2)
        spread = rng.uniform(2.0, 4.8)
        for k in range(n):
            self.sit(pick[0] if rng.random() < 0.72 else pick[1], u + rng.gauss(0, spread), v + rng.gauss(0, spread * 0.8), rng.uniform(0, 360), rng.uniform(0.75, 1.5), embed=0.03)

    def rock_cluster(self, u, v, rng):
        big = rng.choice(ROCKS_BIG[:3])
        s = rng.uniform(0.6, 1.25)
        self.sit(big, u, v, rng.uniform(0, 360), (s * rng.uniform(1.0, 1.4), s, s * rng.uniform(0.75, 1.1)), embed=0.28, pitch=rng.uniform(-12, 12), roll=rng.uniform(-12, 12))
        for k in range(rng.randint(3, 8)):
            a = rng.uniform(0, math.tau)
            d = rng.uniform(1.2, 3.8)
            s2 = rng.uniform(0.3, 0.75)
            self.sit(rng.choice(WALL_STONES + MOSS_SMALL), u + d * math.cos(a), v + d * math.sin(a), rng.uniform(0, 360), (s2 * 1.2, s2, s2 * 0.8), embed=0.28, pitch=rng.uniform(-16, 16), roll=rng.uniform(-16, 16))
        for k in range(rng.randint(3, 7)):
            a = rng.uniform(0, math.tau)
            d = rng.uniform(1.0, 3.2)
            self.sit(rng.choice(TUFTS + FERNS + FLOWERS), u + d * math.cos(a), v + d * math.sin(a), rng.uniform(0, 360), rng.uniform(0.6, 1.3), embed=0.05)

    def fence_piece(self, u, v, rng, length=None):
        length = length or rng.uniform(9, 22)
        ang = rng.uniform(0, 360)
        f = self.frame(u, v, ang)
        step = 2.8
        n = int(length / step)
        prev = None
        for k in range(n + 1):
            lean = (rng.uniform(-9, 9), rng.uniform(-9, 9))
            pu, pv = f(k * step - length * 0.5, rng.uniform(-0.1, 0.1))
            if rng.random() < 0.12 or self.slope(pu, pv) > 28:
                prev = None
                continue
            self.prim("FT_Block_Wood", pu, pv, 0.42, 0.13, 0.13, 1.25, rng.uniform(0, 90), lean[0], lean[1])
            if prev is not None and rng.random() < 0.82:
                mu, mv = (prev[0] + pu) * 0.5, (prev[1] + pv) * 0.5
                drop = rng.choice((0.0, 0.0, 0.0, rng.uniform(0.3, 0.9)))
                a2 = math.degrees(math.atan2(pv - prev[1], pu - prev[0]))
                g0, g1 = self.ground(*prev), self.ground(pu, pv)
                pitch = math.degrees(math.atan2((g1 - g0) / 100.0, step))
                rail_z = 0.78 - drop * 0.5
                self.prim("FT_Block_Wood", mu, mv, rail_z, step, 0.07, 0.1, a2, pitch + rng.uniform(-4, 4) + drop * 10, rng.uniform(-5, 5))
                if rng.random() < 0.6:
                    self.prim("FT_Block_Wood", mu, mv, 0.42 - drop * 0.35, step, 0.07, 0.1, a2, pitch + rng.uniform(-4, 4), rng.uniform(-5, 5))
            prev = (pu, pv)
            for j in range(rng.randint(1, 3)):
                self.sit(rng.choice(TUFTS), pu + rng.uniform(-0.6, 0.6), pv + rng.uniform(-0.6, 0.6), rng.uniform(0, 360), rng.uniform(0.7, 1.2), embed=0.05)

    def hay(self, u, v, rng):
        for k in range(rng.randint(2, 5)):
            a = rng.uniform(0, math.tau)
            d = rng.uniform(0.2, 2.6)
            pu, pv = u + d * math.cos(a), v + d * math.sin(a)
            r = rng.uniform(1.0, 1.35)
            self.prim("FT_Block_Hay", pu, pv, r * 0.42, r, r, 1.0, rng.uniform(0, 180), 0.0, 90.0)

    def firewood(self, u, v, rng):
        ang = rng.uniform(0, 360)
        f = self.frame(u, v, ang)
        for row, (y0, count) in enumerate([(0, 6), (0.22, 5), (0.44, 3)]):
            for k in range(count):
                pu, pv = f(rng.uniform(-0.05, 0.05), (k - (count - 1) * 0.5) * 0.24)
                self.prim("FT_Block_BarkCyl", pu, pv, y0 + 0.14, 0.22, 0.22, 1.5, ang + 90 + rng.uniform(-3, 3), 90.0, 0.0)

    def old_camp(self, u, v, rng):
        for k in range(7):
            a = math.tau * k / 7
            self.sit(rng.choice(WALL_STONES), u + 0.9 * math.cos(a), v + 0.9 * math.sin(a), rng.uniform(0, 360), rng.uniform(0.26, 0.36), embed=0.3)
        self.prim("FT_Block_BarkCyl", u, v, 0.1, 0.18, 0.18, 1.0, rng.uniform(0, 180), 90.0, 0.0)
        self.prim("FT_Block_BarkCyl", u + 0.15, v, 0.15, 0.16, 0.16, 0.9, rng.uniform(0, 180), 90.0, 0.0)
        a = rng.uniform(0, math.tau)
        self.prim("FT_Block_Sack", u + 2.0 * math.cos(a), v + 2.0 * math.sin(a), 0.08, 1.9, 0.75, 0.12, rng.uniform(0, 180))
        self.prim("FT_Block_Wood", u + 2.2 * math.cos(a + 1.3), v + 2.2 * math.sin(a + 1.3), 0.3, 0.7, 0.7, 0.6, rng.uniform(0, 90))

    def signpost(self, u, v, rng, angle):
        tilt = rng.uniform(-5, 5)
        self.prim("FT_Block_Wood", u, v, 1.15, 0.18, 0.18, 2.7, angle, tilt, 0.0)
        f = self.frame(u, v, angle)
        pu, pv = f(0.45, 0.0)
        self.prim("FT_Block_Wood", pu, pv, 2.05, 1.2, 0.07, 0.28, angle + rng.uniform(-3, 3), rng.uniform(-3, 3))
        pu, pv = f(-0.35, 0.05)
        self.prim("FT_Block_Wood", pu, pv, 1.7, 0.95, 0.07, 0.24, angle + 180 + rng.uniform(-3, 3), rng.uniform(-3, 3))
        self.cairn(u + 0.35, v - 0.3, rng)

    def well_or_shrine(self, u, v, rng, angle):
        kind = rng.random()
        if kind < 0.5:   # 작은 돌 사당
            f = self.frame(u, v, angle)
            for ly in (-0.8, 0.8):
                pu, pv = f(0, ly)
                self.prim("FT_Block_Stone", pu, pv, 0.75, 0.55, 0.45, 1.7, angle + rng.uniform(-3, 3))
            pu, pv = f(0, 0)
            self.prim("FT_Block_Stone", pu, pv, 1.72, 0.7, 2.3, 0.25, angle)
            self.prim("FT_Block_Mushroom", pu, pv, 0.65, 0.3, 0.3, 0.4)
            for k in range(4):
                a = rng.uniform(0, math.tau)
                self.sit(rng.choice(FLOWERS), u + 1.4 * math.cos(a), v + 1.4 * math.sin(a), rng.uniform(0, 360), 1.2, embed=0.03)
        else:             # 돌 우물
            for k in range(9):
                a = math.tau * k / 9
                self.sit(rng.choice(WALL_STONES), u + 1.0 * math.cos(a), v + 1.0 * math.sin(a), rng.uniform(0, 360), (0.42, 0.32, 0.55), embed=0.2)
            self.prim("FT_Block_Wood", u, v, 1.5, 0.12, 2.3, 0.12, angle)
            for ly in (-1.1, 1.1):
                f = self.frame(u, v, angle)
                pu, pv = f(0, ly)
                self.prim("FT_Block_Wood", pu, pv, 1.0, 0.14, 0.14, 2.0, angle)

    def scarecrow(self, u, v, rng, angle):
        self.prim("FT_Block_Wood", u, v, 1.1, 0.12, 0.12, 2.4, angle, rng.uniform(-5, 5))
        self.prim("FT_Block_Wood", u, v, 1.65, 1.6, 0.1, 0.1, angle)
        self.prim("FT_Block_Sack", u, v, 1.95, 0.4, 0.4, 0.45, angle)
        self.prim("FT_Block_Sack", u, v, 1.2, 0.5, 0.3, 0.9, angle)

    def shards(self, u, v, rng):
        for k in range(rng.randint(3, 7)):
            a = rng.uniform(0, math.tau)
            d = rng.uniform(0, 1.6)
            s = rng.uniform(0.18, 0.55)
            self.prim("FT_Block_Shard", u + d * math.cos(a), v + d * math.sin(a), s * 0.55, s * 0.55, s * 0.55, s * 1.6, rng.uniform(0, 360), rng.uniform(-18, 18), rng.uniform(-18, 18))
        self.rock_cluster(u + 1.2, v - 0.8, rng)

    # ── 큰 콘셉트 — 다른 자리에 1 ~ 2 번 더 ──
    def small_beacon(self, u, v, rng):
        tops = []
        for k in range(10):
            a = math.tau * k / 10
            ft = rng.choice(WALL_STONES)
            s = rng.uniform(0.6, 0.85)
            pu, pv = u + 1.6 * math.cos(a), v + 1.6 * math.sin(a)
            self.sit(ft, pu, pv, rng.uniform(0, 360), s, embed=0.2)
            tops.append(self.ground(pu, pv) + 0.8 * 2.0 * self.bounds[ft][5] * s)
        for k in range(10):
            # 윗단 — 아래 두 돌 중 낮은 쪽 윗면에 얹는다(뜨지 않게)
            a = math.tau * (k + 0.5) / 10
            ft = rng.choice(WALL_STONES)
            s = rng.uniform(0.45, 0.65)
            _, _, oz, _, _, ez = self.bounds[ft]
            base = min(tops[k], tops[(k + 1) % 10]) - 0.14 * 2.0 * ez * s
            self.put(ft, u + 1.45 * math.cos(a), v + 1.45 * math.sin(a), base - s * (oz - ez), rng.uniform(0, 360), s)
        for k in range(5):
            a = math.tau * k / 5
            self.prim("FT_Block_BarkCyl", u + 0.55 * math.cos(a), v + 0.55 * math.sin(a), 1.2, 0.16, 0.16, 1.5, math.degrees(a), 24.0, 0.0)
        self.bump("Beacon")

    def ruin_house(self, u, v, rng, angle):
        f = self.frame(u, v, angle)
        w, d = rng.uniform(4.4, 6.4), rng.uniform(3.6, 4.8)
        for side, (a, b, c, e) in enumerate([(-w / 2, -d / 2, w / 2, -d / 2), (w / 2, -d / 2, w / 2, d / 2), (w / 2, d / 2, -w / 2, d / 2), (-w / 2, d / 2, -w / 2, -d / 2)]):
            n = int(math.hypot(c - a, e - b) / 0.7)
            for k in range(n + 1):
                t = k / max(n, 1)
                lx, ly = a + (c - a) * t, b + (e - b) * t
                pu, pv = f(lx, ly)
                courses = rng.randint(0, 4) if side != 0 else rng.randint(2, 6)
                z = 0.0
                for c1 in range(courses):
                    s = rng.uniform(0.55, 0.78)
                    ft = rng.choice(WALL_STONES)
                    _, _, oz, _, _, ez = self.bounds[ft]
                    sz = s * 0.72
                    h = 2.0 * ez * sz
                    self.put(ft, pu + rng.uniform(-0.05, 0.05), pv + rng.uniform(-0.05, 0.05), self.ground(pu, pv) + z * 100.0 - h * 0.15 - sz * (oz - ez), angle + side * 90 + rng.uniform(-10, 10), (s, s * 0.85, sz), rng.uniform(-4, 4), rng.uniform(-4, 4))
                    z += h / 100.0 * 0.8
        for k in range(14):
            pu, pv = f(rng.uniform(-w * 0.7, w * 0.7), rng.uniform(-d * 0.8, d * 0.8))
            self.sit(rng.choice(WALL_STONES), pu, pv, rng.uniform(0, 360), rng.uniform(0.35, 0.65), embed=0.26, pitch=rng.uniform(-20, 20), roll=rng.uniform(-20, 20))
        for k in range(10):
            a = rng.uniform(0, math.tau)
            self.sit(rng.choice(FERNS + TUFTS), u + rng.uniform(1.5, 4.0) * math.cos(a), v + rng.uniform(1.5, 4.0) * math.sin(a), rng.uniform(0, 360), rng.uniform(0.7, 1.4), embed=0.05)
        self.bump("Ruin")

    def small_stone_ring(self, u, v, rng):
        r = rng.uniform(4.5, 7.5)
        n = rng.randint(7, 11)
        for k in range(n):
            a = math.tau * k / n + rng.uniform(-0.12, 0.12)
            if rng.random() < 0.18:
                continue
            pu, pv = u + r * math.cos(a), v + r * math.sin(a)
            h = rng.uniform(1.0, 2.1)
            s = rng.uniform(0.42, 0.62)
            self.sit("FT_Dress_Wall4", pu, pv, rng.uniform(0, 360), (s * 0.7, s * 0.8, h), embed=0.14, pitch=rng.uniform(-7, 7), roll=rng.uniform(-7, 7))
        self.bump("StoneRing")

    def small_farm(self, u, v, rng, heading):
        f = self.frame(u, v, heading)
        for k in range(3):   # 밭 띠 셋
            pu, pv = f(0, (k - 1) * 4.2)
            self.prim("FT_Block_Hay", pu, pv, 0.1, 15.0, 3.2, 0.5, heading + 90.0, 90.0, 0.0)
        self.fence_piece(*f(-10, 0), rng, length=16)
        self.fence_piece(*f(10, 3), rng, length=14)
        self.hay(*f(-13, -5), rng)
        self.scarecrow(*f(2, 2), rng, heading)
        self.firewood(*f(9, -6), rng)
        self.bump("Farm")

    def fill(self):
        """사이 채우기 — 길가는 18 ~ 34 m 마다, 그 밖은 포아송으로 고른다. 종류는 땅 성격을 본다(물가 · 숲 그늘 · 비탈 · 들)"""
        rng = self.rng
        # 큰 콘셉트 다시 쓰기(자리는 평탄한 곳을 찾는다) — 맵을 돌며 「또 있네」가 아니라 「이 땅엔 사람이 살았구나」로 읽히게
        planned = [("farm", 1180, 562, 16, 62), ("beacon", 1298, 575, 8, 0), ("beacon", 1548, 412, 8, 0),
                   ("ruin", 1402, 438, 12, 25), ("ruin", 640, 626, 12, 140), ("ring", 1465, 468, 10, 0)]
        for kind, u, v, rad, angle in planned:
            spot = self.find_flat(u, v, 24, 20 if kind != "beacon" else 30, 3.0)
            if spot is None or not self.free(spot[0], spot[1], rad):
                unreal.log_warning(f"ld3: 큰 콘셉트 {kind} ({u}, {v}) 자리 없음")
                continue
            if kind == "farm":
                self.small_farm(spot[0], spot[1], rng, angle)
            elif kind == "beacon":
                self.small_beacon(spot[0], spot[1], rng)
            elif kind == "ruin":
                self.ruin_house(spot[0], spot[1], rng, angle)
            else:
                self.small_stone_ring(spot[0], spot[1], rng)
            self.claim(spot[0], spot[1], rad)
        made = {}
        def run(kind, u, v, angle):
            if kind == "cairn":
                self.cairn(u, v, rng)
            elif kind == "log":
                self.log_fallen(u, v, rng)
            elif kind == "stump":
                self.stump(u, v, rng)
            elif kind == "flowers":
                self.flower_patch(u, v, rng)
            elif kind == "rocks":
                self.rock_cluster(u, v, rng)
            elif kind == "fence":
                self.fence_piece(u, v, rng)
            elif kind == "hay":
                self.hay(u, v, rng)
            elif kind == "firewood":
                self.firewood(u, v, rng)
            elif kind == "camp":
                self.old_camp(u, v, rng)
            elif kind == "sign":
                self.signpost(u, v, rng, angle)
            elif kind == "well":
                self.well_or_shrine(u, v, rng, angle)
            elif kind == "scarecrow":
                self.scarecrow(u, v, rng, angle)
            elif kind == "shards":
                self.shards(u, v, rng)
            made[kind] = made.get(kind, 0) + 1
            self.bump(f"Fill.{kind}")
        # 길가: 사람 흔적이 많은 쪽으로 가중
        ROAD = ["cairn", "rocks", "flowers", "fence", "log", "stump", "sign", "hay", "firewood", "camp", "well", "flowers", "rocks", "scarecrow"]
        for line_name, line in self.d.paths.items():
            arc = rng.uniform(14, 28)
            while arc < line.length:
                u, v, (dx, dy) = line.at(arc)
                side = rng.choice((-1, 1))
                width = next(p["Width"] for p in self.raw["Paths"] if p["Name"] == line_name)
                off = width * 0.5 + rng.uniform(5.0, 12.0)
                pu, pv = u - dy * side * off, v + dx * side * off
                if self.c["in_sector"](pu, pv) and self.c["clear_of"](pu, pv, 1.5, True) and self.free(pu, pv, 4) and self.slope(pu, pv) < 24:
                    kind = rng.choice(ROAD)
                    self.claim(pu, pv, 4)
                    run(kind, pu, pv, math.degrees(math.atan2(dy, dx)))
                arc += rng.uniform(13, 26)
        # 그 밖 — 땅 성격에 맞춰(비탈 · 물가 · 숲 그늘 · 들판), 서로 26 m 이상
        tries = 0
        placed = 0
        while placed < 230 and tries < 9000:
            tries += 1
            u, v = rng.uniform(430, 1770), rng.uniform(130, 1080)
            if not self.c["in_sector"](u, v) or not self.c["clear_of"](u, v, 1.0, True) or not self.free(u, v, 11):
                continue
            s = self.slope(u, v)
            if s > 26:
                continue
            water = self.c["near_water"](u, v, 8.0)
            if water:
                choices = ["rocks", "log", "flowers", "stump"]
            elif s > 14:
                choices = ["rocks", "log", "rocks", "fence", "cairn", "shards"]
            else:
                choices = ["flowers", "rocks", "hay", "fence", "stump", "flowers", "log", "camp", "well", "shards", "scarecrow", "firewood"]
            kind = rng.choice(choices)
            self.claim(u, v, 11)
            run(kind, u, v, rng.uniform(0, 360))
            placed += 1
        return made

    def all(self, ripple_material, root):
        walls, wall_rows = ld4_walls.build_all(self)
        unreal.log(f"ld4: 돌담 {walls} 줄 — " + ", ".join(f"{w['name']} {w['length_m']} m" for w in wall_rows))
        creek = self.creek_dress()
        unreal.log(f"ld3: 개울 돌 · 자갈 · 식물 {creek}")
        for body in self._water_bodies():
            big = str(body.name) == "Creek_TowerBrook"
            x, y, z, pool = self.spring(body, big)
            self.spring_ripple(x, y, z, pool, ripple_material)
        self.carts()
        ridge = self.ridge_rocks()
        unreal.log(f"ld3: 등성이 바위 · 급사면 · 너덜 {ridge}")
        made = self.fill()
        unreal.log("ld3: 채우기 " + ", ".join(f"{k}={v}" for k, v in sorted(made.items())))
        info = self.write_wall_report(root, wall_rows)
        unreal.log("ld4: 돌담 " + json.dumps(info, ensure_ascii=False))
        unreal.log("ld3: 개수 " + ", ".join(f"{k}={v}" for k, v in sorted(self.counts.items())))
