"""마법 생물 콘셉트 그림을 문서 사이트에 올린다 — raw/<id>.png → docs/public/art/creatures/<id>.jpg + docs/art/creature-concepts.md

    python -X utf8 art/creatures/publish.py            # 1~3번 지역(오픈 베타 후보)
    python -X utf8 art/creatures/publish.py 1 2 3 4    # 지역을 골라서

그림이 아직 없는 종은 「그림 없음」 카드로 둔다. 전설의 동행 장면 시안은 raw/<id>-b.png(있으면 같은 카드에 둘째 그림).
옛 판 raw/<id>-v1.png · <id>-v2.png 가 있으면 카드 아래 접힌 칸에 둔다(비교용). 카드의 「FX」 줄은 make_prompts.py 의 MAGIC.
문서 그림은 LFS 가 아니므로 JPG(가로 1536, 품질 85)로 줄인다. 명단 · 설명의 정본은 make_prompts.py 의 CREATURES.
"""
import html
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
from make_prompts import CREATURES, FUNCTIONS, MAGIC, REGIONS  # noqa: E402

OUT_IMG = os.path.join(ROOT, "docs", "public", "art", "creatures")
OUT_MD = os.path.join(ROOT, "docs", "art", "creature-concepts.md")
WIDTH, QUALITY = 1536, 85

# 지역마다 표 위에 붙이는 한 줄
REGION_NOTE = {
    1: "오픈 베타의 땅 — **10종 모두 들어간다**(고르는 대상이 아니다). 재칼로프 · 켈피 · 검은 개는 이미 게임 데이터가 있다.",
    2: "오픈 베타로 **미리 옮겨 올 후보**. 숲 · 늪 생물이라 1번 지역의 고목 둘레 · 연못 저지대 · 작은 계곡에 놓기 쉽다.",
    3: "오픈 베타로 **미리 옮겨 올 후보**. 바닷가 · 절벽 생물이라 1번 지역 남쪽 바닷가 곶 · 북쪽 고지 절벽에 놓을 수 있다. 뇌우 조건부 종은 날씨가 있어야 나온다.",
}

STYLE = """<style>
.creature-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 16px; margin: 16px 0 28px; }
.creature-card { border: 1px solid var(--vp-c-divider); border-radius: 8px; overflow: hidden; background: var(--vp-c-bg-soft); }
.creature-card a.shot { display: block; }
.creature-card img { display: block; width: 100%; height: auto; }
.creature-card .none { padding: 48px 12px; text-align: center; color: var(--vp-c-text-3); }
.creature-card .body { padding: 10px 12px 12px; font-size: 14px; line-height: 1.55; }
.creature-card .name { font-size: 16px; font-weight: 600; }
.creature-card .tag { display: inline-block; margin-left: 4px; padding: 0 6px; border-radius: 4px; font-size: 12px; background: var(--vp-c-default-soft); }
.creature-card .tag.legend { background: var(--vp-c-yellow-soft); }
.creature-card .tag.boss { background: var(--vp-c-red-soft); }
.creature-card .meta { margin-top: 6px; font-size: 12.5px; color: var(--vp-c-text-2); }
.creature-card .alt { padding: 4px 12px; font-size: 12px; color: var(--vp-c-text-2); border-top: 1px solid var(--vp-c-divider); }
.creature-card details.old { border-top: 1px solid var(--vp-c-divider); font-size: 12px; color: var(--vp-c-text-2); }
.creature-card details.old summary { padding: 4px 12px; cursor: pointer; }
.creature-card .fx { margin-top: 6px; font-size: 12.5px; }
</style>"""


def esc(text):
    # 카드 안은 HTML 이라 Vue 템플릿이 읽는다 — 꺾쇠 · 중괄호를 막는다.
    return html.escape(text, quote=False).replace("{{", "&#123;&#123;")


