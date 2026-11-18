# 초원 꾸밈 표 셋을 만든다(사람이 고치기 쉽게 — 값을 바꾸면 이 파일을 고치고 다시 돌린다. CSV 를 손으로 고쳐도 된다).
#   python Scripts/Dressing/make_meadow_tables.py
#   → Content/Grimoire/Data/ReactiveProps.csv · HarvestNodes.csv · Journal.csv (게임이 런타임에 읽는다 — UGrimDressingSettings)
# 줍기 · 보상 · 재생성 · 불 번짐 값은 기획안 제안을 쓴 **잠정값**이다(Q151~Q157 미결 — docs/spec/meadow-dressing.md).
import csv
import io
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
DATA = os.path.join(ROOT, "Content", "Grimoire", "Data")
SHG = "/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Grass_Flowers_Foliage"
SHF = "/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample"
SHR = "/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Rocks_and_Stones"
D = "/Game/Grimoire/Dressing"
EPR = "/Game/EnchantedPotionRoom/Meshes/Props"


def a(p):
    return f"{p}.{p.rsplit('/', 1)[1]}" if p else ""


def v(x, y, z):
    return f"(X={x},Y={y},Z={z})"


def r(p, yw, rl):
    return f"(Pitch={p},Yaw={yw},Roll={rl})"


PROP_COLS = ["Name", "Note", "DisplayName", "Mesh", "MeshMaterial", "MeshScale", "MeshOffset", "MeshRotation", "CullDistance", "bBlocksPawn", "bTargetable",
             "HitExtent", "HitOffset", "Materials", "InitialState", "InitialIntensity", "AltState", "AltMesh", "AltMaterial", "AltScale", "AltRotation",
             "bAltUsesRotation", "bAltHidesBody", "AccentMesh", "AccentMaterial", "AccentScale", "AccentOffset", "AccentShow", "LoopFX", "LoopFXOffset",
             "LoopFXWhen", "LoopFXHideState", "BurstFX", "BurstState", "FlameOffset", "FlameScale", "LightCandela", "LightRadius", "LightMaxDrawDistance",
             "RewardState", "Rewards", "NothingWeight", "RewardOffset", "ResetRule", "ResetSeconds", "DriftState", "DriftDistance", "KickState", "KickHeight",
             "KickSeconds", "JournalId"]
BASE = dict(Note="", DisplayName="", Mesh="", MeshMaterial="", MeshScale=v(1, 1, 1), MeshOffset=v(0, 0, 0), MeshRotation=r(0, 0, 0), CullDistance=12000,
            bBlocksPawn=False, bTargetable=True, HitExtent=v(30, 30, 30), HitOffset=v(0, 0, 30), Materials="()", InitialState="None", InitialIntensity=1,
            AltState="None", AltMesh="", AltMaterial="", AltScale=v(0, 0, 0), AltRotation=r(0, 0, 0), bAltUsesRotation=False, bAltHidesBody=False,
            AccentMesh="", AccentMaterial="", AccentScale=v(1, 1, 1), AccentOffset=v(0, 0, 0), AccentShow="Always", LoopFX="", LoopFXOffset=v(0, 0, 0),
            LoopFXWhen="Always", LoopFXHideState="None", BurstFX="", BurstState="None", FlameOffset=v(0, 0, 30), FlameScale=v(0.05, 0.05, 0.1),
            LightCandela=0, LightRadius=500, LightMaxDrawDistance=3000, RewardState="None", Rewards="", NothingWeight=0, RewardOffset=v(60, 0, 40),
            ResetRule="Never", ResetSeconds=0, DriftState="None", DriftDistance="(X=200,Y=500)", KickState="None", KickHeight=60, KickSeconds=1.2,
            JournalId="None")

