# 지형 1차(M3 갈래 W) 에셋 — Content/Grimoire/Terrain 의 머티리얼 · 레이어 정보 · 폴리지 타입 · 풀 타입.
# 없을 때만 만든다(손으로 고친 색 · 거리는 그대로 둔다). refresh=True 면 지우고 새로 만든다.
# 엔진 기본 도형 · 색만 쓰는 임시 모습이다 — 정식 머티리얼 · 식생은 지형 3차(M7).
import unreal

ASSET_DIR = "/Game/Grimoire/Terrain"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# 색(선형) — MI 의 파라미터로 들어가니 에디터에서 바꿔도 된다
COLORS = {
    "Grass": (0.10, 0.22, 0.05),
    "Grass2": (0.20, 0.27, 0.06),
    "Dirt": (0.16, 0.10, 0.05),
    "Rock": (0.20, 0.19, 0.18),
    "Sand": (0.50, 0.42, 0.26),
    "Path": (0.30, 0.20, 0.10),
    "Forest": (0.04, 0.08, 0.03),
}

FOLIAGE_COLORS = {
    "MI_Terrain_TreeTrunk": (0.12, 0.07, 0.03),
    "MI_Terrain_TreeCrown": (0.06, 0.20, 0.04),
    "MI_Terrain_DarkCrown": (0.02, 0.07, 0.03),
    "MI_Terrain_Bush": (0.08, 0.22, 0.05),
    "MI_Terrain_Rock": (0.22, 0.21, 0.20),
    "MI_Terrain_Grass": (0.12, 0.28, 0.05),
}


def _path(name):
    return f"{ASSET_DIR}/{name}"


def _fresh(name, refresh):
    """있으면 그대로 쓰고(None 아님), refresh 면 지운다"""
    path = _path(name)
    if eal.does_asset_exist(path):
        if not refresh:
            return eal.load_asset(path)
        eal.delete_asset(path)
    return None


def _expr(material, cls, x, y):
    return mel.create_material_expression(material, cls, x, y)


def _connect(src, src_out, dst, dst_in):
    if not mel.connect_material_expressions(src, src_out, dst, dst_in):
        raise RuntimeError(f"머티리얼 연결 실패: {src.get_name()}.{src_out} -> {dst.get_name()}.{dst_in}")


