# 레벨 디자인 4차(docs/dev/level-design.md 「11」) — 이유가 있는 자리에만 쌓는 마른 돌담. ld3_dress.Level3.all 이 부른다.
#
# 담의 자리 · 이유 · 높이는 레이아웃 "StoneWalls" 에 있다(코드에 자리를 적지 않는다). 한 줄 = 이름 · 이유(Why) · 종류(Kind)
#   Road   길가: Path · ArcStart · ArcEnd · Side · Offset   — 마을(탑 앞 · 거점) 가까운 길가만
#   Field  밭 경계: Points(꺾은점)                         — 풍차 밭처럼 지키는 땅이 있을 때만
#   Fold   양 우리: Center · Radius · OpenAt(입구 각도)    — 둥근 낮은 담, 입구 하나
#
# 생김새 — 실제 마른 돌담(Peak District 안내서 · Low Impact): 납작한 돌이 가로로 이어진 단(course), 단마다 이음을 어긋나게, 위로 갈수록 좁게(배터), 맨 위는 전 폭을 덮는 갓돌.
#   돌 메시는 StyleHex 둥근 돌(SM_Stone_Small_*)을 납작하게 눌러 쓴다(높이 11 ~ 17 cm, 길이 45 ~ 95 cm). 이웃 돌과 살짝 겹치게 놓아 틈이 보이지 않고, 기울기 · 흔들림은 ±2° 안팎.
#   단의 높이는 땅을 그대로 따르지 않고 가로로 곧게 이어진다 — 땅을 고른 선(3 m 창)을 따르고, 비탈이 가파르면 단이 계단처럼 끊겨 내려간다(실제 담의 stepped course).
import math
import random

import section_dress

MIN_ARC = 0.05


