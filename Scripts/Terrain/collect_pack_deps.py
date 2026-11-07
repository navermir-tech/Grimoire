# 우리 맵 · 에셋이 쓰는 마켓 · Fab 팩 파일만 모은다(docs/dev/market-assets.md 「쓰는 에셋만 올리기」).
# 시작점: Content/Grimoire 의 모든 에셋 + 우리 맵의 외부 액터 · 외부 오브젝트. 에셋 레지스트리의 패키지 의존(단단한 것 + 부드러운 것)을
# 끝까지 따라가, 팩 폴더(Content/<팩>/ — 저장소가 기본으로 빼는 곳) 안의 패키지만 적는다.
#
#   Scripts\Terrain\add_pack_assets.ps1 이 부른다(직접: -run=pythonscript -script=... -nullrhi).
# 결과: Saved\PackDeps\used_pack_files.txt(한 줄에 Content 기준 파일 경로) · summary.txt(팩별 개수 · 크기)
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT_DIR = os.path.join(ROOT, "Saved", "PackDeps")
# 저장소에 들어가는 Content 폴더(.gitignore 의 ! 줄) — 여기 밖이면 팩
TRACKED = ("Grimoire", "Characters", "Input", "NiagaraExamples", "__ExternalActors__", "__ExternalObjects__")
ROOTS = ["/Game/Grimoire", "/Game/__ExternalActors__/Grimoire", "/Game/__ExternalObjects__/Grimoire"]

ar = unreal.AssetRegistryHelpers.get_asset_registry()
ar.scan_paths_synchronous(["/Game"], True)

start = set()
for root in ROOTS:
    for a in ar.get_assets_by_path(root, recursive=True, include_only_on_disk_assets=True):
        start.add(str(a.package_name))
unreal.log(f"collect_pack_deps: 시작 패키지 {len(start)}")

options = unreal.AssetRegistryDependencyOptions(True, True, False, False, False)   # 단단한 · 부드러운 패키지 참조


def is_pack(package):
    if not package.startswith("/Game/"):
        return False
    top = package[len("/Game/"):].split("/", 1)[0]
    return top not in TRACKED


seen = set(start)
queue = list(start)
packs = set()
while queue:
    pkg = queue.pop()
    deps = ar.get_dependencies(unreal.Name(pkg), options) or []
    for d in deps:
        d = str(d)
        if d in seen or not d.startswith("/Game/"):
            continue
        seen.add(d)
        queue.append(d)
        if is_pack(d):
            packs.add(d)

files = []
for pkg in sorted(packs):
    rel = pkg[len("/Game/"):]
    for ext in (".uasset", ".umap"):
        disk = os.path.join(ROOT, "Content", *rel.split("/")) + ext
        if os.path.exists(disk):
            files.append(("Content/" + rel + ext, os.path.getsize(disk)))
            break
    else:
        unreal.log_warning(f"collect_pack_deps: 파일 없음 {pkg}")
    for extra in (".uexp", ".ubulk"):   # 따로 떨어진 데이터가 있으면 같이
        disk = os.path.join(ROOT, "Content", *rel.split("/")) + extra
        if os.path.exists(disk):
            files.append(("Content/" + rel + extra, os.path.getsize(disk)))

os.makedirs(OUT_DIR, exist_ok=True)
with open(os.path.join(OUT_DIR, "used_pack_files.txt"), "w", encoding="utf-8") as f:
    for path, _ in files:
        f.write(path + "\n")
by_pack = {}
for path, size in files:
    pack = path.split("/")[1]
    n, s = by_pack.get(pack, (0, 0))
    by_pack[pack] = (n + 1, s + size)
total = sum(s for _, s in files)
lines = [f"{pack}: {n}개 {s / 1048576:.1f} MB" for pack, (n, s) in sorted(by_pack.items())]
lines.append(f"합계: {len(files)}개 {total / 1048576:.1f} MB")
with open(os.path.join(OUT_DIR, "summary.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
for line in lines:
    unreal.log(f"collect_pack_deps: {line}")
