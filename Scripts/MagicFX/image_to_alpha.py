# 검은 배경 위에 그린 이펙트 그림(GPT 등)을 알파 텍스처로 바꾼다 — 파이썬 3 + numpy + Pillow, 언리얼 밖에서.
#
#   python Scripts/MagicFX/image_to_alpha.py mask  <그림> <출력.png> [--size 256] [--black 0.04] [--gamma 1.0] [--pad 0.06]
#       회색조 마스크(밝기 = 알파). 가산 · 침식 머티리얼(fx_materials)이 R 채널만 쓴다. 색은 파티클 색이 입힌다.
#   python Scripts/MagicFX/image_to_alpha.py rgba  <그림> <출력.png> [...]
#       색을 살린 RGBA — 검은 배경을 뺀 「곱해지지 않은」 색 + 알파(검은 테두리 번짐 없음). 반투명 머티리얼용.
#   python Scripts/MagicFX/image_to_alpha.py sheet <그림> <출력.png> --grid 2x2 [--cell 0] [...]
#       콘셉트 시트처럼 칸으로 나뉜 그림에서 칸 하나(--cell, 왼쪽 위부터 0)를 잘라 mask 로.
#   python Scripts/MagicFX/image_to_alpha.py flipbook <출력.png> <그림1> <그림2> … [--cols 4] [--size 128] [--mode mask|rgba]
#       낱장 여러 장을 같은 크기 칸으로 맞춰 플립북 한 장으로(칸 수가 모자라면 마지막 장을 되풀이).
#       GPT 는 칸이 정확한 플립북 시트를 잘 못 그린다 — 「검은 배경 위 불꽃 혀 하나」 같은 낱장을 여럿 뽑아 여기서 묶는다.
#
# 공통: 물체를 찾아(밝기 > --black) 가운데로 옮기고 여백 --pad 을 두어 정사각형으로 맞춘 뒤 --size 로 줄인다.
# 가장자리 칸 선 · 숫자 같은 잔여물은 --black 을 올리거나 sheet 의 --inset 으로 칸 테두리를 깎는다.

import argparse
import os

import numpy as np
from PIL import Image


def load_linear(path):
    """sRGB 그림 → 0..1 실수 RGB."""
    a = np.asarray(Image.open(path).convert("RGB"), dtype=np.float32) / 255.0
    return a


def luminance(rgb):
    return rgb[..., 0] * 0.2126 + rgb[..., 1] * 0.7152 + rgb[..., 2] * 0.0722


def background_level(rgb):
    """배경 밝기 — 테두리 픽셀의 중앙값(완전 검정이 아닌 어두운 청회색 배경도 뺀다)."""
    lum = luminance(rgb)
    edge = np.concatenate([lum[0], lum[-1], lum[:, 0], lum[:, -1]])
    return float(np.median(edge))


def to_alpha(rgb, black, gamma):
    bg = background_level(rgb)
    lum = luminance(rgb)
    alpha = np.clip((lum - bg - black) / max(1e-4, 1.0 - bg - black), 0.0, 1.0)
    return alpha ** gamma, bg


def fit_square(arr, alpha, pad, size, threshold=0.02):
    """물체 둘레 상자를 찾아 정사각형으로 · 여백 · 크기."""
    ys, xs = np.nonzero(alpha > threshold)
    if len(xs) == 0:
        ys, xs = np.array([0, alpha.shape[0] - 1]), np.array([0, alpha.shape[1] - 1])
    cx, cy = (xs.min() + xs.max()) / 2.0, (ys.min() + ys.max()) / 2.0
    half = max(xs.max() - xs.min(), ys.max() - ys.min()) / 2.0 * (1.0 + 2.0 * pad) + 1
    x0, y0 = int(round(cx - half)), int(round(cy - half))
    side = int(round(half * 2))
    canvas_shape = (side, side) + arr.shape[2:]
    out = np.zeros(canvas_shape, dtype=np.float32)
    sx0, sy0 = max(0, x0), max(0, y0)
    sx1, sy1 = min(arr.shape[1], x0 + side), min(arr.shape[0], y0 + side)
    out[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0] = arr[sy0:sy1, sx0:sx1]
    return resize(out, size)


def resize(arr, size):
    if arr.ndim == 2:
        img = Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8), "L")
    else:
        img = Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8), "RGBA")
    return np.asarray(img.resize((size, size), Image.LANCZOS), dtype=np.float32) / 255.0


