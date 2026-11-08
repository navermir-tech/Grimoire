# 레벨 아트 1차(시작 구역) 에셋 — Content/Grimoire/Terrain/Dress 에 우리 에셋만 만든다. 팩 에셋(Content/<팩>/)은 고치지 않고 참조만 한다.
# dress_openbeta.py 가 부른다. 돌릴 때마다 지우고 새로 만든다(설정이 이 파일 한곳에 있게 — 손으로 고친 값은 여기에 옮겨 적는다).
#
# 팩 · 고른 까닭은 docs/dev/level-design.md 「8. 레벨 아트 1차」:
# - 지형 재질: StyleHex 지형 텍스처(1K, 붓 터치) + 우리 경사 · 높이 · 레이어 규칙(M_Terrain_Auto 와 같은 레이어 Shore · Path · Forest)
# - 풀 · 꽃 · 풀포기: StyleHex Grass Flowers · Forest Sample 메시 → 랜드스케이프 그래스(GT_Dress_Meadow)
# - 나무 · 덤불 · 바위: StyleHex 메시 → 폴리지 타입 FT_Dress_*
# - 구름: HandPaintedClouds 텍스처 → 우리 빛 받는 반투명 머티리얼(M_Dress_CloudCard — 밤에는 어두워진다)
import math

import unreal

DRESS_DIR = "/Game/Grimoire/Terrain/Dress"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

SH = "/Game/StyleHex_Studio"
SH_FOREST = f"{SH}/Free_Packs/FREE_Stylized_Forest_Sample"
SH_GRASS = f"{SH}/Free_Packs/FREE_Stylized_Grass_Flowers_Foliage"
SH_ROCKS = f"{SH}/Free_Packs/FREE_Stylized_Rocks_and_Stones"
SH_TERRAIN_TEX = f"{SH}/Shared_Resources/Free_Packs/Shared_Terrain/Shared_Terrain_Textures/Terrain_Textures_FSBF"

TEX = {
    "Grass": f"{SH_TERRAIN_TEX}/T_Fsbf_Terrain_Grass_1_BC",
    "Dirt": f"{SH_TERRAIN_TEX}/T_Fsbf_Terrain_Dirt_1_BC",
    "Dirt2": f"{SH_TERRAIN_TEX}/T_Fsbf_Terrain_Dirt_2_BC",
    "Rock": f"{SH_ROCKS}/Textures/T_Stone_Large_BC",
    "Macro": f"{SH}/Shared_Resources/Free_Packs/Shared_Textures/Nosies/T_Macro_Variation",
}
CLOUD_TEXTURES = [f"/Game/HandPaintedClouds/Textures/Cloud/ghibli_cloud_0{i}" for i in (1, 2, 3)]

# 손으로 놓는 소품(dress_openbeta.py) — 사실적 스캔 · Megaplants 는 Q124 · Q125 판단 자료로 몇 개만
PROPS = {
    "FallenTrunk": [("/Game/BlackCherryBrokenTrunks01/2_Low/Static_Meshes/SM_BlackCherryBrokenTrunk01_LOW", "static"),
                    ("/Game/BlackCherryBrokenTrunks01/2_Low/Static_Meshes/SM_BlackCherryBrokenTrunk02_LOW", "static")],
    # 스캔 그루터기는 하나에 4K 텍스처 4장(약 60 MB) — 저장소 용량 때문에 한 종류만(크기 · 방향으로 다르게)
    "Stump": [("/Game/Stump_Scanned/Meshes/Stump_2_mesh", "static")],
    "Oak": [("/Game/Megaplant_Library/Tree_English_Oak/Tree_English_Oak_Forest_01/Tree_English_Oak_Forest_01_B", "skeletal"),
            ("/Game/Megaplant_Library/Tree_English_Oak/Tree_English_Oak_Forest_01/Tree_English_Oak_Forest_01_C", "skeletal")],
    "Boulder": [(f"{SH_FOREST}/Meshes/Stones/SM_Stone_Boulder_1", "static"), (f"{SH_FOREST}/Meshes/Stones/SM_Stone_3", "static")],
}

