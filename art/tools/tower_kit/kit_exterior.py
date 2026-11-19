"""마법탑 외관 키트 — 콘셉트 tower-exterior(Q95): 양피지색 마름돌 원통 · 남색 슬레이트 원뿔 지붕 · 놋쇠 혼천의 관측실 · 동쪽 현관.

탑 1층 방 배치(32 × 24 m, Scripts/Tutorial/tower_greybox.py)를 그대로 두려고 1층은 반지름 20.8 m 둥근 기단(드럼)으로 감싸고,
그 위로 가는 탑 몸(반지름 7.4 → 6.6 m)이 38 m 까지 오른다(docs/art/tower-art.md 「콘셉트와 다른 점」).
"""
import math

from kit_common import (Kit, Opening, R, S, T, arch_half, arch_height, arch_outline, ashlar_ring, ashlar_wall, box, circle_outline, cylinder,
                        frame_xz, lathe, panel_xz, polar, prism, rect_half, rect_outline, resample_star, ring_band, round_half, sphere, stone_block,
                        torus)

DRUM_R = 20.8          # 기단 바깥 반지름
DRUM_T = 0.75
DRUM_H = 6.0
SHAFT_Z0, SHAFT_Z1 = 5.5, 29.6
FLOOR_ZS = [9.5, 13.5, 17.5, 21.5, 25.5]   # 층 띠(몰딩) 높이 — 층마다 창
DOOR_HALF = 2.3        # 기단 문 구멍 반폭(현관 안쪽)


def shaft_r(z):
    t = (z - SHAFT_Z0) / (SHAFT_Z1 - SHAFT_Z0)
    return 7.4 - 0.8 * max(0.0, min(1.0, t))


# ─────────────── 창 ───────────────

BORDER = 0.2   # 창 둘레 돌테 폭


def win_opening(at, base, kind, w, spring=1.5):
    """ashlar_ring / ashlar_wall 이 돌을 끊을 구멍(창 바닥 = base)"""
    if kind == "round":
        return Opening(at, base, round_half(w), lambda h: round_half(w + 2 * BORDER)(h + BORDER), w + BORDER)
    if kind == "rect":
        return Opening(at, base, rect_half(w, spring), rect_half(w + 2 * BORDER, spring + BORDER), spring + BORDER)
    return Opening(at, base, arch_half(w, spring), arch_half(w + 2 * BORDER, spring), arch_height(w + 2 * BORDER, spring))


def window(kit, M, kind, w, spring=1.5, depth=0.34, mullion=True, glass="Glass"):
    """벽 바깥면 위 창 하나. M = 창 아래 가운데(턱) 위치, 로컬 +X = 바깥. kind = arch(뾰족) · round · rect.
    둘레 돌테는 구멍과 같은 모양(폭 BORDER)이라 둘레 마름돌이 그대로 보인다"""
    if kind == "round":
        inner = circle_outline(w / 2, w / 2, 48)
        outer = circle_outline(w / 2 + BORDER, w / 2, 48)
        top, center = w, (0.0, w / 2)
    elif kind == "rect":
        inner = rect_outline(w, spring)
        outer = rect_outline(w + 2 * BORDER, spring + BORDER, z0=0.0)
        top, center = spring, (0.0, spring * 0.5)
    else:
        inner = arch_outline(w, spring, 32)
        outer = arch_outline(w + 2 * BORDER, spring, 32)
        top, center = arch_height(w, spring), (0.0, spring * 0.55)
    n = 64
    o = resample_star(outer, center, n)
    i = resample_star(inner, center, n)
    rot = R(-90)
    kit.add(frame_xz(o, i, -depth, 0.09), M @ rot, "Stone")
    kit.add(panel_xz(inner, -depth - 0.04, -depth), M @ rot, glass)
    if kind != "round":
        kit.add(box(0.26, w + 2 * BORDER + 0.16, 0.12, bevel=0.02), M @ T(0.07, 0, -0.06), "Stone")   # 창턱
    if kind == "arch":                                            # 쐐기돌
        kit.add(stone_block(0.24, 0.34, 0.42, kit.rng, bevel=0.03, jitter=0.004), M @ T(0.04, 0, arch_height(w + 2 * BORDER, spring) - 0.16), "Stone")
    if mullion:
        bar = 0.045
        if kind == "round":
            kit.add(box(bar, bar, w), M @ T(-depth + 0.03, 0, w / 2), "Iron")
            kit.add(box(bar, w, bar), M @ T(-depth + 0.03, 0, w / 2), "Iron")
        else:
            kit.add(box(bar, bar, top), M @ T(-depth + 0.03, 0, top / 2), "Iron")
            for zz in (top * 0.33, spring * 0.92):
                kit.add(box(bar, w, bar), M @ T(-depth + 0.03, 0, zz), "Iron")
    return top


