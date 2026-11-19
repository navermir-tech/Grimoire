"""마스코트 후보를 문서 사이트에 올린다 — raw/<id>.png → docs/public/art/mascot/<id>.jpg + 비교 그림 + docs/art/mascot-concepts.md

    python -X utf8 art/mascot/publish.py

v2(지금): make_prompts.py 의 CANDIDATES(그린 후보) · REJECTED(쳐낸 후보). 비교 그림 mascot-compare.jpg(4 × 2).
v1(반려, 2026-11-19): make_prompts_v1.py 의 DIRECTIONS — 페이지 아래 「옛 판(반려)」 접힘, 비교 그림 mascot-compare-v1.jpg.
문서 그림은 LFS 가 아니므로 JPG(가로 1536, 품질 85)로 줄인다.
"""
import html
import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
from make_prompts import CANDIDATES, REJECTED  # noqa: E402
from make_prompts_v1 import DIRECTIONS as V1  # noqa: E402

OUT_IMG = os.path.join(ROOT, "docs", "public", "art", "mascot")
OUT_MD = os.path.join(ROOT, "docs", "art", "mascot-concepts.md")
WIDTH, QUALITY = 1536, 85
FONT = "C:/Windows/Fonts/malgunbd.ttf"


def esc(text):
    return html.escape(text, quote=False).replace("{{", "&#123;&#123;")


def cell(text):
    # 표 칸 — 세로 막대를 막는다.
    return text.replace("|", "\\|")


def publish_image(job):
    src = os.path.join(HERE, "raw", job + ".png")
    if not os.path.exists(src):
        return False
    image = Image.open(src).convert("RGB")
    if image.width > WIDTH:
        image = image.resize((WIDTH, round(image.height * WIDTH / image.width)), Image.LANCZOS)
    image.save(os.path.join(OUT_IMG, job + ".jpg"), quality=QUALITY, optimize=True)
    return True


