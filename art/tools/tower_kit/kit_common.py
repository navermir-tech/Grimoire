"""마법탑 모듈 키트(블렌더) 공용 도우미 — 조각을 작은 bmesh 로 만들어 변환한 뒤 큰 메시 하나로 모은다.

단위는 m(언리얼은 가져올 때 cm 로 바꾼다), Z 위. 탑 기준 좌표: +X = 동쪽(탑 문 · 현관), 탑 가운데가 원점, 땅 = 0.

- 재질 슬롯 이름이 언리얼 재질 인스턴스를 고른다(Stone → MI_GrimEnv_Stone …, Scripts/Tutorial/tower_art_import.py).
- UV: 면 방향으로 상자 투영, 1 UV = UV_M m(모든 조각이 같은 텍셀 밀도). 조각마다 UV 를 무작위로 어긋나게 해 돌마다 무늬가 다르다.
- 정점 색: R = 조각마다 무작위 밝기 변이(마스터 재질 「VariationAmount」), G = 조각 안 높이 0~1(아래 때 · 이끼), B = 0.
  잎 · 꽃 · 천처럼 색을 정점에 직접 칠하는 재질(Leaf · Cloth)은 RGB 가 색 그 자체다(재질 「UseVertexColor」).
"""
import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

UV_M = 2.0
MATERIALS = ["Stone", "StoneDark", "Flagstone", "Slate", "Wood", "WoodInk", "Brass", "Iron", "Glass", "Cloth", "Banner", "Rug", "Leaf", "Soil", "Rune"]
DIRECT_COLOR = {"Leaf", "Cloth"}


def ensure_materials():
    for name in MATERIALS:
        if name not in bpy.data.materials:
            m = bpy.data.materials.new(name)
            m.diffuse_color = {
                "Stone": (0.78, 0.70, 0.56, 1), "StoneDark": (0.55, 0.52, 0.47, 1), "Flagstone": (0.62, 0.60, 0.55, 1),
                "Slate": (0.12, 0.15, 0.32, 1), "Wood": (0.45, 0.32, 0.20, 1), "WoodInk": (0.10, 0.12, 0.22, 1),
                "Brass": (0.85, 0.65, 0.30, 1), "Iron": (0.15, 0.15, 0.16, 1), "Glass": (0.20, 0.24, 0.30, 1),
                "Cloth": (0.6, 0.6, 0.6, 1), "Banner": (0.12, 0.15, 0.40, 1), "Rug": (0.15, 0.17, 0.38, 1),
                "Leaf": (0.25, 0.45, 0.20, 1), "Soil": (0.22, 0.16, 0.11, 1), "Rune": (0.85, 0.65, 0.30, 1),
            }[name]


# ─────────────── 작은 조각(임시 bmesh) 만들기 ───────────────

def box(sx, sy, sz, bevel=0.0, segments=2, center_z=True):
    """크기 sx · sy · sz(m) 상자. bevel > 0 이면 모서리를 깎는다. center_z=False 면 바닥이 z=0"""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector((sx, sy, sz)), verts=bm.verts)
    if not center_z:
        bmesh.ops.translate(bm, vec=Vector((0, 0, sz / 2)), verts=bm.verts)
    if bevel > 0:
        bevel = min(bevel, min(sx, sy, sz) * 0.45)
        bmesh.ops.bevel(bm, geom=list(bm.edges) + list(bm.verts), offset=bevel, offset_type="OFFSET", segments=segments, profile=0.5, affect="EDGES", clamp_overlap=True)
    return bm


def stone_block(sx, sy, sz, rng, bevel=0.03, jitter=0.012):
    """마름돌 하나: 깎인 모서리 + 면이 살짝 울퉁불퉁(정점을 조금 흔든다)"""
    bm = box(sx, sy, sz, bevel=bevel, segments=2)
    for v in bm.verts:
        v.co += Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))) * jitter
    return bm


