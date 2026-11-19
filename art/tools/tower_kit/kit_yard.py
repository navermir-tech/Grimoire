"""탑 앞마당 키트 — 콘셉트 tower-exterior · tower-yard · tower-dusk: 둥근 돌 포장 + 놋쇠 상감 · 돌 화단 · 놋쇠 구 등 기둥 · 장작 헛간 · 통.

석등 · 화로 제단 · 돌무더기 · 우물은 둥근 덩어리라 Tripo 몫이다(art/env/tower-yard-*). Tripo 결과가 없을 때 쓰는 블렌더 대체 모델도
여기 있다(이름 끝 _Blender) — 둘 다 같은 크기 · 같은 원점(바닥 가운데)이라 바꿔 끼울 수 있다.
"""
import math

from kit_common import Kit, R, S, T, box, cylinder, ico, lathe, prism, ring_band, sphere, stone_block, torus

COURT_R = 9.0


def court(name="SM_Tower_Court", r_max=COURT_R):
    """둥근 돌 포장 마당(가운데 원점, 윗면 z = 0.04): 동심원 줄 판석 · 놋쇠 고리 둘 · 나침반 살 여덟 · 바깥 연석"""
    kit = Kit(name)
    rng = kit.rng
    kit.add(lathe([(0.0, 0.04), (1.1, 0.04), (1.12, 0.02), (1.12, -0.06), (0.0, -0.06)], 48), T(), "Flagstone")
    r = 1.15
    while r < r_max - 0.05:
        w = min(rng.uniform(0.55, 0.75), r_max - r)
        rm = r + w / 2
        circ = 2 * math.pi * rm
        n = max(6, int(circ / rng.uniform(0.75, 1.0)))
        a = rng.uniform(0, 2 * math.pi)
        lens = [rng.uniform(0.7, 1.3) for _ in range(n)]
        total = sum(lens)
        for L in lens:
            da = 2 * math.pi * L / total
            mid = a + da / 2
            a += da
            chord = 2 * rm * math.sin(da / 2)
            kit.add(stone_block(w - 0.018, chord - 0.018, 0.1, rng, bevel=0.02, jitter=0.004),
                    T(rm * math.cos(mid), rm * math.sin(mid), -0.01 + rng.uniform(-0.006, 0.008)) @ R(math.degrees(mid)), "Flagstone")
        r += w
    for rr, w in ((3.0, 0.08), (6.1, 0.1), (6.3, 0.03)):
        kit.add(ring_band(rr - w / 2, rr + w / 2, 0.035, 0.046, 160), T(), "Rune")
    for k in range(8):
        a = math.pi / 4 * k
        L = 5.95 if k % 2 == 0 else 4.2
        kit.add(prism([(1.25, -0.07), (L, -0.012), (L, 0.012), (1.25, 0.07)], 0.035, 0.046), R(math.degrees(a)), "Rune")
    kit.add(ring_band(r_max, r_max + 0.42, -0.12, 0.1, 128), T(), "StoneDark")
    return kit.build()


