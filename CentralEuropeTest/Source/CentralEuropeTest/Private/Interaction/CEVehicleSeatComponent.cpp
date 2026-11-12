#include "Interaction/CEVehicleSeatComponent.h"

#include "Character/CEPlayerController.h"
#include "GameFramework/Pawn.h"

UCEVehicleSeatComponent::UCEVehicleSeatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ExitOffsets = { FVector(0.f, -220.f, 60.f), FVector(0.f, 220.f, 60.f), FVector(-380.f, 0.f, 60.f), FVector(0.f, 0.f, 260.f) };
}

void UCEVehicleSeatComponent::Interact_Implementation(APawn* InstigatorPawn)
{
	APawn* Vehicle = Cast<APawn>(GetOwner());
	ACEPlayerController* PC = InstigatorPawn ? Cast<ACEPlayerController>(InstigatorPawn->GetController()) : nullptr;
	if (Vehicle && PC && !Vehicle->IsPlayerControlled())
	{
		PC->EnterVehicle(Vehicle, this);
	}
}