def make_mask(rgb, a):
    alpha, _ = to_alpha(rgb, a.black, a.gamma)
    return fit_square(alpha, alpha, a.pad, a.size)


def make_rgba(rgb, a):
    alpha, bg = to_alpha(rgb, a.black, a.gamma)
    # 배경색을 뺀 뒤 알파로 나눠 「곱해지지 않은」 색을 되살린다: 화면 = 색 × 알파 + 배경 × (1 − 알파).
    bg_rgb = np.median(np.concatenate([rgb[0], rgb[-1], rgb[:, 0], rgb[:, -1]]), axis=0)
    color = np.clip((rgb - bg_rgb * (1.0 - alpha[..., None])) / np.maximum(alpha[..., None], 1e-3), 0.0, 1.0)
    rgba = np.concatenate([color, alpha[..., None]], axis=-1)
    return fit_square(rgba, alpha, a.pad, a.size)


def save(arr, path):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    mode = "L" if arr.ndim == 2 else "RGBA"
    Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8), mode).save(path)
    print("저장", path, arr.shape)


def main():
    p = argparse.ArgumentParser(description="검은 배경 그림 → 알파 텍스처")
    p.add_argument("command", choices=["mask", "rgba", "sheet", "flipbook"])
    p.add_argument("paths", nargs="+")
    p.add_argument("--size", type=int, default=256)
    p.add_argument("--black", type=float, default=0.04, help="배경 위 이만큼 밝아야 물체로 본다(0..1)")
    p.add_argument("--gamma", type=float, default=1.0, help="알파 곡선(>1 이면 가장자리가 더 투명)")
    p.add_argument("--pad", type=float, default=0.06, help="물체 둘레 여백 비율")
    p.add_argument("--grid", default="2x2", help="sheet: 열x행")
    p.add_argument("--cell", type=int, default=0, help="sheet: 칸 번호(왼쪽 위 0, 가로 먼저)")
    p.add_argument("--inset", type=float, default=0.03, help="sheet: 칸 테두리 깎는 비율(칸 선 제거)")
    p.add_argument("--label", type=float, default=0.14, help="sheet: 왼쪽 위 칸 번호 자리를 이 비율만큼 지운다(0 = 끔)")
    p.add_argument("--cols", type=int, default=4, help="flipbook: 가로 칸 수(세로도 같다)")
    p.add_argument("--mode", choices=["mask", "rgba"], default="mask", help="flipbook: 칸 형식")
    a = p.parse_args()

    if a.command in ("mask", "rgba"):
        src, out = a.paths[0], a.paths[1]
        rgb = load_linear(src)
        save(make_mask(rgb, a) if a.command == "mask" else make_rgba(rgb, a), out)
    elif a.command == "sheet":
        src, out = a.paths[0], a.paths[1]
        rgb = load_linear(src)
        cols, rows = (int(v) for v in a.grid.lower().split("x"))
        h, w = rgb.shape[0] // rows, rgb.shape[1] // cols
        r, c = divmod(a.cell, cols)
        ix, iy = int(w * a.inset), int(h * a.inset)
        cell = rgb[r * h + iy:(r + 1) * h - iy, c * w + ix:(c + 1) * w - ix].copy()
        if a.label > 0:
            # 번호 자리를 배경색으로 칠한다(콘셉트 시트는 칸마다 왼쪽 위에 번호가 있다).
            lh, lw = int(cell.shape[0] * a.label), int(cell.shape[1] * a.label)
            cell[:lh, :lw] = np.median(cell[-4:, :], axis=(0, 1))
        save(make_mask(cell, a), out)
    else:
        out, sources = a.paths[0], a.paths[1:]
        n = a.cols * a.cols
        frames = [make_mask(load_linear(s), a) if a.mode == "mask" else make_rgba(load_linear(s), a) for s in sources]
        frames += [frames[-1]] * (n - len(frames))
        shape = (a.size * a.cols, a.size * a.cols) + frames[0].shape[2:]
        sheet = np.zeros(shape, dtype=np.float32)
        for i, f in enumerate(frames[:n]):
            r, c = divmod(i, a.cols)
            sheet[r * a.size:(r + 1) * a.size, c * a.size:(c + 1) * a.size] = f
        save(sheet, out)


if __name__ == "__main__":
    main()
