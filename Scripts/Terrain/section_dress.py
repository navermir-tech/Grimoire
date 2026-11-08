# 레벨 디자인 2차 — 구간마다 콘셉트 아트(docs/art/region1-concepts.md)대로 구도를 잡는 소품 · 블록아웃 + 하늘의 구역 이름.
# dress_openbeta.py 가 부른다(태그 LevelDress — 다시 돌리면 걷고 다시 놓는다). 무엇을 왜: docs/dev/level-design.md 「9」.
#
#   큰길 돌담 · 돌 표지  r1-key-approach — 길가 낮은 마른 돌담(끊기고 무너진 곳), 놋쇠 고리를 얹은 돌 표지 기둥, 앞마당 돌 등불 줄
#   능선 바위 · 꼭대기 나무  BotW 삼각형 규칙 — 등성이 꼭대기에 바위 무더기 · 외딴 큰 나무를 얹어 실루엣이 하늘에 걸리게
#   풍차 마당          lm-windmill · poi-s7-mill-yard — 밀밭 띠 · 울타리 · 건초 더미 · 곳간
#   봉화 언덕          poi-s5-beacon — 꼭대기 돌 봉화대
#   서쪽 연못          poi-s3-pond — 물속 징검돌 줄 → 가운데 작은 섬 · 상자
#   부유석 언덕        lm-floating-rock — 섬으로 오를 디딤돌(지금은 땅에 내려앉은 돌들)이 언덕을 감아 오른다
#   길가 초원          poi-s6-meadow — 속 빈 고목 둥치 · 들벌 둥지
#   하늘 구역 이름      사용자 요청 — 구역 위 하늘에 큰 한글 이름(AGrimDevLabel, bSectionLabel). 레이아웃 Sections
#
# 기본 도형은 「아직 에셋이 없는 자리」 블록아웃이다(재질 = MI_Terrain_* 단색). 돌담 · 바위는 StyleHex 돌 메시(폴리지 인스턴스).
import math
import random

import unreal

import landmark_greybox

TAG = "LevelDress"
# 구간 블록아웃 단색(terrain_assets.make_color_instance — 없으면 만든다)
SECTION_COLORS = {
    "Wheat": ("MI_Terrain_Wheat", (0.42, 0.30, 0.07)),
    "Hay": ("MI_Terrain_Hay", (0.38, 0.30, 0.13)),
}
MATERIAL_DIR = "/Game/Grimoire/Terrain"


def catmull_rom(points, subdiv=8):
    """생성기(FPolyline::Smooth)와 같은 곡선 — 길 가운데선을 레이아웃 꺾은점이 아니라 실제 깎인 곡선으로"""
    pts = [(p["X"], p["Y"]) for p in points]
    if len(pts) < 3:
        return pts
    out = []
    for i in range(len(pts) - 1):
        p0, p1, p2, p3 = pts[max(i - 1, 0)], pts[i], pts[i + 1], pts[min(i + 2, len(pts) - 1)]
        for s in range(subdiv):
            t = s / subdiv
            t2, t3 = t * t, t * t * t
            out.append(tuple(0.5 * (2 * p1[k] + (p2[k] - p0[k]) * t + (2 * p0[k] - 5 * p1[k] + 4 * p2[k] - p3[k]) * t2
                                    + (3 * p1[k] - p0[k] - 3 * p2[k] + p3[k]) * t3) for k in range(2)))
    out.append(pts[-1])
    return out


class Polyline:
    def __init__(self, pts):
        self.pts = pts
        self.cum = [0.0]
        for i in range(1, len(pts)):
            self.cum.append(self.cum[-1] + math.dist(pts[i - 1], pts[i]))

    @property
    def length(self):
        return self.cum[-1]

    def at(self, arc):
        """(u, v, 진행 방향 단위 벡터)"""
        arc = max(0.0, min(arc, self.length))
        for i in range(len(self.pts) - 1):
            if arc <= self.cum[i + 1] or i == len(self.pts) - 2:
                seg = self.cum[i + 1] - self.cum[i]
                t = (arc - self.cum[i]) / seg if seg > 0 else 0.0
                a, b = self.pts[i], self.pts[i + 1]
                d = (b[0] - a[0], b[1] - a[1])
                n = math.hypot(*d) or 1.0
                return a[0] + d[0] * t, a[1] + d[1] * t, (d[0] / n, d[1] / n)
        return self.pts[-1][0], self.pts[-1][1], (1.0, 0.0)