# 색 보정(선형) — 텍스처 × 색. 회화풍 팔레트(docs/spec/art-direction.md)에 맞춰 채도를 조금 낮추고 따뜻하게
TINT = {
    "Grass": (0.95, 1.0, 0.82),
    "Dirt": (1.0, 0.92, 0.80),
    "Path": (1.05, 0.95, 0.80),
    "Rock": (0.85, 0.85, 0.88),
    "Sand": (1.25, 1.15, 0.90),
    "Forest": (0.55, 0.60, 0.45),
}

# 풀(랜드스케이프 그래스) — (메시, 밀도 = 10 m × 10 m(100 m²) 당 개수 — 엔진 GrassDensity 단위, 시작 · 끝 컬 cm, 크기 범위). 규칙 10 · 12(docs/systems/performance.md): 풀 끝 50 m, 덤불 · 꽃 120 m, 시작 = 끝의 70 %
GRASS_VARIETIES = [
    (f"{SH_GRASS}/Meshes/SM_Fol_Grass_Clump_1", 55, 3200, 4500, (0.8, 1.2)),
    (f"{SH_GRASS}/Meshes/SM_Fol_Grass_Clump_2", 40, 3200, 4500, (0.8, 1.2)),
    (f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Grass_Clump_1", 35, 3200, 4500, (0.8, 1.1)),
    (f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Grass_Clump_4", 20, 3200, 4500, (0.8, 1.1)),
    (f"{SH_GRASS}/Meshes/SM_Fol_Flower_Clump_1", 3, 4500, 6500, (0.8, 1.2)),
    (f"{SH_GRASS}/Meshes/SM_Fol_Flower_Clump_3", 3, 4500, 6500, (0.8, 1.2)),
    (f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Flower_Clump_1", 4, 4500, 6500, (0.9, 1.3)),
    (f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Flower_Clump_2", 4, 4500, 6500, (0.9, 1.3)),
    (f"{SH_GRASS}/Meshes/SM_Fol_Plant_2", 0.8, 4500, 6500, (0.8, 1.2)),
    (f"{SH_GRASS}/Meshes/SM_Fol_Plant_6", 0.8, 4500, 6500, (0.8, 1.2)),
    (f"{SH_GRASS}/Meshes/SM_Fol_Plant_9", 0.8, 4500, 6500, (0.8, 1.2)),
]

# 폴리지 타입 — 이름: (메시, 시작 · 끝 컬 cm, 막힘, 그림자). 끝 컬 상한: 나무 400 m · 바위 200 m · 덤불 120 m(규칙 10), 그림자는 나무 · 큰 바위만(규칙 11)
FOLIAGE = {
    # 나무 끝 컬 480 m(레벨 디자인 2차 — 300 m 면 먼 언덕이 민둥산으로 보였다. World Partition 스트리밍 512 m 안)
    "FT_Dress_Broadleaf1": (f"{SH_FOREST}/Meshes/Trees/SM_Tree_Broadleaf_1", 36000, 48000, True, True),
    "FT_Dress_Broadleaf2": (f"{SH_FOREST}/Meshes/Trees/SM_Tree_Broadleaf_2", 36000, 48000, True, True),
    "FT_Dress_Birch1": (f"{SH_FOREST}/Meshes/Trees/SM_Tree_Birch_1", 36000, 48000, True, True),
    "FT_Dress_Birch2": (f"{SH_FOREST}/Meshes/Trees/SM_Tree_Birch_2", 36000, 48000, True, True),
    "FT_Dress_Bush1": (f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Bush_1", 8400, 12000, False, False),
    "FT_Dress_Bush2": (f"{SH_FOREST}/Meshes/Foliage/SM_Fol_Bush_2", 8400, 12000, False, False),
    "FT_Dress_Bush3": (f"{SH_GRASS}/Meshes/SM_Fol_Bush_2", 8400, 12000, False, False),
    "FT_Dress_Bush4": (f"{SH_GRASS}/Meshes/SM_Fol_Bush_3", 8400, 12000, False, False),
    "FT_Dress_RockLarge": (f"{SH_ROCKS}/Meshes/SM_Stone_Large_1", 14000, 20000, True, True),
    "FT_Dress_RockCluster1": (f"{SH_ROCKS}/Meshes/SM_Stone_Cluster_1", 14000, 20000, True, True),
    "FT_Dress_RockCluster2": (f"{SH_ROCKS}/Meshes/SM_Stone_Cluster_2", 14000, 20000, True, True),
    "FT_Dress_RockSmall1": (f"{SH_ROCKS}/Meshes/SM_Stone_Small_1", 8400, 12000, True, False),
    "FT_Dress_RockSmall3": (f"{SH_ROCKS}/Meshes/SM_Stone_Small_3", 8400, 12000, True, False),
    "FT_Dress_RockSmall5": (f"{SH_ROCKS}/Meshes/SM_Stone_Small_5", 8400, 12000, True, False),
    # 레벨 디자인 2차: 돌담 돌 · 능선 바위(docs/dev/level-design.md 「9」)
    "FT_Dress_RockSmall2": (f"{SH_ROCKS}/Meshes/SM_Stone_Small_2", 8400, 12000, True, False),
    "FT_Dress_RockSmall4": (f"{SH_ROCKS}/Meshes/SM_Stone_Small_4", 8400, 12000, True, False),
    "FT_Dress_Boulder1": (f"{SH_FOREST}/Meshes/Stones/SM_Stone_Boulder_1", 14000, 20000, True, True),
    "FT_Dress_Boulder2": (f"{SH_FOREST}/Meshes/Stones/SM_Stone_3", 14000, 20000, True, True),
}

# 잎 색(레벨 디자인 2차): 팩 기본 잎은 밝은 연두라 땅보다 떠 보인다 — 팩 잎 머티리얼 인스턴스를 부모로 우리 인스턴스를 만들어
# 「Color Base」(그늘 쪽) · 「Color Top」(빛 받는 위쪽)을 콘셉트 아트(짙은 초록 그늘 · 따뜻한 황록 윗면)에 맞춘다. 폴리지 타입 덮어쓰기 재질(슬롯 0 = 잎).
SH_LEAF = f"{SH_FOREST}/Materials/Mat_Trees"
LEAF_MATERIALS = {   # 이름: (부모, Color Base, Color Top, Subsurface)
    "MI_Dress_Leaves_BroadleafDeep": (f"{SH_LEAF}/MI_Tree_Leaves_Broadleaf", (0.010, 0.058, 0.022), (0.105, 0.255, 0.040), (0.012, 0.060, 0.025)),
    "MI_Dress_Leaves_BroadleafOlive": (f"{SH_LEAF}/MI_Tree_Leaves_Broadleaf", (0.024, 0.070, 0.016), (0.165, 0.290, 0.050), (0.020, 0.065, 0.020)),
    "MI_Dress_Leaves_Birch": (f"{SH_LEAF}/MI_Tree_Leaves_Birch_A", (0.040, 0.120, 0.008), (0.170, 0.330, 0.060), (0.018, 0.058, 0.030)),
}
LEAF_OVERRIDE = {"FT_Dress_Broadleaf1": "MI_Dress_Leaves_BroadleafDeep", "FT_Dress_Broadleaf2": "MI_Dress_Leaves_BroadleafOlive",
                 "FT_Dress_Birch1": "MI_Dress_Leaves_Birch", "FT_Dress_Birch2": "MI_Dress_Leaves_Birch"}


def make_leaf_materials():
    out = {}
    for name, (parent, base, top, sss) in LEAF_MATERIALS.items():
        mi = _existing_or_new(name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, _load(parent))
        for param, rgb in (("Color Base", base), ("Color Top", top), ("Subsurface Color", sss)):
            mel.set_material_instance_vector_parameter_value(mi, param, unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        eal.save_loaded_asset(mi)
        out[name] = mi
    return out


# 생성기 식생 규칙 → 폴리지 타입 고르기(레벨 디자인 2차). pick(종 값 0~1, 월드 x, y) — 종 값은 생성기 Variant(같은 종끼리 무리 짓는 노이즈).
# 부품 크기는 규칙 크기(레이아웃 MinScale ~ MaxScale)에 곱한다. 메시 크기 1 = 나무 약 14 m.
def _hash01(x, y):
    return (abs(math.sin(x * 0.000123 + y * 0.000731) * 43758.5453)) % 1.0


def _pick_tree(var, x, y):
    # 자작나무 숲덩이(종 값 낮음) · 섞인 띠 · 활엽수 숲 — 같은 종이 무리 짓고 경계에서 섞인다
    if var < 0.33 or (var < 0.45 and _hash01(x, y) < 0.5):
        return "FT_Dress_Birch1" if _hash01(y, x) < 0.55 else "FT_Dress_Birch2"
    return "FT_Dress_Broadleaf1" if _hash01(y, x) < 0.6 else "FT_Dress_Broadleaf2"


def _pick_dark(var, x, y):
    return "FT_Dress_Broadleaf2" if var < 0.7 else "FT_Dress_Broadleaf1"


def _pick_bush(var, x, y):
    if var < 0.5:
        return "FT_Dress_Bush1" if _hash01(x, y) < 0.5 else "FT_Dress_Bush2"
    return "FT_Dress_Bush3" if _hash01(x, y) < 0.6 else "FT_Dress_Bush4"


def _pick_rock(var, x, y):
    h = _hash01(x, y)
    if var < 0.35:
        return ["FT_Dress_RockSmall1", "FT_Dress_RockSmall3", "FT_Dress_RockSmall5", "FT_Dress_RockSmall2"][int(h * 4) % 4]
    if var < 0.75:
        return "FT_Dress_RockCluster1" if h < 0.5 else "FT_Dress_RockCluster2"
    return "FT_Dress_RockLarge" if h < 0.6 else "FT_Dress_Boulder1"


RULE_SWAP = {
    "Tree": (_pick_tree, 0.66),
    "DarkTree": (_pick_dark, 0.62),
    "Bush": (_pick_bush, 0.75),
    "Rock": (_pick_rock, 0.9),
}


def _path(name):
    return f"{DRESS_DIR}/{name}"


def _material(name):
    """있으면 노드를 모두 지우고 다시 짠다(맵이 참조 중이라 에셋을 지우지 않는다), 없으면 새로"""
    path = _path(name)
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
        return m
    return asset_tools.create_asset(name, DRESS_DIR, unreal.Material, unreal.MaterialFactoryNew())


def _existing_or_new(name, cls, factory):
    path = _path(name)
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    return asset_tools.create_asset(name, DRESS_DIR, cls, factory)


def _load(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"dress_assets: 팩 에셋이 없다 — {path} (워크트리면 Content/<팩> 정션, docs/dev/market-assets.md)")
    return asset


def _expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def _connect(src, src_out, dst, dst_in):
    if not mel.connect_material_expressions(src, src_out, dst, dst_in):
        raise RuntimeError(f"머티리얼 연결 실패: {src.get_name()}.{src_out} -> {dst.get_name()}.{dst_in}")


def _scalar(m, name, value, x, y):
    node = _expr(m, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def _vec(m, name, rgb, x, y):
    node = _expr(m, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mask = _expr(m, unreal.MaterialExpressionComponentMask, x + 200, y)
    for ch, on in (("r", True), ("g", True), ("b", True), ("a", False)):
        mask.set_editor_property(ch, on)
    _connect(node, "", mask, "")
    return mask


def _bin(m, cls, a, b, x, y, a_out="", b_out=""):
    node = _expr(m, cls, x, y)
    _connect(a, a_out, node, "A")
    _connect(b, b_out, node, "B")
    return node


def _lerp(m, a, b, alpha, x, y, a_out="", b_out=""):
    node = _expr(m, unreal.MaterialExpressionLinearInterpolate, x, y)
    _connect(a, a_out, node, "A")
    _connect(b, b_out, node, "B")
    _connect(alpha, "", node, "Alpha")
    return node


def _ramp(m, value, start_name, start, width_name, width, x, y):
    s = _scalar(m, start_name, start, x, y)
    w = _scalar(m, width_name, width, x, y + 80)
    sub = _bin(m, unreal.MaterialExpressionSubtract, value, s, x + 200, y)
    div = _bin(m, unreal.MaterialExpressionDivide, sub, w, x + 350, y)
    sat = _expr(m, unreal.MaterialExpressionSaturate, x + 500, y)
    _connect(div, "", sat, "")
    return sat


def _mask(m, src, channels, x, y):
    node = _expr(m, unreal.MaterialExpressionComponentMask, x, y)
    for ch in ("r", "g", "b", "a"):
        node.set_editor_property(ch, ch in channels)
    _connect(src, "", node, "")
    return node


def _coords(m, scale_m, x, y):
    """랜드스케이프 좌표 UV — 칸이 1 m 라 scale_m 마다 한 번 반복"""
    node = _expr(m, unreal.MaterialExpressionLandscapeLayerCoords, x, y)
    node.set_editor_property("mapping_scale", float(scale_m))
    return node


def _tex(m, path, uv, x, y):
    node = _expr(m, unreal.MaterialExpressionTextureSample, x, y)
    node.set_editor_property("texture", _load(path))
    node.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)   # 공유 샘플러 — 랜드스케이프 샘플러 16 개 한도
    if uv is not None:
        _connect(uv, "", node, "UVs")
    return node


def _layer(m, name, x, y):
    node = _expr(m, unreal.MaterialExpressionLandscapeLayerSample, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("preview_weight", 0.0)
    return node


def _painted(m, tex_node, tint_name, tint, desat, x, y):
    """텍스처 RGB → 채도 낮춤 → 색 보정"""
    d = _expr(m, unreal.MaterialExpressionDesaturation, x, y)
    _connect(tex_node, "RGB", d, "")
    _connect(desat, "", d, "Fraction")
    return _bin(m, unreal.MaterialExpressionMultiply, d, _vec(m, tint_name, tint, x, y + 120), x + 250, y)


def make_grass_type(name="GT_Dress_Meadow"):
    factory = getattr(unreal, "LandscapeGrassTypeFactory", None)
    grass = _existing_or_new(name, unreal.LandscapeGrassType, factory() if factory else None)
    varieties = []
    for mesh_path, density, cull_start, cull_end, (smin, smax) in GRASS_VARIETIES:
        v = unreal.GrassVariety()
        v.set_editor_property("grass_mesh", _load(mesh_path))
        d = unreal.PerPlatformFloat()
        d.set_editor_property("default", density)
        v.set_editor_property("grass_density", d)
        dq = unreal.PerQualityLevelFloat()   # 풀 품질 단계를 켠 프로젝트면 이쪽을 쓴다 — 같은 값
        dq.set_editor_property("default", density)
        v.set_editor_property("grass_density_quality", dq)
        s = unreal.PerPlatformInt()
        s.set_editor_property("default", cull_start)
        e = unreal.PerPlatformInt()
        e.set_editor_property("default", cull_end)
        v.set_editor_property("start_cull_distance", s)
        v.set_editor_property("end_cull_distance", e)
        v.set_editor_property("scaling", unreal.GrassScaling.UNIFORM)
        v.set_editor_property("scale_x", unreal.FloatInterval(smin, smax))
        v.set_editor_property("random_rotation", True)
        v.set_editor_property("align_to_surface", True)
        v.set_editor_property("cast_dynamic_shadow", False)   # 규칙 11 — 풀은 그림자 끔
        v.set_editor_property("cast_contact_shadow", False)
        varieties.append(v)
    grass.set_editor_property("grass_varieties", varieties)
    eal.save_loaded_asset(grass)
    return grass


def make_landscape_material(grass_type, name="M_Terrain_Dress"):
    """M_Terrain_Auto 와 같은 규칙(경사 · 높이 · 레이어 Shore · Path · Forest · 풀 출력)에 StyleHex 지형 텍스처를 입힌다"""
    m = _material(name)

    # 경사 · 높이
    normal = _expr(m, unreal.MaterialExpressionVertexNormalWS, -3200, 0)
    slope = _expr(m, unreal.MaterialExpressionOneMinus, -2800, 0)
    _connect(_mask(m, normal, "b", -3000, 0), "", slope, "")
    world_z = _mask(m, _expr(m, unreal.MaterialExpressionWorldPosition, -3200, 400), "b", -3000, 400)
    rock = _ramp(m, slope, "RockSlopeStart", 0.30, "RockSlopeWidth", 0.12, -2600, -200)
    dirt = _ramp(m, slope, "DirtSlopeStart", 0.13, "DirtSlopeWidth", 0.10, -2600, 0)
    high = _ramp(m, world_z, "HighStart", 9000.0, "HighWidth", 4000.0, -2600, 400)

    # 큰 얼룩(타일 반복 숨김) — 80 m · 23 m 두 배율
    macro_a = _tex(m, TEX["Macro"], _coords(m, 80, -2600, 700), -2300, 700)
    macro_b = _tex(m, TEX["Macro"], _coords(m, 23, -2600, 900), -2300, 900)
    macro = _bin(m, unreal.MaterialExpressionMultiply, macro_a, macro_b, -2050, 800, "R", "G")
    macro_strength = _scalar(m, "MacroStrength", 0.55, -2050, 1000)
    one = _expr(m, unreal.MaterialExpressionConstant, -2050, 1080)
    one.set_editor_property("r", 1.0)
    macro_mul = _lerp(m, one, _bin(m, unreal.MaterialExpressionAdd, macro, _scalar(m, "MacroLift", 0.75, -1900, 1100), -1800, 900), macro_strength, -1600, 900)

    desat = _scalar(m, "Desaturate", 0.12, -2300, 1300)
    # 풀: 5 m · 17 m 두 배율을 얼룩으로 섞는다
    g1 = _tex(m, TEX["Grass"], _coords(m, 5, -2600, 1400), -2300, 1400)
    g2 = _tex(m, TEX["Grass"], _coords(m, 17, -2600, 1600), -2300, 1600)
    g_mix = _lerp(m, g1, g2, macro_a, -2000, 1500, "RGB", "RGB")
    grass = _bin(m, unreal.MaterialExpressionMultiply, g_mix, _vec(m, "GrassTint", TINT["Grass"], -2000, 1700), -1750, 1500)
    grass_d = _expr(m, unreal.MaterialExpressionDesaturation, -1550, 1500)
    _connect(grass, "", grass_d, "")
    _connect(desat, "", grass_d, "Fraction")

    dirt_c = _painted(m, _tex(m, TEX["Dirt"], _coords(m, 4, -2600, 1900), -2300, 1900), "DirtTint", TINT["Dirt"], desat, -2000, 1900)
    path_c = _painted(m, _tex(m, TEX["Dirt2"], _coords(m, 3, -2600, 2200), -2300, 2200), "PathTint", TINT["Path"], desat, -2000, 2200)
    rock_c = _painted(m, _tex(m, TEX["Rock"], _coords(m, 7, -2600, 2500), -2300, 2500), "RockTint", TINT["Rock"], desat, -2000, 2500)
    sand_c = _bin(m, unreal.MaterialExpressionMultiply, path_c, _vec(m, "SandTint", TINT["Sand"], -1750, 2800), -1500, 2700)
    forest_c = _bin(m, unreal.MaterialExpressionMultiply, dirt_c, _vec(m, "ForestTint", TINT["Forest"], -1750, 3000), -1500, 2950)

    high_half = _bin(m, unreal.MaterialExpressionMultiply, high, _scalar(m, "HighDirtAmount", 0.6, -1800, 450), -1550, 420)
    dirt_mask = _bin(m, unreal.MaterialExpressionMax, dirt, high_half, -1400, 300)
    c1 = _lerp(m, grass_d, dirt_c, dirt_mask, -1200, 1500)
    forest_layer = _layer(m, "Forest", -1400, 3200)
    c2 = _lerp(m, c1, forest_c, forest_layer, -1000, 1600)
    c3 = _lerp(m, c2, rock_c, rock, -800, 1700)
    shore_layer = _layer(m, "Shore", -1000, 3400)
    c4 = _lerp(m, c3, sand_c, shore_layer, -600, 1800)
    path_layer = _layer(m, "Path", -800, 3600)
    c5 = _lerp(m, c4, path_c, path_layer, -400, 1900)
    final = _bin(m, unreal.MaterialExpressionMultiply, c5, macro_mul, -200, 1900)
    mel.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(_scalar(m, "Roughness", 0.95, -200, 2100), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(_scalar(m, "Specular", 0.25, -200, 2200), "", unreal.MaterialProperty.MP_SPECULAR)

    # 풀 출력: 풀밭 = (1 - 바위)(1 - 흙)(1 - 모래)(1 - 길)(1 - 계곡 숲)
    weight = None
    for i, msk in enumerate([rock, dirt_mask, shore_layer, path_layer, forest_layer]):
        inv = _expr(m, unreal.MaterialExpressionOneMinus, 0, 2400 + i * 80)
        _connect(msk, "", inv, "")
        weight = inv if weight is None else _bin(m, unreal.MaterialExpressionMultiply, weight, inv, 200, 2400 + i * 80)
    out = _expr(m, unreal.MaterialExpressionLandscapeGrassOutput, 500, 2500)
    gi = unreal.GrassInput()
    gi.set_editor_property("name", "Meadow")
    gi.set_editor_property("grass_type", grass_type)
    out.set_editor_property("grass_types", [gi])
    _connect(weight, "", out, "Meadow")

    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def make_cloud_material(name="M_Dress_CloudCard"):
    """손그림 구름 카드 — 빛을 받는 반투명(해 · 달 빛으로 밤에 어두워진다 — GrimSkyController 와 따로 놀지 않게)"""
    m = _material(name)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
    tex = _expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
    tex.set_editor_property("parameter_name", "CloudTex")
    tex.set_editor_property("texture", _load(CLOUD_TEXTURES[0]))
    color = _bin(m, unreal.MaterialExpressionMultiply, tex, _vec(m, "Tint", (1.0, 0.98, 0.95), -800, 300), -400, 0, "RGB")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    opacity = _bin(m, unreal.MaterialExpressionMultiply, tex, _scalar(m, "Opacity", 0.9, -800, 500), -400, 300, "A")
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    instances = []
    for i, tex_path in enumerate(CLOUD_TEXTURES):
        mi = _existing_or_new(f"MI_Dress_Cloud_{i + 1:02d}", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, m)
        mel.set_material_instance_texture_parameter_value(mi, "CloudTex", _load(tex_path))
        eal.save_loaded_asset(mi)
        instances.append(mi)
    return instances


def make_foliage_types():
    """폴리지 타입 — 팩 메시 그대로(머티리얼 덮어쓰기 없음). 컬 · 그림자는 FOLIAGE 표대로 매번 맞추고, 충돌은 처음 만들 때 정한다"""
    out = {}
    leaves = make_leaf_materials()
    for name, (mesh_path, cull_start, cull_end, block, shadow) in FOLIAGE.items():
        ft = unreal.GrimTerrainEditorLibrary.create_foliage_type_asset(_path(name), _load(mesh_path), None, cull_start, cull_end, block, shadow)
        if ft is None:
            raise RuntimeError(f"폴리지 타입을 만들지 못했다: {name}")
        ft.set_editor_property("mesh", _load(mesh_path))
        ft.set_editor_property("cull_distance", unreal.Int32Interval(cull_start, cull_end))
        ft.set_editor_property("cast_shadow", shadow)
        if name in LEAF_OVERRIDE:
            ft.set_editor_property("override_materials", [leaves[LEAF_OVERRIDE[name]]])
        eal.save_loaded_asset(ft)
        out[name] = ft
    return out


def make_all(rebuild_landscape=True):
    """rebuild_landscape=False 면 지형 재질 · 풀 타입은 있는 것을 그대로 쓴다(맵의 랜드스케이프가 이미 쓰는 중 — dress_openbeta.py 참고)"""
    a = {}
    if rebuild_landscape or not eal.does_asset_exist(_path("M_Terrain_Dress")):
        a["grass"] = make_grass_type()
        a["landscape"] = make_landscape_material(a["grass"])
    else:
        a["grass"] = eal.load_asset(_path("GT_Dress_Meadow"))
        a["landscape"] = eal.load_asset(_path("M_Terrain_Dress"))
    a["clouds"] = make_cloud_material()
    a["foliage"] = make_foliage_types()
    return a
