# Roadmap: from the test valley to an open-world game

This is the plan to agree on before more code is written. It covers the goal, an honest look at scope,
the C++ / Blueprint decision, the project structure, the milestones in order, and how each step is run.
The companion file `QUESTIONNAIRE.md` holds the decisions I need from you; several milestones below
depend on those answers and are marked.

## 1. The goal, and what it realistically means

**Goal:** an open-world game on a European map, in the spirit of GTA: on-foot and vehicle gameplay,
a vehicle workshop for tuning cars, motorcycles and trucks, a weapon workshop, missions and money.

**Scope reality.** GTA-scale games are built by very large teams over many years, and most of that
effort is content: models, animation, buildings, voices, missions. Code is the smaller part. For a
one-person project working with me, the realistic target is a **vertical slice**: one small map where
every system exists once and works properly. Concretely:

- one valley with a garage, a harbour and a handful of buildings;
- a few vehicles of each kind rather than hundreds;
- three or four weapons with real customisation;
- five to ten missions built on a reusable mission framework.

Everything is then built so that adding more content is cheap. The roadmap below is that slice. Growing
the map and the content afterwards is mostly art and level work that you (or collaborators) do in the editor.

**What I can and cannot contribute.**

| I can | I cannot |
|---|---|
| Write C++ systems, editor Python scripts, config, data tables, documentation | Create or edit binary assets: Blueprints, 3D models, animations, textures, maps, UI widgets |
| Generate data: heightmaps, masks, placeholder sounds | Run the editor, see the screen, or hear the game |
| Fix build and script errors from the logs you send | Judge look and feel; I depend on your description |

This is why every step ends with you building, testing and reporting back.

## 2. C++ or Blueprints: recommendation

**Use both, with a fixed rule for which goes where.**

| Layer | Where | Who | Examples |
|---|---|---|---|
| **Systems and rules** | C++ | me | inventory, weapon wheel logic, economy, save game, mission framework, vehicle tuning maths, camera modes, parachute and flight physics |
| **Content and tuning** | Data assets and data tables | you, in the editor (I supply the structures and starting values) | weapon stats, vehicle parts and prices, mission definitions, item lists |
| **Assembly** | Thin Blueprint children of the C++ classes | you, following my guides | `BP_Player` (assign Manny, animations), `BP_SportsCar` (assign mesh, wheels, sounds), `BP_Garage` |
| **Presentation** | UMG widgets, Animation Blueprints, materials, Niagara | you, with my guides; I provide C++ base classes that hold the logic | HUD, weapon wheel visuals, workshop menu, character animation graph |
| **Level scripting** | Level placement plus mission data; no logic in the Level Blueprint | you | placing mission markers, garages, spawners |

**Rules that keep it clean**

1. No gameplay logic in Blueprints beyond wiring events to C++ functions. If a Blueprint graph grows past one screen, the logic moves into C++.
2. Blueprints never reference each other directly; they talk through C++ base classes and interfaces.
3. Every tunable number lives in a data asset or table, never hard-coded in a graph.
4. One C++ class per Blueprint parent, named the same (`ACEVehicle` → `BP_Vehicle_*`).

**Why not C++ only:** meshes, animations and UI layouts have to be assigned somewhere, and doing that in
code is brittle. **Why not Blueprint only:** I cannot write Blueprints, large Blueprint projects are hard
to review and merge, and systems such as saving and vehicle physics are far easier to keep correct in code.

## 3. Project structure

**Source code**: one runtime module, organised by feature, plus a small editor module. Split into
separate modules only when a feature grows large.

```
Source/CentralEuropeTest/
  Core/          game mode, game instance, save game, shared types, interfaces
  Player/        character, cameras (first / third person), input, interaction
  Inventory/     items, inventory component, weapon and item wheel logic
  Combat/        weapons, projectiles, melee, health, damage
  Vehicles/      base vehicle, car, motorcycle, truck, boat, aircraft, seats, tuning
  Air/           parachute, glider / flight
  Economy/       wallet, prices, shops, workshops (vehicle and weapon)
  Missions/      mission framework, objectives, rewards, markers
  AI/            enemies, later pedestrians and traffic
  World/         ambience, time of day, weather, spawners, water helpers
  UI/            HUD and widget base classes
  Audio/         sound playback
  Dev/           stand-ins and test tools (preview terrain, block mannequin, test carts, dummies)
Source/CentralEuropeTestEditor/   editor-only tools (landscape import, validation, build scripts' helpers)
```

**Content**

```
Content/CE/
  Core/  Player/  Weapons/  Vehicles/  Items/  Missions/  UI/  Audio/  World/  Materials/
  Data/          data tables and data assets
  Maps/          L_CentralEuropeTest and sub-levels
Content/ThirdParty/   everything downloaded (Starter Content, Fab packs), never edited in place
```

**Conventions**

- Prefix `CE` on all C++ types (already done). Asset prefixes: `BP_`, `DA_`, `DT_`, `WBP_`, `M_`, `MI_`, `SM_`, `SK_`, `ABP_`, `S_`.
- Stand-ins live under `Dev/` and are removed as their real replacement arrives, so the two never mix.
- `docs/` holds this roadmap, the design document, and one short design note per milestone.
- One branch and one pull request per milestone. `main` always builds and runs.
- Large binary files go through Git LFS once real assets are added (questionnaire item 39).

