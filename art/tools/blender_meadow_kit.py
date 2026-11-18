# 초원 꾸밈 1차 묶음(docs/spec/meadow-dressing.md 「5」) 자체 메시 — 블렌더 배경 실행으로 FBX 를 만든다.
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup -P art/tools/blender_meadow_kit.py -- <출력 폴더>
# 출력: SM_Meadow_*.fbx(1 m = 100 언리얼 단위, 원점 = 바닥 가운데). 언리얼로는 Scripts/Dressing/make_meadow_assets.py 가 들여온다.
# 메시마다 재질 칸 하나(언리얼에서 우리 재질 인스턴스로 바꾼다). 모양은 회화풍 낮은 폴리(art-direction) — 시드 고정이라 다시 돌려도 같다.
import math
import os
import random
import sys

import bpy
import bmesh
from mathutils import Matrix, Vector

OUT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else os.path.join(os.getcwd(), "Saved", "MeadowKit")
os.makedirs(OUT, exist_ok=True)
rng = random.Random(20261118)


def clear():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    for block in (bpy.data.meshes, bpy.data.materials):
        for item in list(block):
            block.remove(item)


def material(name):
    return bpy.data.materials.get(name) or bpy.data.materials.new(name)


def finish(parts, name, slot, smooth=False):
    """parts 를 하나로 합치고 UV(스마트) · 재질 칸 하나 · 원점 = 바닥 가운데 → FBX"""
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    if len(parts) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    obj.data.materials.clear()
    obj.data.materials.append(material(slot))
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    for poly in obj.data.polygons:
        poly.use_smooth = smooth
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    path = os.path.join(OUT, name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={"MESH"}, apply_scale_options="FBX_SCALE_UNITS",
                             mesh_smooth_type="FACE", use_mesh_modifiers=True, add_leaf_bones=False, bake_anim=False)
    print(f"meadow_kit: {path} tris={sum(len(p.vertices) - 2 for p in obj.data.polygons)}")
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)


def cyl(r, depth, loc, rot=(0, 0, 0), verts=8, r2=None):
    if r2 is None:
        bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=r, depth=depth, location=loc, rotation=rot)
    else:
        bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r, radius2=r2, depth=depth, location=loc, rotation=rot)
    return bpy.context.active_object


def box(sx, sy, sz, loc, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc, rotation=rot)
    o = bpy.context.active_object
    o.scale = (sx, sy, sz)
    return o


def ico(r, loc, sub=1, squash=(1, 1, 1)):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub, radius=r, location=loc)
    o = bpy.context.active_object
    o.scale = squash
    return o


def jitter(obj, amount, seed):
    """꼭짓점을 조금씩 흔들어 손으로 깎은 느낌"""
    r = random.Random(seed)
    for v in obj.data.vertices:
        v.co += Vector((r.uniform(-amount, amount), r.uniform(-amount, amount), r.uniform(-amount, amount)))
    return obj


def segment(a, b, r1, r2, verts=6):
    """a → b 원뿔대"""
    a, b = Vector(a), Vector(b)
    d = b - a
    o = cyl(r1, d.length, (a + b) / 2, verts=verts, r2=r2)
    o.rotation_mode = "QUATERNION"
    o.rotation_quaternion = d.to_track_quat("Z", "Y")
    return o


clear()

# ── D04 돌 등잔: 정강이 높이(≈ 52 cm) 팔각 받침 · 기둥 · 네 기둥 불집 · 지붕 · 꼭지. 불꽃은 액터가 불집 안(≈ 33 cm)에 ──
parts = [
    jitter(cyl(0.15, 0.06, (0, 0, 0.03), verts=8), 0.006, 1),
    jitter(cyl(0.11, 0.03, (0, 0, 0.075), verts=8), 0.004, 2),
    jitter(cyl(0.06, 0.16, (0, 0, 0.17), verts=8, r2=0.05), 0.004, 3),
    jitter(box(0.20, 0.20, 0.035, (0, 0, 0.265)), 0.005, 4),
]
for i, (x, y) in enumerate([(-0.07, -0.07), (0.07, -0.07), (-0.07, 0.07), (0.07, 0.07)]):
    parts.append(jitter(box(0.035, 0.035, 0.13, (x, y, 0.345)), 0.003, 10 + i))
parts.append(jitter(box(0.22, 0.22, 0.03, (0, 0, 0.425)), 0.005, 20))
roof = cyl(0.19, 0.09, (0, 0, 0.485), verts=4, r2=0.03)
roof.rotation_euler = (0, 0, math.radians(45))
parts.append(jitter(roof, 0.004, 21))
parts.append(ico(0.028, (0, 0, 0.545), sub=1))
finish(parts, "SM_Meadow_StoneLamp", "Stone")

