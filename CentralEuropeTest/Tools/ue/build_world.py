"""Builds the real test level in one run. In the Unreal Editor:

    Tools > Execute Python Script...  ->  Tools/ue/build_world.py

Steps (each one reports success or failure in a summary dialog and in the Output Log):
  1. Creates / opens the level  /Game/Maps/L_CentralEuropeTest
  2. Builds the landscape material M_CE_Landscape (slope / height / snow blending, road strip),
     textured from Epic's Starter Content when that pack is in the project, flat colours otherwise,
     and the lake material M_CE_Water
  3. Imports the generated heightmap as a real Landscape (via the project's C++ helper)
  4. Places sun, sky light, sky atmosphere, volumetric clouds and height fog
  5. Places the PlayerStart on the valley road, saves, and makes the level the startup map

Safe to run again: existing actors and assets are reused and updated.

Written without access to an editor, so individual steps may need fixing - every step is isolated
so one failure does not stop the others. If something fails, send the Output Log lines that start
with [build_world].
"""
import os

import unreal

MAP_PATH = "/Game/Maps/L_CentralEuropeTest"
MAT_DIR = "/Game/CE/Materials"
TEX_DIR = "/Game/CE/Terrain"
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
HEIGHTMAP = os.path.join(PROJECT_DIR, "SourceArt", "Terrain", "heightmap_2017.png")
ROAD_MASK = os.path.join(PROJECT_DIR, "SourceArt", "Terrain", "road_mask.png")
PLAYER_START = (-1322.0, 8367.0, 3176.0 + 150.0)   # on the valley loop road; range targets are in sight from here
PLAYER_YAW = 30.0

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
results = []


def log(msg):
    unreal.log("[build_world] " + msg)


def step(title, fn):
    """Runs one step; records OK / FAILED so the summary tells the whole story."""
    try:
        note = fn()
        results.append("OK      %s%s" % (title, (" - " + note) if note else ""))
        log("OK: %s %s" % (title, note or ""))
    except Exception as exc:  # keep going: later steps are independent where possible
        results.append("FAILED  %s - %s" % (title, exc))
        unreal.log_error("[build_world] FAILED: %s: %s" % (title, exc))


def actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def find_actor(cls):
    for actor in actor_subsystem().get_all_level_actors():
        if isinstance(actor, cls):
            return actor
    return None


def find_or_spawn(cls, label, location=None, folder="Lighting"):
    actor = find_actor(cls)
    if actor is None:
        actor = actor_subsystem().spawn_actor_from_class(cls, location or unreal.Vector(0, 0, 0))
        actor.set_actor_label(label)
        actor.set_folder_path(folder)
    return actor


def set_props(obj, props):
    for name, value in props.items():
        try:
            obj.set_editor_property(name, value)
        except Exception as exc:
            unreal.log_warning("[build_world] %s.%s not set: %s" % (obj.get_name(), name, exc))


# ----------------------------------------------------------------------------- level

def open_level():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if eal.does_asset_exist(MAP_PATH):
        if not les.load_level(MAP_PATH):
            raise RuntimeError("could not open " + MAP_PATH)
        return "opened existing level"
    if not les.new_level(MAP_PATH):
        raise RuntimeError("could not create " + MAP_PATH)
    return "created new level"


# ----------------------------------------------------------------------------- materials

WEIGHTS = """
float3 Nrm = normalize(VertexNormal);
float SlopeDeg = degrees(acos(saturate(Nrm.z)));
float Height = WorldPos.z;
float wCliff = smoothstep(42.0, 48.0, SlopeDeg);
float wScree = saturate(max(smoothstep(27.0, 33.0, SlopeDeg), smoothstep(65000.0, 70000.0, Height))) * (1.0 - wCliff);
float Rest = 1.0 - wCliff - wScree;
float wForest = Rest * (1.0 - smoothstep(12.0, 18.0, SlopeDeg)) * (1.0 - smoothstep(45000.0, 50000.0, Height));
float wMeadow = Rest - wForest;
float Snow = saturate((Height - 85000.0) / 15000.0) * saturate((Nrm.z - 0.5) / 0.3);
"""

