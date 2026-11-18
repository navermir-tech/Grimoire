# 초원 꾸밈 이펙트 텍스처(자체 — PIL). 다시 돌려도 같다(시드 고정).
#   python art/tools/make_meadow_textures.py      → art/meadow/textures/*.png
#  - T_Meadow_Butterfly.png  RGBA 128²: 위아래 날개 한 쌍(흰 바탕 무늬 — 색은 머티리얼이 파티클마다 고른다), 알파 = 모양
#  - T_Meadow_Puff.png       RGBA 64²: 부드러운 솜 덩이(가장자리 보송)
import math
import os
import random

from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "meadow", "textures")
os.makedirs(OUT, exist_ok=True)
rng = random.Random(20261118)

# ── 나비: 몸은 세로 가운데, 날개는 좌우. 머티리얼이 U 를 가운데 기준으로 접었다 펴며 날갯짓한다 ──
S = 128
img = Image.new("RGBA", (S * 4, S * 4), (0, 0, 0, 0))   # 4배로 그리고 줄인다(가장자리 부드럽게)
d = ImageDraw.Draw(img)
c = S * 2


def wing(cx, cy, rx, ry, angle, fill):
    pts = []
    for i in range(48):
        a = i / 48 * math.tau
        r = 1.0 + 0.12 * math.sin(a * 3)
        x, y = math.cos(a) * rx * r, math.sin(a) * ry * r
        ca, sa = math.cos(angle), math.sin(angle)
        pts.append((cx + x * ca - y * sa, cy + x * sa + y * ca))
    d.polygon(pts, fill=fill)


for side in (-1, 1):
    wing(c + side * 120, c - 60, 120, 82, side * -0.5, (250, 250, 245, 255))   # 앞날개
    wing(c + side * 95, c + 95, 82, 70, side * 0.55, (240, 238, 230, 255))     # 뒷날개
    # 무늬: 날개 끝 어두운 테 · 점
    wing(c + side * 175, c - 95, 48, 34, side * -0.5, (60, 45, 40, 255))
    d.ellipse((c + side * 120 - 22, c - 70 - 22, c + side * 120 + 22, c - 70 + 22), fill=(70, 55, 45, 255))
    d.ellipse((c + side * 100 - 14, c + 105 - 14, c + side * 100 + 14, c + 105 + 14), fill=(255, 255, 255, 255))
d.ellipse((c - 14, c - 120, c + 14, c + 150), fill=(45, 35, 30, 255))   # 몸
d.line((c - 6, c - 118, c - 40, c - 200), fill=(45, 35, 30, 255), width=6)   # 더듬이
d.line((c + 6, c - 118, c + 40, c - 200), fill=(45, 35, 30, 255), width=6)
img = img.resize((S, S), Image.LANCZOS)
img.save(os.path.join(OUT, "T_Meadow_Butterfly.png"))

# ── 솜 덩이: 겹친 원 몇 개 + 흐림 ──
P = 64
puff = Image.new("L", (P * 4, P * 4), 0)
pd = ImageDraw.Draw(puff)
for i in range(9):
    a = rng.uniform(0, math.tau)
    rr = rng.uniform(0, 40)
    r = rng.uniform(42, 70)
    x, y = P * 2 + math.cos(a) * rr, P * 2 + math.sin(a) * rr
    pd.ellipse((x - r, y - r, x + r, y + r), fill=255)
puff = puff.filter(ImageFilter.GaussianBlur(18)).resize((P, P), Image.LANCZOS)
rgba = Image.merge("RGBA", (Image.new("L", (P, P), 255), Image.new("L", (P, P), 252), Image.new("L", (P, P), 245), puff))
rgba.save(os.path.join(OUT, "T_Meadow_Puff.png"))
print("textures:", sorted(os.listdir(OUT)))
