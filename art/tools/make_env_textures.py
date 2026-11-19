"""환경 마스터 재질(M_GrimEnv_Master)이 쓰는 반복(타일) PBR 텍스처를 절차적으로 만든다 — 사 온 텍스처 없이.

    python art/tools/make_env_textures.py [결과 폴더=art/env/materials] [크기=1024]

세트마다 세 장: <이름>_BC(색, sRGB) · <이름>_N(탄젠트 노멀, OpenGL 식 +Y) · <이름>_ORM(R = AO, G = 거칠기, B = 금속).
- 색은 일부러 **중립(밝은 회색 · 낮은 채도)** 으로 둔다 — 색조는 마스터 재질 인스턴스의 Tint 가 정한다(양피지색 돌 · 남색 슬레이트 · 놋쇠).
- 모두 가로세로로 이어진다(주기 노이즈). 돌 이음매 · 모서리 깎임은 텍스처가 아니라 모델(블렌더 키트)에 있다 — 여기는 표면 결만.
- 손그림처럼 빛을 그려 넣지 않는다(아트 방향 「Realistic PBR Materials」). 높이에서 노멀 · AO 를 계산한다.
"""
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "env", "materials")
N = int(sys.argv[2]) if len(sys.argv) > 2 else 1024
rng = np.random.default_rng(20261118)


def fbm(n, beta=2.0, seed=0, lo=1, hi=None):
    """주기(타일) 프랙탈 노이즈 — 주파수 영역에서 1/f^beta 로 거른 백색 잡음. 0~1."""
    r = np.random.default_rng(seed)
    f = np.fft.fft2(r.standard_normal((n, n)))
    fy = np.fft.fftfreq(n)[:, None] * n
    fx = np.fft.fftfreq(n)[None, :] * n
    k = np.sqrt(fx * fx + fy * fy)
    k[0, 0] = 1.0
    amp = k ** (-beta / 2.0)
    amp[k < lo] = 0.0
    if hi:
        amp[k > hi] = 0.0
    amp[0, 0] = 0.0
    out = np.real(np.fft.ifft2(f * amp))
    out -= out.min()
    return out / max(out.max(), 1e-9)


def aniso(n, beta, seed, stretch_x=1.0, stretch_y=1.0):
    """한 방향으로 늘인 주기 노이즈(나무 결 · 정 자국 · 긁힘). stretch_y 가 크면 세로로 길게 이어진다"""
    r = np.random.default_rng(seed)
    f = np.fft.fft2(r.standard_normal((n, n)))
    fy = np.fft.fftfreq(n)[:, None] * n * stretch_y
    fx = np.fft.fftfreq(n)[None, :] * n * stretch_x
    k = np.sqrt(fx * fx + fy * fy)
    k[k < 1e-6] = 1e6
    out = np.real(np.fft.ifft2(f * k ** (-beta / 2.0)))
    out -= out.min()
    return out / max(out.max(), 1e-9)


def blur(a, radius):
    """주기 가우시안 블러(주파수 영역)"""
    n = a.shape[0]
    fy = np.fft.fftfreq(n)[:, None]
    fx = np.fft.fftfreq(n)[None, :]
    g = np.exp(-2.0 * (np.pi * radius) ** 2 * (fx * fx + fy * fy))
    return np.real(np.fft.ifft2(np.fft.fft2(a) * g))


def normal_from_height(h, strength):
    """높이(0~1) → 탄젠트 노멀(+Y 위, OpenGL). strength = 높이 1 이 몇 픽셀 깊이인가"""
    dx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * 0.5 * strength
    dy = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * 0.5 * strength
    nz = np.ones_like(h)
    length = np.sqrt(dx * dx + dy * dy + nz * nz)
    return np.stack([-dx / length, dy / length, nz / length], -1) * 0.5 + 0.5   # 그림 행은 아래로 자라므로 dy 부호가 +


