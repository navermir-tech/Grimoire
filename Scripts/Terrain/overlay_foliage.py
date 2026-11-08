# preview_openbeta.py 가 남긴 지도(Saved/Terrain/preview/*.png)에 식생 자리(foliage_*.csv)를 점으로 얹는다 — 시스템 파이썬 + Pillow.
# 나무 = 진한 초록(크기 비례, 종 값으로 색), 덤불 = 연두, 바위 = 회색. 결과는 같은 폴더의 *_foliage.png.
#   python Scripts\Terrain\overlay_foliage.py
import csv
import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "Saved", "Terrain", "preview")
# preview_openbeta.py REGIONS 와 같아야 한다
REGIONS = {
    "sector": ((380, 60), (1840, 1160), 1.25),
    "detail": ((1150, 250), (1750, 750), 0.5),
    "tree": ((700, 1600), (1500, 2350), 1.0),
    "map": ((0, 0), (2032, 4064), 2.5),
}
STYLE = {   # 규칙: (색 A, 색 B — 종 값 0 → 1, 지름 m)
    "Tree": ((20, 70, 25), (120, 150, 40), 7.0),
    "DarkTree": ((10, 45, 30), (30, 70, 45), 6.0),
    "Bush": ((90, 170, 60), (150, 200, 80), 2.6),
    "Rock": ((150, 150, 150), (110, 105, 100), 2.2),
}


def load(name):
    path = os.path.join(OUT, f"foliage_{name}.csv")
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8") as f:
        return [tuple(float(x) for x in row) for row in csv.reader(f)]


for region, ((u0, v0), (u1, v1), mpp) in REGIONS.items():
    src = os.path.join(OUT, f"{region}.png")
    if not os.path.exists(src):
        continue
    img = Image.open(src).convert("RGB")
    draw = ImageDraw.Draw(img)
    h = img.size[1]
    for rule in ("Rock", "Bush", "DarkTree", "Tree"):
        a, b, diameter = STYLE.get(rule, ((255, 0, 255), (255, 0, 255), 3.0))
        for u, v, scale, variant in load(rule):
            if not (u0 <= u <= u1 and v0 <= v <= v1):
                continue
            x = (u - u0) / mpp
            y = h - 1 - (v - v0) / mpp
            r = max(0.8, diameter * scale * 0.5 / mpp)
            c = tuple(int(a[i] + (b[i] - a[i]) * variant) for i in range(3))
            draw.ellipse((x - r, y - r, x + r, y + r), fill=c)
    dst = os.path.join(OUT, f"{region}_foliage.png")
    img.save(dst)
    print(dst)
sys.exit(0)
