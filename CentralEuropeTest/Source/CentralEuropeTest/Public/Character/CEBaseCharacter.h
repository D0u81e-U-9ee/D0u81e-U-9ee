#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/CEWeaponTypes.h"
#include "CEBaseCharacter.generated.h"

class UCEBlockMannequinComponent;
class UCameraComponent;
class UCECombatComponent;
class UCEHealthComponent;
class USpringArmComponent;
struct FInputActionValue;

/**
 * BP_BaseCharacter parent: Enhanced Input driven third-person character with walk / sprint /
 * crouch / swim / in-vehicle states and an attached UCECombatComponent.
 */
UCLASS(Blueprintable)
class CENTRALEUROPETEST_API ACEBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACEBaseCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void PostInitializeComponents() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCECombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCEHealthComponent> HealthComponent;

	/** Code-built stand-in body, used whenever no skeletal mesh is assigned or found. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mannequin")
	TObjectPtr<UCEBlockMannequinComponent> BlockMannequin;

	/**
	 * When the mesh component has no asset, the first of these that exists in the project is used.
	 * Defaults point at the UE5 Mannequin from the Third Person content pack (Manny / Quinn).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mannequin")
	TArray<FSoftObjectPath> MannequinMeshCandidates;

	/** Anim Blueprint classes tried in order for an auto-assigned mannequin mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mannequin")
	TArray<FSoftObjectPath> MannequinAnimClassCandidates;

	// ---- tuning (cm/s) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion") float WalkSpeed = 250.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion") float SprintSpeed = 550.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion") float AimWalkSpeed = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion") float CrouchSpeed = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion") float SwimSpeed = 300.f;

	/** Multiplier on look input (degrees per mouse count after the engine axis sensitivity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float LookSensitivity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float DefaultFOV = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float AimFOV = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float SniperAimFOV = 22.f;

	/** Draw weapon / ammo / state as on-screen debug text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugHud = true;

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	ECELocomotionState GetLocomotionState() const { return LocomotionState; }

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool IsSprinting() const { return LocomotionState == ECELocomotionState::Sprinting; }

	// Called by the player controller around vehicle possession.
	void OnEnterVehicle(APawn* Vehicle, FName SeatSocket);
	void OnExitVehicle(const FVector& ExitLocation, const FRotator& ExitRotation);

protected:
	virtual void BeginPlay() override;

private:
	void Input_Move(const FInputActionValue& Value);
	void Input_MoveCompleted(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_Crouch(const FInputActionValue& Value);
	void Input_AimStarted(const FInputActionValue& Value);
	void Input_AimCompleted(const FInputActionValue& Value);
	void Input_FireStarted(const FInputActionValue& Value);
	void Input_FireCompleted(const FInputActionValue& Value);
	void Input_Reload(const FInputActionValue& Value);
	void Input_WeaponSlot(const FInputActionValue& Value);
	void Input_JumpStarted(const FInputActionValue& Value);
	void Input_JumpCompleted(const FInputActionValue& Value);

	void UpdateLocomotionState();
	void ConfigureHitCollision();
	void TryAutoAssignMannequin();
	void DrawDebugHud() const;

	ECELocomotionState LocomotionState = ECELocomotionState::Walking;
	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bWantsSprint = false;
	bool bInVehicle = false;
};
