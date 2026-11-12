# CentralEuropeTest — UE5 prototype

First playable slice of the design in [`../docs/UE5_OpenWorld_Combat_TDD.md`](../docs/UE5_OpenWorld_Combat_TDD.md):
a generated 2 × 2 km alpine terrain and a C++ character / combat framework that runs **without any
authored assets** (inputs, weapons and targets are built in code, with placeholders for meshes).

> **Status: not compiled or run in Unreal.** This was written in an environment without the engine.
> The terrain generator was run and its output checked; the C++ and the editor Python script were
> only reviewed by hand. Expect to fix a few compile errors on first build (target: UE 5.5).

## What is here

| Path | Contents |
|---|---|
| `CentralEuropeTest.uproject`, `Config/` | Project, plugins (Water, Chaos Vehicles, PCG, Python), Lumen + VSM renderer settings, Enhanced Input classes, the `Weapon` / `Melee` / `Projectile` collision channels |
| `Source/CentralEuropeTest/` | Gameplay module (below) |
| `Tools/generate_terrain.py` | Terrain generator (numpy only) — heightmap, layout JSON, preview |
| `SourceArt/Terrain/` | Generated `heightmap_2017.png` (16-bit), `terrain_layout.json`, `terrain_preview.png` |
| `SourceArt/Materials/LandscapeAutoWeights.ush` | Slope / height / snow weights for a material Custom node |
| `Content/Data/DT_Weapons.csv` | Weapon rows, importable as a DataTable of `CEWeaponData` |
| `Tools/ue/build_world.py` | Editor script: builds the real level in one run (see below) |
| `Tools/ue/setup_level.py` | Editor script: river / road / waterfall markers for laying water splines |
| `Tools/generate_sounds.py`, `SourceArt/Audio/` | Synthesised placeholder sound set |
| `Content/Python/init_unreal.py` | Runs at editor start: imports the sounds |

### C++ classes (parents for the Blueprints named in the design)

| Class | Blueprint to derive | Role |
|---|---|---|
| `ACEBaseCharacter` | `BP_BaseCharacter` | Walk / sprint / crouch / swim / in-vehicle, third-person camera, aim FOV |
| `UCECombatComponent` | `BP_CombatComponent` | Slots, fire timing, hitscan, projectile spawn, melee sweeps, recoil, anim-layer linking |
| `FCEWeaponData` | `ST_WeaponData` | DataTable row; the nine specified fields plus optional extensions |
| `ACESniperBullet` | `BP_SniperBullet` | 85 000 cm/s, gravity scale 1.0, swept, sub-stepped |
| `UCEAnimNotifyState_MeleeTrace` | `ANS_MeleeTrace` | Opens / ticks / closes the melee damage window |
| `UCECombatAnimInstance` | `ABP_Base` | Publishes speed, direction, state, aim offsets, weapon type |
| `UCEInputSubsystem` | — | Builds `IA_*` and `IMC_Default / IMC_Combat / IMC_Vehicle` in code |
| `ACEPlayerController` | — | `IA_Interact`, vehicle enter / exit, context swapping |
| `ICEInteractable`, `UCEVehicleSeatComponent` | `BPI_Interactable` | Add the seat component to any vehicle pawn to make it drivable |
| `ACETestDummy`, `UCEHealthComponent` | `BP_TestDummy` | Target that logs every hit with damage and range |
| `ACEActorSpawner` | `BP_VehicleSpawner` | Ground-snapped spawner for the `DL_Spawners` layer |
| `ACETestGameMode` | `BP_TestGameMode` | Default pawn / controller, preview terrain, dummy range at 3 / 25 / 100 / 300 / 500 m, test carts |
| `UCEBlockMannequinComponent` | — | Asset-free animated stand-in body with hit volumes and weapon attach points |
| `ACETerrainPreviewActor` | — | Runtime mesh of the heightmap with collision, lake plane and swimming |
| `ACETestCart` | — | Kinematic stand-in vehicle (fast and truck profiles) for enter / exit and road-grade tests |
| `UCEAudioSubsystem`, `ACEAmbienceActor` | — | Sound playback by name; position-based ambience mix |
| `UCEEditorLibrary` | — | `ImportTerrainLandscape`, called by the build script |

## Controls

WASD move · mouse look · Space jump · Shift sprint · C / Ctrl crouch · E interact (enter / exit vehicle)
· LMB fire · RMB aim · R reload · 1 pistol · 2 rifle · 3 sniper · 4 knife (press the active slot again for fists) · mouse wheel or Q cycles weapons.
In a cart: W / S throttle and reverse · A / D steer · Space brake · E exit.
Gamepad is mapped too. `combat.Debug 0` hides tracers and melee sweeps.

## First playtest (no editor work needed)

1. **Build.** Right-click the `.uproject` → *Generate Visual Studio project files*, build *Development Editor*.
   If your engine is not 5.5, change `EngineAssociation` first.
2. **Open the project and press Play.** The startup map is the engine's basic template level. Because it has
   no Landscape, the game mode builds the generated heightmap as a **runtime preview terrain** (4 m grid,
   coloured by the auto-material rules, lake at Z = 0) and starts you on the valley loop road.
3. **What is there to test**
   - Five mannequin targets at 3 / 25 / 100 / 300 / 500 m up the foothills. Each hit is logged on screen with
     damage, range and HEAD / body. The start point was chosen so all five are in plain sight; if a target
     would be hidden, the range slides it sideways.
   - Two stand-in carts to your right (E to enter / exit): a fast one and a truck-profile one that shows
     speed and road grade. They are kinematic test vehicles, **not Chaos vehicles**.
   - The lake: follow the loop road and the harbour branch (about 600 m south-west) and walk in to swim.

