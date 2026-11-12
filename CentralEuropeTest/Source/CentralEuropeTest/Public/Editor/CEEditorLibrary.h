#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CEEditorLibrary.generated.h"

class UMaterialInterface;

/** Shared heightmap constants and loader (must match Tools/generate_terrain.py). */
namespace CETerrain
{
	const float CmPerSample = 100.f;           // 1 m per heightmap pixel
	const float CmPerHeightUnit = 1.953125f;   // landscape Z scale 250
	const float SeaLevelUnits = 4096.f;        // 16-bit value at Z = 0
	const float LandscapeZScale = 250.f;
	const float LandscapeZOffset = 56000.f;    // actor Z so that SeaLevelUnits lands on Z = 0

	/** Reads a square 16-bit grayscale PNG. Returns false (and logs why) on any problem. */
	CENTRALEUROPETEST_API bool LoadHeightmap16(const FString& AbsolutePath, TArray<uint16>& OutHeights, int32& OutResolution);
}

/**
 * Editor automation entry points, callable from Python as unreal.CEEditorLibrary.*.
 * They do nothing (and return null) outside the editor.
 */
UCLASS()
class CENTRALEUROPETEST_API UCEEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Creates the real Landscape actor from the generated heightmap in the given world:
	 * 63x63 quads per section, 1 section per component, centred on the origin, 1 m per pixel,
	 * lake level at Z = 0. Returns the new actor, or the existing Landscape if there already is one.
	 */
	UFUNCTION(BlueprintCallable, Category = "CE|Editor", meta = (WorldContext = "WorldContextObject"))
	static AActor* ImportTerrainLandscape(UObject* WorldContextObject, const FString& HeightmapFile, UMaterialInterface* Material);
};
