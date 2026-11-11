"""Editor helper - run inside the Unreal Editor with the target level open:

    Tools > Execute Python Script...  ->  Tools/ue/setup_level.py

What it does (idempotent, re-run any time):
  1. Finds or spawns the lighting stack and applies the settings from the design document
     (movable sun with Atmosphere Sun Light, real-time-capture sky light, sky atmosphere,
     volumetric clouds, exponential height fog with volumetric fog).
  2. Places a PlayerStart on the valley loop road.
  3. Drops labelled TargetPoint markers for every river / road / waterfall control point from
     SourceArt/Terrain/terrain_layout.json, so water body splines and landscape splines can be
     snapped to them by hand.

NOT automated (no stable Python API): landscape import, World Partition grid values, data layer
assignment, water body spline editing. See docs/PROTOTYPE_SETUP.md for those steps.

This script has not been executed by its author (no editor in the build environment). Every
property write is wrapped so one renamed property cannot abort the run - check the Output Log.
"""
import json
import os

import unreal

LAYOUT = os.path.join(unreal.Paths.project_dir(), "SourceArt", "Terrain", "terrain_layout.json")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def log(msg):
    unreal.log("[setup_level] " + msg)


def find_or_spawn(cls, label, location=unreal.Vector(0, 0, 0)):
    for a in actors.get_all_level_actors():
        if isinstance(a, cls):
            return a
    a = actors.spawn_actor_from_class(cls, location)
    a.set_actor_label(label)
    a.set_folder_path("Lighting")
    log("spawned " + label)
    return a


def set_props(obj, props):
    for name, value in props.items():
        try:
            obj.set_editor_property(name, value)
        except Exception as exc:  # property renamed between engine versions
            unreal.log_warning("[setup_level] %s.%s not set: %s" % (obj.get_name(), name, exc))


def movable(actor):
    try:
        actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    except Exception as exc:
        unreal.log_warning("[setup_level] mobility not set on %s: %s" % (actor.get_name(), exc))


def setup_lighting():
    sun = find_or_spawn(unreal.DirectionalLight, "Sun", unreal.Vector(0, 0, 150000))
    movable(sun)
    sun.set_actor_rotation(unreal.Rotator(0, -42, 215), False)   # roll, pitch, yaw: late-morning sun from the south
    set_props(sun.get_component_by_class(unreal.DirectionalLightComponent), {
        "intensity": 10.0,
        "atmosphere_sun_light": True,
        "atmosphere_sun_light_index": 0,
        "cast_shadows": True,
        "cast_dynamic_shadows": True,
        "light_source_angle": 0.5357,
    })

    sky = find_or_spawn(unreal.SkyLight, "SkyLight", unreal.Vector(0, 0, 150000))
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
        "fog_density": 0.01,
        "fog_height_falloff": 0.2,
        "volumetric_fog": True,
        "volumetric_fog_scattering_distribution": 0.2,
        "volumetric_fog_distance": 6000.0,
    })


def marker(label, p, folder):
    for a in actors.get_all_level_actors():
        if a.get_actor_label() == label:
            a.set_actor_location(unreal.Vector(p["x"], p["y"], p["z"]), False, True)
            return
    a = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(p["x"], p["y"], p["z"]))
    a.set_actor_label(label)
    a.set_folder_path(folder)


def setup_markers():
    if not os.path.exists(LAYOUT):
        unreal.log_warning("[setup_level] %s missing - run Tools/generate_terrain.py first" % LAYOUT)
        return
    with open(LAYOUT) as f:
        layout = json.load(f)

    for name, pts in layout["rivers"].items():
        for i, p in enumerate(pts):
            marker("%s_%02d_w%d" % (name, i, p["width"]), p, "Markers/Rivers/" + name)
    for name, road in layout["roads"].items():
        for i, p in enumerate(road["points"]):
            marker("%s_%02d" % (name, i), p, "Markers/Roads/" + name)
    for fall in layout["waterfalls"]:
        marker(fall["name"] + "_Lip", fall["lip"], "Markers/Waterfalls")
        marker(fall["name"] + "_Base", fall["base"], "Markers/Waterfalls")
        marker(fall["name"] + "_ExclusionCentre", fall["exclusion_box_center"], "Markers/Waterfalls")

    start = layout["player_start"]
    ps = None
    for a in actors.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart):
            ps = a
            break
    loc = unreal.Vector(start["x"], start["y"], start["z"] + 150)
    if ps is None:
        ps = actors.spawn_actor_from_class(unreal.PlayerStart, loc)
    else:
        ps.set_actor_location(loc, False, True)
    log("player start at %s" % loc)


with unreal.ScopedEditorTransaction("CentralEuropeTest level setup"):
    setup_lighting()
    setup_markers()
log("done")
