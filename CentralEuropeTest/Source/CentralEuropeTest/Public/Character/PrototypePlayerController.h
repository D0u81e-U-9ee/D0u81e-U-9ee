#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PrototypePlayerController.generated.h"

class ABaseCharacter;
class UVehicleSeatComponent;
struct FInputActionValue;

/**
 * Owns IA_Interact so the same key enters and exits vehicles regardless of which pawn is possessed,
 * and swaps the mapping contexts (on foot <-> IMC_Vehicle).
 */
UCLASS()
class CENTRALEUROPETEST_API APrototypePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Reach of the interaction sweep in front of the character (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractDistance = 350.f;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EnterVehicle(APawn* Vehicle, UVehicleSeatComponent* Seat);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ExitVehicle();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInVehicle() const { return DrivenVehicle != nullptr; }

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;

private:
	void BindInteract();
	void Input_Interact(const FInputActionValue& Value);
	void TryInteract();
	FVector FindExitLocation() const;

	UPROPERTY(Transient)
	TObjectPtr<ABaseCharacter> StoredCharacter;

	UPROPERTY(Transient)
	TObjectPtr<APawn> DrivenVehicle;

	UPROPERTY(Transient)
	TObjectPtr<UVehicleSeatComponent> CurrentSeat;

	bool bInteractBound = false;
};
