#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CESniperBullet.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

/** Ballistic projectile (BP_SniperBullet parent): 850 m/s, full gravity, swept collision. */
UCLASS(Blueprintable)
class CENTRALEUROPETEST_API ACESniperBullet : public AActor
{
	GENERATED_BODY()

public:
	ACESniperBullet();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ExposeOnSpawn = "true"))
	float Damage = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ExposeOnSpawn = "true"))
	float HeadshotMultiplier = 2.f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

private:
	FVector SpawnLocation = FVector::ZeroVector;
};
