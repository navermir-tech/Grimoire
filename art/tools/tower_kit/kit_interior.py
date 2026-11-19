"""마법탑 1층 실내 키트 — 탑 실내 키트 T01~T13(docs/art/region1-assets.md) 가운데 이번 시범에 필요한 것.

그레이박스 방 배치(Scripts/Tutorial/tower_greybox.py: 벽 높이 5 m · 두께 40 cm · 문 높이 3 m)에 맞춘 치수다.
벽 조각은 길이 4 · 2 · 1 m, 문 위 상인방 · 문틀은 문 폭 2 · 3 · 4 m. 놓는 일은 Scripts/Tutorial/tower_art_dress.py.
"""
import math

from kit_common import Kit, R, S, T, ashlar_wall, box, cylinder, ico, lathe, panel_xz, prism, ring_band, sphere, stone_block, torus

WALL_H, WALL_T, DOOR_H = 5.0, 0.4, 3.0


def wall_piece(name, length, z0=0.0, z1=WALL_H, seed=None, lintel=False):
    """실내 벽 한 조각(가운데 원점, X 로 뻗음). 아래 굽돌 띠 · 마름돌 · 위 나무 띠(천장 아래)"""
    kit = Kit(name, seed)
    h = z1 - z0
    if not lintel:
        ashlar_wall(kit, length, 0.0, 0.36, WALL_T + 0.06, x0=-length / 2, mat="StoneDark", block=(0.9, 1.5), course=(0.36, 0.36))
        ashlar_wall(kit, length, 0.36, h - 0.28, WALL_T, x0=-length / 2, block=(0.6, 1.2), course=(0.36, 0.46))
    else:
        # 문 위(문 높이 3 m 부터): 아래 참나무 상인방 · 그 위 마름돌
        kit.add(box(length, WALL_T + 0.08, 0.3, bevel=0.02), T(0, 0, 0.15), "Wood")
        ashlar_wall(kit, length, 0.3, h - 0.28, WALL_T, x0=-length / 2, block=(0.6, 1.2), course=(0.36, 0.46))
    kit.add(box(length, WALL_T + 0.1, 0.28, bevel=0.02), T(0, 0, h - 0.14), "Wood")
    return kit.build()


def door_frame(name, width):
    """문 둘레 돌 문설주 + 위 머릿돌(벽 양면으로 4 cm 튀어나옴). 문 구멍 바깥에 붙어 길을 좁히지 않는다"""
    kit = Kit(name)
    t = WALL_T + 0.08
    for side in (-1, 1):
        x = side * (width / 2 + 0.16)
        z = 0.0
        for k in range(6):
            hgt = 0.5
            wdt = 0.34 if k % 2 == 0 else 0.26
            kit.add(stone_block(wdt, t, hgt - 0.012, kit.rng, bevel=0.025, jitter=0.004), T(x + side * (wdt - 0.3) / 2, 0, z + hgt / 2), "Stone")
            z += hgt
    kit.add(stone_block(width + 0.7, t, 0.34, kit.rng, bevel=0.03, jitter=0.004), T(0, 0, DOOR_H + 0.17), "Stone")
    kit.add(stone_block(0.34, t + 0.02, 0.44, kit.rng, bevel=0.03, jitter=0.004), T(0, 0, DOOR_H + 0.2), "Stone")
    return kit.build()


def floor_flag(name, size=4.0):
    """판석 바닥 4 × 4 m(가운데 원점, 윗면 z = 0.03)"""
    from kit_exterior import flag_floor
    kit = Kit(name)
    flag_floor(kit, -size / 2, size / 2, -size / 2, size / 2, top=0.03)
    return kit.build()


def floor_wood(name, size=4.0):
    """나무 마루 4 × 4 m — 널 폭 22 cm, 끝이 엇갈린다"""
    kit = Kit(name)
    rng = kit.rng
    y = -size / 2
    while y < size / 2 - 0.01:
        wdt = min(0.22, size / 2 - y)
        x = -size / 2 - rng.uniform(0, 1.6)
        while x < size / 2:
            L = rng.uniform(1.2, 2.6)
            a, b = max(x, -size / 2), min(x + L, size / 2)
            if b - a > 0.1:
                kit.add(box(b - a - 0.008, wdt - 0.008, 0.04, bevel=0.006), T((a + b) / 2, y + wdt / 2, 0.01 + rng.uniform(-0.003, 0.003)), "Wood", uv_offset=(rng.random() * 5, rng.random() * 5))
            x += L
        y += wdt
    return kit.build()


