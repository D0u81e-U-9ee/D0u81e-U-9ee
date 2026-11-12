#include "Test/TerrainPreviewActor.h"

#include "CentralEuropeTest.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ProceduralMeshComponent.h"

namespace
{
	// Must match Tools/generate_terrain.py and the Landscape import settings.
	const float CmPerSample = 100.f;          // 1 m per heightmap pixel
	const float CmPerHeightUnit = 1.953125f;  // Z scale 250
	const float SeaLevelUnits = 4096.f;       // 16-bit value at Z = 0
	const int32 ChunkCells = 63;

	enum ELayer { Forest, Meadow, Scree, Cliff, Snow, LakeBed, NumLayers };

	const FLinearColor LayerColors[NumLayers] = {
		FLinearColor(0.045f, 0.075f, 0.025f),   // forest floor / humus
		FLinearColor(0.16f, 0.26f, 0.05f),      // alpine meadow
		FLinearColor(0.26f, 0.23f, 0.19f),      // scree / gravel
		FLinearColor(0.11f, 0.11f, 0.115f),     // cliff rock
		FLinearColor(0.88f, 0.9f, 0.95f),       // snow
		FLinearColor(0.12f, 0.11f, 0.08f),      // lake bed
	};

	/** Hard-edged version of the auto-material rules (see SourceArt/Materials/LandscapeAutoWeights.ush). */
	ELayer Classify(float SlopeDeg, float Zcm)
	{
		if (Zcm < -50.f)
		{
			return LakeBed;
		}
		if (Zcm > 92500.f && SlopeDeg < 58.f)
		{
			return Snow;       // midpoint of the 850 m -> 1000 m blend
		}
		if (SlopeDeg > 45.f)
		{
			return Cliff;
		}
		if (SlopeDeg > 30.f || Zcm > 70000.f)
		{
			return Scree;
		}
		if (SlopeDeg > 15.f || Zcm > 50000.f)
		{
			return Meadow;
		}
		return Forest;
	}
}

ATerrainPreviewActor::ATerrainPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

bool ATerrainPreviewActor::LoadHeightmap()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), HeightmapPath);
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FullPath))
	{
		UE_LOG(LogCETest, Warning, TEXT("Terrain preview: cannot read %s (run Tools/generate_terrain.py)"), *FullPath);
		return false;
	}

	IImageWrapperModule& ImageModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> Wrapper = ImageModule.CreateImageWrapper(EImageFormat::PNG);
	TArray64<uint8> Raw;
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(FileData.GetData(), FileData.Num()) || !Wrapper->GetRaw(ERGBFormat::Gray, 16, Raw))
	{
		UE_LOG(LogCETest, Warning, TEXT("Terrain preview: %s is not a 16-bit grayscale PNG"), *FullPath);
		return false;
	}

	const int32 Width = static_cast<int32>(Wrapper->GetWidth());
	const int32 Height = static_cast<int32>(Wrapper->GetHeight());
	if (Width != Height || Width < 2 || Raw.Num() < static_cast<int64>(Width) * Height * 2)
	{
		UE_LOG(LogCETest, Warning, TEXT("Terrain preview: unexpected heightmap size %d x %d"), Width, Height);
		return false;
	}

	Resolution = Width;
	Heights.SetNumUninitialized(Resolution * Resolution);
	FMemory::Memcpy(Heights.GetData(), Raw.GetData(), static_cast<SIZE_T>(Resolution) * Resolution * sizeof(uint16));
	return true;
}

float ATerrainPreviewActor::HeightAtSample(int32 Col, int32 Row) const
{
	Col = FMath::Clamp(Col, 0, Resolution - 1);
	Row = FMath::Clamp(Row, 0, Resolution - 1);
	return (static_cast<float>(Heights[Row * Resolution + Col]) - SeaLevelUnits) * CmPerHeightUnit;
}

float ATerrainPreviewActor::GetHeightAt(float WorldX, float WorldY) const
{
	if (Resolution < 2)
	{
		return 0.f;
	}
	// Image column -> world X, row -> world Y, map centred on the origin.
	const float Half = (Resolution - 1) * 0.5f;
	const float Fx = FMath::Clamp(WorldX / CmPerSample + Half, 0.f, Resolution - 1.001f);
	const float Fy = FMath::Clamp(WorldY / CmPerSample + Half, 0.f, Resolution - 1.001f);
	const int32 X0 = FMath::FloorToInt(Fx);
	const int32 Y0 = FMath::FloorToInt(Fy);
	const float Tx = Fx - X0;
	const float Ty = Fy - Y0;
	const float Top = FMath::Lerp(HeightAtSample(X0, Y0), HeightAtSample(X0 + 1, Y0), Tx);
	const float Bottom = FMath::Lerp(HeightAtSample(X0, Y0 + 1), HeightAtSample(X0 + 1, Y0 + 1), Tx);
	return FMath::Lerp(Top, Bottom, Ty);
}

