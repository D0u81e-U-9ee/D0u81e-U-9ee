#include "Test/TestDummy.h"

#include "CentralEuropeTest.h"
#include "Combat/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ATestDummy::ATestDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	// Engine cylinder is 100 cm tall / 100 cm wide with its pivot at the centre: 50 cm wide, 180 cm tall body.
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Body->SetStaticMesh(Cylinder.Object);
	}
	Body->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.8f));
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Block);

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
	const FString Text = FString::Printf(TEXT("%s  hit #%d  dmg %.0f  range %.1f m  height %.0f cm  by %s  hp %.0f"),
		*GetName(), HitCount, Damage, RangeM, HitLocation.Z - GetActorLocation().Z, *GetNameSafe(DamageCauser), HealthAfter);

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
