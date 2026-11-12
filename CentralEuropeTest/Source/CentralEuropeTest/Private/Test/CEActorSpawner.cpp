#include "Test/CEActorSpawner.h"

#include "CentralEuropeTest.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACEActorSpawner::ACEActorSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ACEActorSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (bSpawnOnBeginPlay && HasAuthority())
	{
		SpawnNow();
	}
}

AActor* ACEActorSpawner::SpawnNow()
{
	UWorld* World = GetWorld();
	if (!World || !SpawnClass || Spawned)
	{
		return Spawned;
	}

	FVector Location = GetActorLocation();
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpawnerGround), false, this);
	if (World->LineTraceSingleByChannel(Ground, Location + FVector(0.f, 0.f, 500.f), Location - FVector(0.f, 0.f, 5000.f), ECC_WorldStatic, Params))
	{
		Location = Ground.ImpactPoint;
	}
	Location.Z += GroundClearance;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Spawned = World->SpawnActor<AActor>(SpawnClass, Location, FRotator(0.f, GetActorRotation().Yaw, 0.f), SpawnParams);
	if (Spawned)
	{
		Spawned->OnDestroyed.AddDynamic(this, &ACEActorSpawner::HandleSpawnedDestroyed);
	}
	else
	{
		UE_LOG(LogCETest, Warning, TEXT("%s failed to spawn %s"), *GetName(), *GetNameSafe(SpawnClass));
	}
	return Spawned;
}

void ACEActorSpawner::HandleSpawnedDestroyed(AActor* DestroyedActor)
{
	Spawned = nullptr;
	if (RespawnDelay > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &ACEActorSpawner::RespawnTimerFired, RespawnDelay, false);
	}
}

void ACEActorSpawner::RespawnTimerFired()
{
	SpawnNow();
}
