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
| `Content/Data/DT_Weapons.csv` | Weapon rows, importable as a DataTable of `WeaponData` |
| `Tools/ue/setup_level.py` | Editor script: lighting stack, PlayerStart, river / road markers |

### C++ classes (parents for the Blueprints named in the design)

| Class | Blueprint to derive | Role |
|---|---|---|
| `ABaseCharacter` | `BP_BaseCharacter` | Walk / sprint / crouch / swim / in-vehicle, third-person camera, aim FOV |
| `UCombatComponent` | `BP_CombatComponent` | Slots, fire timing, hitscan, projectile spawn, melee sweeps, recoil, anim-layer linking |
| `FWeaponData` | `ST_WeaponData` | DataTable row; the nine specified fields plus optional extensions |
| `ASniperBullet` | `BP_SniperBullet` | 85 000 cm/s, gravity scale 1.0, swept, sub-stepped |
| `UAnimNotifyState_MeleeTrace` | `ANS_MeleeTrace` | Opens / ticks / closes the melee damage window |
| `UCombatAnimInstance` | `ABP_Base` | Publishes speed, direction, state, aim offsets, weapon type |
| `UPrototypeInputSubsystem` | — | Builds `IA_*` and `IMC_Default / IMC_Combat / IMC_Vehicle` in code |
| `APrototypePlayerController` | — | `IA_Interact`, vehicle enter / exit, context swapping |
| `IInteractable`, `UVehicleSeatComponent` | `BPI_Interactable` | Add the seat component to any vehicle pawn to make it drivable |
| `ATestDummy`, `UHealthComponent` | `BP_TestDummy` | Target that logs every hit with damage and range |
| `AActorSpawner` | `BP_VehicleSpawner` | Ground-snapped spawner for the `DL_Spawners` layer |
| `ATestGameMode` | `BP_TestGameMode` | Default pawn / controller, spawns a dummy range at 3 / 25 / 100 / 300 / 500 m |

## Controls

WASD move · mouse look · Space jump · Shift sprint · C / Ctrl crouch · E interact (enter / exit vehicle)
· LMB fire · RMB aim · R reload · 1 pistol · 2 rifle · 3 sniper · 4 knife (press the active slot again for fists).
Gamepad is mapped too. `combat.Debug 0` hides tracers and melee sweeps.

## Getting it running

1. **Build.** Right-click the `.uproject` → *Generate Visual Studio project files*, build *Development Editor*.
   If your engine is not 5.5, change `EngineAssociation` first.
2. **Quick combat test (no terrain needed).** Open any level with a floor and a PlayerStart and press Play.
   The game mode is already the project default; five dummies appear in front of you.
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
5. **Mannequin (optional).** Add the Third Person content, derive `BP_BaseCharacter` from `ABaseCharacter`,
   assign the Manny mesh and an Anim Blueprint parented to `CombatAnimInstance`, and add the sockets
   `hand_r_socket`, `holster_thigh_r`, `spine_rifle_socket`, `spine_sniper_socket` to the skeleton.
   Until then the character is a capsule with placeholder box weapons.
6. **Vehicles (optional).** Add the Vehicle template content, add a `VehicleSeatComponent` to its pawn Blueprint,
   and place an `ActorSpawner` with that Blueprint as `SpawnClass`.

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
- **Input assets are built in code** rather than as `IA_` / `IMC_` assets. `IA_WeaponSlot1_4` is one Axis1D action whose value is the slot number.
- **Road beds are baked into the heightmap** (the document said to cut them with landscape splines) so the grades are verifiable now. Splines can still be laid on the markers for road meshes.
- **Foothills were reshaped**: a 19° meadow band, then a 50° rock flank, then the cliff band. The ridge is too narrow for the 25–35° forest slope the document described.
- **Single-player authority only.** Damage is applied on the owning machine; there are no server RPCs yet.

## Not done yet

Water bodies, data layers, HLOD layers, the landscape material asset, PCG foliage, animation graphs
(`ALI_Combat` layers), and vehicle / boat assets all need the editor. Weapon meshes, audio and VFX are placeholders.