PROPS = [
    ("D01_WindCotton", dict(
        Note="바람솜꽃(D01): 바람에 솜이 흩날리고 솜씨앗이 떨어진다. 솜이 빠지면 줄기만 — 다음 아침에 다시 난다(잠정 Q153)", DisplayName="바람솜꽃",
        Mesh=a(f"{D}/Meshes/SM_Meadow_CottonStems"), MeshScale=v(1.2, 1.2, 1.2), HitOffset=v(0, 0, 35), Materials="(Material.Seedhead)",
        AltState="State.Spent", AccentMesh=a(f"{D}/Meshes/SM_Meadow_CottonPuffs"), AccentScale=v(1.2, 1.2, 1.2), AccentShow="Default",
        BurstFX=a(f"{D}/FX/NS_Meadow_CottonBurst"), BurstState="State.Spent", LoopFXOffset=v(0, 0, 55),
        RewardState="State.Spent", Rewards="WindCotton:1-2:1", RewardOffset=v(45, 0, 30), ResetRule="Dawn")),
    ("D02_DryGrass", dict(
        Note="마른 풀 더미(D02): 불에 8초 타며 2.5 m 번진다(Grass_Ignite). 다 타면 재 자국 밑에서 돌멩이 · 재가 드러난다(빈손 1/5). 다음 아침에 다시 자란다(잠정 Q153 · Q154)",
        DisplayName="마른 풀 더미", Mesh=a(f"{SHG}/Meshes/SM_Fol_Grass_Clump_2"), MeshMaterial=a(f"{D}/Materials/MI_Meadow_DryGrass"), MeshScale=v(1.1, 1.1, 1.3),
        HitExtent=v(50, 50, 25), HitOffset=v(0, 0, 25), Materials="(Material.Flammable,Material.Consumable)", AltState="State.Charred", bAltHidesBody=True,
        AccentMesh=a(f"{D}/Meshes/SM_Meadow_AshPatch"), AccentShow="Alt", FlameOffset=v(0, 0, 30), FlameScale=v(0.6, 0.6, 0.4), LightCandela=60, LightRadius=800,
        RewardState="State.Charred", Rewards="Stone:1:4|Ash:1-2:4", NothingWeight=2, RewardOffset=v(0, 0, 20), ResetRule="Dawn")),
    ("D03_ThirstyFlower", dict(
        Note="목마른 들꽃(D03): 고개 숙인 시든 꽃. 물(비 · 물 마법)을 받으면 일어나 피고 나비가 오며 들꽃잎이 떨어진다. 다음 아침에 다시 시든다", DisplayName="목마른 들꽃",
        Mesh=a(f"{SHF}/Meshes/Foliage/SM_Fol_Flower_Clump_3"), MeshMaterial=a(f"{D}/Materials/MI_Meadow_Wilted"), MeshScale=v(1.0, 1.0, 0.35), MeshRotation=r(-28, 0, 22),
        HitExtent=v(30, 30, 20), HitOffset=v(0, 0, 15), Materials="(Material.Thirsty)", InitialState="State.Wilted", AltState="State.Bloomed",
        AltMaterial=a(f"{SHF}/Materials/Mat_Foliage/MI_Fol_Flower_Atlas"), AltScale=v(1.25, 1.25, 1.25), AltRotation=r(0, 0, 0), bAltUsesRotation=True,
        LoopFX=a(f"{D}/FX/NS_Meadow_Butterflies"), LoopFXOffset=v(0, 0, 60), LoopFXWhen="Alt",
        RewardState="State.Bloomed", Rewards="WildPetal:1-2:1", RewardOffset=v(45, 0, 20), ResetRule="Dawn")),
    ("D04_StoneLamp", dict(
        Note="돌 등잔(D04): 불 마법으로 켜고 돌풍 · 물 · 비에 꺼진다. 아침이면 저절로 꺼진다. 빛 그림자 끔 · 30 m 에서 빛 끔(성능 규칙 23). 검은 개와의 관계는 2차",
        DisplayName="돌 등잔", Mesh=a(f"{D}/Meshes/SM_Meadow_StoneLamp"), MeshScale=v(0.85, 0.85, 0.85), bBlocksPawn=True, HitExtent=v(14, 14, 25), HitOffset=v(0, 0, 25),
        Materials="(Material.Flammable)", FlameOffset=v(0, 0, 26), FlameScale=v(0.035, 0.035, 0.08), LightCandela=40, LightRadius=900, ResetRule="Dawn")),
    ("D16_Butterflies", dict(
        Note="낮 나비(D16): 꽃밭 위를 난다(낮만). 바람에 흩어졌다 20초 뒤 돌아온다. EFT_Grim_Ambient — 곳당 입자 16 이하", DisplayName="나비",
        bTargetable=False, HitExtent=v(80, 80, 60), HitOffset=v(0, 0, 100), Materials="(Material.Swarm)", LoopFX=a(f"{D}/FX/NS_Meadow_Butterflies"),
        LoopFXOffset=v(0, 0, 90), LoopFXWhen="Day", LoopFXHideState="State.Scattered")),
    ("D20_Campfire", dict(
        Note="식은 모닥불 자리(D20): 불 마법으로 다시 탄다 — 곁에 있으면 추위가 풀리고(생존 열원 규칙) 날것이 익는다. 비 · 물에 꺼진다. 되돌리지 않는다",
        DisplayName="식은 모닥불", Mesh=a(f"{EPR}/SM_CampfireStone"), MeshScale=v(0.75, 0.75, 0.75), bBlocksPawn=True, HitExtent=v(45, 45, 20), HitOffset=v(0, 0, 18),
        Materials="(Material.Flammable)", AccentMesh=a(f"{D}/Meshes/SM_Meadow_FireLogs"), AccentShow="Always", FlameOffset=v(0, 0, 30), FlameScale=v(0.32, 0.32, 0.5),
        LightCandela=120, LightRadius=1500)),
    ("D21_Page", dict(
        Note="흩날리는 책장(D21): F 로 주우면 도감에 한 줄(사람마다 한 번 — 잠정 Q153). 센 바람에 2.5~6 m 날려 간다. 쪽마다 다른 줄은 배치 스크립트가 JournalOverride 로",
        DisplayName="낡은 책장", Mesh=a(f"{EPR}/SM_Paper_01"), MeshScale=v(0.55, 0.55, 0.55), MeshOffset=v(0, 0, 10), MeshRotation=r(10, 0, -6), CullDistance=8000,
        HitExtent=v(18, 18, 12), HitOffset=v(0, 0, 12), Materials="(Material.Paper)", DriftState="State.Blown", DriftDistance="(X=250,Y=600)", JournalId="Page_01")),
    ("D29_FloatStone", dict(
        Note="떠 있는 돌멩이(D29): 땅 위 30~80 cm 에서 천천히 돈다(WPO — 틱 없음). 충격(기본 공격)에 튕겼다 돌아온다. 부유석 언덕에 가까울수록 많다",
        DisplayName="떠 있는 돌", Mesh=a(f"{SHR}/Meshes/SM_Stone_Small_1"), MeshMaterial=a(f"{D}/Materials/M_Meadow_FloatStone"), MeshScale=v(0.3, 0.3, 0.28),
        HitExtent=v(16, 16, 16), HitOffset=v(0, 0, 0), Materials="(Material.Floating)", KickState="State.Struck", KickHeight=70, KickSeconds=1.2)),
]

