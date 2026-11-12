#include "Audio/CEAudioSubsystem.h"

#include "CentralEuropeTest.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

namespace
{
	/** Real recordings from Epic's Starter Content are preferred over the synthesised beds when present. */
	const TCHAR* PreferredOverride(FName Key)
	{
		if (Key == FName(TEXT("Amb_Wind")))
		{
			return TEXT("/Game/StarterContent/Audio/Starter_Wind06");
		}
		if (Key == FName(TEXT("Amb_Birds")))
		{
			return TEXT("/Game/StarterContent/Audio/Starter_Birds01");
		}
		return nullptr;
	}

	USoundBase* LoadIfPresent(const FString& PackagePath)
	{
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr;
		}
		const FString ObjectPath = PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath);
		return LoadObject<USoundBase>(nullptr, *ObjectPath);
	}
}

UCEAudioSubsystem* UCEAudioSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = WorldContextObject ? UGameplayStatics::GetGameInstance(WorldContextObject) : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UCEAudioSubsystem>() : nullptr;
}

USoundBase* UCEAudioSubsystem::FindSound(FName Key)
{
	if (Key.IsNone() || Missing.Contains(Key))
	{
		return nullptr;
	}
	if (const TObjectPtr<USoundBase>* Cached = Cache.Find(Key))
	{
		return Cached->Get();
	}

	USoundBase* Sound = nullptr;
	if (const TCHAR* Override = PreferredOverride(Key))
	{
		Sound = LoadIfPresent(Override);
	}
	if (!Sound)
	{
		Sound = LoadIfPresent(FString(TEXT("/Game/CE/Audio/")) + Key.ToString());
	}

	if (Sound)
	{
		Cache.Add(Key, Sound);
	}
	else
	{
		Missing.Add(Key);
		UE_LOG(LogCETest, Warning, TEXT("Audio: sound '%s' not found in /Game/CE/Audio (restart the editor once so the audio import runs)"), *Key.ToString());
	}
	return Sound;
}

USoundBase* UCEAudioSubsystem::FindLooping(FName Key)
{
	USoundBase* Sound = FindSound(Key);
	if (USoundWave* Wave = Cast<USoundWave>(Sound))
	{
		Wave->bLooping = true;   // in case the import did not flag it
	}
	return Sound;
}

USoundAttenuation* UCEAudioSubsystem::MakeAttenuation(float InnerRadius, float FalloffDistance)
{
	USoundAttenuation* Asset = NewObject<USoundAttenuation>(this);
	FSoundAttenuationSettings& Settings = Asset->Attenuation;
	Settings.bAttenuate = true;
	Settings.bSpatialize = true;
	Settings.AttenuationShape = EAttenuationShape::Sphere;
	Settings.AttenuationShapeExtents = FVector(InnerRadius, 0.f, 0.f);
	Settings.FalloffDistance = FalloffDistance;
	Settings.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	return Asset;
}

USoundAttenuation* UCEAudioSubsystem::GetAttenuation(ECESoundRange Range)
{
	switch (Range)
	{
	case ECESoundRange::Near:
		if (!NearAttenuation) { NearAttenuation = MakeAttenuation(150.f, 2500.f); }
		return NearAttenuation;
	case ECESoundRange::Far:
		if (!FarAttenuation) { FarAttenuation = MakeAttenuation(1500.f, 90000.f); }
		return FarAttenuation;
	default:
		if (!MidAttenuation) { MidAttenuation = MakeAttenuation(400.f, 12000.f); }
		return MidAttenuation;
	}
}

void UCEAudioSubsystem::PlayAt(const UObject* WorldContextObject, FName Key, const FVector& Location, ECESoundRange Range, float Volume, float Pitch)
{
	UCEAudioSubsystem* Audio = Get(WorldContextObject);
	USoundBase* Sound = Audio ? Audio->FindSound(Key) : nullptr;
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, Volume, Pitch, 0.f, Audio->GetAttenuation(Range));
	}
}

void UCEAudioSubsystem::Play2D(const UObject* WorldContextObject, FName Key, float Volume, float Pitch)
{
	UCEAudioSubsystem* Audio = Get(WorldContextObject);
	USoundBase* Sound = Audio ? Audio->FindSound(Key) : nullptr;
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(WorldContextObject, Sound, Volume, Pitch);
	}
}

void UCEAudioSubsystem::PlayVariantAt(const UObject* WorldContextObject, FName KeyBase, int32 NumVariants, const FVector& Location, ECESoundRange Range, float Volume, float Pitch)
{
	const int32 Variant = FMath::RandRange(1, FMath::Max(NumVariants, 1));
	PlayAt(WorldContextObject, FName(*FString::Printf(TEXT("%s_%d"), *KeyBase.ToString(), Variant)), Location, Range, Volume, Pitch);
}

UAudioComponent* UCEAudioSubsystem::StartLoop2D(const UObject* WorldContextObject, FName Key, float Volume)
{
	USoundBase* Sound = FindLooping(Key);
	if (!Sound || !WorldContextObject)
	{
		return nullptr;
	}
	// A volume of exactly 0 would stop the voice, so loops are started just above silence.
	return UGameplayStatics::SpawnSound2D(WorldContextObject, Sound, FMath::Max(Volume, 0.001f), 1.f, 0.f, nullptr, false, false);
}

UAudioComponent* UCEAudioSubsystem::StartLoopAt(const UObject* WorldContextObject, FName Key, const FVector& Location, float InnerRadius, float FalloffDistance, float Volume)
{
	USoundBase* Sound = FindLooping(Key);
	if (!Sound || !WorldContextObject)
	{
		return nullptr;
	}
	USoundAttenuation* Attenuation = MakeAttenuation(InnerRadius, FalloffDistance);
	LoopAttenuations.Add(Attenuation);
	return UGameplayStatics::SpawnSoundAtLocation(WorldContextObject, Sound, Location, FRotator::ZeroRotator, Volume, 1.f, 0.f, Attenuation, nullptr, false);
}

UAudioComponent* UCEAudioSubsystem::StartLoopAttached(FName Key, USceneComponent* AttachTo, float FalloffDistance, float Volume)
{
	USoundBase* Sound = FindLooping(Key);
	if (!Sound || !AttachTo)
	{
		return nullptr;
	}
	USoundAttenuation* Attenuation = MakeAttenuation(300.f, FalloffDistance);
	LoopAttenuations.Add(Attenuation);
	return UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
		false, FMath::Max(Volume, 0.001f), 1.f, 0.f, Attenuation, nullptr, false);
}