# ─────────────── 기단(드럼) ───────────────

def drum_windows():
    out = []
    for k in range(14):
        a = 2 * math.pi * k / 14 + math.pi / 14
        d = (a + math.pi) % (2 * math.pi) - math.pi
        if abs(d) < math.radians(30):
            continue
        out.append(a)
    return out


def build_drum():
    kit = Kit("SM_Tower_Drum")
    wins = drum_windows()
    w, spring, sill = 1.2, 2.0, 1.4
    top = arch_height(w, spring)
    door = Opening(0.0, 0.0, rect_half(2 * DOOR_HALF, 6.2), rect_half(2 * DOOR_HALF, 6.2), 6.2)
    openings = [door] + [win_opening(a, sill, "arch", w, spring) for a in wins]
    # 바닥 띠(어두운 돌, 살짝 두껍게) · 몸 · 처마 몰딩
    ashlar_ring(kit, lambda z: DRUM_R + 0.1, 0.0, 0.55, DRUM_T + 0.1, openings=[door], mat="StoneDark", block=(1.1, 1.6), course=(0.55, 0.55))
    ashlar_ring(kit, lambda z: DRUM_R, 0.55, 5.4, DRUM_T, openings=openings)
    prof = [(DRUM_R - 0.3, 5.38), (DRUM_R + 0.04, 5.38), (DRUM_R + 0.04, 5.5), (DRUM_R + 0.14, 5.56), (DRUM_R + 0.14, 5.7), (DRUM_R + 0.26, 5.78), (DRUM_R + 0.26, 6.0), (DRUM_R - 0.3, 6.0)]
    door_gap = math.asin((DOOR_HALF + 0.4) / DRUM_R)
    kit.add(lathe(prof, 160, arc=2 * math.pi - 2 * door_gap, start=door_gap), mat="Stone")
    for a in wins:
        window(kit, polar(DRUM_R, a, sill), "arch", w, spring, depth=0.38)
    # 버팀벽(창 사이 하나 걸러) — 둥근 기단의 실루엣에 세로 리듬
    for k, a in enumerate(wins[:-1]):
        if k % 2:
            continue
        mid = (a + wins[k + 1]) / 2
        for step, (dx, h, wdt) in enumerate([(0.85, 1.7, 0.75), (0.6, 3.4, 0.65), (0.38, 4.9, 0.58)]):
            z0 = [0.0, 1.7, 3.4][step]
            kit.add(stone_block(dx + 0.3, wdt, h - z0 - 0.01, kit.rng, bevel=0.035), polar(DRUM_R + dx / 2 - 0.15, mid, (z0 + h) / 2), "Stone")
            kit.add(box(dx + 0.38, wdt + 0.08, 0.12, bevel=0.03), polar(DRUM_R + dx / 2 - 0.13, mid, h + 0.05) @ R(0, 0, -14), "StoneDark")
    return kit.build()