def ceiling(name, size=4.0):
    """나무 천장 4 × 4 m: 위 널판 + 가운데 큰 들보 하나 + 작은 장선. 윗면 z = 5.0(벽 위), 들보 아래 4.62"""
    kit = Kit(name)
    kit.add(box(size, size, 0.06), T(0, 0, WALL_H - 0.03), "Wood")
    kit.add(box(0.28, size, 0.32, bevel=0.02), T(0, 0, WALL_H - 0.22), "Wood")
    for k in range(-3, 4):
        if k == 0:
            continue
        kit.add(box(0.1, size, 0.14, bevel=0.01), T(k * size / 7, 0, WALL_H - 0.12), "Wood")
    return kit.build()


def bookcase(name="SM_TowerIn_Bookcase"):
    """책장 1.6(폭) × 0.55(깊이) × 2.6 m, 위 아치 장식 · 책 다섯 칸(색 가죽). 원점 = 바닥 가운데, 앞 = +X"""
    kit = Kit(name)
    rng = kit.rng
    w, d, h = 1.6, 0.55, 2.6
    for s in (-1, 1):
        kit.add(box(d, 0.07, h, bevel=0.01, center_z=False), T(0, s * (w / 2 - 0.035), 0), "Wood")
    kit.add(box(0.03, w, h, center_z=False), T(-d / 2 + 0.015, 0, 0), "Wood")
    kit.add(box(d + 0.06, w + 0.1, 0.14, bevel=0.02), T(0.02, 0, 0.07), "Wood")
    kit.add(box(d + 0.08, w + 0.14, 0.1, bevel=0.02), T(0.02, 0, h + 0.05), "Wood")
    semi = [((w / 2 - 0.05) * math.cos(math.pi * i / 16), 0.28 * math.sin(math.pi * i / 16)) for i in range(17)]
    kit.add(panel_xz(semi, -0.025, 0.025), T(d / 2 - 0.05, 0, h + 0.1) @ R(90), "Wood")          # 위 반원 장식판
    kit.add(sphere(0.05, 10, 6), T(d / 2 - 0.05, 0, h + 0.43), "Brass", smooth=True)
    shelves = [0.14, 0.62, 1.1, 1.58, 2.06]
    for zs in shelves[1:]:
        kit.add(box(d - 0.04, w - 0.14, 0.035), T(0, 0, zs), "Wood")
    palette = [(0.12, 0.16, 0.38), (0.42, 0.10, 0.10), (0.14, 0.30, 0.18), (0.40, 0.26, 0.12), (0.55, 0.42, 0.20), (0.22, 0.12, 0.25), (0.30, 0.20, 0.10)]
    for k, zs in enumerate(shelves):
        y = -w / 2 + 0.09
        top = (shelves[k + 1] if k + 1 < len(shelves) else h) - 0.03
        while y < w / 2 - 0.12:
            bw = rng.uniform(0.03, 0.075)
            bh = min(rng.uniform(0.22, 0.38), top - zs - 0.05)
            bd = rng.uniform(0.2, 0.28)
            lean = rng.uniform(-6, 6) if rng.random() < 0.15 else 0.0
            c = rng.choice(palette)
            c = tuple(min(1.0, v * rng.uniform(0.8, 1.2)) for v in c)
            kit.add(box(bd, bw, bh, bevel=0.004), T(d / 2 - bd / 2 - 0.04, y + bw / 2, zs + 0.035 + bh / 2) @ R(0, lean), "Cloth", color=c)
            y += bw + rng.uniform(0.0, 0.01)
            if rng.random() < 0.06:
                y += rng.uniform(0.08, 0.2)
    return kit.build()


