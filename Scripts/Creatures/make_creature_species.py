# M3 생물 3종(재칼로프 · 켈피 · 검은 개)의 종 정의 DA_Creature_* 를 만든다 / 고친다. 값은 이 스크립트가 정본이다(스펙 docs/spec/creatures 「M3 생물 3종」).
# 모델은 엔진 기본 도형 조합(몸 · 머리 · 다리 · 뿔 · 꼬리) — 진짜 메시가 오면 BodyParts 를 비우고 메시를 붙인다.
# 색은 sRGB 16진수로 적고 선형으로 바꿔 넣는다(BasicShapeMaterial 의 Color 파라미터는 선형).
#
# 실행(에디터를 닫고, 저장소 루트에서):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Creatures\make_creature_species.py" -unattended -nop4 -nosplash -nullrhi

import unreal

SPECIES_DIR = "/Game/Grimoire/Creatures/Species"

Shape = unreal.GrimCreatureShape
Sex = unreal.GrimCreatureSexFilter


def tag(name):
    result = unreal.GrimCreatureSpecies.request_tag(name)
    if name not in result.export_text():
        raise RuntimeError("GameplayTag 없음: " + name)
    return result


def tags(*names):
    for n in names:
        tag(n)
    return unreal.GrimCreatureSpecies.make_tag_container(list(names))


def srgb(hex_text):
    """'#RRGGBB' → 선형 LinearColor"""
    h = hex_text.lstrip("#")
    def lin(c):
        c = c / 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return unreal.LinearColor(lin(int(h[0:2], 16)), lin(int(h[2:4], 16)), lin(int(h[4:6], 16)), 1.0)


def palette(*hexes):
    return [srgb(h) for h in hexes]


def region(name, base, male=(), female=(), jitter=True):
    r = unreal.GrimCreatureColorRegion()
    r.set_editor_property("region_name", name)
    r.set_editor_property("base_palette", palette(*base))
    r.set_editor_property("male_palette", palette(*male))
    r.set_editor_property("female_palette", palette(*female))
    r.set_editor_property("jitter", jitter)
    return r


