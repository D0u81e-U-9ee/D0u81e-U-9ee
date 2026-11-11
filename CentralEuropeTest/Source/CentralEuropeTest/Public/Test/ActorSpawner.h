#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "ActorSpawner.generated.h"

/**
 * Generic spawner for the DL_Spawners data layer: vehicles (set SpawnClass to a Chaos vehicle BP
 * that carries a UVehicleSeatComponent), test dummies or loot points. Snaps the spawn to the ground.
 */
UCLASS(Blueprintable)
class CENTRALEUROPETEST_API AActorSpawner : public AActor
{
	GENERATED_BODY()

public:
	AActorSpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AActor> SpawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bSpawnOnBeginPlay = true;

	/** Height above the ground hit to spawn at (vehicles need clearance for their suspension). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float GroundClearance = 50.f;

	/** Respawn when the spawned actor is destroyed. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float RespawnDelay = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	AActor* SpawnNow();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleSpawnedDestroyed(AActor* DestroyedActor);

private:
	void RespawnTimerFired();

	UPROPERTY(Transient)
	TObjectPtr<AActor> Spawned;

	FTimerHandle RespawnTimer;
};