def build_drum_roof():
    kit = Kit("SM_Tower_DrumRoof")
    re, ze, rt, zt = DRUM_R + 0.5, 5.98, 6.9, 9.3
    rows = 11
    prof = [(DRUM_R - 0.2, 5.84), (re, 5.84), (re + 0.03, 5.98)]
    for i in range(rows):
        t0, t1 = i / rows, (i + 1) / rows
        r0, z0 = re + (rt - re) * t0, ze + (zt - ze) * t0
        r1, z1 = re + (rt - re) * t1, ze + (zt - ze) * t1
        prof += [(r0, z0 + 0.06), (r1 + 0.02, z1 - 0.01)]
    prof += [(rt, zt + 0.06), (rt - 0.4, zt + 0.3)]
    kit.add(lathe(prof, 160), mat="Slate", smooth=True, uv_scale=1.6)
    kit.add(torus(re + 0.04, 0.045, 160, 8), T(0, 0, 5.92), "Brass", smooth=True)
    kit.add(torus(rt + 0.05, 0.05, 96, 8), T(0, 0, zt + 0.08), "Brass", smooth=True)
    return kit.build()


# ─────────────── 탑 몸 ───────────────

def shaft_windows():
    out = []
    for f, fz in enumerate(FLOOR_ZS):
        a0 = 0.0 if f % 2 == 0 else math.pi / 4
        for k in range(4):
            a = a0 + k * math.pi / 2
            kind = "arch" if (k + f) % 2 == 0 else "round"
            out.append((f, a, kind, fz))
    return out


def build_shaft():
    kit = Kit("SM_Tower_Shaft")
    openings = []
    for f, a, kind, fz in shaft_windows():
        if kind == "arch":
            openings.append(win_opening(a, fz + 0.9, "arch", 1.0, 1.55))
        else:
            openings.append(win_opening(a, fz + 1.4, "round", 1.1))
    ashlar_ring(kit, shaft_r, SHAFT_Z0, SHAFT_Z1, 0.6, openings=openings, block=(0.75, 1.25), course=(0.42, 0.5))
    # 층 띠 몰딩
    for fz in FLOOR_ZS + [SHAFT_Z1 - 0.3]:
        r = shaft_r(fz)
        prof = [(r - 0.3, fz - 0.16), (r + 0.02, fz - 0.16), (r + 0.1, fz - 0.06), (r + 0.16, fz - 0.06), (r + 0.16, fz + 0.1), (r + 0.06, fz + 0.16), (r - 0.3, fz + 0.16)]
        kit.add(lathe(prof, 96), mat="Stone")
    for f, a, kind, fz in shaft_windows():
        if kind == "arch":
            z = fz + 0.9
            M = polar(shaft_r(z + 1), a, z)
            window(kit, M, "arch", 1.0, 1.55)
            # 층마다 아치 창 하나에 발코니(층마다 방향을 돌린다) — 콘셉트의 층층 발코니
            arch_as = [aa for ff, aa, kk, _ in shaft_windows() if ff == f and kk == "arch"]
            if a == arch_as[f % len(arch_as)]:
                balcony(kit, polar(shaft_r(z), a, z - 0.12))
        else:
            z = fz + 1.4
            window(kit, polar(shaft_r(z + 0.5), a, z), "round", 1.1)
    # 꼭대기 층(관측실 아래) 처마 받침돌
    for k in range(36):
        a = 2 * math.pi * k / 36
        r = shaft_r(SHAFT_Z1)
        kit.add(stone_block(0.55, 0.3, 0.32, kit.rng, bevel=0.03), polar(r + 0.15, a, SHAFT_Z1 - 0.62), "Stone")
        kit.add(stone_block(0.8, 0.34, 0.22, kit.rng, bevel=0.03), polar(r + 0.3, a, SHAFT_Z1 - 0.34), "Stone")
    return kit.build()


def balcony(kit, M, radius=1.25):
    """반원 돌 발코니 + 받침돌 + 철 난간 + 꽃 상자. M = 창 턱 바깥면, +X 바깥"""
    n = 16
    pts = [(0.0, -radius)] + [(radius * math.cos(-math.pi / 2 + math.pi * i / n), radius * math.sin(-math.pi / 2 + math.pi * i / n)) for i in range(1, n)] + [(0.0, radius)]
    kit.add(prism(pts, -0.2, 0.0), M, "Stone")
    for k, y in enumerate((-0.7, 0.0, 0.7)):
        kit.add(stone_block(0.5 + (0.2 if k == 1 else 0), 0.25, 0.45, kit.rng, bevel=0.03), M @ T(0.22, y, -0.42) @ R(0, 0, 18), "Stone")
    for i in range(n + 1):
        a = -math.pi / 2 + math.pi * i / n
        kit.add(box(0.035, 0.035, 0.95, center_z=False), M @ T((radius - 0.08) * math.cos(a), (radius - 0.08) * math.sin(a), 0.0), "Iron")
    kit.add(torus(radius - 0.08, 0.03, 32, 6, arc=math.pi), M @ R(-90) @ T(0, 0, 0.95), "Iron", smooth=True)
    kit.add(torus(radius - 0.08, 0.02, 32, 6, arc=math.pi), M @ R(-90) @ T(0, 0, 0.5), "Iron", smooth=True)
    flower_box(kit, M @ T(radius - 0.05, 0, 0.82), 1.0)


