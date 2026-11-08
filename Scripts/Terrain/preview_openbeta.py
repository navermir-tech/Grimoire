# 맵을 만들지 않고 레이아웃만 미리 보기(레벨 디자인 2차) — 높이 지도 생성 · 설계 검사 · 등고선 지도 · 식생 자리 CSV.
# 레이아웃(openbeta_layout.json)을 고치며 몇 분 안에 모양을 보려고 쓴다. 맵 · 에셋은 건드리지 않는다(-nullrhi 로 돌려도 된다).
#
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Terrain\preview_openbeta.py" -unattended -nop4 -nosplash -nullrhi
#   python Scripts\Terrain\overlay_foliage.py          # 식생 자리를 지도 위에 점으로(시스템 파이썬 + Pillow)
#
# 결과: Saved/Terrain/preview/ — analysis.txt(설계 검사), sector.png(마탑 구간 등고선), tree.png(고목 둘레), map.png(전체),
#       foliage_<규칙>.csv(지도 U, V, 크기, 종 값). 그림의 범위는 아래 REGIONS.
import json
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
LAYOUT_PATH = os.environ.get("GRIM_LAYOUT", os.path.join(HERE, "openbeta_layout.json"))
OUT = os.path.join(ROOT, "Saved", "Terrain", "preview")
# 이름: (지도 최소, 지도 최대, 픽셀당 m, 등고선 m)
REGIONS = {
    "sector": ((380, 60), (1840, 1160), 1.25, 2.0),
    "detail": ((1150, 250), (1750, 750), 0.5, 1.0),
    "tree": ((700, 1600), (1500, 2350), 1.0, 2.0),
    "map": ((0, 0), (2032, 4064), 2.5, 5.0),
}

lib = unreal.GrimTerrainEditorLibrary
os.makedirs(OUT, exist_ok=True)


def first_out(result):
    if isinstance(result, tuple):
        if not result[0]:
            raise RuntimeError("C++ 호출 실패(LogGrimTerrain)")
        return result[1]
    if result is None or result is False:
        raise RuntimeError("C++ 호출 실패(LogGrimTerrain)")
    return result


layout = first_out(lib.load_layout_from_json(LAYOUT_PATH))
summary = first_out(lib.generate_terrain(layout))
with open(LAYOUT_PATH, encoding="utf-8") as f:
    raw = json.load(f)
origin = raw["WorldOrigin"]
report = lib.run_terrain_analysis()
with open(os.path.join(OUT, "analysis.txt"), "w", encoding="utf-8") as f:
    f.write(f"높이 {summary.min_height:.1f} ~ {summary.max_height:.1f} m, 언덕 {summary.hill_count} · 작은 모양 {summary.outcrop_count}\n")
    f.write(report)
unreal.log("preview_openbeta: 설계 검사\n" + report)
for name, (lo, hi, mpp, contour) in REGIONS.items():
    lib.export_region_preview_image(os.path.join(OUT, f"{name}.png"), unreal.Vector2D(*lo), unreal.Vector2D(*hi), mpp, contour)

identity = unreal.Transform()
for rule in raw.get("Foliage", []):
    name = rule["Name"]
    result = lib.get_foliage_instances(name, identity)
    transforms, variants = (result[1], result[2]) if isinstance(result, tuple) and len(result) == 3 else (result[0], result[1])
    with open(os.path.join(OUT, f"foliage_{name}.csv"), "w", encoding="utf-8") as f:
        for t, var in zip(transforms, variants):
            u = t.translation.y / 100.0 + origin["X"]
            v = t.translation.x / 100.0 + origin["Y"]
            f.write(f"{u:.1f},{v:.1f},{t.scale3d.x:.2f},{var:.3f}\n")
    unreal.log(f"preview_openbeta: 식생 {name} {len(transforms)}개")
unreal.log(f"preview_openbeta: 끝 — {OUT}")
