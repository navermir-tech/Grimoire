"""마스코트 콘셉트 시안을 문서 사이트에 올린다 — raw/<id>.png → docs/public/art/mascot/<id>.jpg + 비교 그림 + docs/art/mascot-concepts.md

    python -X utf8 art/mascot/publish.py

비교 그림 mascot-compare.jpg 는 모든 시안을 3 × 2 로 모은 한 장(번호 · 이름 띠). 그림이 없는 시안은 「그림 없음」 칸.
문서 그림은 LFS 가 아니므로 JPG(가로 1536, 품질 85)로 줄인다. 방향의 정본은 make_prompts.py 의 DIRECTIONS.
"""
import html
import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
from make_prompts import DIRECTIONS  # noqa: E402

OUT_IMG = os.path.join(ROOT, "docs", "public", "art", "mascot")
OUT_MD = os.path.join(ROOT, "docs", "art", "mascot-concepts.md")
WIDTH, QUALITY = 1536, 85
FONT = "C:/Windows/Fonts/malgunbd.ttf"


def esc(text):
    return html.escape(text, quote=False).replace("{{", "&#123;&#123;")


def publish_image(job):
    src = os.path.join(HERE, "raw", job + ".png")
    if not os.path.exists(src):
        return False
    image = Image.open(src).convert("RGB")
    if image.width > WIDTH:
        image = image.resize((WIDTH, round(image.height * WIDTH / image.width)), Image.LANCZOS)
    image.save(os.path.join(OUT_IMG, job + ".jpg"), quality=QUALITY, optimize=True)
    return True