def flower_box(kit, M, length):
    """난간에 건 나무 꽃 상자 + 보라 · 흰 · 노랑 꽃"""
    kit.add(box(0.22, length, 0.18, bevel=0.015), M @ T(0.12, 0, 0), "Wood")
    rng = kit.rng
    for i in range(int(length * 14)):
        y = rng.uniform(-length / 2 + 0.05, length / 2 - 0.05)
        x = 0.12 + rng.uniform(-0.06, 0.06)
        kit.add(ico(rng.uniform(0.05, 0.08), 2), M @ T(x, y, 0.12) @ S(1, 1, 0.7), "Leaf", color=(0.16, 0.30 + rng.uniform(0, 0.08), 0.10))
        c = rng.choice([(0.42, 0.28, 0.75), (0.92, 0.92, 0.88), (0.95, 0.78, 0.25), (0.55, 0.35, 0.80)])
        kit.add(ico(rng.uniform(0.025, 0.04), 2), M @ T(x + rng.uniform(-0.03, 0.03), y, 0.17 + rng.uniform(0, 0.05)), "Leaf", color=c)


def ico(r, subdiv=1):
    from kit_common import ico as _ico
    return _ico(r, subdiv)


# ─────────────── 관측실 · 지붕 · 혼천의 ───────────────

def build_observatory():
    kit = Kit("SM_Tower_Observatory")
    z0 = SHAFT_Z1
    kit.add(ring_band(5.6, 8.1, z0, z0 + 0.38, 128), mat="Stone")                              # 둘레 걸음판
    kit.add(lathe([(8.1, z0), (8.16, z0 + 0.06), (8.16, z0 + 0.3), (8.1, z0 + 0.38)], 128), mat="Stone")
    for k in range(32):                                                                           # 받침돌(층층)
        a = 2 * math.pi * (k + 0.5) / 32
        kit.add(stone_block(0.6, 0.32, 0.3, kit.rng, bevel=0.03), polar(7.75, a, z0 - 0.15), "Stone")
    n_post = 72
    for k in range(n_post):                                                                       # 철 난간
        a = 2 * math.pi * k / n_post
        kit.add(box(0.04, 0.04, 1.0, center_z=False), polar(7.95, a, z0 + 0.38), "Iron")
    for zz, rr in ((z0 + 1.38, 0.035), (z0 + 0.9, 0.022)):
        kit.add(torus(7.95, rr, 128, 6), T(0, 0, zz), "Iron", smooth=True)
    for i in range(6):                                                                            # 난간 꽃 상자
        flower_box(kit, polar(7.95, 2 * math.pi * (i + 0.25) / 6, z0 + 1.2), 1.4)
    # 유리 벽 · 놋쇠 창살 · 위 고리
    rg, panes = 6.1, 18
    kit.add(ring_band(5.7, 6.4, z0 + 0.38, z0 + 0.8, 96), mat="Stone")
    for k in range(panes):
        a = 2 * math.pi * k / panes
        chord = 2 * rg * math.sin(math.pi / panes)
        kit.add(box(0.06, chord, 2.5, center_z=False), polar(rg - 0.05, a + math.pi / panes, z0 + 0.8), "Glass")
        kit.add(box(0.16, 0.14, 2.6, center_z=False), polar(rg, a, z0 + 0.8), "Brass")
        for zz in (z0 + 1.7, z0 + 2.6):
            kit.add(box(0.08, chord, 0.05), polar(rg, a + math.pi / panes, zz), "Brass")
    kit.add(ring_band(5.6, 6.55, z0 + 3.35, z0 + 3.62, 96), mat="Brass")
    kit.add(ring_band(5.6, 6.7, z0 + 3.62, z0 + 3.85, 96), mat="Stone")
    return kit.build()


