"""마스코트 후보를 문서 사이트에 올린다 — raw/<id>.png → docs/public/art/mascot/<id>.jpg + 비교 그림 + docs/art/mascot-concepts.md

    python -X utf8 art/mascot/publish.py

v4(2026-11-21, 톤 축): make_prompts_v4.py 의 TONES · CANDIDATES · REJECTED. 비교 그림 mascot-compare-v4.jpg(3 × 3 — 비색이 3안 + 새 후보 6).
v3(반려 2026-11-21, 한국 신화에서 출발 · 귀여움에 치중): make_prompts_v3.py — 페이지에서 접힘. 비교 그림 mascot-compare-v3.jpg.
v2(2026-11-20, 고리 먼저): make_prompts.py 의 CANDIDATES · REJECTED. 비교 그림 mascot-compare.jpg(4 × 2).
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
from make_prompts_v3 import CANDIDATES as V3, REJECTED as V3_REJECTED  # noqa: E402
from make_prompts_v4 import CANDIDATES as V4, REJECTED as V4_REJECTED, TONES  # noqa: E402
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
    """후보를 한 장에 — 칸 768 × 512 + 위 이름 띠. items = [(id, 띠 제목, 한 줄)]."""
    cw, ch, band = 768, 512, 64
    rows = (len(items) + cols - 1) // cols
    sheet = Image.new("RGB", (cw * cols, (ch + band) * rows), (236, 232, 226))
    draw = ImageDraw.Draw(sheet)
    big = ImageFont.truetype(FONT, 28)
    small = ImageFont.truetype(FONT.replace("malgunbd", "malgun"), 18)
    for i, (job, title, line) in enumerate(items):
        x, y = (i % cols) * cw, (i // cols) * (ch + band)
        draw.rectangle([x, y, x + cw, y + band], fill=(38, 34, 52))
        draw.text((x + 16, y + 6), title, font=big, fill=(245, 230, 190))
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




def candidate_sections(cands, have, level, extra=()):
    """후보마다 그림 + 표. extra = 표 맨 위에 더할 (이름, 키) — v3 의 「출발한 신화」."""
    L = []
    for i, c in enumerate(cands, 1):
        L += [f"{level} {i}. {c['title']} {{#{c['id']}}}", "", f"**{c['hook']}**", ""]
        if have[c["id"]]:
            L += ['<div class="mascot-shot">' + shot(c["id"], esc(c["title"])) + "</div>", ""]
        else:
            L += ["*(그림 없음)*", ""]
        L += ["| | |", "|---|---|"]
        for label, key in extra:
            L.append(f"| {label} | {cell(c[key])} |")
        L += [
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
    return L


def main():
    os.makedirs(OUT_IMG, exist_ok=True)
    have = {c["id"]: publish_image(c["id"]) for c in CANDIDATES + V3 + V4}
    compare([(c["id"], f"{i}. {c['title'].split(' · ')[0]} — {c['tone']}" + (" (와일드카드)" if c.get("wild") else ""),
              "같은 고리 — " + c["contrast"] if c["id"].startswith("bisaek") else short_hook(c))
             for i, c in enumerate(V4, 1)], "mascot-compare-v4.jpg", 3)
    compare([(c["id"], f"{i}. {c['title']}", short_hook(c)) for i, c in enumerate(CANDIDATES, 1)], "mascot-compare.jpg", 4)
    doodle = next(c for c in CANDIDATES if c["id"] == "doodle")
    compare([(c["id"], f"{i}. {c['title']}", short_hook(c)) for i, c in enumerate(V3, 1)]
            + [("doodle", "비교 · v2 끄적이", short_hook(doodle))], "mascot-compare-v3.jpg", 3)
    L = [
        "---",
        "aside: false",
        "---",
        "",
        "# 마스코트 후보",
        "",
        "> 상태: **v4 톤 축 후보 9개(비색이 3안 + 새 후보 6) + v2 후보 8개 — 검토용** · 2026-11-21 · GPT 이미지(Codex CLI). 사용자 요청(2026-11-19) 「팰월드 까부냥, 포켓몬 피카츄처럼 게임 얼굴이 될 오리지널 마법 생물 — **오픈 베타에 꼭 넣는다**」.",
        ">",
        "> - **v4**(위): v3 반려 「다별론데... 비색이는 스토리는 좋은데 생김새가 그닥이야. 너무 귀여운거에만 치중되어 있지 않아? 다른 시안 여러 컨셉으로 뽑아줘」 → 「귀여움」 한 방향을 벗어나 **톤 축**(위엄 · 기묘 · 우아 · 날렵 · 아이콘 · 트릭스터 · 태고)으로 다시 뽑았다. 비색이는 이야기만 살리고 생김새를 톤이 다른 3안으로. 한국 신화 출발은 그대로(신화 밖 와일드카드는 표시).",
        "> - **v3**(반려 2026-11-21, 접힘): 한국 신화 8개 — 귀여움에 치중. 비색이 이야기만 v4 로 살렸다. 카벙클은 마스코트로 쓰지 않는다(파이널판타지 · 뿌요뿌요가 대표 캐릭터로 써 왔고 민담 이름이라 독점할 수 없다 — 주요 종으로 남는다).",
        "> - **v2**(아래): v1 이 「너무 틀에 박힌 느낌」으로 반려된 뒤 고리(행동 · 성격 · 플레이어와의 관계)부터 쓴 8개. 반려되지 않았다.",
        "> - v1 시안 6개는 맨 아래 「옛 판(반려)」에 접어 두었다.",
        ">",
        "> 고를 것은 [미결 Q215](../spec/decisions#q215)(v4 · v2 가운데 어느 후보 · 어느 톤). 고른 뒤에 4면 시트를 그리고 [지역별 생물 명단](../spec/creature-roster)에 넣는다.",
        "",
        "<script setup>",
        "import { withBase } from 'vitepress'  // 그림 링크(href)는 base(/Grimoire/)가 저절로 붙지 않는다",
        "</script>",
        "",
        STYLE,
        "",
        "## 기준",
        "",
        "그리기 전에 글로 통과시켰다(2026-11-20 메인 세션 정리). v2 · v3 · v4 공통.",
        "",
        "| # | 기준 |",
        "|---|---|",
        "| 1 | 「실제 동물 + 붙인 마법 소품」 금지 — 마법이 꾸밈이 아니라 몸의 구조이거나 존재 이유 |",
        "| 2 | **한 줄 고리** — 플레이어가 「걔 ○○한다?」 하고 남에게 말할 행동 · 성격 · 플레이어와의 관계 |",
        "| 3 | **대비 하나** — 귀여운데 이상하다 · 작은데 위험하다 · 아름다운데 묘하다(공포 아님) · 하찮은 것이 마법적이다 |",
        "| 4 | 실루엣으로 알아보고, 피카츄 · 이브이 · 까부냥 · 램볼 · 커비 · 토토로 · 파이널판타지 카벙클 · 모그리 등과 겹치지 않는다 |",
        "| 5 | 그리모리 정체성과 닿는다 — 마법서 · 서클 · 마나 · 파크툼 계약 · 마법탑 · 생존 |",
        "| 6 | 구현 가능성(몸은 메시 · 재질, 흩어지는 것은 FX)과 [아트 방향](../spec/art-direction)(회화적 3D 스타일라이즈드) |",
        "| v3 | 한국 신화에서 출발하되 원전을 그대로 옮기지 않는다 — 서양식 마법 세계(마탑 · 마법서 · 서클)와 섞고, 민화 · 단청의 색과 선은 2D 그림체가 아니라 **몸의 재질 · 무늬**(돌 · 쇠 · 싸리 · 청자 · 나전 · 먹 · 비단)로. 「해태」 그대로(해태제과 · 서울 해치) · 케이팝 데몬 헌터스의 민화 호랑이 · 까치 · 모코코 · 카카오 · 라인프렌즈 · 쿠키런과 겹치지 않고, 무속 · 종교 상징을 가볍게 쓰지 않는다 |",
        "| v4 | **귀여움이 중심이 아니어도 된다** — 마스코트는 「게임의 얼굴」이라 위엄 · 기묘 · 우아 · 멋 · 능청으로 끌 수 있다. 큰 머리 · 동그란 몸 · 큰 눈 · 활짝 웃는 얼굴 · 혀 내밀기 · 인형 비율 금지. 후보끼리 비율 · 크기(손바닥 ~ 40 m) · 실루엣 · 표정 · 빛이 겹치지 않게. 더 피할 것: 오카미 아마테라스 · 지브리(하쿠 · 사슴신 · 전반) · 드래곤 길들이기 투슬리스 · 오리 · 할로우 나이트 · 포켓몬 전설 · 팰월드 |",
        "",
        "그림은 시트가 아니라 **그 생물다운 순간 한 장**이다. 사람은 손 · 발 · 소매나 멀리 작은 실루엣만 보인다(크기 비교). 그린 뒤 「캡처해서 남에게 보여 주고 싶은가 · 카벙클 · 끄적이 옆에서 지지 않는가 · 신화를 그대로 옮기거나 또 동물 + 소품이 되지 않았는가 · **또 귀여움만인가**」로 스스로 평가해, 못 미친 그림은 다시 그렸다.",
        "",
        "## v4 — 톤 축으로 다시 뽑은 후보 {#v4}",
        "",
        "v3 여덟이 모두 「귀여움」 한 방향(큰 머리 · 동그란 몸 · 웃는 얼굴 · 혀 내밀기 · 인형 비율)이었다. 이번에는 톤 축을 먼저 정하고 축마다 고르게 나눴다. 비색이는 같은 이야기로 톤이 다른 3안, 새 후보 6개 가운데 귀여움이 매력의 중심인 것은 없다.",
        "",
        "| 톤 축 | 방향 | 후보 |",
        "|---|---|---|",
    ]
    for tone, direction in TONES:
        names = " · ".join(f"[{c['title']}](#{c['id']})" for c in V4 if c["tone"] == tone)
        L.append(f"| **{tone}** | {cell(direction)} | {names} |")
    L += [
        "",
        '<div class="mascot-shot">' + shot("mascot-compare-v4", "v4 톤 축 후보 9개 비교 — 비색이 3안과 새 후보 6") + "</div>",
        "",
        "| # | 후보 | 톤 축 | 출발 | 고리 — 「걔 ○○한다?」 | 대비 |",
        "|---|---|---|---|---|---|",
    ]
    for i, c in enumerate(V4, 1):
        L.append(f"| {i} | [**{c['title']}**](#{c['id']}) | {c['tone']} | {cell(c['myth'])} | {cell(c['hook'])} | {cell(c['contrast'])} |")
    L += [
        "",
        "비색이 3안이 함께 지키는 것 — 이야기(여의주가 없어 용이 못 된 이무기 · 동그란 것을 턱밑에 맞춰 보기 · 10서클에 용)와 「용이 되다 만」 표시: 턱밑의 **동그란 빈 홈**(여의주 자리) · 다 자라지 못한 뿔 · 몸 앞쪽에만 있는 용 비늘 · 돋다 만 앞발. v3 의 통통한 청자 뱀 인형 비율은 버렸다.",
        "",
    ]
    L += candidate_sections(V4, have, "###", extra=(("톤 축", "tone"), ("출발", "myth"), ("톤을 그림으로", "mood")))
    L += [
        f"### v4 쳐낸 아이디어 — {len(V4_REJECTED)}개",
        "",
        f"톤 축마다 글로 쓴 {len(V4) + len(V4_REJECTED)}개 가운데 기준으로 쳐낸 것. 고리가 좋은 것은 남은 후보에 녹였다.",
        "",
        "| 아이디어 | 출발 | 톤 축 | 고리 | 쳐낸 까닭 |",
        "|---|---|---|---|---|",
    ]
    for name, myth, tone, hook, why in V4_REJECTED:
        L.append(f"| {cell(name)} | {cell(myth)} | {tone} | {cell(hook)} | {cell(why)} |")
    L += [
        "",
        "## 옛 판(반려) — v3 한국 신화 후보 8개 (2026-11-20) {#v3}",
        "",
        '<details class="mascot-old"><summary>반려(2026-11-21): 귀여움에 치중 — 비색이 이야기만 살림. v3 보기 — 불먹이 · 쇠먹이 · 몽당이 · 비색이 · 우렁이 · 어둑이 · 보따리 · 불개</summary>',
        "",
        "사용자 「다별론데... 비색이는 스토리는 좋은데 생김새가 그닥이야. 너무 귀여운거에만 치중되어 있지 않아?」 — 여덟이 모두 큰 머리 · 동그란 몸 · 웃는 얼굴 · 혀 내밀기 · 인형 비율의 「귀여움」 한 방향이었다. 정본은 `art/mascot/make_prompts_v3.py`. 아홉째 칸은 비교용 v2 끄적이다.",
        "",
        '<div class="mascot-shot">' + shot("mascot-compare-v3", "v3 한국 신화 후보 8개와 v2 끄적이 비교") + "</div>",
        "",
        "| # | 후보 | 출발한 신화 | 고리 — 「걔 ○○한다?」 | 대비 |",
        "|---|---|---|---|---|",
    ]
    for i, c in enumerate(V3, 1):
        L.append(f"| {i} | [**{c['title']}**](#{c['id']}) | {cell(c['myth'])} | {cell(c['hook'])} | {cell(c['contrast'])} |")
    L.append("")
    L += candidate_sections(V3, have, "####", extra=(("출발한 신화", "myth"),))
    L += [
        f"#### v3 쳐낸 후보 — {len(V3_REJECTED)}개",
        "",
        f"한국 신화 · 민속에서 고리를 먼저 쓴 {len(V3) + len(V3_REJECTED)}개 가운데 기준으로 쳐낸 것.",
        "",
        "| 후보 | 출발한 신화 | 고리 | 쳐낸 까닭 |",
        "|---|---|---|---|",
    ]
    for name, myth, hook, why in V3_REJECTED:
        L.append(f"| {cell(name)} | {cell(myth)} | {cell(hook)} | {cell(why)} |")
    L += [
        "",
        "</details>",
        "",
        "## v2 — 고리 먼저 쓴 후보 {#v2}",
        "",
        "v1 여섯이 모두 「귀여운 실제 동물 + 붙인 게임 소품 + 인형 비율」 한 공식이라 반려된 뒤, 고리부터 33개를 글로 쓰고 기준으로 쳐내 8개를 그렸다. 넷(불발이 · 갉작이 · 도장이 · 안 깬 알)은 첫 판을 버리고 다시 그렸다.",
        "",
        '<div class="mascot-shot">' + shot("mascot-compare", "v2 마스코트 후보 8개 비교") + "</div>",
        "",
        "| # | 후보 | 고리 — 「걔 ○○한다?」 | 대비 | 그리모리와 닿는 곳 |",
        "|---|---|---|---|---|",
    ]
    for i, c in enumerate(CANDIDATES, 1):
        L.append(f"| {i} | [**{c['title']}**](#{c['id']}) | {cell(c['hook'])} | {cell(c['contrast'])} | {cell(c['tie'])} |")
    L.append("")
    L += candidate_sections(CANDIDATES, have, "###")
    L += [
        f"### v2 쳐낸 후보 — {len(REJECTED)}개",
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
        "- 후보 · 프롬프트의 정본: v4 `art/mascot/make_prompts_v4.py`(`TONES` 톤 축 · `CANDIDATES` · `REJECTED` · 귀여움 금지 공식을 넣은 `RULES`), v3 `art/mascot/make_prompts_v3.py`(`CANDIDATES` · `REJECTED` · 한국 신화 규칙 `KOREAN`), v2 `art/mascot/make_prompts.py`(`CANDIDATES` · `REJECTED` · 공통 `RULES` · `STYLE`), v1 `make_prompts_v1.py`. 프롬프트 `art/mascot/prompts/`, 이 페이지 · JPG · 비교 그림은 `art/mascot/publish.py`. 원본 PNG `art/mascot/raw/`(버린 첫 판은 `raw/rejected/`)는 git 에 올리지 않는다.",
        "- 그리기는 생물 콘셉트와 같은 `art/tools/run_codex_job.sh` — 그림 하나에 codex 세션 하나, 자기 세션 폴더의 그림인지 비교한다. 같은 파이프라인을 쓰는 다른 세션과 섞이지 않게 한 번에 2장까지만 돌렸다.",
        "",
    ]
    with open(OUT_MD, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L))
    print(f"그림 {sum(have.values())}/{len(have)} → {OUT_MD}")


if __name__ == "__main__":
    main()
