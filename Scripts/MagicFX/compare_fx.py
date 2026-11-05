# 콘셉트 시트(왼쪽)와 게임 화면 네 순간(오른쪽 2×2)을 한 장으로 붙인다 — 시안 대 결과 확인용.
#   python Scripts/MagicFX/compare_fx.py <시트 png> <캡처 폴더> <출력 jpg> <잘라낼 상자 x0,y0,x1,y1> <순간 4개 쉼표>
# 예: python Scripts/MagicFX/compare_fx.py art/fx/raw/ember.png art/fx/ingame/ember-v8 docs/public/art/fx/ember-compare.jpg 380,180,860,660 0.15,0.3,0.45,0.6
# 캡처 폴더의 shot_<순간>.png 는 capture_spell.ps1 이 만든다.

import os
import sys

from PIL import Image, ImageDraw

LABELS = ["1 시전", "2 몸통", "3 꼬리", "4 터짐"]


def main():
    sheet, shots, out, box, moments = sys.argv[1:6]
    box = tuple(int(v) for v in box.split(","))
    moments = moments.split(",")
    side = 1024
    concept = Image.open(sheet).convert("RGB").resize((side, side))
    canvas = Image.new("RGB", (side * 2 + 16, side), (20, 22, 26))
    canvas.paste(concept, (0, 0))
    cell = side // 2
    draw = ImageDraw.Draw(canvas)
    for i, m in enumerate(moments[:4]):
        im = Image.open(os.path.join(shots, "shot_%s.png" % m)).convert("RGB").crop(box).resize((cell, cell))
        x = side + 16 + (i % 2) * cell
        y = (i // 2) * cell
        canvas.paste(im, (x, y))
        draw.text((x + 10, y + 8), "%s  t=%ss" % (LABELS[i].split()[0], m), fill=(255, 255, 255))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    canvas.save(out, quality=88)
    print(out)


if __name__ == "__main__":
    main()