def ao_from_height(h, radius, strength):
    """오목한 곳(둘레 평균보다 낮은 곳)을 어둡게"""
    return np.clip(1.0 - np.clip(blur(h, radius) - h, 0, 1) * strength, 0, 1)


def save(name, bc, normal, orm):
    os.makedirs(OUT, exist_ok=True)
    for suffix, data in (("BC", bc), ("N", normal), ("ORM", orm)):
        img = Image.fromarray((np.clip(data, 0, 1) * 255.0 + 0.5).astype(np.uint8), "RGB")
        img.save(os.path.join(OUT, f"T_GrimEnv_{name}_{suffix}.png"))
    print(f"  {name}: BC 평균 {bc.mean():.2f} · 거칠기 평균 {orm[..., 1].mean():.2f}")


def grey(v, warm=0.0):
    """밝기 v → RGB(warm > 0 이면 살짝 따뜻하게 — 색조는 인스턴스 Tint 가 다시 정한다)"""
    return np.stack([v * (1 + warm), v, v * (1 - warm)], -1)


def stone():
    """석회암 마름돌 표면: 큰 얼룩 · 잔 알갱이 · 얕은 구멍 · 비스듬한 정 자국"""
    big = fbm(N, 2.6, 1, hi=10)
    mid = fbm(N, 2.0, 2, lo=6, hi=60)
    grain = fbm(N, 0.9, 3, lo=40)
    pits = np.clip((fbm(N, 1.6, 4, lo=20, hi=140) - 0.66) * 6.0, 0, 1)
    chisel = aniso(N, 1.4, 5, stretch_x=0.25, stretch_y=1.0)
    chisel = np.roll(chisel, 37, 0) * 0.6 + np.roll(chisel.T, 91, 1) * 0.4
    height = 0.45 * mid + 0.2 * grain + 0.25 * chisel + 0.1 * big - 0.5 * pits
    height = (height - height.min()) / (height.max() - height.min())
    value = 0.74 + 0.10 * (big - 0.5) + 0.07 * (mid - 0.5) + 0.05 * (grain - 0.5) - 0.12 * pits
    warm = 0.02 + 0.04 * (fbm(N, 2.4, 6, hi=8) - 0.5)
    bc = np.stack([value * (1 + warm), value, value * (1 - warm * 1.4)], -1)
    ao = ao_from_height(height, 6, 2.2)
    rough = np.clip(0.82 + 0.10 * (grain - 0.5) + 0.08 * pits - 0.05 * (big - 0.5), 0, 1)
    save("Stone", bc, normal_from_height(height, 9.0), np.stack([ao, rough, np.zeros_like(ao)], -1))


def slate():
    """슬레이트 지붕널: 가로 줄(겹친 단) 8 · 줄마다 엇갈린 널 6. V 가 처마 → 용마루. 널 아래 끝이 위로 겹쳐 그늘이 진다"""
    rows, cols = 8, 6
    y = np.arange(N)[:, None] / N * rows
    x = np.arange(N)[None, :] / N * cols
    row = np.floor(y)
    fy = y - row                                   # 0 = 줄 위(용마루 쪽 · 그림 위), 1 = 줄 아래(처마 쪽)
    shift = (row % 2) * 0.5 + 0.13 * np.sin(row * 2.4)
    fx = (x + shift) % 1.0
    col_id = (np.floor(x + shift) % cols + row * cols).astype(int)
    ids = rng.random(rows * cols * 2)
    per = ids[col_id % len(ids)]
    gap = np.clip(1.0 - np.minimum(fx, 1 - fx) / 0.03, 0, 1)             # 널 사이 틈
    lip = np.clip((fy - 0.86) / 0.14, 0, 1)                                # 아래 끝(두께 그늘)
    slope = fy * 0.8 + per * 0.25                                          # 널마다 비스듬히 겹침
    cleave = aniso(N, 1.8, 11, stretch_x=1.0, stretch_y=0.2)               # 쪼개진 결(세로)
    fine = fbm(N, 1.2, 12, lo=30)
    height = np.clip(slope - 0.55 * gap - 0.0 * lip + 0.12 * (cleave - 0.5) + 0.06 * (fine - 0.5), 0, None)
    height = (height - height.min()) / (height.max() - height.min())
    value = 0.62 + 0.16 * (per - 0.5) + 0.07 * (cleave - 0.5) + 0.04 * (fine - 0.5) - 0.2 * gap + 0.05 * lip
    bc = grey(value, -0.01)
    ao = np.clip(ao_from_height(height, 10, 2.5) - 0.25 * gap, 0, 1)
    rough = np.clip(0.52 + 0.18 * (per - 0.5) + 0.1 * (fine - 0.5) + 0.2 * gap, 0, 1)
    save("Slate", bc, normal_from_height(height, 22.0), np.stack([ao, rough, np.zeros_like(ao)], -1))


