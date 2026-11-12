# Questions for the first playtest build

Answer in any form (a number and a few words is enough). Until then the **default** in each row is what the
code assumes.

## Blocking — I cannot do these from here

| # | Question | Default assumed |
|---|---|---|
| 1 | **Which engine version and platform** do you build with (5.5 / 5.6 / 5.7, Windows)? | UE 5.5, Windows, DX12 |
| 2 | **Can you compile the project and send me the build output?** Nothing here has been compiled; the first error list is the single most useful thing you can give me. | — |
| 3 | After it builds: **does Play work on the preview terrain**, and what feels wrong (look speed, movement speeds, camera distance, cart handling)? | — |

## Scope of the test version

| # | Question | Default assumed |
|---|---|---|
| 4 | **Single-player only**, or must the test version already run in multiplayer? | Single-player; no server RPCs |
| 5 | **Camera:** third-person over the shoulder, first-person, or switchable? | Third-person, right shoulder |
| 6 | **What must a tester be able to do** for the playtest to count as passed? | The ten acceptance tests in the design document |
| 7 | **Opponents:** static targets only, or simple AI that moves / shoots back? | Static targets |
| 8 | **Player rules:** can the player die and respawn? Limited ammo and pickups, or infinite reserve? | No death (health refills after 3 s), infinite reserve ammo |
| 9 | **Target hardware and frame rate** for the playtest? | RTX 3060 class, 60 fps at 1080p |

## Content

| # | Question | Default assumed |
|---|---|---|
| 10 | **Mannequin:** is the Third Person content pack (Manny / Quinn) acceptable, or do you have your own character? | Block mannequin, replaced automatically by Manny once the pack is added |
| 11 | **Weapon meshes and animations:** do you have any (Fab / Lyra / own), or should placeholders stay for now? | Placeholder boxes, procedural arm poses |
| 12 | **Vehicles:** which real ones should replace the test carts (Vehicle template car, off-road, a truck, a boat), and are they needed for the first playtest? | Test carts only for now |
| 13 | **Terrain:** is the generated layout (see `CentralEuropeTest/SourceArt/Terrain/terrain_preview.png`) acceptable, or what should change (lake size, valley size, ridge shape, more roads)? | Keep as generated |

## Project housekeeping

| # | Question | Default assumed |
|---|---|---|
| 14 | **C++ parent classes** with Blueprint children on top — acceptable, or do you need Blueprint-only? | C++ parents |
| 15 | **Repository:** this is your profile repository. Keep the game here, or move it to its own repository (with Git LFS for binary assets)? | Stay here, no LFS |