def part(name, shape, loc, scale, color, rot=(0.0, 0.0, 0.0), sex=Sex.ANY, visible_when_hidden=False):
    p = unreal.GrimCreatureBodyPart()
    p.set_editor_property("part_name", name)
    p.set_editor_property("shape", shape)
    p.set_editor_property("location", unreal.Vector(*loc))
    # rot = (pitch, yaw, roll)
    p.set_editor_property("rotation", unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    p.set_editor_property("scale", unreal.Vector(*scale))
    p.set_editor_property("color_region", color)
    p.set_editor_property("sex", sex)
    p.set_editor_property("visible_when_hidden", visible_when_hidden)
    return p


def attack(name, element, damage, rng, cooldown, knockback, stimulus, weight=1.0):
    a = unreal.GrimCreatureAttack()
    a.set_editor_property("attack_name", name)
    a.set_editor_property("element", tag(element))
    a.set_editor_property("damage", damage)
    a.set_editor_property("range", rng)
    a.set_editor_property("cooldown", cooldown)
    a.set_editor_property("knockback", knockback)
    a.set_editor_property("stimulus_intensity", stimulus)
    a.set_editor_property("weight", weight)
    return a


def loot(item_id, min_count, max_count, chance):
    l = unreal.GrimCreatureLootEntry()
    l.set_editor_property("item_id", item_id)
    l.set_editor_property("min_count", min_count)
    l.set_editor_property("max_count", max_count)
    l.set_editor_property("chance", chance)
    return l


# ───────────────────────── 재칼로프 ─────────────────────────
# 소형 · 초식 · 겁 많음. 주 바람(뿔 돌진 넉백) · 부 풀(초원 · 밭 둘레, 풀씨를 먹고 퍼뜨린다). 낮, 무리 3~6. 캡슐 반지름 22 · 반높이 22.
JACKALOPE = dict(
    asset="DA_Creature_Jackalope",
    species_id="Jackalope",
    display_name="재칼로프",
    origin="북미 민담 — 사슴뿔 난 토끼",
    primary_element="Element.Wind",
    secondary_element="Element.Earth.Plant",
    size_class=unreal.GrimCreatureSizeClass.SMALL,
    ability_tags=["Creature.Ability.SeedSpread"],
    max_health=30.0, power=1.0, walk_speed=220.0, run_speed=650.0,
    temperament=unreal.GrimCreatureTemperament.TIMID,
    perception_radius=1400.0, aggro_radius=0.0, flee_health_ratio=0.0,
    attacks=[
        attack("뿔 돌진", "Element.Wind", 4.0, 90.0, 2.5, 500.0, 0.6, 2.0),
        attack("풀씨 뿌리기", "Element.Earth.Plant", 2.0, 150.0, 4.0, 0.0, 0.5, 1.0),
    ],
    diet=unreal.GrimCreatureDiet.HERBIVORE,
    activity=unreal.GrimCreatureActivity.DIURNAL,
    hide_outside_active_time=False, avoids_light=False,
    herd_size_min=3, herd_size_max=6,
    habitat_tags=["Habitat.Meadow", "Habitat.Field"],
    water_movement=unreal.GrimCreatureWaterMovement.SWIM,
    loot=[loot("JackalopeMeat", 1, 2, 1.0), loot("JackalopeFur", 1, 1, 0.7), loot("JackalopeAntler", 1, 1, 0.35)],
    color_regions=[
        region("털(등)", ["#8A6A4A", "#9C8263", "#6E5A48", "#B59B7A", "#7D7468", "#5B4636"]),
        region("배 · 꼬리", ["#E8DCC6", "#D9C9AE", "#F2EBDD", "#C9B89C"]),
        region("뿔", ["#CDBB9A", "#B8A47F", "#E0D3B8", "#A89270"], male=["#8E7652", "#7A6342", "#A0875F", "#6B563A"]),
        region("눈", ["#1C1410", "#2A1C12"], jitter=False),
    ],
    mutant_palette=palette("#F5F5F5", "#2B2B2B", "#A8C8E8", "#E8A8C0"),
    male_ratio=0.5, giant_chance=0.02,
    capsule_radius=22.0, capsule_half_height=22.0,
    body_parts=[
        part("몸", Shape.SPHERE, (0, 0, -2), (0.46, 0.28, 0.28), 0),
        part("배", Shape.SPHERE, (2, 0, -8), (0.36, 0.22, 0.17), 1),
        part("머리", Shape.SPHERE, (22, 0, 9), (0.2, 0.17, 0.17), 0),
        part("귀L", Shape.CUBE, (19, -5, 23), (0.03, 0.045, 0.16), 0, rot=(0, 0, -10)),
        part("귀R", Shape.CUBE, (19, 5, 23), (0.03, 0.045, 0.16), 0, rot=(0, 0, 10)),
        part("눈L", Shape.SPHERE, (30, -5, 12), (0.035, 0.035, 0.035), 3),
        part("눈R", Shape.SPHERE, (30, 5, 12), (0.035, 0.035, 0.035), 3),
        part("앞다리L", Shape.CUBE, (12, -7, -16), (0.06, 0.06, 0.13), 0),
        part("앞다리R", Shape.CUBE, (12, 7, -16), (0.06, 0.06, 0.13), 0),
        part("뒷다리L", Shape.CUBE, (-12, -9, -15), (0.13, 0.07, 0.14), 0),
        part("뒷다리R", Shape.CUBE, (-12, 9, -15), (0.13, 0.07, 0.14), 0),
        part("꼬리", Shape.SPHERE, (-25, 0, 3), (0.1, 0.1, 0.1), 1),
        # 수컷: 갈라진 큰 뿔 / 암컷: 작은 뿔 돌기(성별 부위)
        part("뿔L", Shape.CYLINDER, (20, -6, 27), (0.025, 0.025, 0.2), 2, rot=(0, 0, -25), sex=Sex.MALE_ONLY),
        part("뿔R", Shape.CYLINDER, (20, 6, 27), (0.025, 0.025, 0.2), 2, rot=(0, 0, 25), sex=Sex.MALE_ONLY),
        part("뿔가지L", Shape.CONE, (22, -10, 33), (0.04, 0.04, 0.08), 2, rot=(-30, 0, -10), sex=Sex.MALE_ONLY),
        part("뿔가지R", Shape.CONE, (22, 10, 33), (0.04, 0.04, 0.08), 2, rot=(-30, 0, 10), sex=Sex.MALE_ONLY),
        part("뿔돌기L", Shape.CONE, (21, -4, 20), (0.04, 0.04, 0.07), 2, sex=Sex.FEMALE_ONLY),
        part("뿔돌기R", Shape.CONE, (21, 4, 20), (0.04, 0.04, 0.07), 2, sex=Sex.FEMALE_ONLY),
    ],
    tameable=True, taming_circle=1, preferred_pact_element="Element.Wind", mountable=False,
)

# ───────────────────────── 켈피 ─────────────────────────
# 중형 · 초식 · 탈것 · 온순. 주 물(물을 뿜어 밀어내고 적신다) · 부 바람(개울 · 연못 가, 빠르게 달린다). 낮 · 해 질 녘, 물가 2~3. 캡슐 반지름 45 · 반높이 85.
KELPIE = dict(
    asset="DA_Creature_Kelpie",
    species_id="Kelpie",
    display_name="켈피",
    origin="스코틀랜드 민담 — 물가에 사는 물말",
    primary_element="Element.Water",
    secondary_element="Element.Wind",
    size_class=unreal.GrimCreatureSizeClass.MEDIUM,
    ability_tags=["Creature.Ability.WaterRun"],
    max_health=90.0, power=1.2, walk_speed=250.0, run_speed=900.0,
    temperament=unreal.GrimCreatureTemperament.DOCILE,
    perception_radius=1500.0, aggro_radius=0.0, flee_health_ratio=0.0,
    attacks=[
        attack("물 뿜기", "Element.Water", 8.0, 400.0, 3.0, 700.0, 1.0, 2.0),
        attack("돌풍 걷어차기", "Element.Wind", 6.0, 120.0, 2.0, 900.0, 0.8, 1.0),
    ],
    diet=unreal.GrimCreatureDiet.HERBIVORE,
    activity=unreal.GrimCreatureActivity.DAY_AND_DUSK,
    hide_outside_active_time=False, avoids_light=False,
    herd_size_min=2, herd_size_max=3,
    habitat_tags=["Habitat.Waterside"],
    water_movement=unreal.GrimCreatureWaterMovement.RUN_ON_SURFACE,
    # 고기는 안 됨(물에 녹는 몸 — 스펙 제안)
    loot=[loot("KelpieMane", 1, 2, 1.0), loot("KelpieWaterEssence", 1, 1, 0.5)],
    color_regions=[
        region("몸", ["#2F4A5C", "#3B5F6E", "#24343F", "#4C6B73", "#1E2A30", "#56707A"]),
        region("갈기", ["#2E4A3A", "#1F3329", "#3F5E4B", "#203038"], male=["#16261E", "#2A4A36", "#0F1A15", "#3A6248"]),
        region("발굽", ["#2A2A2A", "#3A3530", "#1E1E1E"]),
        region("눈", ["#9FD8E8", "#7CC2D8"], jitter=False),
    ],
    mutant_palette=palette("#E8F4F8", "#C8A040", "#7A2E5A", "#40C890"),
    male_ratio=0.5, giant_chance=0.02,
    capsule_radius=45.0, capsule_half_height=85.0,
    body_parts=[
        part("몸통", Shape.SPHERE, (0, 0, 5), (1.3, 0.52, 0.55), 0),
        part("목", Shape.CYLINDER, (52, 0, 42), (0.22, 0.22, 0.62), 0, rot=(-35, 0, 0)),
        part("머리", Shape.CUBE, (82, 0, 72), (0.46, 0.2, 0.24), 0, rot=(25, 0, 0)),
        part("눈L", Shape.SPHERE, (88, -10, 80), (0.05, 0.05, 0.05), 3),
        part("눈R", Shape.SPHERE, (88, 10, 80), (0.05, 0.05, 0.05), 3),
        part("앞다리L", Shape.CYLINDER, (42, -17, -45), (0.12, 0.12, 0.72), 0),
        part("앞다리R", Shape.CYLINDER, (42, 17, -45), (0.12, 0.12, 0.72), 0),
        part("뒷다리L", Shape.CYLINDER, (-42, -17, -45), (0.13, 0.13, 0.72), 0),
        part("뒷다리R", Shape.CYLINDER, (-42, 17, -45), (0.13, 0.13, 0.72), 0),
        part("발굽FL", Shape.CUBE, (44, -17, -82), (0.15, 0.14, 0.07), 2),
        part("발굽FR", Shape.CUBE, (44, 17, -82), (0.15, 0.14, 0.07), 2),
        part("발굽BL", Shape.CUBE, (-40, -17, -82), (0.15, 0.14, 0.07), 2),
        part("발굽BR", Shape.CUBE, (-40, 17, -82), (0.15, 0.14, 0.07), 2),
        part("꼬리", Shape.CONE, (-72, 0, 10), (0.16, 0.16, 0.6), 1, rot=(-150, 0, 0)),
        # 수컷: 길게 늘어진 해초 갈기 / 암컷: 짧은 갈기(성별 부위)
        part("갈기(긴)", Shape.CUBE, (44, 0, 58), (0.7, 0.06, 0.3), 1, rot=(-35, 0, 0), sex=Sex.MALE_ONLY),
        part("갈기(짧은)", Shape.CUBE, (48, 0, 58), (0.5, 0.05, 0.14), 1, rot=(-35, 0, 0), sex=Sex.FEMALE_ONLY),
    ],
    tameable=True, taming_circle=2, preferred_pact_element="Element.Water", mountable=True,
)

# ───────────────────────── 검은 개 ─────────────────────────
# 중형 · 육식 · 공격적. 주 그림자술(그림자에 숨었다가 덮친다) · 부 불(밤에만, 불씨 눈, 모닥불을 피한다). 밤, 혼자 또는 둘. 캡슐 반지름 35 · 반높이 45.
BLACK_DOG = dict(
    asset="DA_Creature_BlackDog",
    species_id="BlackDog",
    display_name="검은 개",
    origin="영국 민담 — 밤길에 나타나는 불타는 눈의 검은 개(블랙 독)",
    primary_element="Element.Dark.Shadow",
    secondary_element="Element.Fire",
    size_class=unreal.GrimCreatureSizeClass.MEDIUM,
    ability_tags=["Creature.Ability.ShadowStep"],
    max_health=70.0, power=1.4, walk_speed=300.0, run_speed=800.0,
    temperament=unreal.GrimCreatureTemperament.AGGRESSIVE,
    perception_radius=1500.0, aggro_radius=1000.0, flee_health_ratio=0.15,
    attacks=[
        attack("그림자 물기", "Element.Dark.Shadow", 9.0, 80.0, 1.6, 200.0, 0.6, 3.0),
        attack("불씨 눈빛", "Element.Fire", 6.0, 350.0, 4.0, 0.0, 1.0, 1.0),
    ],
    diet=unreal.GrimCreatureDiet.CARNIVORE,
    activity=unreal.GrimCreatureActivity.NOCTURNAL,
    hide_outside_active_time=True, avoids_light=True,
    herd_size_min=1, herd_size_max=2,
    habitat_tags=["Habitat.Road", "Habitat.Meadow"],
    water_movement=unreal.GrimCreatureWaterMovement.SWIM,
    loot=[loot("BlackDogHide", 1, 1, 1.0), loot("BlackDogFang", 1, 2, 0.6)],
    color_regions=[
        region("털", ["#141214", "#1C1A1E", "#0C0B0D", "#221E1C", "#18161A"]),
        region("눈(불씨)", ["#FF7A1A", "#FF4A10", "#FFB030", "#E83A0A"]),
        region("목털", ["#1A1618", "#26201E", "#100E10"], male=["#2E2420", "#1E1814", "#3A2C24"]),
        region("발", ["#0E0D0E", "#1A1716", "#24201C"]),
    ],
    mutant_palette=palette("#D8D8E0", "#5A1A10", "#2A3A6A", "#6AE8FF"),
    male_ratio=0.5, giant_chance=0.02,
    capsule_radius=35.0, capsule_half_height=45.0,
    body_parts=[
        part("몸통", Shape.SPHERE, (0, 0, 2), (0.95, 0.38, 0.4), 0),
        part("가슴", Shape.SPHERE, (30, 0, 8), (0.42, 0.36, 0.42), 0),
        part("머리", Shape.CUBE, (52, 0, 28), (0.32, 0.24, 0.24), 0),
        part("주둥이", Shape.CUBE, (70, 0, 22), (0.2, 0.14, 0.12), 0),
        part("귀L", Shape.CONE, (46, -9, 44), (0.08, 0.06, 0.14), 0),
        part("귀R", Shape.CONE, (46, 9, 44), (0.08, 0.06, 0.14), 0),
        # 불씨 눈 — 그림자 걸음 중에도 보인다
        part("눈L", Shape.SPHERE, (64, -7, 32), (0.05, 0.05, 0.05), 1, visible_when_hidden=True),
        part("눈R", Shape.SPHERE, (64, 7, 32), (0.05, 0.05, 0.05), 1, visible_when_hidden=True),
        part("앞다리L", Shape.CYLINDER, (30, -12, -26), (0.1, 0.1, 0.4), 0),
        part("앞다리R", Shape.CYLINDER, (30, 12, -26), (0.1, 0.1, 0.4), 0),
        part("뒷다리L", Shape.CYLINDER, (-32, -12, -26), (0.11, 0.11, 0.4), 0),
        part("뒷다리R", Shape.CYLINDER, (-32, 12, -26), (0.11, 0.11, 0.4), 0),
        part("발FL", Shape.SPHERE, (32, -12, -43), (0.1, 0.09, 0.05), 3),
        part("발FR", Shape.SPHERE, (32, 12, -43), (0.1, 0.09, 0.05), 3),
        part("발BL", Shape.SPHERE, (-30, -12, -43), (0.1, 0.09, 0.05), 3),
        part("발BR", Shape.SPHERE, (-30, 12, -43), (0.1, 0.09, 0.05), 3),
        part("꼬리", Shape.CYLINDER, (-55, 0, 14), (0.06, 0.06, 0.45), 0, rot=(-60, 0, 0)),
        # 수컷: 덥수룩한 목털 / 암컷: 가는 목털(성별 부위)
        part("목털(큰)", Shape.SPHERE, (38, 0, 22), (0.42, 0.44, 0.36), 2, sex=Sex.MALE_ONLY),
        part("목털(작은)", Shape.SPHERE, (38, 0, 20), (0.3, 0.34, 0.24), 2, sex=Sex.FEMALE_ONLY),
    ],
    tameable=True, taming_circle=3, preferred_pact_element="Element.Dark.Shadow", mountable=False,
)

SPECIES = [JACKALOPE, KELPIE, BLACK_DOG]

ENUM_FIELDS = ("size_class", "temperament", "diet", "activity", "water_movement")
FLOAT_FIELDS = ("max_health", "power", "walk_speed", "run_speed", "perception_radius", "aggro_radius", "flee_health_ratio",
                "male_ratio", "giant_chance", "capsule_radius", "capsule_half_height")
INT_FIELDS = ("herd_size_min", "herd_size_max", "taming_circle")
BOOL_FIELDS = ("hide_outside_active_time", "avoids_light", "tameable", "mountable")
LIST_FIELDS = ("attacks", "loot", "color_regions", "mutant_palette", "body_parts")


def make_or_load(asset):
    path = SPECIES_DIR + "/" + asset
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.GrimCreatureSpecies)
    return tools.create_asset(asset, SPECIES_DIR, unreal.GrimCreatureSpecies, factory)