def build_roof():
    kit = Kit("SM_Tower_Roof")
    zb = SHAFT_Z1 + 3.82
    re, apex = 7.45, zb + 6.0
    rows = 14

    def rz(t):
        return re * (1 - t) ** 1.3 + 0.16 * t, zb + (apex - zb) * t

    prof = [(5.8, zb - 0.06), (re, zb - 0.06), (re + 0.04, zb + 0.1)]
    for i in range(rows):
        r0, z0 = rz(i / rows)
        r1, z1 = rz((i + 1) / rows)
        prof += [(r0 + 0.02, z0 + 0.08), (r1 + 0.02, z1 - 0.0)]
    prof += [(0.16, apex), (0.0, apex + 0.05)]
    kit.add(lathe(prof, 96), mat="Slate", smooth=True, uv_scale=1.6)
    kit.add(torus(re + 0.05, 0.05, 96, 8), T(0, 0, zb + 0.02), "Brass", smooth=True)
    # 지붕창 넷(작은 박공) — 원뿔만 있으면 단조롭다
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        t = 0.22
        r, z = rz(t)
        M = polar(r - 0.3, a, z)
        kit.add(box(0.9, 0.9, 1.1, center_z=False, bevel=0.02), M, "Stone")
        window(kit, M @ T(0.45, 0, 0.18), "round", 0.55, mullion=True)
        for side in (-1, 1):
            kit.add(box(1.1, 0.62, 0.05), M @ T(0.1, side * 0.24, 1.32) @ R(0, side * 42, 0), "Slate")
    # 꼭대기 놋쇠 첨탑
    kit.add(cylinder(0.22, 0.6, 16), T(0, 0, apex - 0.1), "Brass", smooth=True)
    kit.add(sphere(0.34, 20, 12), T(0, 0, apex + 0.8), "Brass", smooth=True)
    kit.add(cylinder(0.12, 2.2, 12, radius_top=0.0), T(0, 0, apex + 1.0), "Brass", smooth=True)
    kit.add(sphere(0.12, 12, 8), T(0, 0, apex + 2.1), "Brass", smooth=True)
    return kit.build(), apex


def build_armillary(center_z):
    """첨탑을 두른 놋쇠 혼천의 — 고리 넷 + 가운데 구슬. 마나 빛줄기의 출발점(콘셉트)"""
    kit = Kit("SM_Tower_Armillary")
    rad = 2.8
    for (rx, ry, rz_, minor) in ((0, 0, 0, 0.11), (90, 0, 0, 0.13), (90, 0, 90, 0.10), (23.5, 0, 0, 0.10)):
        kit.add(torus(rad, minor, 96, 10), T(0, 0, center_z) @ R(rz_, rx, ry), "Brass", smooth=True)
    kit.add(torus(rad + 0.25, 0.08, 96, 8, arc=math.pi), T(0, 0, center_z) @ R(0, 90, 0), "Brass", smooth=True)
    for k in range(12):                                                    # 눈금 마디
        a = 2 * math.pi * k / 12
        kit.add(sphere(0.16, 10, 6), T(rad * math.cos(a), rad * math.sin(a), center_z), "Brass", smooth=True)
    kit.add(sphere(0.42, 24, 14), T(0, 0, center_z), "Glass", smooth=True)
    return kit.build()


# ─────────────── 현관(동쪽 증축) · 문 사이 통로 ───────────────

PORCH_X0, PORCH_X1, PORCH_HW = 19.7, 24.6, 3.6
PORTAL_W, PORTAL_SPRING = 3.4, 2.6