def flower_bed(name="SM_Tower_FlowerBed", r_in=COURT_R + 0.42, depth=1.3, arc_deg=54.0):
    """마당 둘레에 붙는 굽은 돌 화단(원점 = 마당 가운데, 화단은 +X 쪽 arc_deg 만큼) — 낮은 돌 테 · 흙 · 꽃(보라 꽃대 · 흰 수국 · 노랑)"""
    kit = Kit(name)
    rng = kit.rng
    a0, a1 = -math.radians(arc_deg) / 2, math.radians(arc_deg) / 2
    for rr in (r_in + 0.1, r_in + depth - 0.1):
        L = 0.0
        circ = rr * (a1 - a0)
        a = a0
        while a < a1 - 1e-4:
            seg = min(rng.uniform(0.45, 0.75), (a1 - a) * rr)
            da = seg / rr
            kit.add(stone_block(0.22, seg - 0.015, 0.36, rng, bevel=0.025), T(rr * math.cos(a + da / 2), rr * math.sin(a + da / 2), 0.15) @ R(math.degrees(a + da / 2)), "Stone")
            a += da
    for a in (a0, a1):
        rm = r_in + depth / 2
        kit.add(stone_block(depth, 0.22, 0.36, rng, bevel=0.025), T(rm * math.cos(a), rm * math.sin(a), 0.15) @ R(math.degrees(a)), "Stone")
    kit.add(ring_band(r_in + 0.2, r_in + depth - 0.2, 0.0, 0.26, 32, arc=a1 - a0, start=a0), T(), "Soil")
    # 꽃: 덤불(잎) + 꽃 머리
    n = int((a1 - a0) * (r_in + depth / 2) * depth * 9)
    for _ in range(n):
        rr = rng.uniform(r_in + 0.3, r_in + depth - 0.3)
        a = rng.uniform(a0 + 0.02, a1 - 0.02)
        x, y = rr * math.cos(a), rr * math.sin(a)
        kind = rng.random()
        leaf = (0.13 + rng.uniform(0, 0.05), 0.28 + rng.uniform(0, 0.1), 0.09)
        kit.add(ico(rng.uniform(0.12, 0.2), 2), T(x, y, 0.32) @ S(1, 1, 0.7), "Leaf", color=leaf, smooth=True)
        if kind < 0.4:          # 보라 꽃대(라벤더 · 루핀)
            h = rng.uniform(0.35, 0.6)
            for k in range(7):
                kit.add(ico(0.035 - k * 0.003, 2), T(x + rng.uniform(-0.01, 0.01), y, 0.42 + h * k / 7), "Leaf", color=(0.40, 0.28, 0.72))
        elif kind < 0.7:        # 흰 수국
            kit.add(ico(rng.uniform(0.09, 0.13), 2), T(x, y, 0.5), "Leaf", color=(0.92, 0.92, 0.90))
        else:                   # 노랑 · 주황 작은 꽃
            for k in range(4):
                kit.add(ico(0.035, 2), T(x + rng.uniform(-0.08, 0.08), y + rng.uniform(-0.08, 0.08), 0.46 + rng.uniform(0, 0.08)), "Leaf", color=rng.choice([(0.95, 0.78, 0.22), (0.95, 0.55, 0.18)]))
    return kit.build()


def orb_lamp(name="SM_Tower_OrbLamp"):
    """놋쇠 구 등 기둥(돌 받침 위 놋쇠 받침 + 빛나는 유리 구 + 놋쇠 고리) — 높이 1.75 m"""
    kit = Kit(name)
    rng = kit.rng
    kit.add(stone_block(0.56, 0.56, 0.16, rng, bevel=0.025), T(0, 0, 0.08), "StoneDark")
    kit.add(stone_block(0.42, 0.42, 0.86, rng, bevel=0.03), T(0, 0, 0.6), "Stone")
    kit.add(stone_block(0.54, 0.54, 0.12, rng, bevel=0.025), T(0, 0, 1.08), "Stone")
    kit.add(lathe([(0.0, 1.14), (0.16, 1.14), (0.1, 1.2), (0.07, 1.3), (0.15, 1.38), (0.0, 1.38)], 24), T(), "Brass", smooth=True)
    kit.add(sphere(0.2, 24, 14), T(0, 0, 1.56), "Glass", smooth=True)
    for rx in (0, 60, 120):
        kit.add(torus(0.215, 0.012, 32, 6), T(0, 0, 1.56) @ R(rx, 90), "Brass", smooth=True)
    kit.add(torus(0.215, 0.014, 32, 6), T(0, 0, 1.56), "Brass", smooth=True)
    kit.add(cylinder(0.03, 0.2, 8, radius_top=0.0), T(0, 0, 1.76), "Brass", smooth=True)
    return kit.build()


def barrel(name="SM_Tower_Barrel"):
    """참나무 통(지름 0.62 · 높이 0.9) + 쇠테 셋 + 뚜껑"""
    kit = Kit(name)
    prof = [(0.0, 0.0), (0.27, 0.0), (0.3, 0.12), (0.32, 0.45), (0.3, 0.78), (0.27, 0.9), (0.0, 0.9)]
    kit.add(lathe(prof, 20), T(), "Wood", smooth=True, uv_scale=1.0)
    for z, r in ((0.1, 0.298), (0.45, 0.322), (0.8, 0.298)):
        kit.add(torus(r, 0.014, 24, 6), T(0, 0, z) @ S(1, 1, 2.2), "Iron", smooth=True)
    return kit.build()


