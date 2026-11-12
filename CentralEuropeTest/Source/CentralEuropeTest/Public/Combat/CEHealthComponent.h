#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "CEHealthComponent.generated.h"

class UCEHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCEOnHealthChanged, UCEHealthComponent*, HealthComponent, float, NewHealth, float, DamageTaken);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCEOnDeath, UCEHealthComponent*, HealthComponent, AController*, Killer);

/** Minimal health pool fed by the engine damage pipeline (ApplyPointDamage etc.). */
UCLASS(ClassGroup = (Combat), Blueprintable, meta = (BlueprintSpawnableComponent))
class CENTRALEUROPETEST_API UCEHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCEHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** Seconds after death until health refills. 0 = stay dead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float AutoReviveDelay = 3.f;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return Health <= 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FCEOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FCEOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:
	float Health = 100.f;
	FTimerHandle ReviveTimer;
};
