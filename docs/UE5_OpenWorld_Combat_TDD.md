# UE5 Central European Open World Test Environment + Modular Combat Framework
**Technical Design Document & Implementation Roadmap** — target engine: UE 5.5–5.7 (setting names verified against 5.5 conventions; items marked ⚠ must be re-checked in your exact version).

**Guiding principle (Pareto 80/20):** spend effort on (1) streaming/scalability, (2) data-driven weapons, (3) component-based combat, (4) validated traversability. Everything else is placeholder-grade until those work.

**Conventions:** Units = cm. World origin = map center. **+X = North, +Y = East**. Z = 0 is lake level.

---

## 0. Project-Level Configuration (do first)

| Setting (Project Settings) | Value |
|---|---|
| Default RHI | DirectX 12, SM6 |
| Dynamic Global Illumination | Lumen |
| Reflection Method | Lumen |
| Shadow Map Method | **Virtual Shadow Maps** |
| Generate Mesh Distance Fields | ✔ (software Lumen) |
| Support Hardware Ray Tracing | ✘ (enable later as option) |
| Allow Static Lighting | ✘ (`r.AllowStaticLighting=0`) |
| Nanite | ✔ |
| Default Pawn / Game Mode | `BP_TestGameMode` → `BP_BaseCharacter` |
| Default Input Classes | EnhancedPlayerInput / EnhancedInputComponent |
| Plugins | Water, Enhanced Input, Chaos Vehicles, Niagara, PCG, Sun Position (optional), Python Editor Script (validation tools) |

**Custom collision channels** (Project Settings → Collision):

| Name | Type | Default | Purpose |
|---|---|---|---|
| `Weapon` (GameTraceChannel1) | Trace | Block | Hitscan |
| `Melee` (GameTraceChannel2) | Trace | Block | Melee sweeps |
| `Projectile` (GameTraceChannel3) | Object | — | Ballistic bullets |

**Presets:** Capsule `Pawn` → Weapon=Ignore, Melee=Ignore (prevents double hits; the physics-asset mesh is the hit target). `CharacterMesh` → Weapon=Block, Melee=Block, Projectile=Overlap/Block. `BulletProjectile` object=Projectile, blocks WorldStatic/WorldDynamic/Pawn/PhysicsBody/Vehicle/Destructible, ignores Water.

**Folder structure**
```
/Game/Project/
  Maps/L_CentralEuropeTest (+ __ExternalActors__ via OFPA)
  World/{Landscape, Materials, Water, Foliage, PCG, HLOD}
  Core/{Input, Characters, Combat, Weapons, Anim, Vehicles, Interfaces}
  Data/{DT_Weapons, ST_WeaponData, Enums}
  Test/{BP_TestDummy, BP_VehicleSpawner, BP_LootPoint}
```

---

## MODULE 1 — WORLD & TOPOGRAPHY

### 1.1 Landscape Grid

| Parameter | Value | Derivation |
|---|---|---|
| Overall resolution | **2017 × 2017** | 32 × 63 + 1 |
| Quads / section | **63 × 63** | |
| Sections / component | **1 × 1** | → 64×64 verts per component |
| Components | **32 × 32 = 1024** | |
| Scale | X=100, Y=100, **Z=250** | 1 m/px; see height math |
| Extents | 2016 m × 2016 m | |
| Actor location | **(-100800, -100800, 56000)** | centers map on origin; Z offset explained below |
| World Partition landscape grid | **2 × 2 components per proxy (126 m)** → 16×16 = 256 proxies | closest fit to 128 m runtime cell, avoids proxies straddling many cells |
| Edit Layers | ✔ (water brush, road splines, manual sculpt) | |
| Nanite Landscape | ✔ recommended on 5.5+ ⚠ (fallback: classic LOD) | |
| Collision MIP | 0 (vehicles need accurate heightfield) | |

**Height math (16-bit, no waste):**
- Local Z = (h − 32768) / 128 × ZScale → with ZScale 250: **1.953125 cm per height unit**; full range = 65536 × 1.953125 = **1280 m**.
- Actor Z = **+560 m (56000 cm)** → covers **−80 m … +1199.98 m** (lake bed −80 m, ridge cap 1200 m).
- Mapping: `h = 4096 + Z_cm / 1.953125` → Z=0 (lake surface) = **h 4096**; Z=1200 m = 65535 (clamp).
- Resolution: ~2 cm vertical — plenty for road grades.

**Heightmap authoring pipeline (recommended):** Gaea / World Machine / Houdini → export **16-bit RAW (.r16) 2017×2017**, import via Landscape → Import from File. Hand-sculpt only fixes. Roads are cut afterward with splines (do not bake them into the base).