def wood_shed(name="SM_Tower_WoodShed"):
    """장작 헛간 3.6 × 1.6 m — 기둥 넷 · 남색 슬레이트 외쪽 지붕 · 쌓은 장작. 원점 = 바닥 가운데, 뒤(벽 쪽) = -X"""
    kit = Kit(name)
    rng = kit.rng
    w, d = 3.6, 1.6
    for sx, h in ((-1, 2.1), (1, 2.5)):
        for sy in (-1, 1):
            kit.add(box(0.14, 0.14, h, bevel=0.012, center_z=False), T(sx * (d / 2 - 0.1), sy * (w / 2 - 0.1), 0), "Wood")
    kit.add(box(d + 0.5, 0.12, 0.14, bevel=0.01), T(0, w / 2 - 0.1, 2.3) @ R(0, 0, -14), "Wood")
    kit.add(box(d + 0.5, 0.12, 0.14, bevel=0.01), T(0, -(w / 2 - 0.1), 2.3) @ R(0, 0, -14), "Wood")
    slope = -14
    for k in range(6):
        kit.add(box(0.42, w + 0.4, 0.05, bevel=0.008), T(-d / 2 - 0.15 + k * 0.38, 0, 2.17 + k * 0.38 * math.tan(math.radians(14)) + 0.12) @ R(0, 0, slope - 3), "Slate")
    kit.add(box(d + 0.6, w + 0.4, 0.06), T(0.05, 0, 2.38) @ R(0, 0, slope), "Slate")
    kit.add(box(d - 0.1, w - 0.1, 0.08), T(0, 0, 0.04), "Wood")
    for row in range(6):
        for col in range(int((w - 0.3) / 0.24)):
            r = rng.uniform(0.08, 0.12)
            y = -w / 2 + 0.25 + col * 0.24 + (0.12 if row % 2 else 0)
            if y > w / 2 - 0.2:
                continue
            kit.add(cylinder(r, d - 0.4, 8), T(-(d - 0.4) / 2, y, 0.17 + row * 0.2) @ R(0, 0, 90) @ R(rng.uniform(0, 45)), "Wood", smooth=True)
    return kit.build()


def shelter(name="SM_Tower_Shelter"):
    """앞마당 쉼터(지붕 있는 침낭 자리) 4.2 × 4.2 m — 나무 기둥 넷 · 띠장 · 남색 슬레이트 모임지붕. 원점 = 바닥 가운데"""
    kit = Kit(name)
    for sx in (-1, 1):
        for sy in (-1, 1):
            kit.add(box(0.2, 0.2, 2.95, bevel=0.015, center_z=False), T(sx * 1.8, sy * 1.8, 0), "Wood")
            kit.add(stone_block(0.36, 0.36, 0.2, kit.rng, bevel=0.03), T(sx * 1.8, sy * 1.8, 0.08), "StoneDark")
    for k in range(4):
        kit.add(box(3.9, 0.16, 0.22, bevel=0.012), T(0, 0, 2.85) @ R(90 * k) @ T(0, 1.8, 0), "Wood")
    rows = 7
    for i in range(rows):
        t0, t1 = i / rows, (i + 1) / rows
        r0, r1 = 3.3 * (1 - t0) + 0.05, 3.3 * (1 - t1) + 0.05
        z0, z1 = 2.95 + 1.35 * t0, 2.95 + 1.35 * t1
        kit.add(lathe([(r0 + 0.04, z0 - 0.02), (r1, z1)], 4, start=math.pi / 4), T(), "Slate")
    kit.add(lathe([(3.3, 2.88), (3.34, 2.95), (0.0, 4.3)], 4, start=math.pi / 4), T(), "Slate")
    kit.add(sphere(0.1, 10, 6), T(0, 0, 4.36), "Brass", smooth=True)
    return kit.build()


# ─────────────── Tripo 대체(블렌더) ───────────────

def stone_lantern_blender(name="SM_Tower_StoneLantern_Blender"):
    """돌 등불 표지(석등) 높이 1.7 m — 받침 · 기둥 · 네 면 트인 등 칸 · 들린 처마 지붕 · 구슬"""
    kit = Kit(name)
    rng = kit.rng
    kit.add(stone_block(0.6, 0.6, 0.2, rng, bevel=0.03), T(0, 0, 0.1), "StoneDark")
    kit.add(stone_block(0.46, 0.46, 0.14, rng, bevel=0.025), T(0, 0, 0.27), "Stone")
    kit.add(stone_block(0.26, 0.26, 0.6, rng, bevel=0.03), T(0, 0, 0.64), "Stone")
    kit.add(stone_block(0.5, 0.5, 0.1, rng, bevel=0.02), T(0, 0, 0.99), "Stone")
    for sx in (-1, 1):
        for sy in (-1, 1):
            kit.add(stone_block(0.1, 0.1, 0.36, rng, bevel=0.015), T(sx * 0.18, sy * 0.18, 1.22), "Stone")
    kit.add(lathe([(0.0, 1.4), (0.44, 1.4), (0.47, 1.45), (0.3, 1.55), (0.12, 1.66), (0.0, 1.7)], 4, start=math.pi / 4), T(), "Stone")
    kit.add(sphere(0.07, 10, 6), T(0, 0, 1.75), "Stone", smooth=True)
    return kit.build()