COLOR_TEXTURED = WEIGHTS + """
float2 uv = WorldPos.xy / 500.0;
float2 uvFar = WorldPos.xy / 4300.0;
float macro = 0.65 + 0.7 * Texture2DSample(TexScree, TexScreeSampler, WorldPos.xy / 23000.0).r;
float3 cForest = Texture2DSample(TexForest, TexForestSampler, uv).rgb * (0.5 + Texture2DSample(TexForest, TexForestSampler, uvFar).g) * float3(0.55, 0.62, 0.42);
float3 cMeadow = Texture2DSample(TexMeadow, TexMeadowSampler, uv).rgb * (0.5 + Texture2DSample(TexMeadow, TexMeadowSampler, uvFar).g) * float3(0.85, 1.0, 0.6);
float3 cScree = Texture2DSample(TexScree, TexScreeSampler, uv * 0.7).rgb * (0.6 + 0.8 * Texture2DSample(TexScree, TexScreeSampler, uvFar).r);
float3 bw = pow(abs(Nrm), 4.0);
bw /= (bw.x + bw.y + bw.z);
float3 cCliff = (Texture2DSample(TexCliff, TexCliffSampler, WorldPos.yz / 900.0).rgb * bw.x
               + Texture2DSample(TexCliff, TexCliffSampler, WorldPos.xz / 900.0).rgb * bw.y
               + Texture2DSample(TexCliff, TexCliffSampler, WorldPos.xy / 900.0).rgb * bw.z) * float3(0.8, 0.8, 0.84);
float3 Col = (cForest * wForest + cMeadow * wMeadow + cScree * wScree + cCliff * wCliff) * macro;
float road = Texture2DSample(TexRoad, TexRoadSampler, (WorldPos.xy + 100800.0) / 201600.0).r;
Col = lerp(Col, cScree * float3(1.15, 1.08, 0.95), smoothstep(0.35, 0.75, road));
Col = lerp(Col, float3(0.9, 0.92, 0.96), saturate(Snow * (0.6 + 0.6 * macro)));
return Col;
"""

COLOR_FLAT = WEIGHTS + """
float3 Col = float3(0.045, 0.075, 0.025) * wForest + float3(0.16, 0.26, 0.05) * wMeadow
         + float3(0.26, 0.23, 0.19) * wScree + float3(0.11, 0.11, 0.115) * wCliff;
float road = Texture2DSample(TexRoad, TexRoadSampler, (WorldPos.xy + 100800.0) / 201600.0).r;
Col = lerp(Col, float3(0.3, 0.27, 0.22), smoothstep(0.35, 0.75, road));
Col = lerp(Col, float3(0.9, 0.92, 0.96), saturate(Snow));
return Col;
"""

NORMAL_TEXTURED = WEIGHTS + """
float2 uv = WorldPos.xy / 500.0;
float3 n = UnpackNormalMap(Texture2DSample(NForest, NForestSampler, uv)).xyz * wForest
         + UnpackNormalMap(Texture2DSample(NMeadow, NMeadowSampler, uv)).xyz * wMeadow
         + UnpackNormalMap(Texture2DSample(NScree, NScreeSampler, uv * 0.7)).xyz * wScree
         + UnpackNormalMap(Texture2DSample(NCliff, NCliffSampler, WorldPos.xy / 900.0)).xyz * wCliff;
n = lerp(n, float3(0.0, 0.0, 1.0), saturate(Snow) * 0.7);
return normalize(n);
"""

ROUGHNESS = WEIGHTS + """
return lerp(0.95 * wForest + 0.9 * wMeadow + 0.86 * wScree + 0.8 * wCliff, 0.5, saturate(Snow));
"""

WATER_NORMAL = """
float2 p = WorldPos.xy;
float t = Seconds;
float3 n = float3(
    sin(p.x / 210.0 + t * 0.9) * 0.035 + sin((p.x + p.y) / 67.0 - t * 1.7) * 0.02 + sin((p.x * 0.8 - p.y) / 23.0 + t * 2.6) * 0.012,
    cos(p.y / 180.0 - t * 0.8) * 0.035 + cos((p.y - p.x) / 59.0 + t * 1.5) * 0.02 + cos((p.y * 0.7 + p.x) / 19.0 - t * 2.9) * 0.012,
    1.0);
return normalize(n);
"""

# Starter Content textures per layer: first one that exists wins.
STARTER = "/Game/StarterContent/Textures/"
LAYER_TEXTURES = {
    "Forest": ["T_Ground_Moss", "T_Ground_Grass"],
    "Meadow": ["T_Ground_Grass", "T_Ground_Moss"],
    "Scree": ["T_Ground_Gravel"],
    "Cliff": ["T_Rock_Slate", "T_Rock_Basalt", "T_Rock_Sandstone"],
}