def apply(species, values):
    species.set_editor_property("species_id", values["species_id"])
    species.set_editor_property("display_name", unreal.Text(values["display_name"]))
    species.set_editor_property("origin", unreal.Text(values["origin"]))
    species.set_editor_property("primary_element", tag(values["primary_element"]))
    species.set_editor_property("secondary_element", tag(values["secondary_element"]))
    species.set_editor_property("preferred_pact_element", tag(values["preferred_pact_element"]))
    species.set_editor_property("ability_tags", tags(*values["ability_tags"]))
    species.set_editor_property("habitat_tags", tags(*values["habitat_tags"]))
    for key in ENUM_FIELDS + BOOL_FIELDS + LIST_FIELDS:
        species.set_editor_property(key, values[key])
    for key in FLOAT_FIELDS:
        species.set_editor_property(key, float(values[key]))
    for key in INT_FIELDS:
        species.set_editor_property(key, int(values[key]))


def main():
    unreal.EditorAssetLibrary.make_directory(SPECIES_DIR)
    for values in SPECIES:
        species = make_or_load(values["asset"])
        if species is None:
            raise RuntimeError("에셋을 만들지 못함: " + values["asset"])
        apply(species, values)
        if not unreal.EditorAssetLibrary.save_loaded_asset(species, only_if_is_dirty=False):
            raise RuntimeError("저장 실패: " + values["asset"])
        unreal.log("[make_creature_species] %s 저장 — 부위 %d · 색 영역 %d · 공격 %d · 전리품 %d" % (
            values["asset"], len(values["body_parts"]), len(values["color_regions"]), len(values["attacks"]), len(values["loot"])))


main()
