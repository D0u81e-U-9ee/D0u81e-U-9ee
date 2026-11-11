#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestDummy.generated.h"

class UHealthComponent;
class UStaticMeshComponent;

/** BP_TestDummy parent: human-sized target that logs every hit (damage, range, bone) on screen and in the log. */
UCLASS(Blueprintable)
class CENTRALEUROPETEST_API ATestDummy : public AActor
{
	GENERATED_BODY()

public:
	ATestDummy();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dummy")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dummy")
	TObjectPtr<UHealthComponent> Health;

	/** Total damage events received; used to verify "one hit per swing / shot". */
	UPROPERTY(BlueprintReadOnly, Category = "Dummy")
	int32 HitCount = 0;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(UHealthComponent* HealthComponent, AController* Killer);
};
