#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PrototypeInputSubsystem.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Builds the Enhanced Input actions and mapping contexts from the design document in code,
 * so the prototype is playable before any IA_ / IMC_ assets exist. Replace with authored
 * assets later by assigning the same pointers from a data asset.
 *
 *   IMC_Default (priority 0): IA_Move, IA_Look, IA_Jump, IA_Sprint, IA_Crouch, IA_Interact
 *   IMC_Combat  (priority 1): IA_Aim, IA_Fire, IA_Reload, IA_WeaponSlot (Axis1D, value = slot 1-4)
 *   IMC_Vehicle (priority 2): IA_Interact (exit) + throttle / steer / brake / look for the stand-in ATestCart.
 */
UCLASS()
class CENTRALEUROPETEST_API UPrototypeInputSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** IMC_Default + IMC_Combat on, IMC_Vehicle off. */
	void ApplyOnFootContexts();

	/** IMC_Vehicle on, on-foot contexts off. */
	void ApplyVehicleContexts();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Crouch;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Interact;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Aim;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Fire;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_Reload;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_WeaponSlot;

	// Stand-in vehicle (ATestCart). These do not consume their keys, so a Chaos vehicle's own
	// mapping context keeps working alongside IMC_Vehicle.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_VehicleThrottle;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_VehicleSteer;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_VehicleBrake;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> IA_VehicleLook;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputMappingContext> IMC_Default;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputMappingContext> IMC_Combat;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input") TObjectPtr<UInputMappingContext> IMC_Vehicle;

private:
	void BuildDefaults();
};
