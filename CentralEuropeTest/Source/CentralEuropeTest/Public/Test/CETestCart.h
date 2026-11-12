#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CETestCart.generated.h"

class UAudioComponent;
class UBoxComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UCEVehicleSeatComponent;
struct FInputActionValue;

/**
 * Stand-in vehicle for the first playtest: NOT a Chaos vehicle.
 * A kinematic, ground-following cart that needs no assets. It exists to exercise the
 * enter / exit flow, mapping-context swap, road traversal and streaming at vehicle speed.
 * Gravity along the slope is modelled, so with bHeavyTruck it crawls on steep grades the way
 * a loaded truck would - useful for checking the 8-10 % road limit before real vehicles exist.
 */
UCLASS(Blueprintable)
class CENTRALEUROPETEST_API ACETestCart : public APawn
{
	GENERATED_BODY()

public:
	ACETestCart();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void UnPossessed() override;

protected:
	virtual void BeginPlay() override;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart") TObjectPtr<UBoxComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart") TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart") TObjectPtr<UCEVehicleSeatComponent> Seat;

	/** Top speed on the flat, cm/s (2500 = 90 km/h). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart") float MaxSpeed = 2500.f;
	/** Drive acceleration at standstill, cm/s^2. Falls off linearly to zero at MaxSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart") float DriveAcceleration = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart") float BrakeDeceleration = 1400.f;
	/** Degrees per second at low speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart") float TurnRate = 70.f;
	/** Steeper ground than this (degrees) cannot be driven onto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart") float MaxClimbAngle = 30.f;
	/** Truck profile: 55 km/h top speed and a quarter of the acceleration (about 0.15 g of drive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart") bool bHeavyTruck = false;

	UFUNCTION(BlueprintPure, Category = "Cart") float GetSpeedKmh() const { return Speed * 0.036f; }

private:
	void Input_Throttle(const FInputActionValue& Value);
	void Input_ThrottleEnd(const FInputActionValue& Value);
	void Input_Steer(const FInputActionValue& Value);
	void Input_SteerEnd(const FInputActionValue& Value);
	void Input_BrakeStart(const FInputActionValue& Value);
	void Input_BrakeEnd(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);

	bool TraceGround(const FVector& At, FHitResult& OutHit) const;

	float Speed = 0.f;          // signed, along the cart's forward axis
	float Throttle = 0.f;
	float Steer = 0.f;
	bool bBraking = false;
	float GradePercent = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> EngineLoop;
};