def hearth(name="SM_TowerIn_Hearth"):
    """주방 아궁이: 돌 단(2.0 × 1.4 × 0.5, 윗면에 모닥불 액터) + 뒤 돌 갓(북벽에 붙음). 원점 = 단 바닥 가운데, 뒤 = +Y"""
    kit = Kit(name)
    rng = kit.rng
    ashlar_wall(kit, 2.0, 0.0, 0.5, 1.4, x0=-1.0, mat="StoneDark", block=(0.45, 0.8), course=(0.25, 0.25))
    ashlar_wall(kit, 2.8, 0.0, 4.62, 0.5, x0=-1.4, y=0.98, block=(0.45, 0.8), course=(0.3, 0.4))      # 굴뚝 뒷벽(방 북벽까지)
    kit.add(box(2.1, 1.5, 0.06, bevel=0.015), T(0, 0, 0.5), "StoneDark")
    for s in (-1, 1):
        for k in range(5):
            kit.add(stone_block(0.4, 0.55, 0.43, rng, bevel=0.025), T(s * 1.02, 0.45, 0.72 + k * 0.44), "Stone")
    kit.add(box(2.6, 0.65, 0.34, bevel=0.02), T(0, 0.42, 2.92), "Wood")
    kit.add(prism([(-1.35, -0.3), (1.35, -0.3), (1.35, 0.32), (-1.35, 0.32)], 0, 0.01), T(0, 0.43, 3.1), "Stone")
    for k in range(4):
        wdt = 2.7 - k * 0.35
        kit.add(stone_block(wdt, 0.62 - k * 0.06, 0.38, rng, bevel=0.025), T(0, 0.45 + k * 0.03, 3.3 + k * 0.4), "Stone")
    kit.add(box(0.05, 0.05, 1.3), T(0, 0.3, 2.2) @ R(0, 0, 90), "Iron")
    for x in (-0.7, 0.7):
        kit.add(box(0.04, 0.04, 0.9, center_z=False), T(x, 0.3, 1.8), "Iron")
    return kit.build()


def counter(name="SM_TowerIn_Counter"):
    """긴 나무 조리대 2.6 × 0.7 × 0.9 m(서랍 · 놋쇠 손잡이 · 두꺼운 상판). 원점 = 바닥 가운데, 앞 = -Y"""
    kit = Kit(name)
    kit.add(box(2.5, 0.62, 0.82, bevel=0.01, center_z=False), T(0, 0.02, 0), "Wood")
    kit.add(box(2.64, 0.74, 0.08, bevel=0.015), T(0, 0, 0.86), "Wood")
    for k in range(4):
        x = -0.94 + k * 0.627
        kit.add(box(0.56, 0.03, 0.22, bevel=0.008), T(x, -0.3, 0.66), "Wood")
        kit.add(box(0.56, 0.03, 0.4, bevel=0.008), T(x, -0.3, 0.3), "Wood")
        kit.add(sphere(0.022, 8, 6), T(x, -0.33, 0.66), "Brass", smooth=True)
        kit.add(sphere(0.022, 8, 6), T(x + 0.2, -0.33, 0.4), "Brass", smooth=True)
    return kit.build()


def desk(name="SM_TowerIn_Desk"):
    """책상 1.2 × 0.7, 윗면 높이 0.8 m(양초 액터가 그 위). 깎은 다리 넷 · 서랍 하나"""
    kit = Kit(name)
    kit.add(box(1.24, 0.74, 0.05, bevel=0.012), T(0, 0, 0.775), "Wood")
    for sx in (-1, 1):
        for sy in (-1, 1):
            kit.add(lathe([(0.035, 0.0), (0.04, 0.05), (0.03, 0.1), (0.032, 0.5), (0.042, 0.56), (0.03, 0.62), (0.04, 0.75), (0.0, 0.75)], 12), T(sx * 0.54, sy * 0.3, 0), "Wood", smooth=True)
    kit.add(box(1.1, 0.62, 0.1), T(0, 0, 0.7), "Wood")
    kit.add(box(0.4, 0.02, 0.07, bevel=0.005), T(0, -0.32, 0.7), "Wood")
    kit.add(sphere(0.018, 8, 6), T(0, -0.34, 0.7), "Brass", smooth=True)
    kit.add(box(0.2, 0.14, 0.035), T(-0.35, 0.1, 0.82) @ R(12), "Cloth", color=(0.42, 0.10, 0.10))
    kit.add(box(0.17, 0.12, 0.03), T(-0.35, 0.1, 0.852) @ R(4), "Cloth", color=(0.12, 0.16, 0.38))
    return kit.build()


