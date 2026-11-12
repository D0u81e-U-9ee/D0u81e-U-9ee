#include "Audio/CEAmbienceActor.h"

#include "Audio/CEAudioSubsystem.h"
#include "CentralEuropeTest.h"
#include "Components/AudioComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

ACEAmbienceActor::ACEAmbienceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ACEAmbienceActor::BeginPlay()
{
	Super::BeginPlay();

	if (UCEAudioSubsystem* Audio = UCEAudioSubsystem::Get(this))
	{
		WindLoop = Audio->StartLoop2D(this, TEXT("Amb_Wind"), 0.f);
		BirdsLoop = Audio->StartLoop2D(this, TEXT("Amb_Birds"), 0.f);
		LakeLoop = Audio->StartLoop2D(this, TEXT("Amb_Lake"), 0.f);
	}
	LoadEmitters();
}

void ACEAmbienceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UAudioComponent* Loop : { WindLoop.Get(), BirdsLoop.Get(), LakeLoop.Get() })
	{
		if (Loop)
		{
			Loop->Stop();
		}
	}
	for (FEmitter& Emitter : Emitters)
	{
		if (UAudioComponent* Component = Emitter.Component.Get())
		{
			Component->Stop();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ACEAmbienceActor::LoadEmitters()
{
	FString Text;
	const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), LayoutPath);
	if (!FFileHelper::LoadFileToString(Text, *FullPath))
	{
		UE_LOG(LogCETest, Log, TEXT("Ambience: %s not found, no river / waterfall emitters"), *FullPath);
		return;
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return;
	}

	auto PointOf = [](const TSharedPtr<FJsonObject>& Object)
	{
		return FVector(Object->GetNumberField(TEXT("x")), Object->GetNumberField(TEXT("y")), Object->GetNumberField(TEXT("z")));
	};

	// Rivers: one emitter roughly every 120 m along each spline.
	if (Root->HasTypedField<EJson::Object>(TEXT("rivers")))
	{
		for (const auto& River : Root->GetObjectField(TEXT("rivers"))->Values)
		{
			const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
			if (!River.Value.IsValid() || !River.Value->TryGetArray(Points) || !Points)
			{
				continue;
			}
			for (int32 Index = 1; Index < Points->Num(); Index += 3)
			{
				const TSharedPtr<FJsonObject>* Point = nullptr;
				if ((*Points)[Index].IsValid() && (*Points)[Index]->TryGetObject(Point) && Point)
				{
					FEmitter& Emitter = Emitters.AddDefaulted_GetRef();
					Emitter.Location = PointOf(*Point);
					Emitter.Key = TEXT("Amb_River");
					Emitter.InnerRadius = 800.f;
					Emitter.Falloff = 7000.f;
					Emitter.Volume = 0.55f;
				}
			}
		}
	}

	// Waterfalls: heard across the valley.
	const TArray<TSharedPtr<FJsonValue>>* Falls = nullptr;
	if (Root->TryGetArrayField(TEXT("waterfalls"), Falls) && Falls)
	{
		for (const TSharedPtr<FJsonValue>& Fall : *Falls)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (Fall.IsValid() && Fall->TryGetObject(Object) && Object && (*Object)->HasTypedField<EJson::Object>(TEXT("base")))
			{
				FEmitter& Emitter = Emitters.AddDefaulted_GetRef();
				Emitter.Location = PointOf((*Object)->GetObjectField(TEXT("base")));
				Emitter.Key = TEXT("Amb_Waterfall");
				Emitter.InnerRadius = 3000.f;
				Emitter.Falloff = 45000.f;
				Emitter.Volume = 1.f;
			}
		}
	}
	UE_LOG(LogCETest, Log, TEXT("Ambience: %d water emitters"), Emitters.Num());
}

void ACEAmbienceActor::SetLoopVolume(UAudioComponent* Component, float Volume)
{
	if (Component)
	{
		Component->SetVolumeMultiplier(FMath::Max(Volume, 0.001f));
	}
}

void ACEAmbienceActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Listen from the camera (works on foot and in vehicles).
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}
	FVector Listener;
	FRotator Unused;
	PC->GetPlayerViewPoint(Listener, Unused);

	// Altitude mix: sheltered valley floor -> exposed ridge.
	const float AltitudeM = Listener.Z / 100.f;
	const float Exposure = FMath::Clamp((AltitudeM - 60.f) / 640.f, 0.f, 1.f);
	const float TreeLine = 1.f - FMath::Clamp((AltitudeM - 350.f) / 300.f, 0.f, 1.f);
	SetLoopVolume(WindLoop, MasterVolume * WindVolume * FMath::Lerp(0.35f, 1.6f, Exposure));
	SetLoopVolume(BirdsLoop, MasterVolume * BirdsVolume * TreeLine);

	// Lake: distance inland from the L-shaped shoreline (same soft maximum as the generator, in cm).
	const float Soft = 8000.f;
	const float A = -Listener.X;
	const float B = -Listener.Y;
	const float M = FMath::Max(A, B);
	const float TowardLake = M + Soft * FMath::Loge(FMath::Exp((A - M) / Soft) + FMath::Exp((B - M) / Soft));
	const float Inland = ShorelineDistance - TowardLake;                 // <= 0 on the water
	const float NearShore = 1.f - FMath::Clamp(Inland / 12000.f, 0.f, 1.f);
	const float LowEnough = 1.f - FMath::Clamp((AltitudeM - 15.f) / 40.f, 0.f, 1.f);
	SetLoopVolume(LakeLoop, MasterVolume * LakeVolume * NearShore * LowEnough);

	// Water emitters: only the ones within earshot hold a voice.
	UCEAudioSubsystem* Audio = UCEAudioSubsystem::Get(this);
	if (!Audio)
	{
		return;
	}
	for (FEmitter& Emitter : Emitters)
	{
		const float Reach = Emitter.InnerRadius + Emitter.Falloff;
		const float Distance = FVector::Dist(Listener, Emitter.Location);
		UAudioComponent* Component = Emitter.Component.Get();
		if (!Component && Distance < Reach)
		{
			Emitter.Component = Audio->StartLoopAt(this, Emitter.Key, Emitter.Location, Emitter.InnerRadius, Emitter.Falloff, Emitter.Volume * MasterVolume);
		}
		else if (Component && Distance > Reach * 1.15f)
		{
			Component->Stop();
			Component->DestroyComponent();
			Emitter.Component = nullptr;
		}
	}
}
