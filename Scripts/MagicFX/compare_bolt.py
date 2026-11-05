# 볼트 시안(GPT, art/fx/raw/sheet_<속성>.png 세 줄: 시전 · 비행 · 적중)과 게임 화면(capture_bolt.ps1 결과)을 나란히 붙인다.
#   python Scripts/MagicFX/compare_bolt.py <시안 png> <캡처 폴더> <시전 장> <비행 장> <적중 장> <출력 jpg> [제목]
import sys

from PIL import Image, ImageDraw, ImageFont

W = 900          # 한 칸 폭
CROP = (150, 170, 1150, 470)  # 1280x720 옆 카메라 화면에서 완드 ~ 표적 띠


def font(size):
    try:
        return ImageFont.truetype("C:/Windows/Fonts/malgunbd.ttf", size)
    except OSError:
        return ImageFont.load_default()


def main():
    sheet, shots, cast, mid, hit, out = sys.argv[1:7]
    title = sys.argv[7] if len(sys.argv) > 7 else ""
    concept = Image.open(sheet).convert("RGB")
    rows = [concept.crop((0, concept.height * i // 3, concept.width, concept.height * (i + 1) // 3)) for i in range(3)]
    game = [Image.open("%s/shot_%03d.png" % (shots, int(n))).convert("RGB").crop(CROP) for n in (cast, mid, hit)]
    h = int(W * rows[0].height / rows[0].width)
    gh = int(W * game[0].height / game[0].width)
    rh = max(h, gh)
    top = 56
    canvas = Image.new("RGB", (W * 2 + 30, top + rh * 3 + 20), (16, 18, 24))
    d = ImageDraw.Draw(canvas)
    d.text((10, 12), title, fill=(235, 235, 235), font=font(26))
    d.text((W * 2 - 230, 16), "왼쪽 시안 · 오른쪽 게임", fill=(170, 170, 170), font=font(20))
    for i, label in enumerate(("시전", "비행", "적중")):
        y = top + i * rh
        canvas.paste(rows[i].resize((W, h)), (10, y))
        canvas.paste(game[i].resize((W, gh)), (W + 20, y))
        d.text((18, y + 8), label, fill=(255, 255, 255), font=font(22))
    canvas.save(out, quality=88)
    print(out)


if __name__ == "__main__":
    main()
