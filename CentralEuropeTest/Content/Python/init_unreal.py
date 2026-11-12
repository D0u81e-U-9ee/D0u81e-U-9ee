"""Runs automatically every time the Unreal Editor opens this project.

Imports the WAV files from SourceArt/Audio into /Game/CE/Audio (only the ones that are not
there yet) and flags the ambience / engine sounds as looping. UCEAudioSubsystem then finds them
by name. Delete /Game/CE/Audio in the Content Browser to force a fresh import.
"""
import os

import unreal

_DEST = "/Game/CE/Audio"
_state = {"ticks": 0, "handle": None}


def _import_audio():
    src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Audio")
    if not os.path.isdir(src):
        return
    tasks = []
    for name in sorted(os.listdir(src)):
        if not name.lower().endswith(".wav"):
            continue
        asset = name[:-4]
        if unreal.EditorAssetLibrary.does_asset_exist("%s/%s" % (_DEST, asset)):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(src, name))
        task.set_editor_property("destination_path", _DEST)
        task.set_editor_property("destination_name", asset)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    if not tasks:
        return

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = 0
    for task in tasks:
        asset = task.get_editor_property("destination_name")
        path = "%s/%s" % (_DEST, asset)
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.log_warning("[CE audio] import failed: " + asset)
            continue
        imported += 1
        if asset.startswith(("Amb_", "Cart_")):
            wave = unreal.EditorAssetLibrary.load_asset(path)
            try:
                wave.set_editor_property("looping", True)
                unreal.EditorAssetLibrary.save_asset(path, False)
            except Exception as exc:  # the game also sets the flag at runtime
                unreal.log_warning("[CE audio] could not flag %s as looping: %s" % (asset, exc))
    unreal.log("[CE audio] imported %d sound(s) into %s" % (imported, _DEST))


def _tick(_delta):
    # Wait a few seconds so the asset registry has finished its first scan.
    _state["ticks"] += 1
    if _state["ticks"] < 180:
        return
    unreal.unregister_slate_post_tick_callback(_state["handle"])
    try:
        _import_audio()
    except Exception as exc:
        unreal.log_warning("[CE audio] import skipped: %s" % exc)


_state["handle"] = unreal.register_slate_post_tick_callback(_tick)
