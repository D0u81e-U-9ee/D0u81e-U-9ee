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
| `Tools/ue/setup_level.py` | Editor script: lighting stack, PlayerStart, river / road markers |

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

## Building the real level

1. Steps 1–2 above.
2. (Optional) Quick combat test in any level with a floor and a PlayerStart: set `bAutoPreviewTerrain` off.
3. **Terrain level.**
   1. *File → New Level → Open World* and save as `Content/Maps/L_CentralEuropeTest`. Delete the template landscape.
   2. *Landscape mode → Manage → New → Import from File*: `SourceArt/Terrain/heightmap_2017.png`.
      Section size **63×63**, sections per component **1×1**, components **32×32**, resolution **2017×2017**,
      location **(-100800, -100800, 56000)**, scale **(100, 100, 250)**, World Partition grid size **2**.
   3. *World Settings → World Partition*: runtime grid cell size **12800**, loading range **25600**.
   4. *Tools → Execute Python Script* → `Tools/ue/setup_level.py` (lighting, PlayerStart, markers).
   5. Water: add a **Water Body Lake** at Z = 0 around the south / west basin and a **Water Zone** at Z = 0;
      lay **Water Body River** splines along the `River_*` markers (marker names carry the width),
      and a **Water Body Exclusion Volume** at each `*_ExclusionCentre` marker.
   6. Create the three data layers (`DL_Environment`, `DL_RoadsAndInfrastructure`, `DL_Spawners`) and assign actors.
4. **Landscape material.** New material → Custom node with the contents of `LandscapeAutoWeights.ush`
   (inputs `VertexNormalWS`, `WorldPos`, `Noise`; additional output `Snow`), blend four layer texture sets by
   the returned weights, triplanar on the cliff layer, then lerp to snow.
5. **Combat animation (optional).** Derive `BP_BaseCharacter` from `ACEBaseCharacter`, give it an Anim Blueprint
   parented to `CECombatAnimInstance`, and add the sockets `hand_r_socket`, `holster_thigh_r`,
   `spine_rifle_socket`, `spine_sniper_socket` to the mannequin skeleton.
6. **Vehicles (optional).** Add the Vehicle template content, add a `CEVehicleSeatComponent` to its pawn Blueprint,
   and place an `CEActorSpawner` with that Blueprint as `SpawnClass`.

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
