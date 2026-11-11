#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCETest, Log, All);

// Custom collision channels. Must match Config/DefaultEngine.ini ([/Script/Engine.CollisionProfile]).
#define ECC_Weapon     ECC_GameTraceChannel1   // trace channel: hitscan
#define ECC_Melee      ECC_GameTraceChannel2   // trace channel: melee sweeps
#define ECC_Projectile ECC_GameTraceChannel3   // object channel: ballistic bullets