def compare():
    """모든 시안을 한 장에 — 칸 768 × 512 + 위 이름 띠."""
    cw, ch, band, cols = 768, 512, 64, 3
    rows = (len(DIRECTIONS) + cols - 1) // cols
    sheet = Image.new("RGB", (cw * cols, (ch + band) * rows), (236, 232, 226))
    draw = ImageDraw.Draw(sheet)
    big = ImageFont.truetype(FONT, 28)
    small = ImageFont.truetype(FONT.replace("malgunbd", "malgun"), 18)
    for i, d in enumerate(DIRECTIONS):
        x, y = (i % cols) * cw, (i // cols) * (ch + band)
        draw.rectangle([x, y, x + cw, y + band], fill=(38, 34, 52))
        draw.text((x + 16, y + 6), f"{i + 1}. {d['title']} · {d['names'][0]}", font=big, fill=(245, 230, 190))
        draw.text((x + 16, y + 38), d["concept"], font=small, fill=(200, 196, 214))
        src = os.path.join(HERE, "raw", d["id"] + ".png")
        if os.path.exists(src):
            im = Image.open(src).convert("RGB")
            im.thumbnail((cw, ch), Image.LANCZOS)
            sheet.paste(im, (x + (cw - im.width) // 2, y + band + (ch - im.height) // 2))
        else:
            draw.text((x + cw // 2 - 50, y + band + ch // 2), "그림 없음", font=big, fill=(140, 140, 140))
        draw.rectangle([x, y, x + cw - 1, y + band + ch - 1], outline=(255, 255, 255), width=2)
    sheet.save(os.path.join(OUT_IMG, "mascot-compare.jpg"), quality=QUALITY, optimize=True)


def shot(job, alt):
    # img src 에는 base(/Grimoire/)가 저절로 붙지만 a href 에는 안 붙는다 — withBase 로 붙인다.
    url = f"/art/mascot/{job}.jpg"
    return f"<a :href=\"withBase('{url}')\" target=\"_blank\"><img src=\"{url}\" alt=\"{alt}\" loading=\"lazy\"></a>"


def swatches(palette):
    return " ".join(
        f'<span class="sw"><i style="background:{h}"></i>{esc(n)} <code>{h}</code></span>' for n, h in palette
    )


STYLE = """<style>
.mascot-shot img { display: block; width: 100%; height: auto; border-radius: 8px; border: 1px solid var(--vp-c-divider); }
.sw { display: inline-flex; align-items: center; gap: 4px; margin-right: 12px; font-size: 13px; }
.sw i { display: inline-block; width: 14px; height: 14px; border-radius: 3px; border: 1px solid var(--vp-c-divider); }
</style>"""


def main():
    os.makedirs(OUT_IMG, exist_ok=True)
    have = {d["id"]: publish_image(d["id"]) for d in DIRECTIONS}
    compare()
    L = [
        "---",
        "aside: false",
        "---",
        "",
        "# 마스코트 콘셉트 시안",
        "",
        "> 상태: **검토용 시안 6개** · 2026-11-19 · GPT 이미지(Codex CLI). 사용자 요청 「팰월드의 까부냥, 포켓몬의 피카츄처럼 엄청 귀엽고 게임 얼굴이 될 오리지널 마법 생물 — **오픈 베타에 꼭 넣는다**」.",
        ">",
        "> 고를 것은 [미결 Q215~Q219](../spec/decisions#미결-질문)(어느 시안 · 이름 · 오픈 베타 역할 · 성장 · 쓰임새). 고른 뒤에 [지역별 생물 명단](../spec/creature-roster)에 넣는다.",
        "",
        "<script setup>",
        "import { withBase } from 'vitepress'  // 그림 링크(href)는 base(/Grimoire/)가 저절로 붙지 않는다",
        "</script>",
        "",
        STYLE,
        "",
        "## 한눈에",
        "",
        '<div class="mascot-shot">' + shot("mascot-compare", "마스코트 시안 6개 비교") + "</div>",
        "",
        "| # | 시안 | 한 줄 | 이름 후보 | 게임 연결 | 오픈 베타 역할(제안) |",
        "|---|---|---|---|---|---|",
    ]
    for i, d in enumerate(DIRECTIONS, 1):
        L.append(f"| {i} | [**{d['title']}**](#{d['id']}) | {d['concept']} | {' · '.join(d['names'])} | {d['tie']} | {d['role'].split(' — ')[0]} |")
    L += [
        "",
        "## 디자인 원칙",
        "",
        "생물 콘셉트의 「킥」 규칙([마법 생물 콘셉트](./creature-concepts))을 그대로 따르되, 마스코트라 **더 귀엽고 단순하게** 했다. 생물 콘셉트는 「포켓몬 · 팰월드 같은 마스코트풍」을 피하지만, 마스코트는 바로 그 자리를 맡는다.",
        "",
        "| 원칙 | 뜻 |",
        "|---|---|",
        "| 실루엣 | 엄지 크기 아이콘 · 인형 · 스티커 · 로고로 줄여도 알아본다. 큰 모양 2~3개 · 주색 2~3개 · 잔무늬 없음 |",
        "| 비율 · 표정 | 머리 : 몸 ≈ 1 : 1, 짧고 통통한 팔다리, 아주 큰 눈. 시안마다 표정을 돕는 부위(책장 귀 · 깃펜 아가미 · 더듬이 · 등불 · 비늘 · 눈자루)가 있다 |",
        "| 마법 생물다움 | 몸 일부가 마법 물질(책장 · 잉크 펜촉 · 마법진 날개 · 등불 유리 · 마나 수정 · 탑)이고, 큰 그림은 마법을 쓰는 순간 |",
        "| 구현 가능성 | 몸은 메시 · 재질로 만드는 또렷한 덩어리, 흩어지는 것(빛가루 · 불티 · 글자 가루)은 FX — FX 를 지워도 실루엣이 선다 |",
        "| 화풍 | [아트 방향](../spec/art-direction)의 가장 귀여운 끝 — 3D 스타일라이즈드 캐릭터 렌더 마감 · 사실적 PBR 재질 · 회화적 빛. 2D 셀 애니 · 치비 · 플라스틱 장난감 광택은 피한다 |",
        "| 독창성 | 피카츄 · 이브이 · 까부냥 · 램볼 · 커비 · 토토로 등과 실루엣 · 색 조합 · 귀 모양이 겹치지 않게 — 시안마다 「닮을 위험」을 적고 프롬프트에서 피했다 |",
        "",
        "시트 읽는 법: 왼쪽 큰 그림 = 마법 쓰는 순간(3/4), 오른쪽 위 = 정면 · 옆, 가운데 = 표정 넷, 아래 왼쪽 = 키 1.8 m 사람 실루엣과 크기, 아래 오른쪽 = 색 변이. 그림을 누르면 크게 열린다.",
        "",
    ]
    for i, d in enumerate(DIRECTIONS, 1):
        L += [f"## {i}. {d['title']} {{#{d['id']}}}", "", f"**{d['concept']}.**", ""]
        if have[d["id"]]:
            L += ['<div class="mascot-shot">' + shot(d["id"], esc(d["title"])) + "</div>", ""]
        else:
            L += ["*(그림 없음)*", ""]
        L += [
            "| | |",
            "|---|---|",
            f"| 이름 후보 | {' · '.join(d['names'])} |",
            f"| 오픈 베타 역할(제안) | {d['role']} |",
            f"| 게임 연결 | {d['tie']} |",
            f"| 크기 | {d['size']} |",
            f"| 주색 | {swatches(d['palette'])} |",
            f"| 마법 부위(메시 · 재질) | {d['f']} |",
            f"| 마법 쓰는 순간 | {d['m']} |",
            f"| FX | {d['fx']} |",
            f"| 표정 | {d['face']} |",
            f"| 색 변이 | {d['var']} |",
            f"| **닮을 위험** | {d['risk']} |",
            "",
        ]
        if d.get("review"):
            L += [f"> **그림을 보고**: {d['review']}", ""]
    L += [
        "## 만든 과정",
        "",
        "- 방향 · 프롬프트의 정본 `art/mascot/make_prompts.py`(`DIRECTIONS` · 공통 `RULES` · `STYLE` · `LAYOUT`), 프롬프트 `art/mascot/prompts/`, 이 페이지 · JPG · 비교 그림은 `art/mascot/publish.py`. 원본 PNG `art/mascot/raw/` 는 git 에 올리지 않는다(생물 콘셉트와 같다).",
        "- 그리기는 생물 콘셉트와 같은 `art/tools/run_codex_job.sh` — 그림 하나에 codex 세션 하나, 자기 세션 폴더의 그림인지 비교한다. 같은 파이프라인을 쓰는 다른 세션과 섞이지 않게 한 번에 2장까지만 돌렸다.",
        "",
    ]
    with open(OUT_MD, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L))
    print(f"그림 {sum(have.values())}/{len(DIRECTIONS)} → {OUT_MD}")


if __name__ == "__main__":
    main()