def rug(name="SM_TowerIn_Rug", w=2.0, d=1.0):
    """깔개(남색 · 금빛 테 그림) — UV 0~1 = 깔개 그림 한 장. 윗면 판(1.2 cm 위) + 얇은 옆 테두리"""
    import bmesh
    kit = Kit(name)
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    bmesh.ops.create_grid(bm, x_segments=8, y_segments=4, size=0.5, calc_uvs=False)
    for v in bm.verts:
        v.co.x *= w * 2 / 2
        v.co.y *= d * 2 / 2
        v.co.z = 0.012
    for f in bm.faces:
        for loop in f.loops:
            loop[uvl].uv = (loop.vert.co.x / w + 0.5, loop.vert.co.y / d + 0.5)
    kit.add(bm, T(), "Rug", keep_uv=True)
    for sy in (-1, 1):
        kit.add(box(w, 0.02, 0.012), T(0, sy * d / 2, 0.006), "Rug", uv_offset=(0.02, 0.02), uv_scale=0.001)
    for sx in (-1, 1):
        kit.add(box(0.02, d, 0.012), T(sx * w / 2, 0, 0.006), "Rug", uv_offset=(0.02, 0.02), uv_scale=0.001)
    return kit.build()


def planter(name="SM_TowerIn_Planter"):
    """홀 화단 5.2 × 1.2 m, 윗면 0.30 m(작물 자리 액터가 그 높이) — 돌 테두리 · 놋쇠 띠 · 흙"""
    kit = Kit(name)
    rng = kit.rng
    L, W, H = 5.2, 1.2, 0.3
    for s in (-1, 1):
        ashlar_wall(kit, L, 0.0, H - 0.04, 0.16, x0=-L / 2, y=s * (W / 2 - 0.08), block=(0.5, 0.9), course=(0.26, 0.26))
        kit.add(box(0.16, W - 0.32, H - 0.04, bevel=0.015, center_z=False), T(s * (L / 2 - 0.08), 0, 0), "Stone")
    kit.add(box(L + 0.06, 0.2, 0.04, bevel=0.01), T(0, -(W / 2 - 0.08), H - 0.02), "Brass")
    kit.add(box(L + 0.06, 0.2, 0.04, bevel=0.01), T(0, (W / 2 - 0.08), H - 0.02), "Brass")
    kit.add(box(L - 0.3, W - 0.3, 0.2), T(0, 0, H - 0.12), "Soil")
    for k in range(40):
        x, y = rng.uniform(-L / 2 + 0.3, L / 2 - 0.3), rng.uniform(-W / 2 + 0.25, W / 2 - 0.25)
        kit.add(ico(rng.uniform(0.02, 0.05), 1), T(x, y, H - 0.02) @ S(1, 1, 0.4), "Soil")
    return kit.build()


def pedestal(name="SM_TowerIn_Pedestal"):
    """시험 양초 받침돌 0.4 × 0.4 × 0.8 m(윗면 0.8 — 양초 액터)"""
    kit = Kit(name)
    rng = kit.rng
    kit.add(stone_block(0.5, 0.5, 0.14, rng, bevel=0.025), T(0, 0, 0.07), "StoneDark")
    kit.add(lathe([(0.17, 0.14), (0.15, 0.2), (0.15, 0.62), (0.18, 0.68), (0.0, 0.68)], 8, start=math.pi / 8), T(), "Stone")
    kit.add(stone_block(0.46, 0.46, 0.1, rng, bevel=0.02), T(0, 0, 0.73), "Stone")
    kit.add(box(0.47, 0.47, 0.02), T(0, 0, 0.79), "Brass")
    return kit.build()