def brazier_blender(name="SM_Tower_Brazier_Blender"):
    """꺼진 돌 화로 제단 — 팔각 받침(지름 2.4 · 높이 0.9) · 모서리 버팀 · 불 그릇(재)"""
    kit = Kit(name)
    rng = kit.rng
    kit.add(lathe([(0.0, 0.0), (1.2, 0.0), (1.2, 0.2), (1.05, 0.25), (1.0, 0.75), (1.1, 0.8), (1.1, 0.9), (0.0, 0.9)], 8, start=math.pi / 8), T(), "Stone")
    for k in range(8):
        a = math.pi / 4 * k + math.pi / 8
        kit.add(stone_block(0.4, 0.3, 0.9, rng, bevel=0.04), T(1.18 * math.cos(a), 1.18 * math.sin(a), 0.42) @ R(math.degrees(a)) @ R(0, 0, -16), "Stone")
    kit.add(lathe([(0.75, 0.9), (0.8, 1.0), (0.72, 1.05), (0.55, 0.95), (0.0, 0.95)], 32), T(), "StoneDark", smooth=True)
    kit.add(ring_band(0.78, 0.82, 0.92, 1.02, 48), T(), "Brass")
    for k in range(10):
        kit.add(ico(rng.uniform(0.06, 0.12), 1), T(rng.uniform(-0.4, 0.4), rng.uniform(-0.4, 0.4), 0.98) @ S(1, 1, 0.5), "Iron")
    return kit.build()


def cairn_blender(name="SM_Tower_Cairn_Blender"):
    """길손의 돌무더기(높이 1.0 m) — 납작하게 눌린 둥근 돌 일곱"""
    kit = Kit(name)
    rng = kit.rng
    z = 0.0
    for k, r in enumerate([0.55, 0.48, 0.4, 0.33, 0.26, 0.2, 0.14]):
        h = r * 0.55
        bm = ico(r, 2)
        for v in bm.verts:
            v.co *= 1 + rng.uniform(-0.08, 0.08)
        kit.add(bm, T(rng.uniform(-0.04, 0.04), rng.uniform(-0.04, 0.04), z + h * 0.5) @ S(1, 1.1, 0.55) @ R(rng.uniform(0, 180)), "Stone", smooth=True)
        z += h * 0.85
    return kit.build()


def well_blender(name="SM_Tower_Well_Blender"):
    """돌 우물(지름 1.8 · 높이 0.8) + 나무 기둥 둘 · 박공 나무 지붕 · 도르래 · 두레박"""
    from kit_common import ashlar_ring
    kit = Kit(name)
    ashlar_ring(kit, lambda z: 0.9, 0.0, 0.8, 0.28, block=(0.35, 0.55), course=(0.26, 0.28))
    kit.add(ring_band(0.6, 0.95, 0.8, 0.88, 32), T(), "Stone")
    kit.add(cylinder(0.62, 0.05, 24), T(0, 0, 0.3), "Glass")
    for s in (-1, 1):
        kit.add(box(0.14, 0.14, 2.2, bevel=0.012, center_z=False), T(0, s * 0.82, 0.0), "Wood")
    kit.add(cylinder(0.07, 1.7, 12), T(0, -0.85, 1.4) @ R(0, -90), "Wood", smooth=True)
    kit.add(cylinder(0.11, 0.4, 12), T(0, -0.2, 1.4) @ R(0, -90), "Wood", smooth=True)
    for s in (-1, 1):
        kit.add(box(1.5, 1.2, 0.05), T(s * 0.42, 0, 2.38) @ R(0, 0, -s * 32) @ R(90), "Wood")
    kit.add(lathe([(0.0, 0.0), (0.15, 0.0), (0.17, 0.22), (0.0, 0.22)], 12), T(0.55, 0.3, 0.88), "Wood", smooth=True)
    return kit.build()


def build_all():
    return [court(), flower_bed(), orb_lamp(), barrel(), wood_shed(), shelter(), stone_lantern_blender(), brazier_blender(), cairn_blender(), well_blender()]
