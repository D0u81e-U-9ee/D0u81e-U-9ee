#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/CEInteractable.h"
#include "CEVehicleSeatComponent.generated.h"

/**
 * Add to any drivable Pawn (Chaos wheeled vehicle, boat with buoyancy, ...) to make it enterable
 * with IA_Interact. The pawn keeps its own driving input; this only handles enter / exit.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class CENTRALEUROPETEST_API UCEVehicleSeatComponent : public UActorComponent, public ICEInteractable
{
	GENERATED_BODY()

public:
	UCEVehicleSeatComponent();

	/** Socket on the vehicle's root component the hidden driver is attached to. None = root. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seat")
	FName SeatSocket = NAME_None;

	/** Exit positions in vehicle space, tried in order until one is free. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seat")
	TArray<FVector> ExitOffsets;

	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
};
