# 블렌더에서 정리한 환경 에셋(GLB)을 언리얼로 가져온다 — 아트 방향 「한 에셋이 지나는 길」 5단계(docs/spec/art-direction).
#  - /Game/Grimoire/Environment/<종류>/<이름>/ 아래 StaticMeshes · Materials · Textures
#  - Nanite 켜기(정적 메시, 고품질 모델을 그대로 쓴다)
# 커맨드렛은 인자를 넘기기 어려워 환경 변수로 받는다:
#   $env:GRIM_ENV_GLB="C:\...\rock-meadow-a.glb"; $env:GRIM_ENV_DEST="Rocks/SM_Rock_MeadowA"
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Environment\import_env_asset.py" -unattended -nop4 -nosplash -nullrhi
import os

import unreal

SRC = os.environ.get("GRIM_ENV_GLB", "")
DEST = os.environ.get("GRIM_ENV_DEST", "")
if not SRC or not os.path.exists(SRC) or "/" not in DEST:
    raise RuntimeError("GRIM_ENV_GLB(있는 파일) · GRIM_ENV_DEST(<종류>/<이름>) 를 준다")
category, name = DEST.rsplit("/", 1)
folder = f"/Game/Grimoire/Environment/{category}"  # 언리얼(Interchange)이 그 아래 <이름>/StaticMeshes · Materials · Textures 를 만든다

task = unreal.AssetImportTask()
task.set_editor_property("filename", SRC)
task.set_editor_property("destination_path", folder)
task.set_editor_property("destination_name", name)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

imported = [unreal.load_asset(p) for p in task.get_editor_property("imported_object_paths")]
meshes = [a for a in imported if isinstance(a, unreal.StaticMesh)]
if not meshes:
    raise RuntimeError(f"정적 메시가 안 나왔다: {[str(p) for p in task.get_editor_property('imported_object_paths')]}")
for mesh in meshes:
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", settings)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
    box = mesh.get_bounding_box()
    size = box.max - box.min
    unreal.log(f"import_env_asset: {mesh.get_path_name()} 크기 {size.x:.0f}×{size.y:.0f}×{size.z:.0f} cm, Nanite 켬")
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