def cylinder(radius, depth, segments=24, radius_top=None, cap=True, center_z=False):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=cap, cap_tris=False, segments=segments, radius1=radius, radius2=radius if radius_top is None else radius_top, depth=depth)
    if not center_z:
        bmesh.ops.translate(bm, vec=Vector((0, 0, depth / 2)), verts=bm.verts)
    return bm


def sphere(radius, segments=16, rings=10):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius)
    return bm


def ico(radius, subdiv=2):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=radius)
    return bm


def torus(major, minor, segments=48, sides=10, arc=2 * math.pi):
    """XY 평면의 고리(가운데 원점). arc < 2π 면 끊긴 고리"""
    bm = bmesh.new()
    closed = arc >= 2 * math.pi - 1e-6
    n = segments if closed else segments + 1
    rings = []
    for i in range(n):
        a = arc * i / segments
        c, s = math.cos(a), math.sin(a)
        ring = []
        for j in range(sides):
            b = 2 * math.pi * j / sides
            r = major + minor * math.cos(b)
            ring.append(bm.verts.new((r * c, r * s, minor * math.sin(b))))
        rings.append(ring)
    for i in range(n if closed else n - 1):
        r0, r1 = rings[i], rings[(i + 1) % n]
        for j in range(sides):
            bm.faces.new((r0[j], r0[(j + 1) % sides], r1[(j + 1) % sides], r1[j]))
    return bm


def lathe(profile, segments=48, arc=2 * math.pi, start=0.0, cap_axis=True):
    """(r, z) 점 목록을 Z 축 둘레로 돌린 면(지붕 · 몰딩 · 그릇). r=0 인 점은 축 위 한 점으로"""
    bm = bmesh.new()
    closed = arc >= 2 * math.pi - 1e-6
    n = segments if closed else segments + 1
    cols = []
    for i in range(n):
        a = start + arc * i / segments
        c, s = math.cos(a), math.sin(a)
        cols.append([bm.verts.new((r * c, r * s, z)) for r, z in profile])
    for i in range(n if closed else n - 1):
        a, b = cols[i], cols[(i + 1) % n]
        for j in range(len(profile) - 1):
            quad = [a[j], b[j], b[j + 1], a[j + 1]]
            uniq = []
            for v in quad:
                if all((v.co - u.co).length > 1e-6 for u in uniq):
                    uniq.append(v)
            if len(uniq) >= 3:
                try:
                    bm.faces.new(uniq)
                except ValueError:
                    pass
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    return bm


def ring_band(r_in, r_out, z0, z1, segments=64, arc=2 * math.pi, start=0.0):
    """두께 있는 고리 띠(단면 네모) — 바닥 고리 · 처마 띠 · 상감"""
    return lathe([(r_in, z0), (r_out, z0), (r_out, z1), (r_in, z1), (r_in, z0)], segments, arc, start)


def prism(points, z0, z1):
    """XY 다각형(반시계)을 z0 ~ z1 로 세운 기둥(뚜껑 있음)"""
    bm = bmesh.new()
    lo = [bm.verts.new((x, y, z0)) for x, y in points]
    hi = [bm.verts.new((x, y, z1)) for x, y in points]
    bm.faces.new(list(reversed(lo)))
    bm.faces.new(hi)
    n = len(points)
    for i in range(n):
        bm.faces.new((lo[i], lo[(i + 1) % n], hi[(i + 1) % n], hi[i]))
    return bm


