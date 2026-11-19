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
from make_prompts import CREATURES, FUNCTIONS, MAGIC, NEW_GROUP, NEW_SPECIES, REGIONS  # noqa: E402
from ideas import CARBUNCLE_DRAFTS, CRITERIA, CUT_IDEAS, IDEA_GROUP, IDEAS  # noqa: E402

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
    olds = [j for j in (cid + "-v3", cid + "-v2", cid + "-v1", cid + "-b-v1") if publish_image(j)]
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
        lines.append('<details class="old"><summary>옛 판 보기 — v1(2026-11-18)' + ''.join(' · ' + v for v in ('v2', 'v3') if any(j.endswith('-' + v) for j in olds)) + '</summary>')
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


def board(jobs, labels, out_job, cols):
    """여러 그림을 한 장에 모은다(이름표 포함) — 「모아 본 그림」. raw 가 하나라도 없으면 만들지 않는다."""
    from PIL import ImageDraw, ImageFont
    srcs = [os.path.join(HERE, "raw", j + ".png") for j in jobs]
    if not all(os.path.exists(s) for s in srcs):
        return False
    tw, th, pad, bar = 768, 512, 12, 44
    rows = (len(jobs) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * (tw + pad) + pad, rows * (th + bar + pad) + pad), (24, 24, 28))
    try:
        font = ImageFont.truetype("C:/Windows/Fonts/malgunbd.ttf", 26)
    except OSError:
        font = ImageFont.load_default()
    draw = ImageDraw.Draw(sheet)
    for i, (s, label) in enumerate(zip(srcs, labels)):
        x, y = pad + (i % cols) * (tw + pad), pad + (i // cols) * (th + bar + pad)
        sheet.paste(Image.open(s).convert("RGB").resize((tw, th), Image.LANCZOS), (x, y + bar))
        draw.text((x + 4, y + 6), label, fill=(240, 236, 228), font=font)
    sheet.save(os.path.join(OUT_IMG, out_job + ".jpg"), quality=QUALITY, optimize=True)
    return True


def idea_card(job, title, hook, rows, fx=""):
    has = publish_image(job)
    lines = ['<div class="creature-card">', shot(job, esc(title)) if has else '<div class="none">그림 없음</div>']
    if publish_image(job + "-t1"):  # 첫 판(다른 순간)이 따로 있으면 둘째 그림으로
        lines += ['<div class="alt">첫 판 — 다른 순간</div>', shot(job + "-t1", esc(title) + " 첫 판")]
        has += 1
    lines += ['<div class="body">',
             f'<span class="name">{esc(title)}</span> <code>{job}</code><br>', f"<b>「{esc(hook)}」</b>",
             '<div class="meta">' + "<br>".join(esc(r) for r in rows) + "</div>"]
    if fx:
        lines.append(f'<div class="fx"><b>FX:</b> {esc(fx)}</div>')
    lines += ["</div>", "</div>"]
    return "\n".join(lines), int(has)


def ideas_section():
    """새 종 후보 2차(ideas.py) — 카벙클 시안 A/B/C · 새 후보 · 쳐낸 후보 · 1차 판(접어 둠)."""
    done = 0
    lines = [f"## {IDEA_GROUP} {{#new-species}}", "",
             "2026-11-20 사용자 피드백 — 1차 새 종 5종(2026-11-19, 참고 그림 5장의 보석 · 유리 느낌) 중 **카벙클은 「귀엽다, 다듬자」**, "
             "나머지 넷은 **「너무 뻔하다」**(유리 도마뱀 · 날개 뱀 · 새 같은 용 · 머리 여럿 뱀 — 이미 본 판타지 원형에 보석 재질만 입혔다). "
             "그래서 이번에는 시트부터 그리지 않고, 글로 후보 27개를 쓰고 아래 기준으로 쳐낸 뒤 남은 7개를 큰 그림 한 장씩(마법 쓰는 순간) 그렸다. "
             "**고르는 건 사용자다** — 카벙클 시안은 [Q220](../spec/decisions#미결-질문), 새 후보는 Q221. 고른 뒤에 4면 시트를 그린다. "
             "정본은 `art/creatures/ideas.py`.", "",
             "**「뻔하지 않다」 기준** — 그리기 전에 글로 통과시켰다:", ""]
    lines += [f"{i}. {esc(c)}" for i, c in enumerate(CRITERIA, 1)]
    # 카벙클
    lines += ["", "### 카벙클 시안 — 하나를 고른다(Q220) {#carbuncle-drafts}", "",
              "지킨 것: 여우 · 고양이 얼굴 · 큰 귀 · 흰 가슴털 · 무지갯빛 투명 날개 넷 · 지느러미 꼬리 · 회화적인 빛. "
              "고친 것: 파이널판타지 카벙클(긴 귀 · 푸른 몸 · 이마의 붉은 보석)과 거리를 두려고 **보석 자리 · 색 · 모양과 귀 모양을 모두 바꿨고**, "
              "흔한 잠자리 날개를 **보석을 깎은 면(패싯) 같은 얇은 판**으로, 크기가 읽히게 사람 실루엣을 바로 옆에, 아래 띠에 **표정 넷**(보석 빛이 기분)을 넣었다. "
              "원전 고리: 남미 · 스페인 광부 전승 — 동굴에서 보석이 등불처럼 빛나고, 얻으면 부자가 되지만 결코 잡히지 않는다.", ""]
    if board(["carbuncle"] + [d["id"] for d in CARBUNCLE_DRAFTS], ["지금 판(1차)"] + [d["label"] for d in CARBUNCLE_DRAFTS], "carbuncle-drafts", 2):
        lines += ['<div class="creature-card" style="margin:12px 0">' + shot("carbuncle-drafts", "카벙클 시안 모아 보기") + "</div>", ""]
    lines += ["| 시안 | 고리 한 줄 | 몸 · 보석 · 귀 · 날개 |", "|---|---|---|",
              "| 지금 판(1차) | 이마 보석으로 빛을 모은다 | 푸른 털 · 이마 붉은 보석 · 큰 귀 · 잠자리 날개 넷 |"]
    for d in CARBUNCLE_DRAFTS:
        lines.append(f"| **{d['label']}** | 「{d['hook']}」 | {d['body']} · {d['gem']} · {d['ears']} · {d['wings']} |")
    lines += ["", '<div class="creature-grid">']
    for d in CARBUNCLE_DRAFTS:
        text, n = idea_card(d["id"], "카벙클 " + d["label"], d["hook"], [d["moment"]], d["fx"])
        done += n
        lines.append(text)
    lines += ["</div>", ""]
    # 새 후보
    lines += [f"### 새 후보 {len(IDEAS)} — 고른다(Q221) {{#ideas}}", ""]
    if board([d["id"] for d in IDEAS], [f"{i}. {d['name']}" for i, d in enumerate(IDEAS, 1)], "ideas-board", 3):
        lines += ['<div class="creature-card" style="margin:12px 0">' + shot("ideas-board", "새 후보 모아 보기") + "</div>", ""]
    lines += ["| # | 이름 | 고리 한 줄 | 대비 | 닿는 시스템 | 제안 지역 · 크기 |", "|---|---|---|---|---|---|"]
    for i, d in enumerate(IDEAS, 1):
        lines.append(f"| {i} | **{d['name']}** `{d['id']}` | 「{d['hook']}」 | {d['contrast']} | {d['system']} | {d['region']}. {REGIONS[d['region']][0]} · {d['height']} |")
    lines += ["", '<div class="creature-grid">']
    for d in IDEAS:
        rows = [f"속성 {d['main']} · {d['sub']} · {d['size']}({d['height']}) · {d['temper']}", f"원전 {d['src']}",
                f"길들이기 · 쓰임 {d['use']}", f"개수 {d['counts']}", f"겹침 확인: {d['distinct']}"]
        text, n = idea_card(d["id"], d["name"], d["hook"], rows, d["fx"])
        done += n
        lines.append(text)
    lines += ["</div>", ""]
    # 쳐낸 후보
    lines += [f"### 쳐낸 후보 {len(CUT_IDEAS)}", "", "| 후보 | 고리 · 생김새 | 쳐낸 까닭 |", "|---|---|---|"]
    lines += [f"| {a} | {b} | {c} |" for a, b, c in CUT_IDEAS]
    # 1차 판
    old = [c for c in NEW_SPECIES if c[0] != "carbuncle"]
    lines += ["", "### 1차 판(2026-11-19) — 접어 둠", "",
              "「뻔하다」로 쳐낸 1차 넷. 이름 · 설명은 `make_prompts.py` 의 `NEW_SPECIES` 에 남겨 두었다(카벙클 지금 판은 위 시안 표의 「지금 판」).", "",
              '<details class="old"><summary>1차 판 보기 — ' + " · ".join(c[2] for c in old) + "</summary>", "", '<div class="creature-grid">']
    for c in old:
        text, n = card(c)
        done += n
        lines.append(text)
    lines += ["</div>", "", "</details>", ""]
    return done, lines


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
    lines.append(f"| [{IDEA_GROUP}](#new-species) | 카벙클 시안 {len(CARBUNCLE_DRAFTS)} + 후보 {len(IDEAS)} | — | — | 고른다(Q220 · Q221) |")
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
    n, more = ideas_section()
    done += n
    lines += more
    with open(OUT_MD, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    print(f"그림 {done}장 · 종 {len(rows)} → {OUT_MD}")


if __name__ == "__main__":
    main()