bool ATerrainPreviewActor::Build()
{
	if (bBuilt)
	{
		return true;
	}
	if (!LoadHeightmap())
	{
		return false;
	}
	SampleStep = FMath::Clamp(SampleStep, 1, 16);
	if ((Resolution - 1) % SampleStep != 0)
	{
		UE_LOG(LogCETest, Warning, TEXT("Terrain preview: SampleStep %d does not divide %d, using 1"), SampleStep, Resolution - 1);
		SampleStep = 1;
	}

	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	LayerMaterials.Reset();
	for (int32 Layer = 0; Layer < NumLayers; ++Layer)
	{
		UMaterialInstanceDynamic* Material = BaseMaterial ? UMaterialInstanceDynamic::Create(BaseMaterial, this) : nullptr;
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), LayerColors[Layer]);
		}
		LayerMaterials.Add(Material);
	}

	const double StartTime = FPlatformTime::Seconds();
	const int32 SamplesPerSide = (Resolution - 1) / SampleStep + 1;
	const int32 ChunksPerSide = FMath::DivideAndRoundUp(SamplesPerSide - 1, ChunkCells);
	for (int32 ChunkY = 0; ChunkY < ChunksPerSide; ++ChunkY)
	{
		for (int32 ChunkX = 0; ChunkX < ChunksPerSide; ++ChunkX)
		{
			BuildChunk(ChunkX, ChunkY, SamplesPerSide);
		}
	}

	// Lake surface: one flat plane across the whole map (visible only where the terrain is below it).
	if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		WaterPlane = NewObject<UStaticMeshComponent>(this);
		WaterPlane->SetupAttachment(GetRootComponent());
		WaterPlane->SetStaticMesh(Plane);
		WaterPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WaterPlane->SetCastShadow(false);
		const float SizeCm = (Resolution - 1) * CmPerSample;
		WaterPlane->SetWorldLocation(FVector(0.f, 0.f, WaterZ));
		WaterPlane->SetWorldScale3D(FVector(SizeCm / 100.f, SizeCm / 100.f, 1.f));
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* WaterMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			WaterMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.12f, 0.2f));
			WaterPlane->SetMaterial(0, WaterMaterial);
			LayerMaterials.Add(WaterMaterial);
		}
		WaterPlane->RegisterComponent();
	}

	bBuilt = true;
	UE_LOG(LogCETest, Log, TEXT("Terrain preview built: %d chunks, %d m grid, %.2f s"),
		Chunks.Num(), SampleStep, FPlatformTime::Seconds() - StartTime);
	return true;
}