class WallBuilder:
    def __init__(self, level3):
        self.l = level3
        self.n = __import__("ld3_dress")   # ld3_dress 가 이 파일을 부른다 — 불러온 뒤 쓴다
        self.report = []       # 담마다 [이름, 이유, 길이, 돌 수]

    # ── 자리 → 꺾은선 ──
    def road_line(self, spec):
        line = self.l.d.paths[spec["Path"]]
        side, off = spec["Side"], spec["Offset"]
        seed = sum(map(ord, spec["Name"])) % 97
        pts = []
        arc = spec["ArcStart"]
        while arc <= spec["ArcEnd"]:
            u, v, (dx, dy) = line.at(arc)
            o = off + 0.45 * self.n.noise1(arc * 0.05, seed)
            pts.append((u - dy * side * o, v + dx * side * o))
            arc += 1.0
        return pts

    def fold_line(self, spec):
        cu, cv, r = spec["Center"]["X"], spec["Center"]["Y"], spec["Radius"]
        seed = sum(map(ord, spec["Name"])) % 89
        pts = []
        opening = spec.get("OpenAt", 0.0)
        gate = spec.get("GateDeg", 34.0)
        d = gate
        while d <= 360.0 - gate:
            a = math.radians(opening + d)
            wob = r * (1.0 + 0.06 * self.n.noise1(d * 0.03, seed))
            pts.append((cu + wob * math.cos(a), cv + wob * math.sin(a)))
            d += 6.0
        return pts

    def field_line(self, spec):
        return [(p["X"], p["Y"]) for p in spec["Points"]]

    # ── 쌓기 ──
    def wall_ok(self, u, v):
        return self.l._wall_ok(u, v)

    def build(self, spec):
        kind = spec["Kind"]
        pts = self.road_line(spec) if kind == "Road" else self.fold_line(spec) if kind == "Fold" else self.field_line(spec)
        return self.lay(pts, spec)

    def lay(self, pts, spec):
        lv = self.l
        rng = random.Random(sum(map(ord, spec["Name"])) * 131)
        poly = section_dress.Polyline(pts)
        L = poly.length
        if L < 3:
            return 0
        height = spec.get("Height", 1.0)
        width0 = spec.get("Width", 0.62)
        n = int(L / MIN_ARC)
        # 고른 밑선 — 땅을 3 m 창으로 고른 높이(cm). 단은 이 선을 따라 곧게 이어진다
        ground = []
        for i in range(n):
            u, v, _ = poly.at(i * MIN_ARC)
            ground.append(lv.ground(u, v))
        # 밑선: 그 자리 ±0.6 m 의 가장 낮은 땅을 3 m 창으로 고른 것 — 맨 아랫단이 땅 위로 뜨지 않는다(살짝 묻힌다)
        near = int(0.6 / MIN_ARC)
        floor_line = [min(ground[max(0, i - near):min(n, i + near + 1)]) for i in range(0, n)]
        window = int(1.5 / MIN_ARC)
        prefix = [0.0]
        for g in floor_line:
            prefix.append(prefix[-1] + g)

        def smooth(i):
            i = max(0, min(i, n - 1))
            a, b = max(0, i - window), min(n, i + window)
            return (prefix[b] - prefix[a]) / max(b - a, 1)

        # 무너진 자리 — 담 하나에 많아야 하나, 길이 40 m 넘는 담만. 윗단이 낮아질 뿐 끊기지 않는다
        slump = None
        if L > 40.0 and rng.random() < 0.6:
            at = rng.uniform(L * 0.25, L * 0.75)
            slump = (at, rng.uniform(2.2, 3.6))

        def level_cap(arc):
            if slump is None or abs(arc - slump[0]) > slump[1]:
                return 1.0
            t = abs(arc - slump[0]) / slump[1]
            return 0.55 + 0.45 * t * t

        stones = 0
        course_h = spec.get("CourseHeight", 0.145)
        courses = max(3, int(round(height / course_h)))
        base_h = 0.24      # 맨 아랫단은 조금 큰 돌을 묻는다
        z_low = 0.0
        for course in range(courses):
            tall = base_h if course == 0 else course_h * rng.uniform(0.92, 1.1)
            frac = course / max(courses - 1, 1)
            thick = width0 * (1.0 - 0.32 * frac)
            arc = rng.uniform(0.0, 0.35) if course % 2 == 0 else rng.uniform(0.25, 0.6)   # 단마다 이음이 어긋나게
            while arc < L - 0.15:
                length = rng.uniform(0.5, 0.95) if course else rng.uniform(0.7, 1.2)
                if arc + length > L:
                    length = L - arc
                    if length < 0.3:
                        break
                mid = arc + length * 0.5
                cap = level_cap(mid)
                if z_low / height >= cap:      # 무너진 자리 — 이 단은 비운다
                    arc += length * 0.92
                    continue
                u, v, (dx, dy) = poly.at(mid)
                if not self.wall_ok(u, v):
                    arc += length
                    continue
                self.stone(rng, u, v, dx, dy, smooth(int(mid / MIN_ARC)) + z_low * 100.0, length, thick, tall, course, poly, arc, spec)
                stones += 1
                arc += length * rng.uniform(0.86, 0.94)   # 이웃 돌과 살짝 겹친다 — 틈이 보이지 않게
            z_low += tall * 0.86
        # 갓돌 — 담 맨 위를 전 폭으로 덮는다. 세운 납작돌을 촘촘히 이어 위가 위태롭지 않게
        if spec.get("Cope", True):
            arc = rng.uniform(0.05, 0.25)
            while arc < L - 0.12:
                w = rng.uniform(0.2, 0.34)
                mid = arc + w * 0.5
                u, v, (dx, dy) = poly.at(mid)
                if self.wall_ok(u, v) and level_cap(mid) > 0.99:
                    i = int(mid / MIN_ARC)
                    self.stone(rng, u, v, dx, dy, smooth(i) + z_low * 100.0, w, width0 * 0.78, rng.uniform(0.17, 0.24), 99, poly, arc, spec)
                    stones += 1
                arc += w * rng.uniform(0.9, 1.0)
        # 밑동 — 담을 땅에 앉히는 풀포기
        arc = rng.uniform(0, 1)
        while arc < L:
            u, v, (dx, dy) = poly.at(arc)
            for sgn in (-1.0, 1.0):
                if rng.random() < 0.5:
                    off = width0 * 0.5 + rng.uniform(0.06, 0.45)
                    pu, pv = u - dy * sgn * off, v + dx * sgn * off
                    if self.wall_ok(pu, pv):
                        r = rng.random()
                        ft = rng.choice(self.n.TUFTS) if r < 0.7 else rng.choice(self.n.FERNS) if r < 0.9 else rng.choice(self.n.FLOWERS)
                        lv.sit(ft, pu, pv, rng.uniform(0, 360), rng.uniform(0.55, 1.0), embed=0.05)
                        lv.bump("WallBase")
            arc += rng.uniform(0.9, 1.8)
        self.report.append({"name": spec["Name"], "kind": spec["Kind"], "why": spec["Why"], "length_m": round(L, 1), "stones": stones})
        return 1

    def stone(self, rng, u, v, dx, dy, base_z, length, thick, tall, course, poly, arc, spec):
        lv = self.l
        ft = rng.choice(self.n.WALL_STONES)
        ox, oy, oz, ex, ey, ez = lv.bounds[ft]
        upright = course == 99
        sx, sy, sz = length * 100.0 / (2 * ex), thick * 100.0 / (2 * ey), tall * 100.0 / (2 * ez)
        # 길이 방향 기울기는 밑선을 따른다(±2° 흔들림) — 맨 위까지 가로로 곧게
        p0, p1 = poly.at(max(arc, 0.0)), poly.at(min(arc + length, poly.length - 0.001))
        z0, z1 = lv.ground(p0[0], p0[1]), lv.ground(p1[0], p1[1])
        run = max(length, 0.1)
        pitch = max(-14.0, min(14.0, math.degrees(math.atan2((z1 - z0) / 100.0, run)))) * 0.8 + rng.uniform(-1.8, 1.8)
        roll = rng.uniform(-1.6, 1.6)
        cross = rng.uniform(-0.02, 0.02)
        yaw = 90.0 - math.degrees(math.atan2(dy, dx))
        map_yaw = 90.0 - yaw + rng.uniform(-2.5, 2.5)
        cu, cv = u - dy * cross, v + dx * cross
        embed = 0.12 * tall * 100.0
        bottom = base_z - embed
        if course == 0:
            # 맨 아랫단: 네 모서리 어디든 땅 위로 뜨지 않게 내린다(돌이 눌려 묻힌 자리)
            worst = -1e9
            for sa in (-0.48, 0.48):
                for st in (-0.45, 0.45):
                    cx_u = cu + dx * length * sa - dy * thick * st
                    cx_v = cv + dy * length * sa + dx * thick * st
                    worst = max(worst, bottom - lv.ground(cx_u, cx_v))
            if worst > 2.0:
                bottom -= worst - 2.0
        pivot_z = bottom - sz * (oz - ez)
        lv.put(ft, cu, cv, pivot_z, map_yaw, (sx, sy, sz), pitch, roll)
        lv.bump("WallStone")
        lv.wall_stones.append({"u": cu, "v": cv, "bottom": bottom, "top": bottom + tall * 100.0, "len": length, "thick": thick, "course": course,
                               "dx": dx, "dy": dy, "name": spec["Name"]})


def build_all(level3):
    builder = WallBuilder(level3)
    level3.wall_stones = []
    walls = 0
    for spec in level3.raw.get("StoneWalls", []):
        walls += builder.build(spec)
    return walls, builder.report
