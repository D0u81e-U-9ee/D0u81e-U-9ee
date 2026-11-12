#include "Test/CETestDummy.h"

#include "CentralEuropeTest.h"
#include "Audio/CEAudioSubsystem.h"
#include "Combat/CEHealthComponent.h"
#include "Character/CEBlockMannequinComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

ACETestDummy::ACETestDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Body"));
	Body->InitCapsuleSize(30.f, 90.f);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_Weapon, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Melee, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Projectile, ECR_Ignore);
	SetRootComponent(Body);

	Mannequin = CreateDefaultSubobject<UCEBlockMannequinComponent>(TEXT("Mannequin"));
	Mannequin->SetupAttachment(Body);
	Mannequin->BodyColor = FLinearColor(0.85f, 0.4f, 0.1f);
	Mannequin->bAnimate = false;

	Health = CreateDefaultSubobject<UCEHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 100.f;
	Health->AutoReviveDelay = 3.f;
}

void ACETestDummy::BeginPlay()
{
	Super::BeginPlay();
	OnTakePointDamage.AddDynamic(this, &ACETestDummy::HandlePointDamage);
	Health->OnDeath.AddDynamic(this, &ACETestDummy::HandleDeath);
}

void ACETestDummy::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	++HitCount;

	const APawn* Shooter = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	const float RangeM = Shooter ? FVector::Dist(Shooter->GetActorLocation(), HitLocation) / 100.f : 0.f;
	// Health is updated by the generic damage delegate, which fires after this one.
	const float HealthAfter = FMath::Max(Health->GetHealth() - Damage, 0.f);
	const bool bHead = HitComponent && HitComponent->ComponentHasTag(TEXT("head"));
	const FString Text = FString::Printf(TEXT("%s  hit #%d  %s  dmg %.0f  range %.1f m  by %s  hp %.0f"),
		*GetName(), HitCount, bHead ? TEXT("HEAD") : TEXT("body"), Damage, RangeM, *GetNameSafe(DamageCauser), HealthAfter);

	UE_LOG(LogCETest, Log, TEXT("%s"), *Text);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 4.f, FColor::Orange, Text);
	}
}

void ACETestDummy::HandleDeath(UCEHealthComponent* HealthComponent, AController* Killer)
{
	UE_LOG(LogCETest, Log, TEXT("%s destroyed (resets in %.0f s)"), *GetName(), Health->AutoReviveDelay);
	UCEAudioSubsystem::PlayAt(this, TEXT("Target_Down"), GetActorLocation(), ECESoundRange::Mid);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString::Printf(TEXT("%s DOWN"), *GetName()));
	}
}