void ATerrainPreviewActor::BuildChunk(int32 ChunkX, int32 ChunkY, int32 SamplesPerSide)
{
	const int32 X0 = ChunkX * ChunkCells;
	const int32 Y0 = ChunkY * ChunkCells;
	const int32 X1 = FMath::Min(X0 + ChunkCells, SamplesPerSide - 1);
	const int32 Y1 = FMath::Min(Y0 + ChunkCells, SamplesPerSide - 1);
	const int32 Cols = X1 - X0 + 1;
	const int32 Rows = Y1 - Y0 + 1;
	if (Cols < 2 || Rows < 2)
	{
		return;
	}

	const float Half = (Resolution - 1) * 0.5f;
	const float Spacing = CmPerSample * SampleStep;

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	Vertices.Reserve(Cols * Rows);
	Normals.Reserve(Cols * Rows);
	UVs.Reserve(Cols * Rows);

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Col = 0; Col < Cols; ++Col)
		{
			const int32 Sx = (X0 + Col) * SampleStep;   // heightmap column -> world X
			const int32 Sy = (Y0 + Row) * SampleStep;   // heightmap row    -> world Y
			const float Z = HeightAtSample(Sx, Sy);
			Vertices.Emplace((Sx - Half) * CmPerSample, (Sy - Half) * CmPerSample, Z);

			const float DzDx = (HeightAtSample(Sx + SampleStep, Sy) - HeightAtSample(Sx - SampleStep, Sy)) / (2.f * Spacing);
			const float DzDy = (HeightAtSample(Sx, Sy + SampleStep) - HeightAtSample(Sx, Sy - SampleStep)) / (2.f * Spacing);
			Normals.Add(FVector(-DzDx, -DzDy, 1.f).GetSafeNormal());
			UVs.Emplace(Sx / 16.f, Sy / 16.f);
		}
	}

	TArray<int32> AllTriangles;
	TArray<int32> LayerTriangles[NumLayers];
	AllTriangles.Reserve((Cols - 1) * (Rows - 1) * 6);

	auto AddTriangle = [&](int32 A, int32 B, int32 C)
	{
		const FVector FaceNormal = FVector::CrossProduct(Vertices[C] - Vertices[A], Vertices[B] - Vertices[A]).GetSafeNormal();
		const float SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FMath::Abs(static_cast<float>(FaceNormal.Z)), 0.f, 1.f)));
		const float AvgZ = static_cast<float>((Vertices[A].Z + Vertices[B].Z + Vertices[C].Z) / 3.0);
		TArray<int32>& Target = LayerTriangles[Classify(SlopeDeg, AvgZ)];
		Target.Add(A); Target.Add(B); Target.Add(C);
		AllTriangles.Add(A); AllTriangles.Add(B); AllTriangles.Add(C);
	};

	for (int32 Row = 0; Row < Rows - 1; ++Row)
	{
		for (int32 Col = 0; Col < Cols - 1; ++Col)
		{
			const int32 V00 = Row * Cols + Col;
			const int32 V10 = V00 + 1;          // +X
			const int32 V01 = V00 + Cols;       // +Y
			const int32 V11 = V01 + 1;
			// Winding that faces +Z in Unreal's left-handed space.
			AddTriangle(V00, V01, V10);
			AddTriangle(V10, V01, V11);
		}
	}

	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->bUseAsyncCooking = false;                 // collision must exist before the player spawns
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->RegisterComponent();

	const TArray<FLinearColor> NoColors;
	const TArray<FProcMeshTangent> NoTangents;

	// Section 0: every triangle, collision only (hidden) -> the physics mesh is cooked once per chunk.
	Mesh->CreateMeshSection_LinearColor(0, Vertices, AllTriangles, Normals, UVs, NoColors, NoTangents, true);
	Mesh->SetMeshSectionVisible(0, false);

	// Sections 1..N: one per surface layer, render only.
	int32 Section = 1;
	for (int32 Layer = 0; Layer < NumLayers; ++Layer)
	{
		if (LayerTriangles[Layer].Num() == 0)
		{
			continue;
		}
		Mesh->CreateMeshSection_LinearColor(Section, Vertices, LayerTriangles[Layer], Normals, UVs, NoColors, NoTangents, false);
		if (LayerMaterials.IsValidIndex(Layer) && LayerMaterials[Layer])
		{
			Mesh->SetMaterial(Section, LayerMaterials[Layer]);
		}
		++Section;
	}
	Chunks.Add(Mesh);
}

void ATerrainPreviewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bBuilt)
	{
		UpdateSwimming();
	}
}

void ATerrainPreviewActor::UpdateSwimming()
{
	// The preview has no Water plugin volumes, so characters are moved in and out of a brushless
	// water PhysicsVolume by hand. CharacterMovement then switches to / from MOVE_Swimming itself.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!WaterVolume)
	{
		WaterVolume = World->SpawnActor<APhysicsVolume>();
		if (!WaterVolume)
		{
			return;
		}
		WaterVolume->bWaterVolume = true;
		WaterVolume->FluidFriction = 0.3f;
	}

	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Character = *It;
		UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		if (!Capsule || !Movement || Movement->MovementMode == MOVE_None)
		{
			continue;   // e.g. seated in a vehicle
		}

		FVector Location = Character->GetActorLocation();
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		const float GroundZ = GetHeightAt(Location.X, Location.Y);
		const float WaterDepth = WaterZ - GroundZ;
		const bool bInWater = Capsule->GetPhysicsVolume() == WaterVolume;

		if (!bInWater)
		{
			// Chest-deep in water that is deep enough to float in.
			if (WaterDepth > 130.f && Location.Z < WaterZ - 20.f)
			{
				Capsule->SetShouldUpdatePhysicsVolume(false);
				Capsule->SetPhysicsVolume(WaterVolume, true);
			}
		}
		else if (WaterDepth < 110.f && Location.Z - HalfHeight <= GroundZ + 15.f)
		{
			// Feet touch the bed in the shallows: wade out.
			Capsule->SetPhysicsVolume(World->GetDefaultPhysicsVolume(), true);
		}
		else if (Location.Z > WaterZ - 25.f)
		{
			// Keep the swimmer at the surface instead of popping out and falling back in.
			Location.Z = WaterZ - 25.f;
			Character->SetActorLocation(Location);
			if (Movement->Velocity.Z > 0.f)
			{
				Movement->Velocity.Z = 0.f;
			}
		}
	}
}