def wood():
    """참나무 널: 결은 V(세로) 방향, 널 4 장(가로로 나뉨). 긴 결 · 옹이 약간"""
    planks = 4
    x = np.arange(N)[None, :] / N * planks
    plank = np.floor(x).astype(int)
    fx = x - plank
    per = rng.random(planks)[plank % planks] * np.ones((N, 1))
    grain = aniso(N, 3.0, 21, stretch_x=1.0, stretch_y=25.0)     # 세로(V)로 길게 이어지는 결 — y 주파수를 누른다
    grain2 = aniso(N, 1.6, 22, stretch_x=1.0, stretch_y=12.0)
    rings = 0.5 + 0.5 * np.sin((fx * 2.5 + grain * 7.0 + per * 5.0) * 2 * np.pi)
    gap = np.clip(1.0 - np.minimum(fx, 1 - fx) / 0.018, 0, 1) * np.ones((N, 1))
    fine = fbm(N, 1.0, 23, lo=50)
    height = 0.5 * rings * 0.6 + 0.3 * grain2 + 0.1 * fine - 0.9 * gap
    height = (height - height.min()) / (height.max() - height.min())
    value = 0.50 + 0.16 * (rings - 0.5) + 0.10 * (per - 0.5) + 0.14 * (grain2 - 0.5) - 0.25 * gap
    bc = np.stack([value * 1.10, value * 0.93, value * 0.74], -1)
    ao = np.clip(ao_from_height(height, 5, 2.0) - 0.3 * gap, 0, 1)
    rough = np.clip(0.66 + 0.12 * (rings - 0.5) + 0.08 * (fine - 0.5) + 0.15 * gap, 0, 1)
    save("Wood", bc, normal_from_height(height, 7.0), np.stack([ao, rough, np.zeros_like(ao)], -1))


def metal():
    """놋쇠 · 쇠: 두드린 얕은 굴곡 · 잔 긁힘 · 오목한 곳 녹청 자리. 금속 = 1"""
    dents = fbm(N, 2.4, 31, lo=3, hi=24)
    scratch = aniso(N, 0.8, 32, stretch_x=0.03, stretch_y=1.0) * 0.6 + aniso(N, 0.8, 33, stretch_x=1.0, stretch_y=0.05) * 0.4
    fine = fbm(N, 0.8, 34, lo=60)
    patina = np.clip((fbm(N, 1.8, 35, lo=4, hi=40) - 0.6) * 3.0, 0, 1)
    height = 0.7 * dents + 0.2 * scratch + 0.1 * fine
    value = 0.80 + 0.06 * (dents - 0.5) + 0.05 * (scratch - 0.5) - 0.10 * patina
    bc = np.stack([value * 1.02, value * 0.98, value * (0.92 + 0.05 * patina)], -1)
    ao = ao_from_height(height, 12, 1.2)
    rough = np.clip(0.34 + 0.22 * (scratch - 0.5) + 0.10 * (fine - 0.5) + 0.35 * patina, 0.08, 1)
    metal_mask = np.clip(1.0 - 0.5 * patina, 0, 1)
    save("Metal", bc, normal_from_height(height, 5.0), np.stack([ao, rough, metal_mask], -1))


