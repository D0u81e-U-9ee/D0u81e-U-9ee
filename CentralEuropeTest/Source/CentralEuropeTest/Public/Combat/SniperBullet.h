#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SniperBullet.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

/** Ballistic projectile (BP_SniperBullet parent): 850 m/s, full gravity, swept collision. */
UCLASS(Blueprintable)
class CENTRALEUROPETEST_API ASniperBullet : public AActor
{
	GENERATED_BODY()

public:
	ASniperBullet();

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