def get_material(name):
    path = "%s/%s" % (MAT_DIR, name)
    if eal.does_asset_exist(path):
        mat = eal.load_asset(path)
        mel.delete_all_material_expressions(mat)
        return mat
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def custom_node(mat, code, output_type, input_names, x, y):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output_type)
    inputs = []
    for name in input_names:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        inputs.append(entry)
    node.set_editor_property("inputs", inputs)
    return node


def texture_param(mat, name, texture, x, y, normal=False):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", texture)
    try:
        node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal
                                 else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    except Exception as exc:
        unreal.log_warning("[build_world] sampler type on %s: %s" % (name, exc))
    return node


def constant(mat, value, x, y):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def import_road_mask():
    path = TEX_DIR + "/T_RoadMask"
    if not eal.does_asset_exist(path):
        task = unreal.AssetImportTask()
        set_props(task, {"filename": ROAD_MASK, "destination_path": TEX_DIR, "destination_name": "T_RoadMask",
                         "automated": True, "replace_existing": True, "save": True})
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = eal.load_asset(path)
    if tex is None:
        raise RuntimeError("road mask did not import from " + ROAD_MASK)
    set_props(tex, {"srgb": False})
    return tex


def starter_textures():
    """{layer: (diffuse, normal)} when every layer has a Starter Content texture, else None."""
    found = {}
    for layer, names in LAYER_TEXTURES.items():
        for base in names:
            d, n = STARTER + base + "_D", STARTER + base + "_N"
            if eal.does_asset_exist(d) and eal.does_asset_exist(n):
                found[layer] = (eal.load_asset(d), eal.load_asset(n))
                break
        if layer not in found:
            return None
    return found


state = {}


def build_landscape_material():
    road = import_road_mask()
    textures = starter_textures()
    mat = get_material("M_CE_Landscape")

    world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1500, 0)
    vertex_n = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1500, 200)
    road_node = texture_param(mat, "RoadMask", road, -1500, 400)
    f3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
    f1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1

    def wire_common(node):
        mel.connect_material_expressions(world_pos, "", node, "WorldPos")
        mel.connect_material_expressions(vertex_n, "", node, "VertexNormal")

    if textures:
        layers = ["Forest", "Meadow", "Scree", "Cliff"]
        color = custom_node(mat, COLOR_TEXTURED, f3, ["WorldPos", "VertexNormal"] + ["Tex" + l for l in layers] + ["TexRoad"], -700, 0)
        normal = custom_node(mat, NORMAL_TEXTURED, f3, ["WorldPos", "VertexNormal"] + ["N" + l for l in layers], -700, 500)
        wire_common(color)
        wire_common(normal)
        for i, layer in enumerate(layers):
            diffuse, nrm = textures[layer]
            mel.connect_material_expressions(texture_param(mat, layer + "Diffuse", diffuse, -1500, 600 + i * 180), "", color, "Tex" + layer)
            mel.connect_material_expressions(texture_param(mat, layer + "Normal", nrm, -1200, 600 + i * 180, True), "", normal, "N" + layer)
        mel.connect_material_expressions(road_node, "", color, "TexRoad")
        mel.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
    else:
        color = custom_node(mat, COLOR_FLAT, f3, ["WorldPos", "VertexNormal", "TexRoad"], -700, 0)
        wire_common(color)
        mel.connect_material_expressions(road_node, "", color, "TexRoad")

    rough = custom_node(mat, ROUGHNESS, f1, ["WorldPos", "VertexNormal"], -700, 300)
    wire_common(rough)
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(constant(mat, 0.25, -300, 200), "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, False)
    state["landscape_material"] = mat
    return "Starter Content textures" if textures else "flat colours (Starter Content pack not found)"


def build_water_material():
    mat = get_material("M_CE_Water")
    set_props(mat, {"blend_mode": unreal.BlendMode.BLEND_TRANSLUCENT})
    try:
        mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    except Exception as exc:
        unreal.log_warning("[build_world] water lighting mode: %s" % exc)

    base = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -500, 0)
    base.set_editor_property("constant", unreal.LinearColor(0.004, 0.03, 0.045, 1.0))
    world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1100, 300)
    time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1100, 450)
    ripples = custom_node(mat, WATER_NORMAL, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["WorldPos", "Seconds"], -600, 300)
    mel.connect_material_expressions(world_pos, "", ripples, "WorldPos")
    mel.connect_material_expressions(time, "", ripples, "Seconds")

    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ripples, "", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(constant(mat, 0.06, -300, 150), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(constant(mat, 0.6, -300, 220), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(constant(mat, 0.92, -300, 290), "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, False)


# ----------------------------------------------------------------------------- landscape

def import_landscape():
    if not os.path.exists(HEIGHTMAP):
        raise RuntimeError("missing %s - run Tools/generate_terrain.py" % HEIGHTMAP)
    if not hasattr(unreal, "CEEditorLibrary"):
        raise RuntimeError("the project's C++ module is not loaded (rebuild the project, then run this again)")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actor = unreal.CEEditorLibrary.import_terrain_landscape(world, HEIGHTMAP, state.get("landscape_material"))
    if actor is None:
        raise RuntimeError("the importer returned nothing - see LogCETest lines in the Output Log")
    actor.set_folder_path("Terrain")
    return "landscape actor: " + actor.get_actor_label()


# ----------------------------------------------------------------------------- lighting

def movable(actor):
    try:
        actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    except Exception as exc:
        unreal.log_warning("[build_world] mobility on %s: %s" % (actor.get_name(), exc))


def build_lighting():
    high = unreal.Vector(0, 0, 150000)

    sun = find_or_spawn(unreal.DirectionalLight, "Sun", high)
    movable(sun)
    # Late-morning sun from the south-east: lights the valley and rakes across the north / east cliffs.
    sun.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-40.0, yaw=205.0), False)
    set_props(sun.get_component_by_class(unreal.DirectionalLightComponent), {
        "intensity": 10.0,
        "atmosphere_sun_light": True,
        "atmosphere_sun_light_index": 0,
        "cast_shadows": True,
        "cast_dynamic_shadows": True,
        "light_source_angle": 0.5357,
    })

    sky = find_or_spawn(unreal.SkyLight, "SkyLight", high)
    movable(sky)
    set_props(sky.get_component_by_class(unreal.SkyLightComponent), {
        "real_time_capture": True,
        "intensity": 1.0,
        "cast_shadows": True,
    })

    find_or_spawn(unreal.SkyAtmosphere, "SkyAtmosphere")
    find_or_spawn(unreal.VolumetricCloud, "VolumetricCloud")

    fog = find_or_spawn(unreal.ExponentialHeightFog, "HeightFog")
    set_props(fog.get_component_by_class(unreal.ExponentialHeightFogComponent), {
        "fog_density": 0.012,
        "fog_height_falloff": 0.08,
        "volumetric_fog": True,
        "volumetric_fog_scattering_distribution": 0.2,
        "volumetric_fog_distance": 8000.0,
    })


def place_player_start():
    start = find_actor(unreal.PlayerStart)
    location = unreal.Vector(*PLAYER_START)
    if start is None:
        start = actor_subsystem().spawn_actor_from_class(unreal.PlayerStart, location)
    start.set_actor_location(location, False, True)
    start.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=PLAYER_YAW), False)