Moving the existing files into this layout is the first task of Milestone 0.

## 4. Milestones

Sizes are relative effort, not dates: **S** is roughly one or two build-and-test rounds between us,
**M** a handful, **L** many, **XL** a long stretch dominated by content work on your side.

| # | Milestone | What you can do at the end | Depends on | Size |
|---|---|---|---|---|
| **0** | **Foundation** | Same game as now, but: Manny works, weapon keys confirmed, code in the structure above, stand-ins separated into `Dev/`, own repository | answers 1–3, 36, 39 | S |
| **1** | **Player core** | Switch first / third person; open a weapon wheel and an item wheel; pick up, carry and use items; health and medkits; basic HUD | answers 12–15 | M |
| **2** | **Save and economy** | Earn and spend money; state persists between sessions (position, money, inventory, owned vehicles) | answers 25, 29 | M |
| **3** | **Real vehicles I: car** | Drive a real Chaos car with proper enter / exit, first / third person cameras, damage and sound; park and own it | answers 17, 20, 21 | M |
| **4** | **Vehicle workshop** | Drive into the garage; change paint, wheels and performance parts; pay for them; changes affect handling and are saved | answers 23; milestones 2, 3 | L |
| **5** | **Weapons II and weapon workshop** | Proper weapon models and animation; attachments that change stats; buy weapons and ammo | answers 24; milestones 1, 2 | L |
| **6** | **Missions** | Accept a mission from a marker or job board; follow objectives with on-screen guidance; get paid; three mission types working | answers 26, 27; milestones 2, 3 | L |
| **7** | **Water and boat** | Real lake and rivers (Water plugin); a drivable boat with buoyancy; harbour | answer 17 | M |
| **8** | **Parachute and flight** | Jump from the cliffs, open a parachute, steer and land; then the chosen flight type | answers 18, 19 | M |
| **9** | **Motorcycle and truck** | Both drivable and tunable in the workshop; truck can carry or tow for missions | milestones 3, 4 | L |
| **10** | **Living world I** | Enemies that fight back in missions; simple wildlife or bystanders if wanted | answers 9, 35 | L |
| **11** | **World build-out** | Forests, grass and rocks; garage, harbour and houses placed; day / night; weather; World Partition, data layers, HLOD | answers 30–34 | XL |
| **12** | **Living world II** | Traffic, parked cars, optional police / wanted system | answers 22, 28 | XL |
| **13** | **Polish and packaging** | Menus, settings, key remapping, performance pass, a packaged build others can play | all | L |
| later | **Map expansion** | Larger or additional regions | answer 30 | XL |

**Why this order**

- Milestones 1 and 2 come first because everything later plugs into them: the workshops need money and saving, missions need rewards and inventory, vehicles need ownership.
- The car comes before the workshop, and one car before many, so the tuning system is designed against something real.
- Missions arrive once there is something to do in them (driving, shooting, money).
- Visual world-building (11) runs **in parallel from the start** in small pieces, because it is mostly your editor work with my scripts and does not block code. After each code milestone I'll suggest one look-and-feel step, so the game improves visibly throughout rather than only at the end.
- Traffic and police are late because they are the most expensive systems and nothing else depends on them.

**Your newly requested features, placed**

| Request | Milestone |
|---|---|
| Mannequin instead of cubes | 0 |
| Weapon wheel, carrying other items | 1 |
| First / third person switch | 1 (on foot), 3 (in vehicles) |
| Financial system | 2 |
| Vehicle workshop: cars, then motorcycles and trucks | 4, extended in 9 |
| Weapon workshop | 5 |
| Missions | 6 |
| Boat | 7 |
| Parachute and flight | 8 |

## 5. How each milestone runs (step by step)

1. **Decide.** You answer the questionnaire items the milestone depends on.
2. **Design note.** I write a one-page note in `docs/design/`: what the feature does, the classes and data involved, what you will need to do in the editor, and the test checklist. You confirm or correct it before I write code.
3. **Implement.** I write the C++, data and scripts on a milestone branch and push.
4. **Build.** You download the branch and rebuild. If it fails, you send me `Log.txt`; I fix and push again.
5. **Editor steps.** You run my script or follow the click-by-click guide (create a Blueprint child, assign a mesh, place an actor).
6. **Test.** You play through the checklist and report: what works, what does not, what feels wrong.
7. **Fix and tune.** We repeat 3–6 until the checklist passes.
8. **Merge.** The pull request is merged into `main`, the stand-ins the feature replaces are removed, and the roadmap is updated.

**What makes this go faster**

- Sending the exact error text or log rather than a description.
- A screenshot or short description of what you see when something looks wrong, since I cannot see the game.
- Deciding questions marked ★ early; changing them later (multiplayer above all) means rework.
- Getting real assets (vehicles, weapons, trees, buildings) from Fab and telling me their names, so I can wire them in.

## 6. Immediate next steps

1. You answer questionnaire items 1–3 (the two open bugs) and the ★ items.
2. I do Milestone 0: fix the mannequin and the weapon keys if still broken, restructure the code, and write the design note for Milestone 1.
3. You confirm the Milestone 1 design note, and we start.