def door_leaf(kit, M_hinge, width, spring, sign, open_deg):
    """뾰족 아치 쌍여닫이 한 짝. M_hinge = 경첩 아래, 문짝은 로컬 Y 쪽으로 sign 방향(닫힌 상태), 안쪽(-X)으로 open_deg 열림"""
    full = arch_outline(width * 2, spring, 32, pointed=True)
    right = []
    for x, z in full:          # (w,0) → (w,spring) → 오른쪽 호 → 꼭짓점(x=0) 앞에서 멈춘다
        if x <= 1e-6:
            break
        right.append((x, z))
    pts = [(0.0, 0.0)] + right + [(0.0, arch_height(width * 2, spring))]
    pts = [(width - x, z) for x, z in pts]      # 경첩(바깥 끝) = 0, 가운데(맞닿는 끝) = width
    M = M_hinge @ R(-sign * open_deg) @ R(-90 * sign)
    kit.add(panel_xz(pts, -0.05, 0.05), M, "WoodInk")
    for zz in (0.5, 1.4, 2.3):
        kit.add(box(width * 0.9, 0.115, 0.06), M @ T(width * 0.5, 0, zz), "Brass")
    kit.add(box(0.06, 0.115, 3.6, center_z=False), M @ T(width * 0.86, 0, 0.1), "Brass")
    kit.add(torus(0.13, 0.022, 24, 6), M @ T(width * 0.8, -0.08, 1.15) @ R(0, 90), "Brass", smooth=True)


