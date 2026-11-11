#include "Interaction/VehicleSeatComponent.h"

#include "Character/PrototypePlayerController.h"
#include "GameFramework/Pawn.h"

UVehicleSeatComponent::UVehicleSeatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ExitOffsets = { FVector(0.f, -220.f, 60.f), FVector(0.f, 220.f, 60.f), FVector(-380.f, 0.f, 60.f), FVector(0.f, 0.f, 260.f) };
}

void UVehicleSeatComponent::Interact_Implementation(APawn* InstigatorPawn)
{
	APawn* Vehicle = Cast<APawn>(GetOwner());
	APrototypePlayerController* PC = InstigatorPawn ? Cast<APrototypePlayerController>(InstigatorPawn->GetController()) : nullptr;
	if (Vehicle && PC && !Vehicle->IsPlayerControlled())
	{
		PC->EnterVehicle(Vehicle, this);
	}
}
