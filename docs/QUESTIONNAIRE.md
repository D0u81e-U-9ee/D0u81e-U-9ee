# Questionnaire

Answer with the question number and a letter or a few words (for example `7: B`, `12: cars and motorcycles first`).
Where a **default** is given, I will use it if you skip the question. Questions marked ★ block the next
milestone; the rest can wait.

## A. Two open bugs (please answer first)

| # | Question |
|---|---|
| 1 ★ | **Weapon keys.** When you press 1–4 or scroll the mouse wheel, do you see (A) a yellow "Equipped …" message and a weapon in the hand, (B) a red message (which text?), or (C) nothing at all? |
| 2 ★ | **Mannequin.** In the summary window of `build_world.py`, what did the last line say for "Mannequin": found or MISSING? |
| 3 ★ | In the Content Browser, is there a folder `Content/Characters/Mannequins`? If yes, what is inside `Meshes` (file names)? If no, is there a `StarterContent` folder? |

## B. The game itself

| # | Question | Default |
|---|---|---|
| 4 ★ | **Who builds this?** (A) you alone with me, (B) a small team, (C) you plan to hire or recruit later. | A |
| 5 ★ | **What is the goal of the project?** (A) learning / hobby, (B) a playable demo to show others, (C) a commercial release. | B |
| 6 ★ | **Single-player or multiplayer?** (A) single-player only, (B) single-player now, co-op later, (C) online multiplayer from the start. Multiplayer roughly doubles the work on every system and must be decided now. | A |
| 7 | **Tone and setting:** present day? Which part of Europe (alpine village, coast, a city)? Realistic or slightly exaggerated like GTA? | Present day, alpine region, realistic |
| 8 | **Is there a story with characters and dialogue**, or a sandbox with missions that stand alone? | Sandbox with stand-alone missions |
| 9 | **Violence and age rating:** how graphic may combat be (blood, civilians, police)? | Moderate, no civilians harmed for now |
| 10 | **Target hardware:** your own PC's graphics card and RAM? Should it also run on consoles or Steam Deck later? | PC only |
| 11 | **Language of in-game text:** German, English, or both? | English, German later |

## C. Player and camera

| # | Question | Default |
|---|---|---|
| 12 ★ | **First / third person:** (A) free switch on a key everywhere, (B) third person on foot and first person only when aiming, (C) free switch on foot and in vehicles. | C |
| 13 | **Weapon wheel:** hold a key and pick with the mouse (GTA style)? How many slots, and should non-weapon items (medkit, tools, phone, binoculars) be on the same wheel or a second one? | Hold Tab; 8 slots; items on a second wheel |
| 14 | **Inventory:** (A) wheel only, fixed slots, (B) wheel plus a simple list of carried items, (C) full grid inventory with weight. | B |
| 15 | **Health and death:** regenerating health or medkits? What happens on death (respawn at hospital, lose money, reload save)? | Medkits; respawn at nearest safe point, small money loss |
| 16 | **Movement extras wanted:** climbing / vaulting, prone, cover system, stealth? Rank the ones you want. | Vaulting only |

## D. Vehicles, boat, flight, parachute

| # | Question | Default |
|---|---|---|
| 17 ★ | **Order of vehicle types.** Rank: car, motorcycle, truck, boat, aircraft. | Car, boat, motorcycle, truck, aircraft |
| 18 ★ | **Flight mechanic:** what do you mean exactly? (A) aeroplane, (B) helicopter, (C) paraglider / wingsuit, (D) several of these. Arcade handling or simulation? | C first (pairs with the parachute), arcade |
| 19 | **Parachute:** opened manually after jumping from height or aircraft, steerable, with landing damage if opened too late? | Yes to all |
| 20 | **Driving feel:** arcade (GTA) or closer to simulation? Damage model: visual only, or affecting handling? | Arcade; visual damage plus engine failure |
| 21 | **Vehicle assets:** will you (A) use free / bought models from Fab, (B) model them yourself, (C) want placeholders until later? I cannot create 3D models. | A |
| 22 | **Traffic and parked cars** in the world: wanted for the first real version or later? | Later |

## E. Workshops, economy, missions

| # | Question | Default |
|---|---|---|
| 23 ★ | **Vehicle workshop depth:** (A) paint and wheels only, (B) plus performance parts (engine, brakes, suspension, tyres) that change handling, (C) plus visual body parts (bumpers, spoilers) that need matching 3D models per vehicle. | B |
| 24 | **Weapon workshop depth:** (A) attachments (sight, suppressor, magazine, grip) with stat changes, (B) plus skins / paint, (C) plus crafting from parts. | A |
| 25 ★ | **Money:** where does it come from (missions, selling vehicles, jobs, loot) and what is it spent on besides the workshops (weapons, ammo, vehicles, property, clothes)? | Missions and jobs; spent on workshops, weapons, ammo, vehicles |
| 26 | **Mission types wanted first** (pick three): delivery / transport, race, chase, escort, break-in, shoot-out, taxi or tow jobs, photo / scouting. | Delivery, race, shoot-out |
| 27 | **Mission giver style:** phone calls, characters standing in the world, a job board? | Markers in the world plus a job board |
| 28 | **Police / wanted system** like GTA? | Later |
| 29 | **Saving:** autosave plus manual saves at safe houses, or save anywhere? | Autosave plus save at safe house |

## F. World and map

| # | Question | Default |
|---|---|---|
| 30 ★ | **Map ambition:** (A) keep the 2 × 2 km valley and fill it properly, (B) grow to about 4 × 4 km with one town, (C) 8 × 8 km or more with several settlements. Bigger maps multiply the art work far more than the code work. | A now, B later |
| 31 | **Real place or invented?** Should the terrain come from real elevation data of a specific region, or stay fictional? | Fictional, inspired by the Alps |
| 32 | **Settlements:** what must exist in the first real version (a village, petrol station, workshop garage, harbour, airfield)? | Workshop garage, harbour, a few houses |
| 33 | **Day / night cycle and weather** (rain, fog, snow): wanted, and do they affect gameplay (grip, visibility)? | Day / night yes; weather visual only |
| 34 | **Environment assets:** are you willing to download free Megascans / Fab packs (trees, rocks, buildings) through the Epic launcher and tell me their names? | Yes |
| 35 | **NPCs:** pedestrians and animals in the world, or only mission characters and enemies at first? | Mission characters and enemies only |

## G. How we work

| # | Question | Default |
|---|---|---|
| 36 ★ | **C++ and Blueprints:** do you accept the split proposed in `ROADMAP.md` (I write systems in C++; you create thin Blueprint children in the editor to assign meshes, sounds and tuning values)? | Yes |
| 37 ★ | **How much editor work can you take on?** (A) only pressing buttons and running my scripts, (B) following click-by-click guides to place and configure things, (C) you want to learn to build content and Blueprints yourself. | B |
| 38 | **Time per week** you expect to spend testing and doing editor work? This sets how big each step should be. | — |
| 39 | **Repository:** move the game out of your profile repository into its own one, with Git LFS for large files? | Yes, before assets are added |
| 40 | **Which is more important right now:** (A) it looks impressive, (B) the gameplay systems work, (C) both in small alternating steps? | C |
