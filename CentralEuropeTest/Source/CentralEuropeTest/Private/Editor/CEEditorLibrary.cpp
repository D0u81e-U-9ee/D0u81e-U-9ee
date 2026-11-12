#include "Editor/CEEditorLibrary.h"

#include "CentralEuropeTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#endif

bool CETerrain::LoadHeightmap16(const FString& AbsolutePath, TArray<uint16>& OutHeights, int32& OutResolution)
{
	OutHeights.Reset();
	OutResolution = 0;

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *AbsolutePath))
	{
		UE_LOG(LogCETest, Warning, TEXT("Heightmap: cannot read %s (run Tools/generate_terrain.py)"), *AbsolutePath);
		return false;
	}

	IImageWrapperModule& ImageModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> Wrapper = ImageModule.CreateImageWrapper(EImageFormat::PNG);
	TArray64<uint8> Raw;
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(FileData.GetData(), FileData.Num()) || !Wrapper->GetRaw(ERGBFormat::Gray, 16, Raw))
	{
		UE_LOG(LogCETest, Warning, TEXT("Heightmap: %s is not a 16-bit grayscale PNG"), *AbsolutePath);
		return false;
	}

	const int32 Width = static_cast<int32>(Wrapper->GetWidth());
	const int32 Height = static_cast<int32>(Wrapper->GetHeight());
	if (Width != Height || Width < 2 || Raw.Num() < static_cast<int64>(Width) * Height * 2)
	{
		UE_LOG(LogCETest, Warning, TEXT("Heightmap: unexpected size %d x %d"), Width, Height);
		return false;
	}

	OutResolution = Width;
	OutHeights.SetNumUninitialized(Width * Width);
	FMemory::Memcpy(OutHeights.GetData(), Raw.GetData(), static_cast<SIZE_T>(Width) * Width * sizeof(uint16));
	return true;
}

AActor* UCEEditorLibrary::ImportTerrainLandscape(UObject* WorldContextObject, const FString& HeightmapFile, UMaterialInterface* Material)
{
#if WITH_EDITOR
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		UE_LOG(LogCETest, Error, TEXT("ImportTerrainLandscape: no world"));
		return nullptr;
	}

	for (TActorIterator<ALandscape> It(World); It; ++It)
	{
		UE_LOG(LogCETest, Log, TEXT("ImportTerrainLandscape: the level already has a Landscape, keeping it"));
		if (Material)
		{
			It->LandscapeMaterial = Material;
			It->PostEditChange();
		}
		return *It;
	}

	TArray<uint16> Heights;
	int32 Resolution = 0;
	if (!CETerrain::LoadHeightmap16(HeightmapFile, Heights, Resolution))
	{
		return nullptr;
	}

	const int32 QuadsPerSection = 63;
	const int32 SectionsPerComponent = 1;
	if ((Resolution - 1) % QuadsPerSection != 0)
	{
		UE_LOG(LogCETest, Error, TEXT("ImportTerrainLandscape: resolution %d is not N x 63 + 1"), Resolution);
		return nullptr;
	}

	// Centre the map on the origin; the Z offset puts the lake level (height value 4096) at Z = 0.
	const float HalfExtent = (Resolution - 1) * CETerrain::CmPerSample * 0.5f;
	const FVector Location(-HalfExtent, -HalfExtent, CETerrain::LandscapeZOffset);

	ALandscape* Landscape = World->SpawnActor<ALandscape>(Location, FRotator::ZeroRotator);
	if (!Landscape)
	{
		UE_LOG(LogCETest, Error, TEXT("ImportTerrainLandscape: could not spawn the Landscape actor"));
		return nullptr;
	}
	Landscape->SetActorScale3D(FVector(CETerrain::CmPerSample, CETerrain::CmPerSample, CETerrain::LandscapeZScale));
	Landscape->LandscapeMaterial = Material;

	TMap<FGuid, TArray<uint16>> HeightDataPerLayer;
	HeightDataPerLayer.Add(FGuid(), MoveTemp(Heights));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayersPerLayer;
	MaterialLayersPerLayer.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

	Landscape->Import(FGuid::NewGuid(), 0, 0, Resolution - 1, Resolution - 1, SectionsPerComponent, QuadsPerSection,
		HeightDataPerLayer, nullptr, MaterialLayersPerLayer, ELandscapeImportAlphamapType::Additive);

	if (ULandscapeInfo* Info = Landscape->GetLandscapeInfo())
	{
		Info->UpdateLayerInfoMap(Landscape);
	}
	Landscape->RegisterAllComponents();
	Landscape->PostEditChange();
	Landscape->SetActorLabel(TEXT("Landscape"));
	Landscape->MarkPackageDirty();

	UE_LOG(LogCETest, Log, TEXT("ImportTerrainLandscape: created %d x %d landscape (%d components)"),
		Resolution, Resolution, FMath::Square((Resolution - 1) / QuadsPerSection));
	return Landscape;
#else
	return nullptr;
#endif
}
