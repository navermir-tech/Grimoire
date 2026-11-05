# 스킬북 목록(Content/Grimoire/Data/Skillbooks.csv)을 docs/spec/skillbooks.md 의 표로 뽑는다.
# 목록의 정본은 CSV 다 — CSV 를 고치면 이것을 돌려 문서의 「목록」 부분을 바꿔 붙인다.
#
#   python -X utf8 Scripts/Magic/skillbooks_md.py            # 속성별 표(문서 붙여넣기용)
#   python -X utf8 Scripts/Magic/skillbooks_md.py --summary  # 갈래 · 서클 · M7 개수

import csv
import sys
from collections import Counter
from pathlib import Path

CSV_PATH = Path(__file__).resolve().parents[2] / "Content/Grimoire/Data/Skillbooks.csv"

# (태그, 제목) — 문서에 나오는 순서
GROUPS = [
    ("Normal", None, "무속성 — 노말 스킬"),
    ("Pact", None, "계약 — 파크툼"),
    ("Spell", "Element.Fire", "불 — 이그니스"),
    ("Spell", "Element.Water", "물 — 아쿠아"),
    ("Spell", "Element.Water.Ice", "얼음 — 글라키에스"),
    ("Spell", "Element.Wind", "바람 — 벤투스"),
    ("Spell", "Element.Earth", "땅 — 테라"),
    ("Spell", "Element.Earth.Plant", "풀 — 플로라"),
    ("Spell", "Element.Light.Holy", "신성 — 상투스"),
    ("Spell", "Element.Light.Radiance", "빛 — 룩스"),
    ("Spell", "Element.Dark.Hex", "흑마법 — 말레디카"),
    ("Spell", "Element.Dark.Shadow", "그림자술 — 움브라"),
]

DELIVERY = {
    "Bolt": "섬광형", "Stream": "줄기형", "Cone": "원뿔형", "Beam": "광선", "Lob": "포물선",
    "Self": "자신", "Ally": "아군", "Ground": "지점", "Aura": "몸 둘레", "Region": "지역",
}
MANA = {"None": "없음", "Low": "하", "Mid": "중", "High": "상", "Extreme": "최상"}
COOLDOWN = {"None": "없음", "Short": "짧음", "Mid": "보통", "Long": "김", "VeryLong": "아주 김", "Daily": "하루"}
SOURCE = {
    "Tutorial": "튜토리얼", "TowerShop": "탑 서고", "Branch_F": "안개 서원", "Branch_E": "등대탑",
    "Branch_G": "성소탑", "Branch_H": "북탑", "Trial_2": "2서클 승급", "Trial_3": "3서클 승급",
    "Trial_5": "5서클 승급", "Quest": "퀘스트", "Ruin": "유적", "Dungeon": "던전", "Hunter": "사냥꾼",
    "Boss": "필드 보스", "Sanctum": "속성 성소",
}


def load():
    with CSV_PATH.open(encoding="utf-8") as f:
        return list(csv.DictReader(f))


def state(row):
    if row["bImplemented"] == "True":
        return "✅ 있음"
    return "M7" if row["Milestone"] == "M7" else row["Milestone"].replace("Later", "뒤")


def table(rows):
    lines = [
        "| 주문 | 이름 · 별칭 | 서클 | 방식 | 효과 | 마나 · 쿨 | 출처(값) | 상태 |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for r in sorted(rows, key=lambda r: int(r["Circle"])):
        sources = " · ".join(SOURCE.get(s, s) for s in r["Sources"].split("|"))
        price = int(r["PriceSol"])
        if price:
            sources += f" ({price} 솔)"
        lines.append(
            f"| `{r['Incantation']}` | **{r['DisplayName']}** · {r['Epithet']} | {r['Circle']} | "
            f"{DELIVERY[r['Delivery']]} | {r['Effect']} | {MANA[r['ManaClass']]} · {COOLDOWN[r['CooldownClass']]} | "
            f"{sources} | {state(r)} |"
        )
    return "\n".join(lines)


def main():
    rows = load()
    if "--summary" in sys.argv:
        print("갈래:", Counter(r["Kind"] for r in rows), "합", len(rows))
        print("M7 속성 주문:", sum(r["Kind"] == "Spell" and r["Milestone"] in ("M7", "M1", "M2") for r in rows))
        print("서클:", sorted(Counter(int(r["Circle"]) for r in rows).items()))
        print("구현:", [r["SpellId"] for r in rows if r["bImplemented"] == "True"])
        return
    for kind, element, title in GROUPS:
        group = [r for r in rows if r["Kind"] == kind and (element is None or r["Element"] == element)]
        print(f"### {title} ({len(group)})\n")
        print(table(group))
        print()


if __name__ == "__main__":
    main()