def cloth():
    """직물(배너 · 깔개): 씨줄 날줄 짜임 · 잔 보풀. 색은 정점 색 · Tint 가 정한다"""
    threads = 96
    y = np.arange(N)[:, None] / N * threads * 2 * np.pi
    x = np.arange(N)[None, :] / N * threads * 2 * np.pi
    weave = 0.5 + 0.25 * np.sin(x) * np.cos(y * 0.5) + 0.25 * np.sin(y) * np.cos(x * 0.5)
    fuzz = fbm(N, 1.0, 41, lo=40)
    folds = fbm(N, 2.8, 42, hi=6)
    height = 0.6 * weave + 0.2 * fuzz + 0.2 * folds
    value = 0.72 + 0.08 * (weave - 0.5) + 0.05 * (fuzz - 0.5) + 0.06 * (folds - 0.5)
    ao = ao_from_height(height, 3, 1.0)
    rough = np.clip(0.88 + 0.06 * (fuzz - 0.5), 0, 1)
    save("Cloth", grey(value), normal_from_height(height, 2.5), np.stack([ao, rough, np.zeros_like(ao)], -1))


def leaf():
    """잎 · 꽃잎(정점 색으로 칠하는 작은 면): 거의 고른 밝기 + 잔 얼룩. 노멀은 거의 평평"""
    mott = fbm(N, 2.0, 51, lo=4, hi=40)
    fine = fbm(N, 1.0, 52, lo=40)
    value = 0.86 + 0.10 * (mott - 0.5) + 0.04 * (fine - 0.5)
    height = 0.7 * mott + 0.3 * fine
    rough = np.clip(0.55 + 0.1 * (mott - 0.5), 0, 1)
    save("Leaf", grey(value), normal_from_height(height, 1.5), np.stack([np.ones_like(value), rough, np.zeros_like(value)], -1))


def macro():
    """큰 무늬(월드 좌표로 깐다): R = 넓은 얼룩, G = 중간 얼룩, B = 이끼 · 때 자리. 한 장(_Mask)"""
    r = fbm(N, 2.6, 61, hi=12)
    g = fbm(N, 2.0, 62, lo=4, hi=48)
    b = np.clip((fbm(N, 2.3, 63, lo=2, hi=30) - 0.42) * 2.6, 0, 1)
    os.makedirs(OUT, exist_ok=True)
    Image.fromarray((np.stack([r, g, b], -1) * 255 + 0.5).astype(np.uint8), "RGB").save(os.path.join(OUT, "T_GrimEnv_Macro_Mask.png"))
    print("  Macro: 큰 무늬 마스크")