### 1.2 World Partition

| Setting | Value |
|---|---|
| Enable Streaming | ✔ (World Settings → World Partition Setup) |
| Use External Actors (**OFPA**) | ✔ (default for WP; verify on `L_CentralEuropeTest`) |
| Runtime Grid `MainGrid` | **Cell Size 12800**, **Loading Range 25600**, Priority 0, Debug color default |
| Runtime Grid `HLOD0` | Cell 25600, Loading Range 76800 (auto from HLOD layer) |
| Runtime Grid `HLOD1` | Cell 51200, Loading Range = whole map (252000) |
| Streaming Source | PlayerController (default); add `WorldPartitionStreamingSourceComponent` to possessed vehicles (shape = sphere, 25600) |
| Block-on-slow-streaming | ✔ on teleport/respawn only |

Cell budget: radius 256 m over 128 m cells ≈ **16–21 active cells** (~1.5 % of the map). At 30 m/s truck speed the player has ≈ 8.5 s of lookahead — acceptable; HLOD covers visuals beyond.

**HLOD Layers** (`/Game/Project/World/HLOD`):

| Asset | Type | Contents | Notes |
|---|---|---|---|
| `HLOD_Terrain` | Merged/Simplified Mesh (landscape HLOD path) ⚠ | Landscape proxies | Material: baked low-cost version of `M_LandscapeAuto` (RVT baked color) |
| `HLOD_Structures` | Approximate Mesh | Bridges, signs, buildings | Merge distance 100, screen size 300 |
| `HLOD_Vegetation` | Instancing | Foliage / PCG trees | Optional (Pareto: skip for week-1; impostor via Nanite) |
| Spawners / dummies / loot | **No HLOD** (Is Spatially Loaded ✔, HLOD Layer = None) | | |

### 1.3 Data Layers (`DataLayerAsset`s, Type = Runtime)

| Asset | Contents | Initial Runtime State | Debug color |
|---|---|---|---|
| `DL_Environment` | Landscape proxies, foliage/PCG, water bodies, rocks | **Activated** | green |
| `DL_RoadsAndInfrastructure` | Landscape splines, road meshes, bridges, signs | **Activated** | grey |
| `DL_Spawners` | Vehicle spawners, test dummies, loot points | **Loaded→Activated by test GameMode** (toggle via `Set Data Layer Runtime State`) | orange |

Rule: gameplay actors ONLY on `DL_Spawners` so test content can be unloaded for pure streaming/perf captures.

### 1.4 Topology (macro layout, metres from center)

```
            +X (North)
   ┌───────────────────────────┐
   │  CLIFF/RIDGE   CLIFF/RIDGE│  North band: X > +350, crest Z 1000–1200
   │  ┌──────valley──────┐  NE │
   │  │  road network    │ CLIFF│  East band: Y > +350
-Y │  └──────────────────┘     │ +Y (East)
   │ LAKE basin (SW)           │
   │ centre (-600,-600), r≈450 │
   └───────────────────────────┘
```

| Zone | Spec |
|---|---|
| North/East ridges | Valley wall begins at 350 m from center; crest 900–1000 m. Rise ≈ 1100 m over ≈ 600 m → average ≥ 60°. **Design as banded**: forest slope 25–35° up to Z≈500, **cliff band 60–75° from Z≈500 to ≈1100**, snow plateau above. (A uniform 60° over 1100 m would need ≥ 635 m run — use bands, verify in §1.4 validation.) |
| South/West lake | Shoreline ≈ 450 m radius, bed to **−60…−80 m**, water surface **Z = 0** |
| Central valley | Floor rises from Z≈3 m at the lake to ≈ 100 m at the NE valley head (≈ 6 % average over ≈ 1.6 km) |
| Roads | Main loop + 2 branches; **max grade 10 % (5.71°)**, design 8 % (4.57°); 8 m carriageway; min curve radius 40 m (heavy trucks); all cut/fill via Landscape Splines on a `Roads` edit layer |

**Road creation:** Landscape Mode → Manage → Splines; Control point *Width* (verify half-width semantics) so carriageway = 800 cm, *Side Falloff* 600, *Raise/Lower Terrain* ✔, Layer Name = `RoadGravel`, Spline Mesh = road segment mesh on `DL_RoadsAndInfrastructure`.

