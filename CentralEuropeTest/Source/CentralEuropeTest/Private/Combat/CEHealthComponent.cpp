#include "Combat/CEHealthComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UCEHealthComponent::UCEHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCEHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UCEHealthComponent::HandleAnyDamage);
	}
}

void UCEHealthComponent::ResetHealth()
{
	Health = MaxHealth;
	OnHealthChanged.Broadcast(this, Health, 0.f);
}

void UCEHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || IsDead())
	{
		return;
	}
	Health = FMath::Max(Health - Damage, 0.f);
	OnHealthChanged.Broadcast(this, Health, Damage);

	if (IsDead())
	{
		OnDeath.Broadcast(this, InstigatedBy);
		if (AutoReviveDelay > 0.f && GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(ReviveTimer, this, &UCEHealthComponent::ResetHealth, AutoReviveDelay, false);
		}
	}
}