def frame_xz(outer, inner, y0, y1):
    """XZ 평면의 테(바깥 고리 · 안 고리 점 수가 같아야 한다)를 y0 ~ y1 두께로. 창 · 문 둘레 돌테"""
    bm = bmesh.new()
    n = len(outer)
    rows = {}
    for key, pts in (("o", outer), ("i", inner)):
        for side, y in (("f", y0), ("b", y1)):
            rows[key + side] = [bm.verts.new((x, y, z)) for x, z in pts]
    for i in range(n):
        j = (i + 1) % n
        for a, b in (("of", "if"), ("ib", "ob"), ("ob", "of"), ("if", "ib")):
            try:
                bm.faces.new((rows[a][i], rows[a][j], rows[b][j], rows[b][i]))
            except ValueError:
                pass
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def panel_xz(pts, y0, y1):
    """XZ 평면 다각형 판(유리 · 문짝)을 y0 ~ y1 두께로"""
    bm = bmesh.new()
    f = [bm.verts.new((x, y0, z)) for x, z in pts]
    b = [bm.verts.new((x, y1, z)) for x, z in pts]
    bm.faces.new(f)
    bm.faces.new(list(reversed(b)))
    n = len(pts)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((f[j], f[i], b[i], b[j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


# ─────────────── 모양 윤곽 ───────────────

def arch_outline(width, spring, n=24, pointed=True):
    """아래 가운데가 (0, 0) 인 아치 창 · 문 윤곽(XZ, 반시계): 옆 기둥 spring 높이까지 + 위 아치(뾰족 / 반원)"""
    w = width / 2
    pts = [(w, 0.0), (w, spring)]
    if pointed:
        rad = width * 0.8                       # 두 원의 반지름 — 뾰족 아치(고딕)
        cx_r, cx_l = w - rad, -w + rad
        apex_a = math.acos((0 - cx_r) / rad)    # 오른쪽 호(중심 cx_r)가 x=0 에 닿는 각
        for i in range(1, n // 2 + 1):
            a = apex_a * i / (n // 2)
            pts.append((cx_r + rad * math.cos(a), spring + rad * math.sin(a)))
        for i in range(n // 2 - 1, -1, -1):
            a = apex_a * i / (n // 2)
            pts.append((cx_l - rad * math.cos(a), spring + rad * math.sin(a)))
    else:
        for i in range(1, n):
            a = math.pi * i / n
            pts.append((w * math.cos(a), spring + w * math.sin(a)))
    pts += [(-w, spring), (-w, 0.0)]
    return pts


def arch_height(width, spring, pointed=True):
    if pointed:
        rad = width * 0.8
        return spring + math.sqrt(rad * rad - (rad - width / 2) ** 2)
    return spring + width / 2


def resample_star(pts, center, n):
    """닫힌 윤곽을 center 에서 쏜 n 개 각도의 교점으로 다시 뽑는다(테 만들기 — 바깥 · 안 점 수 맞추기)"""
    out = []
    cx, cz = center
    for i in range(n):
        a = 2 * math.pi * i / n
        d = (math.cos(a), math.sin(a))
        best = None
        for k in range(len(pts)):
            p, q = pts[k], pts[(k + 1) % len(pts)]
            ex, ez = q[0] - p[0], q[1] - p[1]
            den = d[0] * ez - d[1] * ex
            if abs(den) < 1e-9:
                continue
            t = ((p[0] - cx) * ez - (p[1] - cz) * ex) / den
            u = ((p[0] - cx) * d[1] - (p[1] - cz) * d[0]) / den
            if t > 0 and -1e-6 <= u <= 1 + 1e-6 and (best is None or t < best):
                best = t
        out.append((cx + d[0] * best, cz + d[1] * best))
    return out


def rect_outline(w, h, z0=0.0):
    return [(w / 2, z0), (w / 2, z0 + h), (-w / 2, z0 + h), (-w / 2, z0)]


def circle_outline(r, cz, n=32):
    return [(r * math.cos(2 * math.pi * i / n), cz + r * math.sin(2 * math.pi * i / n)) for i in range(n)]


# ─────────────── 큰 메시(키트 조각 하나) ───────────────

class Kit:
    """조각들을 모아 메시 오브젝트 하나로 만든다"""

    def __init__(self, name, seed=None):
        self.name = name
        self.rng = random.Random(seed if seed is not None else name)
        self.verts = []
        self.faces = []      # (꼭짓점 번호들, 재질, 매끈함, 조각 번호)
        self.loop_uv = []
        self.loop_col = []
        self.piece = 0

    def add(self, bm, matrix=Matrix(), mat="Stone", smooth=False, variation=None, color=None, uv_offset=None, uv_scale=1.0, keep_uv=False):
        """작은 bmesh 를 matrix 로 옮겨 더한다. color = (r, g, b) 면 정점 색을 그 색으로(Leaf · Cloth)"""
        self.piece += 1
        rng = self.rng
        var = rng.random() if variation is None else variation
        off = uv_offset if uv_offset is not None else (rng.random() * 7.0, rng.random() * 7.0)
        uv_layer = bm.loops.layers.uv.active if keep_uv else None
        bm.verts.index_update()          # verts.new 로 만든 조각(torus · prism)은 번호가 갱신되지 않은 채다
        bm.verts.ensure_lookup_table()
        base = len(self.verts)
        world = [matrix @ v.co for v in bm.verts]
        zs = [p.z for p in world]
        z_lo, z_hi = min(zs), max(zs)
        span = max(z_hi - z_lo, 1e-4)
        self.verts.extend(world)
        normal_matrix = matrix.to_3x3().inverted_safe().transposed()
        for f in bm.faces:
            # 축 위 점을 합친 면(lathe)은 같은 꼭짓점을 두 번 든다 — 겹친 고리를 빼고, 꼭짓점이 셋보다 적으면 버린다(블렌더 「Mesh is not valid」)
            loops, seen = [], set()
            for loop in f.loops:
                if loop.vert.index not in seen:
                    seen.add(loop.vert.index)
                    loops.append(loop)
            if len(loops) < 3:
                continue
            idx = [base + loop.vert.index for loop in loops]
            n = (normal_matrix @ f.normal).normalized()
            ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
            uvs, cols = [], []
            for loop in loops:
                p = world[loop.vert.index]
                if uv_layer is not None:
                    u, v = loop[uv_layer].uv
                elif az >= ax and az >= ay:
                    u, v = p.x / UV_M, p.y / UV_M
                elif ax >= ay:
                    u, v = p.y / UV_M, p.z / UV_M
                else:
                    u, v = p.x / UV_M, p.z / UV_M
                if uv_layer is None:
                    u, v = u * uv_scale + off[0], v * uv_scale + off[1]
                uvs.append((u, v))
                if color is not None:
                    cols.append((color[0], color[1], color[2], 1.0))
                else:
                    cols.append((var, (p.z - z_lo) / span, 0.0, 1.0))
            self.faces.append((idx, mat, smooth, self.piece))
            self.loop_uv.append(uvs)
            self.loop_col.append(cols)
        bm.free()
        return self

    def build(self, smooth_angle=40.0):
        """메시 오브젝트를 만든다(장면에 링크)"""
        ensure_materials()
        mesh = bpy.data.meshes.new(self.name)
        mesh.from_pydata([tuple(v) for v in self.verts], [], [f[0] for f in self.faces])
        mesh.update()
        mats = sorted({f[1] for f in self.faces}, key=MATERIALS.index)
        for m in mats:
            mesh.materials.append(bpy.data.materials[m])
        index = {m: i for i, m in enumerate(mats)}
        uv = mesh.uv_layers.new(name="UVMap")
        col = mesh.color_attributes.new(name="Col", type="FLOAT_COLOR", domain="CORNER")
        li = 0
        for poly, (_, mat, smooth, _) in zip(mesh.polygons, self.faces):
            poly.material_index = index[mat]
            poly.use_smooth = True
        for pi, poly in enumerate(mesh.polygons):
            for k, loop_index in enumerate(poly.loop_indices):
                uv.data[loop_index].uv = self.loop_uv[pi][k]
                col.data[loop_index].color = self.loop_col[pi][k]
        mesh.color_attributes.active_color = col
        try:
            mesh.set_sharp_from_angle(angle=math.radians(smooth_angle))
        except AttributeError:
            pass
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        print(f"[kit] {self.name}: 조각 {self.piece}, 면 {len(mesh.polygons)}, 재질 {mats}")
        return obj


def T(x=0.0, y=0.0, z=0.0):
    return Matrix.Translation(Vector((x, y, z)))


def R(z_deg=0.0, x_deg=0.0, y_deg=0.0):
    return Matrix.Rotation(math.radians(z_deg), 4, "Z") @ Matrix.Rotation(math.radians(y_deg), 4, "Y") @ Matrix.Rotation(math.radians(x_deg), 4, "X")


def S(x, y=None, z=None):
    y = x if y is None else y
    z = x if z is None else z
    return Matrix.Diagonal(Vector((x, y, z, 1.0)))


def polar(r, a_rad, z=0.0):
    """원통 위 한 점의 변환: 원점에서 각 a 방향으로 r, 로컬 +X 가 바깥(법선), +Y 가 둘레 방향"""
    return T(r * math.cos(a_rad), r * math.sin(a_rad), z) @ Matrix.Rotation(a_rad, 4, "Z")


# ─────────────── 돌 쌓기 ───────────────

def course_heights(rng, z0, z1, lo=0.40, hi=0.52):
    """z0 ~ z1 를 높이 lo ~ hi 인 단으로 나눈다(마지막 단은 남은 만큼)"""
    zs = [z0]
    while z1 - zs[-1] > hi * 1.4:
        zs.append(zs[-1] + rng.uniform(lo, hi))
    zs.append(z1)
    return list(zip(zs[:-1], zs[1:]))


class Opening:
    """벽 구멍 하나. at = 가운데(원통은 각 rad, 곧은 벽은 x m), base = 구멍 바닥 높이(m).
    inner(h) · outer(h) = 바닥에서 h 높이의 안(유리 · 길) · 바깥(테) 반폭(m), 그 높이에 없으면 None.
    돌은 구멍 가장자리에서 줄을 끊고(테 아래로 조금 들어가게) 다시 쌓는다 — 네모 구멍 · 틈이 생기지 않는다."""

    def __init__(self, at, base, inner, outer, height):
        self.at, self.base, self.inner, self.outer, self.height = at, base, inner, outer, height

    def edge(self, c0, c1):
        """c0 ~ c1 단에서 돌이 멈출 반폭(m) — 이 단에 구멍이 없으면 None"""
        hs = [c0 + 0.02, (c0 + c1) / 2, c1 - 0.02]
        rel = [z - self.base for z in hs]
        outer = [self.outer(h) for h in rel if 0 <= h <= self.height]
        outer = [o for o in outer if o is not None]
        if not outer:
            return None
        inner = [self.inner(h) for h in rel if 0 <= h <= self.height]
        inner = [i for i in inner if i is not None]
        if not inner:
            return None
        return min(max(inner) + 0.03, max(outer) - 0.02)


def arch_half(width, spring, pointed=True):
    """아치 윤곽의 반폭 함수 h → 반폭(바닥 h=0)"""
    w = width / 2
    rad = width * 0.8

    def f(h):
        if h < 0:
            return None
        if h <= spring:
            return w
        hh = h - spring
        if pointed:
            if hh >= rad:
                return None
            v = (w - rad) + math.sqrt(rad * rad - hh * hh)
        else:
            if hh >= w:
                return None
            v = math.sqrt(w * w - hh * hh)
        return v if v > 0 else None
    return f


def round_half(d):
    r = d / 2

    def f(h):
        if h < 0 or h > d:
            return None
        return math.sqrt(max(r * r - (h - r) ** 2, 0.0))
    return f


def rect_half(w, height):
    return lambda h: w / 2 if 0 <= h <= height else None


def _fill(rng, length, block):
    """length 를 block 범위 길이 돌로 나눈다(합이 정확히 length)"""
    if length < block[0] * 0.6:
        return [length]
    parts = []
    while sum(parts) < length - 1e-6:
        parts.append(rng.uniform(*block))
    if len(parts) > 1 and sum(parts) - length > parts[-1] * 0.5:
        parts.pop()
    scale = length / sum(parts)
    return [p * scale for p in parts]


def ashlar_ring(kit, radius_at, z0, z1, depth, openings=(), mat="Stone", block=(0.8, 1.4), course=(0.40, 0.52), bevel=0.03, gap=0.012):
    """원통 벽을 마름돌로 쌓는다. radius_at(z) = 그 높이의 바깥 반지름. openings = [Opening(at=각)]"""
    rng = kit.rng
    count = 0
    for c0, c1 in course_heights(rng, z0, z1, *course):
        zc = (c0 + c1) / 2
        r = radius_at(zc)
        circ = 2 * math.pi * r
        cuts = []
        for o in openings:
            e = o.edge(c0, c1)
            if e is not None:
                s_mid = (o.at % (2 * math.pi)) * r
                cuts.append((s_mid - e, s_mid + e))
        if not cuts:
            start = rng.uniform(0, 1.5)
            segs = [(start, start + circ)]
        else:
            cuts.sort()
            segs = []
            for k, (a0, a1) in enumerate(cuts):
                n0 = cuts[(k + 1) % len(cuts)][0] + (circ if k == len(cuts) - 1 else 0.0)
                if n0 - a1 > 0.06:
                    segs.append((a1, n0))
        for sa, sb in segs:
            s = sa
            for L in _fill(rng, sb - sa, block):
                mid = (s + L / 2) / r
                s += L
                chord = 2 * (r - depth / 2) * math.sin(L / (2 * r))
                bm = stone_block(depth, max(chord - gap, 0.05), (c1 - c0) - gap, rng, bevel=min(bevel, chord * 0.3))
                kit.add(bm, polar(r - depth / 2, mid, zc), mat)
                count += 1
    return count


def ashlar_wall(kit, length, z0, z1, thickness, x0=0.0, openings=(), mat="Stone", block=(0.7, 1.3), course=(0.40, 0.52), bevel=0.028, gap=0.012, y=0.0, M=None):
    """곧은 벽(X 방향으로 뻗음, 두께 Y). openings = [Opening(at=x)]. M = 벽 전체를 옮길 변환"""
    rng = kit.rng
    M = Matrix() if M is None else M
    for c0, c1 in course_heights(rng, z0, z1, *course):
        cuts = sorted((o.at - e, o.at + e) for o in openings for e in [o.edge(c0, c1)] if e is not None)
        segs, cur = [], x0
        for a0, a1 in cuts:
            if a0 - cur > 0.06:
                segs.append((cur, min(a0, x0 + length)))
            cur = max(cur, a1)
        if x0 + length - cur > 0.06:
            segs.append((cur, x0 + length))
        for k, (sa, sb) in enumerate(segs):
            parts = _fill(rng, sb - sa, block)
            if k == 0 and len(parts) > 1 and rng.random() < 0.5:   # 단마다 이음매가 엇갈리게
                parts.reverse()
            s = sa
            for L in parts:
                bm = stone_block(L - gap, thickness, (c1 - c0) - gap, rng, bevel=min(bevel, L * 0.3))
                kit.add(bm, M @ T(s + L / 2, y, (c0 + c1) / 2), mat)
                s += L


def export_glb(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    kwargs = dict(filepath=path, export_format="GLB", use_selection=True, export_apply=True, export_yup=True, export_texcoords=True, export_normals=True, export_materials="EXPORT")
    try:
        bpy.ops.export_scene.gltf(**kwargs, export_vertex_color="ACTIVE", export_all_vertex_colors=False)
    except TypeError:
        try:
            bpy.ops.export_scene.gltf(**kwargs, export_vertex_color="ACTIVE")
        except TypeError:
            bpy.ops.export_scene.gltf(**kwargs, export_colors=True)
