#include "Combat/SniperBullet.h"

#include "CentralEuropeTest.h"
#include "Combat/WeaponTypes.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ASniperBullet::ASniperBullet()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(2.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_Projectile);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Projectile, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCanEverAffectNavigation(false);
	SetRootComponent(Collision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 85000.f;
	Movement->MaxSpeed = 85000.f;
	Movement->ProjectileGravityScale = 1.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->bForceSubStepping = true;
	Movement->MaxSimulationTimeStep = 1.f / 120.f;
	Movement->MaxSimulationIterations = 8;

	InitialLifeSpan = 3.f;   // ~2.5 km, the full map width
	bReplicates = true;
	SetReplicateMovement(true);
}

void ASniperBullet::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();
	if (AActor* OwnerActor = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(OwnerActor, true);
	}
	if (APawn* InstigatorPawn = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(InstigatorPawn, true);
	}
	Movement->OnProjectileStop.AddDynamic(this, &ASniperBullet::HandleProjectileStop);
}

void ASniperBullet::HandleProjectileStop(const FHitResult& ImpactResult)
{
	// Velocity is already zeroed when this fires; rotation still follows the last flight direction.
	const FVector ShotDirection = GetActorForwardVector();

	if (HasAuthority())
	{
		if (AActor* HitActor = ImpactResult.GetActor())
		{
			const bool bHeadshot = IsHeadshot(ImpactResult);
			const float FinalDamage = Damage * (bHeadshot ? HeadshotMultiplier : 1.f);
			APawn* InstigatorPawn = GetInstigator();
			UGameplayStatics::ApplyPointDamage(HitActor, FinalDamage, ShotDirection, ImpactResult,
				InstigatorPawn ? InstigatorPawn->GetController() : nullptr, this, UDamageType::StaticClass());
		}
	}

#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(GetWorld(), ImpactResult.ImpactPoint, 12.f, 8, FColor::Orange, false, 3.f);
#endif
	UE_LOG(LogCETest, Verbose, TEXT("Sniper bullet impact after %.1f m, drop %.1f cm"),
		FVector::Dist2D(SpawnLocation, ImpactResult.ImpactPoint) / 100.f, SpawnLocation.Z - ImpactResult.ImpactPoint.Z);

	Destroy();
}
