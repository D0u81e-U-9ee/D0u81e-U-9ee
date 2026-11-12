#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CETerrainPreviewActor.generated.h"

class APhysicsVolume;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class UStaticMeshComponent;

/**
 * Playtest stand-in for the Landscape: builds the generated heightmap as collidable procedural
 * mesh chunks at runtime, coloured by the auto-material rules (forest / meadow / scree / cliff /
 * snow), with a flat lake surface at WaterZ and simple swimming support.
 *
 * It lets the map be walked, driven and shot across straight after compiling. It is NOT the final
 * terrain: no World Partition streaming, no Landscape material, no Water plugin. ACETestGameMode
 * only spawns it when the level contains no Landscape. Editor / PIE only (reads SourceArt/).
 */
UCLASS()
class CENTRALEUROPETEST_API ACETerrainPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ACETerrainPreviewActor();

	/** 16-bit grayscale PNG, relative to the project directory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FString HeightmapPath = TEXT("SourceArt/Terrain/heightmap_2017.png");

	/** Use every Nth heightmap sample (4 = 4 m grid, ~0.5 M triangles). Must divide (resolution - 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "16"))
	int32 SampleStep = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float WaterZ = 0.f;

	/** Loads the heightmap and creates all chunks (with collision) synchronously. */
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	bool Build();

	UFUNCTION(BlueprintPure, Category = "Terrain")
	bool IsBuilt() const { return bBuilt; }

	/** Bilinear terrain height (cm) at a world XY position. */
	UFUNCTION(BlueprintPure, Category = "Terrain")
	float GetHeightAt(float WorldX, float WorldY) const;

	virtual void Tick(float DeltaSeconds) override;

private:
	bool LoadHeightmap();
	float HeightAtSample(int32 Col, int32 Row) const;
	void BuildChunk(int32 ChunkX, int32 ChunkY, int32 SamplesPerSide);
	void UpdateSwimming();

	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Chunks;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> LayerMaterials;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> WaterPlane;
	UPROPERTY(Transient) TObjectPtr<APhysicsVolume> WaterVolume;

	TArray<uint16> Heights;
	int32 Resolution = 0;
	bool bBuilt = false;
};