**Validation (Python editor script, run every landscape change):**
```python
# Sample each LandscapeSplineSegment at 1 m steps; fail if |dz/ds| > 0.10
import unreal, math
def check(points):
    bad=[]
    for a,b in zip(points,points[1:]):
        ds=math.hypot(b.x-a.x,b.y-a.y)
        if ds>0 and abs(b.z-a.z)/ds>0.10: bad.append((a,b))
    return bad
```
Acceptance: zero violations; truck (≈ 18 t, Chaos) climbs a 10 % section at ≥ 15 km/h.

### 1.5 Water System (Water plugin)

| Item | Setting |
|---|---|
| `WaterZone` | Location Z = **0**; extent **210000 × 210000** (whole map); render target **2048²** (≈ 1 m/texel); ⚠ increase *Capture Z Offset* so the highest water (600 m) falls inside the capture; **Plan B:** second `WaterZone` "Highland" at Z=600 m covering the river sources |
| `WaterBodyLake` "Lake_Main" | Water surface Z=0; spline loop along shoreline; Affects Landscape ✔; Water Brush on landscape edit layer `WaterCarve` |
| `WaterBodyRiver` ×2–3 | Points from source Z≈**600 m** descending; per-point *River Width* 600–2500, *Depth* 150–400, *Velocity* 150–400. **Z must be monotonically non-increasing.** Last point lands inside Lake_Main (Z=0) using the plugin's River-to-Lake transition |
| Waterfall | Split river into **River_Upper** (ends at cliff lip) and **River_Lower** (starts at cliff base). Between them a **`WaterBodyExclusionVolume`** (box enclosing the vertical drop, ±5 m margin) removes the stretched river mesh. Fill with `NS_Waterfall` (GPU ribbon + mist sprites, wind-driven) and optional `SM_WaterfallSheet` (scrolling-normal material). Audio: looped cue at base |
| Swimming | Water bodies' physics volume (`bWaterVolume`) → `CharacterMovement` enters `MOVE_Swimming` automatically; verify capsule enters at ≥ chest depth |
| Boats | `BP_Boat` with Water plugin `BuoyancyComponent` (pontoons ×4) — interact target for `IA_Interact` |

### 1.6 Landscape Auto-Material (`M_LandscapeAuto`)