HARVEST_COLS = ["Name", "Note", "DisplayName", "Mesh", "MeshMaterial", "CullDistance", "Item", "CountMin", "CountMax", "HitsToHarvest", "Ownership", "ResetRule",
                "ResetSeconds", "InteractRadius"]
HARVEST = [(f"D10_Branch{i}", dict(
    Note="떨어진 곁가지(D10): 큰 나무 밑. F 로 줍는다 → 나무. 세계 공유 · 다음 아침에 다시 떨어진다(잠정 Q151 · Q153)", DisplayName="곁가지",
    Mesh=a(f"{D}/Meshes/SM_Meadow_Twig{i}"), Item="Wood")) for i in (1, 2, 3)] + [(f"D10_Pebble{i}", dict(
        Note="돌멩이(D10): 바위 옆. F 로 줍는다 → 돌. 세계 공유 · 다음 아침에 다시(잠정 Q151 · Q153)", DisplayName="돌멩이",
        Mesh=a(f"{SHR}/Meshes/{mesh}"), Item="Stone")) for i, mesh in ((1, "SM_Stone_Pebble_1"), (2, "SM_Stone_Pebble_2"))]
HARVEST_BASE = dict(MeshMaterial="", CullDistance=6000, CountMin=1, CountMax=1, HitsToHarvest=1, Ownership="Shared", ResetRule="Dawn", ResetSeconds=0, InteractRadius=220)