The preview terrain is a playtest stand-in only: no World Partition streaming, no Landscape material, no
Water plugin, and it works in the editor / PIE only (it reads `SourceArt/`). It switches itself off as soon
as the level contains a real Landscape. Disable it with `bAutoPreviewTerrain` on the game mode.

## The mannequin

- **Always present:** a code-built block mannequin (180 cm, jointed, built from engine basic shapes). It walks,
  sprints, crouches, swims, raises the weapon to aim and punches; its head is a separate hit volume, so
  headshots work. The test targets use the same figure in orange.
- **Automatic upgrade to the UE5 Mannequin:** Epic's Manny / Quinn assets ship with the engine, not with this
  repository. In the Content Browser choose *Add → Add Feature or Content Pack → Third Person*. On the next
  Play, `ACEBaseCharacter` finds `SKM_Manny_Simple` and its Anim Blueprint and uses them instead; weapons
  attach to the `hand_r` / `thigh_r` / `spine_03` / `spine_02` bones until the named sockets are authored
  (tune the grip with `HandAttachOffset` on the combat component). The asset paths are editable on the
  character if your engine version lays the pack out differently.

## Building the real level (one script)

1. Open the project. On the first start after this update the editor adds Epic's **Starter Content** and
   **Third Person** packs by itself (ground textures, wind / bird recordings, the UE5 Mannequin) and imports
   the sound set. If either pack is missing afterwards, add it by hand: Content Browser → *Add* →
   *Add Feature or Content Pack*.
2. Menu **Tools → Execute Python Script…** and choose `Tools/ue/build_world.py`.
3. Read the summary dialog. Each step reports OK or FAILED:
   - creates the level `/Game/Maps/L_CentralEuropeTest`;
   - builds the landscape material (slope / height / snow blending, textured from Starter Content, gravel
     strip along the roads) and a rippling lake material;
   - imports the heightmap as a real Landscape (1 m detail instead of the 4 m preview mesh);
   - places the sun, sky light, sky atmosphere, volumetric clouds and height fog;
   - places the PlayerStart, saves, and makes the level the startup map.
4. Press Play. The preview mesh is gone; the lake surface and swimming still come from the stand-in until
   Water plugin bodies are added.

Still manual afterwards: Water plugin lake / rivers (`Tools/ue/setup_level.py` drops markers for the
splines), World Partition conversion and data layers, HLOD, foliage, combat animation
(`CECombatAnimInstance`, sockets on the skeleton), and real vehicles (add a `CEVehicleSeatComponent` to the
vehicle pawn and spawn it with a `CEActorSpawner`).

## Sound

`Tools/generate_sounds.py` synthesises 26 placeholder sounds into `SourceArt/Audio` (gunshots per weapon,
dry fire, reload, equip, bullet impact, hit confirm, melee swing / hit, footsteps ×4, jump, land, splash,
swim stroke, target down, cart engine, and loops for wind, birds, lake, river and waterfall). They are made
from filtered noise and tone sweeps: functional stand-ins, not recordings, and they have not been judged
by ear. `Content/Python/init_unreal.py` imports them into `/Game/CE/Audio` when the editor starts.

`UCEAudioSubsystem` plays a sound by its file name and skips missing ones, so any sound can be replaced by
importing a better one under the same name. `ACEAmbienceActor` mixes the ambience from the listener's
position: wind grows with altitude, birdsong fades above the tree line, the lake is heard near the shore,
and river / waterfall emitters start when you come within earshot. Where Starter Content is installed its
wind and bird recordings are used instead of the synthesised ones.

## Terrain facts (from the generator's own report)

- Heights −75 m … 1199.9 m; lake (Z < 0) covers about 25 % of the map along the south and west edges.
- 99 % of the cliff band on the north / east ridges is steeper than 60°; 14 % of the map is above the 850 m snow line.
- Roads: valley loop 1443 m plus three branches; steepest 5 m stretch is **8.3 %** (limit 10 %). The generator fails if any road exceeds 10 %.
- Two rivers start at Z = 600 m, drop about 527 m at a waterfall headwall, and reach the lake with a monotonically falling water surface.
- Image orientation: column → world X (north), row → world Y (east), so in the preview north is to the right.

Regenerate with `python3 Tools/generate_terrain.py` (`--seed N` for a different ridge line).

## Deviations from the design document

- **C++ instead of Blueprint assets.** Binary `.uasset` files cannot be authored outside the editor, so each
  specified Blueprint is a C++ class you can subclass.
- **Input assets are built in code** rather than as `IA_` / `IMC_` assets. `IA_WeaponSlot1_4` is four button actions (`IA_WeaponSlot1` … `4`) plus next / previous on the mouse wheel.
- **Road beds are baked into the heightmap** (the document said to cut them with landscape splines) so the grades are verifiable now. Splines can still be laid on the markers for road meshes.
- **Foothills were reshaped**: a 19° meadow band, then a 50° rock flank, then the cliff band. The ridge is too narrow for the 25–35° forest slope the document described.
- **Single-player authority only.** Damage is applied on the owning machine; there are no server RPCs yet.
- **Playtest stand-ins** that the document does not describe: the runtime preview terrain, the block mannequin and the kinematic test carts. They exist so a first playtest needs no authored assets; each is replaced by the real thing (Landscape, UE5 Mannequin, Chaos vehicles) without code changes.

## Not done yet

Water bodies, data layers, HLOD layers, the landscape material asset, PCG foliage, animation graphs
(`ALI_Combat` layers), and vehicle / boat assets all need the editor. Weapon meshes, audio and VFX are placeholders.
