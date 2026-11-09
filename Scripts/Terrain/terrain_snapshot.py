# 지형 스냅숏 — 맵을 건드리지 않고 생성기만 돌려 높이 지도(r16)와 물 · 다리 · POI 자료(JSON)를 Saved/Terrain/snap/<라벨>/ 에 쓴다.
# 경사 분포 · 전망대 터 · 물길 연속 검사는 이 결과를 읽는 terrain_stats.py(numpy)가 한다. 묶어서 돌리기: check_terrain.ps1.
#
#   $env:GRIM_SNAP_LABEL = "now"
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Terrain\terrain_snapshot.py" -unattended -nop4 -nosplash -nullrhi
#
# GRIM_SNAP_VARIANTS 에 JSON 파일 경로를 주면 {라벨: {레이아웃 덮어쓰기}} 를 차례로 돌린다 — 수치 비교용("Ridgelines.X" = 모든 줄, "Ridgelines#이름.X" = 한 줄).
import json
import os
import sys

sys.dont_write_bytecode = True
import unreal  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
LAYOUT_PATH = os.path.join(HERE, "openbeta_layout.json")
lib = unreal.GrimTerrainEditorLibrary


def first_out(result):
    return result[-1] if isinstance(result, tuple) else result


def apply_overrides(raw, overrides):
    for key, value in overrides.items():
        if "." in key:
            head, field = key.split(".", 1)
            name = None
            if "#" in head:            # "Ridgelines#Spur_BeaconSE.Heights" — 한 줄만
                head, name = head.split("#", 1)
            for row in raw[head]:
                if name is None or row.get("Name") == name:
                    row[field] = value
        else:
            raw[key] = value


def snapshot(label, overrides):
    out = os.path.join(ROOT, "Saved", "Terrain", "snap", label)
    os.makedirs(out, exist_ok=True)
    with open(LAYOUT_PATH, encoding="utf-8") as f:
        raw = json.load(f)
    apply_overrides(raw, overrides)
    temp = os.path.join(out, "layout.json")
    with open(temp, "w", encoding="utf-8") as f:
        json.dump(raw, f, ensure_ascii=False)
    layout = first_out(lib.load_layout_from_json(temp))
    summary = first_out(lib.generate_terrain(layout))
    lib.export_heightmap_r16(os.path.join(out, "h.r16"))
    origin = raw["WorldOrigin"]

    def w2m(x, y):
        return y / 100.0 + origin["X"], x / 100.0 + origin["Y"]

    water = []
    for body in summary.water:
        if str(body.kind) != "Creek" and str(body.kind) != "Pond":
            continue
        pts = [(*w2m(p.x, p.y), p.z / 100.0) for p in body.points]
        water.append({"kind": str(body.kind), "name": str(body.name), "points": pts, "widths": [w / 100.0 for w in body.widths], "pools": list(body.pools),
                      "size": body.size / 100.0, "depth": body.depth / 100.0, "surface": bool(body.surface_mesh)})
    bridges = []
    for b in summary.bridges:
        u, v = w2m(b.center.x, b.center.y)
        bridges.append({"name": str(b.name), "u": u, "v": v, "z": b.center.z / 100.0, "span": b.span / 100.0, "angle": b.crossing_angle, "water": b.water_z / 100.0})
    meta = {"size_x": summary.size_x, "size_y": summary.size_y, "quad": raw["QuadSize"], "water": water, "bridges": bridges, "analysis": lib.run_terrain_analysis()}
    with open(os.path.join(out, "meta.json"), "w", encoding="utf-8") as f:
        json.dump(meta, f, ensure_ascii=False)
    unreal.log(f"terrain_snapshot: {label} 끝 ({summary.size_x} × {summary.size_y})")


variants_path = os.environ.get("GRIM_SNAP_VARIANTS", "")
if variants_path:
    with open(variants_path, encoding="utf-8") as vf:
        for name, ov in json.load(vf).items():
            snapshot(name, ov)
else:
    snapshot(os.environ.get("GRIM_SNAP_LABEL", "now"), {})
