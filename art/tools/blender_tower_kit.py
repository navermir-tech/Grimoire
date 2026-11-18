"""마법탑 모듈 키트를 만든다(블렌더를 화면 없이) — 외관 · 현관 · 실내 · 앞마당 조각을 GLB 로 내보낸다.

    "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --factory-startup -P art/tools/blender_tower_kit.py -- [결과 폴더=art/env/tower-kit] [묶음=exterior,interior,yard] [--no-export] [--preview]

- 결과: <폴더>/SM_*.glb(언리얼 가져오기용, 텍스처 없이 재질 슬롯 이름만) · TowerKit.blend(사람이 다듬을 원본) · preview_*.png
- 같은 시드라 다시 돌려도 같은 모양이다. 수치는 tower_kit/*.py 맨 위.
- 언리얼: Scripts/Tutorial/tower_art_import.py(가져오기 · 재질) → tower_art_dress.py(레벨 배치). 순서는 docs/art/tower-art.md.
"""
import math
import os
import sys

import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "tower_kit"))
sys.dont_write_bytecode = True

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
flags = {a for a in argv if a.startswith("--")}
args = [a for a in argv if not a.startswith("--")]
OUT = os.path.abspath(args[0]) if args else os.path.abspath(os.path.join(HERE, "..", "env", "tower-kit"))
GROUPS = args[1].split(",") if len(args) > 1 else ["exterior", "interior", "yard"]
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)

import kit_common  # noqa: E402

objs = []
if "exterior" in GROUPS:
    import kit_exterior  # noqa: E402
    objs += kit_exterior.build_all()
if "interior" in GROUPS:
    import kit_interior  # noqa: E402
    objs += kit_interior.build_all()
if "yard" in GROUPS:
    import kit_yard  # noqa: E402
    objs += kit_yard.build_all()

if "--no-export" not in flags:
    for obj in objs:
        kit_common.export_glb(obj, os.path.join(OUT, obj.name + ".glb"))
    print(f"[kit] 내보냄 {len(objs)} 개 → {OUT}")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "TowerKit_" + "_".join(GROUPS) + ".blend"))


def render_preview(path, objects, cam_loc, target, lens=35, res=(1280, 800)):
    scene = bpy.context.scene
    for o in bpy.context.scene.objects:
        o.hide_render = o.type == "MESH" and o not in objects
    if "Sun" not in bpy.data.objects:
        sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
        sun.data.energy = 3.5
        sun.rotation_euler = (math.radians(52), 0, math.radians(-35))
        scene.collection.objects.link(sun)
        world = bpy.data.worlds.new("World")
        world.use_nodes = True
        world.node_tree.nodes["Background"].inputs[0].default_value = (0.55, 0.65, 0.8, 1)
        world.node_tree.nodes["Background"].inputs[1].default_value = 0.9
        scene.world = world
        bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -0.01))
        g = bpy.context.active_object
        g.name = "Ground"
        gm = bpy.data.materials.new("GroundMat")
        gm.diffuse_color = (0.32, 0.42, 0.22, 1)
        g.data.materials.append(gm)
    bpy.data.objects["Ground"].hide_render = False
    cam = bpy.data.objects.get("Cam") or bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.data.lens = lens
    cam.data.clip_end = 1000
    cam.location = cam_loc
    cam.rotation_euler = (Vector(target) - Vector(cam_loc)).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    # EEVEE: 재질 슬롯 색을 프린시플드 바탕색으로(워크벤치는 -b 에서 죽는다 — 2026-11-18)
    engines = bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items.keys()
    scene.render.engine = "BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in engines else "BLENDER_EEVEE"
    for mat in bpy.data.materials:
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        if bsdf:
            bsdf.inputs["Base Color"].default_value = mat.diffuse_color
            bsdf.inputs["Roughness"].default_value = 0.35 if mat.name in ("Brass", "Glass") else 0.8
            bsdf.inputs["Metallic"].default_value = 1.0 if mat.name in ("Brass", "Iron") else 0.0
    bpy.data.worlds["World"].node_tree.nodes["Background"].inputs[1].default_value = 0.35
    bpy.data.objects["Sun"].data.energy = 4.5
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print(f"[kit] 미리보기 {path}")


if "--preview" in flags:
    meshes = [o for o in objs]
    if "exterior" in GROUPS:
        render_preview(os.path.join(OUT, "preview_exterior.png"), meshes, (78, -34, 16), (0, 0, 17), lens=32)
        render_preview(os.path.join(OUT, "preview_porch.png"), meshes, (36, -9, 3.5), (21, 0, 4.0), lens=30)
    if "interior" in GROUPS:
        render_preview(os.path.join(OUT, "preview_interior.png"), meshes, (6, -6, 1.8), (-3, 6, 1.4), lens=22)
    if "yard" in GROUPS:
        render_preview(os.path.join(OUT, "preview_yard.png"), meshes, (6, -10, 3.0), (0, 2, 0.6), lens=28)
