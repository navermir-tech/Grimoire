# 지형 수치 검사(레벨 디자인 4차, docs/qa/level-art.md) — terrain_snapshot.py 가 쓴 Saved/Terrain/snap/<라벨>/ 을 읽어 잰다. 시스템 파이썬 + numpy · Pillow.
#
#   python Scripts/Terrain/terrain_stats.py <라벨> [--png 왼쪽u,아래v,오른쪽u,위v 이름] [--json 결과파일]
#
# 재는 것
#   경사     마탑 구간 안 경사 분포(칸 1 m). 기준 = 캐릭터가 걸을 수 있는 경사(CharacterMovement 기본 WalkableFloorZ 0.71 = 44.76°)
#   오르기   탑 평지에서 걸어서 갈 수 있는 곳(한 칸 경사 ≤ 44.76° 또는 턱 ≤ MaxStepHeight 45 cm)을 8방향으로 넓혀 나가, 구간의 몇 % 에 닿는가
#   터       POI · 랜드마크 자리: 원판 반지름 안 높이 차 · 최대 경사 · 원판이 땅에서 뜨는 높이 · 탑에서 걸어서 닿는가
#   물길     자연 개울 가운데선을 25 cm 마다: 수심(물 높이 − 땅, 격자 쌍선형 보간 = 실제 랜드스케이프 모양). 2 m · 4 m 간격으로 솎은 땅(먼 LOD)도 같이
import argparse
import json
import math
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
WALK_DEG = math.degrees(math.acos(0.71))
WALK_TAN = math.tan(math.radians(WALK_DEG))
STEP_M = 0.45
MIN_DEPTH_M = 0.30          # 물길 가운데 최소 수심 기준(레이아웃 값이 아니라 검사 기준)


def load(label):
    base = os.path.join(ROOT, "Saved", "Terrain", "snap", label)
    with open(os.path.join(base, "meta.json"), encoding="utf-8") as f:
        meta = json.load(f)
    with open(os.path.join(base, "layout.json"), encoding="utf-8") as f:
        raw = json.load(f)
    data = np.fromfile(os.path.join(base, "h.r16"), dtype=np.uint16)
    # 파일 순서 = 랜드스케이프 순서(가로 = 북쪽 SizeY 개, 세로 = 동쪽 SizeX 개) → h[v, u]
    a = data.reshape(meta["size_x"], meta["size_y"]).astype(np.float32)
    h = ((a - 32768.0) / 128.0).T
    return meta, raw, h


def poly_mask(poly, w, h):
    """다각형 안(래스터) — h[v, u] 모양 불리언"""
    from PIL import ImageDraw
    img = Image.new("L", (w, h), 0)
    ImageDraw.Draw(img).polygon([(p["X"], p["Y"]) for p in poly], fill=1)
    return np.asarray(img, dtype=bool)


def slope_deg(h):
    gy, gx = np.gradient(h, 1.0)
    return np.degrees(np.arctan(np.hypot(gx, gy)))


def reach(h, start_v, start_u):
    """걸어서 닿는 칸 — 맞닿은 칸끼리 경사(또는 턱) 조건을 넘지 않으면 번진다(numpy 전선 탐색)"""
    rows, cols = h.shape
    done = np.zeros(h.shape, dtype=bool)
    done[start_v, start_u] = True
    fv, fu = np.array([start_v]), np.array([start_u])
    dirs = [(dv, du, math.hypot(dv, du)) for dv in (-1, 0, 1) for du in (-1, 0, 1) if dv or du]
    while len(fv):
        nv, nu = [], []
        for dv, du, d in dirs:
            limit = max(WALK_TAN * d, STEP_M)
            tv, tu = fv + dv, fu + du
            inside = (tv >= 0) & (tv < rows) & (tu >= 0) & (tu < cols)
            sv, su, tv, tu = fv[inside], fu[inside], tv[inside], tu[inside]
            ok = (~done[tv, tu]) & (np.abs(h[sv, su] - h[tv, tu]) <= limit)
            tv, tu = tv[ok], tu[ok]
            done[tv, tu] = True
            nv.append(tv)
            nu.append(tu)
        fv, fu = np.concatenate(nv), np.concatenate(nu)
    return done


def bilinear(h, u, v, step=1):
    """step 칸으로 솎은 땅(먼 LOD) 위의 쌍선형 보간"""
    uu, vv = u / step, v / step
    iu, iv = int(math.floor(uu)), int(math.floor(vv))
    fu, fv = uu - iu, vv - iv
    a = h[iv * step, iu * step]
    b = h[iv * step, (iu + 1) * step]
    c = h[(iv + 1) * step, iu * step]
    d = h[(iv + 1) * step, (iu + 1) * step]
    return float(a * (1 - fu) * (1 - fv) + b * fu * (1 - fv) + c * (1 - fu) * fv + d * fu * fv)


