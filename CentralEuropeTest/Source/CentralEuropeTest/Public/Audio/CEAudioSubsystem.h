#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CEAudioSubsystem.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundBase;

UENUM(BlueprintType)
enum class ECESoundRange : uint8
{
	/** Footsteps, handling noises: audible to ~25 m. */
	Near,
	/** Impacts, splashes, melee: ~120 m. */
	Mid,
	/** Gunshots: ~900 m. */
	Far
};

/**
 * Plays the prototype's sounds by name. A key such as "Gun_Pistol" resolves to the asset
 * /Game/CE/Audio/Gun_Pistol (imported automatically from SourceArt/Audio by
 * Content/Python/init_unreal.py). Missing sounds are skipped silently and logged once, so the
 * game runs with or without the audio set. Swap any sound by importing another under the same name.
 */
UCLASS()
class CENTRALEUROPETEST_API UCEAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UCEAudioSubsystem* Get(const UObject* WorldContextObject);

	/** Convenience wrappers that tolerate a missing subsystem / world. */
	static void PlayAt(const UObject* WorldContextObject, FName Key, const FVector& Location, ECESoundRange Range = ECESoundRange::Mid, float Volume = 1.f, float Pitch = 1.f);
	static void Play2D(const UObject* WorldContextObject, FName Key, float Volume = 1.f, float Pitch = 1.f);
	/** Picks Key_1 .. Key_N at random. */
	static void PlayVariantAt(const UObject* WorldContextObject, FName KeyBase, int32 NumVariants, const FVector& Location, ECESoundRange Range = ECESoundRange::Near, float Volume = 1.f, float Pitch = 1.f);

	UFUNCTION(BlueprintCallable, Category = "CE|Audio")
	USoundBase* FindSound(FName Key);

	/** Starts a looping, non-spatial sound (ambience beds). Returns null when the sound is missing. */
	UAudioComponent* StartLoop2D(const UObject* WorldContextObject, FName Key, float Volume);

	/** Starts a looping sound at a world position with its own audible radius. */
	UAudioComponent* StartLoopAt(const UObject* WorldContextObject, FName Key, const FVector& Location, float InnerRadius, float FalloffDistance, float Volume);

	/** Starts a looping sound that follows a component (engines). */
	UAudioComponent* StartLoopAttached(FName Key, USceneComponent* AttachTo, float FalloffDistance, float Volume);

	USoundAttenuation* GetAttenuation(ECESoundRange Range);

private:
	USoundAttenuation* MakeAttenuation(float InnerRadius, float FalloffDistance);
	USoundBase* FindLooping(FName Key);

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<USoundBase>> Cache;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> NearAttenuation;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> MidAttenuation;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> FarAttenuation;

	/** Keeps per-loop attenuation objects alive. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundAttenuation>> LoopAttenuations;

	TSet<FName> Missing;
};
