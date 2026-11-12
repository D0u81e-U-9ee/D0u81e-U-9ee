#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CEAmbienceActor.generated.h"

class UAudioComponent;

/**
 * Ambient sound for the test map, mixed from the listener's position every frame:
 *  - wind bed, louder with altitude;
 *  - birdsong in the valley, fading out above the tree line;
 *  - lake lapping near the south / west shoreline;
 *  - river and waterfall emitters read from SourceArt/Terrain/terrain_layout.json, started only
 *    while the player is within earshot.
 * Spawned by ACETestGameMode. Shoreline maths matches Tools/generate_terrain.py.
 */
UCLASS()
class CENTRALEUROPETEST_API ACEAmbienceActor : public AActor
{
	GENERATED_BODY()

public:
	ACEAmbienceActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience") float MasterVolume = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience") float WindVolume = 0.55f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience") float BirdsVolume = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience") float LakeVolume = 0.6f;

	/** Lake shoreline of the generated map: water where softmax(-X, -Y) exceeds this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience") float ShorelineDistance = 60800.f;

	/** Layout file relative to the project directory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	FString LayoutPath = TEXT("SourceArt/Terrain/terrain_layout.json");

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FEmitter
	{
		FVector Location = FVector::ZeroVector;
		FName Key;
		float InnerRadius = 0.f;
		float Falloff = 0.f;
		float Volume = 1.f;
		TWeakObjectPtr<UAudioComponent> Component;
	};

	void LoadEmitters();
	static void SetLoopVolume(UAudioComponent* Component, float Volume);

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> WindLoop;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> BirdsLoop;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> LakeLoop;

	TArray<FEmitter> Emitters;
};