def func_plain(cid):
    f = FUNCTIONS[cid]
    return " · ".join(f"{k}({d})" for k, d in f) if f else "없음(동행 · 전투)"


def tame_text(use):
    # 카드는 HTML 이라 Markdown 굵은 글씨(**)가 그대로 보인다 — 뺀다.
    return use.replace("**", "")


def publish_image(job):
    src = os.path.join(HERE, "raw", job + ".png")
    if not os.path.exists(src):
        return False
    image = Image.open(src).convert("RGB")
    if image.width > WIDTH:
        image = image.resize((WIDTH, round(image.height * WIDTH / image.width)), Image.LANCZOS)
    image.save(os.path.join(OUT_IMG, job + ".jpg"), quality=QUALITY, optimize=True)
    return True


def shot(job, alt):
    # img src 에는 base(/Grimoire/)가 저절로 붙지만 a href 에는 안 붙는다 — 페이지의 withBase 로 붙인다.
    url = f"/art/creatures/{job}.jpg"
    return f"<a class=\"shot\" :href=\"withBase('{url}')\" target=\"_blank\"><img src=\"{url}\" alt=\"{alt}\" loading=\"lazy\"></a>"


def card(c):
    cid, reg, name, src, adapt, main, sub, size, height, temper, role, habitat, use, rarity, look = c
    has = publish_image(cid)
    has_b = publish_image(cid + "-b")
    olds = [j for j in (cid + "-v2", cid + "-v1", cid + "-b-v1") if publish_image(j)]
    tags = ""
    if rarity == "전설":
        tags += '<span class="tag legend">전설</span>'
    if "보스" in role:
        tags += '<span class="tag boss">필드 보스</span>'
    lines = ['<div class="creature-card">']
    if has:
        lines.append(shot(cid, esc(name)))
    else:
        lines.append('<div class="none">그림 없음</div>')
    if has_b:
        lines.append(f'<div class="alt">시안 B — 길들인 뒤 동행 장면</div>')
        lines.append(shot(cid + "-b", esc(name) + " 시안 B"))
    if olds:
        lines.append('<details class="old"><summary>옛 판 보기 — v1(2026-11-18)' + (' · v2' if any(j.endswith('-v2') for j in olds) else '') + '</summary>')
        lines += [shot(j, esc(name) + " 옛 판") for j in olds]
        lines.append("</details>")
    lines.append('<div class="body">')
    lines.append(f'<span class="name">{esc(name)}</span> <code>{cid}</code>{tags}<br>')
    lines.append(f"{esc(adapt)}")
    lines.append(
        f'<div class="meta">크기 {esc(size)}({esc(height)}) · 속성 {main} · {sub} · {esc(rarity)}<br>'
        f"원전 {esc(src)}<br>기능 {esc(func_plain(cid))}<br>{esc(tame_text(use))}</div>"
    )
    if cid in MAGIC:
        lines.append(f'<div class="fx"><b>FX:</b> {esc(MAGIC[cid]["fx"])}</div>')
    lines += ["</div>", "</div>"]
    return "\n".join(lines), int(has) + int(has_b)