def creek_report(body, h):
    pts = body["points"]
    rows = []
    for i in range(len(pts) - 1):
        (u0, v0, z0), (u1, v1, z1) = pts[i], pts[i + 1]
        seg = math.hypot(u1 - u0, v1 - v0)
        n = max(1, int(seg / 0.25))
        for k in range(n):
            t = k / n
            rows.append((u0 + (u1 - u0) * t, v0 + (v1 - v0) * t, z0 + (z1 - z0) * t, (i + t) * 2.0))
    out = {"name": body["name"], "length": round(rows[-1][3], 1), "samples": len(rows)}
    for step, tag in ((1, "lod0"), (2, "lod1"), (4, "lod2")):
        d = np.array([z - bilinear(h, u, v, step) for u, v, z, _ in rows])
        out[tag] = {"min": round(float(d.min()), 3), "dry": int((d <= 0).sum()), "shallow": int((d < MIN_DEPTH_M).sum()), "median": round(float(np.median(d)), 3)}
        # 끊긴 구간(연속해서 물이 땅 아래인 곳)
        runs, cur = [], None
        for (u, v, z, a), dd in zip(rows, d):
            if dd <= 0.02:
                if cur is None:
                    cur = [a, a, u, v]
                cur[1] = a
            elif cur is not None:
                runs.append(cur)
                cur = None
        if cur is not None:
            runs.append(cur)
        out[tag]["breaks"] = len(runs)
        out[tag]["break_m"] = round(sum(r[1] - r[0] + 0.25 for r in runs), 1)
        if tag == "lod0":
            out[tag]["examples"] = [[round(r[0], 1), round(r[1], 1), round(r[2], 1), round(r[3], 1)] for r in runs[:8]]
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("label")
    ap.add_argument("--png", nargs=2, action="append", default=[], metavar=("BOX", "NAME"))
    ap.add_argument("--json")
    ap.add_argument("--check", action="store_true", help="기준 판정 — 마지막 줄 PASS/FAIL, 실패면 종료 코드 1")
    args = ap.parse_args()
    meta, raw, h = load(args.label)
    sector = raw["Analysis"]["Sector"]["Polygon"]
    mask = poly_mask(sector, h.shape[1], h.shape[0])
    s = slope_deg(h)
    sea = raw.get("SeaLevel", 0.0)
    land = mask & (h > sea + 0.3)
    v = s[land]
    bins = [0, 5, 25, 30, 35, 40, WALK_DEG, 90]
    hist, _ = np.histogram(v, bins=bins)
    report = {"label": args.label, "walk_deg": round(WALK_DEG, 2), "cells": int(land.sum())}
    report["slope_pct"] = {f"{bins[i]:.0f}-{bins[i + 1]:.0f}": round(100.0 * hist[i] / len(v), 2) for i in range(len(hist))}
    report["slope_p95"] = round(float(np.percentile(v, 95)), 1)
    report["slope_p99"] = round(float(np.percentile(v, 99)), 1)
    report["slope_max"] = round(float(v.max()), 1)
    report["unwalkable_pct"] = round(100.0 * float((v > WALK_DEG).mean()), 3)
    # 의도한 벼랑(레이아웃 Analysis.Sector.IntentionalCliffs) 밖에서 못 오르는 칸
    outside = land.copy()
    for box in raw["Analysis"]["Sector"].get("IntentionalCliffs", []):
        outside[box["Min"]["Y"]:box["Max"]["Y"], box["Min"]["X"]:box["Max"]["X"]] = False
    report["unwalkable_outside_intended_pct"] = round(100.0 * float((s[outside] > WALK_DEG).mean()), 3)
    report["steep40_outside_intended_pct"] = round(100.0 * float((s[outside] > 40.0).mean()), 3)
    tower = raw["TowerHill"]["Center"]
    done = reach(h, int(tower["Y"]), int(tower["X"]))
    report["reach_pct"] = round(100.0 * float(done[land].mean()), 2)

    spots = []
    for row in raw["Pois"] + raw["Landmarks"]:
        cu, cv = row["Center"]["X"], row["Center"]["Y"]
        r = row.get("Radius", 10)
        iu, iv = int(cu), int(cv)
        R = int(r + 2)
        if iu - R < 1 or iv - R < 1 or iu + R >= h.shape[1] - 1 or iv + R >= h.shape[0] - 1:
            continue
        vv, uu = np.mgrid[iv - R:iv + R + 1, iu - R:iu + R + 1]
        disc = (uu - cu) ** 2 + (vv - cv) ** 2 <= r * r
        ground = h[iv - R:iv + R + 1, iu - R:iu + R + 1][disc]
        slopes = s[iv - R:iv + R + 1, iu - R:iu + R + 1][disc]
        center = bilinear(h, cu, cv)
        spots.append({"name": row["Name"], "kind": row.get("Kind"), "r": r, "in_sector": bool(mask[iv, iu]),
                      "range": round(float(ground.max() - ground.min()), 2), "max_slope": round(float(slopes.max()), 1), "mean_slope": round(float(slopes.mean()), 1),
                      "overhang": round(float(center - ground.min()), 2), "reach": bool(done[iv, iu])})
    report["spots"] = spots
    report["creeks"] = [creek_report(b, h) for b in meta["water"] if b["kind"] == "Creek" and b["surface"]]
    report["bridges"] = [{"name": b["name"], "angle": round(b["angle"], 1), "span": round(b["span"], 1), "clear": round(b["z"] - b["water"], 2)} for b in meta["bridges"]]
    report["analysis"] = meta.get("analysis", "")

    # 터(레이아웃 Pads): 평평한 안쪽 — 높이 차 · 최대 경사 · 걸어서 닿는가. 바깥 15 % 는 길 어깨 · 흔들리는 가장자리라 뺀다
    pads = []
    for row in raw.get("Pads", []):
        cu, cv, r = row["Center"]["X"], row["Center"]["Y"], row["Radius"]
        R = int(r + 2)
        iu, iv = int(cu), int(cv)
        vv, uu = np.mgrid[iv - R:iv + R + 1, iu - R:iu + R + 1]
        disc = (uu - cu) ** 2 + (vv - cv) ** 2 <= (r * 0.85) ** 2
        ground = h[iv - R:iv + R + 1, iu - R:iu + R + 1][disc]
        slopes = s[iv - R:iv + R + 1, iu - R:iu + R + 1][disc]
        pads.append({"name": row["Name"], "r": r, "range": round(float(ground.max() - ground.min()), 2), "max_slope": round(float(np.percentile(slopes, 98)), 1), "reach": bool(done[iv, iu])})
    report["pads"] = pads

    for box, name in args.png:
        u0, v0, u1, v1 = [int(x) for x in box.split(",")]
        region = h[v0:v1, u0:u1]
        gy, gx = np.gradient(region, 1.0)
        # 낮은 해 + 경사 색 — 벼랑 띠 · 톱니가 드러나게
        light = np.array([-0.6, 0.55, 0.58])
        n = np.dstack((-gx, -gy, np.ones_like(region)))
        n /= np.linalg.norm(n, axis=2, keepdims=True)
        shade = np.clip((n @ (light / np.linalg.norm(light))), 0, 1)
        sl = s[v0:v1, u0:u1]
        rgb = np.dstack((shade, shade, shade))
        for lim, col in ((35.0, (1.0, 0.85, 0.3)), (40.0, (1.0, 0.5, 0.15)), (WALK_DEG, (1.0, 0.1, 0.1))):
            m = sl > lim
            rgb[m] = rgb[m] * 0.45 + np.array(col) * 0.55
        img = Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8)).transpose(Image.FLIP_TOP_BOTTOM)
        target = os.path.join(ROOT, "Saved", "Terrain", "snap", args.label, name + ".png")
        img.save(target)
        report.setdefault("png", []).append(target)

    brief = {k: v for k, v in report.items() if k not in ("spots", "analysis", "creeks")}
    print(json.dumps(brief, ensure_ascii=False, indent=1))
    for row in report["creeks"]:
        print(json.dumps(row, ensure_ascii=False))
    print("-- 원판이 뜨거나 경사가 큰 자리(겹쳐 놓을 곳: 원판 뜸 > 0.5 m 또는 최대 경사 > 15°, 구간 안 먼저)")
    for row in sorted(report["spots"], key=lambda x: -x["overhang"]):
        if row["overhang"] > 0.5 or row["max_slope"] > 15:
            print(json.dumps(row, ensure_ascii=False))
    for row in report["pads"]:
        print(json.dumps(row, ensure_ascii=False))
    if args.json:
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump(report, f, ensure_ascii=False, indent=1)
    if args.check:
        fails = []
        for c in report["creeks"]:
            if c["lod0"]["dry"] or c["lod0"]["breaks"]:
                fails.append(f"{c['name']} 물길 끊김 LOD0 {c['lod0']['breaks']} 곳")
            if c["lod0"]["min"] < MIN_DEPTH_M:
                fails.append(f"{c['name']} 최소 수심 {c['lod0']['min']} m < {MIN_DEPTH_M}")
            if c["lod1"]["breaks"]:
                fails.append(f"{c['name']} 물길 끊김 LOD1 {c['lod1']['breaks']} 곳")
        for row in report["pads"]:
            if next((p.get("CheckSkip") for p in raw["Pads"] if p["Name"] == row["name"]), None):
                continue   # 레이아웃이 까닭을 적어 뺀 터(Q140)
            if row["range"] > 1.3 or row["max_slope"] > 12.0 or not row["reach"]:
                fails.append(f"터 {row['name']} 높이 차 {row['range']} · 경사 {row['max_slope']} · 닿음 {row['reach']}")
        if report["unwalkable_outside_intended_pct"] > 0.2:
            fails.append(f"의도한 벼랑 밖 못 오르는 칸 {report['unwalkable_outside_intended_pct']} %")
        if report["reach_pct"] < 99.5:
            fails.append(f"걸어서 닿는 땅 {report['reach_pct']} %")
        for b in report["bridges"]:
            if b["angle"] < 75.0:
                fails.append(f"다리 {b['name']} 사이각 {b['angle']}°")
        for line in fails:
            print("FAIL: " + line)
        print("terrain_stats: " + ("PASS" if not fails else "FAIL"))
        if fails:
            sys.exit(1)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