# ----------------------------------------------------------------------------- save / config

def save_all():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    eal.save_directory("/Game/CE", False, True)


def set_startup_map():
    ini = os.path.join(PROJECT_DIR, "Config", "DefaultEngine.ini")
    with open(ini, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()
    changed = False
    for i, line in enumerate(lines):
        for key in ("EditorStartupMap=", "GameDefaultMap="):
            if line.startswith(key) and line != key + MAP_PATH:
                lines[i] = key + MAP_PATH
                changed = True
    if changed:
        with open(ini, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
    return "startup map updated" if changed else "already set"


def content_report():
    packs = []
    packs.append("Starter Content: " + ("found" if eal.does_directory_exist("/Game/StarterContent") else "MISSING"))
    packs.append("Mannequin: " + ("found" if eal.does_directory_exist("/Game/Characters/Mannequins") else "MISSING"))
    packs.append("Sounds: " + ("found" if eal.does_directory_exist("/Game/CE/Audio") else "MISSING (restart the editor once)"))
    return ", ".join(packs)


def main():
    step("Open level", open_level)
    step("Landscape material", build_landscape_material)
    step("Water material", build_water_material)
    step("Import landscape", import_landscape)
    step("Sun, sky, clouds, fog", build_lighting)
    step("Player start", place_player_start)
    step("Save", save_all)
    step("Startup map", set_startup_map)
    step("Content packs", content_report)

    summary = "\n".join(results)
    log("summary:\n" + summary)
    failed = any(r.startswith("FAILED") for r in results)
    try:
        unreal.EditorDialog.show_message(
            "CentralEuropeTest - build world",
            summary + ("\n\nSome steps failed. Send the Output Log lines starting with [build_world]." if failed
                       else "\n\nDone. Press Play."),
            unreal.AppMsgType.OK)
    except Exception:
        pass


main()
