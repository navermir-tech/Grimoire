# 마법 이펙트용 회색조 텍스처를 절차적으로 만든다(파이썬 3 + numpy + Pillow, 언리얼 밖에서).
# 밝기 = 알파(가산 · 반투명 둘 다 R 채널만 쓴다). 색은 머티리얼 · 파티클 색이 입힌다.
#
#   python Scripts/MagicFX/make_fx_textures.py          → art/fx/textures/T_FX_*.png
#
# 언리얼로 들여오기는 make_magicfx_assets.py 의 textures 단계(/Game/Grimoire/MagicFX/Textures).
# 손그림 느낌이 필요한 것(불꽃 혀 · 연기 덩이)은 GPT 시트에서 잘라 쓴다 — art/fx/README.md.

import math
import os

import numpy as np
from PIL import Image

OUT = os.path.join(os.path.dirname(__file__), "..", "..", "art", "fx", "textures")
RNG = np.random.default_rng(7)


def save(name, arr):
    arr = np.clip(arr, 0.0, 1.0)
    img = Image.fromarray((arr * 255.0 + 0.5).astype(np.uint8), mode="L")
    os.makedirs(OUT, exist_ok=True)
    img.save(os.path.join(OUT, name + ".png"))
    print("텍스처", name, img.size)


def grid(n):
    """-1..1 좌표 격자."""
    v = (np.arange(n) + 0.5) / n * 2.0 - 1.0
    return np.meshgrid(v, v)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def tile_noise(n, cells, seed):
    """이어 붙여도 끊기지 않는 값 노이즈(격자 cells, 3차 보간)."""
    rng = np.random.default_rng(seed)
    lattice = rng.random((cells, cells))
    x = np.arange(n) / n * cells
    i0 = np.floor(x).astype(int)
    f = x - i0
    f = f * f * (3 - 2 * f)
    i1 = (i0 + 1) % cells
    a = lattice[np.ix_(i0, i0)]
    b = lattice[np.ix_(i0, i1)]
    c = lattice[np.ix_(i1, i0)]
    d = lattice[np.ix_(i1, i1)]
    fx = f[None, :]
    fy = f[:, None]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fbm(n, base_cells, octaves, seed):
    total = np.zeros((n, n))
    amp = 1.0
    norm = 0.0
    for o in range(octaves):
        total += amp * tile_noise(n, base_cells * (2 ** o), seed + o)
        norm += amp
        amp *= 0.5
    return total / norm


def soft_glow(n=256):
    x, y = grid(n)
    r = np.sqrt(x * x + y * y)
    core = np.exp(-(r / 0.18) ** 2)
    halo = np.exp(-(r / 0.5) ** 2) * 0.45
    return (core + halo) * smoothstep(1.0, 0.85, r)


def flare(n=256, spikes=4):
    """별 섬광: 가는 빛살 + 밝은 심."""
    x, y = grid(n)
    r = np.sqrt(x * x + y * y) + 1e-6
    ang = np.arctan2(y, x)
    rays = np.abs(np.cos(ang * spikes / 2.0)) ** 60
    rays2 = np.abs(np.cos(ang * spikes / 2.0 + math.pi / spikes)) ** 90 * 0.45
    falloff = np.exp(-r * 3.2)
    core = np.exp(-(r / 0.09) ** 2)
    return np.clip((rays + rays2) * falloff + core + np.exp(-(r / 0.35) ** 2) * 0.25, 0, 1) * smoothstep(1.0, 0.8, r)


def ring(n=256, radius=0.78, width=0.07):
    x, y = grid(n)
    r = np.sqrt(x * x + y * y)
    band = np.exp(-((r - radius) / width) ** 2)
    inner = np.exp(-((r - radius) / (width * 3.5)) ** 2) * 0.25 * (r < radius)
    return band + inner


def spark(n=128):
    """속도 방향으로 늘이는 불티 · 빛 알갱이(가로로 긴 타원 + 밝은 심)."""
    x, y = grid(n)
    e = np.sqrt((x / 0.85) ** 2 + (y / 0.16) ** 2)
    return np.exp(-e * e * 2.5) + np.exp(-(np.sqrt(x * x + y * y) / 0.12) ** 2)


def noise(n=256):
    v = fbm(n, 4, 5, 11)
    return (v - v.min()) / (v.max() - v.min())


def ribbon(n=256):
    """리본 꼬리: V(세로)는 가운데가 밝은 띠, U(가로)는 결 무늬 — 가로로 흘려 쓴다."""
    x, y = grid(n)
    streak = fbm(n, 8, 3, 23)
    streak = np.repeat(streak[n // 2:n // 2 + 1, :], n, axis=0)  # 세로 결
    across = np.exp(-(y / 0.42) ** 2)
    core = np.exp(-(y / 0.12) ** 2)
    return across * (0.55 + 0.45 * streak) + core * 0.6


def flipbook(n_frame, cols, frame_fn):
    rows = cols
    out = np.zeros((n_frame * rows, n_frame * cols))
    for i in range(rows * cols):
        t = i / (rows * cols - 1)
        r, c = divmod(i, cols)
        out[r * n_frame:(r + 1) * n_frame, c * n_frame:(c + 1) * n_frame] = frame_fn(t, i)
    return out


def fire_frame_factory(n):
    """불꽃 덩이 한 장: 노이즈로 깎인 둥근 덩어리가 t 에 따라 커지며 침식된다(불꽃 혀가 위로 늘어짐)."""
    x, y = grid(n)
    base = fbm(n, 4, 4, 31)
    base2 = fbm(n, 8, 3, 37)

    def frame(t, i):
        shift = int(t * n * 0.35)
        nz = np.roll(base, shift, axis=0) * 0.65 + np.roll(base2, shift * 2, axis=0) * 0.35
        yy = y + 0.25 * t  # 위로 들림
        r = np.sqrt((x / (0.55 + 0.25 * t)) ** 2 + (np.where(yy < 0, yy / (0.75 + 0.5 * t), yy / 0.5)) ** 2)
        shape = 1.0 - r
        erode = 0.15 + 0.75 * t
        v = smoothstep(erode - 0.25, erode + 0.25, shape + nz * 0.9 - 0.25)
        core = np.exp(-(r / (0.35 * (1 - t) + 0.02)) ** 2) * (1 - t)
        return np.clip(v * (1.0 - 0.5 * t) + core * 0.6, 0, 1) * smoothstep(1.0, 0.9, np.maximum(np.abs(x), np.abs(y)))
    return frame


def smoke_frame_factory(n):
    x, y = grid(n)
    base = fbm(n, 3, 5, 51)

    def frame(t, i):
        nz = np.roll(base, int(t * n * 0.2), axis=1)
        r = np.sqrt(x * x + y * y) / (0.45 + 0.4 * t)
        v = smoothstep(0.0, 0.5, (1.0 - r) + (nz - 0.5) * 0.9)
        return v * (1.0 - t) ** 1.3 * smoothstep(1.0, 0.9, np.maximum(np.abs(x), np.abs(y)))
    return frame


def main():
    save("T_FX_SoftGlow", soft_glow())
    save("T_FX_Flare4", flare(spikes=4))
    save("T_FX_Flare6", flare(spikes=6))
    save("T_FX_Ring", ring())
    save("T_FX_Spark", spark())
    save("T_FX_Noise", noise())
    save("T_FX_RibbonStreak", ribbon())
    save("T_FX_FireFlipbook_4x4", flipbook(128, 4, fire_frame_factory(128)))
    save("T_FX_SmokeFlipbook_4x4", flipbook(128, 4, smoke_frame_factory(128)))


if __name__ == "__main__":
    main()
