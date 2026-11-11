#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "TestGameMode.generated.h"

/**
 * BP_TestGameMode parent. Spawns ABaseCharacter with APrototypePlayerController and, optionally,
 * a firing range of test dummies in front of the player so combat can be tested in any level.
 */
UCLASS()
class CENTRALEUROPETEST_API ATestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATestGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Range")
	bool bSpawnTestRange = true;

	/** Distances (cm) in front of the player start. Far targets are skipped when their ground is not streamed in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Range")
	TArray<float> DummyDistances;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnTestRange();

	FTimerHandle RangeTimer;
};
