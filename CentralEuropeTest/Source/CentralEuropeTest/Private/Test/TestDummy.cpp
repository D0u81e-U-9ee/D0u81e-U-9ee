#include "Test/TestDummy.h"

#include "CentralEuropeTest.h"
#include "Combat/HealthComponent.h"
#include "Character/BlockMannequinComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

ATestDummy::ATestDummy()
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

	Mannequin = CreateDefaultSubobject<UBlockMannequinComponent>(TEXT("Mannequin"));
	Mannequin->SetupAttachment(Body);
	Mannequin->BodyColor = FLinearColor(0.85f, 0.4f, 0.1f);
	Mannequin->bAnimate = false;

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 100.f;
	Health->AutoReviveDelay = 3.f;
}

void ATestDummy::BeginPlay()
{
	Super::BeginPlay();
	OnTakePointDamage.AddDynamic(this, &ATestDummy::HandlePointDamage);
	Health->OnDeath.AddDynamic(this, &ATestDummy::HandleDeath);
}

void ATestDummy::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
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

void ATestDummy::HandleDeath(UHealthComponent* HealthComponent, AController* Killer)
{
	UE_LOG(LogCETest, Log, TEXT("%s destroyed (resets in %.0f s)"), *GetName(), Health->AutoReviveDelay);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString::Printf(TEXT("%s DOWN"), *GetName()));
	}
}