**Inputs:** `Nz = VertexNormalWS.z` (use *vertex* normal, not pixel normal, so detail normal maps don't jitter slope); `Z = AbsoluteWorldPosition.z` (cm).
`SlopeDeg = ACos(saturate(Nz)) × 57.29578`.

| Layer | Slope | Height | Texture set | Mapping |
|---|---|---|---|---|
| L1 ForestFloor_Humus | 0–15° | Z < 500 m (50000) | BC + packed N/ORD | UV, 2 tile sizes (macro/micro) |
| L2 AlpineMeadow_Grass | 15–30° (and flat ground 500–700 m) | Z < 700 m (70000) | BC + packed | UV + Grass Output |
| L3 Scree_Gravel | 30–45° (and flat ground > 700 m) | any | BC + packed | UV |
| L4 CliffRock | **> 45°** | any | BC + packed | **Triplanar** (`WorldAlignedTexture`, blend sharpness 4, tile 800 cm) |
| Snow (overlay) | reduced on slopes > 50° | **Lerp 85000 → 100000** | BC + packed | UV |

Gap-resolution decision: flat ground above 500 m would be uncovered in a literal spec → falls to L2 until 700 m, then L3 below the snow line.

**Weights (smoothstep `S(a,b,x)` with 6° transition bands to avoid hard lines):**
```
wCliff  = S(42, 48, Slope)
wScree  = saturate(max(S(27, 33, Slope), S(65000, 70000, Z))) * (1 - wCliff)   // steep OR flat above 700 m
Rest    = 1 - wCliff - wScree
wForest = Rest * (1 - S(12, 18, Slope)) * (1 - S(45000, 50000, Z))            // flat AND below 500 m
wMeadow = Rest - wForest                                                       // weights sum to 1 by construction
SnowAlpha = saturate((Z - 85000)/15000) * saturate((Nz - 0.5)/0.3) * NoiseBreakup(0.8..1.2)
FinalColor = Lerp(Blend(w_i), SnowColor, SnowAlpha)   // also drive Roughness/Normal
```
Equivalent cos-thresholds for the slope math: 15°→0.966, 30°→0.866, 45°→0.707, 60°→0.5.

**Manual override:** wrap the procedural result with `LandscapeLayerBlend` where painted weights (`Road`, `Mud`, `Override_*`) win over auto weights.

**Cost controls:** shared `Wrap` sampler source (stay under 16 samplers); channel-packed textures (ORD); use **Runtime Virtual Texture** (BaseColor/Normal/Roughness/WorldHeight) on the landscape → one-time cost, enables foliage/mesh blending; Nanite landscape ignores unsupported nodes ⚠. Triplanar = 3 samples of one texture only on wCliff>0 (dynamic branch).
**Foliage:** Landscape Grass Output reads the same weights (forest grass/ferns on L1/L2). Trees/rocks via **PCG** graph with slope (<30°) and height (<900 m) filters, runtime generation on WP grid.

### 1.7 Atmosphere & Lighting

| Actor | Settings |
|---|---|
| Directional Light | Mobility **Movable**; Cast Shadows ✔; Cast Dynamic Shadows ✔; **Atmosphere Sun Light ✔, Index 0**; Intensity 10 lux (EV-based); Shadow Method inherits **VSM**; Source Angle 0.5357° |
| Sky Light | Mobility **Movable**; Source = SLS Captured Scene; **Real Time Capture ✔**; Cast Shadows ✔; Lower Hemisphere Color black; Intensity 1 |
| SkyAtmosphere | Planet Radius 6360 km; Transform mode: *Planet Top at Component Transform* at Z=0; Rayleigh/Mie defaults; Ground Albedo (0.3,0.3,0.3) |
| VolumetricCloud | Layer bottom altitude 5 km, height 10 km, tracing max distance 50 km; cloud material `M_Cloud_Default`; ✔ Affect lighting on sky capture |
| ExponentialHeightFog | **Volumetric Fog ✔**; Density 0.01; Height Falloff 0.2; Start 0; Volumetric View Distance 6000 (extend to 12000 if perf allows); Scattering Distribution 0.2 |
| Post Process Volume (unbound) | Exposure: Auto, Min/Max EV 5 / 14 ; Lumen Scene Detail 1.0; Final Gather Quality 1; Method Lumen |
| Time of day | Sun Position plugin at 47.5° N, 13° E (Alpine Central Europe) or a `BP_TimeOfDay` rotating the light |

---

## MODULE 2 — CHARACTER & COMBAT ARCHITECTURE

### 2.1 Enhanced Input

**Input Actions**

| Asset | Value Type | Triggers | Modifiers |
|---|---|---|---|
| `IA_Move` | Axis2D | — | (in IMC) W: Swizzle YXZ; S: Swizzle+Negate; A: Negate; D: none |
| `IA_Look` | Axis2D | — | Mouse XY with Negate Y; gamepad stick DeadZone 0.2 + Scalar |
| `IA_Jump` | Digital | Pressed (Started/Completed) | — |
| `IA_Sprint` | Digital | Down (hold) | — |
| `IA_Crouch` | Digital | Pressed (toggle in code) | — |
| `IA_Interact` | Digital | Pressed | — |
| `IA_Aim` | Digital | Down (hold) | — |
| `IA_Fire` | Digital | Down; Started/Completed used for auto-fire | — |
| `IA_Reload` | Digital | Pressed | — |
| `IA_WeaponSlot` | Axis1D | Pressed | Scalar: key 1→1, 2→2, 3→3, 4→4 (ActionValue = slot index) |

**Contexts**

| IMC | Priority | Mappings | Added when |
|---|---|---|---|
| `IMC_Default` | 0 | Move, Look, Jump, Sprint, Crouch, Interact | `BP_BaseCharacter` BeginPlay (local controller) |
| `IMC_Combat` | 1 | Aim, Fire, Reload, WeaponSlot | on possess / equip; removed when entering a vehicle |
| `IMC_Vehicle` | 2 (Consume Input ✔) | Throttle/Steer/Brake/Handbrake/Exit(IA_Interact) | on vehicle possess |

**Blueprint workflow – input binding (`BP_BaseCharacter`):**
1. `Event BeginPlay` → `Get Player Controller → Get Enhanced Input Local Player Subsystem → Add Mapping Context(IMC_Default, 0)` and `(IMC_Combat, 1)`.
2. `SetupPlayerInputComponent` → Cast to `EnhancedInputComponent`; bind:
   - `IA_Move Triggered` → `Get Control Rotation` → yaw-only `GetForward/RightVector` → `Add Movement Input` (Y,X of value).
   - `IA_Look Triggered` → `Add Controller Yaw/Pitch Input`.
   - `IA_Jump Started→Jump`, `Completed→Stop Jumping`.
   - `IA_Sprint Started→ SetWantsSprint(true)`, `Completed→false`.
   - `IA_Crouch Started→ if bIsCrouched UnCrouch else Crouch`.
   - `IA_Interact Started → TryInteract`.
   - `IA_Aim Started/Completed → CombatComponent.SetAiming`.
   - `IA_Fire Started → CombatComponent.StartFire`, `Completed → StopFire`.
   - `IA_Reload Started → CombatComponent.Reload`.
   - `IA_WeaponSlot Started → CombatComponent.EquipSlot(Round(ActionValue))`.

### 2.2 `BP_BaseCharacter` & Locomotion

`enum E_LocomotionState { Walking, Sprinting, Crouching, Swimming, InVehicle }` (Falling derived from `IsFalling`).

| CharacterMovement | Value (cm/s) |
|---|---|
| Max Walk Speed | 250 (walk) |
| Sprint (set on state) | 550 |
| Max Walk Speed Crouched | 150 |
| Max Swim Speed | 300 |
| Jump Z Velocity | 420 |
| Capsule | R 34, HalfHeight 90 (crouch 60) |
| Braking Deceleration | 2048 walking |
| Can Crouch / Walk off ledges | ✔ |

**Rules:** sprint blocked while crouched, aiming, or swimming; sprint ends at speed < 50 or no forward input; state recomputed in `UpdateLocomotionState()` called on movement-mode change (`OnMovementModeChanged`), crouch events, vehicle enter/exit.

**Vehicle/boat interaction (`BPI_Interactable::Interact(Instigator)`):**
1. `TryInteract` → `SphereTraceByChannel` (Visibility) from camera, 300 cm, r=30.
2. Hit actor `DoesImplementInterface(BPI_Interactable)` → `Interact`.
3. Vehicle `Interact`: (server) `Possess(Vehicle)`, character `Attach to Seat socket`, hide + disable collision, `Remove IMC_Default/IMC_Combat`, `Add IMC_Vehicle`, state=InVehicle, holster weapon.
4. Exit: `UnPossess`, `TryFindSafeLocation(DoorSocket)` via capsule sweep, re-enable collision, restore IMCs, `SetLocomotionState`.

### 2.3 Weapon Data

**`E_WeaponType`**: Unarmed, Knife, Pistol, Rifle, Sniper. **`E_FireMode`**: Single, Auto, Burst, Melee.

**`ST_WeaponData`** (Blueprint Struct used as DataTable row):

| Field | Type |
|---|---|
| WeaponName | Name |
| WeaponType | E_WeaponType |
| WeaponMesh | Soft Object Ref (Skeletal Mesh; Static Mesh variant `WeaponMeshStatic` optional) |
| FireMode | E_FireMode |
| BaseDamage | Float |
| FireRate | Float (seconds between attacks) |
| UseProjectile | Bool (false = hitscan) |
| RecoilPitch | Float (degrees) |
| AnimOverlayLayer | Class<AnimInstance> (implements `ALI_Combat`) |
| *(optional extensions)* | Range, SpreadDeg, MagSize, ReloadTime, BurstCount, SocketName (holster), ProjectileClass |

**`DT_Weapons` starting values**

| Row | Type | Mode | Damage | FireRate | Projectile | Recoil | Holster socket | Anim layer |
|---|---|---|---|---|---|---|---|---|
| Fists | Unarmed | Melee | 15 | 0.55 | ✘ | 0 | — | `ABP_Layers_Unarmed` |
| Knife | Knife | Melee | 40 | 0.70 | ✘ | 0 | (hidden when stowed) | `ABP_Layers_Knife` |
| Pistol | Pistol | Single | 25 | 0.25 | ✘ | 1.2 | `holster_thigh_r` | `ABP_Layers_Pistol` |
| AssaultRifle | Rifle | Auto | 20 | 0.10 (600 rpm) | ✘ | 0.6 (+yaw ±0.25) | `spine_rifle_socket` | `ABP_Layers_Rifle` |
| SniperRifle | Sniper | Single | 120 | 1.40 | ✔ | 4.0 | `spine_sniper_socket` | `ABP_Layers_Sniper` |

Slot map: **1 Pistol, 2 Rifle, 3 Sniper, 4 Knife**; pressing the active slot again holsters → **Fists** (default Unarmed).

**Mannequin sockets** (add on the shared UE5 Mannequin skeleton, so both Manny/Quinn share them):

| Socket | Parent bone | Use |
|---|---|---|
| `hand_r_socket` | `hand_r` | active weapon / knife / fists |
| `holster_thigh_r` | `thigh_r` | holstered pistol |
| `spine_rifle_socket` | `spine_03` | slung rifle |
| `spine_sniper_socket` | `spine_02` | slung sniper |

Starting offsets are art-dependent: tune in the Skeleton Editor with each weapon as preview asset (expected: hand socket rotation ≈ (0, −90, 0) region; rifle/sniper slung rotated ~ (0, 0, 90) back-down). Add `muzzle` to each gun mesh and `blade_base`/`blade_tip` to the knife.

### 2.4 `BP_CombatComponent` (ActorComponent on `BP_BaseCharacter`)

**Variables:** `WeaponTable (DT_Weapons)`, `SlotRows[4] (Name)`, `CurrentSlot (int)`, `CurrentData (ST_WeaponData)`, `CurrentWeaponActor (BP_WeaponBase)`, `bIsAiming`, `bWantsFire`, `LastAttackTime (float)`, `SpawnedWeapons (Map<Name, Actor>)`, `HitActors (Set<Actor>)`, `bMeleeTracing`, `PrevTraceTransform`.
**Dispatchers:** `OnWeaponChanged`, `OnAttack`, `OnHit`.

**`EquipSlot(Slot)` workflow**
1. If `Slot == CurrentSlot` → `Slot = 0 (Fists)`.
2. `Get Data Table Row(WeaponTable, SlotRows[Slot])` → `NewData`.
3. Old weapon: `AttachToComponent(Mesh, holster socket, KeepWorld/Snap)`; knife/fists hidden.
4. `Async Load` (or already loaded) `NewData.WeaponMesh` → spawn/enable `BP_WeaponBase` → `Attach To Component(hand_r_socket, SnapToTarget)`.
5. `Mesh → Unlink Anim Class Layers(old)`; `Link Anim Class Layers(NewData.AnimOverlayLayer)`.
6. Reset `LastAttackTime`, `bWantsFire`; broadcast `OnWeaponChanged`.

**`StartFire`** → `bWantsFire = true` → `TryAttack`; if FireMode == Auto/Melee-hold: `Set Timer by Event (FireRate, looping)` → `TryAttack`. `StopFire` → clear timer.

**`TryAttack`**
1. Guard: `Now − LastAttackTime ≥ FireRate` (Branch; anti-spam), not InVehicle, not reloading.
2. `LastAttackTime = Now`.
3. `FireMode == Melee` → play montage (`PlayMontage` on `UpperBody` slot) → notifies drive the trace (§2.6).
4. else `UseProjectile ? SpawnProjectile : FireHitscan`.
5. Apply recoil (§2.5), play fire montage/FX/audio; broadcast `OnAttack`.

Authority: all damage and spawns run on the server (`Server_TryAttack` RPC from the owning client; hitscan optionally validated); cosmetic FX multicast. Standalone/test build behaves identically.

### 2.5 Ballistics vs Hitscan

**Hitscan (Pistol, Rifle)**
1. `Camera Location`, `Camera Forward`; apply spread: `RandomUnitVectorInConeInRadians(Forward, ConeHalfAngle)`.
2. `LineTraceByChannel(Start=CamLoc, End=CamLoc + Dir×Range, Channel=Weapon, TraceComplex=false, Ignore=[Self, WeaponActor])` (Range: pistol 6000, rifle 20000).
3. Hit → `Apply Point Damage(HitActor, BaseDamage × HeadshotMult (×2 if bone == head), Dir, HitResult, InstigatorController, Owner, DT_Bullet)`; spawn impact FX by `PhysMaterial.SurfaceType`.
4. Optional correctness pass: second trace from `muzzle` to impact point to avoid shooting through near-camera geometry; draw tracer to the final hit.

**Projectile – `BP_SniperBullet`**

| Component / Setting | Value |
|---|---|
| Root | `SphereCollision` radius **2 cm**, profile `BulletProjectile`, Generate Overlap off, **Notify Rigid Body Collision ✔** |
| `ProjectileMovementComponent` | **Initial Speed 85000, Max Speed 85000**, **Gravity Scale 1.0**, Rotation Follows Velocity ✔, Should Bounce ✘, `bForceSubStepping ✔`, `MaxSimulationTimeStep 0.0083` |
| Lifespan | 3 s (≈ 2.5 km, the full map width) |
| Replication | Replicates ✔, Replicate Movement ✔ |
| Mesh/FX | tiny tracer ribbon (Niagara); no visible mesh required |

**Spawn workflow:** `SpawnActorDeferred(BP_SniperBullet, Transform(Muzzle loc, aimed at camera trace point), Owner=Character, Instigator=Character)` → `IgnoreActorWhenMoving(Owner)` → `FinishSpawning`. Aim point: camera trace (Weapon channel, 100000 cm) hit or end-of-trace; bullet direction = `(AimPoint − Muzzle).Normalize`.
**Impact:** `OnProjectileStop / OnComponentHit` → `Apply Point Damage` (Damage = `CurrentData.BaseDamage`, passed through `ExposeOnSpawn`), impact FX, `Destroy`.

**Ballistic sanity numbers (g = 980 cm/s²):**

| Range | Flight time | Drop |
|---|---|---|
| 100 m | 0.118 s | 6.8 cm |
| 300 m | 0.353 s | 61 cm |
| 500 m | 0.588 s | 169 cm |
| 1000 m | 1.176 s | 678 cm |

Optional zeroing: launch pitch θ ≈ g·R/(2v²) → 300 m ≈ 0.117°, 500 m ≈ 0.32°.

**Recoil:** `AddControllerPitchInput(−RecoilPitch)` (negative = up with the template's inverted mouse-Y) + `AddControllerYawInput(RandRange(−0.25,0.25)×RecoilPitch)`; recovery via Timeline interpolating back 60 % over 0.25 s after `StopFire`.

### 2.6 Melee Framework

**`ANS_MeleeTrace` (AnimNotifyState Blueprint)** – exposed: `TraceKind (Box/Sphere)`, `StartSocket`, `EndSocket`, `Radius/HalfExtent`, `DebugDraw`.
- `Received_NotifyBegin(MeshComp)` → `Owner → CombatComponent → BeginMelee()`: clear `HitActors`, cache `Prev = current socket transforms`, `bMeleeTracing = true`.
- `Received_NotifyTick(MeshComp, Anim, DeltaTime)` → `MeleeTraceTick()`:
  1. `Start/End` = current `StartSocket/EndSocket` world locations (knife: `blade_base`/`blade_tip`; fists: `hand_r_socket`, r=12).
  2. **Sweep from previous to current frame** (prevents tunneling at fast swings and low fps):
     - Knife: `BoxTraceMultiByChannel(PrevCenter → CurCenter, HalfSize=(BladeLen/2, 2, 2), Orientation=blade rot, Channel=Melee, Ignore=[Self])`
     - Fists: `SphereTraceMultiByChannel(PrevHand → CurHand, Radius=12, Channel=Melee)`.
  3. `ForEach Hit`: `if HitActors.AddUnique succeeded` → `Apply Point Damage(BaseDamage, DT_Melee)` → `OnHit`, impact FX, camera shake.
  4. `Prev = Cur`.
- `Received_NotifyEnd` → `bMeleeTracing = false`; clear set.

Notes: the set guarantees **one hit per actor per swing**; for server-side melee set the mesh's *Visibility Based Anim Tick Option = Always Tick Pose and Refresh Bones* (or drive the trace on a server timer using montage position). Fists reuse the same notify with the hand socket; add `hand_l` if bilateral combos wanted.

### 2.7 Animation Integration (Linked Anim Layers)

**`ALI_Combat` (Animation Layer Interface)** functions:
- `AL_Stance` (full body: Idle/Walk/Sprint/Crouch/Swim poses for the weapon class)
- `AL_UpperBody` (aim pose + aim offset + additive fire/reload overlay; Layered Blend Per Bone from `spine_01`)
- `AL_Melee` (optional swing set; montages play through `DefaultSlot.UpperBody`)

**`ABP_Base`** (assigned to the mannequin mesh): state machine **Locomotion** (Idle/Walk/Run/Sprint blendspace) → **Crouch** → **InAir** (Jump start/loop/land) → **Swim** → **Vehicle seated**; each state calls the `AL_*` layer (`Linked Anim Layer` nodes) so the main graph never changes when weapons change. Variables via *Property Access* + *Thread Safe Update*: `GroundSpeed`, `Direction`, `bIsCrouching`, `bIsFalling`, `bIsSwimming`, `bIsAiming`, `AimPitch/AimYaw`, `LocomotionState`.

**Runtime swap:** `Mesh → Link Anim Class Layers(CurrentData.AnimOverlayLayer)` on equip (unlink previous). Layers: `ABP_Layers_Unarmed/Knife/Pistol/Rifle/Sniper`, each implementing `ALI_Combat` — adding a weapon type = one new layer BP + one DT row, zero state-machine edits.

---

## PERFORMANCE & SCALABILITY BUDGET

| Budget | Target |
|---|---|
| Frame | 60 fps @ 1080p, High scalability, RTX 3060-class: ≤ 16.6 ms (GPU ≤ 15, Game ≤ 8) |
| Draw / Nanite | Nanite for all rocks/buildings/trees; foliage cull distance 25000 |
| Streaming | No hitch > 50 ms when driving at 25 m/s; loaded cells 16–21 |
| Memory | < 6 GB VRAM textures; landscape RVT 4096² virtual page budget |
| Debug commands | `stat unit`, `stat gpu`, `stat fps`, `profilegpu`, `wp.Runtime.ToggleDrawRuntimeHash2D`, `viewmode lumenscene`, `r.Nanite.Visualize Overview`, `ShowFlag.VisualizeVirtualShadowMap 1` |

---

## IMPLEMENTATION ROADMAP (priority order)

### Day 1 — Foundation & Terrain *(biggest risk reduction)*
1. Project config (§0), collision channels, folders, source control (Git LFS / Perforce).
2. New Open World level, World Partition + OFPA, runtime grid 12800/25600, 3 Data Layers.
3. Generate 2017² 16-bit heightmap (§1.1 mapping), import with exact component settings, actor at (-100800,-100800,56000), Z scale 250.
4. First streaming test: PIE with `wp.Runtime.ToggleDrawRuntimeHash2D`; fly across map.
**Exit:** map loads, 1024 components, streaming stable, ridges N/E, basin SW.

### Day 2 — Roads, Water, Material, Atmosphere
1. Road splines + `Validate_Grades.py` (≤ 10 %); raise/lower terrain.
2. WaterZone, Lake (Z=0), rivers from Z≈600, waterfall split + exclusion volume + Niagara.
3. `M_LandscapeAuto` (slope/height/snow) + RVT; grass output; basic PCG forest.
4. Lighting stack (§1.7); HLOD layers build (`Build HLODs`).
**Exit:** drivable valley, river→waterfall→lake, correct material zoning, Lumen+VSM lit scene, HLOD distance view.

### Day 3 — Character, Input, Combat Core
1. IA/IMC assets; `BP_BaseCharacter` locomotion (walk/sprint/crouch/swim).
2. `ABP_Base` + `ALI_Combat` + layers for Unarmed/Pistol/Rifle; sockets on skeleton.
3. `ST_WeaponData`, enums, `DT_Weapons`, `BP_WeaponBase`, `BP_CombatComponent` (equip, timers, guards).
4. Hitscan pistol/rifle + damage + `BP_TestDummy` (health, hit log).
**Exit:** slot 1/2/4 switching, auto-fire at 10 shots/s, damage numbers on dummies, no state-machine edits to add a weapon.

### Day 4 — Ballistics, Melee, Vehicles, Test Harness, Polish
1. `BP_SniperBullet` (85000 cm/s, gravity 1.0) + drop verification at 300/500 m.
2. `ANS_MeleeTrace` with sweep + once-per-swing set for Fists and Knife.
3. `BPI_Interactable`, `BP_VehicleSpawner` (Chaos car/truck, boat) on `DL_Spawners`; `IMC_Vehicle` swap.
4. Test arena: dummies at 25/100/300/500 m, loot points; profiling pass against the budget; bug bash.
**Exit:** all acceptance tests below pass; 15-minute soak with no leaks/hitches.

**Slip plan (cut in this order):** HLOD_Vegetation → boat buoyancy → burst fire → headshot multiplier → snow noise breakup. Never cut: streaming, grade validation, data-driven weapons, ANS hit-set.

### Acceptance tests
| ID | Test | Pass |
|---|---|---|
| A1 | Landscape stats | 1024 components, 2017², Z range −80…1200 m |
| A2 | Road grade validator | 0 segments > 10 % |
| A3 | Truck climb | 10 % grade, 18 t, ≥ 15 km/h |
| A4 | Streaming | 25 m/s drive, no hitch > 50 ms, ≤ 21 cells |
| A5 | Material | Cliff triplanar > 45°; snow starts ≥ 850 m, full at 1000 m |
| A6 | Water | River Z monotonic, enters lake at 0; swimming works |
| A7 | Hitscan | Pistol ≥ 3.5 shots/s cap; rifle 10/s; one damage event per shot |
| A8 | Sniper | Drop at 500 m ≈ 169 cm (±5 %) |
| A9 | Melee | Exactly one damage event per target per swing at 15/30/60 fps |
| A10 | Anim swap | Equip each weapon with unchanged `ABP_Base`; no pops > 0.2 s |

### Risk Register
| Risk | Mitigation |
|---|---|
| Water capture at 600 m vs Zone Z=0 | Plan B second WaterZone (§1.5) |
| Landscape material sampler/instruction limits | RVT + packed textures + dynamic branches |
| Melee misses at low fps | Prev→Cur sweep (not point trace) |
| Server melee notifies don't fire | Always-tick-pose option or timer-driven trace |
| Version drift (⚠ items) | Re-verify names on target engine version on Day 1 |
