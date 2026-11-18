"""콘셉트 그림과 게임 스크린숏을 나란히 붙인 비교 그림을 만든다(docs/art/tower-art.md · docs/qa/level-art.md).

    python Scripts/QA/compare_concept.py

짝(콘셉트 → 스크린숏 전 · 후)은 아래 PAIRS. 결과는 docs/public/qa/tower-compare-<이름>.jpg — 왼쪽 콘셉트, 가운데 전, 오른쪽 후.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
CONCEPT = os.path.join(ROOT, "docs", "public", "art", "region1")
QA = os.path.join(ROOT, "docs", "public", "qa")
PAIRS = [
    ("exterior", "tower-exterior", "tower-aerial"),
    ("dusk", "tower-dusk", "tower-road-dusk"),
    ("road", "tower-dusk", "tower-road"),
    ("yard", "tower-yard", "tower-yard"),
    ("hall", "tower-in-hall", "tower-hall"),
    ("start", "tower-in-startroom", "tower-start"),
    ("library", "tower-in-library", "tower-library"),
    ("kitchen", "tower-in-kitchen", "tower-kitchen"),
]
H = 540


def fit(path):
    im = Image.open(path).convert("RGB")
    return im.resize((int(im.width * H / im.height), H), Image.LANCZOS)


def font(size):
    for f in ("C:/Windows/Fonts/malgun.ttf", "C:/Windows/Fonts/gulim.ttc"):
        if os.path.exists(f):
            return ImageFont.truetype(f, size)
    return ImageFont.load_default()


def main():
    made = 0
    for name, concept, shot in PAIRS:
        cpath = os.path.join(CONCEPT, concept + ".jpg")
        before = os.path.join(QA, f"{shot}-before.jpg") if not shot.endswith("-dusk") else os.path.join(QA, f"{shot[:-5]}-before-dusk.jpg")
        after = os.path.join(QA, f"{shot}-after.jpg") if not shot.endswith("-dusk") else os.path.join(QA, f"{shot[:-5]}-after-dusk.jpg")
        panels = [(cpath, "콘셉트"), (before, "전(그레이박스 · 팩)"), (after, "후(자체 제작)")]
        panels = [(p, t) for p, t in panels if os.path.exists(p)]
        if len(panels) < 2:
            print(f"건너뜀 {name}: 그림 모자람")
            continue
        ims = [(fit(p), t) for p, t in panels]
        out = Image.new("RGB", (sum(im.width for im, _ in ims) + 8 * (len(ims) - 1), H + 40), (24, 24, 28))
        d = ImageDraw.Draw(out)
        x = 0
        for im, title in ims:
            out.paste(im, (x, 40))
            d.text((x + 10, 8), title, fill=(235, 230, 220), font=font(22))
            x += im.width + 8
        path = os.path.join(QA, f"tower-compare-{name}.jpg")
        out.save(path, quality=86)
        made += 1
        print(f"[OK] {path}")
    return made


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