class SectionDresser:
    def __init__(self, ctx):
        """ctx: dict — raw(레이아웃 JSON), lib, actors(EditorActorSubsystem), world, foliage(이름 → 폴리지 타입), map_to_world,
        clear_of(u, v, margin, allow_paths=False), in_sector(u, v), water_level(이름 → m), count(종류, n)"""
        self.c = ctx
        self.raw = ctx["raw"]
        self.lib = ctx["lib"]
        self.rng = random.Random(20261108)
        mats = {}
        import terrain_assets
        for key, (asset_name, _rgb) in terrain_assets.LANDMARK_COLORS.items():
            m = unreal.load_asset(f"{MATERIAL_DIR}/{asset_name}")
            if m is not None:
                mats[key] = m
        # 구간 블록아웃 전용 단색(밀밭 · 건초) — 그레이박스 단색 부모(M_Terrain_Color)의 인스턴스
        parent = unreal.load_asset(f"{MATERIAL_DIR}/M_Terrain_Color")
        for key, (asset_name, rgb) in SECTION_COLORS.items():
            mats[key] = terrain_assets.make_color_instance(asset_name, parent, rgb, False)
        self.placer = landmark_greybox.Placer(ctx["actors"], self.ground_cm, ctx["map_to_world"], mats, self._on_spawn)
        self.instances = {}   # 폴리지 타입 이름 → [Transform]
        self.paths = {p["Name"]: Polyline(catmull_rom(p["Points"])) for p in self.raw["Paths"]}

    # ── 공통 ──
    def ground_cm(self, x, y):
        return self.lib.get_ground_height(x, y)

    def _on_spawn(self, actor):
        actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(f"{TAG}.Blockout")])
        actor.set_editor_property("is_spatially_loaded", True)
        folder = actor.get_folder_path()
        if not str(folder).startswith("LevelDress"):
            actor.set_folder_path(f"LevelDress/Sections/{folder}")
        self.c["count"]("Blockout")

    def add_instance(self, ft_name, u, v, yaw, scale, z_off_cm=0.0, pitch=0.0, roll=0.0):
        x, y = self.c["map_to_world"](u, v)
        z = self.ground_cm(x, y) + z_off_cm
        s = scale if isinstance(scale, tuple) else (scale, scale, scale)
        self.instances.setdefault(ft_name, []).append(
            unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll, pitch, yaw), unreal.Vector(*s)))

    def flush_instances(self):
        world = self.c["world"]
        for name, transforms in self.instances.items():
            unreal.InstancedFoliageActor.add_instances(world, self.c["foliage"][name], transforms)
            self.c["count"](f"Section.{name}", len(transforms))
        total = sum(len(v) for v in self.instances.values())
        self.instances = {}
        return total

    def near_path(self, u, v, margin):
        return landmark_greybox.distance_to_paths(u, v, self.raw) < margin

    # ── 큰길: 마른 돌담 · 돌 표지 기둥 · 앞마당 등불 (r1-key-approach) ──
    def approach_road(self):
        main = self.paths["Path_Main"]
        sector = self.c["in_sector"]
        stones = ["FT_Dress_RockSmall1", "FT_Dress_RockSmall3", "FT_Dress_RockSmall5", "FT_Dress_RockSmall2", "FT_Dress_RockSmall4"]
        # 담 구간(호 길이 m, 쪽 +1 = 진행 방향 왼쪽) — 탑 앞 구간은 비우고, 굽이 바깥쪽에 담을 둔다
        walls = []   # 레벨 디자인 3차: 돌담은 ld3_dress.Level3.dry_stone_walls(돌마다 땅에 앉히고 쌓는다)
        placed = 0
        for a0, a1, side in walls:
            arc = a0
            while arc < a1:
                if self.rng.random() < 0.08:          # 끊긴 곳(2~5 m)
                    arc += self.rng.uniform(2.0, 5.0)
                    continue
                u, v, (dx, dy) = main.at(arc)
                nx, ny = -dy * side, dx * side
                off = 5.2 + 0.25 * math.sin(arc * 0.35)
                wu, wv = u + nx * off, v + ny * off
                if not sector(wu, wv) or self.c["near_water"](wu, wv, 2.0) or landmark_greybox.distance_to_paths(wu, wv, self.raw) < 3.6:
                    arc += 1.0
                    continue
                yaw_world = 90.0 - math.degrees(math.atan2(dy, dx))
                collapsed = (math.sin(arc * 0.07 + a0) > 0.75)   # 무너진 곳: 아랫단만, 돌이 흩어짐
                # 아랫단(큰 돌) · 윗단(작은 돌, 반 칸 어긋남)
                self.add_instance(self.rng.choice(stones), wu, wv, yaw_world + self.rng.uniform(-25, 25),
                                  (self.rng.uniform(0.55, 0.7), self.rng.uniform(0.45, 0.55), self.rng.uniform(0.5, 0.65)), z_off_cm=-8)
                placed += 1
                if not collapsed:
                    u2, v2, _ = main.at(arc + 0.45)
                    self.add_instance(self.rng.choice(stones), u2 + nx * off, v2 + ny * off, yaw_world + self.rng.uniform(-25, 25),
                                      (self.rng.uniform(0.42, 0.55), self.rng.uniform(0.38, 0.48), self.rng.uniform(0.4, 0.5)), z_off_cm=38)
                    placed += 1
                elif self.rng.random() < 0.5:
                    self.add_instance(self.rng.choice(stones), wu + nx * self.rng.uniform(0.8, 1.8), wv + ny * self.rng.uniform(0.8, 1.8),
                                      self.rng.uniform(0, 360), self.rng.uniform(0.35, 0.5), z_off_cm=-5)
                arc += 0.85
        # 돌 표지 기둥(놋쇠 고리) — 70 m 마다 번갈아
        posts = 0
        arc = 60.0
        side = 1
        while arc < main.length and arc < 1000:
            u, v, (dx, dy) = main.at(arc)
            nx, ny = -dy * side, dx * side
            pu, pv = u + nx * 3.6, v + ny * 3.6
            near_story = any(p.get("bNoGreybox") and math.hypot(p["Center"]["X"] - pu, p["Center"]["Y"] - pv) < 16 for p in self.raw["Pois"])
            if sector(pu, pv) and not self.c["near_water"](pu, pv, 3.0) and not near_story:
                heading = math.degrees(math.atan2(dy, dx))
                p = self.placer.at(pu, pv, heading, "Waystones", f"Waystone_{posts:02d}")
                p.shape(landmark_greybox.CUBE, "Stone", "Post", 0, 0, -0.4, 0.75, 0.6, 2.6, roll=self.rng.uniform(-3, 3))
                p.shape(landmark_greybox.CUBE, "Stone", "Cap", 0, 0, 2.15, 0.95, 0.8, 0.25)
                p.shape(landmark_greybox.CYLINDER, "KindViewpoint", "BrassRing", 0, 0, 0, 0.7, 0.12, 0.7, pitch=90, block=False, z_center=2.75)
                p.shape(landmark_greybox.SPHERE, "KindViewpoint", "BrassOrb", 0, 0, 0, 0.28, 0.28, 0.28, block=False, z_center=2.75)
                posts += 1
            arc += 70.0
            side = -side
        # 앞마당에서 북쪽으로 이어지는 꺼진 돌 등불 줄(tower-yard)
        lamps = 0
        for k, arc in enumerate([52, 70, 88]):
            u, v, (dx, dy) = main.at(arc)
            for side in (1, -1):
                pu, pv = u - dy * side * 3.8, v + dx * side * 3.8
                p = self.placer.at(pu, pv, math.degrees(math.atan2(dy, dx)), "YardLanterns", f"Lantern_{k}_{'L' if side > 0 else 'R'}")
                p.shape(landmark_greybox.CUBE, "Stone", "Post", 0, 0, -0.3, 0.32, 0.32, 1.25)
                p.shape(landmark_greybox.CUBE, "Dark", "Head", 0, 0, 0.95, 0.42, 0.42, 0.38)
                p.shape(landmark_greybox.CUBE, "Stone", "Roof", 0, 0, 1.33, 0.56, 0.56, 0.12)
                lamps += 1
        return placed, posts, lamps

    # ── 능선 꼭대기: 바위 무더기 · 외딴 큰 나무(삼각형 꼭대기에 시선이 걸리게) ──
    def ridge_crests(self):
        """레벨 디자인 2차 — 3차부터는 ld3_dress.Level3.ridge_rocks 가 대신한다(봉우리 표식만 남긴다)"""
        return 0, 0

    def _ridge_crests_v2(self):
        rocks = ["FT_Dress_RockLarge", "FT_Dress_RockCluster1", "FT_Dress_RockCluster2", "FT_Dress_Boulder1", "FT_Dress_Boulder2"]
        trees = ["FT_Dress_Broadleaf1", "FT_Dress_Broadleaf2"]
        n_rocks = n_trees = 0
        for ridge in self.raw.get("Ridgelines", []):
            line = Polyline(catmull_rom(ridge["Points"]))
            heights = ridge.get("Heights") or [ridge.get("Height", 20)] * len(ridge["Points"])
            # 높은 꺾은점(이웃보다 높거나 끝점이 아닌 봉우리) 둘레에 무더기
            for i, h in enumerate(heights):
                if h < 14:
                    continue
                arc = line.cum[min(i * 8, len(line.cum) - 1)] if len(ridge["Points"]) >= 3 else line.cum[min(i, len(line.cum) - 1)]
                u, v, (dx, dy) = line.at(arc + self.rng.uniform(-15, 15))
                if not self.c["clear_of"](u, v, 2.0):
                    continue
                for j in range(self.rng.randint(3, 6)):
                    ru = u + self.rng.uniform(-9, 9) - dy * self.rng.uniform(-5, 5)
                    rv = v + self.rng.uniform(-9, 9) + dx * self.rng.uniform(-5, 5)
                    if not self.c["clear_of"](ru, rv, 0.0):
                        continue
                    big = j == 0
                    self.add_instance(self.rng.choice(rocks[:3] if big else rocks), ru, rv, self.rng.uniform(0, 360),
                                      self.rng.uniform(1.3, 2.0) if big else self.rng.uniform(0.7, 1.3), z_off_cm=-60 if big else -35,
                                      pitch=self.rng.uniform(-12, 12), roll=self.rng.uniform(-12, 12))
                    n_rocks += 1
                # 가장 높은 봉우리 몇에만 외딴 큰 나무(바위에서 조금 비켜)
                if h >= 22 and self.rng.random() < 0.8:
                    tu, tv = u + dy * 12, v - dx * 12
                    if self.c["clear_of"](tu, tv, 3.0):
                        self.add_instance(self.rng.choice(trees), tu, tv, self.rng.uniform(0, 360), self.rng.uniform(0.95, 1.15), z_off_cm=-15)
                        n_trees += 1
        return n_rocks, n_trees

    # ── 풍차 마당(poi-s7-mill-yard · lm-windmill): 밀밭 띠 · 울타리 · 건초 더미 · 곳간 ──
    def windmill_farm(self):
        field = next(p for p in self.raw["Pois"] if p["Name"] == "Poi_Windmill_Field")
        fu, fv = field["Center"]["X"], field["Center"]["Y"]
        heading = 25.0
        p = self.placer.at(fu, fv, heading, "WindmillFarm", "Wheat")
        n = 0
        for k in range(5):   # 밀밭 띠 5(사이 고랑)
            ly = (k - 2) * 4.6
            p.shape(landmark_greybox.CUBE, "Wheat", f"Strip_{k}", self.rng.uniform(-1, 1), ly, -0.4, self.rng.uniform(22, 30), 3.6, 1.3,
                    roll=self.rng.uniform(-2, 2), block=False, shadow=False)
            n += 1
        # 울타리(나무 말뚝 + 가로대) — 밭 둘레 세 변(길 쪽은 연다)
        for side, (x0, y0, x1, y1) in enumerate([(-17, -13, 17, -13), (17, -13, 17, 13), (-17, 13, 17, 13)]):
            length = math.hypot(x1 - x0, y1 - y0)
            steps = int(length / 3.0)
            for s in range(steps + 1):
                t = s / max(steps, 1)
                lx, ly = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
                p.shape(landmark_greybox.CUBE, "Wood", f"Post_{side}_{s}", lx, ly, -0.3, 0.2, 0.2, 1.5, roll=self.rng.uniform(-6, 6))
            ang = math.degrees(math.atan2(y1 - y0, x1 - x0))
            p.shape(landmark_greybox.CUBE, "Wood", f"Rail_{side}", (x0 + x1) / 2, (y0 + y1) / 2, 0.85, length, 0.12, 0.14, yaw=ang)
            n += steps + 2
        # 건초 더미(누운 원통) · 묶음
        for k in range(7):
            lx, ly = self.rng.uniform(-28, -19), self.rng.uniform(-14, 14)
            p.shape(landmark_greybox.CYLINDER, "Hay", f"Hay_{k}", lx, ly, 0, 1.3, 1.3, 1.1, pitch=90, yaw=self.rng.uniform(0, 180), z_center=0.6)
            n += 1
        # 곳간(닫힌 문) — 밭 북동쪽, 길에서 비켜
        bu, bv = fu + 28, fv + 22
        if self.c["clear_of"](bu, bv, 4.0, True):
            b = self.placer.at(bu, bv, heading + 90, "WindmillFarm", "Barn", level=True)
            b.shape(landmark_greybox.CUBE, "Wood", "Body", 0, 0, -0.5, 9, 7, 5)
            b.shape(landmark_greybox.CUBE, "Roof", "RoofL", 0, -1.8, 0, 9.6, 4.6, 0.4, roll=38, z_center=5.6)
            b.shape(landmark_greybox.CUBE, "Roof", "RoofR", 0, 1.8, 0, 9.6, 4.6, 0.4, roll=-38, z_center=5.6)
            b.shape(landmark_greybox.CUBE, "Dark", "Door", 4.55, 0, 0, 0.2, 2.6, 3.0)
            n += 4
        return n

    # ── 봉화 언덕 꼭대기 돌 봉화대(poi-s5-beacon) ──
    def beacon(self):
        top = next(p for p in self.raw["Pois"] if p["Name"] == "Poi_Beacon_Top")
        u, v = top["Center"]["X"] - 7, top["Center"]["Y"] + 4
        p = self.placer.at(u, v, 0, "Beacon", "Beacon", level=True)
        p.shape(landmark_greybox.CYLINDER, "Stone", "Base", 0, 0, -0.5, 4.2, 4.2, 3.2)
        p.shape(landmark_greybox.CYLINDER, "Stone", "Neck", 0, 0, 2.6, 2.6, 2.6, 1.6)
        p.shape(landmark_greybox.CYLINDER, "Dark", "Bowl", 0, 0, 4.1, 3.4, 3.4, 0.9)
        for i in range(4):
            a = i * 90 + 20
            p.shape(landmark_greybox.CUBE, "Wood", f"Log_{i}", 1.0 * math.cos(math.radians(a)), 1.0 * math.sin(math.radians(a)), 0, 2.4, 0.35, 0.35,
                    yaw=a, pitch=25, block=False, z_center=5.2)
        for i in range(5):   # 둘레 무너진 돌
            a = self.rng.uniform(0, 360)
            r = self.rng.uniform(3.2, 5.5)
            p.shape(landmark_greybox.CUBE, "Stone", f"Rubble_{i}", r * math.cos(math.radians(a)), r * math.sin(math.radians(a)), -0.2,
                    self.rng.uniform(0.6, 1.2), self.rng.uniform(0.5, 0.9), self.rng.uniform(0.4, 0.7), yaw=self.rng.uniform(0, 90))
        return 1

    # ── 서쪽 연못(poi-s3-pond): 물속 징검돌 줄 → 가운데 섬 · 상자 ──
    def pond_stones(self):
        pond = next(p for p in self.raw["Ponds"] if p["Name"] == "Pond_TowerWest")
        level = self.c["water_level"]("Pond_TowerWest")
        if level is None:
            return 0
        cu, cv = pond["Center"]["X"], pond["Center"]["Y"]
        stones = next(p for p in self.raw["Pois"] if p["Name"] == "Poi_Pond_IceStones")
        su, sv = stones["Center"]["X"], stones["Center"]["Y"]
        iu, iv = cu - 8, cv + 4   # 섬
        n = 0
        steps = 8
        for k in range(steps):
            t = (k + 0.6) / (steps + 0.4)
            u = su + (iu - su) * t + math.sin(t * 5.0) * 2.0
            v = sv + (iv - sv) * t + math.cos(t * 4.0) * 1.5
            x, y = self.c["map_to_world"](u, v)
            a = self.c["actors"].spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, level * 100.0 - 12.0), unreal.Rotator(0, 0, self.rng.uniform(0, 90)))
            a.static_mesh_component.set_static_mesh(self.placer.meshes[landmark_greybox.CYLINDER])
            a.static_mesh_component.set_material(0, self.placer.materials["Stone"])
            a.set_actor_scale3d(unreal.Vector(1.4, 1.2, 0.4))
            a.set_actor_label(f"PondStone_{k:02d}")
            a.set_folder_path("LevelDress/Sections/PondStones")
            self._on_spawn(a)
            n += 1
        x, y = self.c["map_to_world"](iu, iv)
        isl = self.c["actors"].spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, level * 100.0 - 60.0), unreal.Rotator(0, 0, 20))
        isl.static_mesh_component.set_static_mesh(self.placer.meshes[landmark_greybox.SPHERE])
        isl.static_mesh_component.set_material(0, self.placer.materials["Dirt"])
        isl.set_actor_scale3d(unreal.Vector(9.0, 7.5, 2.2))
        isl.set_actor_label("PondIsland")
        isl.set_folder_path("LevelDress/Sections/PondStones")
        self._on_spawn(isl)
        chest = self.c["actors"].spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, level * 100.0 + 90.0), unreal.Rotator(0, 0, 35))
        chest.static_mesh_component.set_static_mesh(self.placer.meshes[landmark_greybox.CUBE])
        chest.static_mesh_component.set_material(0, self.placer.materials["Wood"])
        chest.set_actor_scale3d(unreal.Vector(1.2, 0.8, 0.8))
        chest.set_actor_label("PondIsland_Chest")
        chest.set_folder_path("LevelDress/Sections/PondStones")
        self._on_spawn(chest)
        return n + 2

    # ── 부유석 언덕: 섬으로 오를 디딤돌(지금은 땅에 내려앉은 돌들)이 언덕을 감아 오른다(lm-floating-rock) ──
    def floating_steps(self):
        lm = next(p for p in self.raw["Landmarks"] if p["Kind"] == "FloatingRock")
        cu, cv = lm["Center"]["X"], lm["Center"]["Y"]
        n = 0
        for k in range(9):
            a = 200 + k * 22
            r = 34 - k * 2.2
            u, v = cu + r * math.cos(math.radians(a)), cv + r * math.sin(math.radians(a))
            p = self.placer.at(u, v, a + 90, "FloatingSteps", f"Step_{k}")
            p.shape(landmark_greybox.CUBE, "FloatRock", "Stone", 0, 0, -0.3, 2.8, 2.0, 0.7, pitch=self.rng.uniform(-6, 6), roll=self.rng.uniform(-8, 8))
            n += 1
        return n

    # ── 길가 초원(poi-s6-meadow): 속 빈 고목 둥치 + 들벌 둥지 ──
    def meadow_stump(self):
        bees = next(p for p in self.raw["Pois"] if p["Name"] == "Poi_Meadow_Bees")
        u, v = bees["Center"]["X"] + 4, bees["Center"]["Y"] - 3
        p = self.placer.at(u, v, 30, "MeadowStump", "HollowStump", level=True)
        for i in range(6):   # 둘레 판 6 장으로 속이 빈 둥치
            a = i * 60
            p.shape(landmark_greybox.CUBE, "Bark", f"Wall_{i}", 1.6 * math.cos(math.radians(a)), 1.6 * math.sin(math.radians(a)), -0.3,
                    0.7, 2.0, self.rng.uniform(3.2, 5.2), yaw=a, roll=self.rng.uniform(-5, 5))
        for i in range(4):   # 드러난 뿌리
            a = i * 90 + 30
            _beam(p, "Bark", f"Root_{i}", (1.8 * math.cos(math.radians(a)), 1.8 * math.sin(math.radians(a)), 1.0),
                  (4.2 * math.cos(math.radians(a)), 4.2 * math.sin(math.radians(a)), -0.2), 0.9, 0.8)
        p.shape(landmark_greybox.SPHERE, "Bone", "Hive", 1.2, 0.4, 0, 1.1, 1.1, 1.5, block=False, z_center=3.9)
        return 1

    # ── 하늘 구역 이름(레이아웃 Sections) ──
    def section_labels(self):
        cls = unreal.load_class(None, landmark_greybox.DEV_LABEL_CLASS)
        n = 0
        for spec in self.raw.get("Sections", []):
            u, v = spec["Center"]["X"], spec["Center"]["Y"]
            x, y = self.c["map_to_world"](u, v)
            region = spec.get("Tier") == "Region"
            # 구역 이름은 소구역 이름보다 40 m 위 — 같은 쪽 하늘에서 겹치지 않게
            z = self.ground_cm(x, y) + (spec.get("Height", 140 if region else 55) + (40 if region else 0)) * 100.0
            if "AbsHeight" in spec:
                z = spec["AbsHeight"] * 100.0
            actor = self.c["actors"].spawn_actor_from_class(cls, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 0))
            actor.set_actor_label(f"SectionLabel_{spec['Name']}")
            actor.set_folder_path("LevelDress/SectionLabels")
            actor.set_editor_property("title", unreal.Text(spec["Label"]))
            actor.set_editor_property("body", unreal.Text("" if region else spec.get("Note", "")))
            color = (1.0, 0.86, 0.45) if region else (0.62, 0.95, 1.0)
            actor.set_editor_property("title_color", unreal.LinearColor(color[0], color[1], color[2], 1.0))
            actor.set_editor_property("section_label", True)
            actor.set_editor_property("title_font_size", 120 if region else 96)
            actor.set_editor_property("body_font_size", 54 if region else 44)
            actor.set_editor_property("world_scale", 14.0 if region else 6.0)
            actor.set_editor_property("wrap_width", 2600.0)
            actor.set_editor_property("max_visible_distance", 450000.0 if region else 60000.0)
            actor.set_editor_property("tags", [unreal.Name(TAG), unreal.Name(f"{TAG}.SectionLabel"), unreal.Name("DevLabel")])
            actor.set_editor_property("is_spatially_loaded", False)   # 멀리서도 보여야 한다(늘 로드, 거리로 숨김)
            self.c["count"]("SectionLabel")
            n += 1
        return n


def _beam(p, material, name, a, b, width, depth):
    return landmark_greybox._beam(p, material, name, a, b, width, depth)
