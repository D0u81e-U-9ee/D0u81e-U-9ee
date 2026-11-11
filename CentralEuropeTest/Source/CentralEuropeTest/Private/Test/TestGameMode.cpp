#include "Test/TestGameMode.h"

#include "CentralEuropeTest.h"
#include "Character/BaseCharacter.h"
#include "Character/PrototypePlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Test/TestDummy.h"
#include "TimerManager.h"

ATestGameMode::ATestGameMode()
{
	DefaultPawnClass = ABaseCharacter::StaticClass();
	PlayerControllerClass = APrototypePlayerController::StaticClass();
	DummyDistances = { 300.f, 2500.f, 10000.f, 30000.f, 50000.f };   // melee, 25 m, 100 m, 300 m, 500 m
}

void ATestGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (bSpawnTestRange)
	{
		// Short delay so the pawn has landed and nearby cells have streamed in.
		GetWorldTimerManager().SetTimer(RangeTimer, this, &ATestGameMode::SpawnTestRange, 1.f, false);
	}
}

void ATestGameMode::SpawnTestRange()
{
	UWorld* World = GetWorld();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!World || !Player)
	{
		return;
	}

	const FVector Origin = Player->GetActorLocation();
	const FRotator Yaw(0.f, Player->GetActorRotation().Yaw, 0.f);
	const FVector Forward = Yaw.Vector();
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

	int32 Index = 0;
	for (const float Distance : DummyDistances)
	{
		// Slight sideways stagger so near targets do not hide far ones.
		const FVector Target = Origin + Forward * Distance + Right * (Index * 120.f - 240.f);
		++Index;

		FHitResult Ground;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TestRangeGround), false, Player);
		const bool bGround = World->LineTraceSingleByChannel(Ground, Target + FVector(0.f, 0.f, 150000.f),
			Target - FVector(0.f, 0.f, 150000.f), ECC_WorldStatic, Params);
		if (!bGround)
		{
			UE_LOG(LogCETest, Warning, TEXT("Test range: no ground at %.0f m (cell not loaded?) - dummy skipped"), Distance / 100.f);
			continue;
		}

		// Cylinder pivot is at its centre: 180 cm tall body -> raise by 90 cm.
		const FVector Location = Ground.ImpactPoint + FVector(0.f, 0.f, 90.f);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<ATestDummy>(ATestDummy::StaticClass(), Location, Yaw, SpawnParams);
	}
}
