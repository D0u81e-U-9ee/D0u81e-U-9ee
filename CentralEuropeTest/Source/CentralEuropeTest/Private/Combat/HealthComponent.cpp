#include "Combat/HealthComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleAnyDamage);
	}
}

void UHealthComponent::ResetHealth()
{
	Health = MaxHealth;
	OnHealthChanged.Broadcast(this, Health, 0.f);
}

void UHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
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
			GetWorld()->GetTimerManager().SetTimer(ReviveTimer, this, &UHealthComponent::ResetHealth, AutoReviveDelay, false);
		}
	}
}