def build_porch():
    kit = Kit("SM_Tower_Porch")
    rng = kit.rng
    x0, x1, hw, t = PORCH_X0, PORCH_X1, PORCH_HW, 0.6
    wall_h = 5.4
    # 옆벽 둘(X 로 뻗음) — 바깥 면에 창 하나씩
    for side in (-1, 1):
        y = side * (hw - t / 2)
        ashlar_wall(kit, x1 - x0 - t, 0.0, wall_h, t, x0=x0, y=y, openings=[win_opening((x0 + x1) / 2, 1.6, "arch", 1.0, 1.3)])
        window(kit, T((x0 + x1) / 2, side * hw, 1.6) @ R(90 * side), "arch", 1.0, 1.3, depth=0.5)
    # 앞벽(Y 로 뻗음) + 뾰족 아치 문
    portal_top = arch_height(PORTAL_W, PORTAL_SPRING)
    Mfront = T(x1 - t / 2, 0, 0) @ R(90)
    pw_out = PORTAL_W + 0.9
    ashlar_wall(kit, 2 * hw, 0.0, wall_h, t, x0=-hw, openings=[Opening(0.0, 0.0, arch_half(PORTAL_W, PORTAL_SPRING), arch_half(pw_out, PORTAL_SPRING), arch_height(pw_out, PORTAL_SPRING))], M=Mfront)
    outer = resample_star(arch_outline(pw_out, PORTAL_SPRING, 48), (0, PORTAL_SPRING * 0.5), 96)
    inner = resample_star(arch_outline(PORTAL_W, PORTAL_SPRING, 48), (0, PORTAL_SPRING * 0.5), 96)
    kit.add(frame_xz(outer, inner, -t / 2 - 0.02, t / 2 + 0.14), T(x1 - t / 2, 0, 0) @ R(-90), "Stone")
    mid = resample_star(arch_outline(PORTAL_W + 0.3, PORTAL_SPRING, 48), (0, PORTAL_SPRING * 0.5), 96)
    kit.add(frame_xz(mid, inner, t / 2 + 0.14, t / 2 + 0.2), T(x1 - t / 2, 0, 0) @ R(-90), "Brass")
    # 문짝(안쪽으로 열림) — 문 구멍은 늘 열려 있다(게임 길)
    for sign in (-1, 1):
        door_leaf(kit, T(x1 - t - 0.06, sign * PORTAL_W / 2, 0.02), PORTAL_W / 2 - 0.02, PORTAL_SPRING, sign, 72)
    # 앞 모서리 버팀 기둥 + 뾰족 갓 + 놋쇠 구슬
    for side in (-1, 1):
        for k, (z0, z1, wd) in enumerate([(0.0, 2.4, 0.95), (2.4, 4.6, 0.85), (4.6, 6.4, 0.75)]):
            kit.add(stone_block(wd, wd, z1 - z0 - 0.01, rng, bevel=0.035), T(x1 + 0.2, side * (hw - 0.35), (z0 + z1) / 2), "Stone")
        kit.add(prism([(0.4, 0.4), (-0.4, 0.4), (-0.4, -0.4), (0.4, -0.4)], 0, 0.12), T(x1 + 0.2, side * (hw - 0.35), 6.4), "StoneDark")
        kit.add(cylinder(0.32, 0.9, 4, radius_top=0.02), T(x1 + 0.2, side * (hw - 0.35), 6.52) @ R(45), "Slate")
        kit.add(sphere(0.12, 12, 8), T(x1 + 0.2, side * (hw - 0.35), 7.5), "Brass", smooth=True)
    # 박공 지붕(용마루 X 방향)
    ridge_z, eave_z, eave_y = 8.7, 5.2, hw + 0.55
    slope = math.atan2(ridge_z - eave_z, eave_y)
    run = math.hypot(eave_y, ridge_z - eave_z)
    length = x1 - x0 + 0.9
    xc = (x0 + x1) / 2 + 0.25
    for side in (-1, 1):
        rows = 9
        for i in range(rows):
            tmid = (i + 0.5) / rows
            y = side * eave_y * (1 - tmid)
            z = eave_z + (ridge_z - eave_z) * tmid
            kit.add(box(length, run / rows + 0.12, 0.07, bevel=0.01), T(xc, y, z + 0.05) @ R(0, -side * math.degrees(slope) - side * 3), "Slate")
        kit.add(box(length, run, 0.16), T(xc, side * eave_y / 2, (eave_z + ridge_z) / 2 - 0.08) @ R(0, -side * math.degrees(slope)), "Slate")
        kit.add(box(0.1, run + 0.1, 0.18), T(x1 + 0.7, side * eave_y / 2, (eave_z + ridge_z) / 2 + 0.03) @ R(0, -side * math.degrees(slope)), "Brass")
    kit.add(box(length, 0.22, 0.2, bevel=0.03), T(xc, 0, ridge_z + 0.1), "Slate")
    # 박공 벽(앞 · 뒤): 단마다 폭이 줄어드는 마름돌(비탈 끝은 지붕 · 박공널이 덮는다) + 앞 둥근 창
    rose = win_opening(0.0, 6.15, "round", 1.15)
    for xg, ops in ((x1 - t / 2, [rose]), (x0 + 0.2, [])):
        gable_ashlar(kit, T(xg, 0, 0) @ R(90), hw, wall_h, ridge_z - 0.1, 0.56, ops)
    window(kit, T(x1, 0, 6.15), "round", 1.15)
    # 천장 · 바닥
    kit.add(box(x1 - x0 - t, 2 * (hw - t), 0.12), T((x0 + x1 - t) / 2, 0, 5.36), "Wood")      # 문 아치 꼭대기(5.1 m) 위
    for k in range(5):
        kit.add(box(0.2, 2 * (hw - t), 0.22, bevel=0.015), T(x0 + 0.5 + k * (x1 - x0 - 1.4) / 4, 0, 5.2), "Wood")
    flag_floor(kit, x0 - 0.3, x1 + 1.3, -hw + 0.3, hw - 0.3)
    return kit.build()


def gable_ashlar(kit, M, hw, z0, apex, thickness, openings=()):
    """삼각 박공을 마름돌로: 단마다 그 단 꼭대기 높이의 폭만큼(가운데 원점, X 로 뻗음 → M)"""
    from kit_common import _fill, course_heights
    rng = kit.rng
    for c0, c1 in course_heights(rng, z0, apex - 0.25, 0.36, 0.46):
        half = hw * (apex - c1) / (apex - z0) + 0.12
        cuts = sorted((o.at - e, o.at + e) for o in openings for e in [o.edge(c0, c1)] if e is not None)
        segs, cur = [], -half
        for a0, a1 in cuts:
            if a0 - cur > 0.06:
                segs.append((cur, a0))
            cur = max(cur, a1)
        if half - cur > 0.06:
            segs.append((cur, half))
        for sa, sb in segs:
            s_ = sa
            for L in _fill(rng, sb - sa, (0.5, 1.0)):
                kit.add(stone_block(L - 0.012, thickness, (c1 - c0) - 0.012, rng, bevel=0.025), M @ T(s_ + L / 2, 0, (c0 + c1) / 2), "Stone")
                s_ += L