# ── D01 바람솜꽃: 무릎 높이 줄기 3~7 포기(줄기 · 잎) + 꼭대기 솜(따로 — 날리면 솜만 사라진다) ──
stems, puffs = [], []
for i in range(6):
    a = i * 1.05 + rng.uniform(-0.3, 0.3)
    base = Vector((math.cos(a) * rng.uniform(0.02, 0.12), math.sin(a) * rng.uniform(0.02, 0.12), 0))
    h = rng.uniform(0.38, 0.56)
    lean = Vector((math.cos(a) * rng.uniform(0.03, 0.09), math.sin(a) * rng.uniform(0.03, 0.09), 0))
    mid = base + lean * 0.4 + Vector((0, 0, h * 0.5))
    top = base + lean + Vector((0, 0, h))
    stems.append(segment(base, mid, 0.006, 0.005, verts=5))
    stems.append(segment(mid, top, 0.005, 0.0035, verts=5))
    for k in range(2):
        leaf_at = base + lean * 0.2 + Vector((0, 0, h * rng.uniform(0.12, 0.35)))
        la = a + rng.uniform(-1.2, 1.2)
        tip = leaf_at + Vector((math.cos(la) * 0.09, math.sin(la) * 0.09, 0.05))
        leaf = segment(leaf_at, tip, 0.012, 0.001, verts=3)
        leaf.scale = (1.0, 0.25, 1.0)
        stems.append(leaf)
    # 솜: 겹친 공 셋(보송한 덩이)
    for k in range(3):
        off = Vector((rng.uniform(-0.012, 0.012), rng.uniform(-0.012, 0.012), rng.uniform(0.0, 0.018)))
        puffs.append(jitter(ico(rng.uniform(0.022, 0.03), top + off, sub=2), 0.004, 100 + i * 3 + k))
finish(stems, "SM_Meadow_CottonStems", "Stem")
finish(puffs, "SM_Meadow_CottonPuffs", "Puff", smooth=True)

# ── D10 곁가지 셋: 굽은 가지 + 잔가지 1~2(길이 55~90 cm) ──
for n in range(3):
    parts = []
    length = rng.uniform(0.55, 0.9)
    r = rng.uniform(0.014, 0.024)
    pts = []
    heading = rng.uniform(0, math.tau)
    for k in range(5):
        t = k / 4
        bend = math.sin(t * math.pi) * rng.uniform(0.03, 0.07)
        pts.append(Vector((t * length - length / 2, bend * (1 if n % 2 else -1), r)))
    for k in range(4):
        parts.append(segment(pts[k], pts[k + 1], r * (1 - 0.15 * k), r * (1 - 0.15 * (k + 1)), verts=6))
    for f in range(1 + n % 2):
        at = pts[1 + f * 2]
        side = 1 if f % 2 == 0 else -1
        tip = at + Vector((rng.uniform(0.08, 0.16), side * rng.uniform(0.1, 0.2), rng.uniform(0.0, 0.02)))
        parts.append(segment(at, tip, r * 0.55, r * 0.25, verts=5))
    finish(parts, f"SM_Meadow_Twig{n + 1}", "Wood")

# ── D02 다 탄 자국: 지름 ≈ 1.1 m 낮은 재 더미 + 숯 조각 ──
bm = bmesh.new()
ring = 18
center = bm.verts.new((0, 0, 0.025))
outer = []
for i in range(ring):
    a = i / ring * math.tau
    rr = 0.55 * (0.8 + 0.25 * math.sin(a * 3 + 0.7) + rng.uniform(-0.05, 0.05))
    outer.append(bm.verts.new((math.cos(a) * rr, math.sin(a) * rr, 0.003)))
mid = []
for i in range(ring):
    a = (i + 0.5) / ring * math.tau
    rr = 0.3 * (0.85 + rng.uniform(-0.1, 0.1))
    mid.append(bm.verts.new((math.cos(a) * rr, math.sin(a) * rr, 0.018)))
for i in range(ring):
    j = (i + 1) % ring
    bm.faces.new((center, mid[i], mid[j]))
    bm.faces.new((mid[i], outer[j], mid[j]))
    bm.faces.new((mid[i], outer[i], outer[j]))
me = bpy.data.meshes.new("ash")
bm.to_mesh(me)
bm.free()
ash = bpy.data.objects.new("ash", me)
bpy.context.collection.objects.link(ash)
parts = [ash]
for i in range(7):
    a = rng.uniform(0, math.tau)
    rr = rng.uniform(0.05, 0.35)
    parts.append(jitter(box(rng.uniform(0.03, 0.07), rng.uniform(0.02, 0.04), 0.02, (math.cos(a) * rr, math.sin(a) * rr, 0.02),
                            rot=(0, 0, rng.uniform(0, 3))), 0.004, 200 + i))
finish(parts, "SM_Meadow_AshPatch", "Ash")

# ── D20 모닥불 장작: 돌 고리(팩 SM_CampfireStone) 안에 걸쳐 놓은 그을린 통나무 넷 + 숯 ──
parts = []
for i in range(4):
    a = i / 4 * math.tau + rng.uniform(-0.2, 0.2)
    out = Vector((math.cos(a) * 0.34, math.sin(a) * 0.34, 0.04))
    inner = Vector((math.cos(a) * 0.04, math.sin(a) * 0.04, 0.2 + rng.uniform(-0.02, 0.03)))
    parts.append(jitter(segment(out, inner, 0.045, 0.035, verts=7), 0.004, 300 + i))
for i in range(6):
    a = rng.uniform(0, math.tau)
    rr = rng.uniform(0.0, 0.16)
    parts.append(jitter(ico(rng.uniform(0.025, 0.04), (math.cos(a) * rr, math.sin(a) * rr, 0.02), sub=1, squash=(1.2, 1, 0.6)), 0.006, 320 + i))
finish(parts, "SM_Meadow_FireLogs", "Char")

print("meadow_kit: done")
