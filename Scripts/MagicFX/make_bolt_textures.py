# 볼트 이펙트 텍스처: GPT 가 검은 바탕에 그린 그림(art/fx/raw/tex_*.png) → 회색조 마스크(밝기 = 알파).
# 언리얼 밖에서 돈다(파이썬 3 + numpy + Pillow). 색은 머티리얼 · 파티클 색이 입힌다.
#
#   python Scripts/MagicFX/make_bolt_textures.py        → art/fx/bolt/T_Bolt_*.png
#
# 언리얼로 들여오기는 make_bolt_fx.py(textures 단계). 원본 시안 · 프롬프트는 art/fx/README.md.

import os

import numpy as np
from PIL import Image, ImageFilter

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "art", "fx")
RAW = os.path.join(ROOT, "raw")
OUT = os.path.join(ROOT, "bolt")

# 원본 → (텍스처 이름, 출력 크기 (가로, 세로), 검은 점, 흰 점). 가로로 긴 것은 가로 · 세로 비를 남긴다.
# 빛줄기 · 불티는 GPT 그림에서 오른쪽을 가리키지만, Niagara 의 속도 정렬 스프라이트는 텍스처 「위」를 진행 방향으로
# 삼는다 — 그래서 저장할 때 왼쪽으로 90° 돌린다(UP).
JOBS = {
    "tex_streak": ("T_Bolt_Streak", (1024, 256), 0.05, 0.85, "UP"),
    "tex_spark": ("T_Bolt_Spark", (256, 128), 0.05, 0.8, "UP"),
    "tex_ring": ("T_Bolt_Ring", (512, 512), 0.05, 0.85),
    "tex_flare": ("T_Bolt_Flare", (512, 512), 0.03, 0.9),
    "tex_burst": ("T_Bolt_Burst", (512, 512), 0.05, 0.85),
    "tex_wisp": ("T_Bolt_Wisp", (512, 256), 0.04, 0.8, "UP"),
}


def to_mask(path, size, black, white):
    img = Image.open(path).convert("RGB")
    a = np.asarray(img).astype(np.float32) / 255.0
    lum = a.max(axis=2)  # 색이 섞여 있어도 가장 밝은 채널 = 빛의 세기
    lum = np.clip((lum - black) / (white - black), 0.0, 1.0)

    # 물체가 있는 곳만 잘라 가운데로 — 가로 세로 비는 출력 비에 맞춰 넓힌다.
    ys, xs = np.nonzero(lum > 0.02)
    if len(xs) == 0:
        raise RuntimeError("빈 그림: " + path)
    cx, cy = (xs.min() + xs.max()) / 2.0, (ys.min() + ys.max()) / 2.0
    w, h = xs.max() - xs.min() + 1, ys.max() - ys.min() + 1
    aspect = size[0] / size[1]
    w, h = max(w, h * aspect) * 1.08, max(h, w / aspect) * 1.08
    box = (int(cx - w / 2), int(cy - h / 2), int(cx + w / 2), int(cy + h / 2))
    m = Image.fromarray((lum * 255).astype(np.uint8), "L").crop(box).resize(size, Image.LANCZOS)

    # 가장자리는 반드시 0 — 스프라이트 테두리에 선이 생기지 않게.
    arr = np.asarray(m.filter(ImageFilter.GaussianBlur(0.6))).astype(np.float32) / 255.0
    yy, xx = np.mgrid[0:size[1], 0:size[0]]
    edge = np.minimum.reduce([xx, yy, size[0] - 1 - xx, size[1] - 1 - yy]).astype(np.float32)
    arr *= np.clip(edge / 8.0, 0.0, 1.0)
    return Image.fromarray((arr * 255 + 0.5).astype(np.uint8), "L")


def procedural(job, size):
    """GPT 그림이 없거나 깨졌을 때(codex 가 빈 파일을 남길 때가 있다) 쓰는 대체 그림 — 번쩍임 · 빛살만."""
    n = size[0]
    v = (np.arange(n) + 0.5) / n * 2.0 - 1.0
    x, y = np.meshgrid(v, v)
    r = np.sqrt(x * x + y * y)
    ang = np.arctan2(y, x)
    rng = np.random.default_rng(11)
    if job == "tex_flare":
        glow = np.exp(-(r / 0.22) ** 2) * 0.9 + np.exp(-(r / 0.06) ** 2)
        rays = sum(np.exp(-(np.abs(np.sin(ang - a)) * r / 0.012) ** 2) * np.clip(1 - r / 0.9, 0, 1) ** 3
                   for a in (0.0, np.pi / 2, np.pi / 4, 3 * np.pi / 4)) * 0.35
        arr = glow + rays
    elif job == "tex_burst":
        arr = np.exp(-(r / 0.08) ** 2)
        for _ in range(28):
            a = rng.uniform(0, 2 * np.pi)
            length = rng.uniform(0.35, 0.85)
            width = rng.uniform(0.006, 0.014)
            d = np.abs(np.sin(ang - a)) * r
            along = np.cos(ang - a) > 0
            arr = arr + along * np.exp(-(d / width) ** 2) * np.clip(1 - r / length, 0, 1) ** 1.5
    else:
        raise RuntimeError("대체 그림 없음: " + job)
    arr = np.clip(arr, 0, 1) * np.clip((0.97 - r) / 0.1, 0, 1)
    return Image.fromarray((arr * 255 + 0.5).astype(np.uint8), "L")


def main():
    os.makedirs(OUT, exist_ok=True)
    for job, (name, size, black, white, *flags) in JOBS.items():
        src = os.path.join(RAW, job + ".png")
        try:
            mask = to_mask(src, size, black, white)
        except (OSError, RuntimeError) as e:
            try:
                mask = procedural(job, size)
                print("대체 그림:", job, e)
            except RuntimeError:
                print("건너뜀:", src, e)
                continue
        if "UP" in flags:
            mask = mask.transpose(Image.Transpose.ROTATE_90)
        mask.save(os.path.join(OUT, name + ".png"))
        print("텍스처", name, mask.size)


if __name__ == "__main__":
    main()