def flag_floor(kit, xa, xb, ya, yb, top=0.05, mat="Flagstone"):
    """판석 바닥(줄마다 길이가 다른 돌)"""
    rng = kit.rng
    y = ya
    while y < yb - 0.05:
        dy = min(rng.uniform(0.45, 0.7), yb - y)
        x = xa - rng.uniform(0, 0.4)
        while x < xb - 0.05:
            dx = min(rng.uniform(0.5, 0.95), xb - x)
            x_lo = max(x, xa)
            if dx - (x_lo - x) > 0.12:
                w = x + dx - x_lo
                kit.add(stone_block(w - 0.015, dy - 0.015, 0.1, rng, bevel=0.018, jitter=0.004), T(x_lo + w / 2, y + dy / 2, top - 0.05 + rng.uniform(-0.006, 0.006)) @ R(rng.uniform(-0.6, 0.6)), mat)
            x += dx
        y += dy


def build_vestibule():
    """기단 문 구멍 → 1층 동쪽 벽 문(X 16 m) 사이 통로: 옆벽 · 천장 · 바닥"""
    kit = Kit("SM_Tower_Vestibule")
    xa, xb = 16.15, 20.25
    for side in (-1, 1):
        ashlar_wall(kit, xb - xa, 0.0, 5.0, 0.4, x0=xa, y=side * 2.4)
    kit.add(box(xb - xa, 4.4, 0.14), T((xa + xb) / 2, 0, 4.93), "Wood")
    for k in range(4):
        kit.add(box(0.2, 4.4, 0.26, bevel=0.015), T(xa + 0.5 + k * 1.0, 0, 4.73), "Wood")
    flag_floor(kit, xa - 0.1, xb + 0.1, -2.2, 2.2)
    return kit.build()


def build_banner(name="SM_Tower_Banner", w=1.0, h=3.0):
    """남색 배너(서클 고리 문양) — 맨 위 놋쇠 막대, 아래 제비 꼬리, 살짝 물결. UV 0~1 = 배너 그림 한 장"""
    import bmesh
    kit = Kit(name)
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    nx, nz = 8, 18
    grid = {}
    for i in range(nx + 1):
        for j in range(nz + 1):
            u, v = i / nx, j / nz
            x = (u - 0.5) * w
            z = -v * h
            if v > 0.86:                               # 제비 꼬리: 가운데가 파인다
                notch = (1 - abs(u - 0.5) * 2) * (v - 0.86) / 0.14 * 0.22 * h
                z += notch
            y = 0.04 * math.sin(u * math.pi * 2.0 + v * 2.2) * (0.3 + v)
            grid[i, j] = (bm.verts.new((x, y, z)), (u, 1 - v))
    for i in range(nx):
        for j in range(nz):
            q = [grid[i, j], grid[i + 1, j], grid[i + 1, j + 1], grid[i, j + 1]]
            f = bm.faces.new([p[0] for p in q])
            for loop, p in zip(f.loops, q):
                loop[uvl].uv = p[1]
    back = bm.copy()
    for f in back.faces:
        f.normal_flip()
    kit.add(bm, T(0, -0.012, 0), "Banner", keep_uv=True, smooth=True)
    kit.add(back, T(0, 0.012, 0), "Banner", keep_uv=True, smooth=True)
    kit.add(cylinder(0.03, w + 0.2, 10), T(-(w + 0.2) / 2, 0, 0.03) @ R(0, 0, 90), "Brass", smooth=True)
    for s in (-1, 1):
        kit.add(sphere(0.055, 10, 6), T(s * (w / 2 + 0.12), 0, 0.03), "Brass", smooth=True)
    return kit.build()


def build_all():
    objs = [build_drum(), build_drum_roof(), build_shaft(), build_observatory()]
    roof, apex = build_roof()
    objs += [roof, build_armillary(apex + 1.0), build_porch(), build_vestibule(), build_banner()]
    return objs
