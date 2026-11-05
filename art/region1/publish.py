"""1번 지역 콘셉트 아트를 문서 사이트에 올린다 — raw/<job>.png → docs/public/art/region1/<job>.jpg + docs/art/region1-concepts.md

    python -X utf8 art/region1/publish.py

그림이 아직 없는 작업은 「생성 중」 으로 둔다. 문서 그림은 LFS 가 아니므로 JPG(가로 1536, 품질 85)로 줄인다.
"""
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
from make_prompts import JOBS  # noqa: E402

OUT_IMG = os.path.join(ROOT, "docs", "public", "art", "region1")
OUT_MD = os.path.join(ROOT, "docs", "art", "region1-concepts.md")

GROUPS = ["지역", "마법탑", "탑 실내", "랜드마크", "흥미 지점"]


def main():
    os.makedirs(OUT_IMG, exist_ok=True)
    done = 0
    lines = [
        "# 1번 지역 콘셉트 아트",
        "",
        "> 상태: **검토용 v1** · 2026-11-06 · GPT 이미지(Codex CLI). 아트 방향은 [Painterly High-Fidelity Stylized Realism](../spec/art-direction).",
        ">",
        "> 장소와 이름은 지형 레이아웃(`Scripts/Terrain/openbeta_layout.json` 의 랜드마크 · POI)과 탑 1층(`Scripts/Tutorial/tower_greybox.py`)과 같다. "
        "마법 생물은 그리지 않고 들어갈 자리(굴 · 둥지 · 빈 물가)만 비웠다. 던전(계곡 동굴)은 아직 그리지 않았다.",
        ">",
        "> 각 그림을 레벨로 만들 때 필요한 3D 모델 목록은 [1번 지역 — 필요한 3D 모델](./region1-assets).",
        "",
        "프롬프트는 `art/region1/prompts/`, 다시 만들기는 `art/region1/make_prompts.py` · `art/tools/run_batch.sh`.",
        "",
    ]
    for group in GROUPS:
        lines += [f"## {group}", ""]
        for job, g, title, body, comp, _ in JOBS:
            if g != group:
                continue
            src = os.path.join(HERE, "raw", job + ".png")
            lines += [f"### {title}", "", f"<small>`{job}` — {body}</small>", ""]
            if os.path.exists(src):
                image = Image.open(src).convert("RGB")
                if image.width > 1536:
                    image = image.resize((1536, round(image.height * 1536 / image.width)), Image.LANCZOS)
                image.save(os.path.join(OUT_IMG, job + ".jpg"), quality=85, optimize=True)
                lines += [f"![{title}](/art/region1/{job}.jpg)", ""]
                done += 1
            else:
                lines += ["_생성 중_", ""]
    with open(OUT_MD, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"올림 {done}/{len(JOBS)} → {OUT_MD}")


if __name__ == "__main__":
    main()