def banner():
    """남색 배너(서클 고리 문양 · 금빛 테두리) — 타일이 아니다. 512 × 1024, 색 한 장(_BC). 아래 끝은 제비 꼬리(모델이 자른다)"""
    w, h = 512, 1024
    s = 4
    img = Image.new("RGB", (w * s, h * s), (24, 30, 74))
    d = ImageDraw.Draw(img)
    gold, gold_dark = (212, 170, 88), (150, 112, 52)
    m = 26 * s
    d.rectangle([m, m, w * s - m, h * s - m], outline=gold, width=7 * s)
    d.rectangle([m + 14 * s, m + 14 * s, w * s - m - 14 * s, h * s - m - 14 * s], outline=gold_dark, width=2 * s)
    cx, cy = w * s // 2, int(h * s * 0.40)
    for radius, width, color in ((150, 10, gold), (112, 4, gold_dark), (78, 7, gold)):
        d.ellipse([cx - radius * s, cy - radius * s, cx + radius * s, cy + radius * s], outline=color, width=width * s)
    # 가운데 네 갈래 별(나침반) — 서클 고리 문양
    for dx, dy, length in ((0, -1, 132), (0, 1, 132), (-1, 0, 96), (1, 0, 96)):
        tip = (cx + dx * length * s, cy + dy * length * s)
        side = 13 * s
        d.polygon([tip, (cx - dy * side, cy - dx * side), (cx + dy * side, cy + dx * side)], fill=gold)
    d.ellipse([cx - 17 * s, cy - 17 * s, cx + 17 * s, cy + 17 * s], fill=(24, 30, 74), outline=gold, width=4 * s)
    # 고리 위 작은 구슬 여섯(속성)
    for i in range(6):
        a = np.pi / 6 + i * np.pi / 3
        px, py = cx + np.cos(a) * 150 * s, cy + np.sin(a) * 150 * s
        d.ellipse([px - 11 * s, py - 11 * s, px + 11 * s, py + 11 * s], fill=gold)
    # 아래 줄무늬
    for i, yy in enumerate((0.70, 0.735, 0.77)):
        d.line([m + 40 * s, int(h * s * yy), w * s - m - 40 * s, int(h * s * yy)], fill=gold if i != 1 else gold_dark, width=(5 if i != 1 else 2) * s)
    img = img.resize((w, h), Image.LANCZOS).filter(ImageFilter.GaussianBlur(0.6))
    arr = np.asarray(img).astype(np.float32) / 255.0
    worn = fbm(1024, 2.0, 71, lo=3, hi=60)[:, :512]
    arr *= (0.86 + 0.22 * worn)[..., None]
    os.makedirs(OUT, exist_ok=True)
    Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8), "RGB").save(os.path.join(OUT, "T_GrimEnv_Banner_BC.png"))
    print("  Banner: 남색 배너 색")


def rug():
    """깔개(남색 바탕 · 금빛 테 · 가운데 원 무늬) — 타일 아님, 1024 × 512, 색 한 장(_BC). 짜임은 Cloth 노멀이 준다"""
    w, h, s = 1024, 512, 2
    img = Image.new("RGB", (w * s, h * s), (30, 36, 82))
    d = ImageDraw.Draw(img)
    gold, cream, wine = (204, 160, 82), (214, 196, 160), (110, 34, 40)
    for k, (m, col, wd) in enumerate(((10, wine, 22), (40, gold, 8), (58, cream, 3), (80, gold, 3))):
        d.rectangle([m * s, m * s, (w - m) * s, (h - m) * s], outline=col, width=wd * s)
    for i in range(24):                                   # 테 안 마름모 띠
        x = (100 + i * 34) * s
        if x > (w - 100) * s:
            break
        for y in (24 * s, (h - 24) * s):
            d.polygon([(x, y - 9 * s), (x + 9 * s, y), (x, y + 9 * s), (x - 9 * s, y)], fill=gold)
    cx, cy = w * s // 2, h * s // 2
    for r, col, wd in ((150, gold, 6), (120, cream, 2), (70, gold, 5)):
        d.ellipse([cx - r * s, cy - r * s, cx + r * s, cy + r * s], outline=col, width=wd * s)
    for k in range(8):
        a = np.pi / 4 * k
        d.line([cx + np.cos(a) * 70 * s, cy + np.sin(a) * 70 * s, cx + np.cos(a) * 150 * s, cy + np.sin(a) * 150 * s], fill=gold, width=3 * s)
    img = img.resize((w, h), Image.LANCZOS)
    arr = np.asarray(img).astype(np.float32) / 255.0
    worn = fbm(1024, 2.0, 81, lo=3, hi=80)[:512, :]
    arr *= (0.84 + 0.24 * worn)[..., None]
    os.makedirs(OUT, exist_ok=True)
    Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8), "RGB").save(os.path.join(OUT, "T_GrimEnv_Rug_BC.png"))
    print("  Rug: 깔개 색")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    print(f"환경 텍스처 → {OUT} ({N}px)")
    stone()
    slate()
    wood()
    metal()
    cloth()
    leaf()
    macro()
    banner()
    rug()