# 책장 20 쪽 — 잠정(Q157 「이 들판에 누가 살았나」: 탑을 드나들던 길손과 떠난 농가 + 사냥꾼의 옅은 복선). 톤은 밝게
PAGES = [
    ("들판 일지 1", "탑에서 내려오는 길손은 늘 같은 굽이에서 쉬어 갔다. 그 자리에 등잔을 둔 것도 그들이다."),
    ("들판 일지 2", "바람솜꽃은 바람 마법을 처음 배운 아이들의 장난감이었다. 솜이 산 아래로 날아가면 소원이 이루어진다고 했다."),
    ("들판 일지 3", "올해 밀은 일찍 누렇게 익었다. 풍차 집 사람들은 가을이 오기 전에 떠났다 — 어디로 갔는지는 쓰여 있지 않다."),
    ("들판 일지 4", "비가 오면 들꽃이 일어선다. 물 마법을 배우는 견습생들은 비 오는 날을 싫어했다 — 할 일이 없어지니까."),
    ("들판 일지 5", "탑의 남색 깃발이 보이면 길을 잃은 게 아니다. 깃발이 안 보이면 등잔 불빛을 따라가라."),
    ("들판 일지 6", "떠 있는 돌은 언덕 위 부유석에서 떨어져 나온 조각이라고 한다. 가까이 갈수록 더 많이 떠 있다."),
    ("들판 일지 7", "모닥불 자리에 앉아 본 사람은 안다. 이 들판의 밤은 생각보다 춥다."),
    ("들판 일지 8", "재칼로프는 마른 풀 타는 냄새를 싫어한다. 풀을 태운 자리 근처에는 굴을 파지 않는다."),
    ("들판 일지 9", "돌멩이 몇 개와 곁가지 하나 — 견습생이 탑을 나서서 처음 만드는 도구는 늘 똑같다."),
    ("들판 일지 10", "나비는 해가 있을 때만 날아온다. 밤의 들판에는 다른 것이 난다고 들었다."),
    ("들판 일지 11", "등잔 기름이 떨어져도 걱정하지 마라. 불 마법 한 번이면 다시 켜진다. 아침이 오면 저절로 꺼진다."),
    ("들판 일지 12", "누군가 개울가에 덫을 놓고 갔다. 탑 사람은 아닐 것이다. 탑은 덫을 쓰지 않는다."),
    ("들판 일지 13", "봉화 언덕에 불이 오르면 들판 사람들은 탑으로 모였다. 마지막으로 불이 오른 것은 오래전이다."),
    ("들판 일지 14", "마른 풀 더미 밑에서 무언가를 찾았다. 풀을 태우면 숨은 것이 드러난다 — 다만 바람 부는 날은 조심할 것."),
    ("들판 일지 15", "책장이 바람에 날려 가는 걸 쫓아갔다가 들판을 반이나 걸었다. 덕분에 길을 다 외웠다."),
    ("들판 일지 16", "풍차는 아직도 돈다. 돌릴 사람이 없는데도. 바람이 대신 일하는 셈이다."),
    ("들판 일지 17", "탑의 학자들은 이 들판을 「첫 교실」이라 불렀다. 불 · 물 · 바람을 처음 써 보는 곳이라서."),
    ("들판 일지 18", "꽃나무 아래에서 낮잠을 잤다. 꽃잎이 얼굴에 내려앉아 눈을 떴다."),
    ("들판 일지 19", "길손의 돌무더기에 돌 하나를 얹고 가면 다음 사람이 길을 잃지 않는다."),
    ("들판 일지 20", "이 쪽이 마지막이다. 나머지는 바람에 날려 갔다 — 들판 어딘가에 아직 남아 있을 것이다."),
]


def cell(value):
    if value is True:
        return "True"
    if value is False:
        return "False"
    return value


def write(name, cols, rows):
    buf = io.StringIO()
    w = csv.writer(buf, lineterminator="\r\n")
    w.writerow(cols)
    for row_name, values in rows:
        values = dict(values, Name=row_name)
        w.writerow([cell(values[c]) for c in cols])
    with open(os.path.join(DATA, name), "w", encoding="utf-8", newline="") as f:
        f.write(buf.getvalue())
    print(f"{name}: {len(rows)} 행")


write("ReactiveProps.csv", PROP_COLS, [(n, dict(BASE, **kw)) for n, kw in PROPS])
write("HarvestNodes.csv", HARVEST_COLS, [(n, dict(HARVEST_BASE, **kw)) for n, kw in HARVEST])
write("Journal.csv", ["Name", "Group", "Title", "Text"], [(f"Page_{i:02d}", dict(Group="Page", Title=t, Text=x)) for i, (t, x) in enumerate(PAGES, 1)])