def compare(items, out, cols):
    """후보를 한 장에 — 칸 768 × 512 + 위 이름 띠. items = [(id, 제목, 한 줄)]."""
    cw, ch, band = 768, 512, 64
    rows = (len(items) + cols - 1) // cols
    sheet = Image.new("RGB", (cw * cols, (ch + band) * rows), (236, 232, 226))
    draw = ImageDraw.Draw(sheet)
    big = ImageFont.truetype(FONT, 28)
    small = ImageFont.truetype(FONT.replace("malgunbd", "malgun"), 18)
    for i, (job, title, line) in enumerate(items):
        x, y = (i % cols) * cw, (i // cols) * (ch + band)
        draw.rectangle([x, y, x + cw, y + band], fill=(38, 34, 52))
        draw.text((x + 16, y + 6), f"{i + 1}. {title}", font=big, fill=(245, 230, 190))
        draw.text((x + 16, y + 38), line, font=small, fill=(200, 196, 214))
        src = os.path.join(HERE, "raw", job + ".png")
        if os.path.exists(src):
            im = Image.open(src).convert("RGB")
            im.thumbnail((cw, ch), Image.LANCZOS)
            sheet.paste(im, (x + (cw - im.width) // 2, y + band + (ch - im.height) // 2))
        else:
            draw.text((x + cw // 2 - 50, y + band + ch // 2), "그림 없음", font=big, fill=(140, 140, 140))
        draw.rectangle([x, y, x + cw - 1, y + band + ch - 1], outline=(255, 255, 255), width=2)
    sheet.save(os.path.join(OUT_IMG, out), quality=QUALITY, optimize=True)


def short_hook(c):
    # 비교 그림 띠 — 고리의 따옴표 안 첫 문장.
    h = c["hook"].split("」")[0].lstrip("「")
    return h if len(h) <= 44 else h[:43] + "…"


def shot(job, alt):
    # img src 에는 base(/Grimoire/)가 저절로 붙지만 a href 에는 안 붙는다 — withBase 로 붙인다.
    url = f"/art/mascot/{job}.jpg"
    return f"<a :href=\"withBase('{url}')\" target=\"_blank\"><img src=\"{url}\" alt=\"{alt}\" loading=\"lazy\"></a>"


STYLE = """<style>
.mascot-shot img { display: block; width: 100%; height: auto; border-radius: 8px; border: 1px solid var(--vp-c-divider); }
.mascot-old summary { cursor: pointer; font-weight: 600; }
.mascot-old .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(260px, 1fr)); gap: 12px; margin-top: 12px; }
.mascot-old .grid img { display: block; width: 100%; height: auto; border-radius: 6px; }
.mascot-old .grid div { font-size: 13px; color: var(--vp-c-text-2); }
</style>"""


def main():
    os.makedirs(OUT_IMG, exist_ok=True)
    have = {c["id"]: publish_image(c["id"]) for c in CANDIDATES}
    compare([(c["id"], c["title"], short_hook(c)) for c in CANDIDATES], "mascot-compare.jpg", 4)
    L = [
        "---",
        "aside: false",
        "---",
        "",
        "# 마스코트 후보",
        "",
        "> 상태: **v2 후보 8개 — 검토용** · 2026-11-20 · GPT 이미지(Codex CLI). 사용자 요청(2026-11-19) 「팰월드 까부냥, 포켓몬 피카츄처럼 게임 얼굴이 될 오리지널 마법 생물 — **오픈 베타에 꼭 넣는다**」.",
        ">",
        "> v1 시안 6개(아래 「옛 판(반려)」)는 사용자가 「너무 틀에 박힌 느낌」으로 반려했다 — 여섯이 모두 「귀여운 실제 동물 + 붙인 게임 소품 + 인형 비율」 한 공식이었다. v2 는 **고리(행동 · 성격 · 플레이어와의 관계)부터** 33개를 글로 쓰고 기준으로 쳐내 8개를 그렸다. 사용자가 좋아한 카벙클 · 켈피 · 맨드레이크처럼 귀여움을 인형 비율이 아니라 마법 생물다움과 행동에서 끌어낸다.",
        ">",
        "> 고를 것은 [미결 Q215](../spec/decisions#q215)(어느 후보 — 하나 또는 몇 개를 더 다듬기). 고른 뒤에 4면 시트를 그리고 [지역별 생물 명단](../spec/creature-roster)에 넣는다.",
        "",
        "<script setup>",
        "import { withBase } from 'vitepress'  // 그림 링크(href)는 base(/Grimoire/)가 저절로 붙지 않는다",
        "</script>",
        "",
        STYLE,
        "",
        "## 한눈에",
        "",
        '<div class="mascot-shot">' + shot("mascot-compare", "마스코트 후보 8개 비교") + "</div>",
        "",
        "| # | 후보 | 고리 — 「걔 ○○한다?」 | 대비 | 그리모리와 닿는 곳 |",
        "|---|---|---|---|---|",
    ]
    for i, c in enumerate(CANDIDATES, 1):
        L.append(f"| {i} | [**{c['title']}**](#{c['id']}) | {cell(c['hook'])} | {cell(c['contrast'])} | {cell(c['tie'])} |")
    L += [
        "",
        "## 기준",
        "",
        "그리기 전에 글로 통과시켰다(2026-11-20 메인 세션 정리).",
        "",
        "| # | 기준 |",
        "|---|---|",
        "| 1 | 「실제 동물 + 붙인 마법 소품」 금지 — 마법이 꾸밈이 아니라 몸의 구조이거나 존재 이유 |",
        "| 2 | **한 줄 고리** — 플레이어가 「걔 ○○한다?」 하고 남에게 말할 행동 · 성격 · 플레이어와의 관계 |",
        "| 3 | **대비 하나** — 귀여운데 이상하다 · 작은데 위험하다 · 아름다운데 묘하다(공포 아님) · 하찮은 것이 마법적이다 |",
        "| 4 | 실루엣으로 알아보고, 피카츄 · 이브이 · 까부냥 · 램볼 · 커비 · 토토로 · 파이널판타지 카벙클 · 모그리 등과 겹치지 않는다 |",
        "| 5 | 그리모리 정체성과 닿는다 — 마법서 · 서클 · 마나 · 파크툼 계약 · 마법탑 · 생존 |",
        "| 6 | 구현 가능성(몸은 메시 · 재질, 흩어지는 것은 FX)과 [아트 방향](../spec/art-direction)(회화적 3D 스타일라이즈드) |",
        "",
        "그림은 시트가 아니라 **그 생물다운 순간 한 장**이다. 사람은 손 · 발 · 소매만 보인다(크기 비교). 그린 뒤 「캡처해서 남에게 보여 주고 싶은가 · 카벙클 · 켈피 옆에서 지지 않는가 · 또 동물 + 소품이 아닌가」로 스스로 평가해, 넷(불발이 · 갉작이 · 도장이 · 안 깬 알)은 첫 판을 버리고 다시 그렸다.",
        "",
    ]
    for i, c in enumerate(CANDIDATES, 1):
        L += [f"## {i}. {c['title']} {{#{c['id']}}}", "", f"**{c['hook']}**", ""]
        if have[c["id"]]:
            L += ['<div class="mascot-shot">' + shot(c["id"], esc(c["title"])) + "</div>", ""]
        else:
            L += ["*(그림 없음)*", ""]
        L += [
            "| | |",
            "|---|---|",
            f"| 이름 후보 | {cell(' · '.join(c['names']))} |",
            f"| 대비 | {cell(c['contrast'])} |",
            f"| 그리모리와 닿는 곳 | {cell(c['tie'])} |",
            f"| 크기 | {cell(c['size'])} |",
            f"| 생김새(메시 · 재질) | {cell(c['form'])} |",
            f"| FX | {cell(c['fx'])} |",
            f"| **닮을 위험** | {cell(c['risk'])} |",
        ]
        if c.get("review"):
            L.append(f"| **그림 평가** | {cell(c['review'])} |")
        L.append("")
    L += [
        f"## 쳐낸 후보 — {len(REJECTED)}개",
        "",
        f"고리를 먼저 쓴 후보 {len(CANDIDATES) + len(REJECTED)}개 가운데 기준으로 쳐낸 것. 고리가 좋은 것은 남은 후보에 녹였다.",
        "",
        "| 후보 | 고리 | 쳐낸 까닭 |",
        "|---|---|---|",
    ]
    for name, hook, why in REJECTED:
        L.append(f"| {cell(name)} | {cell(hook)} | {cell(why)} |")
    L += [
        "",
        "## 옛 판(반려) — v1 시안 6개 (2026-11-19)",
        "",
        '<details class="mascot-old"><summary>v1 보기 — 책장귀 · 깃펜 도롱뇽 · 서클 나방 · 꼬리등불 · 마나결정 천산갑 · 첨탑 소라게</summary>',
        "",
        "사용자 「너무 틀에 박힌 느낌」 — 여섯이 모두 「귀여운 실제 동물(털뭉치 · 도롱뇽 · 나방 · 다람쥐 · 천산갑 · 소라게) + 게임 소재 소품(책장 · 펜촉 · 마법진 · 등불 · 결정 · 탑) + 머리 : 몸 1 : 1 인형 비율」의 한 공식이었다. 정본은 `art/mascot/make_prompts_v1.py`.",
        "",
        '<div class="mascot-shot">' + shot("mascot-compare-v1", "v1 시안 6개 비교") + "</div>",
        '<div class="grid">',
    ]
    for d in V1:
        L.append(f"<div>{shot(d['id'], esc(d['title']))}{esc(d['title'])} — {esc(d['concept'])}</div>")
    L += [
        "</div>",
        "</details>",
        "",
        "## 만든 과정",
        "",
        "- 후보 · 프롬프트의 정본 `art/mascot/make_prompts.py`(`CANDIDATES` · `REJECTED` · 공통 `RULES` · `STYLE`), 프롬프트 `art/mascot/prompts/`, 이 페이지 · JPG · 비교 그림은 `art/mascot/publish.py`. 원본 PNG `art/mascot/raw/`(버린 첫 판은 `raw/rejected/`)는 git 에 올리지 않는다.",
        "- 그리기는 생물 콘셉트와 같은 `art/tools/run_codex_job.sh` — 그림 하나에 codex 세션 하나, 자기 세션 폴더의 그림인지 비교한다. 같은 파이프라인을 쓰는 다른 세션과 섞이지 않게 한 번에 2장까지만 돌렸다.",
        "",
    ]
    with open(OUT_MD, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L))
    print(f"그림 {sum(have.values())}/{len(CANDIDATES)} → {OUT_MD}")


if __name__ == "__main__":
    main()
