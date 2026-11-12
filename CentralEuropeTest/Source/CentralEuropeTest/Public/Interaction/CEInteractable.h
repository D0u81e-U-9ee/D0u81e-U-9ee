#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CEInteractable.generated.h"

class APawn;

UINTERFACE(MinimalAPI, Blueprintable)
class UCEInteractable : public UInterface
{
	GENERATED_BODY()
};

/** BPI_Interactable: implement on an actor or on any of its components (vehicles, boats, loot). */
class CENTRALEUROPETEST_API ICEInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InstigatorPawn);
};