def rune_inlay(name="SM_TowerIn_RuneCircle", r=3.2):
    """바닥 놋쇠 상감 원(마법진 · 서클 고리, T13) — 바닥 윗면 위 4 mm"""
    kit = Kit(name)
    z0, z1 = 0.03, 0.036
    for rr, w in ((r, 0.07), (r - 0.18, 0.03), (r * 0.62, 0.05), (r * 0.25, 0.04)):
        kit.add(ring_band(rr - w / 2, rr + w / 2, z0, z1, 128), T(), "Rune")
    for k in range(8):
        a = math.pi / 4 * k
        L = r * (0.95 if k % 2 == 0 else 0.62)
        kit.add(prism([(r * 0.25, -0.03), (L, -0.006), (L, 0.006), (r * 0.25, 0.03)], z0, z1), R(math.degrees(a)), "Rune")
    for k in range(6):
        a = math.pi / 3 * k + math.pi / 6
        kit.add(ring_band(0.2, 0.26, z0, z1, 32), T(r * 0.8 * math.cos(a), r * 0.8 * math.sin(a), 0), "Rune")
    return kit.build()


def stair_spiral(name="SM_TowerIn_SpiralStair", r_out=1.9, height=4.6, steps=22):
    """홀의 나선 계단(돌 디딤 · 가운데 기둥 · 놋쇠 난간) — 위층은 아직 없어 천장에서 끝난다(장식)"""
    kit = Kit(name)
    rng = kit.rng
    rise = height / steps
    turn = math.radians(300) / steps
    kit.add(cylinder(0.22, height + 0.3, 16), T(), "Stone", smooth=True)
    for i in range(steps):
        a = i * turn
        z = (i + 1) * rise
        bm = prism([(0.18, -0.12), (r_out, -0.42), (r_out, 0.42), (0.18, 0.12)], -0.08, 0.0)
        kit.add(bm, T(0, 0, z) @ R(math.degrees(a)), "Stone")
        kit.add(box(0.03, 0.03, 0.95, center_z=False), T((r_out - 0.06) * math.cos(a), (r_out - 0.06) * math.sin(a), z), "Iron")
    pts = []
    for i in range(steps * 2 + 1):
        a = i * turn / 2
        pts.append(((r_out - 0.06) * math.cos(a), (r_out - 0.06) * math.sin(a), rise + i * rise / 2 + 0.95))
    for p, q in zip(pts[:-1], pts[1:]):
        mid = [(p[k] + q[k]) / 2 for k in range(3)]
        dx, dy, dz = q[0] - p[0], q[1] - p[1], q[2] - p[2]
        L = math.sqrt(dx * dx + dy * dy + dz * dz)
        yaw = math.degrees(math.atan2(dy, dx))
        pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy)))
        kit.add(box(L + 0.01, 0.05, 0.05), T(*mid) @ R(yaw, 0, -pitch), "Brass")
    return kit.build()


def banner_wall(name="SM_TowerIn_Banner"):
    from kit_exterior import build_banner
    return build_banner(name, 0.9, 2.6)


def build_all():
    objs = [
        wall_piece("SM_TowerIn_Wall400A", 4.0, seed=401), wall_piece("SM_TowerIn_Wall400B", 4.0, seed=402),
        wall_piece("SM_TowerIn_Wall200", 2.0, seed=201), wall_piece("SM_TowerIn_Wall100", 1.0, seed=101),
    ]
    for w in (200, 300, 400):
        objs.append(wall_piece(f"SM_TowerIn_Lintel{w}", w / 100, z0=DOOR_H, seed=w + 5, lintel=True))
        objs.append(door_frame(f"SM_TowerIn_DoorFrame{w}", w / 100))
    objs += [floor_flag("SM_TowerIn_FloorFlagA"), floor_flag("SM_TowerIn_FloorFlagB"), floor_wood("SM_TowerIn_FloorWood"), ceiling("SM_TowerIn_Ceiling")]
    objs += [bookcase(), hearth(), counter(), desk(), rug(), planter(), pedestal(), rune_inlay(), stair_spiral(), banner_wall()]
    return objs
