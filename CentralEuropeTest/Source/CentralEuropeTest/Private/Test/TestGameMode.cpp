#include "Test/TestGameMode.h"

#include "CentralEuropeTest.h"
#include "Character/BaseCharacter.h"
#include "Character/PrototypePlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Test/TerrainPreviewActor.h"
#include "Test/TestCart.h"
#include "Test/TestDummy.h"
#include "TimerManager.h"

ATestGameMode::ATestGameMode()
{
	DefaultPawnClass = ABaseCharacter::StaticClass();
	PlayerControllerClass = APrototypePlayerController::StaticClass();
	DummyDistances = { 300.f, 2500.f, 10000.f, 30000.f, 50000.f };   // melee, 25 m, 100 m, 300 m, 500 m
	DummyLateralOffsets = { -150.f, -600.f, 800.f, -1000.f, 0.f };
}

void ATestGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (!bAutoPreviewTerrain || !GetWorld())
	{
		return;
	}
	// A real Landscape (or a hand-placed preview) wins.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->IsA<ATerrainPreviewActor>())
		{
			PreviewTerrain = Cast<ATerrainPreviewActor>(*It);
			PreviewTerrain->Build();
			return;
		}
		if (It->GetClass()->GetName().StartsWith(TEXT("Landscape")))
		{
			return;
		}
	}

	ATerrainPreviewActor* Terrain = GetWorld()->SpawnActor<ATerrainPreviewActor>();
	if (Terrain && Terrain->Build())
	{
		PreviewTerrain = Terrain;
	}
	else if (Terrain)
	{
		Terrain->Destroy();   // heightmap missing: fall back to whatever the level provides
	}
}

APawn* ATestGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	if (PreviewTerrain && PreviewTerrain->IsBuilt())
	{
		const float GroundZ = PreviewTerrain->GetHeightAt(PreviewStartXY.X, PreviewStartXY.Y);
		const FTransform OnTerrain(FRotator(0.f, PreviewStartYaw, 0.f), FVector(PreviewStartXY.X, PreviewStartXY.Y, GroundZ + 130.f));
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, OnTerrain);
	}
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

void ATestGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	// On the preview terrain the level's PlayerStart is ignored, so its facing must be too.
	const bool bOnPreview = PreviewTerrain && PreviewTerrain->IsBuilt();
	Super::FinishRestartPlayer(NewPlayer, bOnPreview ? FRotator(0.f, PreviewStartYaw, 0.f) : StartRotation);
}

bool ATestGameMode::FindGround(const FVector& At, const AActor* Ignore, FVector& OutPoint) const
{
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TestRangeGround), false, Ignore);
	if (GetWorld()->LineTraceSingleByChannel(Ground, At + FVector(0.f, 0.f, 150000.f), At - FVector(0.f, 0.f, 150000.f), ECC_WorldStatic, Params))
	{
		OutPoint = Ground.ImpactPoint;
		return true;
	}
	return false;
}

bool ATestGameMode::HasLineOfSight(const FVector& Eye, const FVector& GroundPoint, const AActor* Ignore) const
{
	// Chest and head of a 180 cm target must both be visible.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TestRangeSight), false, Ignore);
	FHitResult Hit;
	for (const float Height : { 90.f, 158.f })
	{
		if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, GroundPoint + FVector(0.f, 0.f, Height), ECC_WorldStatic, Params))
		{
			return false;
		}
	}
	return true;
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

	// Pick every position first (so already-spawned targets cannot block the sight checks), then spawn.
	// If the planned spot is hidden behind terrain, slide it sideways until the player can see it.
	static const float SearchOffsets[] = { 0.f, 600.f, -600.f, 1200.f, -1200.f, 2000.f, -2000.f, 3000.f, -3000.f, 4500.f, -4500.f, 6000.f, -6000.f };
	const FVector Eye = Origin + FVector(0.f, 0.f, 65.f);
	TArray<FVector> Locations;
	for (int32 Index = 0; Index < DummyDistances.Num(); ++Index)
	{
		const float Distance = DummyDistances[Index];
		const float Lateral = DummyLateralOffsets.IsValidIndex(Index) ? DummyLateralOffsets[Index] : 0.f;

		bool bPlaced = false;
		FVector FirstGround = FVector::ZeroVector;
		bool bAnyGround = false;
		for (const float Search : SearchOffsets)
		{
			FVector GroundPoint;
			if (!FindGround(Origin + Forward * Distance + Right * (Lateral + Search), Player, GroundPoint))
			{
				continue;
			}
			if (!bAnyGround)
			{
				bAnyGround = true;
				FirstGround = GroundPoint;
			}
			if (HasLineOfSight(Eye, GroundPoint, Player))
			{
				Locations.Add(GroundPoint);
				bPlaced = true;
				break;
			}
		}
		if (!bPlaced && bAnyGround)
		{
			UE_LOG(LogCETest, Warning, TEXT("Test range: target at %.0f m has no clear sight line, placed anyway"), Distance / 100.f);
			Locations.Add(FirstGround);
		}
		else if (!bPlaced)
		{
			UE_LOG(LogCETest, Warning, TEXT("Test range: no ground at %.0f m (cell not loaded?) - dummy skipped"), Distance / 100.f);
		}
	}

	for (const FVector& GroundPoint : Locations)
	{
		// Dummy origin is its body centre, 90 cm above the feet. Face the shooter.
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FRotator FaceShooter(0.f, (Origin - GroundPoint).Rotation().Yaw, 0.f);
		World->SpawnActor<ATestDummy>(ATestDummy::StaticClass(), GroundPoint + FVector(0.f, 0.f, 90.f), FaceShooter, SpawnParams);
	}

	if (bSpawnTestCarts)
	{
		// Parked to the player's right: a fast cart and a truck-profile one.
		for (int32 CartIndex = 0; CartIndex < 2; ++CartIndex)
		{
			FVector GroundPoint;
			if (!FindGround(Origin + Right * (500.f + CartIndex * 350.f) + Forward * 200.f, Player, GroundPoint))
			{
				continue;
			}
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ATestCart* Cart = World->SpawnActor<ATestCart>(ATestCart::StaticClass(), GroundPoint + FVector(0.f, 0.f, 70.f), Yaw, SpawnParams))
			{
				Cart->bHeavyTruck = CartIndex == 1;
			}
		}
	}
}
