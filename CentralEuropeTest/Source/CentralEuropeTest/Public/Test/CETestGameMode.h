#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "CETestGameMode.generated.h"

/**
 * BP_TestGameMode parent. Spawns ACEBaseCharacter with ACEPlayerController and, optionally,
 * a firing range of test dummies in front of the player so combat can be tested in any level.
 */
UCLASS()
class CENTRALEUROPETEST_API ACETestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACETestGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Range")
	bool bSpawnTestRange = true;

	/** Distances (cm) in front of the player start. Far targets are skipped when their ground is not streamed in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Range")
	TArray<float> DummyDistances;

	/** Sideways offset (cm, + = right) per target so nearer targets do not hide farther ones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Range")
	TArray<float> DummyLateralOffsets;

	/**
	 * When the level has no Landscape, build the generated heightmap as a runtime preview terrain
	 * and start the player on the valley loop road. Lets the map be playtested before the manual
	 * Landscape import.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview Terrain")
	bool bAutoPreviewTerrain = true;

	/**
	 * Player start on the preview terrain (world cm): a point on the valley loop road, facing
	 * north-east up the foothills. Chosen offline so all five range targets are in plain sight.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview Terrain")
	FVector2D PreviewStartXY = FVector2D(-1322.0, 8367.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview Terrain")
	float PreviewStartYaw = 30.f;

	/** Wind, birds, lake and river ambience (ACEAmbienceActor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	bool bSpawnAmbience = true;

	/** Spawn the stand-in cart (and a truck-profile one) next to the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Range")
	bool bSpawnTestCarts = true;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnTestRange();
	bool FindGround(const FVector& At, const AActor* Ignore, FVector& OutPoint) const;
	bool HasLineOfSight(const FVector& Eye, const FVector& GroundPoint, const AActor* Ignore) const;

	UPROPERTY(Transient)
	TObjectPtr<class ACETerrainPreviewActor> PreviewTerrain;

	FTimerHandle RangeTimer;
};