def main():
    regions = [int(a) for a in sys.argv[1:]] or [1, 2, 3]
    os.makedirs(OUT_IMG, exist_ok=True)
    rows = [c for c in CREATURES if c[1] in regions]
    lines = [
        "---",
        "aside: false",
        "---",
        "",
        "# 마법 생물 콘셉트 아트 — 오픈 베타 후보",
        "",
        "> 상태: **검토용 v2** · 2026-11-19 · GPT 이미지(Codex CLI). v1(2026-11-18)이 「실제 동물에 뿔 · 이끼를 얹은」 느낌이라 「킥」 규칙으로 다시 그렸다 — 종마다 **있을 수 없는 특징 하나**(몸 일부가 속성 재질 · 떠 있는 부위 · 몸에서 나는 빛)와 **자기 마법을 쓰는 순간**. 몸은 메시 · 재질로 만들 수 있는 또렷한 형태이고, 흩어지는 것(안개 · 물방울 · 불티 · 음파)은 카드의 **FX** 줄에 적은 대로 나이아가라 효과로 낸다. 옛 판은 카드 아래 「옛 판 보기」에 접어 두었다.",
        ">",
        "> 아트 방향은 [Painterly High-Fidelity Stylized Realism](../spec/art-direction), 생물 톤 규칙은 [지역별 생물 명단 — 톤 · 겉모습 규칙](../spec/creature-roster#톤-·-겉모습-규칙).",
        ">",
        "> **쓰임**: 오픈 베타 30종(Q50)을 그림을 보고 고른다. 1번 지역 10종은 다 들어가고, 2 · 3번 지역 16종에서 골라 채운다 — **셋을 다 넣어도 26종**이라 30종에는 4종이 모자란다(4번 지역부터 당겨 오거나 종 수를 줄이는 것은 정할 일).",
        ">",
        "> **전설(☆)도 잡고 교배할 수 있다**(Q48 답 — [지역별 생물 명단 「전설」](../spec/creature-roster#전설-—-잡고-·-자랑하고-·-교배한다-q48)). 전설 카드에는 도감 그림과 「길들인 뒤 동행 장면」 시안 B 를 함께 둔다.",
        "",
        "그림 읽는 법: 3/4 앞 시점 전신, 옆의 회색 실루엣은 키 1.8 m 사람(축척), 아래 띠는 색 변이 셋(기본 · 암수 · 돌연변이). 그림을 누르면 크게 열린다. 이름 · 설명의 정본은 [지역별 생물 명단](../spec/creature-roster)(`art/creatures/make_prompts.py`), 프롬프트는 `art/creatures/prompts/`, 이 페이지는 `art/creatures/publish.py` 가 만든다.",
        "",
        "<script setup>",
        "import { withBase } from 'vitepress'  // 그림 링크(href)는 base(/Grimoire/)가 저절로 붙지 않는다 — img src 는 붙는다",
        "</script>",
        "",
        STYLE,
        "",
        "## 한눈에",
        "",
        "| 지역 | 종 | 전설 | 필드 보스 | 오픈 베타 |",
        "|---|---|---|---|---|",
    ]
    for reg in regions:
        rs = [c for c in rows if c[1] == reg]
        legend = " · ".join(c[2] for c in rs if c[13] == "전설") or "—"
        boss = " · ".join(c[2] for c in rs if "보스" in c[10]) or "—"
        lines.append(f"| [{reg}. {REGIONS[reg][0]}](#region-{reg}) | {len(rs)} | {legend} | {boss} | {'전부' if reg == 1 else '고른다'} |")
    lines.append("")
    done = 0
    for reg in regions:
        rname, circle, focus = REGIONS[reg]
        rs = [c for c in rows if c[1] == reg]
        lines += [f"## {reg}. {rname} — {len(rs)}종 {{#region-{reg}}}", "", f"서클 {circle} · 주 속성 {focus}. {REGION_NOTE.get(reg, '')}".rstrip(), ""]
        lines += ["| 이름 | 한 줄 | 크기 | 원전 | 기능 | 희귀도 |", "|---|---|---|---|---|---|"]
        for c in rs:
            cid, _, name, src, adapt, main, sub, size, height, temper, role, habitat, use, rarity, look = c
            lines.append(f"| **{name}** `{cid}` | {adapt} | {size}({height}) | {src} | {func_plain(cid)} | {rarity} |")
        lines += ["", '<div class="creature-grid">']
        for c in rs:
            text, n = card(c)
            done += n
            lines.append(text)
        lines += ["</div>", ""]
    with open(OUT_MD, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    print(f"그림 {done}장 · 종 {len(rows)} → {OUT_MD}")


if __name__ == "__main__":
    main()
