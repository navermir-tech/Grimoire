"""Tripo 결과(GLB)를 게임용으로 정리한다 — 블렌더를 화면 없이 돌린다.

    "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --factory-startup -P art/tools/blender_cleanup.py -- <입력.glb> <결과 폴더> <이름> <높이cm> [--sink 0.08]

한 일:
- 메시를 하나로 합치고 변환을 적용한다(언리얼에서 회전 · 크기 0 기준).
- 실제 크기: 가장 높은 쪽이 <높이cm> 가 되게 같은 비율로 키운다(Tripo 결과는 크기가 제멋대로다).
- 기준점: 바닥 가운데. --sink 는 높이의 그 비율만큼 땅에 묻는다(바위가 땅 위에 떠 보이지 않게).
- 결과: <이름>.glb(언리얼 가져오기용, 텍스처 포함) · <이름>.blend(사람이 다듬을 원본) · <이름>_preview.png(3/4 미리보기).
"""
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

argv = sys.argv[sys.argv.index("--") + 1:]
if len(argv) < 4:
    sys.exit(__doc__)
SRC, OUT_DIR, NAME, HEIGHT_CM = os.path.abspath(argv[0]), os.path.abspath(argv[1]), argv[2], float(argv[3])  # 블렌더는 상대 경로를 .blend 기준으로 읽는다
SINK = float(argv[argv.index("--sink") + 1]) if "--sink" in argv else 0.0
os.makedirs(OUT_DIR, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=SRC)

meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
if not meshes:
    sys.exit(f"메시가 없다: {SRC}")

# 부모(빈 객체) 변환까지 메시에 굽고 하나로 합친다.
bpy.ops.object.select_all(action="DESELECT")
for obj in meshes:
    obj.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
if len(meshes) > 1:
    bpy.ops.object.join()
obj = bpy.context.view_layer.objects.active
obj.name = NAME
obj.data.name = NAME
for other in [o for o in bpy.context.scene.objects if o != obj]:
    bpy.data.objects.remove(other, do_unlink=True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# 실제 크기 — 블렌더 단위는 m(glTF 와 같다), 언리얼은 가져올 때 cm 로 바꾼다.
corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
size = Vector((max(c[i] for c in corners) - min(c[i] for c in corners) for i in range(3)))
scale = (HEIGHT_CM / 100.0) / max(size.z, 1e-6)
obj.scale = (scale, scale, scale)
bpy.ops.object.transform_apply(scale=True)

# 기준점 = 바닥 가운데, 높이의 SINK 만큼 묻는다.
corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
low = Vector((min(c[i] for c in corners) for i in range(3)))
high = Vector((max(c[i] for c in corners) for i in range(3)))
height = high.z - low.z
offset = Vector(((low.x + high.x) / 2, (low.y + high.y) / 2, low.z + height * SINK))
obj.data.transform(Matrix.Translation(-offset))
obj.location = (0, 0, 0)
final = Vector((high.x - low.x, high.y - low.y, height))
print(f"[cleanup] {NAME}: 크기 {final.x * 100:.0f} × {final.y * 100:.0f} × {final.z * 100:.0f} cm, 면 {len(obj.data.polygons)}, 묻힘 {SINK * 100:.0f}%")

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, NAME + ".blend"))
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT_DIR, NAME + ".glb"), export_format="GLB", use_selection=False)

# 미리보기: 회색 바닥 · 해 · 3/4 카메라 · EEVEE.
scene = bpy.context.scene
bpy.ops.mesh.primitive_plane_add(size=max(final.x, final.y) * 4, location=(0, 0, 0))
ground = bpy.context.active_object
ground_mat = bpy.data.materials.new("Ground")
ground_mat.diffuse_color = (0.35, 0.36, 0.33, 1)
ground.data.materials.append(ground_mat)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 3.0
sun.rotation_euler = (math.radians(50), 0, math.radians(35))
scene.collection.objects.link(sun)
world = bpy.data.worlds.new("World")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[0].default_value = (0.55, 0.62, 0.72, 1)
world.node_tree.nodes["Background"].inputs[1].default_value = 0.8
scene.world = world
radius = max(final.x, final.y, final.z) * 1.9
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
cam.location = (radius * 0.8, -radius * 0.9, final.z * 0.5 + radius * 0.45)
direction = Vector((0, 0, final.z * 0.4)) - cam.location
cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
scene.collection.objects.link(cam)
scene.camera = cam
scene.render.engine = "BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items.keys() else "BLENDER_EEVEE"
scene.render.resolution_x = 1024
scene.render.resolution_y = 768
scene.render.filepath = os.path.join(OUT_DIR, NAME + "_preview.png")
bpy.ops.render.render(write_still=True)
print(f"[cleanup] 저장 {OUT_DIR}/{NAME}.glb · .blend · _preview.png")