def _scalar(material, name, value, x, y):
    node = _expr(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def _color(material, name, rgb, x, y):
    """벡터 파라미터 → RGB 만(float3)"""
    node = _expr(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mask = _expr(material, unreal.MaterialExpressionComponentMask, x + 200, y)
    for channel, on in (("r", True), ("g", True), ("b", True), ("a", False)):
        mask.set_editor_property(channel, on)
    _connect(node, "", mask, "")
    return mask


def _binop(material, cls, a, b, x, y, a_out="", b_out=""):
    node = _expr(material, cls, x, y)
    _connect(a, a_out, node, "A")
    _connect(b, b_out, node, "B")
    return node


def _lerp(material, a, b, alpha, x, y):
    node = _expr(material, unreal.MaterialExpressionLinearInterpolate, x, y)
    _connect(a, "", node, "A")
    _connect(b, "", node, "B")
    _connect(alpha, "", node, "Alpha")
    return node


def _ramp(material, value, start_name, start, width_name, width, x, y):
    """saturate((value - start) / width) — 경사 · 높이 마스크"""
    s = _scalar(material, start_name, start, x, y)
    w = _scalar(material, width_name, width, x, y + 80)
    sub = _binop(material, unreal.MaterialExpressionSubtract, value, s, x + 200, y)
    div = _binop(material, unreal.MaterialExpressionDivide, sub, w, x + 350, y)
    sat = _expr(material, unreal.MaterialExpressionSaturate, x + 500, y)
    _connect(div, "", sat, "")
    return sat


def _layer(material, name, x, y):
    node = _expr(material, unreal.MaterialExpressionLandscapeLayerSample, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("preview_weight", 0.0)
    return node


def make_grass_type(refresh, grass_mi):
    existing = _fresh("GT_Terrain_Meadow", refresh)
    if existing:
        return existing
    factory = getattr(unreal, "LandscapeGrassTypeFactory", None)
    grass = asset_tools.create_asset("GT_Terrain_Meadow", ASSET_DIR, unreal.LandscapeGrassType, factory() if factory else None)
    if grass is None:
        unreal.log_warning("terrain_assets: 풀 타입을 만들지 못했다 — 풀 없이 간다")
        return None
    variety = unreal.GrassVariety()
    variety.set_editor_property("grass_mesh", unreal.load_asset("/Engine/BasicShapes/Cone"))
    variety.set_editor_property("override_materials", [grass_mi])
    density = unreal.PerPlatformFloat()
    density.set_editor_property("default", 12.0)
    variety.set_editor_property("grass_density", density)
    start = unreal.PerPlatformInt()
    start.set_editor_property("default", 3000)
    end = unreal.PerPlatformInt()
    end.set_editor_property("default", 5000)
    variety.set_editor_property("start_cull_distance", start)
    variety.set_editor_property("end_cull_distance", end)
    variety.set_editor_property("scaling", unreal.GrassScaling.FREE)
    variety.set_editor_property("scale_x", unreal.FloatInterval(0.05, 0.09))
    variety.set_editor_property("scale_y", unreal.FloatInterval(0.05, 0.09))
    variety.set_editor_property("scale_z", unreal.FloatInterval(0.25, 0.5))
    variety.set_editor_property("random_rotation", True)
    variety.set_editor_property("align_to_surface", True)
    grass.set_editor_property("grass_varieties", [variety])
    eal.save_loaded_asset(grass)
    return grass


def make_landscape_material(refresh, grass_type):
    """경사 · 높이로 풀 · 흙 · 바위, 레이어(Shore · Path · Forest)로 모래 · 길 · 계곡 숲 바닥. 풀 출력 = 풀밭 무게"""
    existing = _fresh("M_Terrain_Auto", refresh)
    if existing:
        return existing
    m = asset_tools.create_asset("M_Terrain_Auto", ASSET_DIR, unreal.Material, unreal.MaterialFactoryNew())

    # 경사(1 - 법선 Z) · 높이(월드 Z, cm)
    normal = _expr(m, unreal.MaterialExpressionVertexNormalWS, -2400, 0)
    normal_z = _expr(m, unreal.MaterialExpressionComponentMask, -2200, 0)
    for channel, on in (("r", False), ("g", False), ("b", True), ("a", False)):
        normal_z.set_editor_property(channel, on)
    _connect(normal, "", normal_z, "")
    slope = _expr(m, unreal.MaterialExpressionOneMinus, -2000, 0)
    _connect(normal_z, "", slope, "")
    world = _expr(m, unreal.MaterialExpressionWorldPosition, -2400, 400)
    world_z = _expr(m, unreal.MaterialExpressionComponentMask, -2200, 400)
    for channel, on in (("r", False), ("g", False), ("b", True), ("a", False)):
        world_z.set_editor_property(channel, on)
    _connect(world, "", world_z, "")

    rock = _ramp(m, slope, "RockSlopeStart", 0.30, "RockSlopeWidth", 0.12, -1800, -200)     # 약 45도 → 바위
    dirt = _ramp(m, slope, "DirtSlopeStart", 0.12, "DirtSlopeWidth", 0.10, -1800, 0)        # 약 28도 → 흙
    high = _ramp(m, world_z, "HighStart", 9000.0, "HighWidth", 4000.0, -1800, 400)          # 90 m 위 → 흙 · 바위 섞임

    # 풀 색 얼룩(노이즈)
    noise = _expr(m, unreal.MaterialExpressionNoise, -1800, 700)
    noise.set_editor_property("scale", 0.002)
    noise.set_editor_property("output_min", 0.0)
    noise.set_editor_property("output_max", 1.0)   # 위치 입력을 비우면 월드 위치를 쓴다

    grass = _lerp(m, _color(m, "GrassColor", COLORS["Grass"], -1400, 600), _color(m, "GrassColor2", COLORS["Grass2"], -1400, 760), noise, -1000, 650)
    high_half = _binop(m, unreal.MaterialExpressionMultiply, high, _scalar(m, "HighDirtAmount", 0.7, -1400, 450), -1150, 420)
    dirt_mask = _binop(m, unreal.MaterialExpressionMax, dirt, high_half, -1000, 300)
    c1 = _lerp(m, grass, _color(m, "DirtColor", COLORS["Dirt"], -1000, 900), dirt_mask, -700, 500)
    forest_layer = _layer(m, "Forest", -1000, 1100)
    c2 = _lerp(m, c1, _color(m, "ForestColor", COLORS["Forest"], -700, 1100), forest_layer, -450, 600)
    c3 = _lerp(m, c2, _color(m, "RockColor", COLORS["Rock"], -450, 1250), rock, -200, 650)
    shore_layer = _layer(m, "Shore", -450, 1450)
    c4 = _lerp(m, c3, _color(m, "SandColor", COLORS["Sand"], -200, 1450), shore_layer, 50, 700)
    path_layer = _layer(m, "Path", -200, 1650)
    c5 = _lerp(m, c4, _color(m, "PathColor", COLORS["Path"], 50, 1650), path_layer, 300, 750)
    mel.connect_material_property(c5, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(_scalar(m, "Roughness", 0.92, 300, 950), "", unreal.MaterialProperty.MP_ROUGHNESS)

    # 풀 출력: 풀밭 = (1 - 바위)(1 - 흙)(1 - 모래)(1 - 길)(1 - 계곡 숲)
    if grass_type is not None:
        weight = None
        for i, mask in enumerate([rock, dirt_mask, shore_layer, path_layer, forest_layer]):
            inv = _expr(m, unreal.MaterialExpressionOneMinus, 300, 1100 + i * 80)
            _connect(mask, "", inv, "")
            weight = inv if weight is None else _binop(m, unreal.MaterialExpressionMultiply, weight, inv, 500, 1100 + i * 80)
        out = _expr(m, unreal.MaterialExpressionLandscapeGrassOutput, 800, 1200)
        grass_input = unreal.GrassInput()
        grass_input.set_editor_property("name", "Meadow")
        grass_input.set_editor_property("grass_type", grass_type)
        out.set_editor_property("grass_types", [grass_input])
        _connect(weight, "", out, "Meadow")

    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def make_color_material(refresh):
    """폴리지 · 풀용 단색(파라미터 Color · Roughness), 인스턴스드 메시에 쓴다"""
    existing = _fresh("M_Terrain_Color", refresh)
    if existing:
        return existing
    m = asset_tools.create_asset("M_Terrain_Color", ASSET_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_instanced_static_meshes", True)
    mel.connect_material_property(_color(m, "Color", (0.1, 0.25, 0.05), -600, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(_scalar(m, "Roughness", 0.85, -600, 200), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def make_water_material(refresh):
    """반투명 물(연못 · 개울 · 바다 · 강 공용)"""
    existing = _fresh("M_Terrain_Water", refresh)
    if existing:
        return existing
    m = asset_tools.create_asset("M_Terrain_Water", ASSET_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    mel.connect_material_property(_color(m, "WaterColor", (0.03, 0.16, 0.22), -600, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(_scalar(m, "Opacity", 0.72, -600, 200), "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(_scalar(m, "Roughness", 0.06, -600, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(_scalar(m, "Specular", 0.6, -600, 400), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def make_color_instance(name, parent, rgb, refresh):
    existing = _fresh(name, refresh)
    if existing:
        return existing
    mi = asset_tools.create_asset(name, ASSET_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    eal.save_loaded_asset(mi)
    return mi


def make_layer_info(name, layer, debug_rgb, refresh):
    _fresh(name, refresh)
    info = unreal.GrimTerrainEditorLibrary.create_layer_info_asset(_path(name), layer, unreal.LinearColor(debug_rgb[0], debug_rgb[1], debug_rgb[2], 1.0))
    if info is None:
        raise RuntimeError(f"레이어 정보를 만들지 못했다: {name}")
    eal.save_loaded_asset(info)
    return info


def make_foliage_type(name, mesh_path, material, cull_start, cull_end, block, shadow, refresh):
    _fresh(name, refresh)
    ft = unreal.GrimTerrainEditorLibrary.create_foliage_type_asset(_path(name), unreal.load_asset(mesh_path), material, cull_start, cull_end, block, shadow)
    if ft is None:
        raise RuntimeError(f"폴리지 타입을 만들지 못했다: {name}")
    eal.save_loaded_asset(ft)
    return ft


def make_all(refresh=False):
    """모든 지형 에셋을 만들고(없을 때만) 이름 → 에셋 사전으로 돌려준다"""
    a = {}
    a["color"] = make_color_material(refresh)
    for name, rgb in FOLIAGE_COLORS.items():
        a[name] = make_color_instance(name, a["color"], rgb, refresh)
    a["water"] = make_water_material(refresh)
    a["grass"] = make_grass_type(refresh, a["MI_Terrain_Grass"])
    a["landscape"] = make_landscape_material(refresh, a["grass"])
    a["layers"] = [
        make_layer_info("LI_Terrain_Shore", "Shore", (0.9, 0.8, 0.4), refresh),
        make_layer_info("LI_Terrain_Path", "Path", (0.7, 0.4, 0.2), refresh),
        make_layer_info("LI_Terrain_Forest", "Forest", (0.1, 0.4, 0.1), refresh),
    ]
    cyl, cone, sphere = "/Engine/BasicShapes/Cylinder", "/Engine/BasicShapes/Cone", "/Engine/BasicShapes/Sphere"
    a["FT_Terrain_TreeTrunk"] = make_foliage_type("FT_Terrain_TreeTrunk", cyl, a["MI_Terrain_TreeTrunk"], 30000, 40000, True, True, refresh)
    a["FT_Terrain_TreeCrown"] = make_foliage_type("FT_Terrain_TreeCrown", cone, a["MI_Terrain_TreeCrown"], 30000, 40000, False, True, refresh)
    a["FT_Terrain_DarkTrunk"] = make_foliage_type("FT_Terrain_DarkTrunk", cyl, a["MI_Terrain_TreeTrunk"], 30000, 40000, True, True, refresh)
    a["FT_Terrain_DarkCrown"] = make_foliage_type("FT_Terrain_DarkCrown", cone, a["MI_Terrain_DarkCrown"], 30000, 40000, False, True, refresh)
    a["FT_Terrain_Bush"] = make_foliage_type("FT_Terrain_Bush", sphere, a["MI_Terrain_Bush"], 9000, 12000, False, True, refresh)
    a["FT_Terrain_Rock"] = make_foliage_type("FT_Terrain_Rock", sphere, a["MI_Terrain_Rock"], 15000, 20000, True, True, refresh)
    return a
